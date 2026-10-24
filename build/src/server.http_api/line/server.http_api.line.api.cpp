#include "pch.h"

// - CGDK: enable rapidjson SSE4.2 SIMD on x86-64 (skip whitespace/scan strings with _mm_cmpestri)
#if (defined(_M_X64) || defined(__x86_64__)) && !defined(RAPIDJSON_SSE42) && !defined(RAPIDJSON_SSE2) && !defined(RAPIDJSON_NEON)
	#if defined(_MSC_VER) || defined(__SSE4_2__)
		#define RAPIDJSON_SSE42		// msvc compiles sse4.2 intrinsics without a flag; gcc needs -msse4.2 (__SSE4_2__)
	#else
		#define RAPIDJSON_SSE2		// sse2 is baseline on x86-64 (gcc enables it without a flag)
	#endif
#endif

#ifdef _MSC_VER
	#pragma warning(disable:6102)
	#pragma warning(disable:6313)
	#pragma warning(disable:6385)
	#pragma warning(disable:4996)
	#pragma warning(disable:5054)
	#pragma warning(disable:26495)
	#pragma warning(disable:33010)
#endif

#if !defined(__linux__)
	#pragma warning(disable:5054)
	//#pragma diag_suppress 177 // suppress #177-D function  was declared but never referenced
	#include "rapidjson/document.h"
	#include "rapidjson/writer.h"
	#include "rapidjson/stringbuffer.h"
	#include "rapidjson/filereadstream.h"
	#include "rapidjson/filewritestream.h"
	#pragma warning(default:5054)
#else
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
#endif

#ifdef _MSC_VER
	#pragma warning(default:6102)
	#pragma warning(default:6313)
	#pragma warning(default:6385)
	#pragma warning(default:4996)
	#pragma warning(default:5054)
	#pragma warning(default:26495)
	#pragma warning(default:33010)
#endif

namespace CGDK
{

// 1) LINE OAuth
http_api::line::RESPONSE_LINE_ACCESS_TOKEN http_api::line::request(const REQUEST_LINE_ACCESS_TOKEN& /*_request*/)
{
	//// ------------------------------------------------------------------
	//// 
	//// LINE 인증 - 코드 받기
	////
	//// 요청 메시지 URL
	////   
	////   2) HTTP URL		/v2/oauth/accessToken
	////	 3) HTTP Method		POST
	////	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded"
	////   5) Parameters
	////		- grant_type		(필수)	client_credentials
	////		- client_id			(필수)	Channel ID. Found on the console.
	////		- client_secret		(필수)	Channel secret. Found on the console.
	////
	////   요청)	GET /oauth/authorize?client_id={app_key}&redirect_uri={redirect_uri}&response_type=code HTTP/1.1 
	////			Host: kauth.kakao.com
	////
	////   결과)	HTTP / 1.1 302 Found
	////				Content - Length: 0
	////				Location : {redirect_uri} ? code = { authorize_code }
	////
	//// ------------------------------------------------------------------
	//// declare) 
	//const std::size_t SIZE_BUFFER = 8192;

	//// check) 
	//CGASSERT_ERROR(_request.str_url.empty() == false);

	//// check) 
	//THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	//// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	//auto prequest_http = make_object<query::http::query>();

	//// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	//auto str_object = "/v2/oauth/accessToken";

	//// 3) HTTP Method를 설정한다.
	//auto http_method = eHTTP_METHOD::POST_;

	//// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	//std::vector<std::string_view> vector_header{ "Content-Type:application/x-www-form-urlencoded" };

	//// 5) parameter를 설정한다.
	//auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//buf_option.append<text<char>>("client_id="); buf_option.append<text<char>>(_request.client_id);
	//buf_option.append<text<char>>("&grant_type=client_credentials");
	//buf_option.append<text<char>>("&client_secret=");	buf_option.append<text<char>>(_request.client_secret);
	//buf_option[buf_option.size()] = 0;

	//// declare)
	//char temp_length[256];
	//std::snprintf(temp_length, std::size(temp_length), std::string_view("Content-Length:%I64u").data(), buf_option.size());

	//// 6) 길이 정보를 써넣는다.
	//vector_header.push_back(temp_length);

	//// 7) Accept 받을 것을 설정.
	//std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };

	//// 8) request를 Set한다.
	//prequest_http->set_request(
	//	http_method,
	//	_request.str_url,
	//	443,
	//	"HTTP/1.1",
	//	str_object,
	//	std::string_view(),
	//	std::move(vector_header),
	//	buf_option,
	//	std::move(vector_accept),
	//	1		// 0: normal, 1:ssl
	//);

	// declare) 
	http_api::line::RESPONSE_LINE_ACCESS_TOKEN	response;

	//// 9) 기본값을 설정...
	//response.rsp_code = eRESPONSE_CODE::A0000_;
	//response.rsp_message.clear();

	//// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	//prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	//{
	//	// check) Result가 0이면 실패이므로 끝낸다.
	//	if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
	//	{
	//		// - 내부처리 중 Error을 명시
	//		response.rsp_code	 = eRESPONSE_CODE::FAIL_;
	//		response.rsp_message = _msg.status_message;

	//		// - buffer를 clear한다.
	//		response.buf_response.clear();

	//		// Return)
	//		return;
	//	}

	//	// - 마지막을 null을 붙인다.
	//	_msg.contents[0].append<char>(0);

	//	// - buffer를 넣는다.
	//	response.buf_response = _msg.contents[0];

	//	// declare) 
	//	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	//	// - Parsing한다.
	//	const auto&	result = json_data.Parse(_msg.contents[0].data());

	//	// check) Parsing에 실패했으면 false를 리턴한다.
	//	RETURN_IF(result.IsNull())

	//	// - 값을 설정한다.
	//	for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
	//	{
	//		auto str_name = (*iter).name.GetString();
	//		auto& value = (*iter).value;

	//		// - accessToken
	//		if (strcmp(str_name, "access_token")==0)
	//		{
	//			response.access_token = value.GetString();
	//		}
	//		// - expire_in
	//		else if(strcmp(str_name, "expires_in") == 0)
	//		{
	//			response.expire_in = value.GetInt64();
	//		}
	//		// - token_type
	//		else if (strcmp(str_name, "token_type") == 0)
	//		{
	//			response.token_type = value.GetString();
	//		}
	//		// - error
	//		else if (strcmp(str_name, "error") == 0)
	//		{
	//			response.rsp_code = eRESPONSE_CODE::FAIL_;
	//		}
	//		// - error_message
	//		else if (strcmp(str_name, "error_description") == 0)
	//		{
	//			response.rsp_message = value.GetString();
	//		}
	//	}
	//});

	//// 11) request를 요청한다.
	//prequest_http->request();

	//// 12) 완료를 기다린다.
	//prequest_http->wait();

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_REVOKE http_api::line::request(const REQUEST_LINE_REVOKE& /*_request*/)
{
	//// ------------------------------------------------------------------
	//// 
	//// LINE 인증 - Revoke channel access token
	////
	//// 요청 메시지 URL
	////   
	////   2) HTTP URL		https://api.line.me/v2/oauth/revoke
	////	 3) HTTP Method		POST
	////	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
	////   5) Parameters
	////		- access_token		(필수)	Channel access token
	////
	////	요청)	POST https://api.line.me/v2/oauth/revoke
	////
	//// ------------------------------------------------------------------
	//// declare) 
	//const std::size_t SIZE_BUFFER = 8192;

	//// check) 
	//CGASSERT_ERROR(_request.str_url.empty() == false);

	//// check) 
	//THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	//// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	//auto prequest_http = make_object<query::http::query>();

	//// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	//auto str_object = "/v2/oauth/revoke";

	//// 3) HTTP Method를 설정한다.
	//auto http_method = eHTTP_METHOD::POST_;

	//// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	//std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded" };

	//// 5) parameter를 설정한다.
	//auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//buf_option.append<text<char>>("access_token="); buf_option.append<text<char>>(_request.access_token);
	//buf_option[buf_option.size()] = 0;

	//// declare)
	//char temp_length[256];
	//std::snprintf(temp_length, std::size(temp_length), std::string_view("Content-Length:%I64u").data(), buf_option.size());

	//// 6) 길이 정보를 써넣는다.
	//vector_header.push_back(temp_length);

	//// 7) Accept 받을 것을 설정.
	//std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };

	//// 8) request를 Set한다.
	//prequest_http->set_request(
	//	http_method,
	//	_request.str_url,
	//	443,
	//	//800,
	//	"HTTP/1.1",
	//	str_object,
	//	std::string_view(),
	//	std::move(vector_header),
	//	buf_option,
	//	std::move(vector_accept),
	//	1		// 0: normal, 1:ssl
	//);

	// declare) 
	http_api::line::RESPONSE_LINE_REVOKE response;

	//// 9) 기본값을 설정...
	//response.rsp_code = eRESPONSE_CODE::A0000_;
	//response.rsp_message.clear();

	//// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	//prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	//{
	//	// check) Result가 0이면 실패이므로 끝낸다.
	//	if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
	//	{
	//		// - 내부처리 중 Error을 명시
	//		response.rsp_code = eRESPONSE_CODE::FAIL_;
	//		response.rsp_message = _msg.status_message;

	//		// - buffer를 clear한다.
	//		response.buf_response.clear();

	//		// Return)
	//		return;
	//	}

	//	// - 마지막을 null로 바꾼다.
	//	_msg.contents[0].append<char>(0);

	//	// - buffer를 넣는다.
	//	response.buf_response = _msg.contents[0];

	//	// declare) 
	//	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	//	// - Parsing한다.
	//	const auto&	result = json_data.Parse(_msg.contents[0].data());

	//	// check) Parsing에 실패했으면 false를 리턴한다.
	//	RETURN_IF(result.IsNull())

	//	// - 값을 설정한다.
	//	for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
	//	{
	//		auto str_name = (*iter).name.GetString();
	//		auto& value = (*iter).value;

	//		// - error
	//		if (strcmp(str_name, "error") == 0)
	//		{
	//			response.rsp_code = eRESPONSE_CODE::FAIL_;
	//		}
	//		// - error_message
	//		else if (strcmp(str_name, "error_description") == 0)
	//		{
	//			response.rsp_message = value.GetString();
	//		}
	//	}
	//});

	//// 11) request를 요청한다.
	//prequest_http->request();

	//// 12) 완료를 기다린다.
	//prequest_http->wait();

	// return) 
	return response;
}

// 2) LINE Message
http_api::line::RESPONSE_LINE_MESSAGE_REPLY http_api::line::request(const REQUEST_LINE_MESSAGE_REPLY& /*_request*/)
{
	//// ------------------------------------------------------------------
	//// 
	//// LINE 인증 - 로그아웃
	////
	//// 요청 메시지 URL
	////   
	////   2) HTTP URL		https://api.line.me/v2/bot/message/reply
	////	 3) HTTP Method		POST
	////	 4) Content-Type	"Content-Type: application/json"
	////   5) Parameters
	////		- Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxx
	////
	////	요청)	POST https://api.line.me/v2/bot/message/reply
	////
	//// ------------------------------------------------------------------
	//// declare) 
	//const std::size_t SIZE_BUFFER = 8192;

	//// check) 
	//CGASSERT_ERROR(_request.str_url.empty() == false);

	//// check) 
	//THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	//// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	//auto prequest_http = make_object<query::http::query>();

	//// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	//auto str_object = "/v2/bot/message/reply";

	//// 3) HTTP Method를 설정한다.
	//auto http_method = eHTTP_METHOD::POST_;

	//// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	//std::vector<std::string_view> vector_header{ "Content-Type: application/json" };

	//// 5) Header에 Authorization을 추가한다.
	//auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//std::snprintf(buf_header.data<char>(), SIZE_BUFFER, std::string_view("Authorization: Bearer %s").data(), _request.access_token.c_str());
	//vector_header.push_back(buf_header.data());

	//// 6) parameter를 설정한다.
	//auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//buf_option.append<text<char>>("replyToken=");	buf_option.append<text<char>>(_request.replyToken);
	//buf_option.append<text<char>>("&messages=");	buf_option.append<text<char>>(_request.messages.front());
	//buf_option[buf_option.size()] = 0;

	//// declare)
	//char temp_length[256];
	//std::snprintf(temp_length, std::size(temp_length), std::string_view("Content-Length:%I64u").data(), buf_option.size());

	//// 7) 길이 정보를 써넣는다.
	//vector_header.push_back(temp_length);

	//// 8) Accept 받을 것을 설정.
	//std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };

	//// 9) request를 Set한다.
	//prequest_http->set_request(
	//	http_method,
	//	_request.str_url,
	//	443,
	//	//800,
	//	"HTTP/1.1",
	//	str_object,
	//	std::string_view(),
	//	std::move(vector_header),
	//	buf_option,
	//	std::move(vector_accept),
	//	1		// 0: normal, 1:ssl
	//);

	// declare) 
	http_api::line::RESPONSE_LINE_MESSAGE_REPLY response;

	//// 10) 기본값을 설정...
	//response.rsp_code = eRESPONSE_CODE::A0000_;
	//response.rsp_message.clear();

	//// 11) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	//prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	//{
	//	// check) Result가 0이면 실패이므로 끝낸다.
	//	if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
	//	{
	//		// - 내부처리 중 Error을 명시
	//		response.rsp_code	 = eRESPONSE_CODE::FAIL_;
	//		response.rsp_message = _msg.status_message;

	//		// - buffer를 clear한다.
	//		response.buf_response.clear();

	//		// Return)
	//		return;
	//	}

	//	// - 마지막을 null로 바꾼다.
	//	_msg.contents[0].append<char>(0);

	//	// - buffer를 넣는다.
	//	response.buf_response = _msg.contents[0];

	//	// declare) 
	//	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	//	// - Parsing한다.
	//	const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

	//	// check) Parsing에 실패했으면 false를 리턴한다.
	//	RETURN_IF(result.IsNull())

	//	// - 값을 설정한다.
	//	for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
	//	{
	//		auto str_name = (*iter).name.GetString();
	//		auto& value = (*iter).value;

	//		// - error
	//		if (strcmp(str_name, "error") == 0)
	//		{
	//			response.rsp_code = eRESPONSE_CODE::FAIL_;
	//		}
	//		// - error_message
	//		else if (strcmp(str_name, "error_description") == 0)
	//		{
	//			response.rsp_message = value.GetString();
	//		}
	//	}
	//});

	//// 12) request를 요청한다.
	//prequest_http->request();

	//// 13) 완료를 기다린다.
	//prequest_http->wait();

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_MESSAGE_PUSH http_api::line::request(const REQUEST_LINE_MESSAGE_PUSH& /*_request*/)
{
	//// ------------------------------------------------------------------
	//// 
	//// LINE 인증 - 앱연결
	////
	//// 요청 메시지 URL
	////   
	////   2) HTTP URL		https://api.line.me/v2/bot/message/push
	////	 3) HTTP Method		POST
	////	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
	////   5) Parameters
	////		- Authorization: Bearer {access_token}
	////	    - Content-Type: application/x-www-form-urlencoded;charset=utf-8
	////
	////	요청)	curl - v - X POST https ://kapi.kakao.com/v1/user/signup 
	////				-H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" 
	////				--data-urlencode 'properties={"age":"23", "gender":"female"}'
	////
	////	결과)	HTTP / 1.1 200 OK
	////			Content-Type: application/json;charset=UTF-8
	////			{
	////				"id":123456789
	////			}
	////
	//// ------------------------------------------------------------------
	//// check) messages가 하나도 없다면 그냥 끝낸다.
	//RETURN_IF(_request.messages.empty(), http_api::line::RESPONSE_LINE_MESSAGE_PUSH());

	//// declare) 
	//const std::size_t SIZE_BUFFER = 8192;

	//// check) 
	//CGASSERT_ERROR(_request.str_url.empty() == false);

	//// check) 
	//THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	//// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	//auto prequest_http = make_object<query::http::query>();

	//// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	//auto str_object = "/v2/bot/message/push";

	//// 3) HTTP Method를 설정한다.
	//auto http_method = eHTTP_METHOD::POST_;

	//// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	//std::vector<std::string_view> vector_header{ "Content-Type: application/json" };

	//// 5) Header에 Authorization을 추가한다.
	//auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//std::snprintf(buf_header.data(), SIZE_BUFFER, std::string_view("Authorization: Bearer %s").data(), _request.access_token.c_str());
	//vector_header.push_back(buf_header.data());

	//// 7) parameter를 설정한다.
	//auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	//buf_option.append<text<char>>("{\"to\":\"");			buf_option.append<text<char>>(_request.to);
	//buf_option.append<text<char>>("\", \"messages\":[");
	//buf_option.append(_request.messages.size() - 1, _request.messages.data());	// NULL을 제거하기 위해 message.size_에서 -1을 해준다.
	//buf_option.append<text<char>>("]}");
	//buf_option[buf_option.size()] = 0;

	//// declare)
	//char temp_length[256];
	//std::snprintf(temp_length, std::size(temp_length), std::string_view("Content-Length:%I64u").data(), buf_option.size());

	//// 8) 길이 정보를 써넣는다.
	//vector_header.push_back(temp_length);

	//// 9) Accept 받을 것을 설정.
	//std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };

	//// 10) request를 Set한다.
	//prequest_http->set_request(
	//	http_method,
	//	_request.str_url,
	//	443,
	//	//800,
	//	"HTTP/1.1",
	//	str_object,
	//	std::string_view(),
	//	std::move(vector_header),
	//	buf_option,
	//	std::move(vector_accept),
	//	1		// 0: normal, 1:ssl
	//);

	// declare) 
	http_api::line::RESPONSE_LINE_MESSAGE_PUSH	response;

	//// 11) 기본값을 설정...
	//response.rsp_code = eRESPONSE_CODE::A0000_;
	//response.rsp_message.clear();

	//// 12) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	//prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	//{
	//	// check) Result가 0이면 실패이므로 끝낸다.
	//	if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
	//	{
	//		// - 내부처리 중 Error을 명시
	//		response.rsp_code	 = eRESPONSE_CODE::FAIL_;
	//		response.rsp_message = _msg.status_message;

	//		// - buffer를 clear한다.
	//		response.buf_response.clear();

	//		// Return)
	//		return;
	//	}

	//	// - 마지막을 null로 바꾼다.
	//	_msg.contents[0].append<char>(0);

	//	// - buffer를 넣는다.
	//	response.buf_response = _msg.contents[0];

	//	// declare) 
	//	rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

	//	// - Parsing한다.
	//	const auto&	result = json_data.Parse(_msg.contents[0].data());

	//	// check) Parsing에 실패했으면 false를 리턴한다.
	//	RETURN_IF(result.IsNull())

	//	// - 값을 설정한다.
	//	for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
	//	{
	//		auto str_name = (*iter).name.GetString();
	//		auto& value = (*iter).value;

	//		// - error
	//		if (strcmp(str_name, "error") == 0)
	//		{
	//			response.rsp_code = eRESPONSE_CODE::FAIL_;
	//		}
	//		// - error_message
	//		else if (strcmp(str_name, "error_description") == 0)
	//		{
	//			response.rsp_message = value.GetString();
	//		}
	//	}
	//});

	//// 13) request를 요청한다.
	//prequest_http->request();

	//// 14) 완료를 기다린다.
	//prequest_http->wait();

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_MESSAGE_MULTICAST http_api::line::request(const REQUEST_LINE_MESSAGE_MULTICAST& /*_request*/)
{
	// ------------------------------------------------------------------
	// 
	// LINE 인증 - 앱연결해제
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://kapi.kakao.com/v1/user/unlink HTTP/1.1
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded;charset=utf-8"
	//   5) Parameters
	//		- Authorization: KakaoAK {admin_key}
	//	    - Content-Type: application/x-www-form-urlencoded;charset=utf-8
	//
	//   6) ...
	//			POST /v1/user/unlink HTTP/1.1
	//			Host: kapi.kakao.com
	//			Authorization : Bearer{ access_token }
	//
	//			POST /v1/user/unlink HTTP/1.1
	//			Host: kapi.kakao.com
	//			Authorization: KakaoAK {admin_key}
	//
	//
	//	요청1)	curl -v -X POST https://kapi.kakao.com/v1/user/unlink 
	//			  -H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	//
	//	요청)	curl -v -X POST https://kapi.kakao.com/v1/user/unlink 
	//			  -H "Authorization: KakaoAK kkkkkkkkkkkkkkkkkkkkkkkkkkkkkk" 
	//			  -d 'target_id_type=user_id&target_id=123456789'
	//
	//	결과)	HTTP/1.1 200 OK
	//			Content-Type: application/json;charset=UTF-8
	//			{
	//			    "id":123456789
	//			}
	//
	// ------------------------------------------------------------------
	// declare) 
	http_api::line::RESPONSE_LINE_MESSAGE_MULTICAST	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_GET_CONTENTS http_api::line::request(const REQUEST_LINE_GET_CONTENTS& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_GET_CONTENTS	response;

	// return) 
	return response;
}

// 3) LINE Profile
http_api::line::RESPONSE_LINE_PROFILE http_api::line::request(const REQUEST_LINE_PROFILE& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_PROFILE	response;

	// return) 
	return response;
}

// 4) LINE Group
http_api::line::RESPONSE_LINE_GROUP_MEMBER http_api::line::request(const REQUEST_LINE_GROUP_MEMBER& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_GROUP_MEMBER	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_GROUP_MEMBER_IDS http_api::line::request(const REQUEST_LINE_GROUP_MEMBER_IDS& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_GROUP_MEMBER_IDS	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_GROUP_LEAVE http_api::line::request(const REQUEST_LINE_GROUP_LEAVE& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_GROUP_LEAVE	response;

	// return) 
	return response;
}

// 5) LINE Room
http_api::line::RESPONSE_LINE_ROOM_MEMBER http_api::line::request(const REQUEST_LINE_ROOM_MEMBER& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_ROOM_MEMBER	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_ROOM_MEMBER_IDS http_api::line::request(const REQUEST_LINE_ROOM_MEMBER_IDS& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_ROOM_MEMBER_IDS	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_ROOM_LEAVE http_api::line::request(const REQUEST_LINE_ROOM_LEAVE& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_ROOM_LEAVE	response;

	// return) 
	return response;
}

// 6) LINE Rich menu
http_api::line::RESPONSE_LINE_RICHMENU_GET http_api::line::request(const REQUEST_LINE_RICHMENU_GET& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_GET	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_CREATE http_api::line::request(const REQUEST_LINE_RICHMENU_CREATE& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_CREATE	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_DELETE http_api::line::request(const REQUEST_LINE_RICHMENU_DELETE& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_DELETE	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_USER http_api::line::request(const REQUEST_LINE_RICHMENU_USER& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_USER	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_USER_LINK http_api::line::request(const REQUEST_LINE_RICHMENU_USER_LINK& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_USER_LINK	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_USER_UNLINK http_api::line::request(const REQUEST_LINE_RICHMENU_USER_UNLINK& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_USER_UNLINK	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_CONTENT_DOWNLOAD http_api::line::request(const REQUEST_LINE_RICHMENU_CONTENT_DOWNLOAD& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_CONTENT_DOWNLOAD	response;

	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_CONTENT_UPLOAD http_api::line::request(const REQUEST_LINE_RICHMENU_CONTENT_UPLOAD& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_CONTENT_UPLOAD	response;
	// return) 
	return response;
}
http_api::line::RESPONSE_LINE_RICHMENU_LIST http_api::line::request(const REQUEST_LINE_RICHMENU_LIST& /*_request*/)
{
	// declare) 
	http_api::line::RESPONSE_LINE_RICHMENU_LIST	response;

	// return) 
	return response;
}


}