#include "pch.h"

using namespace CGDK;


net::email::smtp::smtp()
{
}

net::email::smtp::~smtp() noexcept
{
}

bool net::email::smtp::is_valid_address(std::string_view _address) noexcept
{
	// check) 길이 (RFC 5321: 254)
	if (_address.empty() || _address.size() > 254)
		return false;

	// check) '@'가 하나이고 앞뒤가 비어 있지 않아야 한다.
	auto pos_at = _address.find('@');
	if (pos_at == std::string_view::npos || pos_at == 0 || pos_at + 1 >= _address.size() || _address.find('@', pos_at + 1) != std::string_view::npos)
		return false;

	// check) 공백, 제어문자, 명령/헤더를 깨뜨릴 수 있는 문자
	for (unsigned char iter : _address)
	{
		if (iter <= 0x20 || iter == 0x7f)
			return false;

		switch (iter)
		{
		case '<': case '>': case ',': case ';': case '"': case '(': case ')': case '[': case ']': case '\\':
			return false;
		default:
			break;
		}
	}

	// return)
	return true;
}

size_t net::email::smtp::get_queued_count() const noexcept
{
	std::lock_guard cs(this->m_cs_email_request);

	return this->m_list_send_email.size();
}

void net::email::smtp::send(const email::sREQUEST& _email_info)
{
	this->send(email::sREQUEST(_email_info));
}

void net::email::smtp::send(email::sREQUEST&& _email_send_request)
{
	// 1) 기본값 (from, auth)
	this->process_fill_default(_email_send_request);

	// check) 받는 사람 / 보내는 사람
	if (_email_send_request.to.empty())
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) 'email receiver' is empty"sv);

	if (!is_valid_address(_email_send_request.to))
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) invalid email receiver '{}'", _email_send_request.to));

	if (!is_valid_address(_email_send_request.from))
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) invalid email sender '{}'", _email_send_request.from));

	for (const auto* plist : { &_email_send_request.cc_list, &_email_send_request.bcc_list })
	{
		for (const auto& iter : *plist)
		{
			if (!is_valid_address(iter))
				throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) invalid email receiver '{}'", iter));
		}
	}

	// check) content type
	if (_email_send_request.content_type.empty())
		_email_send_request.content_type = "text/plain";

	if (_email_send_request.content_type != "text/plain" && _email_send_request.content_type != "text/html")
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) not supported content type '{}' (text/plain, text/html)", _email_send_request.content_type));

	// check) 암호화하지 않는 접속으로는 인증 정보를 보내지 않는다.
	if (this->m_security == eSECURITY::NONE && !_email_send_request.auth.empty())
		throw throwable::failure(eRESULT::NOT_ALLOWED, 0, "(excp) authentication is not allowed without TLS (security: none)"sv);

	// 2) 접속 수가 남아 있으면 바로 보내고 아니면 queue에 넣는다.
	SCOPED_LOCK(this->m_cs_email_request)
	{
		// check)
		if (!this->m_is_started)
			throw throwable::failure(eRESULT::NOT_READY, 0, "(excp) email SMTP sender is not started"sv);

		if (this->m_list_socket_exist.size() >= this->m_max_concurrent_socket)
		{
			this->m_list_send_email.push_back(std::move(_email_send_request));
			return;
		}
	}

	// 3) 보낸다.
	this->process_start_socket(std::move(_email_send_request));
}

void net::email::smtp::process_fill_default(sREQUEST& _email_send_request) const
{
	if (_email_send_request.from.empty())
	{
		_email_send_request.from = this->m_default_from;

		if (_email_send_request.from_name.empty())
			_email_send_request.from_name = this->m_default_from_name;
	}

	if (_email_send_request.auth.empty())
		_email_send_request.auth = this->m_default_auth;
}

void net::email::smtp::process_start_socket(sREQUEST&& _email_send_request)
{
	// 1) on_email_sending
	try
	{
		this->on_email_sending(_email_send_request);
	}
	catch (...)
	{
	}

	// 2) socket을 만들어 목록에 넣는다.
	auto psocket = make_object<socket>();

	SCOPED_LOCK(this->m_cs_email_request)
	{
		this->m_list_socket_exist.push_back(psocket);
	}

	// 3) 접속해서 보낸다. (결과는 socket이 process_complete_send로 알린다.)
	psocket->start_send(object_ptr<smtp>(this), std::move(_email_send_request));
}

result_code net::email::smtp::_process_start(context& _context)
{
	const size_t DEFAULT_MAX_CONCURRENT_SOCKET = 8;

	// trace)
	LOG_PROGRESS << "@ <BEGIN> start 'email SMTP sender'['"sv << name() << "']"sv;

	// 1) get address
	auto temp_connect_param = CGDK::get_parameter_connection(_context, true);

	// check)
	if (temp_connect_param.remote_endpoint.data().is_bind_any())
	{
		// log)
		LOG_ERROR << "  (excp) email 'remote_endpoint' is empty."sv;

		// raise)
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) email 'remote_endpoint' is empty."sv);
	}

	if (temp_connect_param.bind_endpoint.port() != 0)
	{
		// - set as 0
		temp_connect_param.bind_endpoint.port(0);

		// log)
		LOG_INFO << "  ! bind_endpoint.port is ignored.";
	}

	// 2) security ("ssl" / "starttls" / "none". 기본: 465 -> ssl, 그 외 -> starttls)
	eSECURITY security = (temp_connect_param.remote_endpoint.port() == 465) ? eSECURITY::SSL : eSECURITY::STARTTLS;
	if (auto& context_find = _context.find("security"sv); context_find.is_valid())
	{
		std::string value = context_find;

		if (value == "ssl"sv || value == "tls"sv)
			security = eSECURITY::SSL;
		else if (value == "starttls"sv)
			security = eSECURITY::STARTTLS;
		else if (value == "none"sv)
			security = eSECURITY::NONE;
		else
			throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) invalid email 'security' '{}' (ssl, starttls, none)", value));
	}

	// 3) port (0이면 보안 방식의 기본 port)
	if (temp_connect_param.remote_endpoint.port() == 0)
	{
		temp_connect_param.remote_endpoint.port((security == eSECURITY::SSL) ? 465 : (security == eSECURITY::STARTTLS) ? 587 : 25);

		// log)
		LOG_INFO << "  - it set port of remote_endpoint as default value (="sv << temp_connect_param.remote_endpoint.port() << ")"sv;
	}

	// 4) host name (인증서 확인 + SNI. 기본: remote_endpoint.address)
	std::string host_name;
	if (auto& context_find = _context.find("host_name"sv); context_find.is_valid())
	{
		host_name = (std::string&&)context_find;
	}
	else if (auto& context_endpoint = _context.find("remote_endpoint"sv); context_endpoint.is_valid())
	{
		if (auto& context_address = context_endpoint.find("address"sv); context_address.is_valid())
			host_name = (std::string&&)context_address;
	}

	// check) TLS를 쓰면 서버 이름이 있어야 인증서를 확인할 수 있다.
	if (security != eSECURITY::NONE && host_name.empty())
		LOG_WARNING << "  (warn) email 'host_name' is empty. the server certificate name is not verified"sv;

	// 5) get 'max_concurrent_socket'
	size_t max_concurrent_socket = DEFAULT_MAX_CONCURRENT_SOCKET;
	if (auto& context_find = _context.find("max_concurrent_socket", "max concurrent socket"); context_find.is_valid())
	{
		max_concurrent_socket = context_find;

		if (max_concurrent_socket == 0)
			max_concurrent_socket = DEFAULT_MAX_CONCURRENT_SOCKET;
		else if (max_concurrent_socket > 8192)
			max_concurrent_socket = 8192;
	}

	// 6) email domain (EHLO)
	std::string email_server_domain;
	if (auto& context_find = _context.find("email_server_domain", "email server domain", "email_domain", "email domain"); context_find.is_valid())
	{
		email_server_domain = (std::string&&)context_find;
	}

	// check) EHLO 이름은 공백/제어문자가 없어야 한다.
	if (email_server_domain.empty() || std::any_of(email_server_domain.begin(), email_server_domain.end(), [](unsigned char _ch) { return _ch <= 0x20 || _ch == 0x7f; }))
	{
		// log)
		LOG_ERROR << "  (excp) 'email_server_domain' is empty or invalid."sv;

		// raise)
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, "(excp) 'email_server_domain' is empty or invalid."sv);
	}

	// 7) 기본 보내는 사람 / 인증 정보
	std::string default_from;
	std::string default_from_name;
	sAUTH default_auth;

	if (auto& context_find = _context.find("from"sv); context_find.is_valid())
		default_from = (std::string&&)context_find;

	if (auto& context_find = _context.find("from_name"sv); context_find.is_valid())
		default_from_name = (std::string&&)context_find;

	if (auto& context_auth = _context.find("auth"sv); context_auth.is_valid())
	{
		if (auto& context_find = context_auth.find("name"sv); context_find.is_valid())
			default_auth.name = (std::string&&)context_find;
		if (auto& context_find = context_auth.find("password"sv); context_find.is_valid())
			default_auth.password = (std::string&&)context_find;
		if (auto& context_find = context_auth.find("oauth2_token"sv); context_find.is_valid())
			default_auth.oauth2_token = (std::string&&)context_find;
	}

	// check) 암호화하지 않으면 인증 정보를 보내지 않는다.
	if (security == eSECURITY::NONE && !default_auth.empty())
		throw throwable::failure(eRESULT::NOT_ALLOWED, 0, "(excp) email 'auth' is not allowed without TLS (security: none)"sv);

	if (!default_from.empty() && !is_valid_address(default_from))
		throw throwable::failure(eRESULT::INVALID_VALUE, 0, std::format("(excp) invalid email 'from' '{}'", default_from));

	// 8) timeout (초)
	chrono::tick::duration timeout = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::seconds(30));
	if (auto& context_find = _context.find("timeout"sv); context_find.is_valid())
	{
		int64_t seconds = context_find;

		if (seconds > 0)
			timeout = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::seconds(seconds));
	}

	// log)
	LOG_INFO << "  + email server: "sv << to_string(temp_connect_param.remote_endpoint) << " (security: "sv
			 << ((security == eSECURITY::SSL) ? "ssl"sv : (security == eSECURITY::STARTTLS) ? "starttls"sv : "none"sv)
			 << ", host_name: '"sv << host_name << "', ehlo: '"sv << email_server_domain << "', auth: "sv << (default_auth.empty() ? "no"sv : "yes"sv) << ")"sv;

	// 9) set...
	SCOPED_LOCK(this->m_cs_email_request)
	{
		this->m_connect_parameter = std::move(temp_connect_param);
		this->m_security = security;
		this->m_host_name = std::move(host_name);
		this->m_email_server_domain = std::move(email_server_domain);
		this->m_default_from = std::move(default_from);
		this->m_default_from_name = std::move(default_from_name);
		this->m_default_auth = std::move(default_auth);
		this->m_timeout = timeout;
		this->m_max_concurrent_socket = max_concurrent_socket;
		this->m_is_started = true;
	}

	// 10) call 'on_start'
	this->on_start(_context);

	// trace)
	LOG_PROGRESS << "@ <END> start 'email SMTP sender'['"sv << name() << "']"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code net::email::smtp::_process_stopping() noexcept
{
	// trace)
	LOG_PROGRESS << "@ <BEGIN> stop 'email SMTP sender'['"sv << name() << "']"sv;

	// 1) call 'on_stopping'
	try
	{
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// declare)
	std::list<sREQUEST> list_send_email;
	std::list<object_ptr<socket>> list_socket_exist;

	// 2) 더 받지 않는다. (queue의 메일은 취소한다.)
	SCOPED_LOCK(this->m_cs_email_request)
	{
		this->m_is_started = false;
		list_send_email = std::move(this->m_list_send_email);
		this->m_list_send_email.clear();
		list_socket_exist = this->m_list_socket_exist;
	}

	// 3) 보내지 않은 메일 (CANCEL)
	for (auto& iter : list_send_email)
	{
		sRESULT result;
		result.request = std::move(iter);
		result.result = eRESULT::CANCEL;
		result.message = "email SMTP sender is stopped";
		this->process_complete_send(result);
	}

	// 4) 보내는 중인 접속을 끊는다. (각 접속이 CANCEL로 결과를 알린다.)
	for (auto& iter : list_socket_exist)
	{
		iter->cancel_sending();
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code net::email::smtp::_process_stop() noexcept
{
	// 1) call 'on_stop'
	try
	{
		this->on_stop();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// 2) clear socket
	SCOPED_LOCK(this->m_cs_email_request)
	{
		this->m_list_socket_exist.clear();
	}

	// trace)
	LOG_PROGRESS << "@ <END> stop 'email SMTP sender'['"sv << name() << "']"sv;

	// return)
	return eRESULT::SUCCESS;
}

void net::email::smtp::process_complete_send(const sRESULT& _email_send_result) noexcept
{
	// - 'on_email_sent' (complete)
	try
	{
		this->on_email_sent(_email_send_result);
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "(err) CGDK.net.email: exception in email complete callback"sv;
	}
}

void net::email::smtp::process_socket_closed(socket* _psocket) noexcept
{
	// declare)
	sREQUEST email_send_request;
	bool is_next = false;

	// 1) 목록에서 빼고 queue에 메일이 있으면 꺼낸다.
	SCOPED_LOCK(this->m_cs_email_request)
	{
		this->m_list_socket_exist.remove_if([=](const object_ptr<socket>& _iter) { return _iter.get() == _psocket; });

		if (this->m_is_started && !this->m_list_send_email.empty() && this->m_list_socket_exist.size() < this->m_max_concurrent_socket)
		{
			email_send_request = std::move(this->m_list_send_email.front());
			this->m_list_send_email.pop_front();
			is_next = true;
		}
	}

	// 2) 다음 메일을 보낸다.
	if (is_next)
	{
		try
		{
			this->process_start_socket(std::move(email_send_request));
		}
		catch (...)
		{
			// log)
			LOG_ERROR << "(err) CGDK.net.email: fail to start sending queued email"sv;
		}
	}
}
