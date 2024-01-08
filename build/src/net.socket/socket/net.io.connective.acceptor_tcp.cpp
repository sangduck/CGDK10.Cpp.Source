#include "pch.h"

namespace CGDK
{

extern std::size_t g_max_accept_socket;

#if defined(_WINSOCK2API_)
class net::io::connective::acceptor::Ntcp::executable_connective :
	public						Iexecutable,
	public						Npoolable<executable_connective>,
	public						factory::_traits_system
{
public:
	virtual ~executable_connective() noexcept {}

public:
	void* get_connectable_buffer() noexcept { return reinterpret_cast<void*>(this->m_buffer_connectable); }

	DEFAULT_FACTORY_NAME(net::io::connective::acceptor::Ntcp::executable_connective)

private:
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t /*_param*/) override
	{
		// - call 'process_connective_connect'
		this->m_pconnective->process_connective_connect(this->m_pconnectable, _result);
		
		// - reset
		this->m_pconnectable.reset();
		this->m_pconnective.reset(); 
	
		// return)
		return intptr_t(0);
	}

	// 3) on_final_release함수를 재정의한다.
	virtual	void				on_final_release() noexcept override { this->m_pconnectable.reset(); this->m_pconnective.reset(); }

public:
			object_ptr<Iconnectable> m_pconnectable;
			object_ptr<Iconnective>	m_pconnective;
			char				m_buffer_connectable[256]{};
};
#elif defined(_SYS_EPOLL_H)
class net::io::connective::acceptor::Ntcp::executable_accept :
	// inherited classes)
	public						Iexecutable,
	public						Npoolable<executable_accept>,
	public						factory::_traits_system
{
	// constructor)
public:
	
	virtual ~executable_accept() noexcept {}

	// Define Pool Name)
	DEFAULT_FACTORY_NAME(net::io::connective::acceptor::Ntcp::executable_accept)

		// implementations) 
private:
	// 1) I/O가 성공적으로 완료 되었을 때 불려지는 함수.
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t /*_param*/) override { this->m_pconnective->process_connective_accept(_result); return intptr_t(0); }

public:
			net::io::connective::acceptor::Ntcp* m_pconnective = nullptr;
};
#endif

net::io::connective::Iacceptor::end_point_type net::io::connective::acceptor::Ntcp::get_accept_endpoint() const noexcept
{
	// declare) 
	end_point_type temp_local_endpoint;

	// 1) 설정된 accept_endpoint가 존재하면 그것을 endpoint로 한다.
	if(this->m_accept_endpoint.exist())
	{
		// - 설정된 accept_endpoint를 복사한다.
		temp_local_endpoint = this->m_accept_endpoint;

		// - 설정된 accept_endpoint의 port값이 0이면 현재 bind된 port를 설정한다.
		if(temp_local_endpoint.port() == 0)
		{
			temp_local_endpoint.port(local_endpoint().port());
		}
	}
	// 2) 설정된 accept_endpoint가 없으면 
	else
	{
		// - socket Address를 얻는다.
		temp_local_endpoint = local_endpoint();

		// - bind_any일 경우 local address를 얻어서 설정한다.
		if (temp_local_endpoint.address().is_unspecified())
		{
			// declare) 
			net::sockaddr sockaddr_temp;

			// - Public Address를 얻는다.
			api::socket::instance().get_local_address_public_first(sockaddr_temp);

			// - Port만 읽어온다.
			sockaddr_temp.port(temp_local_endpoint.port());

			// - 값을 그대로 복사
			temp_local_endpoint = sockaddr_temp;
		}
	}

	// return)
	return temp_local_endpoint;
}

void net::io::connective::acceptor::Ntcp::set_accept_endpoint(const end_point_type& _local_endpoint) noexcept
{
	this->m_accept_endpoint = _local_endpoint;
}
void net::io::connective::acceptor::Ntcp::reset_accept_endpoint() noexcept
{
	this->m_accept_endpoint.clear();
}

void net::io::connective::acceptor::Ntcp::process_set_parameters(context& /*_context*/)
{
}

#if defined(_SYS_EPOLL_H)
object_ptr<net::io::socket::Ntcp> net::io::connective::acceptor::Ntcp::_process_starting_uds(const START_PARAMETER& /*_parameter*/)
{
	//// 1) create socket object for UDS
	//auto psocket_uds = make_object<net::io::socket::Ntcp>();

	//// 2) prepare UDS socket
	//psocket_uds->process_prepare_socket(PF_FILE);

	//// 3) get UDS socket handle
	//CGSOCKET socket_UDS_handle_listen = psocket_uds->native_handle();

	//// 4) generate path for UDS bind
	//std::string path_uds = "uds_stream_20000";

	//// declare) 
	//sockaddr_in addr_uds;
	//__zero_memory(&addr_uds, sizeof(addr_uds));
	//addr_uds.sun_family = AF_UNIX;
	//strcpy(addr_uds.sun_path, path_uds.c_str());

	//// 5) bind
	//auto result = api::socket::bind(socket_UDS_handle_listen, (::sockaddr*)&addr_uds, sizeof(addr_uds));

	//// check)
	//ERROR_RETURN_IF(result == SOCKET_ERROR, object_ptr<net::io::socket::Ntcp>(), LOG_ERROR << "@ (err ) ACCEPTOR['"sv << name() << "'] fail to binding socket for listen. error:"sv << api::get_last_error_string() << ", "sv << api::get_last_error() <<" " __CGFUNCTION__ ""sv);

	//// trace)
	//EXECUTE_IF(this->m_enable_progress_log,LOG_INFO << "  > UDS socket binded on "sv << path_uds);

	//// 6) start listening
	//result = api::socket::listen(socket_UDS_handle_listen);

	//// check) 
	//ERROR_RETURN_IF(result == false, , LOG_ERROR << "@ (error) ACCEPTOR['"sv << name() << "'] fail to listen for UDS socket. error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv);

	//// trace)
	//EXECUTE_IF(this->m_enable_progress_log,LOG_INFO << fmt::format("  > listen on " << path_uds); 

	return object_ptr<net::io::socket::Ntcp>();
}
#endif

result_code net::io::connective::acceptor::Ntcp::_process_starting(context& _context)
{
	// check) socket 상태가 CLOSED여야지만 한다.
	CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

	// declare) 
	START_PARAMETER parameter;

	// 1) set default values
	parameter.enable_progress_log = this->m_enable_progress_log;

	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		// - get parameter (use 'enable_progress_log' only)
		parameter = _context.v.value_buffer.front<START_PARAMETER>();
	}
	else
	{
		// - 'enable progress_log
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
	}

	// 2) set...
	this->m_enable_progress_log = parameter.enable_progress_log;

	// trace)
	EXECUTE_IF(parameter.enable_progress_log, LOG_PROGRESS << "@ <BEGIN> start ACCEPTOR['"sv << name() << "']"sv);



	//-----------------------------------------------------------------
	// 1. call 'on_starting'
	//-----------------------------------------------------------------
	// 1) on_initializing 함수를 호출한다.
	this->on_starting(_context);


	//-----------------------------------------------------------------
	// 2. read parameter
	//-----------------------------------------------------------------
	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
	{
		// - get parameter (reload)
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - casting to int64_t
		int64_t prepare_on_start = static_cast<int64_t>(parameter.prepare_on_start);
		int64_t must_prepare = static_cast<int64_t>(parameter.must_prepare);

		// trace)
		if(parameter.enable_progress_log)
		{
			if(prepare_on_start > 0)
				LOG_INFO << "  + parameter['prepare on start']: "sv << prepare_on_start;
			else if (prepare_on_start < 0)
				LOG_INFO << "  + parameter['prepare on start']: 'core' x "sv << prepare_on_start;
			else
				LOG_INFO << "  + parameter['prepare on start']: default"sv;

			// trace)
			if(must_prepare > 0)
				LOG_INFO << "  + parameter['must prepare']: "sv << must_prepare;
			else if (must_prepare < 0)
				LOG_INFO << "  + parameter['must prepare']: 'core' x "sv << must_prepare;
			else
				LOG_INFO << "  + parameter['must prepare']: default"sv;
		}
	}
	else
	{

		// 1) local address
		if (auto context_find = _context.find("local_endpoint"sv, "local endpoint"sv); context_find.is_valid())
		{
			// declare) 
			net::ip::tcp::resolver::results_type resolve_result;

			// - get endpoint
			auto listen_endpoint = get_parameter_endpoint(context_find, true);

			// - set local endpoint
			resolve_result = net::ip::tcp::resolver().resolve(
				listen_endpoint.str_address,
				listen_endpoint.str_service,
				listen_endpoint.flag
			);

			// check) 
			if (resolve_result.empty())
			{
				// log)
				LOG_ERROR << " error) invalid address or port for bind (address:"sv << listen_endpoint.str_address << ", service:"sv << listen_endpoint.str_service << ")"sv;

				// trhwo) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}

			// - get one
			parameter.local_endpoint = *resolve_result.begin();

			// - set bind_any if str_address is nullptr 
			if (listen_endpoint.str_address.empty())
			{
				parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6().any(), parameter.local_endpoint.port() };
			}
		}

		// 2) accept address
		if (auto context_find = _context.find("accept_endpoint"sv, "accept endpoint"sv); context_find.is_valid())
		{
			// declare) 
			net::ip::tcp::resolver::results_type resolve_result;

			// - get endpoint
			auto accept_endpoint = get_parameter_endpoint(context_find, true);

			// - set accept endpoint
			resolve_result = net::ip::tcp::resolver().resolve(
				accept_endpoint.str_address,
				accept_endpoint.str_service,
				accept_endpoint.flag
			);

			// check) 
			if (resolve_result.empty())
			{
				// log)
				LOG_ERROR << " error) invalid address or port for bind (address:"sv << accept_endpoint.str_address << ", service:"sv << accept_endpoint.str_service << ")"sv;

				// throw) 
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
			}

			// - get one
			parameter.accept_endpoint = *resolve_result.begin();

			// - if str_address is nullptr, set address as public ip in host
			if (parameter.accept_endpoint.address().is_loopback())
			{
				parameter.accept_endpoint.address(get_accept_endpoint().address());
			}

			// - if port is zero, set port as listen port
			if (parameter.accept_endpoint.port() == 0)
			{
				parameter.accept_endpoint.port(parameter.local_endpoint.port());
			}

			// trace)
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['acceptable endpoint']: "sv << to_string<char>(parameter.accept_endpoint) << ":" << parameter.accept_endpoint.port());
		}

		// 4) [Optional] <prepare on start>값 읽기 (default = 0)
		if (std::string str_find = (std::string)_context.find("prepare_on_start", "prepare on start"); str_find.empty() == false)
		{
			// - "default"면 default값으로 설정
			if (__stricmp(str_find, "default") == 0)
			{
				parameter.prepare_on_start = 0;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: 'default'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if(str_find.size() != 0 && (str_find[0] == 'x' || str_find[0] == 'X' || str_find[0] == '*'))
			{
				// - get multiply value
				int64_t prepare_on_start = __from_string<int64_t>(str_find.substr(1));

				// - set value
				parameter.prepare_on_start = static_cast<std::size_t>(-prepare_on_start);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: 'core' x "sv << prepare_on_start);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else
			{
				// - get value
				auto prepare_on_start = __from_string<std::int64_t>(str_find);

				// - 
				if (prepare_on_start > 65536)
				{
					// trace)
					LOG_WARNING << "(warn) 'prepare on start' value exceeded 65536.(" << prepare_on_start << ")";
				}

				// set value
				parameter.prepare_on_start = static_cast<std::size_t>(prepare_on_start);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['prepare on start']: "sv << parameter.prepare_on_start);
			}
		}

		// 5) [Optional] <must prepare>값 읽기 (Default=0)
		if (std::string str_find = (std::string)_context.find("must_prepare", "must prepare"); str_find.empty() == false)
		{
			// - to lower case
			auto value = __to_lower(std::string_view(str_find));

			// - "default"면 default값으로 설정
			if (value == "default")
			{
				parameter.must_prepare = 0;

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: 'default'"sv);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else if(str_find.size() != 0 && (str_find[0] == 'x' || str_find[0] == 'X' || str_find[0] == '*'))
			{
				// - get multiply value
				int64_t must_prepare = __from_string<int64_t>(str_find.substr(1));

				// - set value
				parameter.must_prepare = static_cast<std::size_t>(-must_prepare);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: 'core' x "sv << must_prepare);
			}
			// - 문자열을 integer로 변경해 넣는다.
			else
			{
				// - get value
				auto must_prepare = __from_string<std::int64_t>(str_find);

				// - 
				if (must_prepare > 65536)
				{
					// trace)
					LOG_WARNING << "(warn) 'must prepare' value exceeded 65536.("sv << must_prepare << ")"sv;
				}

				// set value
				parameter.must_prepare = static_cast<std::size_t>(must_prepare);

				// trace)
				EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  + parameter['must prepare']: "sv << parameter.must_prepare);
			}
		}
	}

	// 6) listen이 empty일 경우 default로 설정된다.
	if (parameter.local_endpoint.empty())
	{
		parameter.local_endpoint = end_point_type{ net::ip::address_v6::any(), this->process_get_default_port() };
	}
	// 7) listen이 empty일 경우 default로 설정된다.
	else if (parameter.local_endpoint.port() == 0)
	{
		// - get default port
		auto default_port = this->process_get_default_port();

		// - set default port
		parameter.local_endpoint.port(default_port);

		// log) 
		EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  ! 'listen port' set as default port (loopback, port:"sv << default_port << ")  "sv);
	}

	// check) 
	if (parameter.local_endpoint.port() == 0)
	{
		// log) 
		LOG_ERROR << NFMT::format("   (err ) invalid listen port (port number is '0')");

		// raise)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "invalid listen port (port number is '0')"sv);
	}

	// 8) ProcessSetParameter
	this->process_set_parameters(_context);


	//-----------------------------------------------------------------
	// 3. prepare accept
	//-----------------------------------------------------------------
	// 1) Address Family를 설정한다.
	this->m_address_family = static_cast<ADDRESS_FAMILY>(parameter.local_endpoint.data().family());

	// 2) accept Address를 설정한다.
	if (parameter.accept_endpoint.exist())
	{
		set_accept_endpoint(parameter.accept_endpoint);
	}

	// 3) socket을 준비한다.
	this->process_prepare_socket(parameter.local_endpoint.data().family());

	// 4) 현재 <socket Handle>을 얻는다.
	CGSOCKET socket_handle_listen = this->native_handle();

	// 5) <Address Family>가 AF_INET6일 경우 IPv6 옵션을 설정한다.
	if(parameter.local_endpoint.is_v6())
	{
		api::socket::set_sock_opt(socket_handle_listen, IPPROTO_IPV6, IPV6_V6ONLY, &this->m_is_ip6_only, sizeof(this->m_is_ip6_only));
	}

	// 6) set socket option
	{
		// - enable 'address reuse'
		auto result = api::socket::set_reuse_address(socket_handle_listen, true);

		// trace)
		EXECUTE_IF(result == true, LOG_INFO << "  - enabled 'SOCKET_REUSEADDRESS'"sv);

	#if defined(_WINSOCK2API_)
		// - enable 'fast loopback'
		result = api::socket::set_loopback_fast_path(socket_handle_listen, true);

		// trace)
		EXECUTE_IF(result == true, LOG_INFO << "  - enabled 'LOOPBACK_FASTPATH'"sv);
	#endif
	}

	// 7) start accept
	{
		// - bind한다.
		auto result = api::socket::bind(socket_handle_listen, (::sockaddr*)&parameter.local_endpoint, parameter.local_endpoint.size<int>());

		// check) result가 false이면 bind 실패다!!!
		if (result == SOCKET_ERROR)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// log) 
			LOG_ERROR << "@ (err ) ACCEPTOR['"sv << name() << "'] fail to binding socket for listen. error:"sv << api::get_last_error_string() << ", error_codee:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

			// raise) 
			throw throwable::failure(eRESULT::API_FAILURE, 0, "fail to binding socket for listen"sv);
		}

		// trace)
		EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << NFMT::format("  > binded on {}:{} [requestd:{}]",
					(local_endpoint().is_bind_any() == false) ? parameter.local_endpoint.address().to_string<char>().c_str() : "bind_any",
					parameter.local_endpoint.port(),
					to_string<char>(parameter.local_endpoint).c_str()));

		// - listen
		result = api::socket::listen(socket_handle_listen);

		// check) 
		if (result == false)
		{
			// -get last error
			this->m_last_socket_error = api::get_last_error();

			// log)
			LOG_ERROR << "@ (error) ACCEPTOR['"sv << name() << "'] fail to listen for accepting error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

			// return)
			return true;
		}

		// trace)
		EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << NFMT::format("  > listen on {}:{}",
			(local_endpoint().is_bind_any() == false) ? to_string<char>(local_endpoint()).c_str() : "bind_any",
			parameter.local_endpoint.port()));
	}

	#if defined(_SYS_EPOLL_H)
	// 8) prepare UDS accept
	//if(m_flag & ENABLE_UDS)
	{
		this->m_psocket_uds = this->_process_starting_uds(parameter);
	}
	#endif

	// 9) set socket state
	this->socket_state(eSOCKET_STATE::LISTEN);

	// check) 만약 parameter.prepare_on_start가 -1이면 start_accept를 수행하지 않고 그냥 0를 return한다.
	ERROR_RETURN_IF(parameter.prepare_on_start == static_cast<std::size_t>(NO_START_ACCEPT), eRESULT::SUCCESS , LOG_INFO << "  - accepting is not ready because parameter.prepare_on_start is 'NO_START_ACCEPT' [address="sv << to_string<char>(local_endpoint()) << "]"sv);


	//-----------------------------------------------------------------
	// 4. prepare accept
	//-----------------------------------------------------------------
#if defined(_WINSOCK2API_)
	// declare) 
	int64_t prepare_on_start = static_cast<int64_t>(parameter.prepare_on_start);
	int64_t must_prepare = static_cast<int64_t>(parameter.must_prepare);

	// 1A) <parameter.prepare_on_start>이 0이라면 default
	if(prepare_on_start == 0)
	{
		// - core갯수 x 8 개만큼을 준비한다.
		parameter.prepare_on_start = api::get_core_count() * 8;
	}
	// 1B) 양수이라면 값을 그대로 넣는다.
	else if (prepare_on_start > 0)
	{
		parameter.prepare_on_start = prepare_on_start;
	}
	// 1C) 음수값라면 prepare_on_start값은 배수다.
	else
	{
		// - core갯수 x (-prepare_on_start)로 설정한다.
		parameter.prepare_on_start = api::get_core_count() * (-prepare_on_start);
	}

	// check) parameter.prepare_on_start가 혹시 0개이면 끝낸다.(prepare이 하나도 없으면 accept가 되지를 않는다.)
	CGASSERT_ERROR(parameter.prepare_on_start != 0);

	// 2A) <parameter.must_prepare>이 0이라면 default
	if(must_prepare == 0)
	{
		// - core갯수 x 4 개만큼을 준비한다.
		parameter.must_prepare = api::get_core_count() * 4;
	}
	// 2B) 양수이라면 값을 그대로 넣는다.
	else if (must_prepare > 0)
	{
		parameter.must_prepare = must_prepare;
	}
	// 2C) 음수값라면 must_prepare는 배수다.
	else
	{
		// - core갯수 x (-must_prepare)로 설정한다.
		parameter.must_prepare = api::get_core_count() * (-must_prepare);
	}

	// 3) 최하 1개는 되어야 한다.
	SATURATE_MIN(parameter.must_prepare, 1);
	SATURATE_MIN(parameter.prepare_on_start, 1);

	// check) must_prepare이 prepare_on_start보다 클수는 없다.
	if (parameter.prepare_on_start < parameter.must_prepare)
	{
		LOG_WARNING << "(warn) 'must_prepare' is greather than 'prepare_on_start'"sv;
	}

	// 4) <parameter.prepare_on_start>값을 설정한다.
	//    prepare_on_start값이 must_prepare볻다 작을 경우에는 must_prepare값으로 설정된다.
	parameter.prepare_on_start = __select_max(parameter.prepare_on_start, parameter.must_prepare);

	// trace)
	EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  - prepare on start="sv << parameter.prepare_on_start << ", must prepare="sv << parameter.must_prepare);
#endif

	if (this->m_max_allocate > g_max_accept_socket)
	{
		// trace) 
		LOG_WARNING << "  !) max concurrent session is "sv << g_max_accept_socket << " (free release or license expired)"sv;

		// - set max _allcate
		this->m_max_allocate = g_max_accept_socket;
	}

	// trace) 
	if (parameter.enable_progress_log)
	{
		if (static_cast<int64_t>(this->m_max_allocate) < 0)
		{
			LOG_INFO << "  - 'max concurrent session' : unlimited"sv;
		}
		else
		{
			LOG_INFO << "  - 'max concurrent session' : "sv << this->m_max_allocate;
		}
	}


	//-----------------------------------------------------------------
	// 6. 준비하기
	//-----------------------------------------------------------------
	// 1) acceptor를 일단 enable시킨다.
	this->enable();


	//-----------------------------------------------------------------
	// 7. accept를 준비한다.
	//-----------------------------------------------------------------
#if defined(_WINSOCK2API_)
	{
		// declare) 
		std::size_t accepted = 0;

		// 1) Accept 시도 횟수를 계산한다.
		parameter.prepare_on_start -= this->m_socket_count_prepared_to_accept;

		try
		{
			// 2) _count_accept_prepare_on_start만큼 accept를 건다.
			for(std::size_t i = parameter.prepare_on_start; i > 0; --i, ++accepted)
			{
				// - Accept를 수행한다.
				this->request_accept(this->m_address_family);
			}

			// trace) 걸어놓은 accpet갯수를 출력한다.
			EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << "  > ACCEPTOR["sv << name() << "] started ["sv << accepted << " sockets are ready for accepting]"sv);
		}
		catch (throwable::failure& _e)
		{
			// log) 
			LOG_DEBUG << "@ (excp) '" << _e.m_reason << "' exception complete ACCEPTOR['"sv << name() << "'] socket:"sv << static_cast<int64_t>(this->native_handle()) << ") ("sv << _e.description() << " " __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			if (throwable::execute_exception_handler(this, _e, __FUNCTION__) == false)
			{
				// - close acceptor
				this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

				// reraise) 
				throw;
			}
		}
		catch(std::exception& _e)
		{
			// log) 
			LOG_DEBUG << "@ (excp) eeception complete ACCEPTOR['"sv << name() << "'] socket:"sv << static_cast<int64_t>(this->native_handle()) << "" __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			if(throwable::execute_exception_handler(this, _e, __FUNCTION__) == false)
			{
				// - close acceptor
				this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

				// reraise) 
				throw;
			}
		}
		catch (...)
		{
			// log) 
			LOG_DEBUG << "@ (excp) exception complete ACCEPTOR['"sv << name() << "'] sSocket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			if(throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
			{
				// - close acceptor
				this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

				// reraise) 
				throw;
			}
		}

		// 3) 값을 설정한다.
		this->m_accept_count_must_prepare = parameter.must_prepare;
		this->m_listen_endpoint = parameter.local_endpoint;
	}
#else
	{
		// 1) executable accept 할당받기.
		auto pexecutable_accept = make_object<executable_accept>();

		// check) 
		CGASSERT_ERROR(pexecutable_accept->reference_count() == 1);

		// 2) acceptor를 this로 설정한다.
		pexecutable_accept->m_pconnective = this;

		// 4) Ready
		auto pio_context = io_context();

		// check) io_context가 없으면 잘못된 것이다.
		CGASSERT_ERROR(pio_context.exist());

		// 5) event를 설정한다.
		auto result = pio_context->attach(this->native_handle(), pexecutable_accept, EPOLLEXCLUSIVE | EPOLLIN | EPOLLOUT | EPOLLERR);

		// check) 
		THROW_IF(result == false, throwable::failure(eRESULT::API_FAILURE));

		// 6) ...
		this->m_accept_count_must_prepare = 0;
		this->m_listen_endpoint = parameter.local_endpoint;

		// 7) create executable_accept
		this->m_pexecutable_accept = std::move(pexecutable_accept);
	}
#endif

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_start(context& _context)
{
	// 1) starting 호출
	this->on_start(_context);

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> start ACCEPTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_stopping() noexcept
{
	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <BEGIN> stop ACCEPTOR['"sv << name() << "']"sv);

#if defined(_SYS_EPOLL_H)
	// 1) reset m_pexecutable_accept
	this->m_pexecutable_accept.reset();
#endif
	
	// 2) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stopping' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// 3) 먼저 Disable한다.
	this->disable();

	// 4) socket을 닫는다.
	this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

	// trace) 걸어놓은 Accpet갯수를 출력한다.
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  < ACCEPTOR["sv << name() << "] closed"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_stop() noexcept
{
	// 1) on_stop를 호출한다.
	try
	{
		this->on_stop();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) exception on 'on_stop' function call ACCEPTOR '"sv << name() << "' " __CGFUNCTION__ ""sv;
	}

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> stop ACCEPTOR['"sv << name() << "']"sv);

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::connective::acceptor::Ntcp::_process_destroy() noexcept
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

bool net::io::connective::acceptor::Ntcp::process_set_enable(bool _is_state)
{
	// declare)
	int	pre_state;

	// 1) disable로 상태를 바꾼다.
	pre_state = this->m_is_enable.exchange((_is_state) ? 1 : 0);

	// check) 이전 상태가 Disable된 상태라면 그냥 끝낸다.
	RETURN_IF(pre_state != 0, false);

	if (_is_state == true)
	{
		// trace) 성공한 Accpet갯수를 출력한다.
		EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  (info) ACCEPTOR['" << name() << "'] enabled (hSocket:"sv << static_cast<int64_t>(this->native_handle()) << ") "sv);

		// notify) notify to observer
		transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_ENABLE, static_cast<net::io::Iconnective*>(this) });

		// notify) notify to connective manager
		net::io::connective::manager::instance()->transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_ENABLE, static_cast<net::io::Iconnective*>(this) });
	}
	else
	{
		// trace) 성공한 Accpet갯수를 출력한다.
		EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  (info) ACCEPTOR['" << name() << "'] disabled (hSocket:"sv << static_cast<int64_t>(this->native_handle()) << ")"sv);

		// notify) message 날리기..
		try
		{
			// - notify to observer
			transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_DISABLE, static_cast<net::io::Iconnective*>(this) });

			// - notify to connective manager
			net::io::connective::manager::instance()->transmit_message(sMESSAGE{ eMESSAGE::NETWORK::SOCKET::CONNECTIVE_DISABLE, static_cast<net::io::Iconnective*>(this) });
		}
		catch (...)
		{
			// log) 
			LOG_DEBUG << "@ (excp) exception on sending messages ACCEPTOR '" << name() << "' " __CGFUNCTION__ ""sv;
		}
	}

	// return) 
	return true;
}

bool net::io::connective::acceptor::Ntcp::process_get_enable() const
{
	return this->m_is_enable != 0;;
}

const char* SAFE_GET_NAME(Ireferenceable* _poject)
{
	// 1) Inameable Interface를  얻는다.
	auto pnameable = dynamic_cast<object::Inameable*>(_poject);

	// check) pnameable이 nullptr이면 그냥 ""를 리턴한다.
	RETURN_IF(pnameable == nullptr, "");

	// 2) Name을 얻는다.
	auto pname = pnameable->name();

	// 3) pname이 nullptr이 아니면 해당 Name을 아니면 ""를 리턴한다.
	return (pname.empty() == false) ? pname.data() : "";
}

void net::io::connective::acceptor::Ntcp::process_create_socket(ADDRESS_FAMILY _address_family)
{
	// 1) socket 생성
#if defined(_WINSOCK2API_)
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP, api::socket::instance().rio_is_enable() ? WSA_FLAG_REGISTERED_IO : WSA_FLAG_OVERLAPPED);

	// check) socket Option의 수행에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// log) 
		LOG_DEBUG << "@ (err ) fail to creating socket for ACCEPTOR. error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// raise) 
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to creating socket handle for acceptor"sv);
	}

	// log) 
	EXECUTE_IF(api::socket::instance().rio_is_enable() && this->m_enable_progress_log, LOG_INFO << "  - enabled 'REGISTERED_IO'");

#elif defined(_SYS_EPOLL_H)
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_STREAM, IPPROTO_TCP);

	// check) socket Option의 수행에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// log) 
		LOG_DEBUG << "@ (err ) fail to creating socket for ACCEPTOR. error_code:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

		// raise)
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to creating socket handle for acceptor"sv);
	}

	// 4) set as non-block socket
	api::socket::set_nonblock(handle_socket_new);
#endif

	// 2) set new socket handle
	CGSOCKET handle_socket_old = this->native_handle(handle_socket_new);

	// 3) close old socket handle
	if(handle_socket_old != INVALID_SOCKET)
	{
	#if defined(_SYS_EPOLL_H)
		// check) io_context must exist
		CGASSERT_ERROR(io_context().exist());

		// 4) detach socket from epoll
		io_context()->detach((CGHANDLE)handle_socket_old);
	#endif

		// - closesocket
		api::socket::closesocket(handle_socket_old);
	}

	// 4) set address family
	address_family(_address_family);

	// 5) set socket option
	api::socket::set_nonblock(handle_socket_new, true);

	// 6) get io_context
	auto pio_context = io_context();

	// 7) io_context가 없으면 Default io_context를 설정한다.
	if (pio_context.empty())
	{
		// - get system io_context
		pio_context = system_executor::executor();

		// - set io_context
		io_context(pio_context);

		// check) 
		CGASSERT_ERROR(pio_context.exist());
	}

	// 설명) windows의 경우 listen을 하기 전에 attach를 해야하고 linux의 경우 listen을 한후 attach해야한다.(EPOLLEXCLUSIVE 때문에...)

#if defined(_WINSOCK2API_)
	// 8) io_context에 붙인다.
	auto result = pio_context->attach((CGHANDLE)handle_socket_new);

	// check) result가 false이면 Exception 던진다.
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL, 0, "fail to attaching socket to io_context"sv), LOG_DEBUG << "(err ) fail to attaching acceptor to io_context " __CGFUNCTION__ ""sv);

	// trace)
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  > attached to EXECUTOR['"sv << SAFE_GET_NAME(pio_context) << "']"sv);
#endif
}

void net::io::connective::acceptor::Ntcp::process_prepare_socket(ADDRESS_FAMILY _address_family)
{
	// 1) create socket
	this->process_create_socket(_address_family);

	// 2) socket을 Closed로 한다.
	this->socket_state(eSOCKET_STATE::CLOSED);

	// statistics)
	this->reset_statistics();

	// 3) reset socket을 수행한다.
	this->on_prepare_socket();

	// trace) 
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  > prepare socket (hSocket="sv << static_cast<int64_t>(this->native_handle()) << ")"sv);
}

void net::io::connective::acceptor::Ntcp::process_dispose_socket() noexcept
{
}

bool net::io::connective::acceptor::Ntcp::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	// 1) socket Status를 Disconnected로...
	auto pre_socket_status = socket_state(eSOCKET_STATE::CLOSED);

	// check) 이전 Status도 Closed상태라면 그냥 함수를 종료한다.
	RETURN_IF(pre_socket_status == eSOCKET_STATE::CLOSED, false);

	// 2) Reason을 설정한다.
	this->m_disconnect_reason |= _disconnect_reason;
	this->m_disconnect_reason |= DISCONNECT_REASON_NODISCONNECTEX;

	// trace) 
	EXECUTE_IF(this->m_enable_progress_log, LOG_INFO << "  - ACCEPTOR['" << name() << "']: close socket (native_handle:"sv << static_cast<int64_t>(this->native_handle()) << ")"sv);

	// 3) on_closesocket함수를 호출한다.
	this->on_closesocket(this->m_disconnect_reason);

	// 4) listen socket을 닫는다.
	auto socket_handle = this->native_handle(INVALID_SOCKET);

	// check) 
	RETURN_IF(socket_handle == INVALID_SOCKET, false);

	// 5) 모든 IO가 완료될 때까지 기다린다.
	//    - Accept중인 socket을 닫는다.
	//    - 접속된 socket을 완전히 제거한다.
	auto pio_context = this->io_context();

	// check) io_context must exist
	CGASSERT_ERROR(this->io_context().exist());

#if defined(_SYS_EPOLL_H)
	// 6) detach socket from epoll
	pio_context->detach((CGHANDLE)socket_handle);
#endif

	// 7) socket을 Delete한다.
	api::socket::closesocket(socket_handle);

	// 8) 모든 accepted된 socket을 종료한다.
	this->close_connectable_all();

	// 9) io_context를 돌려 IOCP의 I/O를 처리한다. (최대 MAX_LOOP만큼만 돈다.)
#ifndef NDEBUG
	// definitions) 
	const int MAX_LOOP = 10000;

	for(int retry = MAX_LOOP; retry > 0 && (this->m_socket_count_prepared_to_accept != 0 || count() != 0); --retry)
#else
	while(this->m_socket_count_prepared_to_accept != 0 || count()!=0)
#endif
	{
		// - execute를 실행한다.
		pio_context->execute_all();

		// - Sleep하기.
		api::sleep_for(1ms);
	}

#if defined(_WINSOCK2API_)
	// check) 아직 Accept중인 socket이 남이 있을 겨우!
	EXECUTE_IF(this->m_socket_count_prepared_to_accept != 0, LOG_ERROR << log_level_t::important() << "@ (excp) listen soocket is exist yet[prepared:"sv << this->m_socket_count_prepared_to_accept << "]"sv);

	// check) 아직 접속 종료되지 않았을 경우!
	EXECUTE_IF(count()!=0, LOG_ERROR << log_level_t::important() << "@ (excp) connected socket is exist yet[connected:"sv << static_cast<int64_t>(count()) << "]"sv);
#elif defined(_SYS_EPOLL_H)
	
#else
#endif

	// 10) io_context를 reset한다.
	this->reset_io_context();

	// return) 
	return true;
}

#if defined(_WINSOCK2API_)
void net::io::connective::acceptor::Ntcp::request_accept(ADDRESS_FAMILY _address_family)
{
	// check) Accept가 Disable된 상태라면 잘못된거다.(프로그래밍을 잘못 짠것임.)
	RETURN_IF(this->is_disable());

	// check) 소켓의 상태가 eSOCKET_STATE::LISTEN가 아니라면 false를 리턴한다.
	RETURN_IF(this->socket_state()!= eSOCKET_STATE::LISTEN);


	//-----------------------------------------------------------------
	// 1. connective Completor 준비하기.
	//-----------------------------------------------------------------
	// 1) 새로운 Accept를 댄다.
	auto pconnectable = this->process_alloc_connectable();

	// check) pconnectable가 nullptr이어서는 안된다.
	CGASSERT_ERROR(pconnectable != nullptr);

	// 2) io_context를 얻는다.
	auto pio_context = this->io_context();

	// check) io_context가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pio_context.empty());

	// 2) socket을 reset한다.
	pconnectable->io_context(pio_context);

	// 3) socket option을 설정한다.
	pconnectable->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
	pconnectable->maximum_receive_buffer_size(net::io::socket::Ntcp::default_maximum_receive_buffer_size());

	// check) _address_family가 AF_UNSPEC이어서는 안된다.
	CGASSERT_ERROR(_address_family != AF_UNSPEC);

	// 4) io_context를 설정한다.
	try
	{
		pconnectable->prepare_socket(_address_family);
	}
	catch(...)
	{
		// -  reset connectable
		pconnectable.reset();

		// reraise)
		throw;
	}

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->native_handle() != INVALID_SOCKET);

	// 5) connectable manager를 nullptr로 설정한다.
	pconnectable->m_flexible.get<Iconnectable_manager*>() = nullptr;


	//-----------------------------------------------------------------
	// 2. socket Option설정하기.
	//-----------------------------------------------------------------
	{
		// 1) port를 reuse하도록 한다.(중요:diconnect_ex로 socket을 재활용하므로 port를 재활용(Reuse)하지 않으면 error를 발생시킬수 있다.)
		api::socket::set_reuse_address(pconnectable->native_handle(), true);

		// 2) fast Loopback 활성화
		api::socket::set_loopback_fast_path(pconnectable->native_handle(), true);
	}

	//-----------------------------------------------------------------
	// 3. socket State 'eSOCKET_STATE::SYN'상태로 바꾸기.
	//-----------------------------------------------------------------
	{
		// 1) change socket state from 'eSOCKET_STATE::CLOSED' to 'eSOCKET_STATE::SYN'
		bool is_changed = pconnectable->set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

		// check) socket state must be 'eSOCKET_STATE::CLOSED'
		CGASSERT_ERROR(is_changed == true);

		// check) socket state must be 'eSOCKET_STATE::CLOSED'
		EXECUTE_IF(is_changed != true, LOG_DEBUG << "@ (err ) accept_async() state of socket is not 'eSOCKET_STATE::CLOSED' " __CGFUNCTION__ ""sv);
	}

	//-----------------------------------------------------------------
	// 4. prepare executable_connective
	//-----------------------------------------------------------------
	// 1) alloc executable_connective object
	auto pexecutable_connective = make_object<executable_connective>();

	// check) 
	CGASSERT_ERROR(pexecutable_connective->reference_count() == 1);

	// 2) set variable 
	pexecutable_connective->m_pconnective = this;
	pexecutable_connective->m_pconnectable = pconnectable;

	// 3) increase reference count
	pexecutable_connective->add_ref();



	//-----------------------------------------------------------------
	// 5. process_request_connect
	//-----------------------------------------------------------------
	// 1) on_request_accept
	this->on_request_accept(pconnectable);

	// declare)
	net::sockaddr address_peer;

	// 2) process request_connect
	pconnectable->process_request_connect(address_peer);


	//-----------------------------------------------------------------
	// 6. accept
	//-----------------------------------------------------------------
	// statistics) 
	this->statistics_on_increase_async();

	// 1) Accept를 수행한다.
	auto result = api::socket::accept_async(this->native_handle(), pconnectable->native_handle(), pexecutable_connective->get_connectable_buffer(), pexecutable_connective.get());

	// check) accept_async를 실패했다.
	if (result == false)
	{
		// - set last error
		pconnectable->m_last_socket_error = api::get_last_error();

		// trace) 
		LOG_DEBUG << "@ (err ) fail to 'api::socket::accept_async()'. error_code:" << api::get_last_error() << " (socket:" << static_cast<int64_t>(this->native_handle()) << ")";

		// - process_close_socket
		pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_FAIL_TO_SOCKET_ACCEPT);

		// statistics) 
		this->statistics_on_decrese_async();

		// - release reference count
		pexecutable_connective->release();

		// statistics)
		this->statistics_on_fail_connect();

		// throw) 
		throw throwable::failure(eRESULT::API_FAILURE, 0, "fail to accept socket"sv);
	}

	// 2) accept한 socket을 하나 더한다.
	++this->m_socket_count_prepared_to_accept;

	// statistics)
	this->statistics_on_try();

	// 3) 할당된 소켓수를 증가시킨다.
	++this->m_socket_count_allocated;
}

class net::io::connective::acceptor::Ntcp::executable_retry_request_accept :
	virtual public	Iexecutable,
	public			Npoolable<executable_retry_request_accept>,
	public			factory::_traits_system
{
public:
	virtual	intptr_t process_execute(intptr_t _result, std::size_t _param) override;

public:
	object_ptr<net::io::connective::acceptor::Ntcp>	m_pconnective_acceptor;
};

intptr_t net::io::connective::acceptor::Ntcp::executable_retry_request_accept::process_execute(intptr_t /*_result*/, std::size_t /*_param*/)
{
	// 1) Process disconnect
	this->m_pconnective_acceptor->process_retry_accept();

	// 2) Release
	this->m_pconnective_acceptor.reset();

	// return) Success!!
	return intptr_t(0);
}

void net::io::connective::acceptor::Ntcp::process_request_retry_accept()
{
	scoped_lock(this->m_lockable_retry_accept_now)
	{
		// check)
		RETURN_IF(this->m_retry_accept_now);

		// 1) set flag
		this->m_retry_accept_now = true;

		// 2) ...
		auto result = this->_process_request_retry_accept();

		// check)
		if (result == false)
		{
			this->m_retry_accept_now = false;
		}
	}
}

bool net::io::connective::acceptor::Ntcp::_process_request_retry_accept()
{
	// 1) make executable object
	auto pexecutable = make_object< executable_retry_request_accept>();

	// 2) set 'm_pconnective_acceptor' this
	pexecutable->m_pconnective_acceptor = this;

	// 3) request 
	return system_executor::post(pexecutable, chrono::tick::now() + 1s);

}

void net::io::connective::acceptor::Ntcp::process_retry_accept()
{
	// check)
	CGASSERT_ERROR(this->m_retry_accept_now == true);

	scoped_lock(this->m_lockable_retry_accept_now)
	{
		// check)
		if (is_disable())
		{
			// - set ...
			this->m_retry_accept_now = false;

			// return) 
			return;
		}
	}

	// 1) get param
	auto prepared_now = this->m_socket_count_prepared_to_accept.load();
	auto accept_count_must_prepare = this->m_accept_count_must_prepare;

	// check) 
	if (accept_count_must_prepare <= prepared_now)
	{
		scoped_lock(this->m_lockable_retry_accept_now)
		{
			this->m_retry_accept_now = false;
		}

		// return) 
		return;
	}

	// 2) accept걸어야할 수를 계산한다.
	auto count_request_accpet = accept_count_must_prepare - prepared_now;

	try
	{
		// 4) _count_accept_prepare_on_start만큼 accept를 건다.
		for (count_request_accpet; count_request_accpet > 0; --count_request_accpet)
		{
			// - Accept를 수행한다.
			this->request_accept(this->m_address_family);
		}
	}
	catch (...)
	{
	}

	// 3) retry if prepared_socket is 0
	scoped_lock(this->m_lockable_retry_accept_now)
	{
		if (count_request_accpet == 0)
		{
			this->m_retry_accept_now = false;
		}
		else
		{
			// - ...
			auto result = _process_request_retry_accept();

			// check)
			if (result == false)
			{
				this->m_retry_accept_now = false;
			}
		}
	}
}

void net::io::connective::acceptor::Ntcp::process_connective_connect( Iconnectable* _pconnectable, uintptr_t _result)
{
	//-----------------------------------------------------------------
	// ProcessCompleteAccept()함수는 크게 두 부분으로 나눌수 있다.
	//
	// * 첫째는 Accept받은 socket의 접속처리 부분.
	// * 둘째는 새로운 Accept를 거는 부분
	//
	// 으로 나눌 수 있다.
	// 하나의 처리가 다른 하나의 처리에 영향을 미치지 않고 따로 Error나 Exception 
	// 처리를 수행한다.
	//-----------------------------------------------------------------
	// check) _pconnectable이 nullptr이 되면 안된다.
	CGASSERT_ERROR(_pconnectable != nullptr);


	//-----------------------------------------------------------------
	// 1. Accept완료를 했으므로 걸어 놓은 수를 줄인다.
	//-----------------------------------------------------------------
	// 1) Accept를 받았으므로 준비중인 Accept수를 하나 줄인다.
	--this->m_socket_count_prepared_to_accept;


	//-----------------------------------------------------------------
	// 2. 현재 acceptor가 Disable 혹은 소켓 최대 한도 초과 시
	//-----------------------------------------------------------------
	// check) Accept가 Disable된 상태나 최대 할당 수보다 많은 경우라면 끝낸다.
	if(this->is_disable() || api::socket::is_socket_count_overflowed())
	{
		// - reason
		auto disconnect_reason = (this->is_disable()) ? (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_DISABLED) : (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_OPEN_SOCKET_LIMITS);

		// - connectable의 socket을 닫는다.
		_pconnectable->process_close_socket(disconnect_reason);

		// - 할당된 소켓수를 증가시킨다.
		--this->m_socket_count_allocated;

		// return)
		return;
	}

	try
	{
		//-----------------------------------------------------------------
		// 3. Accept받은 socket의 처리.
		//-----------------------------------------------------------------
		// Case SUCCRESS) accept가 성공했을 경우
		//   - accept가 제대로 되었을 때는 accept처리를 한다.
		//   - accept 받은 socket의 option을 설정하고... (update_accept_context)
		//   - on_accept()함수를 호출하고...
		//   - _pconnectable의 process_complete_connect()함수를 호출하여 
		//     소켓별 접속처리를 진행한다.
		if ((_result == 0) && (count() < this->m_max_allocate) && (count() < g_max_accept_socket))
		{
			// 1) update accept context 옵션을 설정한다.
			{
				// - call 'update_accept_context'
				auto result = api::socket::update_accept_context(_pconnectable->native_handle(), this->native_handle());

				// check) socket Option에 실패했을 경우 socket을 닫는다.
				//        - socket의 Option 설정이 잘못되었을 경우 socket 자체가 망가진 것이다.
				//        - 따라서 Option 설정에 실패했을 경우 소켓을 닫고 릴리즈 한다.
				//        - 이과정은 아래 과정에서 실행될 것이다.
				if (result == false)
				{
					// - set last error
					this->m_last_socket_error = api::get_last_error();

					// log) 
					LOG_DEBUG << "(excp) fail to setting socket option 'SO_UPDATE_ACCEPT_CONTEXT' socket="sv << static_cast<uint64_t>(_pconnectable->native_handle()) << "/ error:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

					// raise) 
					throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to setting socket option 'SO_UPDATE_ACCEPT_CONTEXT'"sv);
				}
			}

			// 2) call 'process_on_accept'
			this->process_on_accept(_pconnectable);

			// 3) set variable
			//    주의) _pconnectable->m_pconnectable_notify = this 설정은 반드시 process_complete_connect(...)가 호출되기 전에 설정되어야 한다.
			//          process_complete_connect 처리 과정 중에 this->m_pconnectable_notify 값을 on_connect() 호출에서 인자로 넘긴다.
			_pconnectable->m_pconnectable_notify = this;

			// 주의) statistics_on_success_connect() 함수는 '_pconnectable->process_complete_connect(...)'의 처리중 실패시 statistics상 disconnect로 처리되므로
			// //    반드시 여기서 호출되어야 한다.
			//       이 처리과정은 '접속 성공' 루틴이므로 '_pconnectable->process_complete_connect(...)'에서 on_fail_connect(...)루틴을 타지는 않는다!

			// statistics) 
			this->statistics_on_success_connect();

			// 4) call 'process_complete_connect'
			if (_pconnectable->process_complete_connect(_result) == true)
			{
				// 설명) process_complete_connect가 성공했으므로 이후 실패는 접속종료로 처리된다.
				//       - 일단 on_connect()호출이 끝난 이후므로 이후에 실패시 on_disconnect()가 호출되어야 한다.
				//         따라서 이후 실패처리는 접속 종료 처리로 진행된다.
				//       - 처리 실패시 exception이 던져지고 catch에서 접속 종료 처리가 진행된다.

				// - call 'add_connectable'
				//   주의) add_connectable(_pconnectable)은 반드시 process_complete_connect(...)의 호출 이후에 한다.
				//         그렇지 않으면 on_connect()처리 도중에 acceptor의 connectable_list에 등록되므로 여기서 socket이 참조되어 처리가 될수 있기 때문에 문제가 될수 있다.
				//         즉 on_connect() 도중에 다른 처리가 진행될 수 있다.
				//         특히 epoll의 경우 100% 문제가 발생하므로 반드시 주의해야 한다.
				auto result = this->add_connectable(_pconnectable);

				// check)
				CGASSERT_ERROR(result == true);

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
			}
			else
			{
				// - rollbaack
				_pconnectable->m_pconnectable_notify = nullptr;
			}
		}
		else
		{
			// - process_on_fail_connect
			this->process_on_fail_accept(_pconnectable);

			// - closesocket
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_CONNECTION_FULL);

			{
				// - call 'process_complete_connect'
				_pconnectable->process_complete_connect(1);

				// statistics) 
				this->statistics_on_fail_connect();

				// - decrease socket_count_allocated
				--this->m_socket_count_allocated;
			}
		}
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:" << (int64_t)((_pconnectable != nullptr) ? static_cast<int64_t>(_pconnectable->native_handle()) : 0) << " " __CGFUNCTION__ ""sv;

		// - connectable의 socket을 닫는다.
		_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_EXCEPTION_OCCURED);

		// statistics) 
		this->statistics_on_fail_connect();

		// - decrease socket_count_allocated
		--this->m_socket_count_allocated;
	}


	//-----------------------------------------------------------------
	// 4. prepare new accept
	//-----------------------------------------------------------------
	try
	{
		// 1) get prepare count
		std::size_t requested_count = (this->m_accept_count_must_prepare > this->m_socket_count_prepared_to_accept) ? this->m_accept_count_must_prepare - this->m_socket_count_prepared_to_accept : 0;
		
		// 2) request prepare accept
		for(; requested_count > 0 && this->m_socket_count_prepared_to_accept < this->m_accept_count_must_prepare; --requested_count)
		{
			this->request_accept(this->m_address_family);
		}

		// check)
		CGASSERT_ERROR(this->m_socket_count_prepared_to_accept >= 0);
	}
	catch(std::exception& e)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// - 
		if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
		{
			this->process_request_retry_accept();
		}
	}
	catch(...)
	{
		// trace) 
		LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, throwable::failure(eRESULT::EXCEPTION), __FUNCTION__);

		// - 
		if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
		{
			this->process_request_retry_accept();
		}
	}

	// statistics)
	this->statistics_on_decrese_async();
}
#elif defined(_SYS_EPOLL_H)
bool net::io::connective::acceptor::Ntcp::request_accept(CGSOCKET _handle_socket_new, ADDRESS_FAMILY _address_family)
{
	//-----------------------------------------------------------------
	// 1. connective Completor 준비하기.
	//-----------------------------------------------------------------
	// 1) 새로운 socket을 할당받는다.
	auto pconnectable = process_alloc_connectable();

	// 2) get io_context
	auto pio_context = pconnectable->io_context();

	// 3) set native socket handle
	{
		// - set socket
		auto handle_socket_old = pconnectable->native_handle(_handle_socket_new);

		// - 기존에 설정되어 있던 소켓핸들이 INVALID_SOCKET이 아니라면 소켓을 닫는다.
		if (handle_socket_old != INVALID_SOCKET)
		{
			// check) io_context must exist
			if (pio_context.exist())
			{
				// - detach socket from epoll
				pio_context->detach((CGHANDLE)handle_socket_old);
			}

			// - closesocket
			api::socket::closesocket(handle_socket_old);
		}

		// check) _address_family가 AF_UNSPEC이어서는 안된다.
		CGASSERT_ERROR(_address_family != AF_UNSPEC);
		CGASSERT_ERROR(_address_family == AF_INET || _address_family == AF_INET6);

		// - set address family
		pconnectable->address_family(_address_family);

		// - socket을 Nonblock으로 설정한다.
		api::socket::set_nonblock(_handle_socket_new);

		// - 설정되어 있는 io_context가 없으면 Default io_context로 설정한다.
		if (pio_context.empty())
		{
			pio_context = system_executor::executor();
			pconnectable->io_context(pio_context);
		}

		// - prepare exeecutable socket
		pconnectable->process_prepare_executable_socket();
	}

	// 3) io_context를 설정한다.
	pconnectable->prepare_socket(_address_family);

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->native_handle() != INVALID_SOCKET);

	// check) pconnectable의 socket handle이 INVALID_SOCKET이여서는 안된다.
	CGASSERT_ERROR(pconnectable->socket_state() == eSOCKET_STATE::CLOSED);

	// 4) reset connectable manager ptr
	pconnectable->m_flexible.get<Iconnectable_manager*>() = nullptr;


	//-----------------------------------------------------------------
	// 2. set socket state to SYN
	//-----------------------------------------------------------------
	// 1) change socket state from 'eSOCKET_STATE::CLOSED' to 'eSOCKET_STATE::SYN'
	bool is_changed = pconnectable->set_socket_state_if(eSOCKET_STATE::CLOSED, eSOCKET_STATE::SYN);

	// check) socket state must be 'eSOCKET_STATE::CLOSED'
	CGASSERT_ERROR(is_changed == true);

	// check) socket state must be 'eSOCKET_STATE::CLOSED'
	EXECUTE_IF(is_changed != true, LOG_DEBUG << "@ (err ) accept_async() state of socket is not 'CLOSED' " __CGFUNCTION__ ""sv);


	//-----------------------------------------------------------------
	// 3. process_request_connect
	//-----------------------------------------------------------------
	// 1) on_request_accept를 호출한다.
	this->on_request_accept(pconnectable);

	// declare)
	net::sockaddr address_peer;

	// 2) process request_connect
	pconnectable->process_request_connect(address_peer);


	//-----------------------------------------------------------------
	// 4. process connective connect
	//-----------------------------------------------------------------
	// 1) increase allocated socket count
	++this->m_socket_count_allocated;

	// 2) call 'process_connective_connect'
	this->process_connective_connect(pconnectable, 0);

	// return)
	return true;
}

void net::io::connective::acceptor::Ntcp::process_connective_accept(uintptr_t _result)
{
	// check) accept가 disable된 상태나 최대 할당 수보다 많은 경우라면 끝낸다.
	RETURN_IF(is_disable());

	if (_result & EPOLLIN)
	{
		for (;;)
		{
			// declare)
			net::sockaddr sockaddr_listen;
			socklen_t addr_len = sizeof(sockaddr_listen);

			// statistics)
			this->statistics_on_try();

			// 1) 접속해온 socket handle를 accept 받는다.
			CGSOCKET handle_socket_new = api::socket::accept(this->native_handle(), reinterpret_cast<::sockaddr*>(&sockaddr_listen), &addr_len);

			// check) break if no more pended accepting socket
			if (handle_socket_new < 0)
			{
				// - get error
				auto error_code = api::get_last_error();

				// check)
				BREAK_IF(error_code == EAGAIN || error_code == EWOULDBLOCK);

				// 설명) EMFILE, ENFILE, ENOBUFS, ENOMEM와 같은 이유로 accept를 받지 못할 경우 계속 event가 발생해
				//       CPU 100%를 차지할 수 있으므로 적절한 처리가 필요하다.

				// - set socket error code
				this->m_last_socket_error = error_code;

				switch (error_code)
				{
				// case) "too many opn file"
				case	EMFILE:
						break;

				// case) "file table overflow"
				case	ENFILE:
						break;

				// case) have no memory
				case	ENOBUFS:
				case	ENOMEM:
						break;
				}

				// statistics)
				this->statistics_on_fail_connect();

				// break) 
				break;
			}

			// 2) request accept
			try
			{
				this->request_accept(handle_socket_new, this->m_address_family);
			}
			catch (...)
			{
				// statistics)
				this->statistics_on_fail_connect();
			}
		}
	}

	if (_result & EPOLLPRI)
	{
		this->statistics_on_decrese_async();
	}
}

void net::io::connective::acceptor::Ntcp::proceoss_closesocket_fail(net::io::Iconnectable* _pconnectable, uint64_t _disconnect_reason) noexcept
{
	// check) 
	CGASSERT_ERROR(_pconnectable->native_handle() != INVALID_SOCKET);

	// 1) reset native socket handle
	_pconnectable->native_handle(INVALID_SOCKET);

	// 2) set disconnect reason
	_pconnectable->disconnect_reason(_disconnect_reason);

	// 3) get connectable tcp
	auto pconnectable_tcp = dynamic_cast<net::io::connectable::Ntcp*>(_pconnectable);

	if (pconnectable_tcp != nullptr)
	{
		// 4) completee closesocket
		pconnectable_tcp->process_complete_closesocket();
	}

	// 5) reset executable socket (releas _conneective) 
	pconnectable_tcp->process_reset_executable_socket();
}

void net::io::connective::acceptor::Ntcp::process_connective_connect(Iconnectable* _pconnectable, uintptr_t _result)
{
	// check) _pconnectable이 nullptr이 되면 안된다.
	CGASSERT_ERROR(_pconnectable != nullptr);

	// check) closesocket if accept is disabled
	if (this->is_disable() || api::socket::is_socket_count_overflowed())
	{
		// - reason
		auto disconnect_reason = (this->is_disable()) ? (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_DISABLED) : (DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_OPEN_SOCKET_LIMITS);

		// - closesocket
		proceoss_closesocket_fail(_pconnectable, disconnect_reason);

		// - decrease socket_count_allocated
		--this->m_socket_count_allocated;

		// return)
		return;
	}

	if ((_result == 0) && (count() < this->m_max_allocate) && (count() < g_max_accept_socket))
	{
		try
		{
			// 1) call 'process_on_accept'
			this->process_on_accept(_pconnectable);

			// 2) set variable
			_pconnectable->m_pconnectable_notify = this;
		}
		catch (...)
		{
			// - process_on_fail_connect
			this->process_on_fail_accept(_pconnectable);

			// - close socket
			proceoss_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_EXCEPTION_OCCURED);

			// - decrease allocated socket count
			--this->m_socket_count_allocated;

			// return)
			return;
		}

		// statistics)
		this->statistics_on_success_connect();

		// 3) call 'process_complete_connect'
		{
			// - call 'process_complete_connect'
			auto result = _pconnectable->process_complete_connect(_result);

			// check) 
			if (result == false)
			{
				// - reset variable
				//   주의) _pconnectable->m_pconnectable_notify = this 설정은 반드시 process_complete_connect(...)가 호출되기 전에 설정되어야 한다.
				//         process_complete_connect 처리 과정 중에 m_pconnectable_notify 값을 on_connect() 호출에서 인자로 넘긴다.
				_pconnectable->m_pconnectable_notify = nullptr;

				// - decrease socket_count_allocated
				--this->m_socket_count_allocated;

				// - close socket
				proceoss_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE);

				// return)
				return;
			}
		}

		// 설명) 여기서부터 실패는 접속 종료 처리를 해야한다.
		//       - 일단 on_connect()호출이 끝난 이후므로 이후에 실패시 on_disconnect()가 호출되어야 한다.
		//         따라서 이후 실패처리는 접속 종료 처리로 진행된다.
		//       - 처리 실패시 exception이 던져지고 catch에서 접속 종료 처리가 진행된다.

		try
		{
			// 4) add to connected list
			//   주의) add_connectabl(_pconnectable)은 반드시 process_complete_connect(...)의 호출 이후에 한다.
			//         그렇지 않으면 on_connect()처리 도중에 acceptor의 connectable_list에 등록되므로 여기서 socket이 참조되어 처리가 될수 있기 때문에 문제가 될수 있다.
			//         즉, on_connect() 도중에 다른 처리가 진행될 수 있다.
			//         특히 epoll의 경우 100% 문제가 발생하므로 반드시 주의해야 한다.
			{
				// - call 'add_connectable'
				auto result = this->add_connectable(_pconnectable);

				// check)
				CGASSERT_ERROR(result == true);

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
			}

			// 5) attach to io_context
			//    주의) - io_context에 attach()는 반드시 process_complete_connect()가 실행된 이후에 처리되어야 한다.
			//            process_complete_connect()함수에서 on_connect()가 호출되기 때문에 이 함수 호출이 끝난 후에
			//            receive event를 받아야 하기 때문이다.
			//            또 이 과정에서 closesocket()이 바로 호출될 수 있기 때문에 그에 대한 대비 역시 필요하다.
			//            만약에 closesocket()이 호출되었다면 peio_context->attach가 실패할 것이다.
			//          - 이 시점부터 접속성공으로 처리된다. 따라서 이 이후에 실패 처리는 접속 종료로 처리한다.
			//            즉 통계적으로 on_fail_accept가 아니라 on_disconnect 처리된다.
			//            또 on_disconnect()함수도 호출된다.
			{
				// - get io_context
				auto pio_context = this->io_context();

				// check)
				CGASSERT_ERROR(pio_context.exist());

				// 6) do start =>attach to socket eio_context
				auto result = pio_context->attach((CGHANDLE)_pconnectable->native_handle(), _pconnectable->m_pexecutable_socket, _pconnectable->m_epoll_event_flag);

				// check)
				THROW_IF(result == false, throwable::failure(eRESULT::FAIL));
			}
		}
		catch (...)
		{
			// - connectable의 socket을 닫는다.
			_pconnectable->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

			// - get connectable tcp
			auto psocket_tcp = dynamic_cast<net::io::socket::Ntcp*>(_pconnectable);

			if (psocket_tcp != nullptr)
			{
				// - call 'process_complete_connectable'
				psocket_tcp->m_pconnectable->process_complete_disconnect();
			}
		}
	}
	else
	{
		// - process_on_fail_connect
		this->process_on_fail_accept(_pconnectable);

		// - connectable의 process_complete_connect()함수를 수행한다.
		_pconnectable->process_complete_connect(1);

		// - close socket
		proceoss_closesocket_fail(_pconnectable, DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ABORTIVE);

		// statistics) 접속 실패 횟수
		this->statistics_on_fail_connect();

		// 4) 할당된 소켓수를 줄인다.
		--this->m_socket_count_allocated;
	}
}
#else
#endif

void net::io::connective::acceptor::Ntcp::process_connective_disconnect( Iconnectable* _pconnectable)
{
	// 1) connectable manager에서 제거한다. 
	this->remove_connectable(_pconnectable);

	// 2) connectable의 pconnectable notify를 this로 한다.
	_pconnectable->m_pconnectable_notify = nullptr;

	// 3) Allocated된 socket수를 줄인다.
	--this->m_socket_count_allocated;

	// statistics)
	this->statistics_on_error_disconnect(_pconnectable->statistics_get_error_disconnected());

	// statistics) 
	this->statistics_on_disconnect();

#if defined(_WINSOCK2API_)
	// check) 만약 prepare된 숫자가 모자랄 경우 추가 Accept를 건다.
	if(this->is_enable() && this->m_accept_count_must_prepare > this->m_socket_count_prepared_to_accept)
	{
		try
		{
			// request accept
			this->request_accept(this->m_address_family); // (*) exception이 발생할 수 있다.

			// check)
			CGASSERT_ERROR(this->m_socket_count_allocated != 0)
		}
		catch(std::exception& e)
		{
			// trace) 
			LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - ...
			if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
			{
				this->process_request_retry_accept();
			}
		}
		catch(...)
		{
			// trace) 
			LOG_DEBUG << "@ (excp) ACCEPTOR['" << name() << "']: exception complete socket:"sv << static_cast<int64_t>(this->native_handle()) << " " __CGFUNCTION__ ""sv;

			// declare)
			throwable::failure e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);

			// - 
			if (this->m_socket_count_prepared_to_accept == 0 && api::get_last_error() == WSAETOOMANYREFS)
			{
				this->process_request_retry_accept();
			}
		}
	}
#endif
}


}
