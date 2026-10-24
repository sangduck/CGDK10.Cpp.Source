#include "pch.h"

using namespace CGDK;

std::size_t net::io::socket::Ntcp::m_s_default_maximum_send_buffer_size = MAX_TCP_SEND_BUFFER_SIZE;
std::size_t net::io::socket::Ntcp::m_default_maximum_receive_buffer_size = MAX_TCP_RECEIVE_BUFFER_SIZE;
#if defined(_WINSOCK2API_)
bool net::io::socket::Ntcp::m_s_enable_explicit_socket_buffer_size = true;		// - windows : SO_SNDBUF/SO_RCVBUF are set like before
#else
bool net::io::socket::Ntcp::m_s_enable_explicit_socket_buffer_size = false;	// - linux : the kernel sizes the buffers (autotuning). enable_explicit_socket_buffer_size(true) sets them
#endif


#if defined(_SYS_EPOLL_H)
intptr_t net::io::socket::Ntcp::process_execute_epoll(intptr_t _event, std::size_t _param)
{
	// 1) clear signaled event flag
	SCOPED_LOCK(this->m_epoll_event_cs)
	{
		// check)
		RETURN_IF(this->socket_state() == eSOCKET_STATE::CLOSED, 0);

		// - 설정되지 않은 flag는 제거한다.
		_event &= this->get_epoll_event_flag();

		// check) epoll event가 처리 중인 상황이라면 event flag만 추가하고 리턴한다.
		if (this->m_epoll_event_processing_now == true)
		{
			// - add events
			this->m_epoll_event_flag_queued |= _event;

			// return) 
			return 0;
		}

		// - 받은 flag 제거
		this->remove_epoll_event_flag(_event);

		// - event 처리 중으로 표시
		this->m_epoll_event_processing_now = true;
	}

	// 1.1) EPOLLERR : the MSG_ZEROCOPY notifications come through the error queue (sendable::Ntcp_async_gather_buffered). handled => not an error
	if ((_event & EPOLLERR) != 0 && this->m_psendable != nullptr && this->m_psendable->process_complete_error_queue())
		_event &= ~static_cast<intptr_t>(EPOLLERR);

	// declare)
	uint32_t flag_epoll_event_added = 0;

	for (;;)
	{
		try
		{
			// 2) EPOLLOUT - event signaled on sendable
			//    - send시 WOULDBLOCK이 뜬 이후 다시 전송하는해졌을 때 뜨는 이벤트다.
			//    - 이럴 경우 버퍼에 큐잉 중인 전송 대기 중 데이터들을 전송처리하게 된다.
			if (_event & EPOLLOUT)
			{
				// check) 
				CGASSERT_ERROR(this->m_psendable != nullptr);

				// check)
				CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0)

				// - call 'process_recover_sendable'
				flag_epoll_event_added |= this->m_psendable->process_recover_sendable(_event, _param);
			}

			// 3) EPOLLIN - 
			//    - 받을 데이터가 존재하거나 접속이 종료되었을 경우 뜨게 된다.
			//    - 접속이 종료된 경우 읽어 낼 수 있는 데이터가 0byte가 된다.
			//      만약 접속이 종료되면 process_complete_receivable에서 접속 종료처리가 호출된다.
			if (_event & EPOLLIN)
			{
				// check) 
				CGASSERT_FATAL(this->m_preceivable_stream != nullptr);

				// - call 'process_complete_receivable'
				auto result = this->m_preceivable_stream->process_complete_receivable(_event, _param);

				if (result != EPOLLHUP)
				{
					flag_epoll_event_added |= result;
				}
				else
				{
					_event |= result;
				}
			}
		}
		catch (...)
		{
		}

		SCOPED_LOCK_NAMED(this->m_epoll_event_cs, cs)
		{
			// 4) queing된 event가 있는지 확인 후 있으면 다시 반복 처리
			if (this->m_epoll_event_flag_queued != 0)
			{
				// - EPOLLIN이나 EPOLLOUT flag는 처릳되었을 것이므로 제거한다.
				_event &= ~(EPOLLIN | EPOLLOUT);

				// - queuing된 event를 추가해 넣는다.(process_close_socket에서 EPOLLHUP이 설정될 수 있다.)
				_event |= this->m_epoll_event_flag_queued;

				// - reset 'm_epoll_event_flag_queued'
				this->m_epoll_event_flag_queued = 0;

				// - remove processed events
				this->remove_epoll_event_flag(_event);

				// continue) EPOLLIN이나 EPOLLOUT이 있으면 다시한번 실행한다.
				CONTINUE_IF((_event & (EPOLLIN | EPOLLOUT)) != 0);
			}

			// 5-1) Neither EPOLLHUP nor EPOLLRDHUP
			if ((_event & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) == 0)
			{
				// check) flag_epoll_event_added는 EPOLLHUP이나 EPOLLREDHUP이 있을 수는 없습니다.
				CGASSERT_ERROR((flag_epoll_event_added & (EPOLLHUP | EPOLLRDHUP)) == 0);

				// - get io_context
				//   (raw pointer : event마다 모든 socket이 공유하는 io_context의 참조 count를 올리고 내리지 않는다.
				//    처리 중(m_epoll_event_processing_now)에는 close가 detach(m_pio_context 해제)를 하지 못하므로 바뀌지 않는다)
				auto pio_context = this->m_pio_context.get();

				// check)
				CGASSERT_ERROR(pio_context != nullptr);

				// - get io_context
				auto epoll_handle = this->get_epoll_handle();

				// check) 
				CGASSERT_ERROR(epoll_handle != INVALID_SOCKET);

				// check) 이미 EPOLLOUT이 설정되어 있는데 신규로 EPOLLOUT의 설정이 될 수는 없다.
				CGASSERT_ERROR(((this->get_epoll_event_flag() & EPOLLOUT) == 0) || ((flag_epoll_event_added & EPOLLOUT) == 0));

				// - set flag
				this->add_epoll_event_flag((EPOLLONESHOT | EPOLLWAKEUP | EPOLLHUP | EPOLLRDHUP | EPOLLERR) | flag_epoll_event_added);

				// - event를 설정한다.
				//   (executable은 attach(또는 접속 완료 시 set_event)로 등록된 m_pexecutable_socket을 그대로 쓴다 : nullptr.
				//    재등록마다 bucket을 unique lock하고 add_ref/release하지 않는다)
				auto result = pio_context->set_event(epoll_handle, this->get_epoll_event_flag(), nullptr);

				// check)
				if (result == false)
				{
					// lock leave)
					SCOPED_LOCK_LEAVE(cs);

					// - fail
					this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;

					// check)
					CGASSERT_ERROR(io_context().exist());

					// - closesocket
					this->process_close_socket();

					// - call 'process_complete_connectable'
					this->m_pconnectable->process_complete_disconnect();

					SCOPED_LOCK(this->m_epoll_event_cs)
					{
						// check)
						CGASSERT_ERROR(this->io_context().empty());

						// check)
						CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

						// check)
						CGASSERT_ERROR(this->get_epoll_handle() == INVALID_SOCKET);

						// check)
						CGASSERT_ERROR(this->native_handle() == INVALID_SOCKET);

						// - reset 'm_epoll_event_processing_now'
						this->m_epoll_event_processing_now = false;
					}

					// break) 
					break;
				}

				// - proceessing_now == false
				this->m_epoll_event_processing_now = false;
			}
			// 5-2) EPOLLHUP | EPOLLRDHUP - HANG UP and READ HANG UP
			else
			{
				// lock leave)
				SCOPED_LOCK_LEAVE(cs);

				// check)
				CGASSERT_ERROR(this->m_pconnectable != nullptr);

				// - if error occurred, add socket reason DISCONNECT_REASON_FAIL 
				if (_event & EPOLLERR)
				{
					this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;
				}

				// check)
				CGASSERT_ERROR(this->io_context().exist());

				// - closesocket
				this->process_close_socket();

				// - call 'process_complete_connectable'
				this->m_pconnectable->process_complete_disconnect();

				SCOPED_LOCK(this->m_epoll_event_cs)
				{
					// check)
					CGASSERT_ERROR(this->io_context().empty());

					// check)
					CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

					// check)
					CGASSERT_ERROR(this->get_epoll_handle() == INVALID_SOCKET);

					// check)
					CGASSERT_ERROR(this->native_handle() == INVALID_SOCKET);

					// - reset 'm_epoll_event_processing_now'
					this->m_epoll_event_processing_now = false;
				}
			}
		}

		// for(;;) 탈출
		break;
	}

	// return)
	return 0;
}

bool net::io::socket::Ntcp::process_is_io_completion_supported() const noexcept
{
	// - the receivable (receivable::Nstream) and the sendable (sendable::Ntcp_async_gather) of the socket do their I/O with the ops
	return this->m_preceivable_stream != nullptr && this->m_preceivable_stream->is_io_completion_supported()
		&& this->m_psendable != nullptr && this->m_psendable->is_io_completion_supported();
}

bool net::io::socket::Ntcp::process_start_io_completion() noexcept
{
	// check)
	CGASSERT_ERROR(this->m_preceivable_stream != nullptr);

	// 1) the first RECV op
	return this->m_preceivable_stream->process_start_receive();
}
#endif

net::io::socket::Ntcp::Ntcp() noexcept :
	Ntcp(SOCKET_DEFAULT_FLAG)
{
	// statistics)
	this->statistics_set_protocol(IPPROTO_TCP);

	// 1) set default value
	this->maximum_send_buffer_size(m_s_default_maximum_send_buffer_size);
	this->maximum_receive_buffer_size(m_default_maximum_receive_buffer_size);
}

net::io::socket::Ntcp::Ntcp(uint32_t _flag) noexcept : 
	m_flag(_flag)
{
	// statistics)
	this->statistics_set_protocol(IPPROTO_TCP);

	// 1) set default value
	this->maximum_send_buffer_size(m_s_default_maximum_send_buffer_size);
	this->maximum_receive_buffer_size(m_default_maximum_receive_buffer_size);
}

net::io::socket::Ntcp::~Ntcp() noexcept
{
#if defined(_WINSOCK2API_)
	// 1) 소켓핸들을 얻는다.
	auto handle_socket = this->native_handle(INVALID_SOCKET);

	// check)
	if (handle_socket != INVALID_SOCKET)
	{
		api::socket::abortive_closesocket(handle_socket);
	}
#elif defined(_SYS_EPOLL_H)
	// 1) reset epoll_handle
	auto epoll_socket = this->detach_epoll_handle();

	// check)
	if (epoll_socket != INVALID_SOCKET)
	{
		api::socket::abortive_closesocket(epoll_socket);
	}

	// 3) reeset 
	this->process_reset_executable_socket();
#endif
}

void net::io::socket::Ntcp::process_create_socket(ADDRESS_FAMILY _address_family)
{
	// check) _address_family가 AF_UNSPEC이어서는 안된다.
	CGASSERT_ERROR(_address_family != AF_UNSPEC);
	CGASSERT_ERROR(_address_family == AF_INET || _address_family == AF_INET6 || _address_family == AF_UNIX);

	// 1) 새로운 소켓을 생성해 소켓핸들을 얻는다.
#if defined(_WINSOCK2API_)
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP, this->m_flag);
#elif defined(_SYS_EPOLL_H)
	//auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP, this->m_flag);
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, 0, this->m_flag);
#else
	#error	"[CGDK10] Not supported Platform"
#endif

	// check) 새로운 소켓의 생성에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// - set last error
		this->m_last_socket_error = api::get_last_error();

		// log)
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to creating socket. error:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// raise) 
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), CGDK::stream_string("fail to creating socket handle errcode:", api::get_last_error()));
	}

	// 2) set new socket handle
	auto handle_socket_old = native_handle(handle_socket_new);

	// 3) close old socket handle
	if(handle_socket_old != INVALID_SOCKET)
	{
#if defined(_SYS_EPOLL_H)
		// check) io_context must exist
		CGASSERT_ERROR(io_context().exist());

		// - detach socket from epoll
		this->io_context()->detach((CGHANDLE)handle_socket_old);
#endif
		// - closesocket
		api::socket::closesocket(handle_socket_old);
	}

	// 4) set address family
	this->address_family(_address_family);

	// 5) set socket option (linux : nonblocking already. create_socket() uses SOCK_NONBLOCK)
#if defined(_WINSOCK2API_)
	api::socket::set_nonblock(handle_socket_new, true);
#endif

	// 6) get_io_context
	auto pio_context = this->io_context();

	// 7) set default io_context if empty
	if (pio_context.empty())
	{
		// - get system io_context
		pio_context = system_executor::executor();

		// - set io_context
		this->io_context(pio_context);

		// check) 
		CGASSERT_ERROR(pio_context.exist());
	}

#if defined(_WINSOCK2API_)
	// 8)  attach to io_context
	auto result = pio_context->attach((CGHANDLE)handle_socket_new);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL, 0, "fail to attaching TCP socket to io_context"sv), LOG_DEBUG << "(err ) CGDK.net.socket: fail to attaching tcp socket to io_context " __CGFUNCTION__ ""sv);
#elif defined(_SYS_EPOLL_H)
	// 8) prepare executable socket
	this->process_prepare_executable_socket();
#endif
}

void net::io::socket::Ntcp::process_dispose_socket() noexcept
{
	// 1) clear sendable
	if (this->m_psendable != nullptr)
	{
		this->m_psendable->process_reset_sendable();
	}
}

bool net::io::socket::Ntcp::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	// statistics) 통계처리
	this->statistics_check_error_disconnect();

	// - send defer : 모아 둔 송신(그룹 락 안 / send cork)이 있으면 닫기 전에 보낸다. (다른 thread가 모아 둔 것도)
	//   (그룹 락 안에서 send한 뒤 closesocket하거나, 다른 thread가 그룹 락 안에서 보낸 직후 closesocket해도 보낸 것이 먼저 간다)
	if (this->m_psendable != nullptr)
	{
		this->m_psendable->process_flush_deferred_send_on_close();
	}

	// 1) 먼저 socket_handle을 INVALID_SOCKET으로 바꾸고 이전 socket_handle을 받아온다.
	//    - 송신 lock 안에서 바꾼다. 송신은 같은 lock 안에서 handle을 읽고 send까지 하므로
	//      바꾸기 전에 읽은 송신은 send가 끝났고, 바꾼 뒤의 송신은 INVALID_SOCKET을 본다.
	//      (닫은 뒤 같은 handle 값으로 다시 만들어진 다른 socket에 보내지 않는다)
	//      (windows : iocp 송신. epoll은 송신 lock 밖에서 send하므로 잡지 않는다)
	CGSOCKET handle_socket;
#if defined(_WINSOCK2API_)
	if (auto plockable = (this->m_psendable != nullptr) ? this->m_psendable->process_get_lockable_sending() : nullptr; plockable != nullptr)
	{
		std::lock_guard lock(*plockable);
		std::lock_guard lock_receiving(this->get_lockable_receiving());	// - 수신도 같은 방식 (receive : _call_receiving)
		handle_socket = this->native_handle(INVALID_SOCKET);
	}
	else
	{
		std::lock_guard lock_receiving(this->get_lockable_receiving());
		handle_socket = this->native_handle(INVALID_SOCKET);
	}
#else
	// - io_uring completion : like windows (the ops are posted under the locks). is_receiving_op : a RECV op is in flight (its
	//   completion completes the disconnect of the closed socket. none : completed below)
	bool is_receiving_op = false;

	if (this->is_io_completion())
	{
		if (auto plockable = (this->m_psendable != nullptr) ? this->m_psendable->process_get_lockable_sending() : nullptr; plockable != nullptr)
		{
			std::lock_guard lock(*plockable);
			std::lock_guard lock_receiving(this->get_lockable_receiving());
			handle_socket = this->native_handle(INVALID_SOCKET);
			is_receiving_op = this->m_is_receiving_op;
		}
		else
		{
			std::lock_guard lock_receiving(this->get_lockable_receiving());
			handle_socket = this->native_handle(INVALID_SOCKET);
			is_receiving_op = this->m_is_receiving_op;
		}
	}
	else
	{
		handle_socket = this->native_handle(INVALID_SOCKET);
	}
#endif

	// check) 기존 설정되어 있던 소켓핸들이 INVALID_SOCKET면 그냥 끝낸다.
	RETURN_IF(handle_socket == INVALID_SOCKET, false);

	// 2) 접속종료 옵션을 얻는다.
	this->m_disconnect_reason |= _disconnect_reason;

	// 3) on_closesocket함수를 호출한다.
	this->on_closesocket(m_disconnect_reason);

#if defined(_WINSOCK2API_)
	// declare) 
	bool result;

	// 4.A) abortive disconnect
	if (_disconnect_reason & (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE))
	{
		result = api::socket::abortive_closesocket(handle_socket);
	}
	// 4.B) graceful disconnect
	else
	{
		result = api::socket::graceful_closesocket(handle_socket);
	}

	// 5) reset io_context
	this->reset_io_context();

#elif defined(_SYS_EPOLL_H)
	// declare) 
	bool result = true;

	// - the state is read once : a connect completed by another thread between two reads (SYN, then ESTABLISHED) took none of
	//   the branches below (the handle taken, the fd never closed, no disconnect : stop() waited 30s and the peer stayed connected).
	//   SYN here goes to 4.B, whose process_complete_connecting(EPOLLHUP) completes the disconnect of a socket connected meanwhile
	const auto socket_state_close = this->socket_state();

	// 4.A) 접속 중인 경우 => epoll은 recv 처리 이후에 처리되록 하기 위하 shutdown을 무조건 먼저 건다.
	//      - EPOLLRDHUP 신호가 뜨면 그 이후에 접속해제한다.
	//      - PASSIVE close일 경우 바로 호출되는 process_complete_disconnect()에서 접속 종료 처리된다.
	if (socket_state_close > eSOCKET_STATE::SYN)
	{
		// - hold를 한다.(disconnect 처리로 객체가 소멸되버릴 수도 있으므로...)
		object_ptr<net::io::socket::Ntcp> phold(this);

		// - shutdown
		api::socket::shutdown(handle_socket, SD_SEND);

		// - io_uring completion : the ops of the handle are cancelled (they complete with -ECANCELED through their executables) and
		//   the handle is closed now (an op holds the file : closing right after the cancel is safe. the fd number may be reused at
		//   once : nothing of this socket refers to it any more). the disconnect is completed by the RECV op in flight (-ECANCELED,
		//   or the bytes received and the receive() after that fails on the closed handle). none in flight => below, like epoll
		if (this->is_io_completion())
		{
			if (auto* pio_context = this->m_pio_context.get(); pio_context != nullptr)
				pio_context->submit_cancel_fd(static_cast<CGHANDLE>(handle_socket));

			if (this->get_epoll_handle() == static_cast<CGHANDLE>(handle_socket))
				this->reset_epoll_handle();

			if (this->m_disconnect_reason & (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE))
				api::socket::abortive_closesocket(handle_socket);
			else
				api::socket::graceful_closesocket(handle_socket);
		}

		SCOPED_LOCK(this->m_epoll_event_cs)
		{
			// - 처리 중 => flag만 설정해 준다.
			if (this->m_epoll_event_processing_now == true)
			{
				// - flag만 설정해준다.
				this->m_epoll_event_flag_queued |= EPOLLHUP;

				// return) 끝
				return result;
			}

			// - io_uring completion : a RECV op in flight completes the disconnect
			if (is_receiving_op)
				return result;

			// - 처리 중이 아닐 때 => 즉시 종료 처리한다.
			this->m_epoll_event_processing_now = true;
		}

		// check) 
		CGASSERT_ERROR(this->m_pconnectable != nullptr);

		// - call 'process_complete_connectable'
		this->m_pconnectable->process_complete_disconnect();

		SCOPED_LOCK(this->m_epoll_event_cs)
		{
			this->m_epoll_event_processing_now = false;
		}
	}
	// 4.B) 접속시도중인 경우 => 바로 닫는다.
	else if (socket_state_close == eSOCKET_STATE::SYN)
	{
		// - detach from epoll first (process_complete_connecting can't : the handle is INVALID_SOCKET already)
		//   a closed fd is reused at once : the registration left stops the next connect
		if (auto pio_context = this->io_context(); pio_context.exist())
		{
			pio_context->detach(static_cast<CGHANDLE>(handle_socket));

			// - io_uring completion : the CONNECT op in flight is cancelled (it completes with -ECANCELED : dropped by the sequence or the state)
			if (pio_context->backend() == executor::io_backend::io_uring_completion)
				pio_context->submit_cancel_fd(static_cast<CGHANDLE>(handle_socket));
		}

		// - reset the epoll handle too (the destructor detaches and closes it : the closed fd may be another socket's by then)
		if (this->get_epoll_handle() == static_cast<CGHANDLE>(handle_socket))
			this->reset_epoll_handle();

		// - closesocket
		api::socket::abortive_closesocket(handle_socket);

		// - get connect_requestable
		auto pconnect_requestable = dynamic_cast<net::io::Iconnect_requestable*>(this);

		// - complete connecting (being processed by another thread => the close is queued and that thread completes the disconnect)
		if (pconnect_requestable != nullptr)
		{
			pconnect_requestable->process_complete_connecting(EPOLLHUP);
		}
	}
	// 4.C) bind만 되거나 접속 종료된 상태일 경우( 접속 중이거나 접속 시도 중이 아닌 경우)
	else if (socket_state_close <= eSOCKET_STATE::BINDED)
	{
		// - reset the epoll handle too (same as above)
		if (this->get_epoll_handle() == static_cast<CGHANDLE>(handle_socket))
			this->reset_epoll_handle();

		// - closesocket
		api::socket::abortive_closesocket(handle_socket);
	}
#endif

	// return)
	return result;
}

void net::io::socket::Ntcp::set_default_maximum_send_buffer_size(std::size_t _size) noexcept
{
	// 1) saturate max
	if (_size > MAX_TCP_SEND_BUFFER_SIZE)
	{
		_size = MAX_TCP_SEND_BUFFER_SIZE;
	}

	// check) return if same
	RETURN_IF(_size == net::io::socket::Ntcp::m_s_default_maximum_send_buffer_size);

	// 2) set value
	net::io::socket::Ntcp::m_s_default_maximum_send_buffer_size = _size;
}

void net::io::socket::Ntcp::set_default_maximum_receive_buffer_size(std::size_t _size) noexcept
{
	// 1) saturate max
	if (_size > MAX_TCP_RECEIVE_BUFFER_SIZE)
	{
		_size = MAX_TCP_RECEIVE_BUFFER_SIZE;
	}

	if (_size < MIN_TCP_RECEIVE_BUFFER_SIZE)
	{
		_size = MIN_TCP_RECEIVE_BUFFER_SIZE;
	}

	// check)
	RETURN_IF(_size == net::io::socket::Ntcp::m_default_maximum_receive_buffer_size);

	// 2) set value
	net::io::socket::Ntcp::m_default_maximum_receive_buffer_size = _size;
}
