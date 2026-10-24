//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket mail classes                        *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::email::smtp
//
//  1) SMTP로 메일을 보낸다. (RFC 5321)
//     - 메일 하나마다 접속해서 보내고 QUIT 후 접속을 끊는다.
//     - 동시에 'max_concurrent_socket'개까지 접속하고 나머지는 queue에 넣어 차례로 보낸다.
//     - 결과는 sREQUEST::complete(또는 on_email_sent)로 한 번 알린다.
//
//  2) 보안 ("security")
//     - "ssl"      : 접속하자마자 TLS (465)
//     - "starttls" : 평문으로 접속 후 STARTTLS (587/25). 서버가 STARTTLS를 지원하지 않으면 보내지 않는다.
//     - "none"     : 암호화하지 않는다. (내부 relay 서버용. 인증 정보는 보내지 않는다.)
//     - 서버 인증서는 system CA 저장소로 검증하고 "host_name"과 비교한다. (SNI)
//
//  3) 인증 (EHLO 응답의 AUTH 중에서 XOAUTH2(token이 있을 때) > PLAIN > LOGIN 순으로 고른다.)
//
//  4) 메일 형식 (RFC 5322 / MIME)
//     - Subject, From 이름은 UTF-8 encoded-word(RFC 2047), 본문은 UTF-8 + base64
//
//  5) 설정 (start(context))
//     {
//         "remote_endpoint": { "address": "smtp.gmail.com", "port": 465 },
//         "security": "ssl",                    // ssl / starttls / none (기본: 465 -> ssl, 그 외 -> starttls)
//         "host_name": "smtp.gmail.com",        // 인증서 확인용 서버 이름 (기본: remote_endpoint.address)
//         "email_server_domain": "cgcii.co.kr", // EHLO에 쓰는 이 서버의 이름
//         "from": "noreply@cgcii.co.kr",        // 기본 보내는 사람
//         "from_name": "gruma",
//         "auth": { "name": "...", "password": "..." },
//         "timeout": 30,                        // 응답 대기 시간 (초)
//         "max_concurrent_socket": 8
//     }
//
// ----------------------------------------------------------------------------
class net::email::smtp :
	public                      object::Ninitializable_startable,
	public						object::Nnameable
{
public:
			smtp();
	virtual	~smtp() noexcept;

public:
	// - 메일을 보낸다. (입력이 잘못되었거나 시작되지 않았으면 exception을 던진다. 결과는 complete로 알린다.)
			void				send(const sREQUEST& _email_request);
			void				send(sREQUEST&& _email_request);

			size_t				get_max_concurrent_socket() const noexcept { return m_max_concurrent_socket; }
			void				set_max_concurrent_socket(size_t _value) noexcept { m_max_concurrent_socket = _value; }
			eSECURITY			get_security() const noexcept { return m_security; }
			size_t				get_queued_count() const noexcept;

	// - 메일 주소 형식 확인 (local@domain. 공백, 제어문자, <>,;" 는 허용하지 않는다.)
	[[nodiscard]] static bool	is_valid_address(std::string_view _address) noexcept;

protected:
	virtual	void				on_email_sending(const sREQUEST& /*_email_send_request*/) {}
	virtual	void				on_email_sent(const sRESULT& _email_send_result)  { if (_email_send_result.request.complete) _email_send_result.request.complete(_email_send_result); }

private:
			class socket;
			friend class socket;

	virtual result_code			_process_start(context& _context) override;
	virtual result_code			_process_stopping() noexcept override;
	virtual result_code			_process_stop() noexcept override;

			void				process_start_socket(sREQUEST&& _email_send_request);
			void				process_complete_send(const sRESULT& _email_send_result) noexcept;
			void				process_socket_closed(socket* _psocket) noexcept;
			void				process_fill_default(sREQUEST& _email_send_request) const;

	mutable	lock_object			m_cs_email_request;
			std::list<sREQUEST>	m_list_send_email;
			std::list<object_ptr<socket>> m_list_socket_exist;
			size_t				m_max_concurrent_socket{ 8 };
			bool				m_is_started{ false };

			// - setting
			sSOCKET_CONNECT_REQUEST_PARAMETER m_connect_parameter;
			eSECURITY			m_security{ eSECURITY::STARTTLS };
			std::string			m_host_name;
			std::string			m_email_server_domain;
			std::string			m_default_from;
			std::string			m_default_from_name;
			sAUTH				m_default_auth;
			chrono::tick::duration m_timeout{ std::chrono::duration_cast<chrono::tick::duration>(std::chrono::seconds(30)) };
};


}
