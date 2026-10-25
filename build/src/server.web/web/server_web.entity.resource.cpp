#include "pch.h"

namespace CGDK
{

server::web::entity::resource::resource() noexcept
{
}

server::web::entity::resource::resource(const std::string_view _source, server::web::eENTITY_TYPE _entity_type, chrono::time::utc::time_point _time_last_modified, chrono::time::duration _duration_expires)
{
	initialize(_source, _entity_type, _time_last_modified, _duration_expires);
}

server::web::entity::resource::~resource() noexcept
{
}

result_code server::web::entity::resource::initialize(const std::string_view _source, server::web::eENTITY_TYPE _entity_type, chrono::time::utc::time_point _time_last_modified, chrono::time::duration _duration_expires)
{
	// declare) 
	context	context_setting;

	// 1) set parameter
	context_setting["source"sv] = _source;
	context_setting["entity_type"sv] = static_cast<uint32_t>(_entity_type);
	context_setting["duration_expires"sv] = _duration_expires;
	context_setting["time_last_modified"sv] = _time_last_modified;

	// 2) initialize
	return this->initialize(context_setting);
}

void server::web::entity::resource::on_initializing(context& _context)
{
	// common)
	init_context();

	// 1) get source directory
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"sv]));

	// 2) type
	switch (entity_type)
	{
	// case general file
	case	server::web::eENTITY_TYPE::RESOURCE_FILE:
			initialize_file(_context);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_BMP:
	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_GIF:
	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_JPEG:
	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_PNG:
	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_ETC:
			initialize_image(_context);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_WAV:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_XWAV:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_XPNWAV:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_MP3:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_OGG:
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_WEBM:
			initialize_audio(_context);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO:
	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MOV:
	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MPEG:
	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_OGG:
	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_WEBM:
			initialize_video(_context);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	}
}

void server::web::entity::resource::on_initialize(context& /*_context*/)
{
}

void server::web::entity::resource::on_destroying()
{
}

void server::web::entity::resource::on_destroy()
{
}

void server::web::entity::resource::on_starting(context& /*_context*/)
{
}

void server::web::entity::resource::on_start(context& /*_context*/)
{
}

void server::web::entity::resource::on_stopping()
{
}

void server::web::entity::resource::on_stop()
{
}

result_code server::web::entity::resource::process_update(context& /*_context*/)
{
	return eRESULT::SUCCESS;
}

void server::web::entity::resource::initialize_file(context& _context)
{
	// 1) get source directory
	auto str_source = (std::string&&)_context["source"];
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"sv]));
	const auto& duration_expires = _context["duration_expires"sv];
	const auto& time_last_modified = _context["time_last_modified"sv];

	// 1) get path name
	std::filesystem::path path_full{ static_cast<std::string_view>(str_source) };

	// 2) read file
	auto buf_read = api::file::read(path_full.string());

	// 3) store
	this->m_buffer_data = buf_read;
	this->m_entity_type = entity_type;
	this->m_duration_expires = (duration_expires.is_valid()) ? static_cast<chrono::time::duration>(duration_expires) : chrono::time::duration();
	this->m_time_last_modified = (time_last_modified.is_valid()) ? static_cast<chrono::time::utc::time_point>(time_last_modified) : chrono::time::utc::time_point();

	// 4) get filename
	this->name(path_full.filename().string());
	this->m_path_source = path_full.string();
}

void server::web::entity::resource::initialize_image(context& _context)
{
	// 1) get source directory
	auto str_source = (std::string&&)_context["source"];
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"sv]));
	auto duration_expires = _context["duration_expires"sv];
	auto time_last_modified = _context["time_last_modified"sv];

	// 2) get path name
	std::filesystem::path path_full{ static_cast<std::string_view>(str_source) };

	// 3) read file
	auto buf_read = api::file::read(path_full.string());

	// 4) store
	this->m_buffer_data = buf_read;
	this->m_entity_type = entity_type;
	this->m_duration_expires = (duration_expires.is_valid()) ? static_cast<chrono::time::duration>(duration_expires) : chrono::time::duration();
	this->m_time_last_modified = (time_last_modified.is_valid()) ? static_cast<chrono::time::utc::time_point>(time_last_modified) : chrono::time::utc::time_point();

	// 5) get filename
	this->name(path_full.filename().string());
	this->m_path_source = path_full.string();
}

void server::web::entity::resource::initialize_audio(context& _context)
{
	// 1) get source directory
	auto str_source = (std::string&&)_context["source"sv];
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"sv]));
	auto duration_expires = _context["duration_expires"sv];
	auto time_last_modified = _context["time_last_modified"sv];

	// 2) get path name
	std::filesystem::path path_full{ static_cast<std::string_view>(str_source) };

	// 3) read file
	auto buf_read = api::file::read(path_full.string());

	// 4) store
	this->m_buffer_data = buf_read;
	this->m_entity_type = entity_type;
	this->m_duration_expires = (duration_expires.is_valid()) ? static_cast<chrono::time::duration>(duration_expires) : chrono::time::duration();
	this->m_time_last_modified = (time_last_modified.is_valid()) ? static_cast<chrono::time::utc::time_point>(time_last_modified) : chrono::time::utc::time_point();

	// 5) get filename
	this->name(path_full.filename().string());
	this->m_path_source = path_full.string();
}

void server::web::entity::resource::initialize_video(context& _context)
{
	// 1) get source directory
	auto str_source = (std::string&&)_context["source"];
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"]));
	auto duration_expires = _context["duration_expires"sv];
	auto time_last_modified = _context["time_last_modified"sv];

	// 2) get path name
	std::filesystem::path path_full{ static_cast<std::string_view>(str_source) };

	// 3) read file
	auto buf_read = api::file::read(path_full.string());

	// 4) store
	this->m_buffer_data = buf_read;
	this->m_entity_type = entity_type;
	this->m_duration_expires = (duration_expires.is_valid()) ? static_cast<chrono::time::duration>(duration_expires) : chrono::time::duration();
	this->m_time_last_modified = (time_last_modified.is_valid()) ? static_cast<chrono::time::utc::time_point>(time_last_modified) : chrono::time::utc::time_point();

	// 5) set path...
	this->name(path_full.filename().string());
	this->m_path_source = path_full.string();
}

void append_http_date(shared_buffer& _buf_dest, const chrono::time::utc::time_point& _time);

int server::web::entity::resource::process_request(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& _path, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// declare) 
	int result = 0;

	// 1) get entity type
	server::web::eENTITY_TYPE entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(m_entity_type) & 0x000ff000);
	
	// 2) call by entity type
	switch(entity_type)
	{
	case	server::web::eENTITY_TYPE::RESOURCE_FILE:
			result = this->process_request_file(_msg, _path, _webserver_parameters);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE:
			result = this->process_request_image(_msg, _path, _webserver_parameters);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO:
			result = this->process_request_audio(_msg, _path, _webserver_parameters);
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO:
			result = this->process_request_viedo(_msg, _path, _webserver_parameters);
			break;

	default:
			CGASSERT_ERROR(false);
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);
	}

	// return) 
	return result;
}

std::string_view get_context_type(server::web::eENTITY_TYPE _entity_type)
{
	// declare)
	std::string_view result;
	
	switch (_entity_type)
	{
	// case) resource image
	case	server::web::eENTITY_TYPE::TEXT:
			result = "text/plain"sv;
			break;

	// case) text
	case	server::web::eENTITY_TYPE::TEXT_HTML:
			result = "text/html"sv;
			break;

	case	server::web::eENTITY_TYPE::TEXT_CSS:
			result = "text/css"sv;
			break;

	case	server::web::eENTITY_TYPE::TEXT_PLAIN:
			result = "text/plain"sv;
			break;

	// case) application
	case	server::web::eENTITY_TYPE::APPLICATION_SCRIPT:
			result = "applicaton/javascript"sv;
			break;

	// case) resource
	case	server::web::eENTITY_TYPE::RESOURCE:
			result = "image/jpeg"sv;
			break;

	//      - resource general
	case	server::web::eENTITY_TYPE::RESOURCE_FILE:
			result = "image/jpeg"sv;
			break;

	//      - resource image
	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE:
			result = "image/*"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_BMP:
			result = "image/bmp"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_GIF:
			result = "image/gif"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_JPEG:
			result = "image/jpeg"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_PNG:
			result = "image/png"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_IMAGE_ETC:
			result = "image/*"sv;
			break;

	//      - resource audio
	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO:
			result = "audio/*"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_WAV:
			result = "audio/wav"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_XWAV:
			result = "audio/x-wav"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_XPNWAV:
			result = "audio/x-pn-wav"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_MP3:
			result = "audio/mp3"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_OGG:
			result = "audio/ogg"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_AUDIO_WEBM:
			result = "audio/webm"sv;
			break;

	//      - resource video
	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO:
			result = "video/*"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MOV:
			result = "video/mov"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_MPEG:
			result = "video/mpeg"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_AVI:
			result = "video/avi"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_OGG:
			result = "video/ogg"sv;
			break;

	case	server::web::eENTITY_TYPE::RESOURCE_VIDEO_WEBM:
			result = "video/webm"sv;
			break;

	// 8) 
	default:
			result = "text/plain"sv;
			break;
	}

	// return)
	return result;
}

int server::web::entity::resource::process_request_file(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& /*_path*/, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// check)
	THROW_IF(_msg.method != eHTTP_METHOD::GET_ && _msg.method != eHTTP_METHOD::POST_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc shred buffer for http header
	auto buf_head = alloc_shared_buffer(1024);

	// 2) make http header
	{
		// - add response head
		buf_head.append_text(CGDK::stream_string("HTTP/", 1, ".", 1, " ", 200, " ", "OK"sv, "\r\n"));

		// - get now
		auto time_now = chrono::time::utc::now();

		// - append 'Date'
		buf_head.append_text("Date: "sv);
		append_http_date(buf_head, time_now);
		// - append 'Expires'
		buf_head.append_text("Expires: "sv);
		append_http_date(buf_head, time_now + m_duration_expires);
		// - append 'Last-Modified'
		buf_head.append_text("Last-Modified: "sv);
		append_http_date(buf_head, m_time_last_modified);

		// - server info (Server, X-Powered-By, Expires, Last-Modified, ...)
		//buf_head.push_back(m_buffer_head_server_info);

		// - append additional http header
		buf_head.append_text("Cache-control: no-store, no-cache, must-revalidate\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Content-Type: ", get_context_type(m_entity_type), "\r\n"));
		buf_head.append_text(CGDK::stream_string("MIME-version ", 1, ".", 0, "\r\n"));
		buf_head.append_text(CGDK::stream_string("Content-length:", m_buffer_data.size(), "\r\n"));
		buf_head.append_text(CGDK::stream_string("Server: ", _webserver_parameters.server_name, "/", _webserver_parameters.server_version_high, ".", _webserver_parameters.server_version_low, "\r\n"));
		buf_head.append_text("\r\n"sv);
	}

	struct sWEB_SERVER_CONTEXT
	{
		std::string_view		server_name{ "CGDK10" };
		uint32_t				server_version_high{ 1 };
		uint32_t				server_version_low{ 0 };
	};

	// 3) get socket
	auto psocket = _msg.get_source<net::socket::web_server>();

	// 4) send http response message
	psocket->send(buf_head);
	psocket->send(m_buffer_data);

	// return) 
	return 0;
}

int server::web::entity::resource::process_request_image(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& /*_path*/, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// check)
	THROW_IF(_msg.method != eHTTP_METHOD::GET_ && _msg.method != eHTTP_METHOD::POST_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc shared buffer
	auto buf_head = alloc_shared_buffer(1024);

	// 2) make http header
	{
		// - append http response header
		buf_head.append_text(CGDK::stream_string("HTTP/", 1, ".", 1, " ", 200, " ", "OK"sv, "\r\n"));

		// - get now
		auto time_now = chrono::time::utc::now();

		// - append 'Date'
		buf_head.append_text("Date: "sv);
		append_http_date(buf_head, time_now);
		// - append 'Expires'
		buf_head.append_text("Expires: "sv);
		append_http_date(buf_head, time_now + m_duration_expires);
		// - append 'Last-Modified'
		buf_head.append_text("Last-Modified: "sv);
		append_http_date(buf_head, m_time_last_modified);

		// - server info (Server, X-Powered-By, Expires, Last-Modified, ...)
		//buf_head.push_back(m_buffer_head_server_info);

		// - append additional http header
		buf_head.append_text("Cache-control: no-store, no-cache, must-revalidate\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Content-Type: ", get_context_type(m_entity_type), "\r\n"));
		buf_head.append_text(CGDK::stream_string("MIME-version ", 1, ".", 0, "\r\n"));
		buf_head.append_text(CGDK::stream_string("Content-length:", m_buffer_data.size(), "\r\n"));
		buf_head.append_text(CGDK::stream_string("Server: ", _webserver_parameters.server_name, "/", _webserver_parameters.server_version_high, ".", _webserver_parameters.server_version_low, "\r\n"));
		buf_head.append_text("\r\n"sv);
	}

	// 3) get socket
	auto psocket = _msg.get_source<net::socket::web_server>();

	// 4) send http response message
	psocket->send(buf_head);
	psocket->send(m_buffer_data);

	// return) 
	return 0;
}

int server::web::entity::resource::process_request_audio(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& /*_path*/, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// check)
	THROW_IF(_msg.method != eHTTP_METHOD::GET_ && _msg.method != eHTTP_METHOD::POST_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc shred buffer for http header
	auto buf_head = alloc_shared_buffer(1024);

	// 2) make http header
	{
		// - add response head
		buf_head.append_text(CGDK::stream_string("HTTP/", 1, ".", 1, " ", 200, " ", "OK"sv, "\r\n"));

		// - get now
		auto time_now = chrono::time::utc::now();

		// - append 'Date'
		buf_head.append_text("Date: "sv);
		append_http_date(buf_head, time_now);
		// - append 'Expires'
		buf_head.append_text("Expires: "sv);
		append_http_date(buf_head, time_now + m_duration_expires);
		// - append 'Last-Modified'
		buf_head.append_text("Last-Modified: "sv);
		append_http_date(buf_head, m_time_last_modified);

		// - server info (Server, X-Powered-By, Expires, Last-Modified, ...)
		//buf_head.push_back(m_buffer_head_server_info);

		// - append additional http header
		buf_head.append_text("Cache-control: no-store, no-cache, must-revalidate\r\n"sv);
		buf_head.append_text("Connection: close\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Content-Type: ", get_context_type(m_entity_type), "\r\n"));
		buf_head.append_text("MIME-version 1.0\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Content-length:", m_buffer_data.size(), "\r\n"));
		buf_head.append_text(CGDK::stream_string("Server: ", _webserver_parameters.server_name, "/", _webserver_parameters.server_version_high, ".", _webserver_parameters.server_version_low, "\r\n"));
		buf_head.append_text("\r\n");
	}

	// 3) get socket
	auto psocket = _msg.get_source<net::socket::web_server>();

	// 4) send http response message
	psocket->send(buf_head);
	psocket->send(m_buffer_data);

	// return) 
	return 0;
}

int server::web::entity::resource::process_request_viedo(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& /*_path*/, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// check)
	THROW_IF(_msg.method != eHTTP_METHOD::GET_ && _msg.method != eHTTP_METHOD::POST_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc shred buffer for http header
	auto buf_head = alloc_shared_buffer(1024);

	// 2) make http header
	{
		// - add response head
		buf_head.append_text(CGDK::stream_string("HTTP/", 1, ".", 1, " ", 200, " ", "OK"sv, "\r\n"));

		// - get now
		auto time_now = chrono::time::utc::now();

		// - append 'Date'
		buf_head.append_text("Date: "sv);
		append_http_date(buf_head, time_now);
		// - append 'Expires'
		buf_head.append_text("Expires: "sv);
		append_http_date(buf_head, time_now + m_duration_expires);
		// - append 'Last-Modified'
		buf_head.append_text("Last-Modified: "sv);
		append_http_date(buf_head, m_time_last_modified);

		// - append expires
		//append_last_modified();

		// - server info (Server, X-Powered-By, Expires, Last-Modified, ...)
		//buf_head.push_back(m_buffer_head_server_info);

		// - append additional http header
		buf_head.append_text("Cache-control: no-store, no-cache, must-revalidate\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Content-Type: ", get_context_type(m_entity_type), "\r\n"));
		buf_head.append_text(CGDK::stream_string("MIME-version ", 1, ".", 0, "\r\n"));
		buf_head.append_text(CGDK::stream_string("Content-length:", m_buffer_data.size(), "\r\n"));
		buf_head.append_text("x-content-type-options:nosniff\r\n"sv);
		buf_head.append_text("x-xss-protection:0\r\n"sv);
		buf_head.append_text(CGDK::stream_string("Server: ", _webserver_parameters.server_name, "/", _webserver_parameters.server_version_high, ".", _webserver_parameters.server_version_low, "\r\n"));
		buf_head.append_text("\r\n");
	}

	// 2) alloc shred buffer for http header
	auto buf_context_tail = alloc_shared_buffer(1024);
	buf_context_tail.append_text("\r\n");

	// 3) get socket
	auto psocket = _msg.get_source<net::socket::web_server>();

	// 4) send http response message
	psocket->send(buf_head);
	psocket->send(m_buffer_data);

	// 5) disconnect
	psocket->disconnect();

	// return) 
	return 0;
}


}




//-----------------------------------------------------------------
// 3. head 작성
//
//	② 프로토콜과 응답코드: 3개의 필드로 구성되었으며, 첫출에 위치한다.
//	  A. 응답 프로토콜과 버전
//	  B. 응답 코드
//	     - 100  계속 (요청자는 요청을 계속한다. 서버는 요청의 첫 번째 부분을 받았으며 나머지를 기다리고 있음을 나타낸다.)
//         101	Protocol 전환 (요청자가 서버에 프로토콜 전환을 요청했으며 서버는 이를 승인하는 중임)
//
//	     - 200  성공 (서버가 요청을 제대로 처리했다.
//         201	생성됨 (요청이 성공했으며 새로운 리소스를 만들었다.)
//         202	허용됨 (요청을 받았으나 아직 처리하지 못했다.)
//         204	콘텐츠 없음 (요청을 받았으나 콘텐츠를 제공하지는 않는다.)
//         205	콘텐츠 재설정 (요청을 받았으나 콘텐츠를 제공하지는 않는다. 그리고 문서를 재 설정할 것을 요구한다.)
//         206	일부 성공 (요청의 일부만 성공적으로 처리)
//         207	다중 상태
//
//       - 300	여러 선택 항목
//         301	영구이동 (요청한 페이지가 다른 위치로 영구이동 했다.)
//         302	임시이동 (요청한 페이지가 다른 위치로 임시이동했다. 요청자는 여전히 현재 페이지를 요청해야 한다.)
//         303	기타위치 보기 (요청자가 다른 위치에 별도의 GET 요청을 하여 응답을 검색할 경우)
//         304	수정되지 않음 (마지막 요청 이후 요청한 페이지가 수정되지 않았다. if-Modified-Sine 헤더에 지정된 시간/날짜 이래로 지정된 문서가 변경된 사실이 없는 경우 이 code를 응답한다.)
//         305	프록시 사용 (요청자는 프록시를 사용하여 요청한 페이지만 접근할 수 있다.)
//
//	     - 400	잘못된 요청 (주로 헤더 포맷이 HTTP 규약에 맞지 않을 경우)
//	       401	권한 없음 (인증을 필요로 하는 요청이다. Basic access authentication에 속한다.)
//	       403	금지 (서버가 요청을 거부하고 있다.)
//	       404	찾을 수 없음 (요청한 자원이 서버에 존재하지 않는다.)
//	       405	허용하지 않은 방법 (요청에 지정한 방법을 사용할 수 없다.)
//	       406	허용되지 않음 (요청한 페이지를 콘텐츠 특성 때문에 응답할 수 없다.)
//	       408	요청 시간 초과 (서버의 요청대기가 시간을 초과)
//	       410	사라짐 (요청한 자원이 삭제되었음, 404와 비슷하지만 410은 과거에 있었으나 지금은 없는 자원이다. 에컨데 게시판에서 삭제한 포스트에 접근하는 경우)
//	       411	길이필요 (유효한 콘텐츠 길이를 명시해야 한다.)
//
//	     - 500	내부 서버 오류 (서버에 오류 발생)
//	       501	구현되지 않음 (서버에 요청을 수행할 기능이 없다.)
//	       503	서비스를 사용할 수 없다. (관리 목적 등으로 서버를 다운했다.)
//	       505	HTTP 버전 지원이 안됨 (서버가 요청에 사용된 HTTP 프로토콜 버전을 지원하지 않는다.)
//
//	  C. 응답 메시지
//	③ Date: 날짜
//	④ Server: 서버 프로그램 및 스크립트 정보
//  ④ MIME version: 
//	⑨ Content-Length: 콘텐츠 길이
//
// 
// 	⑨ Context Type
//    - 텍스트 데이터							text/plane, text/html
//	  - 그림 데이터								image/jpeg, image/gif
//    - 음성 데이터								audio/mpeg
//	  - 동영상 데이터							video/mp4
//	  - 그 밖의 데이터							application/pdf
//	  - 복수의 데이터로 이루어진 복합 데이터		multipart/related
//	  - 전자메일 메시지							message/rfc822
//	  - 복수차원으로 구성된 모델 데이터			model/vrml
//	  - 예시용									example/foo-bar
//
//  ⑨ 데이터 종류에 따른 Context Type
//		- text/plane							플레인 텍스트
//		- text/csv								cvs형식 텍스트
//		- text/css								css형식의 스타일 시트
//		- text/html								HTML문저
//		- text/xml								XML문서
//		- image/jpeg							JPEG 이미지
//		- image/gif								GIF 이미지
//		- image/png								PNG 이미지
//		- application/xml						XML문서
//		- application/xhtml+xml					XHTML문서
//		- application/atom+xml					ATOM 문서
//		- application/atomsvc+xml				Atom의 서비스 문저
//		- application/atomcat+mxl				Atom의 카테고리 문서
//		- application/json						JSON문서
//		- application/msword					Word문서
//		- application/vnd.ms-excel				PowerPoint 문서
//		- application/pdf						PDF문서
//		- application/Zip						ZIP파일
//		- application/x-shockwave-flash			Flash 오브펙트
//		- application/x-www-form-urlencoded		HTML 폼형시
//		- application/postscript				Postscript
//		- video/mpeg							MPEG
//		- audit/x-pn-realaudio					Real Player
//
//
// 예)	② HTTP/1.1 200 OK\r\n
//		③ Date: Fri, 08 Jul 2011 00:59:41 GMT\r\n
//		④ Server: CGCII/8.0.0\r\n
// 		④ Last-Modifid: Sat, 07 Feb 2014
// 		④ ETag: "0-23-4024c3a5"
// 		④ Accept-Ranges: bytes
//		⑥ MIME-version 1.0
//		⑨ Content-Length: 102\r\n
//		⑥ Connection: close
//		⑥ Content-Type: image/jpeg
//		⑬ \r\n
//
//
//-----------------------------------------------------------------
