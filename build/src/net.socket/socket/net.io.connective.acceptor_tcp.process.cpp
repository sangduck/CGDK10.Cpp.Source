#include "pch.h"

namespace CGDK
{

extern std::size_t g_max_accept_socket;

#if defined(_SYS_EPOLL_H)

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
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t /*_param*/) override { if (auto pconnective = this->m_pconnective; pconnective.exist()) pconnective->process_connective_accept(_result, this->m_handle_listen, this->m_address_family); return intptr_t(0); }

	// - the executable is pooled : releases the acceptor
	virtual void				on_final_release() noexcept override { this->m_pconnective.reset(); }

public:
	// - a reference : an accept event in process keeps the acceptor (stop() and the release of the acceptor may come meanwhile)
	//   (the acceptor releases this executable at stop : m_pexecutable_accept/m_pexecutable_accept_uds and the epoll registration)
	object_ptr<net::io::connective::acceptor::Ntcp> m_pconnective;

	// - the listen socket (INVALID_SOCKET : the acceptor's socket) and its address family (AF_UNSPEC : the acceptor's)
	CGSOCKET					m_handle_listen = INVALID_SOCKET;
	ADDRESS_FAMILY				m_address_family = AF_UNSPEC;
};



void net::io::connective::acceptor::Ntcp::_process_starting_uds(const START_PARAMETER& _parameter)
{
	//-----------------------------------------------------------------
	// - enable_uds : accepts the connections from the same host through a UDS (Unix domain socket) too.
	//   (the TCP/IP stack is skipped. connect_requestable to a loopback address of the port tries the UDS first)
	//   - the UDS : abstract namespace "\0cgdk.tcp.<listen port>" (no file)
	//   - the sockets accepted from the UDS are the same class and processed like the TCP ones. (on_connect, ...)
	//   - fails => only TCP. (logged)
	// - uds_only : accepts through the UDS only.
	//   - the TCP socket is bound (the port is reserved) but not listened. (TCP connects are refused)
	//   - fails => throws. (no listen at all)
	//-----------------------------------------------------------------

	// check) not enabled
	RETURN_IF(_parameter.enable_uds == false && _parameter.uds_only == false);

	// declare)
	const auto uds_only = _parameter.uds_only;
	auto endpoint_listen = this->local_endpoint();
	const auto port = endpoint_listen.port();

	// - fail : uds_only => error & throw, enable_uds => TCP only
	auto fail = [&](std::string_view _message, int _error_code)
	{
		if (uds_only)
		{
			LOG_ERROR << "@ (err ) ACCEPTOR['"sv << name() << "'] "sv << _message << " 'cgdk.tcp."sv << port << "'. (uds_only) error_code:"sv << _error_code;
			this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			throw throwable::failure(eRESULT::API_FAILURE, 0, "fail to listen on UDS"sv);
		}

		LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] "sv << _message << " 'cgdk.tcp."sv << port << "'. (TCP only) error_code:"sv << _error_code;
	};

	// check) a UDS reaches only the same host : listening on any or loopback only (uds_only : no TCP listen. any address)
	if (uds_only == false && endpoint_listen.address().is_unspecified() == false && endpoint_listen.address().is_loopback() == false)
	{
		EXECUTE_IF(_parameter.enable_progress_log, LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] 'enable_uds' is ignored (listen address is not any or loopback)"sv);
		return;
	}

	// 1) address (the actual listen port)
	::sockaddr_un addr_uds;
	socklen_t addr_length = 0;
	api::socket::make_uds_address(port, addr_uds, addr_length);

	// 2) create socket (protocol 0)
	auto handle_uds = api::socket::instance().create_socket(AF_UNIX, SOCK_STREAM, 0);

	// check)
	if (handle_uds == INVALID_SOCKET)
	{
		fail("fail to creating UDS socket"sv, api::get_last_error());
		return;
	}

	// 3) bind, listen (nonblocking already : create_socket)
	if (::bind(handle_uds, reinterpret_cast<::sockaddr*>(&addr_uds), addr_length) != 0 || api::socket::listen(handle_uds) == false)
	{
		const auto error_code = api::get_last_error();
		api::socket::closesocket(handle_uds);
		fail("fail to listen on UDS"sv, error_code);
		return;
	}

	// 4) executable accept (with the UDS listen socket)
	auto pexecutable_accept = make_object<executable_accept>();
	pexecutable_accept->m_pconnective = this;
	pexecutable_accept->m_handle_listen = handle_uds;
	pexecutable_accept->m_address_family = AF_UNIX;

	// 5) attach to epoll (io_uring completion : a multishot accept op instead)
	auto pio_context = io_context();

	if (this->_is_io_completion_accept())
	{
		pexecutable_accept.reset();

		if (this->_process_start_accept_cq(handle_uds, AF_UNIX) == false)
		{
			const auto error_code = api::get_last_error();
			api::socket::closesocket(handle_uds);
			fail("fail to posting the accept of the UDS socket"sv, error_code);
			return;
		}
	}
	else if (pio_context.empty() || pio_context->attach(handle_uds, pexecutable_accept, EPOLLEXCLUSIVE | EPOLLIN | EPOLLOUT | EPOLLERR) == false)
	{
		const auto error_code = api::get_last_error();
		api::socket::closesocket(handle_uds);
		fail("fail to attaching UDS socket"sv, error_code);
		return;
	}
	else
	{
		this->m_pexecutable_accept_uds = std::move(pexecutable_accept);
	}

	// 6) store
	this->m_handle_uds_listen = handle_uds;

	// trace)
	EXECUTE_IF(_parameter.enable_progress_log, LOG_INFO << "  > listen on UDS 'cgdk.tcp."sv << port << "' ("sv << (uds_only ? "uds only"sv : "same host"sv) << ")"sv);
}

void net::io::connective::acceptor::Ntcp::_process_close_uds() noexcept
{
	// 1) take the UDS listen socket
	auto handle_uds = this->m_handle_uds_listen.exchange(INVALID_SOCKET);

	// check)
	RETURN_IF(handle_uds == INVALID_SOCKET);

	// 2) detach & close (io_uring completion : the multishot accept op is cancelled first)
	if (auto pio_context = this->io_context(); pio_context.exist())
	{
		pio_context->detach((CGHANDLE)handle_uds);

		//   (the op holds the file of the listen socket : the cancel and the delivery of its completion are waited for, the UDS name is
		//    free when close() returns. see process_close_socket())
		if (this->_is_io_completion_accept())
			pio_context->wait_cancelled_fd((CGHANDLE)handle_uds, chrono::seconds(1), true);
	}

	api::socket::closesocket(handle_uds);

	// 3) reset
	this->m_pexecutable_accept_uds.reset();
}
#endif

//-----------------------------------------------------------------------------
// cpu affinity (START_PARAMETER::enable_cpu_affinity)
//
//  - one io_context (executor::io_context_threads) with one thread pinned to each CPU. created on demand, shared by the acceptors.
//  - an accepted socket is given the io_context of the CPU that received its packets (SO_INCOMING_CPU : the RSS queue of the NIC)
//    so its events are processed on that CPU (no cache line migration between the NIC softirq and the socket).
//  - stopped by a destroy handler of the system_executor (a normal shutdown) or stop_cpu_affinity_io_contexts().
//-----------------------------------------------------------------------------
namespace
{
	struct CPU_AFFINITY_IO_CONTEXTS
	{
		std::mutex			lockable;
		std::vector<object_ptr<executor::Iio_context_threads>> vector_io_context;
		bool				is_registered = false;
	};

	CPU_AFFINITY_IO_CONTEXTS& _cpu_affinity_io_contexts()
	{
		static CPU_AFFINITY_IO_CONTEXTS instance;
		return instance;
	}
}

object_ptr<executor::Iio_context> net::io::connective::acceptor::Ntcp::cpu_affinity_io_context(int _cpu)
{
	// check) unknown CPU
	RETURN_IF(_cpu < 0, object_ptr<executor::Iio_context>());

	// 1) get manager
	auto& manager = _cpu_affinity_io_contexts();

	// lock)
	std::lock_guard lock(manager.lockable);

	// 2) one slot per CPU
	if (manager.vector_io_context.empty())
		manager.vector_io_context.resize(api::get_core_count());

	// check) out of range
	RETURN_IF(static_cast<std::size_t>(_cpu) >= manager.vector_io_context.size(), object_ptr<executor::Iio_context>());

	// 3) create on demand : one thread pinned to the CPU
	auto& pio_context = manager.vector_io_context[static_cast<std::size_t>(_cpu)];

	if (pio_context.empty())
	{
		pio_context = executor::make_io_context_threads(CGDK::stream_string("cpu affinity executor [cpu:", _cpu, "]"), 1, _cpu);

		// - stopped with the system_executor (a normal shutdown)
		if (manager.is_registered == false)
		{
			manager.is_registered = true;
			system_executor::register_destroy_handler("net.io.connective.acceptor.cpu_affinity"sv, []() { net::io::connective::acceptor::Ntcp::stop_cpu_affinity_io_contexts(); });
		}
	}

	// return)
	return object_ptr<executor::Iio_context>(pio_context.get());
}

void net::io::connective::acceptor::Ntcp::stop_cpu_affinity_io_contexts() noexcept
{
	// declare)
	std::vector<object_ptr<executor::Iio_context_threads>> vector_stop;

	// 1) take them (out of the lock : stop() joins the threads)
	{
		auto& manager = _cpu_affinity_io_contexts();

		std::lock_guard lock(manager.lockable);

		vector_stop.swap(manager.vector_io_context);
		manager.is_registered = false;
	}

	// 2) stop all (request first : the threads end together)
	for (auto& iter : vector_stop)
		if (iter.exist())
			iter->request_stop();

	for (auto& iter : vector_stop)
		if (iter.exist())
			iter->stop();
}

object_ptr<executor::Iio_context> net::io::connective::acceptor::Ntcp::process_get_connectable_io_context(Iconnectable* _pconnectable)
{
	// - default : the io_context of the acceptor (empty). cpu affinity : the io_context of the CPU that received the connection
	RETURN_IF(this->m_enable_cpu_affinity == false || _pconnectable == nullptr || _pconnectable->native_handle() == INVALID_SOCKET, object_ptr<executor::Iio_context>());

	// return)
	return cpu_affinity_io_context(api::socket::get_incoming_cpu(_pconnectable->native_handle()));
}

result_code net::io::connective::acceptor::Ntcp::_process_starting(context& _context)
{
	// check) socket 상태가 CLOSED여야지만 한다.
	CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

	// declare) 
	START_PARAMETER parameter;

	// 1) set default values
	parameter.enable_progress_log = this->m_enable_progress_log;

	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter (use 'enable_progress_log' only)
		parameter = _context.v.value_buffer.front<START_PARAMETER>();
	}
	else
	{
		// - 'enable progress_log
		if (auto& context_find = _context.find("progress_log", "progress log", "enable_progress_log", "enable progress log"); context_find.is_valid())
		{
			// -get string value
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
	this->process_get_acceptor_parameters(_context, parameter);

	// - cryption type을 보관한다. (accept되는 socket마다 적용된다.)
	this->m_cryption_type = parameter.cryption_type;

	// - cpu affinity (linux : SO_INCOMING_CPU) : an accepted socket runs on the io_context of the CPU that received it
#if defined(_SYS_EPOLL_H)
	this->m_enable_cpu_affinity = parameter.enable_cpu_affinity;
	EXECUTE_IF(parameter.enable_cpu_affinity && parameter.enable_progress_log, LOG_INFO << "  - enabled 'cpu affinity' (an accepted socket runs on the io_context of its CPU)"sv);
#else
	this->m_enable_cpu_affinity = false;
	EXECUTE_IF(parameter.enable_cpu_affinity, LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] 'cpu_affinity' is ignored (linux only)"sv);
#endif

	// - the tcp socket option of the accepted sockets ("socket_option" : only when given. a socket_option() set by code before is kept) and TCP Fast Open of the listen socket
	if (parameter.flag_socket_option != 0)
	{
		this->m_socket_option = parameter.socket_option;
		this->m_flag_socket_option = true;
	}
	this->m_tcp_fastopen = parameter.tcp_fastopen;

	// 8) ProcessSetParameter
	this->process_set_parameters(_context);

	// - uds_only : the TCP socket is bound but not listened (linux)
#if defined(_SYS_EPOLL_H)
	const bool listen_tcp = (parameter.uds_only == false);
#else
	const bool listen_tcp = true;
	EXECUTE_IF(parameter.uds_only, LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] 'uds_only' is ignored (linux only)"sv);
#endif


	//-----------------------------------------------------------------
	// 3. prepare accept
	//-----------------------------------------------------------------
	// 1) Address Family를 설정한다.
	this->m_address_family = static_cast<ADDRESS_FAMILY>(parameter.local_endpoint.data().family());

	// 2) socket을 준비한다.
	this->process_prepare_socket(parameter.local_endpoint.data().family());

	// 3) 현재 <socket Handle>을 얻는다.
	CGSOCKET socket_handle_listen = this->native_handle();

	// 4) <Address Family>가 AF_INET6일 경우 IPv6 옵션을 설정한다.
	if (parameter.local_endpoint.is_v6())
	{
		// - set 'IPv6 only' option
		api::socket::set_sock_opt(socket_handle_listen, IPPROTO_IPV6, IPV6_V6ONLY, &this->m_is_ip6_only, sizeof(this->m_is_ip6_only));

		// log)
		LOG_INFO << "  ! bind ipv6 only: " << (this->m_is_ip6_only == 0 ? "false" : "true");
	}

	// 5) set socket option (uds_only : no 'address reuse'. the port stays reserved for this acceptor only)
	if (listen_tcp)
	{
		// - enable 'address reuse'
		auto result = api::socket::set_reuse_address(socket_handle_listen, parameter.reuse_address);

		// trace)
		EXECUTE_IF(result == true, LOG_INFO << "  - enabled 'SOCKET_REUSEADDRESS'"sv);

	#if defined(_WINSOCK2API_)
		// - enable 'fast loopback'
		result = api::socket::set_loopback_fast_path(socket_handle_listen, true);

		// trace)
		EXECUTE_IF(result == true, LOG_INFO << "  - enabled 'LOOPBACK_FASTPATH'"sv);
	#endif

		// - TCP Fast Open (RFC 7413) : the listen socket accepts the data in the SYN of a client with a TFO cookie (parameter 'tcp_fastopen')
		if (parameter.tcp_fastopen > 0)
		{
			result = api::socket::set_tcp_fastopen(socket_handle_listen, parameter.tcp_fastopen);

			// trace)
			if (result == true)
			{
				LOG_INFO << "  - enabled 'TCP_FASTOPEN' (queue length:"sv << parameter.tcp_fastopen << ")"sv;

			#if defined(_SYS_EPOLL_H)
				// check) the server side TFO needs sysctl net.ipv4.tcp_fastopen & 2 (the default 1 is the client side only)
				if (const auto sysctl_tfo = api::socket::get_sysctl_tcp_fastopen(); sysctl_tfo >= 0 && (sysctl_tfo & 2) == 0)
					LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] 'TCP_FASTOPEN' is set but sysctl net.ipv4.tcp_fastopen is "sv << sysctl_tfo << " (the server side needs bit 2. ex. sysctl -w net.ipv4.tcp_fastopen=3)"sv;
			#endif
			}
			else
			{
				LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] fail to enable 'TCP_FASTOPEN' (error_code:"sv << api::get_last_error() << ")"sv;
			}
		}

		// - TCP_DEFER_ACCEPT (START_PARAMETER::tcp_defer_accept, linux) : the accept wakes up when the first data of the connection arrives
		//   (a connection without data is accepted after the seconds. a server that must send first keeps it off)
		if (parameter.tcp_defer_accept > 0)
		{
		#if defined(_SYS_EPOLL_H)
			result = api::socket::set_tcp_defer_accept(socket_handle_listen, parameter.tcp_defer_accept);

			// trace)
			EXECUTE_IF(result == true, LOG_INFO << "  - enabled 'TCP_DEFER_ACCEPT' ("sv << parameter.tcp_defer_accept << "s)"sv);
			EXECUTE_IF(result == false, LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] fail to set 'TCP_DEFER_ACCEPT' error_code:"sv << api::get_last_error());
		#else
			LOG_WARNING << "  ! ACCEPTOR['"sv << name() << "'] 'tcp_defer_accept' is ignored (linux only)"sv;
		#endif
		}
	}

	// 6) start accept
	{
		while (true)
		{
			// - bind한다.
			auto result = api::socket::bind(socket_handle_listen, (::sockaddr*)&parameter.local_endpoint, parameter.local_endpoint.size<int>());

			// check) result가 false이면 bind 실패다!!!
			if (result == SOCKET_ERROR)
			{
				// - set last error
				this->m_last_socket_error = api::get_last_error();

				if (parameter.reallocate_port_at_duplicate_port_error)
				{
					// - get old port
					uint16_t port_new = parameter.local_endpoint.port() + 1;

					// - reallocate port
					parameter.local_endpoint.port(port_new);
					if (parameter.accept_endpoint.exist())
					{
						parameter.accept_endpoint.port(port_new);
					}

					LOG_INFO << "  ! retry binding at port[" << port_new << "]";

					// - continue
					continue;
				}

				// log) 
				LOG_ERROR << "@ (err ) ACCEPTOR['"sv << name() << "'] fail to binding socket for listen. error:"sv << api::get_last_error_string() << ", error_codee:"sv << api::get_last_error() << " " __CGFUNCTION__ ""sv;

				// raise) 
				throw throwable::failure(eRESULT::API_FAILURE, 0, "fail to binding socket for listen"sv);
			}

			break;
		}

		// trace)
		EXECUTE_IF(parameter.enable_progress_log, LOG_INFO << CGDK::stream_string("  > binded on ", to_string(local_endpoint()), " [requestd:", to_string(parameter.local_endpoint), "]"));

		// - listen (uds_only : not listened. TCP connects are refused)
		auto result = listen_tcp ? api::socket::listen(socket_handle_listen) : true;

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
		EXECUTE_IF(parameter.enable_progress_log && listen_tcp, LOG_INFO << CGDK::stream_string("  > listen on ", to_string(local_endpoint())));
	}

	// 7) set accept Address를 설정한다.
	if (parameter.accept_endpoint.exist())
	{
		set_accept_endpoint(parameter.accept_endpoint);
	}



	// 9) set socket state
	this->socket_state(eSOCKET_STATE::LISTEN);

	// check) 만약 parameter.prepare_on_start가 -1이면 start_accept를 수행하지 않고 그냥 0를 return한다.
	ERROR_RETURN_IF(parameter.prepare_on_start == static_cast<std::size_t>(NO_START_ACCEPT), eRESULT::SUCCESS, LOG_INFO << "  - accepting is not ready because parameter.prepare_on_start is 'NO_START_ACCEPT' [address="sv << to_string(local_endpoint()) << "]"sv);


	//-----------------------------------------------------------------
	// 4. prepare accept
	//-----------------------------------------------------------------
#if defined(_WINSOCK2API_)
	// declare) 
	int64_t prepare_on_start = static_cast<int64_t>(parameter.prepare_on_start);
	int64_t must_prepare = static_cast<int64_t>(parameter.must_prepare);

	// 1A) <parameter.prepare_on_start>이 0이라면 default
	if (prepare_on_start == 0)
	{
		// - core개수 x 8 개만큼을 준비한다.
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
		// - core개수 x (-prepare_on_start)로 설정한다.
		parameter.prepare_on_start = api::get_core_count() * (-prepare_on_start);
	}

	// check) parameter.prepare_on_start가 혹시 0개이면 끝낸다.(prepare이 하나도 없으면 accept가 되지를 않는다.)
	CGASSERT_ERROR(parameter.prepare_on_start != 0);

	// 2A) <parameter.must_prepare>이 0이라면 default
	if (must_prepare == 0)
	{
		// - core개수 x 4 개만큼을 준비한다.
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
		// - core개수 x (-must_prepare)로 설정한다.
		parameter.must_prepare = api::get_core_count() * (-must_prepare);
	}

	// 3) 최하 1개는 되어야 한다.
	SATURATE_MIN(parameter.must_prepare, 1);
	SATURATE_MIN(parameter.prepare_on_start, 1);

	// check) must_prepare이 prepare_on_start보다 클 수는 없다.
	if (parameter.prepare_on_start < parameter.must_prepare)
	{
		LOG_WARNING << "(warn) 'must_prepare' is greater than 'prepare_on_start'"sv;
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
			for (std::size_t i = parameter.prepare_on_start; i > 0; --i, ++accepted)
			{
				// - Accept를 수행한다.
				this->request_accept(this->m_address_family);
			}

			// trace) 걸어놓은 accept개수를 출력한다.
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
		catch (std::exception& _e)
		{
			// log) 
			LOG_DEBUG << "@ (excp) exception complete ACCEPTOR['"sv << name() << "'] socket:"sv << static_cast<int64_t>(this->native_handle()) << "" __CGFUNCTION__ ""sv;

			// - call 'exception handler'
			if (throwable::execute_exception_handler(this, _e, __FUNCTION__) == false)
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
			if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
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
	if (listen_tcp)
	{
		// 1) executable accept 할당받기.
		auto pexecutable_accept = make_object<executable_accept>();

		// check) 
		CGASSERT_ERROR(pexecutable_accept->reference_count() == 1);

		// 2) acceptor를 this로 설정한다.
		pexecutable_accept->m_pconnective = this;
		pexecutable_accept->m_handle_listen = INVALID_SOCKET;		// - the acceptor's socket (the executable is pooled : reset the fields)
		pexecutable_accept->m_address_family = AF_UNSPEC;

		// 4) Ready
		auto pio_context = io_context();

		// check) io_context가 없으면 잘못된 것이다.
		CGASSERT_ERROR(pio_context.exist());

		// 5) event를 설정한다. (io_uring completion : a multishot accept op instead of the registration)
		if (this->_is_io_completion_accept())
		{
			pexecutable_accept.reset();

			// check)
			THROW_IF(this->_process_start_accept_cq(this->native_handle(), AF_UNSPEC) == false, throwable::failure(eRESULT::API_FAILURE));
		}
		else
		{
			auto result = pio_context->attach(this->native_handle(), pexecutable_accept, EPOLLEXCLUSIVE | EPOLLIN | EPOLLOUT | EPOLLERR);

			// check)
			THROW_IF(result == false, throwable::failure(eRESULT::API_FAILURE));

			// 6) create executable_accept
			this->m_pexecutable_accept = std::move(pexecutable_accept);
		}
	}

	// 8) listen on the UDS of the port too or only (START_PARAMETER::enable_uds/uds_only, linux)
	this->m_accept_count_must_prepare = 0;
	this->m_listen_endpoint = parameter.local_endpoint;
	this->_process_starting_uds(parameter);
#endif

	// return)
	return eRESULT::SUCCESS;
}


}
