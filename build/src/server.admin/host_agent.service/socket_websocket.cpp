#include "pch.h"
#include "socket_websocket.h"


socket_websocket::socket_websocket()
{
	// - 크기 제한 (header 16KB, websocket frame 64KB)
	this->set_http_limits(16 * 1024, 64 * 1024);

	// - 시간 제한 ("acceptor_websocket": "request_timeout", "idle_timeout", "ping_interval")
	//   (page가 열려 있는 동안 받을 것이 없을 수 있으므로 PING을 보내고 PONG이 없으면 끊는다.)
	this->set_sustain_timeout(::service::limits_websocket.to_sustain_timeout(), &::service::limits_websocket.count_timeout);
}

bool socket_websocket::on_websocket_upgrade(const sMESSAGE_HTTP_REQUEST& _request)
{
	// check) 허용된 주소 (handshake 전에 거부한다(403).)
	auto address = this->remote_endpoint().address();
	if (::service::is_web_address_allowed(::service::access_policy_websocket, address) == false)
	{
		LOG_WARNING << "(warn) websocket connection rejected (not allowed address: "sv << address.to_string() << ")"sv;
		this->m_is_rejected = true;
		return false;
	}

	// check) Origin (다른 site의 page에서 연 websocket은 handshake 전에 거부한다(403). server_system::is_web_origin_allowed 참고)
	auto ppolicy_origin = ::service::get_web_origin_policy();
	auto origin = _request.get_header("Origin"sv);

	if (server_system::is_web_origin_allowed(origin, _request.get_header("Host"sv), ppolicy_origin->web_port, ppolicy_origin->allowed_origins) == false)
	{
		LOG_WARNING << "(warn) websocket connection rejected (not allowed origin: '"sv << origin << "', address: "sv << this->remote_endpoint().address().to_string() << ")"sv;
		this->m_is_rejected = true;
		return false;
	}

	// check) login (login_key가 있으면 login한 session(cookie)만 받는다.)
	if (::service::is_web_session_valid(_request, this->remote_endpoint().address().to_string()) == false)
	{
		LOG_WARNING << "(warn) websocket connection rejected (not logged in, address: "sv << this->remote_endpoint().address().to_string() << ")"sv;
		this->m_is_rejected = true;
		return false;
	}

	// return)
	return true;
}

void socket_websocket::on_connect()
{
	// - 주소/Origin/login은 on_websocket_upgrade에서 handshake 전에 검사했다.
	this->m_is_rejected = false;

	// - 메시지 제한
	{
		auto ppolicy = ::service::get_web_origin_policy();
		std::lock_guard lock(this->m_lockable_rate);
		this->m_rate_limiter.set_rate(ppolicy->max_messages_per_second, ppolicy->max_messages_burst);
	}

	::service::pgroup_websocket->enter(this);
}

void socket_websocket::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) unregister observable all
	this->unsubscribe_all();

	// 2) leave group
	this->leave_group();
}

result_code socket_websocket::on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/)
{
	// return) 
	return eRESULT::BYPASS;
}

result_code socket_websocket::on_message(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// check) rejected connection
	if (this->m_is_rejected)
		return eRESULT::DONE;

	// check) 메시지 제한 (넘으면 끊는다.)
	bool is_allowed;
	{
		std::lock_guard lock(this->m_lockable_rate);
		is_allowed = this->m_rate_limiter.try_acquire();
	}
	if (is_allowed == false)
	{
		auto str_address = this->remote_endpoint().address().to_string();
		LOG_WARNING << "(warn) websocket message rate exceeded. disconnect (address: "sv << str_address << ")"sv;
		::service::audit.write("web"sv, str_address, "websocket rate limit"sv, 429);
		this->m_is_rejected = true;
		this->disconnect();
		return eRESULT::DONE;
	}

	// audit) 제어 메시지 (SVxx: service, EVxx: time event)
	{
		std::string_view text(_msg.buf_message.data<char>(), _msg.buf_message.size());
		if (text.size() >= 4 && (text.substr(0, 2) == "SV"sv || text.substr(0, 2) == "EV"sv))
		{
			std::string detail(text.substr(0, std::min<std::size_t>(text.size(), 64)));
			std::replace_if(detail.begin(), detail.end(), [](char _ch) { return static_cast<unsigned char>(_ch) < 0x20; }, ' ');
			::service::audit.write("web"sv, this->remote_endpoint().address().to_string(), "websocket "s + std::string(text.substr(0, 4)), 0, detail);
		}
	}

	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code socket_websocket::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}

shared_buffer socket_websocket::make_webmessage_set_main_page(std::string_view _command, std::string_view _page)
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(_command.size() + _page.size() + 1024);

	// 2) set...
	buf_temp.append_text<char>(_command);
	buf_temp.append_text<char>(_page);

	// return)
	return make_shared_buffer(sWEBSOCKET_MESSAGE(buf_temp.to_string<char>()));;
}

void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::string_view _str_value)
{
	_buf_target.append_text<char>(_str_key);
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(_str_value.length());
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(_str_value);
}

void _append_key_value(shared_buffer& _buf_target, std::string_view _str_key, std::size_t _value)
{
	auto str_value = std::to_string(_value);

	_buf_target.append_text<char>(_str_key);
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(str_value.size());
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(str_value);
}

