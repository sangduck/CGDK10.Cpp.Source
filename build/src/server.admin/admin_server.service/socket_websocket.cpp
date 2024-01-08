#include "pch.h"
#include "socket_websocket.h"


socket_websocket::socket_websocket()
{
}

socket_websocket::~socket_websocket()
{
}

void socket_websocket::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	service::get_instance()->m_pgroup_websocket->enter(this);
}

void socket_websocket::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) unregister observable all
	unsubscribe_all();

	// 2) leave group
	leave_group();
}

result_code socket_websocket::on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/)
{
	// return) 
	return eRESULT::BYPASS;
}

result_code socket_websocket::on_message(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
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
	auto buf_temp = alloc_shared_buffer(1024);

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
	auto str_value = __to_string<std::string>(_value);

	_buf_target.append_text<char>(_str_key);
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(str_value.size());
	_buf_target.append<char>('\n');
	_buf_target.append_text<char>(str_value);
}
