#include "pch.h"

using namespace CGDK;


net::io::connect_requestable::Ntcp_reconnect_secure::Ntcp_reconnect_secure() noexcept :
	net::io::connect_requestable::Ntcp_reconnect()
{
}

net::io::connect_requestable::Ntcp_reconnect_secure::Ntcp_reconnect_secure(const std::string_view _name) :
	net::io::connect_requestable::Ntcp_reconnect(_name)
{
}

net::io::connect_requestable::Ntcp_reconnect_secure::~Ntcp_reconnect_secure() noexcept
{
}

void net::io::connect_requestable::Ntcp_reconnect_secure::prepare_ssl(bool _flag_enable)
{
	// 1) prepare한다.
	if (_flag_enable == true)
	{
		m_pssl_context = make_object<crypt::ssl_context>();
	}
	else
	{
		m_pssl_context.reset();
	}
}

void net::io::connect_requestable::Ntcp_reconnect_secure::process_reset_connector() noexcept
{
	// 1) 초기화.
	net::io::connect_requestable::Ntcp_reconnect::process_reset_connector();

	// 2) SSL을 초기화 한다.
	if (m_pssl_context.exist())
	{
		m_pssl_context->initialize_client();
	}
}

void net::io::connect_requestable::Ntcp_reconnect_secure::process_complete_connecting( uintptr_t _result)
{
	if (m_pssl_context.exist())
	{
		try
		{
			// 1) 새로운 SSL객체를 생성한다.
			auto pcrypt = m_pssl_context->alloc_ssl(static_cast<int>(native_handle()));

			// 2) 초기화를 한다.
			pcrypt->initialize_handshake_client(this);
		}
		catch (...)
		{
			// - socket을 close한다.
			process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

			// - Return을 Failure으로...
		#if defined(_WINSOCK2API_)
			_result = 1;
		#elif defined(_SYS_EPOLL_H)
			_result = (EPOLLHUP | EPOLLERR);
		#endif
		}
	}

	// 2) socket Option을 바꾼다.
	net::io::connect_requestable::Ntcp_reconnect::process_complete_connecting(_result);
}
