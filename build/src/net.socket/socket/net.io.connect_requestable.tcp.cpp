#include "pch.h"
#include <charconv>

using namespace CGDK;

net::io::connect_requestable::Ntcp::Ntcp() :
	Nnameable(),
	m_result_connecting(eRESULT::NOT_DEFINED)
{
	this->register_binder(this);
}

net::io::connect_requestable::Ntcp::~Ntcp() noexcept
{
	this->unregister_binder();
}

result_code net::io::connect_requestable::Ntcp::wait_complete_connecting(chrono::tick::duration _duration_wait)
{
	// 1) get executor
	object_ptr<Iexecutor> pio_context = this->io_context();

	// 2) if executor is nullptr, use system_executor
	if(pio_context.empty())
	{
		pio_context = system_executor::executor();
	}

	// declare) 
	result_code	result;
	auto tick_end = chrono::tick::now()+_duration_wait;

	// 3) wait
	auto pthread_now = api::get_current_thread();

	// check)  
	CGASSERT_ERROR(pthread_now.exist());

	// 4) notify  blocked
	pthread_now->notify_blocked();

	try
	{
		// 5) wait util connecting process is completed
		while((result = this->m_result_connecting) >= eRESULT::PROCESS)
		{
			// - it block and wait 10ms maximum util single execution being complete.
			if(pio_context.exist())
			{
				pio_context->execute(chrono::milliseconds(2));
			}

			// check) if time is expired, return eRESULT::TIME_OUT error.
			if(chrono::tick::now() > tick_end)
			{
				result = eRESULT::TIME_OUT;
				break;
			}
		}
	}
	catch (...)
	{
		// - notify  blocked
		pthread_now->notify_unblocked();

		// reraise)
		throw;
	}

	// 6) notify unblocked
	pthread_now->notify_unblocked();

	// return) socket의 상태가 eSOCKET_STATE::SYN보다 크면 접속된 것을 의미한다.
	return result;
}

result_code net::io::connect_requestable::Ntcp::wait_complete_closed(chrono::tick::duration _duration_wait)
{
	// 1) get executor
	object_ptr<executor::Iio_context> pio_context = io_context();

	// 2) if executor is nullptr, use system_executor
	if(pio_context.empty())
	{
		pio_context = system_executor::executor();
	}

	// declare) 
	result_code	result = eRESULT::SUCCESS;

	// 3) get now tick
	auto tick_end = chrono::tick::now() + _duration_wait;

	// 4) wait
	auto pthread_now = api::get_current_thread();

	// check)  
	CGASSERT_ERROR(pthread_now.exist());

	// 5) notify  blocked
	pthread_now->notify_blocked();

	try
	{
		// 6) wait util connecting process is completed
		while(this->m_pconnectable_notify != nullptr || this->socket_state() != eSOCKET_STATE::CLOSED)
		{
			// - it block and wait 10ms maximum util single execution being complete.
			if(pio_context.exist())
			{
				pio_context->execute(chrono::milliseconds(2));
			}

			// check) if time is expired, return eRESULT::TIME_OUT error.
			if(chrono::tick::now() > tick_end)
			{
				result = eRESULT::TIME_OUT;
				break;
			}
		}
	}
	catch (...)
	{
		// - notify  blocked
		pthread_now->notify_unblocked();

		// reraise)
		throw;
	}

	// 7) notify unblocked
	pthread_now->notify_unblocked();

	// return) socket의 상태가 eSOCKET_STATE::SYN보다 크면 접속된 것을 의미한다.
	return result;
}

result_code net::io::connect_requestable::Ntcp::process_start(context& _context)
{
	// declare)
	result_code result;

	try
	{
		// declare)
		START_PARAMETER parameter;

		// 1) set default value
		parameter.enable_progress_log = m_enable_progress_log;;

		// check) socket 상태가 CLOSED여야지만 한다.
		CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

		// 2) 'enable progress_log
		if (auto& context_find = _context.find("progress_log", "progress log", "enable_progress_log", "enable progress log"); context_find.is_valid())
		{
			// - get string value
			std::string str_find = context_find;

			if (CGDK::__stricmp(str_find, "true") == 0
				|| CGDK::__stricmp(str_find, "enable") == 0)
			{
				parameter.enable_progress_log = true;
			}
			else if (CGDK::__stricmp(str_find, "false") == 0
				|| CGDK::__stricmp(str_find, "disable") == 0)
			{
				parameter.enable_progress_log = false;
			}
		}

		// log)
		EXECUTE_IF(parameter.enable_progress_log, LOG_PROGRESS << "@ <BEGIN> start CONNECT_REQUESTABLE['"sv << name() << "']"sv);

		// 3) _process_starting 함수를 호출한다.
		result = this->_process_starting(_context);

		// check)
		RETURN_IF(result != eRESULT::SUCCESS, result);

		// 4) connect를 시도한다.
		try
		{
			if (_context.type() == context::value_type_t::buffer_)
			{
				parameter = _context.v.value_buffer.front<START_PARAMETER>();

				// - the tcp socket option (START_PARAMETER::socket_option when flag_socket_option != 0)
				if (parameter.flag_socket_option != 0)
					this->socket_option(parameter.socket_option);
			}
			else
			{
				// - socket setting을 읽어낸다.
				parameter = get_parameter_connection(_context, parameter.enable_progress_log);

				// - socket_option ("socket_option": {...}) : the tcp socket option of this socket (the keys not given are kept)
				get_parameter_socket_option(_context, this->socket_option(), parameter.enable_progress_log);
			}

			// - 'block until completed connecting'
			switch (parameter.flag_wait_complete_connecting)
			{
			// case) enable 'block until completed connecting'
			case	2:
					enable_block_until_connecting_completed();
					break;

			// case) disable 'block until completed connecting'
			case	1:
					disable_block_until_connecting_completed();
					break;

			// case) not assigned
			default:
				break;
			}

			// - 'block until completed connecting'
			switch (parameter.flag_wait_complete_disconnecting)
			{
			// case) enable 'block until completed connecting'
			case	2:
					enable_block_until_disconnecting_completed();
					break;

			// case) disable 'block until completed connecting'
			case	1:
					disable_block_until_disconnecting_completed();
					break;

			// case) not assigned
			default:
					break;
			}

			// - parameter.remote_endpoint이 empty일 경우 ipv6 localhost로 설정한다.
			if (parameter.remote_endpoint.empty())
			{
				// - set default remote_point
				parameter.remote_endpoint = end_point_type{ net::ip::address_v6::any(), process_get_default_port() };

				// log) 
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  ! 'remote endpoint' set as default endpoint (ipv6::loopback:"sv << process_get_default_port() << ")  "sv);
			}

			// - parameter.remote_endpoint의 port가 0일 경우 default로 설정한다.
			if (parameter.remote_endpoint.port() == 0)
			{
				// - set default remote_point port
				parameter.remote_endpoint.port(process_get_default_port());

				// log) remote 
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  ! 'remote endpoint' port set default ("sv << process_get_default_port() << ")  "sv);
			}

			// check) 
			if (parameter.remote_endpoint.port() == 0)
			{
				// log) 
				LOG_ERROR << "   (err ) invalid remote_endpoint port (port number is '0')"sv;

				// raise)
				throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "invalid remote_endpoint port (port number is '0')"sv);
			}

			// - (linux) a loopback remote endpoint tries the UDS first only when no bind endpoint is given
			this->m_try_uds_loopback = parameter.bind_endpoint.empty() || (parameter.bind_endpoint.address().is_unspecified() && parameter.bind_endpoint.port() == 0);

			// - set socket_addr
			if (parameter.bind_endpoint.empty())
			{
				// - set default address
				if (parameter.remote_endpoint.is_v4())
				{
					parameter.bind_endpoint = net::ip::tcp::endpoint{ { net::ip::address_v4::any() } , 0 };
				}
				else
				{
					parameter.bind_endpoint = net::ip::tcp::endpoint{ { net::ip::address_v6::any() } , 0 };
				}
			}

			// - bind 먼저 한다.
		#if defined(_SYS_EPOLL_H)
			//   (linux) any:0 => no bind. connect() takes an ephemeral port itself.
			//   (a bind to port 0 searches the ephemeral range : slow and fails when many ports are in TIME_WAIT. the UDS path does not need a port at all)
			if (this->m_try_uds_loopback)
				this->prepare_socket(static_cast<ADDRESS_FAMILY>(parameter.bind_endpoint.data().family()));
			else
		#endif
				bind(parameter.bind_endpoint);

			// - connect한다.
			auto b_result = process_request_connecting(parameter.remote_endpoint);

			// - success
			result = (b_result) ? result_code(eRESULT::SUCCESS) : m_result_connecting;
		}
		catch (std::exception& e)
		{
			// - connecting Result를 Fail로 설정한다.
			m_result_connecting = eRESULT::NOT_DEFINED;

			// log) prepare receive에 실패했다!
			LOG_DEBUG << "@ (excp) net::io::connect_requestable::Ntcp. exception complete '"sv << name() << "'(" << static_cast<int64_t>(native_handle()) << ") " __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
			{
				// reraise)
				throw;
			}
		}

		// 3) block 
		if (this->m_enable_block_until_connecting_completed)
		{
			// - set result code
			result = wait_complete_connecting();
		}

		// log)
		EXECUTE_IF(parameter.enable_progress_log, LOG_PROGRESS << "@ <END> start CONNECT_REQUESTABLE['"sv << name() << "']"sv);

		// check)
		RETURN_IF(result != eRESULT::SUCCESS, result);

		// 4) call '_process_initializing_self_complete'
		result = this->_process_starting_child(_context);
	}
	catch (std::exception& _exception)
	{
		// - call exception handler
		throwable::execute_exception_handler(this, _exception, __FUNCTION__);

		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_DEBUG << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string();

		// log) 
		LOG_DEBUG << "(excp) exception occurred on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}

	// return)
	return result;
}

result_code net::io::connect_requestable::Ntcp::process_stop() noexcept
{
	// declare) 
	bool enable_progress_log = m_enable_progress_log;

	// log)
	EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <BEGIN> stop CONNECT_REQUESTABLE['"sv << name() << "']"sv);

	// 1) call '_process_stopping'
	auto result = this->_process_stopping();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) close socket
	this->closesocket();

	// 3) call '_process_destroying_self_complete'
	result = this->_process_stopping_child();

	// 4) wait until completed
	if (this->m_enable_block_until_disconnecting_completed)
	{
		this->wait_complete_closed();
	}

	// log)
	EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <END> stop CONNECT_REQUESTABLE['"sv << name() << "']"sv);

	// return)
	return result;
}

void net::io::connect_requestable::Ntcp::process_reset_connector() noexcept
{
}

bool net::io::connect_requestable::Ntcp::process_request_connecting( const net::sockaddr& _address)
{
	// check) _address의 내용이 설정되어 있지 않으면 안 된다.
	CGASSERT(_address.is_unspecified() == false, false)

	// check) _address의 내용이 설정되어 있지 않으면 안 된다.
	THROW_IF(_address.is_unspecified() == true, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check) 
	RETURN_IF(api::socket::is_socket_count_overflowed(), false);


	//-----------------------------------------------------------------
	// 1. 상태변경
	//
	// - 상태변경을 하면서 이미 connect상태이거나 connect를 시도한 상태인지
	//   를 구분하여 만약 이미 connect를 시도한 상태이거나 connect상태이면
	//   바로 Return할 수 있도록 처리를 해주어야 한다.
	// - 이를 위해 atomic를 사용하여 SocketState를 변경하여 그 이전
	//   값을 Check하여 처리한다.
	//-----------------------------------------------------------------
	// 1) socket상태를 eSOCKET_STATE.SYN으로 바꾼다.
	//    - 내부적으로 atomic함수를 통해 CLOSED 상태로 변경한다.
	//    - 여기서 atomic함수를 사용하는 이유는 상황에 따라 중복적으로 
	//      connect와 disconnect등 socket의  상태가 불려지는 함수가 겹쳐
	//      서 호출될 수 있기 때문이다. 
	//    - socket의 상태가 CLOSED일 때만 connect함수가 수행되고 아닐 경우 
	//      더 이상 진행하지 않고 false를 리턴한다.
#if defined(_SYS_EPOLL_H)
	// - the sequence of this request (io_uring completion : the completion of a request closed and started again meanwhile is dropped
	//   by it. never 0. before the state is SYN : a stale completion that sees SYN sees the new sequence)
	uint32_t seq_connecting = this->m_seq_connecting.load(std::memory_order_relaxed) + 1;
	if (seq_connecting == 0) seq_connecting = 1;
	this->m_seq_connecting.store(seq_connecting, std::memory_order_release);
#endif

	bool is_changed = set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

	// 2) socket이 CLOSED된 상태에서만 socket을 eSOCKET_STATE::SYN상태로 변경시킨다.
	RETURN_IF(is_changed == false, false);

	// 3) Handle을 얻는다.
	auto handle_socket = this->native_handle();

	// check) handle_socket이 INVALID_SOCKET이어서는 안된다.
	CGASSERT_ERROR(handle_socket != INVALID_SOCKET); 

	// check) ...
	RETURN_IF(handle_socket == INVALID_SOCKET, false);
	

	//-----------------------------------------------------------------
	// 2. 접속을 시도한 peer Address를 저장한다.
	//-----------------------------------------------------------------
	// 1) process_request_connect()함수를 호출한다.
	this->process_request_connect(_address);

	// 2) resultConnecting을 Undefined로...
	this->m_result_connecting = eRESULT::REQUEST;
	this->m_is_client_socket = true;


#if defined(_WINSOCK2API_)
	//-----------------------------------------------------------------
	// 3. socket Option을 설정한다.
	//-----------------------------------------------------------------
	// 1) Fast Loopback을 활성화하한다.
	api::socket::set_loopback_fast_path(handle_socket, true);

	// 2) TCP Fast Open (socket_option().tcp_fastopen) : TCP_FASTOPEN before ConnectEx. the TFO cookie of the server is requested on the first connect
	//    (the data in the SYN needs a send buffer in ConnectEx : not used here. with the cookie the server accepts the data in the SYN of the next connects)
	if (this->socket_option().tcp_fastopen)
	{
		auto result_fastopen = api::socket::set_tcp_fastopen_connect(handle_socket, true);

		// trace)
		EXECUTE_IF(result_fastopen == false, LOG_DEBUG << "(err ) CONNECT_REQUESTABLE['" << name() << "'] fail to set 'TCP_FASTOPEN' error_code: "sv << api::get_last_error() << " " __CGFUNCTION__ ""sv);
	}


	//-----------------------------------------------------------------
	// 4. connect를 수행한다. (IOCP)
	//-----------------------------------------------------------------
	// 1) executable을 생성한다.
	auto pexecutable_connector = make_object<net::io::executable_connect_requestable>();

	// 2) connectable을  추가한다.
	pexecutable_connector->set_connectable(this);

	// 3) Reference count를 1 증가시킨다.(일반적으로 2가 된다.)
	pexecutable_connector->add_ref();

	// 4) connect를 수행한다.
	auto result = api::socket::connect_async(handle_socket, pexecutable_connector, reinterpret_cast<const ::sockaddr*>(&_address), _address.length);

	// check) failed?
	if(result == false)
	{
		// - set last error
		this->m_last_socket_error = api::get_last_error();

		// log) 
		LOG_DEBUG << "(err ) CONNECT_REQUESTABLE['" << name() << "'] fail to connecting socket error_code: "sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// - on_fail_connect를 호출한다.
		this->process_complete_connect(1);

		// - Release한다.
		pexecutable_connector->release();

		// - connecting Result를 Fail로 설정한다.
		this->m_result_connecting = eRESULT::NOT_DEFINED;

		// return)
		return false;
	}

	// statistics) 
	this->statistics_on_increase_async_con_dis();

	// statistics) 접속 시도 횟수
	this->statistics_on_connect_try();

#elif defined(_SYS_EPOLL_H)
	//-----------------------------------------------------------------
	// 4. connect를 수행한다. (ePoll)
	//-----------------------------------------------------------------
	// check) addres length가 0이면 안 된다.
	CGASSERT_ERROR(_address.length != 0)

	// statistics) 접속 시도 횟수
	this->statistics_on_connect_try();

	// 1) connect를 요청한다.
	int result = -1;
	bool is_connected_uds = false;

	// - io_uring completion : the connect is an op (TCP) or posted (a UDS connected at once). no readiness registration
	const bool is_completion_backend = io_context().exist() && io_context()->backend() == executor::io_backend::io_uring_completion;

	// - a loopback address : tries the UDS of the port first. ("\0cgdk.tcp.<port>" : an acceptor with enable_uds)
	//   a UDS connect completes at once or fails at once. succeeded => the TCP socket is replaced with the UDS one
	//   (not attached to epoll yet) and the rest is the same. failed (no UDS listener) => TCP.
	if (this->m_try_uds_loopback && is_enable_uds_loopback() && _address.is_loopback() && _address.port() != 0)
	{
		::sockaddr_un addr_uds;
		socklen_t addr_length = 0;
		api::socket::make_uds_address(_address.port(), addr_uds, addr_length);

		auto handle_uds = api::socket::instance().create_socket(AF_UNIX, SOCK_STREAM, 0);

		if (handle_uds != INVALID_SOCKET)
		{
			// - nonblocking already (create_socket) : connects at once or fails at once
			if (::connect(handle_uds, reinterpret_cast<const ::sockaddr*>(&addr_uds), addr_length) == 0)
			{
				// - replace the socket
				auto handle_tcp = this->native_handle(handle_uds);
				if (auto pio_context = this->io_context(); pio_context.exist())
					pio_context->detach(static_cast<CGHANDLE>(handle_tcp));
				api::socket::closesocket(handle_tcp);
				this->address_family(AF_UNIX);
				this->set_epoll_handle(handle_uds);
				handle_socket = handle_uds;
				is_connected_uds = true;
				result = 0;
			}
			else
			{
				api::socket::closesocket(handle_uds);
			}
		}
	}

	// - TCP
	if (is_connected_uds == false)
	{
		// - TCP Fast Open (socket_option().tcp_fastopen) : TCP_FASTOPEN_CONNECT. connect() returns 0 at once (no SYN yet) and the first send() sends the SYN
		//   (with the data when a TFO cookie of the server is cached. without a cookie the send() returns EINPROGRESS : the sendable waits for EPOLLOUT like EWOULDBLOCK)
		//   the socket is writable (EPOLLOUT) at once : process_complete_connecting() is called before the handshake.
		if (this->socket_option().tcp_fastopen && api::socket::set_tcp_fastopen_connect(handle_socket, true) == false)
			LOG_DEBUG << "(err ) CONNECT_REQUESTABLE['" << name() << "'] fail to set 'TCP_FASTOPEN_CONNECT' error_code: "sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// - io_uring completion : a CONNECT op (no registration. its completion calls process_complete_connecting() like the poll :
		//   an immediate failure of the connect comes as a completion too). the readiness backends : connect() and the EPOLLOUT poll below
		if (is_completion_backend)
		{
			// - executor를 생성한다.
			this->process_prepare_executable_socket();

			// - executable을 생성한다.
			auto pexecutable_connector = make_object<net::io::executable_connect_requestable>();

			pexecutable_connector->set_connectable(this);
			pexecutable_connector->m_is_submit = true;
			pexecutable_connector->m_fd = handle_socket;
			pexecutable_connector->m_address = _address;
			pexecutable_connector->m_seq = seq_connecting;

			// - submitted by a worker of the io_context (process_submit_connect_op) : a request belongs to the thread that submitted it
			//   (the kernel cancels the requests of a thread that exits : a connect submitted by a short lived thread failed with -ECANCELED)
			// check) the post failed : no completion follows => fails here like an immediate failure
			if (io_context()->post(object_ptr<Iexecutable>(pexecutable_connector.get())) == false)
			{
				this->m_last_socket_error = api::get_last_error();
				this->m_result_connecting = eRESULT::NOT_DEFINED;
				this->process_complete_connect(1);
				return false;
			}

			// return) pending
			return true;
		}

		result = ::connect(handle_socket, reinterpret_cast<const ::sockaddr*>(&_address), _address.length);
	}

	// 2-1) 바로 접속 완료! - 접속까지 완료! (접속까지 완료된 상황이므로 접속 완료 처리를 해준다.)
	//      the socket is not attached to epoll yet : it is attached below and completed by the EPOLLOUT event like a pending connect.
	//      (process_complete_connecting() only modifies the event of an attached socket. a UDS connect always completes at once)
	{
		// get last error code
		auto error_code = (result == 0) ? 0 : api::get_last_error();

		// 2-2) Pending : 비동기적으로 connect가 처리 중임.
		if(result == 0 || (result < 0 && (error_code == EINPROGRESS || error_code == EAGAIN)))
		{
			// - executor를 생성한다.
			this->process_prepare_executable_socket();

			// - executable을 생성한다.
			auto pexecutable_connector = make_object<net::io::executable_connect_requestable>();

			// - connectable을  추가한다.
			pexecutable_connector->set_connectable(this);

			// - io_uring completion (a UDS connected at once) : the completion is posted (no registration on a completion io_context)
			if (is_completion_backend)
			{
				pexecutable_connector->m_is_connected = true;
				pexecutable_connector->m_seq = seq_connecting;

				if (io_context()->post(object_ptr<Iexecutable>(pexecutable_connector.get())) == false)
				{
					this->m_result_connecting = eRESULT::NOT_DEFINED;
					this->process_complete_connect(1);
					return false;
				}

				// return) pending
				return true;
			}

			// - 새로 생성한 socket을 executor에 붙인다.
			auto result = io_context()->attach(handle_socket, pexecutable_connector, EPOLLONESHOT | EPOLLWAKEUP | EPOLLOUT | EPOLLERR);

			// check) set_event에 실패했을 경우 Release한다.
			if (result == false)
			{
				// - no completion follows : fails here like an immediate failure below
				this->m_result_connecting = eRESULT::NOT_DEFINED;
				this->process_complete_connect(1);
				return false;
			}
		}
		// 2-3) fail
		else
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// log) 
			LOG_DEBUG << "(err ) CONNECT_REQUESTABLE['" << name() << "'] fail to connecting socket error_code: "sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

			// - connecting Result를 Fail로 설정한다.
			this->m_result_connecting = eRESULT::NOT_DEFINED;

			// - on_fail_connect and CLOSED (like the iocp path above). no completion follows an immediate failure (ex. ECONNREFUSED on loopback) :
			//   left SYN, a later close took it as a connected socket (process_complete_disconnect) and wait_complete_closed() waited till the timeout
			this->process_complete_connect(1);

			// return)
			return false;
		}
	}
#else
	#error	"unsupported network API"
#endif

	// return) 성공!!!
	return true;
}

void net::io::connect_requestable::Ntcp::process_complete_connecting(uintptr_t _result)
{
#if defined(_WINSOCK2API_)
	//-----------------------------------------------------------------
	// 1. socket Option을 바꾼다.
	//
	// - 여기서 SO_UPDATE_CONNECT_CONTEXT option을 설정하는 것은 매우
	//   매우 중요하다.
	// - connect_async를 통해 접속을 받은 socket에 대해 이 옵션을 수행해
	//   주지 않으면 각종 socket관련 함수들 이 Not connect라며 Error를 
	//   내게 되므로 반드시 SO_UPDATE_CONNECT_CONTEXT 옵션을 설정해
	//   주어야 한다.
	//
	//-----------------------------------------------------------------
	if(_result == 0)
	{
		// 1) socket 속성을 Update함.
		bool result = api::socket::update_connect_context(this->native_handle());

		// check) socket Option의 수행에 실패했는가?
		EXECUTE_IF(result == false, LOG_DEBUG << "(err ) CONNECT_REQUESTABLE['"sv << name() << "'] fail to setting 'SO_UPDATE_CONNECT_CONTEXT' option " __CGFUNCTION__ ""sv);
	}

	//-----------------------------------------------------------------
	// 2. 완료처리를 한다.
	//-----------------------------------------------------------------
	// 1) process_complete_connect를 수행함.
	auto result = this->process_complete_connect(_result);

	// 2) Result를 설정한다.
	if (result == true)
	{
		// - result를 eRESULT::SUCCESS로
		this->m_result_connecting = eRESULT::SUCCESS;
	}
	else
	{
		// - result를 eRESULT::NOT_DEFINED로
		this->m_result_connecting = eRESULT::NOT_DEFINED;

		// - connective를 reset한다.
		this->m_pconnectable_notify = nullptr;
	}

	// statistics) connect/disconnect를 위해 실행 중인 Overlapped I/O수를 줄임.
	this->statistics_on_decrease_async_con_dis();

#elif defined(_SYS_EPOLL_H)
	//-----------------------------------------------------------------
	// 1. EPOLL event에 따라 _result값을 변경한다.
	//
	// - _result값이 실패면 1, 성공이면 0로 변경한다.
	// - EPOLL_OUT을 받게 되면 접속성공이므로 _result을 1로 설정하고
	//   아니면 0으로 설정한다.
	// - EPOLL일 경우 이 부분이 중복 호추로딜 수 있으므로 그에 대한
	// 	 처리가 필요하다.
	//
	//-----------------------------------------------------------------
	// check) 중복처리 중이면 그냥 끝낸다.(epoll은 중복으로 들어 올 수 있다.)
	bool is_close_after_connect = false;

	SCOPED_LOCK(this->m_epoll_event_cs)
	{
		// check) the completion of a request closed and started again meanwhile (io_uring completion : the sequence in _result) => dropped
		RETURN_IF(this->_process_check_connecting_seq(_result) == false);

		// check) the connect is being processed or was completed by another thread (epoll may report it twice)
		if (this->m_result_connecting != eRESULT::REQUEST)
		{
			// - not a close => a duplicate event : dropped
			RETURN_IF((_result & (EPOLLHUP | EPOLLERR)) == 0);

			// - a close (process_close_socket() of a SYN socket : EPOLLHUP) while that thread processes the connect => queued.
			//   _process_end_complete_connect() of that thread completes the disconnect (the handle is closed by then : no re-arm)
			if (this->m_epoll_event_processing_now)
			{
				this->m_epoll_event_flag_queued |= EPOLLHUP;
				return;
			}

			// - a close after that thread completed the connect meanwhile (the state was SYN when the close began) => the disconnect now
			is_close_after_connect = true;
		}
		else
		{
			// - processing (like an epoll event : a close meanwhile, in on_connect or by another thread, is queued and done by
			//   _process_end_complete_connect(). set here, under the guard : set later, a close queued between was lost)
			this->m_result_connecting = eRESULT::PROCESS;
			this->m_epoll_event_flag_queued = 0;
			this->m_epoll_event_processing_now = true;
		}
	}

	// - the close of a socket connected meanwhile (above) : process_close_socket() closed the handle, the disconnect completes here
	if (is_close_after_connect)
	{
		if (this->socket_state() != eSOCKET_STATE::CLOSED)
			this->process_complete_disconnect();

		return;
	}

	// 2-1) 접속에 실패했을 경우
	if (_result & (EPOLLHUP | EPOLLERR))
	{
		_result = 1;
	}
	// 2-2) 접속에 성공했을 경우
	else if (_result & EPOLLOUT)
	{
		_result = 0;
	}
	else
	{
		// log) 
		LOG_DEBUG << "(warning) undefined epoll event ['"sv << _result << "'] "sv << " " __CGFUNCTION__ ""sv;

		// - assert
		CGASSERT_ERROR(false);

		// - set result as i (failure) 
		_result = 1;
	}

	//-----------------------------------------------------------------
	// 2. 완료처리를 한다.
	//-----------------------------------------------------------------
	// 1) process_complete_connect를 수행함. (a close in on_connect is queued till _process_end_complete_connect)
	auto result = this->process_complete_connect(_result);

	// 2) do event
	if(result == true)
	{
		// - result를 eRESULT::SUCCESS로
		this->m_result_connecting = eRESULT::SUCCESS;

		// - attach to socket executor (or completes the close queued in on_connect)
		auto result_attach = this->_process_end_complete_connect(true);

		// - result를 설정한다.
		this->m_result_connecting = (result_attach == true) ? eRESULT::SUCCESS : eRESULT::FAIL;
	}
	else
	{
		// - end
		this->_process_end_complete_connect(false);

		// - 실패한 접속의 handle을 epoll에서 뗀다. (process_request_connecting에서 붙였다.)
		//   떼지 않으면 같은 fd 번호로 다시 접속할 때 이전 등록(끝난 executable)이 먼저 찾아져 완료를 받지 못한다. (재접속이 멈춘다)
		if (auto handle_socket = this->native_handle(); handle_socket != INVALID_SOCKET)
		{
			if (auto pio_context = this->io_context(); pio_context.exist())
				pio_context->detach(static_cast<CGHANDLE>(handle_socket));
		}

		// - result를 eRESULT::NOT_DEFINED로
		this->m_result_connecting = eRESULT::NOT_DEFINED;

		// - connective를 reset한다.
		this->m_pconnectable_notify = nullptr;
	}
#endif
}

#if defined(_SYS_EPOLL_H)
bool net::io::connect_requestable::Ntcp::process_submit_connect_op(net::io::executable_connect_requestable* _pexecutable) noexcept
{
	//-----------------------------------------------------------------
	// io_uring completion : submits the CONNECT op of the request (posted by process_request_connecting : run by a worker thread)
	//
	//  - the request is still this one (under the guard : the state is SYN (read first), the handle and the sequence are the
	//    ones of the post). closed meanwhile (process_close_socket of the SYN socket completed it) or started again (another
	//    sequence) => dropped
	//  - the submit failed => completed as a failure (no completion follows)
	//-----------------------------------------------------------------
	// check)
	RETURN_IF(_pexecutable == nullptr, false);

	// declare)
	bool is_submitted = false;
	bool is_current = false;

	SCOPED_LOCK(this->m_epoll_event_cs)
	{
		// 1) still the request of the post?
		is_current = (this->socket_state() == eSOCKET_STATE::SYN)
			&& (this->native_handle() == _pexecutable->m_fd)
			&& (this->m_seq_connecting.load(std::memory_order_acquire) == _pexecutable->m_seq)
			&& (this->m_result_connecting == eRESULT::REQUEST);

		// check) dropped
		RETURN_IF(is_current == false, false);

		// 2) the op (the completion : process_complete_connecting with the sequence)
		_pexecutable->m_pconnect_requestable = this;
		_pexecutable->m_is_op = true;

		if (auto pio_context = this->io_context(); pio_context.exist())
			is_submitted = pio_context->submit_connect(_pexecutable->m_fd, reinterpret_cast<const ::sockaddr*>(&_pexecutable->m_address), _pexecutable->m_address.length, _pexecutable);

		// check)
		if (is_submitted == false)
		{
			this->m_last_socket_error = api::get_last_error();
			_pexecutable->m_is_op = false;
			_pexecutable->m_pconnect_requestable.reset();
		}
	}

	// 3) the submit failed => a failure (like the poll reporting EPOLLERR)
	if (is_submitted == false)
		this->process_complete_connecting(static_cast<uintptr_t>(EPOLLERR | EPOLLHUP) | (static_cast<uintptr_t>(_pexecutable->m_seq) << CONNECT_SEQ_SHIFT));

	// return)
	return is_submitted;
}

bool net::io::connect_requestable::Ntcp::_process_end_complete_connect(bool _is_connected) noexcept
{
	SCOPED_LOCK(this->m_epoll_event_cs)
	{
		// 1) get the queued events
		const auto flag_queued = this->m_epoll_event_flag_queued;
		this->m_epoll_event_flag_queued = 0;

		// 2) failed
		if (_is_connected == false)
		{
			this->m_epoll_event_processing_now = false;
			return false;
		}

		// 3) not closed : set the socket executable
		//    (the handle is still this socket's. without this, a close in on_connect closed it and the set_event() replaced the executable of another socket given the same fd number : its connect stopped)
		if ((flag_queued & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) == 0)
		{
			// check)
			CGASSERT_ERROR(this->io_context().exist());

			// check)
			CGASSERT_ERROR(this->get_epoll_handle() != INVALID_SOCKET);

			// - attach to socket executor (io_uring completion : the first RECV op)
			auto result = this->process_start_io(this->io_context().get(), true);

			// - end
			this->m_epoll_event_processing_now = false;

			// return)
			return result;
		}
	}

	// 4) closed meanwhile (in on_connect, or by another thread while this one processed the connect) : completes the disconnect
	//    (like process_close_socket() does when no event is processed)
	this->process_complete_disconnect();

	SCOPED_LOCK(this->m_epoll_event_cs)
	{
		this->m_epoll_event_processing_now = false;
	}

	// return)
	return true;
}
#endif

sSOCKET_ENDPOINT CGDK::get_parameter_endpoint(const context& _context, std::string_view _parent_field_name, bool _enable_progress_log)
{
	// declare)
	sSOCKET_ENDPOINT result_endpoint;

	// 1) get field name
	std::string parent_field_name = _parent_field_name.empty() ? ""s : std::string(_parent_field_name) + ":";

	// 2) get 'flag' value
	if (const auto& context_find = _context["flag"]; context_find.is_valid())
	{
		// - get string
		std::string str_find = context_find;

		// - true면 재접속을 활성화한다.
		if (CGDK::__stricmp(str_find, "ai_passive") == 0
			|| CGDK::__stricmp(str_find, "passive") == 0)
		{
			result_endpoint.flag = AI_PASSIVE;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_PASSIVE'"sv);
		}
		else if (CGDK::__stricmp(str_find, "ai_canonname") == 0
			    || CGDK::__stricmp(str_find, "canonname") == 0)
		{
			result_endpoint.flag = AI_CANONNAME;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_CANONNAME'"sv);
		}
		else if (CGDK::__stricmp(str_find, "ai_numberichost") == 0
			    || CGDK::__stricmp(str_find, "numberichost") == 0)
		{
			result_endpoint.flag = AI_NUMERICHOST;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_NUMERICHOST'"sv);
		}
		else if (CGDK::__stricmp(str_find, "ai_v4mapped") == 0
				|| CGDK::__stricmp(str_find, "v4mapped") == 0)
		{
			result_endpoint.flag = AI_V4MAPPED;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_V4MAPPED'"sv);
		}
		else if (CGDK::__stricmp(str_find, "ai_all") == 0
				|| CGDK::__stricmp(str_find, "all") == 0)
		{
			result_endpoint.flag = AI_ALL;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_ALL'"sv);
		}
		else if (CGDK::__stricmp(str_find, "ai_addrconfig") == 0
			|| CGDK::__stricmp(str_find, "addrconfig") == 0)
		{
			result_endpoint.flag = AI_ADDRCONFIG;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'AI_ADDRCONFIG'"sv);
		}
		else if (CGDK::__stricmp(str_find, "default") == 0)
		{
			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: 'default'"sv);
		}
		// - 그 외의 값이면 잘못된 값이다.
		else
		{
			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "flag']: invalid parameter. use default value"sv);
		}
	}

	// 2) get 'address' value
	if (const auto& context_find = _context["address"sv]; context_find.is_valid())
	{
		// - set 'address'
		result_endpoint.str_address = (std::string&&)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "address']: '"sv << result_endpoint.str_address << "'"sv);
	}

	// 3) get 'port'
	if (const auto& context_find = _context["port"sv]; context_find.is_valid())
	{
		// - set 'service'
		result_endpoint.str_service = (std::string&&)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "port']: "sv << result_endpoint.str_service);
	}

	// 4) get 'service'
	if (const auto& context_find = _context["service"sv]; context_find.is_valid())
	{
		// - set 'service'
		result_endpoint.str_service = (std::string&&)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['" << parent_field_name << "service']: "sv << result_endpoint.str_service);
	}

	// return)
	return	result_endpoint;
}

CGDK::net::ip::tcp::endpoint CGDK::get_sockaddr(const sSOCKET_ENDPOINT& _socket_endpoint)
{
	// declare) 
	net::ip::tcp::resolver::results_type resolve_result;

	// 1) get local endpoint
	resolve_result = net::ip::tcp::resolver().resolve(
		_socket_endpoint.str_address,
		_socket_endpoint.str_service,
		_socket_endpoint.flag
	);

	// check) 
	THROW_IF(resolve_result.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, CGDK::stream_string(" error) invalid address or port for bind (address:", _socket_endpoint.str_address, ", service:", _socket_endpoint.str_service, ")")));

	// - get one
	net::ip::tcp::endpoint local_endpoint = *resolve_result.begin();

	// - set bind_any if str_address is nullptr 
	if (_socket_endpoint.str_address.empty())
	{
		local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6().any(), local_endpoint.port() };
	}

	// return) 
	return local_endpoint;
}

sSOCKET_CONNECT_REQUEST_PARAMETER CGDK::get_parameter_connection(const context& _context, bool _enable_progress_log)
{
	// declare) 
	sSOCKET_CONNECT_REQUEST_PARAMETER socket_connection_parameter;

	// 1) 'reconnection' value
	if (const auto& context_find = _context["reconnection"sv]; context_find.is_valid())
	{
		// - get string
		std::string str_find = context_find;

		// - true면 재접속을 활성화한다.
		if (CGDK::__stricmp(str_find, "true") == 0
		 || CGDK::__stricmp(str_find, "enable") == 0)
		{
			// -  reconnection을 enable한다.
			socket_connection_parameter.reconnection_enable	 = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(CGDK::__stricmp(str_find, "false") == 0
		     || CGDK::__stricmp(str_find, "disable") == 0)
		{
			socket_connection_parameter.reconnection_enable = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 true이다.)
		else if (CGDK::__stricmp(str_find, "default") == 0)
		{
			socket_connection_parameter.reconnection_enable = 0;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection']: 'default'[enable]"sv);
		}
		// - 그 외의 값이면 잘못된 값이다.
		else
		{
		}
	}

	// 2) reconnection_interval를 읽는다.
	if (auto& context_find = _context.find("reconnection_interval", "reconnection interval"); context_find.is_valid())
	{
		// - ReconnectionInterval값을 얻는다.
		std::size_t value_interval = context_find;

		// check) ReconnectionInterval값이 0보다 작거나 65536보다 크면 잘못된 것이다.
		if (value_interval > 0)
		{
			// log)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection_interval']: "sv << value_interval);

			// - 문자열을 ReconnectionInterval로 변경한다.
			socket_connection_parameter.reconnection_interval = chrono::seconds(value_interval);
		}
		else
		{
			// log) 
			LOG_ERROR << "  + parameter['reconnection_interval']: '"sv << static_cast<std::string&&>(context_find) << "' INVALID reconnection interval. default interval("sv << socket_connection_parameter.reconnection_interval.count() << " seconds) applied  "sv;
		}
	}

	// 3) remote address
	if (auto& context_find = _context.find("remote_endpoint", "remote endpoint"); context_find.is_valid())
	{
		auto remote_endpoint = get_parameter_endpoint(context_find, "remote_endpoint"sv, _enable_progress_log);

		// - get remote endpoint
		if (remote_endpoint.str_address.empty() == false || remote_endpoint.str_service != "0"sv)
		{
			// - peer address를 얻는다.
			net::ip::tcp::resolver::results_type resolve_result_remote;

			resolve_result_remote = net::ip::tcp::resolver().resolve(
				remote_endpoint.str_address,
				remote_endpoint.str_service,
				remote_endpoint.flag
			);

			// check) throw exception if resolve_result is empty
			THROW_IF(resolve_result_remote.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "remote end_point is invalid"sv))

			// - get peer address
			socket_connection_parameter.remote_endpoint = resolve_result_remote.begin()->endpoint();
		}
	}

	// 4) local address/bind address
	if (auto& context_find = _context.find("local_endpoint", "local endpoint", "bind_endpoint", "bind endpoint"); context_find.is_valid())
	{
		auto bind_endpoint = get_parameter_endpoint(context_find, "local_endpoint"sv, _enable_progress_log);

		// - get bind endpoint
		if (bind_endpoint.str_address.empty() == false || bind_endpoint.str_service != "0"sv)
		{
			// - peer address를 얻는다.
			net::ip::tcp::resolver::results_type resolve_result_bind;

			resolve_result_bind = net::ip::tcp::resolver().resolve(
				bind_endpoint.str_address,
				bind_endpoint.str_service,
				bind_endpoint.flag
			);

			// check) throw exception if resolve_result is empty
			THROW_IF(resolve_result_bind.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "bind end_point is invalid"sv))

			// - get peer address
			socket_connection_parameter.bind_endpoint = resolve_result_bind.begin()->endpoint();
		}
	}

	// 5) wait connecting completion
	if (auto& context_find = _context.find("wait_complete_connecting", "wait complete connecting"); context_find.is_valid())
	{
		// - get string
		std::string str_find = context_find;

		// - true면 재접속을 활성화한다.
		if (CGDK::__stricmp(str_find, "true") == 0
		 || CGDK::__stricmp(str_find, "enable") == 0)
		{
			// -  flag_wait_complete_connecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_connecting = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_connecting']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(CGDK::__stricmp(str_find, "false") == 0
		     || CGDK::__stricmp(str_find, "disable") == 0)
		{
			// -  flag_wait_complete_connecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_connecting = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_connecting']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 false이다.)
		else if (CGDK::__stricmp(str_find, "default") == 0)
		{
			// -  flag_wait_complete_connecting을 default로.
			socket_connection_parameter.flag_wait_complete_connecting = 0;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_connecting']: 'default'[disable]"sv);
		}
		// - 그 외의 값이면 잘못된 값이다.
		else
		{
		}
	}

	// 6) wait disconnecting completion
	if (auto& context_find = _context.find("wait_complete_disconnecting", "wait complete disconnecting"); context_find.is_valid())
	{
		// - get string
		std::string str_find = context_find;

		// - true면 재접속을 활성화한다.
		if (CGDK::__stricmp(str_find, "true") == 0
		 || CGDK::__stricmp(str_find, "enable") == 0)
		{
			// -  flag_wait_complete_disconnecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_disconnecting = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_disconnecting']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(CGDK::__stricmp(str_find, "false") == 0
		     || CGDK::__stricmp(str_find, "disable") == 0)
		{
			// -  flag_wait_complete_disconnecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_disconnecting = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_disconnecting']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 false이다.)
		else if (CGDK::__stricmp(str_find, "default") == 0)
		{
			// -  flag_wait_complete_disconnecting을 default로.
			socket_connection_parameter.flag_wait_complete_disconnecting = 0;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_disconnecting']: 'default'[disable]"sv);
		}
		// - 그 외의 값이면 잘못된 값이다.
		else
		{
		}
	}

	// return)
	return socket_connection_parameter;
}

bool CGDK::get_parameter_socket_option(const context& _context, sSOCKET_OPTION_TCP& _option, bool _enable_progress_log)
{
	//-----------------------------------------------------------------
	// "socket_option" : the tcp socket option of a socket (sSOCKET_OPTION_TCP)
	//
	//   "socket_option": { "tcp_nodelay": true, "tcp_quickack": true, "busy_poll": 50, "tcp_notsent_lowat": "64k", "tcp_fastopen": true }
	//
	//   - the keys not given keep the value of _option
	//   - a bool    : "true"/"false"/"enable"/"disable"/"1"/"0" ("default" : the default of the key)
	//   - an integer : a number, "64k", "1m" ("default"/"0" : not set)
	//   - an invalid value is warned and ignored
	//-----------------------------------------------------------------
	// 1) find "socket_option"
	const auto& context_option = _context.find("socket_option", "socket option", "tcp_option", "tcp option");

	// check)
	RETURN_IF(context_option.is_valid() == false, false);

	// - reads a bool value. false : invalid (warned)
	auto read_bool = [_enable_progress_log](const context& _context_value, std::string_view _name, bool _default, bool& _result) -> bool
	{
		std::string str_find = _context_value;
		auto value = __to_lower(std::string_view(str_find));

		if (value == "true" || value == "enable" || value == "1")
			_result = true;
		else if (value == "false" || value == "disable" || value == "0")
			_result = false;
		else if (value == "default")
			_result = _default;
		else
		{
			LOG_WARNING << "  ! parameter['socket_option:"sv << _name << "']: invalid value '"sv << str_find << "' (ignored)"sv;
			return false;
		}

		// trace)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['socket_option:"sv << _name << "']: "sv << (_result ? "true"sv : "false"sv));
		return true;
	};

	// - reads an integer value (a number, "64k", "1m". "default" : 0). false : invalid (warned)
	auto read_uint = [_enable_progress_log](const context& _context_value, std::string_view _name, uint32_t& _result) -> bool
	{
		std::string str_find = _context_value;
		auto value = __to_lower(std::string_view(str_find));

		int64_t number = 0;
		if (value != "default")
		{
			auto [pend, ec] = std::from_chars(value.data(), value.data() + value.size(), number);
			bool is_valid = (ec == std::errc()) && number >= 0;

			// - a suffix ('k' : KB, 'm' : MB)
			if (is_valid && pend != value.data() + value.size())
			{
				const std::string_view suffix(pend, value.data() + value.size() - pend);
				if (suffix == "k" || suffix == "kb")
					number *= 1024;
				else if (suffix == "m" || suffix == "mb")
					number *= 1024 * 1024;
				else
					is_valid = false;
			}

			if (is_valid == false)
			{
				LOG_WARNING << "  ! parameter['socket_option:"sv << _name << "']: invalid value '"sv << str_find << "' (ignored)"sv;
				return false;
			}
		}

		_result = static_cast<uint32_t>(std::min<int64_t>(number, UINT32_MAX));

		// trace)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['socket_option:"sv << _name << "']: "sv << _result);
		return true;
	};

	// 2) tcp_nodelay (TCP_NODELAY)
	if (const auto& context_find = context_option.find("tcp_nodelay", "tcp nodelay", "nodelay"); context_find.is_valid())
		read_bool(context_find, "tcp_nodelay"sv, DEFAULT_ENABLE_TCP_NODELAY, _option.tcp_nodelay);

	// 3) tcp_quickack (delayed ACK off)
	if (const auto& context_find = context_option.find("tcp_quickack", "tcp quickack", "quickack"); context_find.is_valid())
		read_bool(context_find, "tcp_quickack"sv, false, _option.tcp_quickack);

	// 4) tcp_fastopen (TCP Fast Open of a connecting socket)
	if (const auto& context_find = context_option.find("tcp_fastopen", "tcp fastopen", "fastopen"); context_find.is_valid())
		read_bool(context_find, "tcp_fastopen"sv, false, _option.tcp_fastopen);

	// 5) busy_poll (SO_BUSY_POLL, microseconds)
	if (const auto& context_find = context_option.find("busy_poll", "busy poll", "so_busy_poll"); context_find.is_valid())
		read_uint(context_find, "busy_poll"sv, _option.busy_poll);

	// 6) tcp_notsent_lowat (TCP_NOTSENT_LOWAT, bytes)
	if (const auto& context_find = context_option.find("tcp_notsent_lowat", "tcp notsent lowat", "notsent_lowat"); context_find.is_valid())
		read_uint(context_find, "tcp_notsent_lowat"sv, _option.tcp_notsent_lowat);

#if defined(_WINSOCK2API_)
	// check) linux only options
	EXECUTE_IF(_option.busy_poll != 0, LOG_WARNING << "  ! parameter['socket_option:busy_poll']: ignored on windows (linux only : SO_BUSY_POLL)"sv);
	EXECUTE_IF(_option.tcp_notsent_lowat != 0, LOG_WARNING << "  ! parameter['socket_option:tcp_notsent_lowat']: ignored on windows (linux only : TCP_NOTSENT_LOWAT)"sv);
#endif

	// return)
	return true;
}
