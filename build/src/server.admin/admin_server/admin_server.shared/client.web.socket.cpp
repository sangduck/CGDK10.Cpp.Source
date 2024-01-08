#include "pch.h"


//extern std::string_view g_string_webpage_frame_htm;
//extern std::string_view g_string_webpage_host_info_htm;
//extern std::string_view g_string_webpage_host_log_htm;
//extern std::string_view g_string_webpage_reject_htm;
//extern std::string_view g_string_webpage_server_info_htm;
//extern std::string_view g_string_webpage_server_overview_info_htm;
//extern std::string_view g_string_webpage_server_factory_info_htm;
//extern std::string_view g_string_webpage_server_executor_info_htm;
//extern std::string_view g_string_webpage_server_network_info_htm;
//extern std::string_view g_string_webpage_server_querier_info_htm;
//extern std::string_view g_string_webpage_server_crash_info_htm;
//extern std::string_view g_string_webpage_server_setting_htm;
//extern std::string_view g_string_webpage_server_statistics_htm;
//extern std::string_view g_string_webpage_time_event_info_htm;
//
//shared_buffer m_buffer_webpage_frame_htm;
//shared_buffer m_buffer_webpage_host_info_htm;
//shared_buffer m_buffer_webpage_host_log_htm;
//shared_buffer m_buffer_webpage_reject_htm;
//shared_buffer m_buffer_webpage_server_info_htm;
//shared_buffer m_buffer_webpage_server_overview_info_htm;
//shared_buffer m_buffer_webpage_server_factory_info_htm;
//shared_buffer m_buffer_webpage_server_executor_info_htm;
//shared_buffer m_buffer_webpage_server_network_info_htm;
//shared_buffer m_buffer_webpage_server_querier_info_htm;
//shared_buffer m_buffer_webpage_server_crash_info_htm;
//shared_buffer m_buffer_webpage_server_setting_htm;
//shared_buffer m_buffer_webpage_server_statistics_htm;
//shared_buffer m_buffer_webpage_time_event_info_htm;

client::web::socket::socket()
{
	//m_buffer_webpage_frame_htm					 = make_shared_buffer(text(g_string_webpage_frame_htm));
	//m_buffer_webpage_host_info_htm				 = make_shared_buffer(text(g_string_webpage_host_info_htm));
	//m_buffer_webpage_host_log_htm				 = make_shared_buffer(text(g_string_webpage_host_log_htm));
	//m_buffer_webpage_reject_htm					 = make_shared_buffer(text(g_string_webpage_reject_htm));
	//m_buffer_webpage_server_info_htm			 = make_shared_buffer(text(g_string_webpage_server_info_htm));
	//m_buffer_webpage_server_overview_info_htm	 = make_shared_buffer(text(g_string_webpage_server_overview_info_htm));
	//m_buffer_webpage_server_factory_info_htm	 = make_shared_buffer(text(g_string_webpage_server_factory_info_htm));
	//m_buffer_webpage_server_executor_info_htm	 = make_shared_buffer(text(g_string_webpage_server_executor_info_htm));
	//m_buffer_webpage_server_network_info_htm	 = make_shared_buffer(text(g_string_webpage_server_network_info_htm));
	//m_buffer_webpage_server_querier_info_htm	 = make_shared_buffer(text(g_string_webpage_server_querier_info_htm));
	//m_buffer_webpage_server_crash_info_htm		 = make_shared_buffer(text(g_string_webpage_server_crash_info_htm));
	//m_buffer_webpage_server_setting_htm			 = make_shared_buffer(text(g_string_webpage_server_setting_htm));
	//m_buffer_webpage_server_statistics_htm		 = make_shared_buffer(text(g_string_webpage_server_statistics_htm));
	//m_buffer_webpage_time_event_info_htm		 = make_shared_buffer(text(g_string_webpage_time_event_info_htm));
}

client::web::socket::~socket()
{
}

void client::web::socket::on_connect(net::io::Iconnective* /*_pconnective*/)
{
	// 1) 임시 websocket임이 인증된 후 호출되어야 한다.
	g_pservice->m_pgroup_client_web->enter(this);
}

void client::web::socket::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) unregister observable all
	unsubscribe_all();

	// 2) leave group
	leave_group();
}

result_code client::web::socket::on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/)
{
	//// 1)
	//if (_msg.path == reinterpret_cast<const char8_t*>("/")) // 
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_frame_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_frame_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_host_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_host_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_host_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_executor_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_executor_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_executor_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_network_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_network_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.status_message = to_string<char>(msg_http_response.status_code);
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_network_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_querier_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_querier_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_querier_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_crash_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_executor_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_crash_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_factory_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_factory_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_factory_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_log.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_time_event_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_time_event_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_overview_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_overview_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_overview_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_statistics.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_server_statistics_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_server_statistics_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}
	//else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_time_event_info.htm"))
	//{
	//	// check)
	//	CGASSERT_ERROR(m_buffer_webpage_time_event_info_htm.empty() == false);

	//	// - make response message
	//	sMESSAGE_HTTP_RESPONSE msg_http_response;
	//	msg_http_response.version = _msg.version;
	//	msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
	//	msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
	//	msg_http_response.content_type = "text/html; charset=utf-8";
	//	msg_http_response.contents.push_back(m_buffer_webpage_time_event_info_htm);

	//	// - send response
	//	send(make_shared_buffer(msg_http_response));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code client::web::socket::on_message(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return) 
	return eRESULT::BYPASS;
}

result_code client::web::socket::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}

shared_buffer client::web::socket::make_webmessage_set_main_page(std::string_view _command, std::string_view _page)
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
