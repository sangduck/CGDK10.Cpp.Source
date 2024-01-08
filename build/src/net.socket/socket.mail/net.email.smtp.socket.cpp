#include "pch.h"

using namespace CGDK;


void net::email::smtp::socket::request_send(net::email::sREQUEST&& _email_send_request)
{
	// check) sender & receiver
	CGASSERT_ERROR(_email_send_request.to.empty() == false);

	// check)
	CGASSERT_ERROR(m_email_smtp_result.state == eSTATE::NONE);

	// 1) store email info
	scoped_lock(m_cs_email_smtp_result)
	{
		// check) 처리 중이면... 예외를 던진다.
		THROW_IF(m_email_smtp_result.state != eSTATE::NONE, throwable::failure(eRESULT::ACCESS_DENIED, 0, "sending is processing. cancel first"sv));

		// 1) set emal_info
		m_email_smtp_result.request = std::move(_email_send_request);
		m_email_smtp_result.state = eSTATE::REQUESTED;
		m_email_smtp_result.vector_result.clear();
	}

	// 2) on~ 함수 호출
	on_email_sending(m_email_smtp_result.request);

	// 3) ...

}

bool net::email::smtp::socket::cancel_sending() noexcept
{
	// lock) 
	std::lock_guard<lockable<>> cs(m_cs_email_smtp_result);

	// 1) clear smtp
	m_email_smtp_result.clear();

	// return) 
	return true;
}

bool net::email::smtp::socket::is_exist() const noexcept
{
	// lock) 
	std::lock_guard<lockable<>> cs(const_cast<lockable<>&>(m_cs_email_smtp_result));

	return m_email_smtp_result.request.to.empty() == false;
}

void net::email::smtp::socket::on_connect()
{
	// check)
	CGASSERT_ERROR(m_email_smtp_result.state == eSTATE::REQUESTED);
}
void net::email::smtp::socket::on_disconnect(uint64_t /*_reason*/)
{
	object_ptr<smtp> psmtp = m_psmtp;

	if (psmtp.exist())
	{
		psmtp->process_socket_closed(this);
	}
}

result_code net::email::smtp::socket::process_message(sMESSAGE& /*_msg*/)
{
	return eRESULT::BYPASS;
}

std::size_t net::email::smtp::socket::process_packet(shared_buffer& _buffer, const net::sockaddr& /*_address_peer*/)
{
	auto buf_recv = _buffer;

	// check) 
	RETURN_IF(_buffer.back<char>(-1) != '\n', 1024);

	try
	{
		// declare)
		std::vector<std::string> vector_result;

		while (buf_recv.size() != 0)
		{
			std::string_view str_temp = buf_recv.extract_text<char>('\r');
			vector_result.emplace_back(str_temp);
			buf_recv.extract<char>();
			buf_recv.extract<char>();
		}

		// check)
		THROW_IF(vector_result.empty(), throwable::failure(eRESULT::FAIL, 0, "invalid email server"sv));

		// 1) 
		object_ptr<smtp> psmtp = m_psmtp;

		// check)
		RETURN_IF(psmtp.empty(), 1024);

		// declare) 
		net::sockaddr temp_sockaddr;

		// check)
		CGASSERT_ERROR(m_email_smtp_result.state != eSTATE::NONE);

		switch(m_email_smtp_result.state)
		{
		// STEP.1.send 'HELO' command
		case	eSTATE::REQUESTED:
				{
					// - 220 cgcii.co.kr
					//   success => 220
					//   error => 554
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 220
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - set state as 1
					m_email_smtp_result.state = eSTATE::SEND_HELLO;

					// - send 'HELO'
					auto result = process_sendable(psmtp->m_buf_EHLO, psmtp->m_buf_EHLO.get_source(), 1, temp_sockaddr);

					// check)
					THROW_IF(result == false, throwable::failure(eRESULT::NOT_READY, 0, "(excp) 'fail to send message('HELO') to email server."sv));
				}
				break;

		// STEP.2.send 'MAIL FROM'
		case	eSTATE::SEND_HELLO:
				{
					// - ...
					auto iter = vector_result.begin();
					
					// - 250 www.cgcii.co.kr 
					//   success => 250
					//   error => 504,550
					auto result_code = __from_string<int>(iter->substr(0, iter->find_first_of(" "sv)));

					// check) code == 250
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// explain) 
					// S: 220 smtp.mailtrap.io SMTP ready
					// C: EHLO client.railsware.com
					// S: 250 smtp.mailtrap.io Hello client.railsware.com
					// 
					// S: 250 AUTH LOGIN PLAIN CRAM-MD5 => eLOGIN_TYPE::PLAIN_CRAM_MD5
					// S: 250 AUTH LOGIN PLAIN XOAUTH XOAUTH2 => eLOGIN_TYPE::XOAUTH_XOAUTH2
					// S: 250 AUTH LOGIN GSSAPI NTLM => eLOGIN_TYPE::GSSAPI_NTLM
					// S: 250 AUTH LOGIN GSSAPI PLAIN => eLOGIN_TYPE::GSSAPI_PLAIN

					while(iter != vector_result.end())
					{
						// - 50 ETRN
						// - 50 STARTTLS

						// - get result code
						result_code = __from_string<int>(iter->substr(0, iter->find_first_of(" "sv)));

						// - get auth type

						// - 
						++iter;
					}

					// - 
					if (this->m_email_smtp_result.auth_type == eAUTH_TYPE::NONE)
					{
						// - set state 2;
						m_email_smtp_result.state = eSTATE::SEND_MAIL_FROM;

						// - send mail
						auto buf_send = alloc_shared_buffer(1024);
						buf_send.append_text<char>("MAIL FROM:"sv);
						buf_send.append_text<char>(m_email_smtp_result.request.from);
						buf_send.append_text<char>("\r\n"sv);
						process_sendable(buf_send, buf_send.move_out_source(), 1, temp_sockaddr);
					}
					// - 
					else
					{
						// check) auth info exist?

						switch(this->m_email_smtp_result.auth_type)
						{
						case	eAUTH_TYPE::PLAIN_CRAM_MD5:
								{

								}
								break;

						case	eAUTH_TYPE::XOAUTH_XOAUTH2:
								break;

						case	eAUTH_TYPE::GSSAPI_NTLM:
								break;

						case	eAUTH_TYPE::GSSAPI_PLAIN:
								break;

						default:
								CGASSERT_ERROR(false);
								break;
						};
					}
				}
				break;

		// STEP.3.1.AUTH - SEND_AUTH
		case	eSTATE::SEND_AUTH:
				{
					switch(this->m_email_smtp_result.auth_type)
					{
					case	eAUTH_TYPE::PLAIN_CRAM_MD5:
							break;

					case	eAUTH_TYPE::XOAUTH_XOAUTH2:
							break;

					case	eAUTH_TYPE::GSSAPI_NTLM:
							break;

					case	eAUTH_TYPE::GSSAPI_PLAIN:
							break;

					default:
							CGASSERT_ERROR(false);
							break;
					};

					// - set state 2;
					m_email_smtp_result.state = eSTATE::SEND_NAME;
				}
				break;

		// STEP.3.2.AUTH - SEND_NAME
		case	eSTATE::SEND_NAME:
				{
					// explain) 
					// S: 250 AUTH LOGIN PLAIN CRAM-MD5
					// C: AUTH LOGIN
					// S: 334 DYT3jf4sdDR5
					// C: TXktVXNlcm5hbWU=
					// S: 334 LIRdf2pekwW3
					// C: TXktUGFzc3dvcmQ=
					// S: 235 Authentication successful

					switch(this->m_email_smtp_result.auth_type)
					{
					case	eAUTH_TYPE::PLAIN_CRAM_MD5:
							break;

					case	eAUTH_TYPE::XOAUTH_XOAUTH2:
							break;

					case	eAUTH_TYPE::GSSAPI_NTLM:
							break;

					case	eAUTH_TYPE::GSSAPI_PLAIN:
							break;

					default:
							CGASSERT_ERROR(false);
							break;
					};

					// - set state 2;
					m_email_smtp_result.state = eSTATE::SEND_PASSWORD;
				}
				break;

		// STEP.3.3.AUTH - SEND_PASSWORD
		case	eSTATE::SEND_PASSWORD:
				{
					// result
					// 334 – the requested security mechanism is accepted
					// 235 – authentication is successful
					// 
					// 530 – authentication problem that mostly requires the STARTTLS command to run 
					// 535 - Authentication failed
					// 538 – encryption required for a requested authentication mechanism
					// 538 – encryption required for a requested authentication mechanism
					// 550 - denotes that your email server requires SMTP authentication.It’s mostly a response to the attempt to send a message.

					// - set state 2;
					m_email_smtp_result.state = eSTATE::SEND_MAIL_FROM;
				}
				break;

		// STEP.3.send 'RCPT TO'
		case	eSTATE::SEND_MAIL_FROM:
				{
					// - 250 2.1.0 Ok
					//   success => 250
					//   error => 552, 451,452,550,553,503
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 250
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - set state 3;
					m_email_smtp_result.state = eSTATE::SEND_RCPT_TO;

					// - send mail
					auto buf_send = alloc_shared_buffer(1024);
					buf_send.append_text<char>("RCPT TO:"sv);
					buf_send.append_text<char>(m_email_smtp_result.request.to);
					buf_send.append_text<char>("\r\n"sv);
 					process_sendable(buf_send, buf_send.move_out_source(), 1, temp_sockaddr);
				}
				break;

		// STEP.4.send 'DATA'
		case	eSTATE::SEND_RCPT_TO:
				{
					// - 250 2.1.5 Ok
					//   success => 250
					//   error => 550, 551, 552,553,450,451,503
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 250
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - set state 4;
					m_email_smtp_result.state = eSTATE::REQUEST_DATA;

					// - send 'DATA\r\n'
					process_sendable(net::email::smtp::m_s_buf_DATA, net::email::smtp::m_s_buf_DATA.get_source(), 1, temp_sockaddr);
				}
				break;

		// STEP.5.send email text
		case	eSTATE::REQUEST_DATA:
				{
					// - 250 2.1.5 Ok
					//   success => 250
					//   error => 552,451,452,550,553,503
					//   error => 451,554,503

					// - 354 End data with <CR><LF>.<CR><LF>
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 354
					THROW_IF(result_code >= 400, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - set state 5;
					m_email_smtp_result.state = eSTATE::SEND_DATA;
					m_email_smtp_result.send_time = chrono::time::utc::now();

					// - make mail header
					auto buf_send = alloc_shared_buffer(8192);
					buf_send.append_text<char>("From: <"sv);
					buf_send.append_text<char>(m_email_smtp_result.request.from);
					buf_send.append_text<char>(">\r\n"sv);

					buf_send.append_text<char>("To: <"sv);
					buf_send.append_text<char>(m_email_smtp_result.request.to);
					buf_send.append_text<char>(">\r\n"sv);

					if (m_email_smtp_result.request.cc_list.empty() == false)
					{
						buf_send.append_text<char>("Cc: "sv);
						for (auto& iter : m_email_smtp_result.request.cc_list)
						{
							buf_send.append_text<char>("<"sv);
							buf_send.append_text<char>(iter);
							buf_send.append_text<char>(">,"sv);
						}
					}
					buf_send.sub_size(1);
					buf_send.append_text<char>("\r\n"sv);
					buf_send.append_text<char>("Date: "sv);
					buf_send.append_text<char>(to_string2<char>(chrono::to_date_time(chrono::time::local::now())));
					buf_send.append_text<char>(" -0500\r\n"sv);
					buf_send.append_text<char>("Subject: "sv);
					buf_send.append_text<char>(m_email_smtp_result.request.subject);
					buf_send.append_text<char>("\r\n"sv);
					buf_send.append_text<char>("\r\n"sv);

					process_sendable(buf_send, buf_send.move_out_source(), 1, temp_sockaddr);
					process_sendable(m_email_smtp_result.request.text, m_email_smtp_result.request.text.get_source(), 1, temp_sockaddr);
					process_sendable(net::email::smtp::m_s_buf_email_DATA_tail, net::email::smtp::m_s_buf_email_DATA_tail.get_source(), 1, temp_sockaddr);
				}
				break;

		// STEP.6.send 'QUIT' command
		case	eSTATE::SEND_DATA:
				{
					// - 250 2.0.0 Ok: queued as XXXXXXXx
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 250
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - set state 6;
					m_email_smtp_result.state = eSTATE::SEND_QUIT;

					// - send 'QUIT\r\n'		//
					process_sendable(m_s_buf_QUIT, m_s_buf_QUIT.get_source(), 1, temp_sockaddr);
				}
				break;

		case	eSTATE::SEND_QUIT:
				{
					// - 
					auto temp_string = vector_result.front();

					// - get result code
					auto result_code = __from_string<int>(temp_string.substr(0, temp_string.find_first_of(" "sv)));

					// check) code == 221
					THROW_IF(result_code >= 300, throwable::failure(eRESULT::FAIL, 0, "(excp) 'fail to send message('HELO') to email server."sv));

					// - connected
					m_email_smtp_result.state = eSTATE::DONE;

					// - email complete
					process_email_complete();
				}
				break;

		default:
				throw throwable::failure(eRESULT::INVALID_STATE, 0, "invalid state"sv);
		};

		// 3) append result
		m_email_smtp_result.vector_result.insert(m_email_smtp_result.vector_result.end(), vector_result.begin(), vector_result.end());

		// 4) set ...
		_buffer = buf_recv;
	}
	catch (throwable::failure& _e)
	{
		// - reason을 설정한다.
		m_email_smtp_result.result = _e.reason();

		// - call 'process_complete send'
		try
		{
			m_psmtp->process_complete_send(this);
		}
		catch (...)
		{
		}
	}
	catch (...)
	{
		// - reason을 설정한다.
		m_email_smtp_result.result = eRESULT::EXCEPTION;

		// - call 'process_complete send'
		try
		{
			m_psmtp->process_complete_send(this);
		}
		catch (...)
		{
		}
	}

	// return)
	return 1024;
}

void net::email::smtp::socket::process_reset_packetable() noexcept
{
	// lock) 
	std::lock_guard<lockable<>> cs(m_cs_email_smtp_result);

	m_email_smtp_result.clear();
}

void net::email::smtp::socket::process_email_complete()
{
	// - 
	on_email_sended(m_email_smtp_result);

	// - call 'process_complete send'
	m_psmtp->process_complete_send(this);
}
