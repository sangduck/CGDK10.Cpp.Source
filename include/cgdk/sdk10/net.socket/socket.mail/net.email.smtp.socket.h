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
// net::email::smtp::socket
//
//  1) 메일 하나를 보내는 SMTP client 접속 (net::email::smtp가 만든다.)
//     - 접속 -> 220 -> EHLO -> (STARTTLS -> EHLO) -> (AUTH) -> MAIL FROM -> RCPT TO... -> DATA -> 본문 -> QUIT
//     - 응답은 여러 줄일 수 있다. ("250-..." 다음 줄이 있음, "250 ..." 마지막 줄)
//     - 각 명령의 응답을 'timeout' 안에 받지 못하면 실패하고 접속을 끊는다.
//
// ----------------------------------------------------------------------------
class net::email::smtp::socket:
	public						net::io::connect_requestable::Ntcp_ssl,
	public						net::io::connect_notify::Nbase,
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::Nsocket_ssl,
	public						net::io::send_request::tcp::Ndefault,
	public						net::io::sender::Nstream,
	virtual public				net::io::Ipacketable
{
public:
			socket() noexcept;
	virtual	~socket() noexcept;

public:
			void				start_send(object_ptr<smtp>&& _psmtp, sREQUEST&& _email_send_request);
			bool				cancel_sending() noexcept;

			bool				is_exist() const noexcept;

protected:
	virtual void				on_connect() override;
	virtual	void				on_fail_connect(uint64_t _reason) override;
	virtual	void				on_disconnect(uint64_t _reason) override;

private:
	virtual	result_code			process_message(sMESSAGE& _msg) override;
	virtual	std::size_t			process_packet(shared_buffer& _buffer, const net::sockaddr& _address_peer) override;
	virtual	void				process_reset_packetable() noexcept override;

			// - 응답 처리 (한 응답의 모든 줄을 받은 후)
			void				process_reply(int _code);
			void				process_reply_ehlo(int _code);
			void				process_start_auth();
			void				process_send_mail_from();
			void				process_send_data();

			// - 송신 / 상태
			void				process_send_command(std::string_view _command, eSTATE _state_next, bool _is_secret = false);
			void				process_start_tls();
			void				process_start_timer();
			void				process_complete(eRESULT _result, std::string_view _message) noexcept;
			void				process_fail(eRESULT _result, std::string_view _message) noexcept;
			void				process_close() noexcept;
			void				process_notify_closed() noexcept;

	// - capabilities (EHLO 응답)
			struct sCAPABILITY
			{
				bool			starttls{ false };
				bool			auth_plain{ false };
				bool			auth_login{ false };
				bool			auth_xoauth2{ false };
				bool			smtputf8{ false };
				std::size_t		size_max{ 0 };
				void			clear() noexcept { *this = sCAPABILITY{}; }
			};

	mutable	lock_object			m_cs_email_smtp_result;
			sRESULT				m_email_smtp_result;
			object_ptr<smtp>	m_psmtp;
			sCAPABILITY			m_capability;
			std::vector<std::string> m_vector_reply_lines;
			std::vector<std::string> m_vector_recipient;
			std::size_t			m_index_recipient{ 0 };
			bool				m_is_tls{ false };
			bool				m_is_completed{ false };
			bool				m_is_closed_notified{ false };
			std::atomic<uint64_t> m_step_serial{ 0 };

			friend class smtp;
};


}
