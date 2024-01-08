#include "pch.h"

void url_decoding(std::u8string_view& _url);

namespace CGDK
{

server::web::entity::application::application() noexcept
{
}

server::web::entity::application::application(const std::string_view _source, server::web::eENTITY_TYPE _entity_type)
{
	initialize(_source, _entity_type);
}

server::web::entity::application::~application() noexcept
{
}

result_code server::web::entity::application::initialize(const std::string_view _source, server::web::eENTITY_TYPE _entity_type)
{
	// declare) 
	context	context_setting;

	// 1) ...
	context_setting["source"sv] = _source;
	context_setting["entity_type"sv] = static_cast<uint32_t>(_entity_type);

	// 2) 
	return this->initialize(context_setting);
}

void server::web::entity::application::on_initializing(context& _context)
{
	// common)
	init_context();

	// 1) get source
	std::string str_source = _context["source"sv];
	auto entity_type = static_cast<server::web::eENTITY_TYPE>(static_cast<uint32_t>(_context["entity_type"sv]));

	// 5) get path name
	std::filesystem::path path_full{ str_source };

	// declare) 
	std::vector<PARTIAL> vector_data;

	// 1) read file
	auto buf_read = api::file::read(str_source);

	// 2) encode한다.
	buf_read = process_encode(buf_read);

	// 3) Parsing을 하여 저장한다.
	{
		// - 추가...
		PARTIAL	tempData;
		tempData.buffer_text = buf_read;
		//tempData.pParser	 = nullptr;

		// - 추가
		vector_data.push_back(tempData);
	}

	// 4) 최종 저장
	m_vector_data.swap(vector_data);

	// 5) set entity
	m_entity_type = entity_type;

	// 6) set path...
	set_name(path_full.filename().string());
	m_path_source = path_full.string();
}

void server::web::entity::application::on_initialize(context& /*_context*/)
{
}

void server::web::entity::application::on_destroying()
{
}

void server::web::entity::application::on_destroy()
{
}

void server::web::entity::application::on_starting(context& /*_context*/)
{
}

void server::web::entity::application::on_start(context& /*_context*/)
{
}

void server::web::entity::application::on_stopping()
{
}

void server::web::entity::application::on_stop()
{
}

int server::web::entity::application::get_encode_type() const noexcept
{
	return	m_encode_type;
}

void server::web::entity::application::set_encode_type(int _encode_type)
{
	// check) 
	RETURN_IF(m_encode_type == _encode_type);

	// 1) encode Type을 설정한다.
	m_encode_type = _encode_type;

	// 2) 재 Load한다.

}

shared_buffer server::web::entity::application::process_encode(shared_buffer& _buffer)
{
	//// 1) 최대 3배의 Buffer를 확보한다.
	//shared_buffer	temp_buffer	 = MEM_AUTO_ALLOC(_buffer.size()*3);

	//// 2) 코드 변환
	//char*	pchReadNow	 = _buffer.data();
	//char*	pchReadEnd	 = _buffer.data()+_buffer.size();

	//char*	pchWriteNow	 = temp_buffer.data();

	//for(;pchReadNow!=pchReadEnd; )
	//{
	//	char	chReadNow	 = *pchReadNow++;

	//	if(!(chReadNow & 0x80))
	//	{
	//		*pchWriteNow++	 = chReadNow;
	//	}
	//	else
	//	{
	//		char*	pchReadNext	 = pchReadNow;

	//		if(pchReadNow!=pchReadEnd)
	//		{

	//		}
	//		else
	//		{

	//		}
	//	}


	//}





	//// 1) 
	//auto bufferDecoded = alloc_shared_buffer(_buffer.size());

	//// 2) 값을 복사한다.
	//char* pchReadNow = _buffer.data();
	//char* pchReadEnd = _buffer.data() + _buffer.size();

	//char* pchWriteNow = bufferDecoded.data();

	//for (; pchReadNow < pchReadEnd;)
	//{
	//	unsigned char	chReadNow = (unsigned char)get_next_char(pchReadNow);

	//	// Case-1) 
	//	if ((chReadNow & 0x80) == 0)
	//	{
	//		*pchWriteNow++ = chReadNow;
	//	}
	//	// Case-2) 110xxxxx 10xxxxxx
	//	else if ((chReadNow & 0xe0)==0xc0)
	//	{
	//		unsigned char	ch1 = (unsigned char)get_next_char(pchReadNow);

	//		*pchWriteNow++ = (chReadNow << 6) | (ch1 & 0x3f);
	//	}
	//	// Case-3) 1110xxxx 10xxxxxx 10xxxxxx
	//	else if ((chReadNow & 0xf0) == 0xe0)
	//	{
	//		unsigned char	ch1 = (unsigned char)get_next_char(pchReadNow);
	//		unsigned char	ch2 = (unsigned char)get_next_char(pchReadNow);

	//		*pchWriteNow++ = (chReadNow<<4) | ((ch1>>2) & 0x0f);
	//		*pchWriteNow++ = (ch1<<6) | (ch2 & 0x3f);
	//	}
	//	// Case-4) 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	//	else if ((chReadNow & 0xf8) == 0xf0)
	//	{
	//		CGASSERT_ERROR(false);
	//	}
	//}

	//// 3) Length를 설정
	//bufferDecoded.size() = pchWriteNow - bufferDecoded.data();

	//// return) 
	//return	bufferDecoded;


	return	_buffer;
}

result_code server::web::entity::application::process_update(context& /*_context*/)
{
	return eRESULT::SUCCESS;
}

std::size_t server::web::entity::application::process_build_body(sMESSAGE_HTTP_REQUEST& /*_msg*/, std::vector<shared_buffer>& _vector_buffer)
{
	// Decalre)
	std::size_t size = 0;

	for(auto iter:m_vector_data)
	{
		_vector_buffer.push_back(iter.buffer_text);
		size  += iter.buffer_text.size();
	}

	// return) 
	return size;
}

// HTTP-date    = rfc1123-date | rfc850-date | asctime-date
// rfc1123-date = wkday "," SP date1 SP time SP "GMT"
// rfc850-date  = weekday "," SP date2 SP time SP "GMT"
// asctime-date = wkday SP date3 SP time SP 4DIGIT
// date1        = 2DIGIT SP month SP 4DIGIT
//                ; day month year (e.g., 02 Jun 1982)
// date2        = 2DIGIT "-" month "-" 2DIGIT
//                ; day-month-year (e.g., 02-Jun-82)
// date3        = month SP ( 2DIGIT | ( SP 1DIGIT ))
//                ; month day (e.g., Jun  2)
// time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT
//                ; 00:00:00 - 23:59:59
// wkday        = "Mon" | "Tue" | "Wed"
//              | "Thu" | "Fri" | "Sat" | "Sun"
// weekday      = "Monday" | "Tuesday" | "Wednesday"
//              | "Thursday" | "Friday" | "Saturday" | "Sunday"
// month        = "Jan" | "Feb" | "Mar" | "Apr"
//              | "May" | "Jun" | "Jul" | "Aug"
//              | "Sep" | "Oct" | "Nov" | "Dec"

void append_http_date(shared_buffer& _buf_dest, const chrono::time::utc::time_point& _time);
std::string_view get_context_type(server::web::eENTITY_TYPE _entity_type);

int server::web::entity::application::process_request(sMESSAGE_HTTP_REQUEST& _msg, std::vector<std::string>& /*_path*/, const sWEB_SERVER_CONTEXT& _webserver_parameters)
{
	// check)
	THROW_IF(_msg.method != eHTTP_METHOD::GET_ && _msg.method != eHTTP_METHOD::POST_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// declare) 
	std::vector<shared_buffer> vec_buf;

	// 1) build http body
	auto bytes_body = process_build_body(_msg, vec_buf);

	// 2) alloc shred buffer for http header
	auto buf_head = alloc_shared_buffer(1024);

	// 3) make http header
	{
		// - response head 만들기
		buf_head.append_text("HTTP/{}.{} {} {}\r\n"sv, 1, 1, "OK"sv, 200);

		// - Date (예: "Date: Fri, 08 Jul 2011 00:59:41 GMT\r\n")
		buf_head.append_text("Date: "sv);
		append_http_date(buf_head, chrono::time::utc::now());

		// - Server Info (Server, X-Powered-By, Expires, Last-Modified, ...)
		//buf_head.push_back(m_buffer_head_server_info);

		// - 나머지 head를 작성한다.
		buf_head.append_text("Cache-control: no-store, no-cache, must-revalidate\r\n"sv);
		buf_head.append_text("Connection: close\r\n"sv);
		buf_head.append_text("Content-Type: {}\r\n"sv, get_context_type(m_entity_type)); // ; charset=UTF-8
		buf_head.append_text("Content-length:{}\r\n"sv, bytes_body);
		buf_head.append_text("Server:{}/{}.{}\r\n"sv, _webserver_parameters.server_name.data(), _webserver_parameters.server_version_high, _webserver_parameters.server_version_low);
		//buf_head.append_text("x-frame-options: SAMEORIGIN\r\n"sv);
		//buf_head.append_text("x-xss-protection:0\r\n"sv);
		buf_head.append_text("\r\n"sv);
	}

	// 4) socket을 얻는다.
	auto psocket = _msg.get_source<net::socket::web_server>();

	// 5) send http response message
	psocket->send(buf_head);
	for(auto& iter: vec_buf)
	{
		psocket->send(iter);
	}
	psocket->send(m_buffer_context_tail);

	// 6) disconnect
	psocket->disconnect();

	// return) 
	return	0;
}


}



//-----------------------------------------------------------------
//
//	② 프로토콜과 응답코드: 3개의 필드로 구성되었으며, 첫출에 위치한다.
//	  A. 응답 프로토콜과 버전
//	  B. 응답 코드 (http://en.wikipedia.org/wiki/List_of_HTTP_status_codes)
//	     - 100  계속 (요청자는 요청을 계속한다. 서버는 요청의 첫번재 부분을 받았으며 나머지를 기다리고 있음을 나타낸다.)
//         101	Protocol 전환 (요청자가 서버에 프로토콜 전환을 요청했으며 서버는 이를 승인하는 중임)
//
//	     - 200  성공 (서버가 요청을 제대로 처리했다.
//         201	생성됨 (요청이 성공했으며 새로운 리소스를 만들었다.)
//         202	허용됨 (요청을 받았으나 아직 처리하지 못했다.)
//         204	컨텐츠 없음 (요청을 받았으나 컨텐츠를 제공하지는 않는다.)
//         205	컨텐츠 재설정 (요청을 받았으나 컨텐츠를 제공하지는 않는다. 그리고 문서를 재 설정할 것을 요구한다.)
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
//	     - 400	잘못된 요청 (주로 헤더 포맷이 HTTq , 규약에 맞지 않을 경우)
//	       401	권한 없음 (인증을 필요로 하는 요청이다. Basic access authentication에 속한다.)
//	       403	금지 (서버가 요청을 거부하고 있다.)
//	       404	찾을 수 없음 (요청한 자원이 서버에 존재하지 않는다.)
//	       405	허용하지 않은 방법 (요청에 지정한 방법을 사용할 수 없다.)
//	       406	허용되지 않음 (요청한 페이지를 콘텐츠 특성 때문에 응답할 수 없다.)
//	       408	요청 시간 초과 (서버의 요청대기가 시간을 초과)
//	       410	사라짐 (요청한 자원이 삭제되었음, 404와 비슷하지만 410은 과거에 있었으나 지금은 없는 자원이다. 에컨데 게시판에서 삭제한 포스트에 접근하는 경우)
//	       411	길이필요 (유효한 컨텐츠 길이를 명시해야 한다.)
//
//	     - 500	내부 서버 오류 (서버에 오류 발생)
//	       501	구현되지 않음 (서버에 요청을 수행할 기능이 없다.)
//	       503	서비스를 사용할수 없다. (관리 목적 등으로 서버를 다운했다.)
//	       505	HTTP 버전 지원이 안됨 (서버가 요청에 사용된 HTTP 프로토콜 버전을 지원하지 않는다.)
//
//	  C. 응답 메시지
//	③ Date: 날짜
//	④ Server: 서버 프로그램 및 스크립트 정보
//	⑤ X-Powered-By: 웹 애플리케이션을 지원하는 기술 명세 (필요없이 트레픽을 증가시켜 제거하기도 한다.)
//	⑥ Expires: 데이터 보관일시
//	⑦ Last-Modified: 컨텐츠의 마지막 수정일
//	⑧ Cache-control: 캐시 제어 방식
//	⑨ Content-Length: 컨텐츠 길이
//	⑩ Keep-Alive: Keep Alive 기능 설정 (HTTP 1.1부터 사용 가능)
//	⑪ Connection: Keep Alive 기능 설정
//	⑫ Content-Type: 컨텐츠 타입
//
// 
// 예)	② HTTP/1.1 200 OK\r\n
//		③ Date: Fri, 08 Jul 2011 00:59:41 GMT\r\n
//		④ Server: CGCII/8.0.0\r\n
//		⑤ X-Powered-By: CGCII/8.0.0\r\n
//		⑥ Expires: Mon, 26 Jul 1997 05:00:00 GMT\r\n
//		⑦ Last-Modified: Fri, 08 Jul 2011 00:59:41 GMT\r\n
//		⑧ Cache-control: no-store, no-cache, must-revalidate\r\n
//		⑨ Content-Length: 102\r\n
//		⑩ Keep-Alive: timeout=15, max=100\r\n
//		⑪ Connection: Keep-Alive\r\n
//		⑫ Content-Type: text/html\r\n
//		⑬ \r\n
//
//
//-----------------------------------------------------------------
