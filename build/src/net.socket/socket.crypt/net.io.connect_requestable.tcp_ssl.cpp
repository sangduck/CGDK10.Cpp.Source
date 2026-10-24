#include "pch.h"

using namespace CGDK;


net::io::connect_requestable::Ntcp_ssl::Ntcp_ssl() noexcept :
	net::io::connect_requestable::Ntcp()
{
}

net::io::connect_requestable::Ntcp_ssl::~Ntcp_ssl() noexcept
{
}

void net::io::connect_requestable::Ntcp_ssl::prepare_ssl(bool _flag_enable, std::string_view _host_name)
{
	// 1) prepare한다.
	//    - client용 ssl context는 공유한다. (서버 인증서를 system CA 저장소로 검증한다.)
	if (_flag_enable == true)
	{
		try
		{
			this->m_pssl_context = crypt::ssl_context::get_default_client();
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "(err) CGDK.net.socket: fail to create ssl client context"sv;

			// - 초기화되지 않은 context를 설정해 접속(handshake)이 실패하도록 한다. (암호화 없이 접속하지 않는다.)
			this->m_pssl_context = make_object<crypt::ssl_context>();
		}

		this->m_ssl_host_name = _host_name;
	}
	else
	{
		this->m_pssl_context.reset();
		this->m_ssl_host_name.clear();
	}
}

void net::io::connect_requestable::Ntcp_ssl::process_reset_connector() noexcept
{
	// 1) 초기화.
	this->net::io::connect_requestable::Ntcp::process_reset_connector();

	// 2) client용 ssl context는 prepare_ssl에서 공유 context로 설정되므로 다시 초기화하지 않는다.
}

void net::io::connect_requestable::Ntcp_ssl::process_complete_connecting( uintptr_t _result)
{
	auto pssl_context = this->m_pssl_context;

	if (pssl_context.exist())
	{
		try
		{
			// 1) 새로운 SSL객체를 생성한다.
			auto pcrypt = pssl_context->alloc_ssl(static_cast<int>(this->native_handle()));

			// 2) 접속할 서버 이름을 설정한다. (SNI + 서버 인증서 hostname 검증)
			pcrypt->set_host_name(this->m_ssl_host_name);

			// 3) 초기화를 한다. (handshake는 기다리지 않는다. 접속 후 ClientHello를 보낸다.)
			pcrypt->initialize_handshake_client(this);
		}
		catch (...)
		{
			// log)
			try { throw; }
			catch (throwable::failure& _e) { LOG_ERROR << "(err) CGDK.net.socket: ssl initialize failed (host:"sv << this->m_ssl_host_name << ") "sv << _e.description(); }
			catch (...) { LOG_ERROR << "(err) CGDK.net.socket: ssl initialize failed (host:"sv << this->m_ssl_host_name << ")"sv; }

			// - socket을 close한다.
			this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

			// - Return을 Failure으로...
		#if defined(_WINSOCK2API_)
			_result = 1;
		#elif defined(_SYS_EPOLL_H)
			_result = (EPOLLHUP | EPOLLERR);
		#endif
		}
	}

	// 4) socket Option을 바꾼다. (접속 처리. on_connect가 호출되고 receive가 시작된다.)
	this->net::io::connect_requestable::Ntcp::process_complete_connecting(_result);

	// 5) (비동기 handshake) 접속에 성공했으면 ClientHello를 보낸다.
	//    - on_connect에서 이미 송신했다면 그때 시작되었으므로 할 것이 없다.
	//    - 서버의 응답은 수신 처리에서 진행된다.
	if (pssl_context.exist() && this->socket_state() >= eSOCKET_STATE::ESTABLISHED)
	{
		if (auto psocket_ssl = dynamic_cast<net::io::Nsocket_ssl*>(this); psocket_ssl != nullptr)
		{
			try
			{
				psocket_ssl->ssl_start_handshake();

				// - handshake 제한 시간 (서버가 응답하지 않는 경우)
				psocket_ssl->start_ssl_handshake_timer(std::chrono::duration_cast<chrono::tick::duration>(net::io::Nsocket_ssl::DEFAULT_SSL_HANDSHAKE_TIMEOUT));
			}
			catch (throwable::failure& _e)
			{
				LOG_ERROR << "(err) CGDK.net.socket: ssl handshake failed (host:"sv << this->m_ssl_host_name << ") "sv << _e.description();
				this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			}
			catch (...)
			{
				this->process_close_socket(DISCONNECT_REASON_FAIL | DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			}
		}
	}
}
