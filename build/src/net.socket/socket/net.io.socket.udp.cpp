#include "pch.h"

using namespace CGDK;

std::size_t net::io::socket::Nudp::m_s_default_maximum_send_buffer_size = DEFAULT_UDP_SEND_BUFFER_SIZE;
std::size_t net::io::socket::Nudp::m_s_default_receive_buffer_size = DEFAULT_UDP_RECEIVE_BUFFER_SIZE;

#if defined(_SYS_EPOLL_H)
intptr_t net::io::socket::Nudp::process_execute_epoll(intptr_t _event, std::size_t _param)
{
	try
	{
		// 1) EPOLLOUT - event signaled on sendable
		if (_event & EPOLLOUT)
		{
			// check) 
			CGASSERT_ERROR(m_psendable != nullptr);

			// - call 'process_recover_sendable'
			this->m_psendable->process_recover_sendable(_event, _param);
		}

		// 2) EPOLLIN
		if (_event & EPOLLIN)
		{
			// check) 
			CGASSERT_ERROR(this->m_preceivable_datagram != nullptr);

			// declare) 
			shared_buffer buffer;
			net::sockaddr address;

			// - call 'process_complete_receivable'
			this->m_preceivable_datagram->process_complete_receivable(_event, _param, buffer, address);
		}

		// 3) if error is occured, add socket reason DISCONNECT_REASON_FAIL 
		if (_event & EPOLLERR)
		{
			//m_disconnect_reason |= DISCONNECT_REASON_FAIL;
		}
	}
	catch (...)
	{

	}

	// 5) addref
	//if (native_handle() != INVALID_SOCKET)
	{
		this->m_pexecutable_socket->add_ref();
	}

	// return) 
	return	0;
}
#endif

net::io::socket::Nudp::Nudp()
{
	// statistics)
	this->statistics_set_protocol(IPPROTO_UDP);
}

net::io::socket::Nudp::~Nudp() noexcept
{
	// 1) socket을 닫는다.
	this->process_close_socket(DISCONNECT_REASON_NONE);

	// check) 
	CGASSERT_ERROR(this->native_handle() == INVALID_SOCKET);
}

void net::io::socket::Nudp::process_create_socket(ADDRESS_FAMILY _address_family)
{
	// 1) socket 생성
	auto handle_socket_new = api::socket::instance().create_socket(_address_family, SOCK_DGRAM, IPPROTO_UDP);

	// check) socket Option의 수행에 실패했는가?
	if (handle_socket_new == INVALID_SOCKET)
	{
		// - set last error
		this->m_last_socket_error = api::get_last_error();

		// log) 
		LOG_DEBUG << "(err ) CGDK.net.socket: Fail to creating socket[Error:"sv << api::get_last_error() << "]"sv;

		// raise) 
		throw throwable::failure(eRESULT::API_FAILURE, api::get_last_error(), "fail to creating UDP socket handle"sv);
	}

	// 2) connect reset기능을 설정한다.(TCP에는 상관없는 기능이다.)
	//    - 이것을 0으로 설정하면 receive Completion이 빈번히 일어나는 것을 막을 수 있다.(Server UDP의 Default는 FALSE, Client UDP의 Default는 true이다.)
#if defined(_WINSOCK2API_)
	api::socket::io_ctrl(handle_socket_new, SIO_UDP_CONNRESET, &m_enable_report_connect_reset);
#endif

	// 3) set socket option
	api::socket::set_send_buffer_size(handle_socket_new, DEFAULT_UDP_SEND_BUFFER_SIZE);
	api::socket::set_receive_buffer_size(handle_socket_new, DEFAULT_UDP_RECEIVE_BUFFER_SIZE);
	api::socket::set_nonblock(handle_socket_new, true);

	// 4) 새소켓을 설정한다.
	auto handle_socket_old = this->native_handle(handle_socket_new);

	// 5) 기존에 socket이 있다면 먼저 closesocket한다.
	if (handle_socket_old != INVALID_SOCKET)
	{
#if defined(_SYS_EPOLL_H)
		// check) io_context must exist
		CGASSERT_ERROR(this->io_context().exist());

		// - detach socket from epoll
		this->io_context()->detach((CGHANDLE)handle_socket_old);
#endif
		// - closesocket
		api::socket::closesocket(handle_socket_old);
	}

	// check) _address_family가 AF_UNSPEC이어서는 안된다.
	CGASSERT_ERROR(_address_family != AF_UNSPEC);
	CGASSERT_ERROR(_address_family == AF_INET || _address_family == AF_INET6);

	// 6) set address family
	this->address_family(_address_family);

	// 7) set socket option
	api::socket::set_nonblock(handle_socket_new, true);

	// 8) get_io_context
	auto pio_context = this->io_context();

	// 9) set default io_context if empty
	if (pio_context.empty())
	{
		pio_context = system_executor::executor();
		this->io_context(pio_context);
	}

#if defined(_SYS_EPOLL_H)
	// 10) prepare executable socket
	this->process_prepare_executable_socket();
#endif

	// check) 
	CGASSERT_ERROR(pio_context.exist());

	// 11) attach to io_context
#if defined(_WINSOCK2API_)
	auto result = pio_context->attach((CGHANDLE)handle_socket_new);
#elif defined(_SYS_EPOLL_H)
	auto result = pio_context->attach((CGHANDLE)handle_socket_new, m_pexecutable_socket, 0);
#endif

	// check)
	THROW_IF(result == false, throwable::failure(eRESULT::FAIL_TO_ATTACHING_TO_EXECUTOR, 0, "(err ) CGDK.net.socket: Fail to attaching UDP socket to io_context"sv), LOG_DEBUG << "(err ) CGDK.net.socket: fail to attaching UDP socket to io_context " __CGFUNCTION__ ""sv);
}

void net::io::socket::Nudp::process_dispose_socket() noexcept
{
}

bool net::io::socket::Nudp::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	//-----------------------------------------------------------------
	// 1. socket을 얻는다.
	//    - 이 때 atomic함수를 사용하여 socket을 변경하고 이전
	//      socket Handle을 얻어낸다. 이 이전 socket Handle이 
	//      INVALID_SOCKET이 아니면 Socketdmf 닫는다.
	//-----------------------------------------------------------------
	// 1) socket Handle을 INVALID_SOCKET으로 설정하고 이전 socket핸들을
	//    얻는다.
	auto socket_handle = this->native_handle(INVALID_SOCKET);

	// check) 이전 socket의 HANDLE이 이미 INVALID_SOCKET이면 socket이
	//        생성된 것이 아니기 때문에 그냥 끝낸다.
	RETURN_IF(socket_handle == INVALID_SOCKET, false);

	// 2) Reason을 설정한다.
	this->m_disconnect_reason |= _disconnect_reason;

	// 3) dispose
	this->process_dispose_socket();

	// 4) on_closesocket함수를 호출한다.
	this->on_closesocket(this->m_disconnect_reason);

#if defined(_SYS_EPOLL_H)
	// check) io_context must exist
	CGASSERT_ERROR(io_context().exist());

	// 5) detach socket from epoll
	this->io_context()->detach((CGHANDLE)socket_handle);
#endif

	// 4) socket을 닫는다.
	api::socket::closesocket(socket_handle);

	// 5) socket의 I/O를 완료할 때까지 기다린다.io_context가 있을 경우만 처리한다...
	while(this->statistics_get_now_async_io() != 0)
	{
		// - io_context를 돌려 IOCP의 I/O를 처리한다.
		system_executor::run_executor();

		// - 잠시 Sleep한다.
		api::sleep_for(1ms);
	}

	// 6) reset io_context
	this->reset_io_context();

	// return) 성공~
	return true;
}

bool net::io::socket::Nudp::set_multicast_ttl(int _TTL)
{
	// check)
	RETURN_IF(_TTL < 1 || _TTL > 255, false);

	// 1) Set socket Option
	return api::socket::set_sock_opt(this->native_handle(), IPPROTO_IP, IP_MULTICAST_TTL, (char*)&_TTL, sizeof(_TTL));
}

bool net::io::socket::Nudp::set_multicast_loopback(bool _flag_enable)
{
	// declare)
	int	is_enabled = _flag_enable ? 1 : 0;

	// 1) Set socket Option
	return api::socket::set_sock_opt(this->native_handle(), IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&is_enabled, sizeof(is_enabled));
}

bool net::io::socket::Nudp::add_multicast_membership(const ip_mreq& _ip_mreq_add)
{
	// 1) Set socket Option
	return api::socket::set_sock_opt(this->native_handle(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&_ip_mreq_add, sizeof(ip_mreq));
}

bool net::io::socket::Nudp::drop_multicast_membership(const ip_mreq& _ip_mreq_drop)
{
	// 1) Set socket Option
	return api::socket::set_sock_opt(this->native_handle(), IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&_ip_mreq_drop, sizeof(ip_mreq));
}

result_code net::io::socket::Nudp::process_start(context& _context)
{
	// log)
	LOG_PROGRESS << "@ <BEGIN> start udp socket['" << name() << "']";

	// 1) Process start
	auto result = this->Nstartable::process_start(_context);

	// log)
	LOG_PROGRESS << "@ <END> start udp socket['" << name() << "']";

	// return)
	return result;
}

result_code net::io::socket::Nudp::process_stop() noexcept
{
	// log)
	LOG_PROGRESS << "@ <BEGIN> stop udp socket['" << name() << "']";

	// 1) Process start
	auto result = this->Nstartable::process_stop();

	// log)
	LOG_PROGRESS << "@ <END> stop udp socket['" << name() << "']";

	// return)
	return result;
}

result_code net::io::socket::Nudp::_process_starting(context& _context)
{
	// check) socket 상태가 CLOSED여야지만 한다.
	CGASSERT_ERROR(this->socket_state() == eSOCKET_STATE::CLOSED);

	// 1) on_starting 함수를 호출한다.
	on_starting(_context);

	try
	{
		// declare)
		START_PARAMETER parameter;

		// 2) get parameters
		if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
		{
			parameter = _context.v.value_buffer.front<START_PARAMETER>();
		}
		else
		{
			// - get bind_endpoint parameter
			auto socoekt_connection_parameter = get_parameter_connection(_context, true);

			// - set bind_endpoint
			parameter.bind_endpoint = socoekt_connection_parameter.bind_endpoint;

			// - enable connect reset
			if (auto& context_find = _context.find("enable_report_connect_reset", "enable report connect reset"); context_find.is_valid())
			{
				// - ReconnectionInterval값을 얻는다.
				parameter.enable_report_connect_reset = context_find;

				// log)
				EXECUTE_IF(parameter.enable_report_connect_reset, LOG_INFO << "  + parameter['enable report connect reset']: " << (const char*)((parameter.enable_report_connect_reset) ? "true" : "false"));
			}
		}

		// 3) default bind end_point
		if (parameter.bind_endpoint.empty())
		{
			parameter.bind_endpoint = end_point_type{ { net::ip::address_v4::loopback() } , 0 };
		}

		// 5) bind한다.
		this->bind(parameter.bind_endpoint);

		// 6) connect reset기능을 설정한다.(TCP에는 상관없는 기능이다.)
		//    - 이것을 0으로 설정하면 receive Completion이 빈번히 일어나는 것을 막을 수 있다.(Server UDP의 Default는 FALSE, Client UDP의 Default는 true이다.)
	#if defined(_WINSOCK2API_)
		api::socket::io_ctrl(native_handle(), SIO_UDP_CONNRESET, &parameter.enable_report_connect_reset);
	#endif

		// 7) set value
		this->m_enable_report_connect_reset = parameter.enable_report_connect_reset;
	}
	catch (std::exception& e)
	{
		// log) prepare receive에 실패했다!
		LOG_ERROR << "@ (excp) CGDK::net::io::socket::Nudp - exception complete process_start:'" << name() << "'(" << static_cast<int64_t>(native_handle()) << ") " __CGFUNCTION__;

		// - call 'exception handler'
		if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
		{
			// reraise)
			throw;
		}
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::socket::Nudp::_process_stopping() noexcept
{
	// 1) on_stopping 함수를 호출한다.
	try
	{
		this->on_stopping();
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ (excp) Exception on 'on_stopping' function call SOCKET '" << name() << "' " __CGFUNCTION__;
	}

	// 2) socket을 닫는다.
	this->closesocket();

	// return)
	return eRESULT::SUCCESS;
}
