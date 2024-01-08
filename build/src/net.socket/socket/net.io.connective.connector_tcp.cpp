#include "pch.h"

using namespace CGDK;


class net::io::connective::connector::Ntcp::executable_connective :
	public						Iexecutable,
	public						Npoolable<executable_connective>,
	public						factory::_traits_system
{
public:
	virtual ~executable_connective() noexcept {}

	DEFAULT_FACTORY_NAME(net::io::connective::connector::Ntcp::executable_connective)

private:
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t /*_param*/) override
	{
	#if defined(_SYS_EPOLL_H)
		object_ptr<Iconnectable> pconnectable;
		object_ptr<Iconnective>	pconnective;

		scoped_lock(this->m_lockable_connective)
		{
			// check)
			RETURN_IF(this->m_pconnectable.empty(), 0);

			pconnectable = std::move(this->m_pconnectable);
			pconnective = std::move(this->m_pconnective);
		}

		// check)
		if (_result & EPOLLOUT)
		{
			_result = 0;
		}

		if (_result & (EPOLLHUP | EPOLLERR))
		{
			_result = 1;
		}

		// 1) process_connective_connect
		pconnective->process_connective_connect(pconnectable, _result);
	#else
		// 1) process_connective_connect
		this->m_pconnective->process_connective_connect(this->m_pconnectable, _result);
		
		// 2) reset
		this->m_pconnectable.reset();
		this->m_pconnective.reset(); 
	#endif
		
		// return) 
		return intptr_t(0);
	}
	virtual	void				on_final_release() noexcept override { this->m_pconnectable.reset(); this->m_pconnective.reset(); }

public:
	#if defined(_SYS_EPOLL_H)
			lockable<>			m_lockable_connective;
	#endif
			object_ptr<Iconnectable> m_pconnectable;
			object_ptr<Iconnective>	m_pconnective;
};

bool net::io::connective::connector::Ntcp::process_set_enable(bool _is_state)
{
	// 1) Disable로 상태를 바꾼다.
	int pre_state = this->m_is_enable.exchange((_is_state) ? 1 : 0);

	// check) 이전 상태가 Disable된 상태라면 그냥 끝낸다.
	RETURN_IF(pre_state != 0, false);

	// notify) message 전달하기
	net::io::connective::manager::instance()->transmit_message(sMESSAGE_NOTIFY((_is_state == true) ? eMESSAGE::NETWORK::SOCKET::CONNECTIVE_ENABLE : eMESSAGE::NETWORK::SOCKET::CONNECTIVE_DISABLE, 0, reinterpret_cast<uintptr_t>(static_cast<net::io::Iconnective*>(this))));

	// return) 
	return true;
}

bool net::io::connective::connector::Ntcp::process_get_enable() const
{
	return this->m_is_enable != 0;
}

result_code net::io::connective::connector::Ntcp::_process_starting(context& _context)
{
	// declare)
	bool enable_progress_log;

	try
	{
		// - 'enable progress_log
		if (_context.type() != eCONTEXT_VALUE_TYPE::BUFFER_)
		{
			// - find context
			if (std::string str_find = _context.find("progress_log"sv, "progress log"sv, "enable_progress_log"sv, "enable progress log"sv); str_find.empty() == false)
			{
				if (__stricmp(str_find, "true") == 0
					|| __stricmp(str_find, "enable") == 0)
				{
					this->m_enable_progress_log = true;
				}
				else if (__stricmp(str_find, "false") == 0
					|| __stricmp(str_find, "disable") == 0)
				{
					this->m_enable_progress_log = false;
				}
			}
		}

		enable_progress_log = this->m_enable_progress_log;

		// trace)
		EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <BEGIN> start CONNECTOR['"sv << name() << "']"sv);

		// 1) on_starting 함수를 호출한다.
		this->on_starting(_context);

		// 2) enable
		this->enable();

		// 3) JSON파일에서 읽어들여 socket객체를 생성한다.
		auto pcontext = _context["connection_list"sv];;

		if(pcontext.is_valid())
		{
			for(auto& iter: pcontext)
			{
				// log) 접속시도한 peer Address를 출력한다.
				EXECUTE_IF(enable_progress_log, LOG_INFO << "  + remote_endpoint['"sv << iter.first << "']"sv);

				try
				{
					// - socket을 만든다.
					auto pconnectable = this->alloc_connectable();

					// check) 
					THROW_IF(pconnectable.empty(), throwable::failure(eRESULT::FAIL_TO_CREATE));

					// - Inameable이면 name을 설정힌다.
					if (auto pnameable = dynamic_cast<Inameable*>(pconnectable.get()); pnameable != nullptr)
					{
						// - set socket name
						pnameable->set_name(iter.first.c_str());
					}

					// - connect를 수행한다.
					auto result = this->process_connecting(pconnectable, iter.second);

					// check) 
					THROW_IF(result == false, throwable::failure(result));
				}
				catch (throwable::failure& _e)
				{
					// log) 
					LOG_ERROR << "  @ (excp) fail to connecting '"sv << iter.first << "' (reason:"sv << to_string<char>(_e.reason<eRESULT>()) << ")"sv;
				}
				catch (...)
				{
					// log) 
					LOG_ERROR << "  @ (excp) fail to connecting '"sv << iter.first << "' "sv;
				}
			}
		}
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_starting' function call CONNECTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;

		// reraise)
		throw;
	}

	// trace)
	EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <END> start CONNECTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::connector::Ntcp::_process_stopping() noexcept
{
	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <BEGIN> stop CONNECTOR['"sv << name() << "']"sv);

	// 1) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call CONNECTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// 2) 먼저 Disable한다.
	this->disable();

	// 3) socket이 Reconnectable이라면 일단 모든 socket의 Reconnect를 Disable한다.
	this->_process_disable_reconnectable();

	// 4) 모든 Accepted된 socket을 종료한다.
	this->close_connectable_all();

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> stop CONNECTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::connector::Ntcp::_process_stop() noexcept
{
	// 1) io_context를 얻는다.
	auto pio_context = this->io_context();

	// 2) io_context가 null이면 기본 io_context를 얻는다.
	if(pio_context.empty())
	{
		pio_context = system_executor::executor();
	}

	// 3) io_context를 돌려 모든 IOCP가 완료 될 때까지 기다린다. (최대 MAX_LOOP만큼만 돈다.)
#if defined(_WINSOCK2API_)
	#ifndef NDEBUG
	// definitions) 
	const int MAX_LOOP = 1000;

	for(int count_loog = MAX_LOOP; count_loog > 0 && (statistics_get_now_async_io() != 0 || count() != 0); --count_loog)
	#else
	while(this->statistics_get_now_async_io() != 0 || count() != 0)
	#endif
#elif defined(_SYS_EPOLL_H)
	#ifndef NDEBUG
	// definitions) 
	const int MAX_LOOP = 1000;

	for (int count_loog = MAX_LOOP; count_loog > 0 && count() != 0; --count_loog)
	#else
	while (count() != 0)
	#endif
#endif
	{
		// - io_context를 돌려 IOCP의 I/O를 처리한다.
		pio_context->execute_all();

		// - socket이 Reconnectable이라면 일단 모든 socket의 reconnect를 disable한다.
		this->_process_disable_reconnectable();

		// - 닫는다.
		this->close_connectable_all();

		// - Sleep하기.
		api::sleep_for(10ms);
	}

	// 4) on_stop함수를 호출한다.
	try
	{
		this->on_stop();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stop' function call ACCEPTOR '" << name() << "' " __CGFUNCTION__ ""sv;
	}

	// check) 아직 IO처리 중일 경우...
	EXECUTE_IF(this->statistics_get_now_async_io() > 0, LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: listen soocket is exist yet.[prepared:"sv << statistics_get_now_async_io() << "]"sv);

	// check) 아직 접속 종료되지 않은 socket이 존재할 경우...
	EXECUTE_IF(this->count() != 0, LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: connected socket is exist yet[connected:"sv << count() << "]"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::connector::Ntcp::_process_destroy() noexcept
{
	// 1) on_destroy
	try
	{
		this->on_destroy();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroy' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// 2) 
	this->set_start_object(nullptr);

	// return)
	return eRESULT::SUCCESS;
}

void net::io::connective::connector::Ntcp::_process_disable_reconnectable()
{
	// declare) 
	Iconnectable* pconnectable = nullptr;

	// 1) Itrarator를 얻는다.
	auto pIterator = this->alloc_iterator();

	// 2) 전체를 돌며 disconnect 처리를 한다.
	while((pconnectable = pIterator->get_next()) != nullptr)
	{
		// 4) Reconnect Interface가 존재하는지 확인한다.
		auto preconnect = dynamic_cast<net::io::connect_requestable::Ntcp_reconnect*>(pconnectable);

		// check) 있다면 일단 Reconnect Cancel한다.
		if (preconnect != nullptr)
		{
			preconnect->disable_reconnect();
			preconnect->destroy();
		}
	}
}

object_ptr<net::io::Iconnectable> net::io::connective::connector::Ntcp::process_request_connect(context& _context_connect)
{
	// check) enable 여부를 확인한다.
	RETURN_IF(this->m_is_enable == false, object_ptr<net::io::Iconnectable>());

	// 1) socket을 만든다.
	auto pconnectable = this->alloc_connectable();

	// check) 
	RETURN_IF(pconnectable.empty(), object_ptr<net::io::Iconnectable>());

	// 2) connect를 수행한다.
	auto result = this->process_connecting(pconnectable, _context_connect);

	// check) 
	RETURN_IF(result == false, object_ptr<net::io::Iconnectable>());

	// return) 성공!!!
	return pconnectable;
}

object_ptr<net::io::Iconnectable> net::io::connective::connector::Ntcp::alloc_connectable()
{
	// 1) Allocated된 socket수를 늘린다.
	if(this->m_socket_count_allocated >= this->max_socket_count())
	{
		// return) nullptr을 리턴한다.
		return object_ptr<net::io::Iconnectable>();
	}

	// declare) connectable의 포인터
	object_ptr<net::io::Iconnectable> pconnectable;

	try
	{
		// 2) socket을 무조건 만든다.(기존에 있는것을 지운다.)
		pconnectable = this->process_alloc_connectable();

		// check) pconnectable가 nullptr이어서는 안된다.
		CGASSERT(pconnectable.exist(), object_ptr<net::io::Iconnectable>());

		// 4) on_alloc_connectable을 호출한다.
		this->on_alloc_connectable(pconnectable);
	}
	catch(std::exception& _e)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete  [reason:"sv << _e.what() << "] "sv << __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// - reset pconnectable
		pconnectable.reset();
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - reset pconnectable
		pconnectable.reset();
	}

	// return) 
	return pconnectable;
}

bool net::io::connective::connector::Ntcp::process_connecting( Iconnectable* _pconnectable, context& _context_connect) // const net::sockaddr& _address_peer
{
	// check)
	CGASSERT_ERROR(_pconnectable != nullptr);

	// check)
	RETURN_IF(_pconnectable == nullptr, false);

	//-----------------------------------------------------------------
	// 1. get parameter
	//-----------------------------------------------------------------
	// 1) ENDPOINT을 얻는다.
	sSOCKET_CONNECT_REQUEST_PARAMETER parameter;

	if (_context_connect.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		// - get parameter
		parameter = _context_connect.v.value_buffer.front<sSOCKET_CONNECT_REQUEST_PARAMETER>();
	}
	else
	{
		parameter = get_parameter_connection(_context_connect, true);
	}

	// check) throw exception if remote endpoint is empty
	RETURN_IF(parameter.remote_endpoint.empty(), false);

	// check) addres length가 0이면 안됀다.
	CGASSERT_ERROR(parameter.remote_endpoint.size() != 0)

	// check) SocketAddress의 길이가 0이 아닌가?
	CGASSERT(parameter.remote_endpoint.address().is_unspecified() == false, false);

	// check) SocketAddress의 길이가 0이면 안된다.
	RETURN_IF(parameter.remote_endpoint.address().is_unspecified(), false);


	//-----------------------------------------------------------------
	// 2. preparing
	//-----------------------------------------------------------------
	// check) m_address_family가 AF_UNSPEC이어서는 안된다.
	CGASSERT_ERROR(parameter.remote_endpoint.data().u_v4.sin_family != AF_UNSPEC);

	try
	{
		// 1) socket을 준비한다.
		_pconnectable->prepare_socket(parameter.remote_endpoint.data().u_v4.sin_family);

		// check) _pconnectable의 reference count를 1여야지만 한다.
		CGASSERT_ERROR(_pconnectable->reference_count() == 1);

		// check)
		CGASSERT(_pconnectable->is_valid_native_handle(), false);

		// 2) connectable manager를 nullptr로 설정한다.
		_pconnectable->m_flexible.get<net::io::Iconnectable_manager*>() = (net::io::Iconnectable_manager*)nullptr;

		// 3) set client socket
		_pconnectable->set_as_client_socket();

		// 4) port를 reuse하도록 한다.(중요:connect의 경우 pool로 동작할 때 socket address를 Reuse하지 않으면 error를 발생시킬수 있다.)
		api::socket::set_reuse_address(_pconnectable->native_handle(), true);
	}
	catch (throwable::failure& _e)
	{
		// - on fail connect
		this->on_fail_connect(_pconnectable, DISCONNECT_REASON_FAIL | _e.reason<uint64_t>());

		// return)
		return false;
	}
	catch (...)
	{
		// - on fail connect
		this->on_fail_connect(_pconnectable, DISCONNECT_REASON_FAIL);

		// return)
		return false;
	}


	//-----------------------------------------------------------------
	// 2. socket State SYN상태로 바꾸기.
	//-----------------------------------------------------------------
	// 1) socket상태를 SYN으로 바꾼다.
	[[maybe_unused]] bool is_changed = _pconnectable->set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

	// check) 이전 상태가 CLOSED가 아니면 안된다.
	EXECUTE_IF(is_changed != true, LOG_DEBUG << "(err ) CGDK.net.socket: state of socket is not 'CLOSED' " __CGFUNCTION__ ""sv);


	//-----------------------------------------------------------------
	// 4. 가로채기 함수 호출.
	//-----------------------------------------------------------------
	// 1) on_request_connect를 호출한다.
	this->on_request_connect(_pconnectable, _context_connect);

	// 2) get Istartablee interface
	auto pstartable = dynamic_cast<Nstartable*>(_pconnectable);

	// 3) connectable의 _process_starting 함수를 호출한다.
	if (pstartable != nullptr)
	{
		pstartable->_process_starting(_context_connect);
	}

	// 4) get Ireconnectable interface
	auto preconnectable = dynamic_cast<net::io::Ireconnectable*>(_pconnectable);

	// 5) reconnectable setting
	if (preconnectable != nullptr)
	{
		switch (parameter.reconnection_enable)
		{
			// case) enable reconnection
		case	2:
				{
					// - enable reconnection
					preconnectable->enable_reconnect();

					// - set reconnection interval
					if (parameter.reconnection_interval > chrono::tick::duration::zero())
					{
						preconnectable->set_reconnect_interval(parameter.reconnection_interval);
					}

					// - 모든 PeerAddress를 제거한다.
					preconnectable->remove_all_peer_address();

					// - 새로 들어온 PeerAddress를 추가한다.
					preconnectable->add_peer_address(parameter.remote_endpoint);

					// - get next peer address 
					preconnectable->move_next_peer_address();

					// - 접속 시도시 m_tick_disconnected 시간을 갱신한다.
					preconnectable->m_tick_disconnected = chrono::tick::now();

					// - Reconning을 false로 한다.
					preconnectable->m_flag_reconnecting = false;

				}
				break;

		// case) disable reconnection
		case	1:
				{
					preconnectable->disable_reconnect();
				}
				break;
	
		// case) not assigned
		default:
				break;
		}
	}

	// statistics) 
	this->statistics_on_try();

	// 6) connectable의 request_connect를 호출한다.
	_pconnectable->process_request_connect(parameter.remote_endpoint);


	//-----------------------------------------------------------------
	// 5. bind 한다.
	//-----------------------------------------------------------------
	{
		// 1) set socket_addr
		if (parameter.bind_endpoint.empty())
		{
			// - set default address
			if (parameter.remote_endpoint.is_v4())
			{
				parameter.bind_endpoint = end_point_type{ { net::ip::address_v4::any() } , 0 };
			}
			else
			{
				parameter.bind_endpoint = end_point_type{ { net::ip::address_v6::any() } , 0 };
			}
		}

		// 설명) windows의 경우 socket을 재활용할 수가 있기 때문에 이미 bind된 소켓이 있을 수 있다.
		//       이미 bind된 경우 connect에 실패하므로 새로운 socket handle을 생성해 bind를 한던 더 해준다.
		//       linux의 경우 socket의 재활용이 없이 접속 때마다 새로 생성하므로 bind에 실패하면 재시도 없이
		//       바로 error 처리한다.

		// declare) 
		int result = SOCKET_ERROR;

		// 2) bind socket
		result = api::socket::bind(_pconnectable->native_handle(), reinterpret_cast<::sockaddr*>(&parameter.bind_endpoint.data()), parameter.bind_endpoint.data().length);

	#if defined(_WINSOCK2API_)
		// check)
		if (result == SOCKET_ERROR)
		{
			// - get error code
			auto error_code = api::get_last_error();

			// - set last socket error
			_pconnectable->m_last_socket_error = error_code;

			// - re-alloc socket handle
			if (error_code == WSAEINVAL || error_code == WSAEADDRINUSE)
			{
				// - alloc new socket handle
				_pconnectable->process_create_socket(_pconnectable->address_family());

				// - try binding one more
				result = api::socket::bind(_pconnectable->native_handle(), (::sockaddr*)&parameter.bind_endpoint.data(), parameter.bind_endpoint.data().length);
			}
		}
	#endif

		// check) SUCCESSED!
		if (result == SOCKET_ERROR)
		{
		#if defined(_SYS_EPOLL_H)
			// - reset executable
			_pconnectable->process_reset_executable_socket();
		#endif
			// - set last error
			_pconnectable->m_last_socket_error = api::get_last_error();

			// trace) 
			LOG_DEBUG << "(err ) CGDK.net.socket: fail to bind socket [error_code:"sv << api::get_last_error() << "] " << __CGFUNCTION__ ""sv;

			// - close socket
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL_TO_SOCKET_BIND);

			// - socket status를 DISCONNECT상태로 다시...
			_pconnectable->socket_state(eSOCKET_STATE::CLOSED);

			// statistics) 
			statistics_on_fail_connect();

			// return) 
			return false;
		}
	}


	//-----------------------------------------------------------------
	// 6. connect를 수행한다.(IOCP)
	//-----------------------------------------------------------------
#if defined(_WINSOCK2API_)
	// 1) executable Accept 할당받기.
	auto pexecutable_connective = make_object<executable_connective>();

	// check) pexecutable_connective의 Reference count는 1여야지 한다.
	CGASSERT_ERROR(pexecutable_connective->reference_count() == 1);

	// 2) connective를 this로 설정한다.
	pexecutable_connective->m_pconnective = this;

	// 3) connectable을 설정힌다.
	pexecutable_connective->m_pconnectable = _pconnectable;

	// 4) connective에 add_ref()한다.
	pexecutable_connective->add_ref();

	// statistics) 
	this->statistics_on_increase_async();

	// 5) connect를 수행한다.
	auto result_connect = api::socket::connect_async(_pconnectable->native_handle(), pexecutable_connective.get(), reinterpret_cast<const ::sockaddr*>(&parameter.remote_endpoint.data()), parameter.remote_endpoint.data().length);

	// check) connect_async의 수행에 실패했는가?
	if(result_connect == false)
	{
		// - set last error
		_pconnectable->m_last_socket_error = api::get_last_error();

		// trace) 
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to connecting socket [error_code:"sv << api::get_last_error() << "] "sv <<  __CGFUNCTION__ ""sv;

		// - call process_connective_connect
		this->process_connective_connect(_pconnectable, 0);

		// - release connective
		pexecutable_connective->release();

		// return)
		return false;
	}

#elif defined(_SYS_EPOLL_H)
	// 1) allocate executable accept object
	auto pexecutable_connective = make_object<executable_connective>();

	// check)
	CGASSERT_ERROR(pexecutable_connective->reference_count() == 1);

	// 2) set connective
	pexecutable_connective->m_pconnective = this;
	pexecutable_connective->m_pconnectable = _pconnectable;

	// 3) set event
	auto result = _pconnectable->io_context()->attach(_pconnectable->native_handle(), pexecutable_connective, EPOLLONESHOT | EPOLLWAKEUP | EPOLLOUT | EPOLLERR);

	// check) 
	if (result == false)
	{
		// - call process_connective_connect
		this->process_connective_connect(_pconnectable, 1);

		// return)
		return false;
	}

	// 5) connect를 요청한다.
	auto result_connect = ::connect(_pconnectable->native_handle(), reinterpret_cast<const ::sockaddr*>(&parameter.remote_endpoint.data()), parameter.remote_endpoint.data().length);

	// 6-1) connection completed! - 접속까지 완료! (접속까지 완료된 상황이므로 접속 완료 처리를 해준다.)
	if (result_connect == 0)
	{
		// - call process_connective_connect
		this->process_connective_connect(_pconnectable, 0);
	}
	else
	{
		// - get last error code
		auto error_code = api::get_last_error();

		// 6-2) failed
		if (result_connect < 0 && (error_code != EINPROGRESS && error_code != EAGAIN))
		{
			// - set last error
			_pconnectable->m_last_socket_error = error_code;

			// trace) 
			LOG_DEBUG << "(err ) CGDK.net.socket: fail to connecting socket  [error_code:"sv << error_code << "] "sv << __CGFUNCTION__ ""sv;

			// - call process_connective_connect
			this->process_connective_connect(_pconnectable, 1);

			// return)
			return false;
		}
	}
#else
	#error	"unsupported network API"
#endif

	// return) 
	return true;
}

void net::io::connective::connector::Ntcp::process_connective_connect( Iconnectable* _pconnectable, uintptr_t _result)
{
	// check) 
	CGASSERT_ERROR(_pconnectable != nullptr);

	// check) 
	THROW_IF(_pconnectable == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) reconnect Interface가 존재하는지 확인한다.
	auto preconnect = dynamic_cast<net::io::connect_requestable::Ntcp_reconnect*>(_pconnectable);

	// check) 있다면 일단 Reconnect Cancel한다.
	if(preconnect != nullptr)
	{
		preconnect->process_cancel_reconnector();
	}

	// check) 접속에 성공했으면 
	if((_result == 0) && m_is_enable && (api::socket::is_socket_count_overflowed() == false))
	{
		try
		{
		#if defined(_WINSOCK2API_)
			// 1) Update connect Context 옵션을 설정한다.
			{
				// - call 'update_connect_context'
				auto result = api::socket::update_connect_context(_pconnectable->native_handle());

				// check) 
				if (result == false)
				{
					// - set last error
					_pconnectable->m_last_socket_error = api::get_last_error();

					// log) 
					LOG_DEBUG << "(err ) CGDK.net.socket: fail to setting 'SO_UPDATE_CONNECT_CONTEXT' option  [error_code:"sv << api::get_last_error() << "] "sv << __CGFUNCTION__ ""sv;

					// raise) 
					throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to setting socket option 'SO_UPDATE_CONNECT_CONTEXT'"sv);
				} 
			}

			// 2) enable 'fast loopback'
			api::socket::set_loopback_fast_path(_pconnectable->native_handle(), true);

		#endif

			// 3) call 'on_connect'
			this->on_connect(_pconnectable);

			// 4) set connective를 this로 설정한다.
			this->_process_connectable_connective_set(_pconnectable);

			// 설명) 여기서부터는 접속 성공으로 보기 때문에 이후의 오류는 disconnect 처리된다.

			// statistics) 
			this->statistics_on_success_connect();

			// 5) process_complete_connect를 수행한다.
			auto result_complete = _pconnectable->process_complete_connect(0);

			// check) rollback
			if (result_complete == true)
			{
				// 6) connected list에 추가한다.
				//    주의) add_connectable은 반드시 set_event 이전에 호출되어야 한다.
				//          그러지 않으면 add_connectable 진행중 다른 쓰레드에서 disconnect와 같은 다른 처리가 진행되어 문제가 될수 있다.
				{
					// - call 'add_connectable'
					auto result = this->add_connectable(_pconnectable);

					// check)
					CGASSERT_ERROR(result == true);

					// check)
					THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
				}

				#if defined(_SYS_EPOLL_H)
				{
					// check)
					CGASSERT_ERROR(_pconnectable->io_context().exist());

					// 6) attach to socket io_context
					auto result = _pconnectable->io_context()->set_event((CGHANDLE)_pconnectable->get_epoll_handle(), _pconnectable->m_epoll_event_flag, _pconnectable->m_pexecutable_socket);

					// check)
					THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
				}
				#endif
			}
			else
			{
				// - roll back (reset m_pconnectable_notify)
				this->_process_connectable_connective_reset(_pconnectable);
			}
		}
		catch(std::exception& e)
		{
			// - remove connectable
			this->remove_connectable(_pconnectable);

			// - roll back (reset m_pconnectable_notify)
			this->_process_connectable_connective_reset(_pconnectable);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// statistics) 
			this->statistics_on_fail_connect();
		}
		catch(...)
		{
			// - remove connectable
			this->remove_connectable(_pconnectable);

			// - roll back (reset m_pconnectable_notify)
			this->_process_connectable_connective_reset(_pconnectable);

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// statistics) 
			this->statistics_on_fail_connect();
		}
	}
	else
	{
		try
		{
			// 1) on_fail_connect를 호출한다.
			this->on_fail_connect(_pconnectable, DISCONNECT_REASON_FAIL);

			// 2) process_close_socket한다.
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL);

			// 3) process_complete_connect를 수행한다.
			_pconnectable->process_complete_connect(1);
		}
		catch(std::exception& _e)
		{
			// trace) 
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete [reason:"sv << _e.what() << "] "sv << __CGFUNCTION__ << ""sv;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, _e, __FUNCTION__);
		}
		catch(...)
		{
			// trace) 
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__ ""sv;

			// declare)
			throwable::failure	e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}

		// statistics)
		this->statistics_on_fail_connect();

		// 5) try reconnect if reconnectable
		if (preconnect != nullptr)
		{
			preconnect->process_request_reconnect();
		}
	}

	// statistics) 
	this->statistics_on_decrese_async();
}

void net::io::connective::connector::Ntcp::process_connective_disconnect( Iconnectable* _pconnectable)
{
	// 1) remove pconnectable
	this->remove_connectable(_pconnectable);

	// 2) reset m_pconnectable_notify
	this->_process_connectable_connective_reset(_pconnectable);

	// statistics)
	this->statistics_on_error_disconnect(_pconnectable->statistics_get_error_disconnected());

	// statistics)
	this->statistics_on_disconnect();

	// 4) get reconnect interface
	auto preconnect = dynamic_cast<net::io::connect_requestable::Ntcp_reconnect*>(_pconnectable);

	// 5) cancel reconnecting
	if (preconnect != nullptr)
	{
		preconnect->process_request_reconnect();
	}
}

void net::io::connective::connector::Ntcp::_process_connectable_connective_set(Iconnectable* _pconnectable) noexcept
{
	// 1) set connective
	_pconnectable->m_pconnectable_notify = this;

	// 2) increase allocated socket count
	++this->m_socket_count_allocated;
}

void net::io::connective::connector::Ntcp::_process_connectable_connective_reset(Iconnectable* _pconnectable) noexcept
{
	// 1) reset connective
	_pconnectable->m_pconnectable_notify = nullptr;

	// 2) decrease allocated socket count
	--this->m_socket_count_allocated;
}
