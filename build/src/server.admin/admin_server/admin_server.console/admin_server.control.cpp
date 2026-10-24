#include "pch.h"

//-----------------------------------------------------------------------------
// admin::socket_control
//-----------------------------------------------------------------------------
std::string_view admin::socket_control::to_method_string(eHTTP_METHOD _method) noexcept
{
	switch (_method)
	{
	case	eHTTP_METHOD::GET_:		return "GET"sv;
	case	eHTTP_METHOD::POST_:	return "POST"sv;
	case	eHTTP_METHOD::PUT_:		return "PUT"sv;
	default:						return "OTHER"sv;
	}
}

admin::socket_control::socket_control()
{
	// - 크기 제한 (header 16KB, body 64KB)
	this->set_http_limits(16 * 1024, 64 * 1024);

	// - 시간 제한 ("acceptor_control": "request_timeout", "idle_timeout")
	this->set_sustain_timeout(service::limits_control.to_sustain_timeout(), &service::limits_control.count_timeout);
}

void admin::socket_control::on_connect()
{
	// declare)
	auto ppolicy = service::access_policy_control.get();
	auto address = this->remote_endpoint().address();

	// 1) allowed address (certify_key도 allowed_addresses도 없으면 loopback만 허용한다.)
	bool is_allowed = ppolicy->is_address_allowed_or_loopback(address);

	// check)
	if (is_allowed == false)
	{
		LOG_WARNING << "(warn) control connection rejected (not allowed address: "sv << address.to_string() << ")"sv;
		this->m_is_rejected = true;
		this->disconnect();
		return;
	}

	this->m_is_rejected = false;
}

bool admin::socket_control::process_check_key(const sMESSAGE_HTTP_REQUEST& _msg) const
{
	// declare)
	auto ppolicy = service::access_policy_control.get();

	// check) no key
	if (ppolicy->certify_key.empty())
		return true;

	// 1) 서명 ('X-CGDK-Timestamp', 'X-CGDK-Signature'. admin_server.policy.h 참고)
	if (auto signature = _msg.get_header("X-CGDK-Signature"sv); signature.empty() == false)
	{
		std::string body;
		for (const auto& iter : _msg.contents)
			body.append(reinterpret_cast<const char*>(iter.data()), iter.size());

		std::string_view path(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size());
		auto now_unix = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		auto result = service::control_signature.verify(ppolicy->certify_key, to_method_string(_msg.method), path, _msg.get_header("X-CGDK-Timestamp"sv), body, signature, now_unix);

		if (result != admin::signature_verifier::eRESULT_VERIFY::OK)
			LOG_WARNING << "(warn) control request signature rejected ("sv << admin::to_verify_string(result) << ")"sv;

		return result == admin::signature_verifier::eRESULT_VERIFY::OK;
	}

	// check) 서명만 받는다. ("require_signature": true)
	//        GET /metrics는 key도 받는다. (prometheus는 서명할 수 없다. "metrics_require_signature": true면 서명만)
	if (service::control_require_signature)
	{
		std::string_view path(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size());
		const bool is_metrics = (_msg.method == eHTTP_METHOD::GET_ && path.substr(0, path.find('?')) == "/metrics"sv);
		if (is_metrics == false || service::control_metrics_require_signature)
			return false;
	}

	// 2) 'X-CGDK-Key: <key>'
	if (auto key = _msg.get_header("X-CGDK-Key"sv); key.empty() == false)
		return ppolicy->is_key_matched(key);

	// 3) 'Authorization: Bearer <key>'
	if (auto authorization = _msg.get_header("Authorization"sv); authorization.substr(0, 7) == "Bearer "sv)
		return ppolicy->is_key_matched(authorization.substr(7));

	// return)
	return false;
}

void admin::socket_control::process_send_response(const sMESSAGE_HTTP_REQUEST& _msg, eHTTP_STATUS_CODE _status_code, std::string_view _content_type, const std::string& _body, std::string_view _retry_after)
{
	// 1) body
	auto buf_body = alloc_shared_buffer(_body.size());
	if (_body.empty() == false)
		memcpy(buf_body.data(), _body.data(), _body.size());
	buf_body.set_size(_body.size());

	// 2) response
	sMESSAGE_HTTP_RESPONSE msg_http_response;
	msg_http_response.version = _msg.version;
	msg_http_response.status_code = _status_code;
	msg_http_response.status_message = to_string(_status_code);
	msg_http_response.content_type = _content_type;
	if (_retry_after.empty() == false)
		msg_http_response.headers.push_back({ "Retry-After"sv, _retry_after });
	msg_http_response.contents.push_back(std::move(buf_body));

	// 3) send
	this->send(make_shared_buffer(msg_http_response));
}

result_code admin::socket_control::on_message(sMESSAGE_HTTP_REQUEST& _msg)
{
	// check) rejected connection
	if (this->m_is_rejected)
		return eRESULT::DONE;

	// declare)
	constexpr std::string_view CONTENT_TYPE_JSON = "application/json; charset=utf-8"sv;

	try
	{
		// declare)
		auto str_address = this->remote_endpoint().address().to_string();

		// check) key를 여러 번 틀린 주소는 잠시 막는다. (admin::auth_fail_limiter)
		if (auto time_blocked = service::control_auth_limiter.get_blocked(str_address); time_blocked.count() > 0)
		{
			service::audit.write("control"sv, str_address, std::string(to_method_string(_msg.method)) + " " + std::string(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size()), 429, "blocked"sv);
			auto str_retry_after = std::to_string(time_blocked.count());
			this->process_send_response(_msg, eHTTP_STATUS_CODE::TOO_MANY_REQUESTS_, CONTENT_TYPE_JSON, R"({"error":"too many auth failures"})", str_retry_after);
			return eRESULT::DONE;
		}

		// check) key
		if (this->process_check_key(_msg) == false)
		{
			auto is_blocked = service::control_auth_limiter.on_fail(str_address);
			LOG_WARNING << "(warn) control request rejected (certify key mismatch) [ip "sv << str_address << "]"sv << (is_blocked ? " (blocked: too many failures)"sv : ""sv);
			service::audit.write("control"sv, str_address, std::string(to_method_string(_msg.method)) + " " + std::string(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size()), 401, is_blocked ? "unauthorized (blocked: too many failures)"sv : "unauthorized"sv);
			this->process_send_response(_msg, eHTTP_STATUS_CODE::UNAUTHORIZED_, CONTENT_TYPE_JSON, R"({"error":"unauthorized"})");
			return eRESULT::DONE;
		}
		service::control_auth_limiter.on_success(str_address);

		// 1) method / path
		auto method = to_method_string(_msg.method);
		std::string_view path(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size());

		// 2) route
		auto route = parse_control_route(method, path);

		// 3) execute
		//    (종료 중에는 g_pservice를 읽지 않는다 : main의 reset과 경합한다.)
		auto pservice = (g_is_service_closing == false) ? g_pservice.get_object_ptr() : object_ptr<service>();
		if (pservice.empty())
		{
			this->process_send_response(_msg, eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_, CONTENT_TYPE_JSON, R"({"error":"service is not ready"})");
			return eRESULT::DONE;
		}

		auto result = pservice->process_control(route);

		// 4) response
		this->process_send_response(_msg, result.status_code, (route.action == eCONTROL_ACTION::METRICS) ? "text/plain; version=0.0.4; charset=utf-8"sv : CONTENT_TYPE_JSON, result.body);

		// log) audit (상태를 바꾸는 요청은 모두 남긴다. 조회는 실패만 남긴다.)
		if (method != "GET"sv)
		{
			LOG_INFO << "(audit) control ["sv << str_address << "] "sv << method << " "sv << path << " -> "sv << static_cast<int>(result.status_code) << " "sv << result.body;
			service::audit.write("control"sv, str_address, std::string(method) + " " + std::string(path), static_cast<int>(result.status_code), result.body);
		}
		else if (result.status_code != eHTTP_STATUS_CODE::OK_)
			LOG_INFO << "(audit) control ["sv << str_address << "] "sv << method << " "sv << path << " -> "sv << static_cast<int>(result.status_code);
	}
	catch (...)
	{
		this->process_send_response(_msg, eHTTP_STATUS_CODE::INTERNAL_SERVER_ERROR_, CONTENT_TYPE_JSON, R"({"error":"internal error"})");
	}

	// return)
	return eRESULT::DONE;
}

