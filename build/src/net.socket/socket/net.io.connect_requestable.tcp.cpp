#include "pch.h"

using namespace CGDK;


net::io::connect_requestable::Ntcp::Ntcp() :
	Nnameable(),
	m_result_connecting(eRESULT::NOT_DEFINED)
{
	this->register_binder(this);
}

net::io::connect_requestable::Ntcp::Ntcp(std::string_view _name) :
	Nnameable(_name),
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
	auto tick_end = chrono::tick::now()+_duration_wait;

	// 4) wait
	auto pthread_now = api::get_current_thread();

	// check)  
	CGASSERT_ERROR(pthread_now.exist());

	// 5) notify  blocked
	pthread_now->notify_blocked();

	try
	{
		// 6) wait util connecting process is completed
		while(this->m_pconnectable_notify != nullptr)
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
		if (std::string str_find = _context.find("progress_log"sv, "progress log"sv, "enable_progress_log"sv, "enable progress log"sv); str_find.empty() == false)
		{
			if (__stricmp(str_find, "true") == 0
				|| __stricmp(str_find, "enable") == 0)
			{
				parameter.enable_progress_log = true;
			}
			else if (__stricmp(str_find, "false") == 0
				|| __stricmp(str_find, "disable") == 0)
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
			if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
			{
				parameter = _context.v.value_buffer.front<START_PARAMETER>();
			}
			else
			{
				// - socket setting을 읽어낸다.
				parameter = get_parameter_connection(_context, parameter.enable_progress_log);
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
				LOG_ERROR << NFMT::format("   (err ) invalid remote_endpoint port (port number is '0')");

				// raise)
				throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "invalid remote_endpoint port (port number is '0')"sv);
			}

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
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_DEBUG << "(excp) exception occure on '_process_starting' ['"sv << pname << "']"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - get name
		auto* pnameable = dynamic_cast<Inameable*>(this);
		auto pname = (pnameable != nullptr && pnameable->name().empty() == false) ? pnameable->name() : std::string_view();

		// log) 
		LOG_DEBUG << "(excp) exception occure on '_process_starting' ['"sv << pname << "']"sv;

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

	// 2) call '_process_destroying_self_complete'
	result = this->_process_stopping_child();

	// 3) wait until completed
	if (m_enable_block_until_disconnecting_completed)
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
	// check) _address의 내용이 설정되어 있지 않으면 안됀다.
	CGASSERT(_address.is_unspecified() == false, false)

	// check) _address의 내용이 설정되어 있지 않으면 안됀다.
	THROW_IF(_address.is_unspecified() == true, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check) 
	RETURN_IF(api::socket::is_socket_count_overflowed(), false);


	//-----------------------------------------------------------------
	// 1. 상태변경
	//
	// - 상태변경을 하면서 이미 connect상태이거나 connect를 시도한 상태인지
	//   를 구분하여 만약 이미 connect를 시도한 상태이거나 connect상태이면
	//   바로 Return할수 있도록 처리를 해주어야 한다.
	// - 이를 위해 atomic를 사용하여 SocketState를 변경하여 그 이전
	//   값을 Check하여 처리한다.
	//-----------------------------------------------------------------
	// 1) socket상태를 eSOCKET_STATE.SYN으로 바꾼다.
	//    - 내부적으로 atomic함수를 통해 CLOSED 상태로 변경한다.
	//    - 여기서 atomic함수를 사용하는 이유는 상황에 따라 중복적으로 
	//      connect와 disconnect등 socket의  상태가 불려지는 함수가 겹쳐
	//      서 호출될수 있기 때문이다. 
	//    - socket의 상태가 CLOSED일 때만 connect함수가 수행되고 아닐 경우 
	//      더 이상 진행하지 않고 false를 리턴한다.
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
	// check) addres length가 0이면 안됀다.
	CGASSERT_ERROR(_address.length != 0)

	// statistics) 접속 시도 횟수
	this->statistics_on_connect_try();

	// 1) connect를 요청한다.
	auto result = ::connect(handle_socket, reinterpret_cast<const ::sockaddr*>(&_address), _address.length);

	// 2-1) 바로 접속 완료! - 접속까지 완료! (접속까지 완료된 상황이므로 접속 완료 처리를 해준다.)
	if (result == 0)
	{
		// - 즉시 process_complete_connecting를 수행한다.
		this->process_complete_connecting(EPOLLOUT);
	}
	else
	{
		// get last error code
		auto error_code = api::get_last_error();

		// 2-2) Pending : 비동기적으로 connect가 처리 중임.
		if(result < 0 && (error_code == EINPROGRESS || error_code == EAGAIN))
		{
			// - executor를 생성한다.
			this->process_prepare_executable_socket();

			// - executable을 생성한다.
			auto pexecutable_connector = make_object<net::io::executable_connect_requestable>();

			// - connectable을  추가한다.
			pexecutable_connector->set_connectable(this);

			// - 새로 생성한 socket을 executor에 붙인다.
			auto result = io_context()->attach(handle_socket, pexecutable_connector, EPOLLONESHOT | EPOLLWAKEUP | EPOLLOUT | EPOLLERR);

			// check) set_event에 실패했을 경우 Release한다.
			RETURN_IF(result == false, false);
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
		// - reesult를 eRESULT::SUCCESS로
		this->m_result_connecting = eRESULT::SUCCESS;
	}
	else
	{
		// - reesult를 eRESULT::NOT_DEFINED로
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
	// cheeck) 중복처리 중이면 그냥 끝낸다.(epoll은 중복으로 들어 올 수 있다.)
	scoped_lock(this->m_epoll_event_cs)
	{
		// check)
		RETURN_IF(this->m_result_connecting != eRESULT::REQUEST);

		// - 처리된 것임을 표시한다.
		this->m_result_connecting = eRESULT::PROCESS;
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
	// 1) process_complete_connect를 수행함.
	auto result = this->process_complete_connect(_result);

	// 2) do event 
	if(result == true)
	{
		// - reesult를 eRESULT::SUCCESS로
		this->m_result_connecting = eRESULT::SUCCESS;

		// check)
		CGASSERT_ERROR(this->io_context().exist());

		// check)
		CGASSERT_ERROR(this->get_epoll_handle() != INVALID_SOCKET);

		// - attach to socket eexecutor
		auto result = this->io_context()->set_event(get_epoll_handle(), this->m_epoll_event_flag, m_pexecutable_socket);

		// - result를 설정한다.
		this->m_result_connecting = (result == true) ? eRESULT::SUCCESS : eRESULT::FAIL;
	}
	else
	{
		// - reesult를 eRESULT::NOT_DEFINED로
		this->m_result_connecting = eRESULT::NOT_DEFINED;

		// - connective를 reset한다.
		this->m_pconnectable_notify = nullptr;
	}
#endif
}

sSOCKET_ENDPOINT CGDK::get_parameter_endpoint(const context& _context, bool _enable_progress_log)
{
	// declare)
	sSOCKET_ENDPOINT result_endpoint;

	// 1) get 'flag' value
	if (std::string context_find = _context["flag"sv]; context_find.empty() == false)
	{
		// - true면 재접속을 활성화한다.
		if (__stricmp(context_find, "ai_passive"sv) == 0
			|| __stricmp(context_find, "passive"sv) == 0)
		{
			result_endpoint.flag = AI_PASSIVE;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_PASSIVE'"sv);
		}
		else if (__stricmp(context_find, "ai_canonname") == 0
			    || __stricmp(context_find, "canonname") == 0)
		{
			result_endpoint.flag = AI_CANONNAME;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_CANONNAME'"sv);
		}
		else if (__stricmp(context_find, "ai_numberichost"sv) == 0
			    || __stricmp(context_find, "numberichost"sv) == 0)
		{
			result_endpoint.flag = AI_NUMERICHOST;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_NUMERICHOST'"sv);
		}
		else if (__stricmp(context_find, "ai_v4mapped"sv) == 0
				|| __stricmp(context_find, "v4mapped"sv) == 0)
		{
			result_endpoint.flag = AI_V4MAPPED;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_V4MAPPED'"sv);
		}
		else if (__stricmp(context_find, "ai_all"sv) == 0
				|| __stricmp(context_find, "all"sv) == 0)
		{
			result_endpoint.flag = AI_ALL;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_ALL'"sv);
		}
		else if (__stricmp(context_find, "ai_addrconfig"sv) == 0
			|| __stricmp(context_find, "addrconfig"sv) == 0)
		{
			result_endpoint.flag = AI_ADDRCONFIG;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'AI_ADDRCONFIG'"sv);
		}
		else if (__stricmp(context_find, "default"sv) == 0)
		{
			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: 'default'"sv);
		}
		// - 그 외의 값이면 잘못된 값이다.
		else
		{
			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['flag']: invalid parameter. use default value"sv);
		}
	}

	// 2) get 'address' value
	if (auto& context_find = _context["address"sv]; context_find.is_valid())
	{
		// - set 'address'
		result_endpoint.str_address = (std::string)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['address']: '"sv << result_endpoint.str_address << "'"sv);
	}

	// 3) get 'port'
	if (auto& context_find = _context["port"sv]; context_find.is_valid())
	{
		// - set 'service'
		result_endpoint.str_service = (std::string)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['port']: "sv << result_endpoint.str_service);
	}

	// 4) get 'service'
	if (auto& context_find = _context["service"sv]; context_find.is_valid())
	{
		// - set 'service'
		result_endpoint.str_service = (std::string)context_find;

		// log)
		EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['service']: "sv << result_endpoint.str_service);
	}

	// return)
	return	result_endpoint;
}

sSOCKET_CONNECT_REQUEST_PARAMETER CGDK::get_parameter_connection(const context& _context, bool _enable_progress_log)
{
	// declare) 
	sSOCKET_CONNECT_REQUEST_PARAMETER socket_connection_parameter;

	// 1) 'reconnection' value
	if (std::string str_find = _context["reconnection"sv]; str_find.empty() == false)
	{
		// - true면 재접속을 활성화한다.
		if (__stricmp(str_find, "true"sv) == 0
		 || __stricmp(str_find, "enable"sv) == 0)
		{
			// -  reconnection을 enable한다.
			socket_connection_parameter.reconnection_enable	 = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(__stricmp(str_find, "false"sv) == 0
		     || __stricmp(str_find, "disable"sv) == 0)
		{
			socket_connection_parameter.reconnection_enable = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['reconnection']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 true이다.)
		else if (__stricmp(str_find, "default"sv) == 0)
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
	if (auto& context_find = _context.find("reconnection_interval"sv, "reconnection interval"sv); context_find.is_valid())
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
			LOG_ERROR << "  + parameter['reconnection_interval']: '"sv << static_cast<std::string>(context_find) << "' INVALID reconnection interval. default interval("sv << socket_connection_parameter.reconnection_interval.count() << " seconds) applied  "sv;
		}
	}

	// 3) remote address
	if (auto& context_find = _context.find("remote_endpoint"sv, "remote endpoint"sv); context_find.is_valid())
	{
		auto remote_endpoint = get_parameter_endpoint(context_find, _enable_progress_log);

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
	if (auto& context_find = _context.find("local_endpoint"sv, "local endpoint"sv, "bind_endpoint"sv, "bind endpoint"sv); context_find.is_valid())
	{
		auto bind_endpoint = get_parameter_endpoint(context_find, _enable_progress_log);

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
	if (std::string str_find = (std::string)_context.find("wait_complete_connecting"sv, "wait complete connecting"sv); str_find.empty() == false)
	{
		// - true면 재접속을 활성화한다.
		if (__stricmp(str_find, "true"sv) == 0
		 || __stricmp(str_find, "enable"sv) == 0)
		{
			// -  flag_wait_complete_connecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_connecting = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_connecting']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(__stricmp(str_find, "false"sv) == 0
		     || __stricmp(str_find, "disable"sv) == 0)
		{
			// -  flag_wait_complete_connecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_connecting = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_connecting']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 false이다.)
		else if (__stricmp(str_find, "default"sv) == 0)
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
	if (std::string str_find = (std::string)_context.find("wait_complete_disconnecting"sv, "wait complete disconnecting"sv); str_find.empty() == false)
	{
		// - true면 재접속을 활성화한다.
		if (__stricmp(str_find, "true"sv) == 0
		 || __stricmp(str_find, "enable"sv) == 0)
		{
			// -  flag_wait_complete_disconnecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_disconnecting = 2;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_disconnecting']: 'enable'"sv);
		}
		// - false면 재접속을 비활성화한다.
		else if(__stricmp(str_find, "false"sv) == 0
		     || __stricmp(str_find, "disable"sv) == 0)
		{
			// -  flag_wait_complete_disconnecting을 enable한다.
			socket_connection_parameter.flag_wait_complete_disconnecting = 1;

			// trace)
			EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + parameter['wait_complete_disconnecting']: 'disable'"sv);
		}
		// - default면 기본 값을 사용한다. (기본값은 false이다.)
		else if (__stricmp(str_find, "default"sv) == 0)
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
