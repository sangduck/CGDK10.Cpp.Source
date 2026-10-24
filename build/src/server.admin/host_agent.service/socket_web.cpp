#include "pch.h"
#include "socket_web.h"

extern std::string_view g_string_webpage_frame_htm;
extern std::string_view g_string_webpage_host_info_htm;
extern std::string_view g_string_webpage_host_log_htm;
extern std::string_view g_string_webpage_server_info_htm;
extern std::string_view g_string_webpage_server_overview_info_htm;
extern std::string_view g_string_webpage_server_factory_info_htm;
extern std::string_view g_string_webpage_server_executor_info_htm;
extern std::string_view g_string_webpage_server_network_info_htm;
extern std::string_view g_string_webpage_server_querier_info_htm;
extern std::string_view g_string_webpage_server_crash_info_htm;
extern std::string_view g_string_webpage_server_setting_htm;
extern std::string_view g_string_webpage_server_statistics_htm;
extern std::string_view g_string_webpage_time_event_info_htm;
extern std::string_view g_string_webpage_login_htm;

namespace
{
	//-------------------------------------------------------------------------
	// web pages (path -> page)
	//  - 처음 쓸 때 한 번만 만든다. (function local static은 thread-safe하게 초기화된다.)
	//-------------------------------------------------------------------------
	const std::map<std::string_view, shared_buffer>& _get_webpages()
	{
		static const std::map<std::string_view, shared_buffer> map_webpages = []
		{
			std::map<std::string_view, shared_buffer> map_pages;
			auto f_add = [&](std::string_view _path, std::string_view _page) { map_pages.emplace(_path, make_shared_buffer(text(_page))); };

			f_add("/"sv,									g_string_webpage_frame_htm);
			f_add("/webpage_host_info.htm"sv,				g_string_webpage_host_info_htm);
			f_add("/webpage_host_log.htm"sv,				g_string_webpage_host_log_htm);
			f_add("/webpage_server_info.htm"sv,				g_string_webpage_server_info_htm);
			f_add("/webpage_server_overview_info.htm"sv,	g_string_webpage_server_overview_info_htm);
			f_add("/webpage_server_factory_info.htm"sv,		g_string_webpage_server_factory_info_htm);
			f_add("/webpage_server_executor_info.htm"sv,	g_string_webpage_server_executor_info_htm);
			f_add("/webpage_server_network_info.htm"sv,		g_string_webpage_server_network_info_htm);
			f_add("/webpage_server_querier_info.htm"sv,		g_string_webpage_server_querier_info_htm);
			f_add("/webpage_server_crash_info.htm"sv,		g_string_webpage_server_crash_info_htm);
			f_add("/webpage_server_setting.htm"sv,			g_string_webpage_server_setting_htm);
			f_add("/webpage_server_statistics.htm"sv,		g_string_webpage_server_statistics_htm);
			f_add("/webpage_time_event_info.htm"sv,			g_string_webpage_time_event_info_htm);

			return map_pages;
		}();

		return map_webpages;
	}

	const shared_buffer& _get_webpage_login()
	{
		static const shared_buffer buf_login = make_shared_buffer(text(g_string_webpage_login_htm));
		return buf_login;
	}
}

socket_web::socket_web()
{
	// - 크기 제한 (header 16KB, body 4KB. login form만 body가 있다.)
	this->set_http_limits(16 * 1024, 4 * 1024);

	// - 시간 제한 ("acceptor_web": "request_timeout", "idle_timeout")
	this->set_sustain_timeout(::service::limits_web.to_sustain_timeout(), &::service::limits_web.count_timeout);

	// - 미리 만든다.
	_get_webpages();
}

socket_web::~socket_web()
{
}

void socket_web::on_connect()
{
	// check) 허용된 주소가 아니면 끊는다.
	auto address = this->remote_endpoint().address();
	this->m_is_rejected = (::service::is_web_address_allowed(::service::access_policy_web, address) == false);

	if (this->m_is_rejected)
	{
		LOG_WARNING << "(warn) web connection rejected (not allowed address: "sv << address.to_string() << ")"sv;
		this->disconnect();
	}
}

void socket_web::on_disconnect(uint64_t /*_disconnect_reason*/)
{
}

void socket_web::process_send_response(const sMESSAGE_HTTP_REQUEST& _msg, eHTTP_STATUS_CODE _status_code, const shared_buffer& _buf_body)
{
	sMESSAGE_HTTP_RESPONSE msg_http_response;
	msg_http_response.version = _msg.version;
	msg_http_response.status_code = _status_code;
	msg_http_response.status_message = to_string(_status_code);
	msg_http_response.content_type = (_status_code == eHTTP_STATUS_CODE::OK_) ? "text/html; charset=utf-8"sv : "text/plain; charset=utf-8"sv;
	msg_http_response.contents.push_back(_buf_body);

	this->send(make_shared_buffer(msg_http_response));
}

void socket_web::process_send_redirect(const sMESSAGE_HTTP_REQUEST& _msg, std::string_view _location, std::string_view _set_cookie)
{
	sMESSAGE_HTTP_RESPONSE msg_http_response;
	msg_http_response.version = _msg.version;
	msg_http_response.status_code = eHTTP_STATUS_CODE::SEE_OTHER_;
	msg_http_response.status_message = to_string(eHTTP_STATUS_CODE::SEE_OTHER_);
	msg_http_response.headers.push_back({ "Location"sv, _location });
	if (_set_cookie.empty() == false)
		msg_http_response.headers.push_back({ "Set-Cookie"sv, _set_cookie });
	msg_http_response.headers.push_back({ "Cache-Control"sv, "no-store"sv });
	msg_http_response.content_type = "text/plain; charset=utf-8"sv;
	msg_http_response.contents.push_back(make_shared_buffer(text("see other"sv)));

	this->send(make_shared_buffer(msg_http_response));
}

bool socket_web::process_login(const sMESSAGE_HTTP_REQUEST& _msg, std::string_view _path)
{
	//-----------------------------------------------------------------
	// web ui login ("acceptor_web"."login_key")
	//  - POST /login (form 'key') : 맞으면 session cookie를 주고 '/'로 보낸다. 틀리면 '/?error=1'
	//  - GET  /logout             : session을 지운다.
	//  - session이 없으면 '/'는 login page, 다른 것은 401
	//  return) true면 여기서 응답했다.
	//-----------------------------------------------------------------
	// check) login이 필요 없다. ('/logout'은 첫 page로 보낸다.)
	auto ppolicy = ::service::get_web_origin_policy();
	if (ppolicy->is_login_required() == false)
	{
		if (_path == "/logout"sv)
		{
			this->process_send_redirect(_msg, "/"sv);
			return true;
		}
		return false;
	}

	auto str_address = this->remote_endpoint().address().to_string();

	// 1) login
	if (_path == "/login"sv && _msg.method == eHTTP_METHOD::POST_)
	{
		// check) 여러 번 틀린 주소
		if (::service::web_sessions.is_blocked(str_address))
		{
			::service::audit.write("web"sv, str_address, "login"sv, 429, "blocked"sv);
			this->process_send_response(_msg, eHTTP_STATUS_CODE::TOO_MANY_REQUESTS_, make_shared_buffer(text("too many login failures"sv)));
			return true;
		}

		// - body
		std::string body;
		for (const auto& iter : _msg.contents)
			body.append(reinterpret_cast<const char*>(iter.data()), iter.size());

		auto key = server_system::get_form_value(body, "key"sv);
		if (key.has_value() && server_system::is_certify_key_matched(ppolicy->login_key, *key))
		{
			::service::web_sessions.on_success(str_address);
			auto token = ::service::web_sessions.create(str_address);
			auto str_cookie = std::string(::service::WEB_SESSION_COOKIE) + "=" + token + "; Path=/; HttpOnly; SameSite=Strict";

			LOG_INFO << "(audit) web ui login ["sv << str_address << "]"sv;
			::service::audit.write("web"sv, str_address, "login"sv, 200);
			this->process_send_redirect(_msg, "/"sv, str_cookie);
		}
		else
		{
			auto is_blocked = ::service::web_sessions.on_fail(str_address);
			LOG_WARNING << "(warn) web ui login failed ["sv << str_address << "]"sv << (is_blocked ? " (blocked: too many failures)"sv : ""sv);
			::service::audit.write("web"sv, str_address, "login"sv, 401, is_blocked ? "blocked: too many failures"sv : "wrong key"sv);
			this->process_send_redirect(_msg, "/?error=1"sv);
		}
		return true;
	}

	// 2) logout
	if (_path == "/logout"sv)
	{
		::service::web_sessions.remove(server_system::get_cookie_value(_msg.get_header("Cookie"sv), ::service::WEB_SESSION_COOKIE));
		auto str_cookie = std::string(::service::WEB_SESSION_COOKIE) + "=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0";
		this->process_send_redirect(_msg, "/"sv, str_cookie);
		return true;
	}

	// 3) session
	if (::service::is_web_session_valid(_msg, str_address))
		return false;

	if (_path == "/"sv)
		this->process_send_response(_msg, eHTTP_STATUS_CODE::OK_, _get_webpage_login());
	else
		this->process_send_response(_msg, eHTTP_STATUS_CODE::UNAUTHORIZED_, make_shared_buffer(text("unauthorized"sv)));
	return true;
}

result_code socket_web::on_message(sMESSAGE_HTTP_REQUEST& _msg)
{
	// check) rejected connection
	if (this->m_is_rejected)
		return eRESULT::DONE;

	// declare)
	auto f_text = [](std::string_view _text) { return make_shared_buffer(text(_text)); };

	// check) Host (DNS rebinding : ip/localhost나 allowed_origins의 host만 받는다.)
	if (server_system::is_web_host_allowed(_msg.get_header("Host"sv), ::service::get_web_origin_policy()->allowed_origins) == false)
	{
		LOG_WARNING << "(warn) web request rejected (not allowed host: '"sv << _msg.get_header("Host"sv) << "')"sv;
		this->process_send_response(_msg, eHTTP_STATUS_CODE::FORBIDDEN_, f_text("forbidden"sv));
		return eRESULT::DONE;
	}

	// 1) path (query string은 뺀다.)
	std::string_view path(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size());
	if (auto pos = path.find('?'); pos != std::string_view::npos)
		path = path.substr(0, pos);

	// 2) login (login_key가 있으면 session이 있어야 한다.)
	if (this->process_login(_msg, path))
		return eRESULT::DONE;

	// 3) page
	const auto& map_webpages = _get_webpages();
	auto iter_find = map_webpages.find(path);

	// 4) response (없으면 404. 응답을 보내지 않으면 client가 기다린다.)
	if (iter_find != map_webpages.end())
		this->process_send_response(_msg, eHTTP_STATUS_CODE::OK_, iter_find->second);
	else
		this->process_send_response(_msg, eHTTP_STATUS_CODE::NOT_FOUND_, f_text("not found"sv));

	// return)
	return eRESULT::DONE;
}

result_code socket_web::on_message(sMESSAGE_HTTP_WEBSOCKET& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}

result_code socket_web::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}
