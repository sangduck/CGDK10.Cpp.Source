#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include <climits>

// - sendmsg의 최대 buffer 수
#if defined(IOV_MAX)
	static constexpr std::size_t CGDK_SEND_MAX_IOV = IOV_MAX;
#else
	static constexpr std::size_t CGDK_SEND_MAX_IOV = 1024;
#endif

// - _process_send_pending이 sendmsg 한번에 넘기는 최대 buffer 수 (iovec 배열은 stack에 둔다)
static constexpr std::size_t SEND_PENDING_MAX_IOV = (CGDK_SEND_MAX_IOV < 256) ? CGDK_SEND_MAX_IOV : 256;

using namespace CGDK;

// - send cork : 이 thread가 처음 모은 시간 (가장 오래 기다린 송신)
static thread_local chrono::tick::time_point t_tick_corked;

// - send cork : 오래 기다렸는지는 시각을 읽어 확인한다. (작은 message를 많이 보낼 때 송신마다 읽으면 그 비용이 크다)
//   확인할 때마다 직전 간격의 송신 속도로 다음 간격(송신 수)을 정한다 : SEND_CORK_DURATION/8 (남은 시간이 더 짧으면 남은 시간) 동안의 송신 수.
//   (빠른 송신은 SEND_CORK_CHECK_MAX번마다, 오래 걸리는 처리 사이의 송신은 매번 확인한다. 모으기 시작하면 다음 송신부터 확인한다)
static thread_local chrono::tick::time_point t_tick_cork_checked;
static thread_local std::size_t t_count_cork_interval = 1;	// - 확인 간격 (송신 수)
static thread_local std::size_t t_count_cork_check = 1;		// - 다음 확인까지 남은 송신 수

static void _cork_begin() noexcept
{
	const auto tick_now = chrono::tick::now();

	t_tick_corked = tick_now;
	t_tick_cork_checked = tick_now;
	t_count_cork_interval = 1;
	t_count_cork_check = 1;
}

static bool _is_cork_expired(chrono::tick::duration _duration, std::size_t _count_check_max) noexcept
{
	// check) 확인할 차례가 아니다.
	if (t_count_cork_check > 1)
	{
		--t_count_cork_check;
		return false;
	}

	// 1) 가장 먼저 모은 것이 기다린 시간
	const auto tick_now = chrono::tick::now();
	const auto tick_waited = tick_now - t_tick_corked;

	// check) 오래 기다렸다.
	if (tick_waited >= _duration)
		return true;

	// 2) 다음 확인 간격 = 지금 간격 x (min(_duration/8, 남은 시간) / 지금 간격에 걸린 시간) (1 ~ _count_check_max)
	const auto ticks_budget = static_cast<int64_t>((std::min)(_duration / 8, _duration - tick_waited).count());
	const auto ticks_elapsed = static_cast<int64_t>((tick_now - t_tick_cork_checked).count());
	const auto count_interval = static_cast<int64_t>(t_count_cork_interval);
	const auto count_max = static_cast<int64_t>(_count_check_max);

	auto count_next = (ticks_elapsed * count_max <= count_interval * ticks_budget) ? count_max : (count_interval * ticks_budget / ticks_elapsed);
	if (count_next < 1)
		count_next = 1;

	t_tick_cork_checked = tick_now;
	t_count_cork_interval = static_cast<std::size_t>(count_next);
	t_count_cork_check = t_count_cork_interval;

	// return)
	return false;
}


void net::io::sendable::Ntcp_async_gather_buffered::QUEUE_SEND::clear() noexcept
{
	// 1) c;ear array node
	this->array_node.clear();
	this->bytes_data = 0;
}

void net::io::sendable::Ntcp_async_gather_buffered::QUEUE_SEND::push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	// declare)
	auto bytes_requested = _buffer.size();

	// - BYTES_SEND_GATHER_BUFFER_MIN보다 작으면 복사를 한다.
	if (this->array_node.size() > COUNT_SEND_GATHER_START_DEPTH && bytes_requested <= BYTES_SEND_GATHER_BUFFER_MIN)
	{
		// check) 
		CGASSERT_ERROR(this->array_node.empty() == false);

		// - is buffer enough to copy (create new)
		if (this->array_node.back().bytes_remained < bytes_requested)
		{
			// - shift count
			auto shift_count = static_cast<uint32_t>(this->array_node.size() - COUNT_SEND_GATHER_START_DEPTH - 1) * 2;

			// - saturate_max
			SATURATE_MAX(shift_count, BYTES_SEND_GATHER_SHIFT_MAX);

			// - get buffer size
			auto size_buffer = (BYTES_SEND_GATHER_BUFFER_MIN << shift_count);

			// check)
			CGASSERT_ERROR(size_buffer <= BYTES_SEND_GATHER_BUFFER_MAX);

			// - alloc new buffer
			auto buf_new = alloc_shared_buffer(size_buffer);

			// - queue allocated buffer
			this->array_node.push_back(NODE{ buf_new, buf_new.get_source(), 0, 0, size_buffer });
		}

		// - get end...
		auto& data_node = this->array_node.back();

		// check)
		CGASSERT_ERROR(data_node.bytes_remained >= bytes_requested);

		// - copy~
		memcpy(data_node.buf_send.data() + data_node.buf_send.size(), _buffer.data(), bytes_requested);
		data_node.buf_send.add_size(bytes_requested);
		data_node.bytes_data += bytes_requested;
		data_node.count_message += _count_messages;
		data_node.bytes_remained -= bytes_requested;
		this->bytes_data += bytes_requested;
	}
	// - BYTES_SEND_GATHER_BUFFER_MIN보다 크면 버퍼 통째로 추가한다.
	else
	{
		// - check 
		if (this->array_node.empty() == false)
		{
			// - get back
			auto& data_back = this->array_node.back();

			if (data_back.powner == _powner && data_back.buf_send.get_back_ptr() == _buffer.data())
			{
				// - expand only
				data_back.buf_send.add_size(bytes_requested);
				data_back.bytes_data += bytes_requested;
				data_back.count_message += _count_messages;
				this->bytes_data += bytes_requested;

				// return) 
				return;
			}
		}

		// - ...
		this->array_node.push_back(NODE{ _buffer, std::move(_powner), bytes_requested, _count_messages, 0 });
		this->bytes_data += bytes_requested;
	}
}

bool net::io::sendable::Ntcp_async_gather_buffered::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT_ERROR(_buffer.size() != 0);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// declare)
	CGSOCKET handle_socket;

	// - send defer : 이 thread가 그룹 락 안에서 보내거나(send_defer) 이 socket이 받은 message를 처리하는 중이다(send cork).
	//   (바로 보내지 않고 m_queue_pending에 모아 두었다가 한번에 보낸다)
	const auto ptoken = send_defer::token();
	const bool is_deferring = send_defer::is_deferring(static_cast<const Isocket*>(this));

	// declare)
	bool is_defer_queued = false;
	bool is_defer_first = false;
	bool is_defer_full = false;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
			return false;

		// 1) get socket native handle
		handle_socket = this->native_handle();

		// check) if invalid socket, return false 
		if(handle_socket == INVALID_SOCKET)
			return false;

		// check) m_flag_sending이 signal된 상태(true)라면 보내고 있는 thread가 있다. (queue에 넣기만 한다 : 보내는 thread가 이어서 보낸다)
		//        (send defer로 이 thread가 모으고 있는 중일 수도 있다)
		if (this->m_flag_sending == true)
		{
			// case) send defer : 이 thread가 모으고 있다. (이 thread가 보내는 thread다)
			//       - 모은 것과 그 사이 다른 thread가 보낸 것은 m_queue_pending에 순서대로 있다. 그 뒤에 넣는다.
			if (this->m_pdefer_owner.load(std::memory_order_relaxed) == ptoken)
			{
				// - queuing to pending_queue (bytes_data는 push_back이 더한다)
				this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

				// - 모은 것이 많은가? (큰 message를 넣었으면 바로 보낸다)
				is_defer_queued = true;
				is_defer_full = this->_is_send_defer_full();
			}
			else
			{
				// check) queue overflowed? (앞에 되돌려 둔 보내던 것은 세지 않는다 : EPOLLOUT을 기다리며 남은 것. iocp의 WSASend 중인 것과 같다)
				if ((this->m_queue_pending.array_node.size() - this->m_count_sending_parked >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data - this->m_bytes_sending_parked >= m_bytes_max_queued) )
				{
					// statistics)
					this->statistics_on_error_send();

					// check)
					this->process_on_send_overflow();

					// return)
					return false;
				}

				// - queuing to pending_queue (bytes_data는 push_back이 더한다)
				this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

				// return) 보내는 thread가 보낸다.
				return true;
			}
		}
		else
		{
			// - send defer : 바로 보내지 않고 m_queue_pending에 넣어 둔다. (처음 모은다 : 이 thread가 보내는 thread가 된다)
			//   - 그 사이 다른 thread가 보내는 것은 m_flag_sending을 보고 그 뒤에 쌓는다. (순서 유지)
			//   - 한번에 모을 크기보다 큰 message는 모으지 않는다. (바로 보낸다)
			if (is_deferring && _buffer.size() < SEND_DEFER_MAX_BYTES)
			{
				// check) 보내는 thread가 없으면 m_queue_pending은 비어 있다.
				CGASSERT_ERROR(this->m_queue_pending.array_node.empty());

				// - queuing to pending_queue
				this->m_queue_pending.push_back(_buffer, std::move(_powner), _count_messages);

				// - 이 thread가 모은다. (등록은 lock 밖에서) 모은 것이 많은가? (queue 한도가 아주 작을 때)
				this->m_pdefer_owner.store(ptoken, std::memory_order_relaxed);
				is_defer_queued = true;
				is_defer_first = true;
				is_defer_full = this->_is_send_defer_full();
			}

			// - set flag_sending
			this->m_flag_sending = true;
		}
	}

	// case) send defer (그룹 락 안 / send cork)
	if (is_defer_queued)
	{
		// - 처음 모았다 => 등록한다. (가장 바깥 그룹 락이 풀릴 때 / send_cork의 범위가 끝날 때 process_flush_deferred_send()로 보낸다)
		//   등록에 실패하면 바로 보낸다.
		if (is_defer_first)
		{
			if (send_defer::push(this, object_ptr<Ireferenceable>(this)) == false)
				is_defer_full = true;

			if (send_defer::is_cork_age_checked())
				_cork_begin();
		}
		// - send cork : 가장 먼저 모은 것이 오래 기다렸다 => 지금 보낸다. (그룹 락 안에서는 그룹 락이 풀릴 때 보낸다)
		else if (is_defer_full == false && send_defer::is_cork_age_checked() && _is_cork_expired(SEND_CORK_DURATION, SEND_CORK_CHECK_MAX))
		{
			is_defer_full = true;
		}

		// - 모은 것이 많거나 오래됐으면 지금 보낸다.
		if (is_defer_full)
		{
			this->process_flush_deferred_send();
		}

		// return)
		return true;
	}

	// return)
	return this->process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_messages);
}

bool net::io::sendable::Ntcp_async_gather_buffered::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	//-----------------------------------------------------------------
	// 설명) 보내는 thread(m_flag_sending == true)가 된 thread가 바로 보낸다. (process_sendable이 부른다)
	//
	//  - 다 보냈으면 그 사이 다른 thread가 m_queue_pending에 쌓은 것을 이어서 보낸다. (_process_send_pending)
	//    queue가 빌 때까지 보내고 m_flag_sending을 푼다. (EPOLLOUT을 걸지 않는다)
	//  - socket 송신 버퍼가 찼을 때(일부만 보냈거나 EWOULDBLOCK)만 남은 것을 m_queue_pending 앞에 넣고
	//    EPOLLOUT을 기다린다. (process_recover_sendable이 보낸다)
	//-----------------------------------------------------------------

	// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
	CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

	// check)
	CGASSERT_ERROR(_buffer.size() != 0);

	// declare)
	const auto bytes_requested = _buffer.size();
	ssize_t bytes_sent;

	// - MSG_ZEROCOPY for a large buffer (the io_uring backend : the owner is held until the kernel notifies the completion)
	int flags_send = MSG_DONTWAIT | MSG_NOSIGNAL | this->_zerocopy_send_flags(_handle_socket, bytes_requested, _powner);

	// - io_uring completion : a large buffer goes as a SEND_ZC op (not sent directly : put back like a full socket buffer, the op is posted below)
	const bool is_zerocopy_op = this->is_io_completion() && this->_is_zerocopy_op(bytes_requested, _powner);

	// 1) send at once (EINTR : again)
	for (bytes_sent = 0; is_zerocopy_op == false;)
	{
		bytes_sent = this->_zerocopy_send(_handle_socket, _buffer.data(), bytes_requested, flags_send);

		if (bytes_sent >= 0 || api::get_last_error() != EINTR)
			break;
	}

	// case) send completed~ => 그 사이 m_queue_pending에 쌓인 것을 이어서 보낸다.
	if (bytes_sent == static_cast<ssize_t>(bytes_requested))
	{
		// - the buffer is not used any more (zerocopy : held until the kernel notifies)
		if ((flags_send & MSG_ZEROCOPY) != 0)
			this->_zerocopy_hold(_buffer, std::move(_powner));
		else
		{
			_powner.reset();
		}

		// - queue가 빌 때까지 보낸다. (보낸 것의 통계는 같이 한다. 다 보냈으면 m_flag_sending을 풀고 SEND_DISCONNECTED면 shutdown한다)
		const auto result = this->_process_send_pending(_handle_socket, bytes_requested, _count_messages);

		// - socket 송신 버퍼가 찼다 => EPOLLOUT을 기다린다. (남은 것은 m_queue_pending에 있다)
		if (result == EPOLLOUT)
		{
			SCOPED_LOCK(this->m_lockable_sending)
			{
				// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
				CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

				// statistics)
				this->statistics_on_increase_async_sending();

				// - set EPOLLOUT event flag (io_uring completion : the SEND op)
				this->_process_post_send_wait();
			}
		}

		// return) 이 buffer는 보냈다. (이어 보내다 실패했으면 접속은 끊었다)
		return true;
	}

	// case) fail to send (EWOULDBLOCK이면 EPOLLOUT을 기다린다)
	if (bytes_sent < 0)
	{
		// - get last error
		const auto error_code = api::get_last_error();

		if (error_code != EWOULDBLOCK && error_code != EINPROGRESS)	// - EINPROGRESS : a TCP Fast Open connect (TCP_FASTOPEN_CONNECT) without a cookie. the SYN is sent, the data waits for EPOLLOUT
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics)
			this->statistics_on_error_send();

			// - closesocket
			this->process_close_socket(DISCONNECT_REASON_FAIL);

			// - clear send queue (m_flag_sending = false)
			SCOPED_LOCK(this->m_lockable_sending)
			{
				this->_process_clear_send_buffers();
			}

			// return) 전송에 실패했다.
			return false;
		}
	}

	// 2) 일부만 보냈거나 EWOULDBLOCK => socket 송신 버퍼가 찼다. (바로 다시 보내도 EWOULDBLOCK이다)
	//    남은 것을 m_queue_pending 앞에 넣고 EPOLLOUT을 기다린다. (그 사이 다른 thread가 쌓은 것은 그 뒤에 있다 : 순서 유지)
	auto buf_remained = _buffer;

	if (bytes_sent > 0)
	{
		// check)
		CGASSERT_ERROR(static_cast<std::size_t>(bytes_sent) < bytes_requested);

		// - zerocopy : the part sent is held until the kernel notifies (the rest below shares the owner)
		if ((flags_send & MSG_ZEROCOPY) != 0)
			this->_zerocopy_hold(buffer_view{ _buffer.data(), static_cast<std::size_t>(bytes_sent) }, object_ptr<Ireferenceable>(_powner));

		// - 전송한 만큼 이동시킨다.
		buf_remained += offset(static_cast<int64_t>(bytes_sent));
	}

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - queuing to head (node의 bytes_data는 통계용 요청 bytes, queue의 bytes_data는 남은 bytes)
		this->m_queue_pending.array_node.push_front(QUEUE_SEND::NODE{ buf_remained, std::move(_powner), bytes_requested, _count_messages, 0 });
		this->m_queue_pending.bytes_data += buf_remained.size();

		// - EPOLLOUT을 기다리는 것 (queue 한도에는 세지 않는다)
		this->m_count_sending_parked = 1;
		this->m_bytes_sending_parked = buf_remained.size();

		// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// statistics)
		this->statistics_on_increase_async_sending();

		// - set EPOLLOUT event flag (io_uring completion : the SEND op)
		this->_process_post_send_wait();
	}

	// return)
	return true;
}

uint32_t net::io::sendable::Ntcp_async_gather_buffered::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	// declare) 
	container_t<QUEUE_SEND::NODE> queue_send;
	
	// 1) get queued SENDING_INFO
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 
		// 설명) EPOLLOUT 이벤트는 중복 실행될 수 있으므로 적절한 처리가 필요하다.
		//  결론을 말하면 m_queue_pending.empty() 면 바로 리턴한다. m_flag_sending는 true일수도 있고 false일 수도 있다.
		// 
		//   EPOLL 이벤트가 중복되었을 때 증상을 정리하자면
		// 
		//    case 1) EPOLLOUT 이벤트의 처리상 문제로 실행도중 중복 진입한 경우.
		//            실행도중 다시 진입한 경우 m_flag_sending == true이고 m_queue_pending은 empty()가 된다.
		//             => 이럴 경우 그냥 return하면 된다.
		//
		//    case 2) EPOLLOUT 이벤트가 처리되고 난 다음 중복 진입한 경우.
		//            모두 처리하고 나간 상태이기 때문에 m_flag_sending == false가 되고 m_queue_pending는 empty() 상태가 된다.
		//             => 이런 경욷도 그냥 return만 하면 된다.
		//
		//  중복 이벤트가 발생하더라도 절대로 발생해서는 안 되는 상황으로 설명하자면...
		//
		//    case 1) m_queue_pending가 empty()가 아닌데 m_flag_sending가 false인 상황
		//    case 1) m_queue_sending이 empty()가 아닌 상황. 
		//            m_queue_sending는 메모리 중복 할당을 피하기 위한 단순 버퍼의 역할만 한다.따라서 항상 empty상태여야 한다.
		//
	
		// check)
		if (this->m_queue_pending.array_node.empty())
		{
			// statistics) 
			this->statistics_on_decrease_async_sending();

			// check)
			CGASSERT_ERROR(this->m_queue_pending.bytes_data == 0);
	
			// - set flag
			this->m_flag_sending = false;
	
			// return)
			return 0;
		}
	
		// check) 
		CGASSERT_ERROR(this->m_flag_sending == true);
	
		// check)
		CGASSERT_ERROR(this->m_queue_sending.array_node.empty() == true);
	
		// - swap data
		this->m_queue_sending.array_node.swap(this->m_queue_pending.array_node);
		this->m_queue_pending.bytes_data = 0;

		// - 이 thread가 보낸다. (EPOLLOUT을 기다리던 것도 가져갔다)
		this->m_count_sending_parked = 0;
		this->m_bytes_sending_parked = 0;

		// - sendmsg는 한번에 IOV_MAX(linux: 1024)개의 buffer까지만 보낼 수 있다. (넘으면 sendmsg가 실패한다.)
		//   넘는 buffer는 m_queue_pending으로 되돌려 다음 EPOLLOUT에서 보낸다. (순서는 유지된다.)
		if (this->m_queue_sending.array_node.size() > CGDK_SEND_MAX_IOV)
		{
			auto iter_split = std::next(this->m_queue_sending.array_node.begin(), CGDK_SEND_MAX_IOV);
			this->m_queue_pending.array_node.splice(this->m_queue_pending.array_node.end(), this->m_queue_sending.array_node, iter_split, this->m_queue_sending.array_node.end());

			for (const auto& iter : this->m_queue_pending.array_node)
				this->m_queue_pending.bytes_data += iter.buf_send.size();
		}

		queue_send.swap(this->m_queue_sending.array_node);
	}

	// declare) 
	msghdr msg_send;

	// 2) prepare ...
	// - alloc iovec buffer
	shared_buffer buf_msg_iov = alloc_shared_buffer(sizeof(iovec) * queue_send.size());

	// - store ...
	std::size_t bytes_sending = 0;
	iovec* piter_dest = buf_msg_iov.data<iovec>();
	for (auto iter : queue_send)
	{
		piter_dest->iov_base = iter.buf_send.data();
		piter_dest->iov_len = iter.buf_send.size();
		bytes_sending += iter.buf_send.size();

		++piter_dest;
	}

	// - set buffer		
	msg_send.msg_name = nullptr;
	msg_send.msg_namelen = 0;
	msg_send.msg_iov = buf_msg_iov.data<iovec>();
	msg_send.msg_iovlen = queue_send.size();
	msg_send.msg_control = nullptr;
	msg_send.msg_controllen = 0;
	msg_send.msg_flags = 0;

	// declare)
	bool is_completed = false;

	for (;;)
	{
		// check)
		CGASSERT_ERROR(bytes_sending != 0);

		// 3) send 한다.
		auto bytes_result = ::sendmsg(this->native_handle(), &msg_send, MSG_DONTWAIT | MSG_NOSIGNAL);

		// 4-1) [case] send completed~
		if (bytes_result == static_cast<int>(bytes_sending))
		{
			// - process sent buffer
			for (auto iter = queue_send.begin(), iter_end = queue_send.end(); iter != iter_end; ++iter)
				this->process_complete_sendable(0, iter->bytes_data, iter->count_message);

			// - clear and swap buffer
			queue_send.clear();
			queue_send.swap(this->m_queue_sending.array_node);

			// - 가져온 것은 다 보냈다. (SEND_DISCONNECTED면 m_queue_pending도 비었을 때 shutdown한다 : 5)
			is_completed = true;

			// break)
			break;
		}
		// // 4-2) [case] it send partial (bytes_result > 0)
		else if (bytes_result > 0)
		{
			// check) 
			CGASSERT_ERROR(static_cast<int>(bytes_sending) > bytes_result);

			// - recalculate byte sending
			bytes_sending -= bytes_result;

			// check)
			CGASSERT_ERROR(bytes_sending != 0);

			// - process sent buffer
			for (auto iter = queue_send.begin(), iter_end = queue_send.end(); iter != iter_end; ++iter, queue_send.pop_front(), ++msg_send.msg_iov, --msg_send.msg_iovlen)
			{
				auto bytes_iter = iter->buf_send.size<int>();

				if (bytes_iter > bytes_result)
				{
					iter->buf_send += offset(bytes_result);
					msg_send.msg_iov->iov_base = iter->buf_send.data();
					msg_send.msg_iov->iov_len = iter->buf_send.size();
					break;
				}

				bytes_result -= bytes_iter;

				// statistics) 
				process_complete_sendable(0, iter->bytes_data, iter->count_message);
			}

			// check) 
			CGASSERT_ERROR(queue_send.empty() == false);
			CGASSERT_ERROR(msg_send.msg_iovlen != 0);

			// continue) 
			continue;
		}
		else
		{
			// - get last error
			auto error_code = api::get_last_error();

			// 4-3) [case] it send not at all but not failure, need waiting for EPOLLOUT (EWOULDBLOCK)
			if (error_code == EWOULDBLOCK || error_code == EINPROGRESS)	// - EINPROGRESS : a TCP Fast Open connect (TCP_FASTOPEN_CONNECT) without a cookie. the SYN is sent, the data waits for EPOLLOUT
			{
				// - move remained to m_queue_pending
				SCOPED_LOCK(this->m_lockable_sending)
				{
					// - EPOLLOUT을 기다리는 것 (queue 한도에는 세지 않는다)
					this->m_count_sending_parked = queue_send.size();
					this->m_bytes_sending_parked = bytes_sending;

					this->m_queue_pending.array_node.splice(this->m_queue_pending.array_node.begin(), queue_send);
					this->m_queue_pending.bytes_data += bytes_sending;
				}

				// - swap
				this->m_queue_sending.array_node.swap(queue_send);

				// break)
				break;
			}
			// 4-4) [case] failure
			else
			{
				// - set last error
				this->m_last_socket_error = error_code;

				// statistics) 
				this->statistics_on_error_send();

				// - closesocket
				this->process_close_socket(DISCONNECT_REASON_FAIL);

				// - clear send queue
				SCOPED_LOCK(this->m_lockable_sending)
				{
					this->_process_clear_send_buffers();
				}

				// break)
				break;
			}
		}
	}

	// declare) 
	bool flag_sending;
	bool is_shutdown;

	// 5) set event flag
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// - set 
		flag_sending = !this->m_queue_pending.array_node.empty();

		// - set flag
		this->m_flag_sending = flag_sending;

		// - 모두 보냈다 => SEND_DISCONNECTED면 shutdown한다. (그 사이 m_queue_pending에 쌓인 것이 있으면 그것까지 보낸 뒤에 한다)
		is_shutdown = (is_completed && flag_sending == false && this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED);
	}

	// - shutdown if send_disconnected
	if (is_shutdown)
	{
		// - set linger Option for graceful disconnect
		api::socket::set_linger_option(this->native_handle(), 0);

		// - send shutdown
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}

	// - post epoll event
	if (flag_sending == false)
	{
		// statistics) 
		this->statistics_on_decrease_async_sending();

		// return)
		return 0;
	}
	else
	{
		return EPOLLOUT;
	}
}

bool net::io::sendable::Ntcp_async_gather_buffered::_is_send_defer_full() const noexcept
{
	// - m_lockable_sending 안에서 부른다. (모은 것은 m_queue_pending에 있다)
	//   queue 한도의 절반을 넘지 않게 한다. (모으느라 overflow가 나지 않는다)
	const auto count = this->m_queue_pending.array_node.size();
	const auto bytes = this->m_queue_pending.bytes_data;

	return count >= SEND_DEFER_MAX_COUNT || bytes >= SEND_DEFER_MAX_BYTES || (count * 2) >= this->m_max_depth_of_send_buffer || (bytes * 2) >= this->m_bytes_max_queued;
}

void net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send() noexcept
{
	// 설명) send defer로 이 thread가 모아 둔 것을 보낸다. (send_defer::flush가 부른다)
	//       이미 보냈거나(조기 flush) 다른 thread가 가져갔으면(closesocket) 아무것도 하지 않는다.
	//       (모으기를 끝내는 것은 이 thread와 closesocket(lock 안)뿐이다 : 한쪽만 보낸다)
	auto ptoken = send_defer::token();

	// check) 이 thread가 모은 것이 아니다.
	if(this->m_pdefer_owner.compare_exchange_strong(ptoken, nullptr, std::memory_order_acq_rel) == false)
		return;

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send_on_close() noexcept
{
	// 설명) socket을 닫기 전에 부른다. 어느 thread가 모아 둔 것이든 지금 보낸다.
	//       (그룹 락 안에서 send한 뒤 다른 thread가 closesocket해도 보낸 것이 먼저 간다)
	//       - lock 안에서 가져온다. (모으는 thread가 lock 안에서 넣는 중일 수 있다 : 그 뒤 그 thread의 송신은 보통 송신으로 쌓인다)
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 모아 둔 것이 없다.
		if(this->m_pdefer_owner.exchange(nullptr, std::memory_order_acq_rel) == nullptr)
			return;
	}

	// - send
	this->_process_send_deferred();
}

void net::io::sendable::Ntcp_async_gather_buffered::_process_send_deferred() noexcept
{
	//-----------------------------------------------------------------
	// 설명) send defer로 모아 둔 것을 보낸다. (m_pdefer_owner를 가져온 thread가 부른다)
	//
	//  - 처음 모을 때 이 thread가 보내는 thread(m_flag_sending)가 되어 m_queue_pending에 모았다.
	//    그 사이 다른 thread가 보낸 것도 그 뒤에 쌓여 있다.
	//  - m_queue_pending이 빌 때까지 보내고, socket 송신 버퍼가 찼을 때만 EPOLLOUT을 기다린다.
	//    (다 보냈으면 EPOLLOUT을 걸지 않는다)
	//-----------------------------------------------------------------

	// 1) get socket native handle
	const auto handle_socket = this->native_handle();

	// check) 그 사이 다른 thread가 socket을 닫았다 => 모아 둔 것은 버린다. (m_flag_sending = false)
	if (handle_socket == INVALID_SOCKET)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			this->_process_clear_send_buffers();
		}

		// return)
		return;
	}

	// 2) m_queue_pending이 빌 때까지 보낸다.
	const auto result = this->_process_send_pending(handle_socket, 0, 0);

	// 3) EPOLLOUT을 기다린다. (남은 것은 m_queue_pending에 있다 : process_recover_sendable이 보낸다)
	if (result == EPOLLOUT)
	{
		SCOPED_LOCK(this->m_lockable_sending)
		{
			// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
			CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

			// statistics)
			this->statistics_on_increase_async_sending();

			// - set EPOLLOUT event flag (io_uring completion : the SEND op)
			this->_process_post_send_wait();
		}
	}
}

uint32_t net::io::sendable::Ntcp_async_gather_buffered::_process_send_pending(CGSOCKET _handle_socket, std::size_t _bytes_completed, std::size_t _count_completed)
{
	//-----------------------------------------------------------------
	// 설명) 보내는 thread(m_flag_sending == true)만 부른다. (send cork의 flush, 바로 보낸 뒤 이어 보내기)
	//
	//  - m_queue_pending을 lock 안에서 list swap으로 가져와(할당/복사 없음) send/sendmsg로 보낸다.
	//    m_queue_pending이 빌 때까지 반복한다. (그 사이 다른 thread가 쌓은 것도 이어서 보낸다)
	//  - return
	//    0                   : 모두 보냈다. (m_flag_sending = false)
	//    EPOLLOUT            : socket 송신 버퍼가 찼다. 남은 것은 m_queue_pending 앞에 되돌려 둔다. (m_flag_sending은 true)
	//    SEND_PENDING_FAILED : 실패해 접속을 끊었다. (m_flag_sending = false)
	//  - 통계(process_complete_sendable)는 호출당 한번만 한다. (_bytes_completed/_count_completed : 부르기 전에 보낸 것도 같이 한다)
	//-----------------------------------------------------------------

	// declare)
	container_t<QUEUE_SEND::NODE> queue_send;
	::iovec array_iovec[SEND_PENDING_MAX_IOV];
	uint32_t result = 0;
	bool is_shutdown = false;

	for (;;)
	{
		// 1) 보낼 것이 없으면 m_queue_pending에서 가져온다.
		if (queue_send.empty())
		{
			// declare)
			bool is_empty;

			SCOPED_LOCK(this->m_lockable_sending)
			{
				// - get
				is_empty = this->m_queue_pending.array_node.empty();

				// - 보낼 것이 없다 => 송신을 끝낸다.
				if (is_empty)
				{
					is_shutdown = (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED);
					this->m_flag_sending = false;
				}
				// - 가져온다.
				else
				{
					queue_send.swap(this->m_queue_pending.array_node);
					this->m_queue_pending.bytes_data = 0;
					this->m_count_sending_parked = 0;
					this->m_bytes_sending_parked = 0;
				}
			}

			// break) 모두 보냈다.
			if (is_empty)
				break;
		}

		// 2) iovec을 채운다. (한번에 SEND_PENDING_MAX_IOV개까지)
		std::size_t count_iovec = 0;
		std::size_t bytes_request = 0;

		for (auto iter = queue_send.begin(); iter != queue_send.end() && count_iovec < SEND_PENDING_MAX_IOV; ++iter, ++count_iovec)
		{
			array_iovec[count_iovec].iov_base = iter->buf_send.data();
			array_iovec[count_iovec].iov_len = iter->buf_send.size();
			bytes_request += iter->buf_send.size();
		}

		// 3) send (one buffer : send. a large user buffer : MSG_ZEROCOPY)
		ssize_t bytes_result;
		bool is_zerocopy = false;

		if (count_iovec == 1)
		{
			const auto& node_first = queue_send.front();

			// - io_uring completion : a large buffer of the user goes as a SEND_ZC op (not sent directly : the caller posts the op like a full socket buffer)
			if (this->is_io_completion() && node_first.bytes_remained == 0 && this->_is_zerocopy_op(array_iovec[0].iov_len, node_first.powner))
			{
				result = EPOLLOUT;
				break;
			}

			int flags_send = MSG_DONTWAIT | MSG_NOSIGNAL | ((node_first.bytes_remained == 0) ? this->_zerocopy_send_flags(_handle_socket, array_iovec[0].iov_len, node_first.powner) : 0);

			bytes_result = this->_zerocopy_send(_handle_socket, array_iovec[0].iov_base, array_iovec[0].iov_len, flags_send);
			is_zerocopy = (bytes_result > 0) && (flags_send & MSG_ZEROCOPY) != 0;
		}
		else
		{
			msghdr msg_send{};
			msg_send.msg_iov = array_iovec;
			msg_send.msg_iovlen = count_iovec;

			bytes_result = ::sendmsg(_handle_socket, &msg_send, MSG_DONTWAIT | MSG_NOSIGNAL);
		}

		// 4) the part sent
		if (bytes_result > 0)
		{
			// - zerocopy : the part sent is held until the kernel notifies
			if (is_zerocopy)
			{
				auto& node_first = queue_send.front();

				this->_zerocopy_hold(buffer_view{ node_first.buf_send.data(), static_cast<std::size_t>(bytes_result) }, object_ptr<Ireferenceable>(node_first.powner));
			}

			// declare)
			auto bytes_left = static_cast<std::size_t>(bytes_result);

			while (bytes_left != 0)
			{
				auto& node_front = queue_send.front();
				const auto bytes_node = node_front.buf_send.size();

				// - 일부만 보냈다 => 남은 위치로 옮긴다.
				if (bytes_left < bytes_node)
				{
					node_front.buf_send += offset(static_cast<int64_t>(bytes_left));
					break;
				}

				// - 다 보냈다.
				bytes_left -= bytes_node;
				_bytes_completed += node_front.bytes_data;
				_count_completed += node_front.count_message;
				queue_send.pop_front();
			}

			// continue) 보내려던 것을 다 보냈으면 이어서 보낸다.
			if(static_cast<std::size_t>(bytes_result) == bytes_request)
				continue;

			// - 덜 보냈다 => socket 송신 버퍼가 찼다. EPOLLOUT을 기다린다.
			result = EPOLLOUT;
			break;
		}

		// get last error
		const auto error_code = api::get_last_error();

		// 5-1) EWOULDBLOCK => EPOLLOUT을 기다린다.
		if (error_code == EWOULDBLOCK || error_code == EINPROGRESS)	// - EINPROGRESS : a TCP Fast Open connect (TCP_FASTOPEN_CONNECT) without a cookie. the SYN is sent, the data waits for EPOLLOUT
		{
			result = EPOLLOUT;
			break;
		}

		// 5-2) EINTR => 다시 보낸다.
		if(error_code == EINTR)
			continue;

		// 5-3) failure
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics)
			this->statistics_on_error_send();

			// - closesocket
			this->process_close_socket(DISCONNECT_REASON_FAIL);

			// - clear send queue (m_flag_sending = false)
			queue_send.clear();

			SCOPED_LOCK(this->m_lockable_sending)
			{
				this->_process_clear_send_buffers();
			}

			// - set result
			result = SEND_PENDING_FAILED;
		}

		// break)
		break;
	}

	// 6) EPOLLOUT을 기다린다 => 남은 것은 m_queue_pending 앞에 되돌린다. (그 사이 쌓인 것은 그 뒤에 있다 : 순서 유지)
	if (result == EPOLLOUT)
	{
		// declare)
		std::size_t bytes_remained = 0;

		for (const auto& iter : queue_send)
			bytes_remained += iter.buf_send.size();

		SCOPED_LOCK(this->m_lockable_sending)
		{
			// - EPOLLOUT을 기다리는 것 (queue 한도에는 세지 않는다)
			this->m_count_sending_parked = queue_send.size();
			this->m_bytes_sending_parked = bytes_remained;

			this->m_queue_pending.array_node.splice(this->m_queue_pending.array_node.begin(), queue_send);
			this->m_queue_pending.bytes_data += bytes_remained;
		}
	}

	// statistics) 보낸 것들의 통계 (한번에)
	if (_bytes_completed != 0 || _count_completed != 0)
	{
		this->process_complete_sendable(0, _bytes_completed, _count_completed);
	}

	// - shutdown if send_disconnected (모두 보냈을 때)
	if (is_shutdown)
	{
		// - set linger Option for graceful disconnect
		api::socket::set_linger_option(this->native_handle(), 0);

		// - send shutdown
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}

	// return)
	return result;
}

std::size_t net::io::sendable::Ntcp_async_gather_buffered::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data + this->m_bytes_sending_op;
}

void net::io::sendable::Ntcp_async_gather_buffered::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

void net::io::sendable::Ntcp_async_gather_buffered::process_close_sender()
{
	// check)
	bool do_shutdown;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		do_shutdown = !this->m_flag_sending;
	}

	if (do_shutdown)
	{
		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(this->native_handle(), 0);

		// - 접속을 종료한다.
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}
}

void net::io::sendable::Ntcp_async_gather_buffered::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		this->_process_clear_send_buffers();

		// - send defer : 모으던 thread의 flush는 아무것도 하지 않는다.
		this->m_pdefer_owner.store(nullptr, std::memory_order_relaxed);
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
}

void net::io::sendable::Ntcp_async_gather_buffered::_process_clear_send_buffers()
{
	for ([[maybe_unused]] auto& iter : this->m_queue_pending.array_node)
		this->process_complete_sendable(1, 0, 0);

	this->m_flag_sending = false;

	this->m_queue_pending.clear();

	// - io_uring completion : a SEND op in flight uses m_queue_sending (its buffers) => moved to the executable till the op ends
	if (this->m_count_sending_op != 0 && this->m_pexecutable_sending.exist())
		this->m_pexecutable_sending->m_queue_hold.splice(this->m_pexecutable_sending->m_queue_hold.end(), this->m_queue_sending.array_node);

	this->m_queue_sending.clear();
	this->m_bytes_sending_op = 0;
	this->m_count_sending_parked = 0;
	this->m_bytes_sending_parked = 0;

	// - zerocopy : the buffers held for the notifications (the socket is closed or reset : the kernel does not notify any more)
	this->_zerocopy_reset();
}


#endif
