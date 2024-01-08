#include "pch.h"
#include "socket_web.h"

extern std::string_view g_string_webpage_frame_htm;
extern std::string_view g_string_webpage_host_info_htm;
extern std::string_view g_string_webpage_host_log_htm;
extern std::string_view g_string_webpage_reject_htm;
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

shared_buffer m_buffer_webpage_frame_htm;
shared_buffer m_buffer_webpage_host_info_htm;
shared_buffer m_buffer_webpage_host_log_htm;
shared_buffer m_buffer_webpage_reject_htm;
shared_buffer m_buffer_webpage_server_info_htm;
shared_buffer m_buffer_webpage_server_overview_info_htm;
shared_buffer m_buffer_webpage_server_factory_info_htm;
shared_buffer m_buffer_webpage_server_executor_info_htm;
shared_buffer m_buffer_webpage_server_network_info_htm;
shared_buffer m_buffer_webpage_server_querier_info_htm;
shared_buffer m_buffer_webpage_server_crash_info_htm;
shared_buffer m_buffer_webpage_server_setting_htm;
shared_buffer m_buffer_webpage_server_statistics_htm;
shared_buffer m_buffer_webpage_time_event_info_htm;

socket_web::socket_web()
{
	if(m_buffer_webpage_frame_htm.empty())
	{
		m_buffer_webpage_frame_htm					 = make_shared_buffer(text(g_string_webpage_frame_htm));
		m_buffer_webpage_host_info_htm				 = make_shared_buffer(text(g_string_webpage_host_info_htm));
		m_buffer_webpage_host_log_htm				 = make_shared_buffer(text(g_string_webpage_host_log_htm));
		m_buffer_webpage_reject_htm					 = make_shared_buffer(text(g_string_webpage_reject_htm));
		m_buffer_webpage_server_info_htm			 = make_shared_buffer(text(g_string_webpage_server_info_htm));
		m_buffer_webpage_server_overview_info_htm	 = make_shared_buffer(text(g_string_webpage_server_overview_info_htm));
		m_buffer_webpage_server_factory_info_htm	 = make_shared_buffer(text(g_string_webpage_server_factory_info_htm));
		m_buffer_webpage_server_executor_info_htm	 = make_shared_buffer(text(g_string_webpage_server_executor_info_htm));
		m_buffer_webpage_server_network_info_htm	 = make_shared_buffer(text(g_string_webpage_server_network_info_htm));
		m_buffer_webpage_server_querier_info_htm	 = make_shared_buffer(text(g_string_webpage_server_querier_info_htm));
		m_buffer_webpage_server_crash_info_htm		 = make_shared_buffer(text(g_string_webpage_server_crash_info_htm));
		m_buffer_webpage_server_setting_htm			 = make_shared_buffer(text(g_string_webpage_server_setting_htm));
		m_buffer_webpage_server_statistics_htm		 = make_shared_buffer(text(g_string_webpage_server_statistics_htm));
		m_buffer_webpage_time_event_info_htm		 = make_shared_buffer(text(g_string_webpage_time_event_info_htm));
	}
}

socket_web::~socket_web()
{
}

void socket_web::on_connect()
{
}

void socket_web::on_disconnect(uint64_t /*_disconnect_reason*/)
{
}

result_code socket_web::on_message(sMESSAGE_HTTP_REQUEST& _msg)
{
	// 1)
	if (_msg.path == reinterpret_cast<const char8_t*>("/")) // 
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_frame_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_frame_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_host_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_host_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_host_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_executor_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_executor_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_executor_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_network_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_network_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.status_message = to_string<char>(msg_http_response.status_code);
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_network_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_querier_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_querier_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_querier_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_crash_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_executor_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_crash_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_factory_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_factory_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_factory_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_log.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_time_event_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_time_event_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_overview_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_overview_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_overview_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_server_statistics.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_server_statistics_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_server_statistics_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}
	else if (_msg.path == reinterpret_cast<const char8_t*>("/webpage_time_event_info.htm"))
	{
		// check)
		CGASSERT_ERROR(m_buffer_webpage_time_event_info_htm.empty() == false);

		// - make response message
		sMESSAGE_HTTP_RESPONSE msg_http_response;
		msg_http_response.version = _msg.version;
		msg_http_response.status_code = eHTTP_STATUS_CODE::OK_;
		msg_http_response.headers.push_back({ "Access-Control-Allow-Origin", "http://127.0.0.1:22052/" });
		msg_http_response.content_type = "text/html; charset=utf-8";
		msg_http_response.contents.push_back(m_buffer_webpage_time_event_info_htm);

		// - send response
		send(make_shared_buffer(msg_http_response));
	}

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
