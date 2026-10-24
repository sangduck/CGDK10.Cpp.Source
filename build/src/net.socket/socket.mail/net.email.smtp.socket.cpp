#include "pch.h"
#include <ctime>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <random>

using namespace CGDK;

//-----------------------------------------------------------------------------
// helper
//-----------------------------------------------------------------------------
namespace
{
	// - 응답 한 줄의 최대 길이 (RFC 5321: 512. 넉넉하게 잡는다.)
	constexpr std::size_t MAX_REPLY_LINE_LENGTH = 4096;
	// - 응답 줄 보관 개수 (EHLO 응답 등. 더 많으면 앞의 것은 버린다.)
	constexpr std::size_t MAX_REPLY_LINES = 256;

	// 1) base64 (RFC 4648)
	std::string _encode_base64(std::string_view _source)
	{
		static constexpr char TABLE[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		std::string result;
		result.reserve((_source.size() + 2) / 3 * 4);

		std::size_t i = 0;
		for (; i + 3 <= _source.size(); i += 3)
		{
			uint32_t value = (static_cast<uint8_t>(_source[i]) << 16) | (static_cast<uint8_t>(_source[i + 1]) << 8) | static_cast<uint8_t>(_source[i + 2]);
			result += TABLE[(value >> 18) & 0x3f];
			result += TABLE[(value >> 12) & 0x3f];
			result += TABLE[(value >> 6) & 0x3f];
			result += TABLE[value & 0x3f];
		}

		if (auto remained = _source.size() - i; remained == 1)
		{
			uint32_t value = static_cast<uint8_t>(_source[i]) << 16;
			result += TABLE[(value >> 18) & 0x3f];
			result += TABLE[(value >> 12) & 0x3f];
			result += "==";
		}
		else if (remained == 2)
		{
			uint32_t value = (static_cast<uint8_t>(_source[i]) << 16) | (static_cast<uint8_t>(_source[i + 1]) << 8);
			result += TABLE[(value >> 18) & 0x3f];
			result += TABLE[(value >> 12) & 0x3f];
			result += TABLE[(value >> 6) & 0x3f];
			result += '=';
		}

		return result;
	}

	// 2) 헤더 값에서 CR/LF를 지운다. (헤더 주입 방지)
	std::string _strip_crlf(std::string_view _value)
	{
		std::string result;
		result.reserve(_value.size());
		for (auto iter : _value)
		{
			if (iter != '\r' && iter != '\n')
				result += iter;
		}
		return result;
	}

	// 3) RFC 2047 encoded-word (UTF-8, base64). ASCII만 있으면 그대로 둔다.
	//    - encoded-word 하나는 75자 이하여야 하므로 UTF-8 문자 경계에서 나눈다. (45 bytes -> base64 60자)
	std::string _encode_header_word(std::string_view _value)
	{
		auto value = _strip_crlf(_value);

		bool is_ascii = std::all_of(value.begin(), value.end(), [](unsigned char _ch) { return _ch >= 0x20 && _ch < 0x7f; });
		if (is_ascii && value.find("=?") == std::string::npos)
			return value;

		std::string result;
		std::size_t pos = 0;
		while (pos < value.size())
		{
			std::size_t length = std::min<std::size_t>(45, value.size() - pos);

			// - UTF-8 연속 byte(10xxxxxx)에서 끊지 않는다.
			while (length > 0 && pos + length < value.size() && (static_cast<uint8_t>(value[pos + length]) & 0xc0) == 0x80)
				--length;
			if (length == 0)
				length = std::min<std::size_t>(45, value.size() - pos);

			if (!result.empty())
				result += "\r\n ";
			result += "=?UTF-8?B?";
			result += _encode_base64(std::string_view(value).substr(pos, length));
			result += "?=";

			pos += length;
		}
		return result;
	}

	// 4) Date (RFC 5322, UTC)
	std::string _make_date()
	{
		static constexpr const char* DAYS[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		static constexpr const char* MONTHS[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

		std::time_t now = std::time(nullptr);
		std::tm tm_now{};
	#if defined(_WIN32)
		gmtime_s(&tm_now, &now);
	#else
		gmtime_r(&now, &tm_now);
	#endif

		return std::format("{}, {:02} {} {} {:02}:{:02}:{:02} +0000", DAYS[tm_now.tm_wday % 7], tm_now.tm_mday, MONTHS[tm_now.tm_mon % 12], tm_now.tm_year + 1900, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	}

	// 5) Message-ID
	std::string _make_message_id(std::string_view _domain)
	{
		static std::atomic<uint64_t> s_serial{ 0 };
		std::random_device random;
		uint64_t value = (static_cast<uint64_t>(random()) << 32) | random();
		return std::format("<{:016x}.{:x}.{:x}@{}>", value, static_cast<uint64_t>(std::time(nullptr)), ++s_serial, _domain);
	}

	// 6) 응답 줄에서 code 뒤의 내용 ("250-SIZE 1000" -> "SIZE 1000")
	std::string_view _get_reply_text(std::string_view _line) noexcept
	{
		return (_line.size() > 4) ? _line.substr(4) : std::string_view();
	}

	// 7) 대문자로 비교
	bool _starts_with_nocase(std::string_view _text, std::string_view _prefix) noexcept
	{
		if (_text.size() < _prefix.size())
			return false;
		for (std::size_t i = 0; i < _prefix.size(); ++i)
		{
			if (std::toupper(static_cast<unsigned char>(_text[i])) != std::toupper(static_cast<unsigned char>(_prefix[i])))
				return false;
		}
		return true;
	}
}


//-----------------------------------------------------------------------------
// net::email::smtp::socket
//-----------------------------------------------------------------------------
net::email::smtp::socket::socket() noexcept
{
}

net::email::smtp::socket::~socket() noexcept
{
}

void net::email::smtp::socket::start_send(object_ptr<smtp>&& _psmtp, net::email::sREQUEST&& _email_send_request)
{
	// check)
	CGASSERT_ERROR(_psmtp.exist());

	// declare)
	auto security = _psmtp->m_security;
	auto host_name = _psmtp->m_host_name;

	// 1) store email info
	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		this->m_psmtp = std::move(_psmtp);
		this->m_email_smtp_result.clear();
		this->m_email_smtp_result.request = std::move(_email_send_request);
		this->m_email_smtp_result.state = eSTATE::REQUESTED;
		this->m_capability.clear();
		this->m_vector_reply_lines.clear();
		this->m_index_recipient = 0;
		this->m_is_tls = (security == eSECURITY::SSL);
		this->m_is_completed = false;
		this->m_is_closed_notified = false;

		// - 받는 사람 (To + Cc + Bcc)
		const auto& request = this->m_email_smtp_result.request;
		this->m_vector_recipient.clear();
		this->m_vector_recipient.push_back(request.to);
		this->m_vector_recipient.insert(this->m_vector_recipient.end(), request.cc_list.begin(), request.cc_list.end());
		this->m_vector_recipient.insert(this->m_vector_recipient.end(), request.bcc_list.begin(), request.bcc_list.end());
	}

	// 2) ssl (implicit TLS만 접속할 때 시작한다. STARTTLS는 응답을 받은 후 시작한다.)
	this->prepare_ssl(security == eSECURITY::SSL, host_name);

	// 3) 접속 (connect + 서버 인사(220) 대기 시간)
	try
	{
		this->process_start_timer();
		this->start(this->m_psmtp->m_connect_parameter);
	}
	catch (throwable::failure& _e)
	{
		this->process_complete(eRESULT::FAIL_TO_CONNECT, _e.description());
		this->process_notify_closed();
	}
	catch (...)
	{
		this->process_complete(eRESULT::FAIL_TO_CONNECT, "fail to connect to email server"sv);
		this->process_notify_closed();
	}
}

bool net::email::smtp::socket::cancel_sending() noexcept
{
	// 1) 취소 (아직 끝나지 않았으면 CANCEL로 알린다.)
	this->process_fail(eRESULT::CANCEL, "canceled"sv);

	// return)
	return true;
}

bool net::email::smtp::socket::is_exist() const noexcept
{
	// lock)
	std::lock_guard cs(this->m_cs_email_smtp_result);

	return !this->m_email_smtp_result.request.to.empty() && !this->m_is_completed;
}

void net::email::smtp::socket::on_connect()
{
	// 1) 서버 인사(220)를 기다린다. (implicit TLS면 handshake 후에 받는다.)
	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		if (this->m_email_smtp_result.state == eSTATE::REQUESTED)
			this->m_email_smtp_result.state = eSTATE::GREETING;
	}
}

void net::email::smtp::socket::on_fail_connect(uint64_t /*_reason*/)
{
	this->process_complete(eRESULT::FAIL_TO_CONNECT, "fail to connect to email server"sv);
	this->process_notify_closed();
}

void net::email::smtp::socket::on_disconnect(uint64_t /*_reason*/)
{
	// declare)
	eSTATE state;

	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		state = this->m_email_smtp_result.state;
	}

	// 1) 끝나기 전에 끊어졌으면 실패로 알린다.
	this->process_complete((state <= eSTATE::GREETING) ? eRESULT::FAIL_TO_CONNECT : eRESULT::FAIL, "disconnected from email server"sv);

	// 2) smtp에 알린다. (다음 메일을 보낸다.)
	this->process_notify_closed();
}

result_code net::email::smtp::socket::process_message(sMESSAGE& /*_msg*/)
{
	return eRESULT::BYPASS;
}

std::size_t net::email::smtp::socket::process_packet(shared_buffer& _buffer, const net::sockaddr& /*_address_peer*/)
{
	// declare)
	const char* pdata = reinterpret_cast<const char*>(_buffer.data());
	const std::size_t size = _buffer.size();
	std::size_t pos = 0;

	try
	{
		// 1) 줄 단위로 응답을 모은다. ("250-..." 은 다음 줄이 있고 "250 ..." 이 마지막 줄)
		while (pos < size)
		{
			// - 줄 끝
			const void* pfind = std::memchr(pdata + pos, '\n', size - pos);
			if (pfind == nullptr)
				break;

			std::size_t pos_end = static_cast<const char*>(pfind) - pdata;
			std::string_view line(pdata + pos, pos_end - pos);
			pos = pos_end + 1;

			if (!line.empty() && line.back() == '\r')
				line.remove_suffix(1);

			// check) 끝났거나 닫는 중이면 무시한다.
			{
				std::lock_guard cs(this->m_cs_email_smtp_result);
				if (this->m_is_completed && this->m_email_smtp_result.state != eSTATE::SEND_QUIT)
					continue;
			}

			// check) "ddd" + (' ' 또는 '-')
			if (line.size() < 3 || !std::isdigit(static_cast<unsigned char>(line[0])) || !std::isdigit(static_cast<unsigned char>(line[1])) || !std::isdigit(static_cast<unsigned char>(line[2])) || (line.size() > 3 && line[3] != ' ' && line[3] != '-'))
				throw throwable::failure(eRESULT::FAIL, 0, std::format("invalid reply from email server '{}'", _strip_crlf(line.substr(0, 64))));

			// - 보관
			if (this->m_vector_reply_lines.size() < MAX_REPLY_LINES)
				this->m_vector_reply_lines.emplace_back(line);

			// - 마지막 줄이면 응답을 처리한다.
			if (line.size() == 3 || line[3] == ' ')
			{
				int code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');

				this->process_reply(code);
				this->m_vector_reply_lines.clear();
			}
		}

		// check) 줄 끝이 없이 너무 길면 잘못된 서버다.
		if (size - pos > MAX_REPLY_LINE_LENGTH)
			throw throwable::failure(eRESULT::FAIL, 0, "too long reply line from email server"sv);
	}
	catch (throwable::failure& _e)
	{
		this->process_fail(_e.reason<eRESULT>(), _e.description());
		pos = size;
	}
	catch (const std::exception& _e)
	{
		this->process_fail(eRESULT::EXCEPTION, _e.what());
		pos = size;
	}
	catch (...)
	{
		this->process_fail(eRESULT::EXCEPTION, "exception"sv);
		pos = size;
	}

	// 2) 처리한 만큼 버린다.
	_buffer = buffer_view(_buffer.data() + pos, size - pos);

	// return) 다음 응답을 받을 크기
	return __select_max<std::size_t>(1024, (size - pos) + 512);
}

void net::email::smtp::socket::process_reset_packetable() noexcept
{
	this->m_vector_reply_lines.clear();
}

void net::email::smtp::socket::process_reply(int _code)
{
	// declare)
	eSTATE state;

	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		state = this->m_email_smtp_result.state;
		this->m_email_smtp_result.reply_code = _code;

		// - 서버 응답을 남긴다. (인증 challenge(334)는 남기지 않는다.)
		if (_code != 334)
		{
			for (auto& iter : this->m_vector_reply_lines)
				this->m_email_smtp_result.vector_result.push_back(iter);
		}
	}

	// - 실패 응답의 내용 (마지막 줄)
	auto reply_text = this->m_vector_reply_lines.empty() ? std::string() : std::string(this->m_vector_reply_lines.back());

	switch (state)
	{
	// 1) 서버 인사 -> EHLO
	case	eSTATE::GREETING:
			if (_code != 220)
				throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused connection: {}", reply_text));

			this->process_send_command(std::format("EHLO {}", this->m_psmtp->m_email_server_domain), eSTATE::SEND_EHLO);
			break;

	// 2) EHLO 응답 -> (STARTTLS) / AUTH / MAIL FROM
	case	eSTATE::SEND_EHLO:
	case	eSTATE::SEND_EHLO_TLS:
			this->process_reply_ehlo(_code);
			break;

	// 3) STARTTLS 응답 -> TLS 시작 -> EHLO (handshake가 끝나면 전송된다.)
	case	eSTATE::SEND_STARTTLS:
			if (_code != 220)
				throw throwable::failure(eRESULT::NOT_SUPPORT, 0, std::format("email server refused STARTTLS: {}", reply_text));

			this->process_start_tls();
			this->process_send_command(std::format("EHLO {}", this->m_psmtp->m_email_server_domain), eSTATE::SEND_EHLO_TLS);
			break;

	// 4) AUTH LOGIN: 사용자 이름 요청(334)
	case	eSTATE::SEND_NAME:
			if (_code != 334)
				throw throwable::failure(eRESULT::INVALID_PASSWORD, 0, std::format("email authentication failed: {}", reply_text));

			this->process_send_command(_encode_base64(this->m_email_smtp_result.request.auth.name), eSTATE::SEND_PASSWORD, true);
			break;

	// 5) AUTH LOGIN: 비밀번호 요청(334)
	case	eSTATE::SEND_PASSWORD:
			if (_code != 334)
				throw throwable::failure(eRESULT::INVALID_PASSWORD, 0, std::format("email authentication failed: {}", reply_text));

			this->process_send_command(_encode_base64(this->m_email_smtp_result.request.auth.password), eSTATE::SEND_AUTH, true);
			break;

	// 6) 인증 결과(235) -> MAIL FROM
	case	eSTATE::SEND_AUTH:
			if (_code == 334)
			{
				// - XOAUTH2 실패는 오류 내용(334)을 보낸 후 빈 줄을 받으면 535를 보낸다.
				this->process_send_command(""sv, eSTATE::SEND_AUTH, true);
				break;
			}

			if (_code != 235)
				throw throwable::failure(eRESULT::INVALID_PASSWORD, 0, std::format("email authentication failed: {}", reply_text));

			this->process_send_mail_from();
			break;

	// 7) MAIL FROM 응답 -> RCPT TO
	case	eSTATE::SEND_MAIL_FROM:
			if (_code != 250)
				throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused sender: {}", reply_text));

			this->m_index_recipient = 0;
			this->process_send_command(std::format("RCPT TO:<{}>", this->m_vector_recipient.at(0)), eSTATE::SEND_RCPT_TO);
			break;

	// 8) RCPT TO 응답 -> 다음 받는 사람 / DATA
	case	eSTATE::SEND_RCPT_TO:
			if (_code != 250 && _code != 251)
				throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused receiver '{}': {}", this->m_vector_recipient.at(this->m_index_recipient), reply_text));

			if (++this->m_index_recipient < this->m_vector_recipient.size())
				this->process_send_command(std::format("RCPT TO:<{}>", this->m_vector_recipient.at(this->m_index_recipient)), eSTATE::SEND_RCPT_TO);
			else
				this->process_send_command("DATA"sv, eSTATE::REQUEST_DATA);
			break;

	// 9) DATA 응답(354) -> 본문
	case	eSTATE::REQUEST_DATA:
			if (_code != 354)
				throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused data: {}", reply_text));

			this->process_send_data();
			break;

	// 10) 본문 응답(250) -> 성공 -> QUIT
	case	eSTATE::SEND_DATA:
			if (_code != 250)
				throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused message: {}", reply_text));

			this->process_complete(eRESULT::SUCCESS, reply_text);
			this->process_send_command("QUIT"sv, eSTATE::SEND_QUIT);
			break;

	// 11) QUIT 응답 -> 접속 종료
	case	eSTATE::SEND_QUIT:
			SCOPED_LOCK(this->m_cs_email_smtp_result)
			{
				this->m_email_smtp_result.state = eSTATE::DONE;
			}
			this->process_close();
			break;

	default:
			throw throwable::failure(eRESULT::INVALID_STATE, 0, std::format("unexpected reply from email server ({})", _code));
	}
}

void net::email::smtp::socket::process_reply_ehlo(int _code)
{
	// check)
	if (_code != 250)
		throw throwable::failure(eRESULT::FAIL, 0, std::format("email server refused EHLO ({})", _code));

	// 1) capabilities (첫 줄은 서버 이름)
	this->m_capability.clear();
	for (std::size_t i = 1; i < this->m_vector_reply_lines.size(); ++i)
	{
		auto text = _get_reply_text(this->m_vector_reply_lines[i]);

		if (_starts_with_nocase(text, "STARTTLS"sv))
		{
			this->m_capability.starttls = true;
		}
		else if (_starts_with_nocase(text, "AUTH"sv) && text.size() > 4 && (text[4] == ' ' || text[4] == '='))
		{
			// - "AUTH PLAIN LOGIN XOAUTH2" / "AUTH=PLAIN LOGIN"
			std::string mechanisms(text.substr(5));
			std::transform(mechanisms.begin(), mechanisms.end(), mechanisms.begin(), [](unsigned char _ch) { return static_cast<char>(std::toupper(_ch)); });
			mechanisms = " " + mechanisms + " ";

			this->m_capability.auth_plain |= (mechanisms.find(" PLAIN ") != std::string::npos);
			this->m_capability.auth_login |= (mechanisms.find(" LOGIN ") != std::string::npos);
			this->m_capability.auth_xoauth2 |= (mechanisms.find(" XOAUTH2 ") != std::string::npos);
		}
		else if (_starts_with_nocase(text, "SMTPUTF8"sv))
		{
			this->m_capability.smtputf8 = true;
		}
		else if (_starts_with_nocase(text, "SIZE"sv) && text.size() > 5)
		{
			std::size_t value = 0;
			for (auto iter : text.substr(5))
			{
				if (!std::isdigit(static_cast<unsigned char>(iter)))
					break;
				value = value * 10 + (iter - '0');
			}
			this->m_capability.size_max = value;
		}
	}

	// 2) STARTTLS (아직 TLS가 아니면)
	if (this->m_psmtp->m_security == eSECURITY::STARTTLS && !this->m_is_tls)
	{
		// check) STARTTLS를 지원하지 않는 서버에는 평문으로 보내지 않는다.
		if (!this->m_capability.starttls)
			throw throwable::failure(eRESULT::NOT_SUPPORT, 0, "email server does not support STARTTLS"sv);

		this->process_send_command("STARTTLS"sv, eSTATE::SEND_STARTTLS);
		return;
	}

	// 3) 인증 / MAIL FROM
	if (!this->m_email_smtp_result.request.auth.empty())
		this->process_start_auth();
	else
		this->process_send_mail_from();
}

void net::email::smtp::socket::process_start_auth()
{
	// check) 인증 정보는 TLS로만 보낸다.
	if (!this->m_is_tls)
		throw throwable::failure(eRESULT::NOT_ALLOWED, 0, "authentication is not allowed without TLS"sv);

	const auto& auth = this->m_email_smtp_result.request.auth;

	// 1) XOAUTH2 (token이 있으면)
	if (!auth.oauth2_token.empty())
	{
		// check)
		if (!this->m_capability.auth_xoauth2)
			throw throwable::failure(eRESULT::NOT_SUPPORT, 0, "email server does not support AUTH XOAUTH2"sv);

		this->m_email_smtp_result.auth_type = eAUTH_TYPE::XOAUTH2;

		std::string value = "user=" + auth.name + "\x01" "auth=Bearer " + auth.oauth2_token + "\x01\x01";
		this->process_send_command("AUTH XOAUTH2 " + _encode_base64(value), eSTATE::SEND_AUTH, true);
		return;
	}

	// 2) PLAIN ("\0name\0password")
	if (this->m_capability.auth_plain)
	{
		this->m_email_smtp_result.auth_type = eAUTH_TYPE::PLAIN;

		std::string value;
		value += '\0';
		value += auth.name;
		value += '\0';
		value += auth.password;
		this->process_send_command("AUTH PLAIN " + _encode_base64(value), eSTATE::SEND_AUTH, true);
		return;
	}

	// 3) LOGIN (이름과 비밀번호를 차례로 보낸다.)
	if (this->m_capability.auth_login)
	{
		this->m_email_smtp_result.auth_type = eAUTH_TYPE::LOGIN;

		this->process_send_command("AUTH LOGIN"sv, eSTATE::SEND_NAME);
		return;
	}

	// raise)
	throw throwable::failure(eRESULT::NOT_SUPPORT, 0, "email server does not support AUTH PLAIN/LOGIN"sv);
}

void net::email::smtp::socket::process_send_mail_from()
{
	this->process_send_command(std::format("MAIL FROM:<{}>", this->m_email_smtp_result.request.from), eSTATE::SEND_MAIL_FROM);
}

void net::email::smtp::socket::process_send_data()
{
	// declare)
	const auto& request = this->m_email_smtp_result.request;
	std::string_view text(reinterpret_cast<const char*>(request.text.data()), request.text.size());

	// 1) header
	std::string message;
	message.reserve(1024 + text.size() * 4 / 3 + text.size() / 38);

	message += "Date: " + _make_date() + "\r\n";

	if (request.from_name.empty())
		message += "From: " + request.from + "\r\n";
	else
		message += "From: " + _encode_header_word(request.from_name) + " <" + request.from + ">\r\n";

	message += "To: " + request.to + "\r\n";

	if (!request.cc_list.empty())
	{
		message += "Cc: ";
		for (std::size_t i = 0; i < request.cc_list.size(); ++i)
		{
			if (i != 0)
				message += ",\r\n ";
			message += request.cc_list[i];
		}
		message += "\r\n";
	}

	message += "Subject: " + _encode_header_word(request.subject) + "\r\n";
	message += "Message-ID: " + _make_message_id(this->m_psmtp->m_email_server_domain) + "\r\n";
	message += "MIME-Version: 1.0\r\n";
	message += "Content-Type: " + request.content_type + "; charset=UTF-8\r\n";
	message += "Content-Transfer-Encoding: base64\r\n";
	message += "\r\n";

	// 2) body (base64, 76자마다 줄바꿈. '.'으로 시작하는 줄이 생기지 않는다.)
	auto body = _encode_base64(text);
	for (std::size_t pos = 0; pos < body.size(); pos += 76)
	{
		message.append(body, pos, 76);
		message += "\r\n";
	}

	// check) 서버의 최대 크기
	if (this->m_capability.size_max != 0 && message.size() > this->m_capability.size_max)
		throw throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH, 0, std::format("email is too large ({} bytes, max {} bytes)", message.size(), this->m_capability.size_max));

	// 3) 끝 (".")
	message += ".";

	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		this->m_email_smtp_result.send_time = chrono::time::utc::now();
	}

	// 4) send
	this->process_send_command(message, eSTATE::SEND_DATA, true);
}

void net::email::smtp::socket::process_send_command(std::string_view _command, eSTATE _state_next, bool _is_secret)
{
	// 1) 상태 (응답이 송신 완료보다 먼저 올 수 있으므로 보내기 전에 바꾼다.)
	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		this->m_email_smtp_result.state = _state_next;

		// - 보낸 명령을 남긴다. (인증 정보, 본문은 남기지 않는다.)
		if (!_is_secret && this->m_email_smtp_result.vector_result.size() < MAX_REPLY_LINES)
			this->m_email_smtp_result.vector_result.push_back(std::format("> {}", _command));
	}

	// 2) send (명령 + CRLF)
	auto buf_send = alloc_shared_buffer(_command.size() + 2);
	buf_send.append_text<char>(_command);
	buf_send.append_text<char>("\r\n"sv);

	net::sockaddr temp_sockaddr;
	if (!this->process_sendable(buf_send, buf_send.move_out_source(), 1, temp_sockaddr))
		throw throwable::failure(eRESULT::NOT_READY, 0, "fail to send command to email server"sv);

	// 3) 응답 대기 시간
	this->process_start_timer();
}

void net::email::smtp::socket::process_start_tls()
{
	// 1) TLS 객체를 만든다. (client용 공유 context: system CA 저장소로 서버 인증서를 확인한다.)
	auto pssl_context = crypt::ssl_context::get_default_client();
	auto pcrypt = pssl_context->alloc_ssl(static_cast<int>(this->native_handle()));

	// 2) 서버 이름 (SNI + 인증서 이름 확인)
	pcrypt->set_host_name(this->m_psmtp->m_host_name);

	// 3) 이 접속에 설정하고 handshake를 시작한다. (ClientHello. 이후 수신은 복호화해서 처리된다.)
	pcrypt->initialize_handshake_client(this);
	this->ssl_start_handshake();
	this->start_ssl_handshake_timer(this->m_psmtp->m_timeout);

	// 4) TLS 이후에는 EHLO를 다시 보내고 capability를 다시 받는다.
	this->m_is_tls = true;
	this->m_capability.clear();
}

void net::email::smtp::socket::process_start_timer()
{
	// declare)
	auto serial = ++this->m_step_serial;
	auto psmtp = this->m_psmtp;

	// check)
	if (psmtp.empty())
		return;

	// 1) 시간이 지나도 같은 단계면 실패로 처리한다.
	auto pexecutable = make_executable([psocket = object_ptr<socket>(this), serial]()
	{
		// check) 다음 단계로 넘어갔으면 할 것이 없다.
		if (psocket->m_step_serial.load() != serial)
			return;

		// - QUIT 응답을 기다리는 중이면 그냥 끊는다.
		bool is_quit = false;
		{
			std::lock_guard cs(psocket->m_cs_email_smtp_result);
			is_quit = psocket->m_is_completed;
		}

		if (is_quit)
			psocket->process_close();
		else
			psocket->process_fail(eRESULT::TIME_OUT, "no reply from email server"sv);
	});

	// 2) post
	system_executor::post(std::move(pexecutable), chrono::tick::now() + psmtp->m_timeout);
}

void net::email::smtp::socket::process_complete(eRESULT _result, std::string_view _message) noexcept
{
	// declare)
	sRESULT result;
	object_ptr<smtp> psmtp;

	try
	{
		SCOPED_LOCK(this->m_cs_email_smtp_result)
		{
			// check) 한 번만 알린다.
			if (this->m_is_completed || this->m_email_smtp_result.request.to.empty())
				return;

			this->m_is_completed = true;
			this->m_email_smtp_result.result = _result;
			this->m_email_smtp_result.message = _message;

			result = this->m_email_smtp_result;
			psmtp = this->m_psmtp;
		}

		// - 인증 정보는 결과에 넣지 않는다.
		result.request.auth.clear();

		// log)
		if (_result != eRESULT::SUCCESS)
			LOG_INFO << "(info) CGDK.net.email: fail to send email to '"sv << result.request.to << "' ("sv << static_cast<uint32_t>(_result) << ", "sv << result.reply_code << ") "sv << _message;

		// 1) 결과를 알린다.
		if (psmtp.exist())
			psmtp->process_complete_send(result);
	}
	catch (...)
	{
	}
}

void net::email::smtp::socket::process_fail(eRESULT _result, std::string_view _message) noexcept
{
	// 1) 실패로 알린다. (이미 끝났으면 무시된다.)
	this->process_complete(_result, _message);

	// 2) 접속을 끊는다.
	this->process_close();
}

void net::email::smtp::socket::process_close() noexcept
{
	// 1) 다음 timer가 동작하지 않게 한다.
	++this->m_step_serial;

	// 2) 접속을 끊는다. (on_disconnect / on_fail_connect에서 smtp에 알린다.)
	if (this->socket_state() != eSOCKET_STATE::CLOSED)
		this->closesocket();
	else
		this->process_notify_closed();
}

void net::email::smtp::socket::process_notify_closed() noexcept
{
	// declare)
	object_ptr<smtp> psmtp;

	SCOPED_LOCK(this->m_cs_email_smtp_result)
	{
		// check) 한 번만 알린다.
		if (this->m_is_closed_notified)
			return;

		this->m_is_closed_notified = true;
		psmtp = std::move(this->m_psmtp);
	}

	// 1) smtp 목록에서 뺀다. (queue에 있는 다음 메일을 보낸다.)
	if (psmtp.exist())
		psmtp->process_socket_closed(this);
}
