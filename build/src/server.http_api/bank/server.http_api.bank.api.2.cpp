#include "pch.h"

#if !defined(__linux__)
	#pragma warning (disable:5054)
	#pragma warning (disable:6340)
	#pragma warning (disable:6313)
	//#pragma diag_suppress 177 // supress #177-D function  was declared but never referenced
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
	#pragma warning (default:6340)
	#pragma warning (default:6313)
	#pragma warning (default:5054)
#else
	#include "../../common/rapidjson/document.h"
	#include "../../common/rapidjson/writer.h"
	#include "../../common/rapidjson/stringbuffer.h"
	#include "../../common/rapidjson/filereadstream.h"
	#include "../../common/rapidjson/filewritestream.h"
#endif

namespace CGDK
{

// functions) 
chrono::date_time from_string_date_time_millsec(std::string_view _string);
chrono::date_time from_string_date_time(std::string_view _string);
chrono::date_time from_string_date(std::string_view _string);
chrono::date_time from_string_time(std::string_view _string);

// 1) OAuth 인증 및 계좌등록
http_api::bank::RESPONSE_AUTHORIZE http_api::bank::request(const REQUEST_AUTHORIZE& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 사용자로그인연결 요청(3-legged): 구버전
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/authorize
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- response_type			고정값: code			OAuth 2.0 인정 요청시 반환되는 형태
	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
	//		- scope					고정값:login			Access Token 권한 범위
	//								(login | inquiry | transfer)
	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/oauth/2.0/authorize?response_type=code&client_id=%s&redirect_uri=%s&scope=login&client_info=%s"sv, _request.client_id.c_str(), _request.redirect_uri.c_str(), _request.client_info.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// 4) 
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// declare) 
	shared_buffer buf_option;

	// 5) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 6) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE	response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE2_FIRST http_api::bank::request(const REQUEST_AUTHORIZE2_FIRST& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 사용자로그인연결 요청(3-legged): 구버전
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/oauth/2.0/authorize2
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- response_type				고정값: code			OAuth 2.0 인정 요청시 반환되는 형태
	//		- client_id					<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
	//		- redirect_uri				<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
	//		- scope						고정값:login			Access Token 권한 범위
	//									(login | inquiry | transfer)
	//		- client_info				<client_info>			이용기관이 셋팅하는 임의의 정보
	//									(Max: 256bytes)			Callback 호출 시 그대로 전달됨
	//		- auth_type					"0"(선택)				사용자인증타입구분
	//															(0:최초인증, 1:재인증, 2:인증생략)
	//		- bg_color					#000000(선택)			Provider Page 배경생상
	//		- txt_color					#000000(선택)			Provider Page 텍스트생상
	//		- btn1_color				#000000(선택)			Provider Page 버튼1 생상
	//		- btn2_color				#000000(선택)			Provider Page 버튼2 생상
	//
	//		- Kftc-Bfop-UserSeqNo		<user_seq_no>(선택)		단 user_ci가 있을 경우 필수입력
	//		- Kftc-Bfop-UserCI			<user_ci>(선택)			CI(Connect Info)
	//		- Kftc-Bfop-UserName		<user_name>(선택)
	//		- Kftc-Bfop-UserInfo		<user_info>(선택)		생년월일 8자리
	//		- Kftc-Bfop-UserCellNo		<user_cell_no>(선택)
	//		- Kftc-Bfop-UserUserEmail	<user_email>(선택)
	//		- Kftc-Bfop-BankCodeStd		<user_bank_code_std>(선택) 고정된 계좌번호 등록필요 시 입력
	//		- Kftc-Bfop-AccountNum		<user_account_num>(선택)
	//
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/oauth/2.0/authorize2?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&client_info=%s&auth_type=0&bg_color=#000000&txt_color=#000000&btn1_color=#000000&btn2_color=#000000"sv, _request.client_id.c_str(), _request.redirect_uri.c_str(), to_string<char>(_request.scope).data(), _request.client_info.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// declare) 
	shared_buffer buf_option;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE2_FIRST	response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE2_REAUTH http_api::bank::request(const REQUEST_AUTHORIZE2_REAUTH& /*_request*/)
{
	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE2_REAUTH	response;

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE2_AUTH_BYPASS http_api::bank::request(const REQUEST_AUTHORIZE2_AUTH_BYPASS& /*_request*/)
{
	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE2_AUTH_BYPASS	response;

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TOKEN http_api::bank::request(const REQUEST_TOKEN& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 사용자로그인연결 요청(3-legged)
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/oauth/2.0/token
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- code						<authorization_code>		사용자인증 성공 후 전송되는 authorization_code
	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
	//		- redirect_uri				<callback_uri>				Access Token을 전달받을 callback_uri
	//		- grant_type				고정값:authorization_code	Access Token을 허용하기 위한 값
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto str_object = "/oauth/2.0/token"sv;

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("code="sv);				buf_option.append<TEXT_WEB>(_request.code);
	buf_option.append<TEXT_WEB>("&client_id="sv);		buf_option.append<TEXT_WEB>(_request.client_id);
	buf_option.append<TEXT_WEB>("&client_secret="sv);	buf_option.append<TEXT_WEB>(_request.client_secret);
	buf_option.append<TEXT_WEB>("&redirect_uri="sv);	buf_option.append<TEXT_WEB>(_request.redirect_uri);
	buf_option.append<TEXT_WEB>("&grant_type=authorization_code"sv);
	buf_option[buf_option.size()] = 0;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view>	vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		//800,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TOKEN	response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parse한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - "rsp_code"
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - "rsp_message"
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "access_token"
			else if(__strcmp(str_name, "access_token"sv) == 0)
			{
				response.access_token = value.GetString();
			}
			// - "token_type"
			else if (__strcmp(str_name, "token_type"sv) == 0)
			{
				response.token_type = get_enum_OAUTH_TOKEN_TYPE<char>(value.GetString());
			}
			// - "expires_in"
			else if (__strcmp(str_name, "expires_in"sv) == 0)
			{
				response.expires_in = value.GetInt64();
			}
			// - "refresh_token"
			else if (__strcmp(str_name, "refresh_token"sv) == 0)
			{
				response.refresh_token = value.GetString();
			}
			// - "scope"
			else if (__strcmp(str_name, "scope"sv) == 0)
			{
				response.scope = get_enum_OAUTH_SCOPE<char>(value.GetString());
			}
			// - "user_seq_no"
			else if (__strcmp(str_name, "user_seq_no"sv) == 0)
			{
				response.user_seq_no = value.GetString();
			}

			// "user_ci"
			else if (__strcmp(str_name, "user_ci"sv) == 0)
			{
				response.user_ci = value.GetString();
			}
			// "user_name"
			else if (__strcmp(str_name, "user_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.user_name, value.GetString());
			}
			// "user_info"
			else if (__strcmp(str_name, "user_info"sv) == 0)
			{
				response.user_info = value.GetString();
			}
			// "user_gender"
			else if (__strcmp(str_name, "user_gender"sv) == 0)
			{
				response.user_gender = get_enum_OAUTH_GENDER<char>(value.GetString());
			}
			// "user_cell_no"
			else if (__strcmp(str_name, "user_cell_no"sv) == 0)
			{
				response.user_cell_no = value.GetString();
			}
			// "user_email"
			else if (__strcmp(str_name, "user_email"sv) == 0)
			{
				response.user_email = value.GetString();
			}
		}
	});

	// 5) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_TOKEN2 http_api::bank::request(const REQUEST_TOKEN2& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 이용기관 Access Token 요청 (2-legged)
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/oauth/2.0/token
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
	//		- grant_type				고정값:client_credentials	2-legged 인증을 위한 인증방식 지정
	//		- scope						고정값:oob					Access Token 권한 범위
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto str_object = "/oauth/2.0/token"sv;

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("client_id=");		buf_option.append<TEXT_WEB>(_request.client_id);
	buf_option.append<TEXT_WEB>("&client_secret=");	buf_option.append<TEXT_WEB>(_request.client_secret);
	buf_option.append<TEXT_WEB>("&grant_type=client_credentials");
	buf_option.append<TEXT_WEB>("&scope=oob");
	buf_option.front<char>(buf_option.size()) = 0;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		//800,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TOKEN2 response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - shared_buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// 1) Parse한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// 2) 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "access_token"
			else if(__strcmp(str_name, "access_token"sv) == 0)
			{
				response.access_token = value.GetString();
			}
			// - "token_type"
			else if (__strcmp(str_name, "token_type"sv) == 0)
			{
				response.token_type = get_enum_OAUTH_TOKEN_TYPE<char>(value.GetString());
			}
			// - "expires_in"
			else if (__strcmp(str_name, "expires_in"sv) == 0)
			{
				response.expires_in = value.GetInt64();
			}
			// - "scope"
			else if (__strcmp(str_name, "scope"sv) == 0)
			{
				response.scope = get_enum_OAUTH_SCOPE<char>(value.GetString());
			}
			// - "client_use_code"
			else if (__strcmp(str_name, "client_use_code"sv) == 0)
			{
				response.client_use_code = value.GetString();
			}
		}
	});

	// 5) request를 요청한다.
	prequest_http->request();

	// return) 
	return response;
}
http_api::bank::RESPONSE_TOKEN_REFRESH http_api::bank::request(const REQUEST_TOKEN_REFRESH& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - Login Access Token 갱신(refresh_token 사용)
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/oauth/2.0/token
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
	//		- refresh_token				<refresh_token>				login 토큰 발급 시 받은 access_token 갱신용 token
	//		- scope						고정값						Access Token 권한 범위
	//									[login|inquiry|transfer]
	//		- grant_type				고정값:refresh_token		Access Token을 허용하기 위한 값
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto str_object = "/oauth/2.0/token"sv;

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("client_id="sv);		buf_option.append<TEXT_WEB>(_request.client_id);
	buf_option.append<TEXT_WEB>("&client_secret="sv);	buf_option.append<TEXT_WEB>(_request.client_secret);
	buf_option.append<TEXT_WEB>("&refresh_token="sv);	buf_option.append<TEXT_WEB>(_request.refresh_token);
	buf_option.append<TEXT_WEB>("&scope="sv);			buf_option.append<TEXT_WEB>(to_string<char>(_request.scope));
	buf_option.append<TEXT_WEB>("&grant_type=refresh_token"sv);
	buf_option.front<char>(buf_option.size()) = 0;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		//800,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_TOKEN_REFRESH	response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// 1) Parse한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// 2) 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "access_token"
			else if (__strcmp(str_name, "access_token"sv) == 0)
			{
				response.access_token = value.GetString();
			}
			// - "token_type"
			else if (__strcmp(str_name, "token_type"sv) == 0)
			{
				response.token_type	= get_enum_OAUTH_TOKEN_TYPE<char>(value.GetString());
			}
			// - "expires_in"
			else if (__strcmp(str_name, "expires_in"sv) == 0)
			{
				response.expires_in = value.GetInt64();
			}
			// - "scope"
			else if (__strcmp(str_name, "scope"sv) == 0)
			{
				response.scope = get_enum_OAUTH_SCOPE<char>(value.GetString());
			}
			// - "user_seq_no"
			else if (__strcmp(str_name, "user_seq_no"sv) == 0)
			{
				response.user_seq_no = value.GetString();
			}
		}
	});

	// 5) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}

// 2) OAuth ...
http_api::bank::RESPONSE_USER_ME http_api::bank::request(const REQUEST_USER_ME& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/user/me
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- user_seq_no			"U123456789"			AN(10) 사용자일련번호
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/v1.0/user/me?user_seq_no=%s"sv, _request.user_seq_no.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
	size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 5) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type:application/x-www-form-urlencoded;charset=UTF-8"sv, buf_header.data<char>() };

	// declare) 
	shared_buffer buf_option;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_USER_ME response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]] size_t	res_cnt	 = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}

			// - "user_seq_no"
			else if (__strcmp(str_name, "user_seq_no"sv) == 0)
			{
				response.user_seq_no = value.GetString();
			}
			// - "user_ci"
			else if (__strcmp(str_name, "user_ci"sv) == 0)
			{
				response.user_ci = value.GetString();
			}
			// - "user_name"
			else if (__strcmp(str_name, "user_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.user_name, value.GetString());
			}
			// - "res_cnt"
			else if (__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt	 = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_USER_ME::ACCOUNT_INFO	AccouuntInfo;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto strName_Nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "fintech_use_num"
						if (__strcmp(strName_Nested, "fintech_use_num"sv) == 0)
						{
							AccouuntInfo.fintech_use_num = value_Nested.GetString();
						}
						// - "account_alias"
						else if (__strcmp(strName_Nested, "account_alias"sv) == 0)
						{
							convert_utf8_to_multibyte(AccouuntInfo.account_alias, value_Nested.GetString());
						}
						// - "bank_code_std"
						else if (__strcmp(strName_Nested, "bank_code_std"sv) == 0)
						{
							AccouuntInfo.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_code_sub"
						else if (__strcmp(strName_Nested, "bank_code_sub"sv) == 0)
						{
							AccouuntInfo.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "bank_name"
						else if (__strcmp(strName_Nested, "bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(AccouuntInfo.bank_name, value_Nested.GetString());
						}
						// - "account_num_masked"
						else if (__strcmp(strName_Nested, "account_num_masked"sv) == 0)
						{
							AccouuntInfo.account_num_masked = value_Nested.GetString();
						}
						// - "account_holder_name"
						else if (__strcmp(strName_Nested, "account_holder_name"sv) == 0)
						{
							convert_utf8_to_multibyte(AccouuntInfo.account_holder_name, value_Nested.GetString());
						}
						// - "account_type"
						else if (__strcmp(strName_Nested, "account_type"sv) == 0)
						{
							AccouuntInfo.account_type = value_Nested.GetString();
						}
						// - "inquiry_agree_yn"
						else if (__strcmp(strName_Nested, "inquiry_agree_yn"sv) == 0)
						{
							AccouuntInfo.inquiry_agree_yn = get_enum_OAUTH_YESNO<char>(value_Nested.GetString());
						}
						// - "inquiry_agree_dtime"
						else if (__strcmp(strName_Nested, "inquiry_agree_dtime"sv) == 0)
						{
							AccouuntInfo.inquiry_agree_dtime = from_string_date_time(value_Nested.GetString());
						}
						// - "transfer_agree_yn"
						else if (__strcmp(strName_Nested, "transfer_agree_yn"sv) == 0)
						{
							AccouuntInfo.transfer_agree_yn = get_enum_OAUTH_YESNO<char>(value_Nested.GetString());
						}
						// - "transfer_agree_dtime"
						else if (__strcmp(strName_Nested, "transfer_agree_dtime"sv) == 0)
						{
							AccouuntInfo.transfer_agree_dtime = from_string_date_time(value_Nested.GetString());
						}
					}

					response.res_list.push_back(AccouuntInfo);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_UNLINK http_api::bank::request(const REQUEST_UNLINK& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/unlink
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
	//   5) Header
	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- client_user_code		"F001234560"			AN(10) 이용기관코드
	//		- user_seq_no			"U123456789"			AN(10) 사용자일련번호
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/oauth/2.0/unlink"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("{client_use_code: "sv);	buf_option.append<TEXT_WEB>(_request.client_use_code);
	buf_option.append<TEXT_WEB>(", user_seq_no: "sv);		buf_option.append<TEXT_WEB>(_request.user_seq_no);
	buf_option.append<TEXT_WEB>("}");
	buf_option[buf_option.size()] = 0;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_UNLINK response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "user_seq_no"
			else if (__strcmp(str_name, "user_seq_no"sv) == 0)
			{
				response.user_seq_no = value.GetString();
			}
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_REGISTER_ACCOUNT http_api::bank::request(const REQUEST_REGISTER_ACCOUNT& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 사용자로그인연결 요청(3-legged): 구버전
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/authorize
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- response_type			고정값: code			OAuth 2.0 인정 요청시 반환되는 형태
	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
	//		- scope					고정값:login			Access Token 권한 범위
	//								(login | inquiry | transfer)
	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/oauth/2.0/register_account?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&client_info=%s"sv, _request.client_id.c_str(), _request.redirect_uri.c_str(), to_string<char>(_request.scope).data(), _request.client_info.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// declare) 
	shared_buffer buf_option;

	// 5) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };

	// 6) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_REGISTER_ACCOUNT response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT http_api::bank::request(const REQUEST_AUTHORIZE_ACCOUNT& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 계좌등록확인 요청 (3-legged): 구버전
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/authorize_account
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- response_type			고정값: code			OAuth 2.0 인정 요청시 반환되는 형태
	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
	//		- scope					[inquiry | transfer]	Access Token 권한 범위
	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check)
	CGASSERT_ERROR(_request.scope == eOAUTH_SCOPE::INQUIRY_ || _request.scope == eOAUTH_SCOPE::TRANSFER_)

	// check)
	THROW_IF(_request.scope != eOAUTH_SCOPE::INQUIRY_ && _request.scope != eOAUTH_SCOPE::TRANSFER_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/oauth/2.0/authorize_account?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&client_info=%s"sv, _request.client_id.c_str(), _request.redirect_uri.c_str(), to_string<char>(_request.scope).data(), _request.client_info.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv };

	// declare) 
	shared_buffer buf_option;

	// 5) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 6) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT	response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		}
	});

	// 9) request를 요청한다.
	prequest_http->request();

	// return) 
	return response;
}

http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_FIRST http_api::bank::request(const REQUEST_AUTHORIZE_ACCOUNT2_FIRST& /*_request*/)
{
	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_FIRST	response;

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_REAUTH http_api::bank::request(const REQUEST_AUTHORIZE_ACCOUNT2_REAUTH& /*_request*/)
{
	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_REAUTH	response;

	// return) 
	return	response;
}
http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_BYPASS http_api::bank::request(const REQUEST_AUTHORIZE_ACCOUNT2_BYPASS& /*_request*/)
{
	// declare) 
	http_api::bank::RESPONSE_AUTHORIZE_ACCOUNT2_BYPASS	response;

	// return) 
	return	response;
}

// 3) OAuth ...
http_api::bank::RESPONSE_ACCOUNT_LIST http_api::bank::request(const REQUEST_ACCOUNT_LIST& _request)
{
	// ------------------------------------------------------------------
	// 
	// OAuth 인증 - 계좌등록확인 요청 (3-legged): 구버전
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://openapi.open.openform.or.kr/v1.0/account/list
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- response_type			고정값: code			OAuth 2.0 인정 요청시 반환되는 형태
	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
	//		- scope					[inquiry | transfer]	Access Token 권한 범위
	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
	//
	// ------------------------------------------------------------------
	// declare) 
	const size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/v1.0/account/list?user_seq_no=%s&include_cancel_yn=%s&sort_order=%s"sv, _request.user_seq_no.c_str(), to_string<char>(_request.include_cancel_yn).data(), to_string<char>(_request.sort_order).data());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
	size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 5) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// declare) 
	shared_buffer buf_option;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_ACCOUNT_LIST	response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// declare)
		[[maybe_unused]]std::size_t res_cnt = 0;

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "user_name"
			else if (__strcmp(str_name, "user_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.user_name, value.GetString());
			}
			// - "res_cnt"
			else if (__strcmp(str_name, "res_cnt"sv) == 0)
			{
				res_cnt	 = __from_string<size_t>(value.GetString());
			}
			// - "res_list"
			else if (__strcmp(str_name, "res_list"sv) == 0)
			{
				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
				{
					// declare) 
					http_api::bank::RESPONSE_ACCOUNT_LIST::ACCOUNT_INFO	AccouuntInfo;

					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
					{
						auto strName_Nested = iterNest->name.GetString();
						auto& value_Nested = iterNest->value;

						// - "fintech_use_num"
						if (__strcmp(strName_Nested, "fintech_use_num"sv) == 0)
						{
							AccouuntInfo.fintech_use_num = value_Nested.GetString();
						}
						// - "account_alias"
						else if (__strcmp(strName_Nested, "account_alias"sv) == 0)
						{
							AccouuntInfo.account_alias = value_Nested.GetString();
						}
						// - "bank_code_std"
						else if (__strcmp(strName_Nested, "bank_code_std"sv) == 0)
						{
							AccouuntInfo.bank_code_std = get_enum_OAUTH_BANK_CODE<char>(value_Nested.GetString());
						}
						// - "bank_code_sub"
						else if (__strcmp(strName_Nested, "bank_code_sub"sv) == 0)
						{
							AccouuntInfo.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
						}
						// - "bank_name"
						else if (__strcmp(strName_Nested, "bank_name"sv) == 0)
						{
							convert_utf8_to_multibyte(AccouuntInfo.bank_name, value_Nested.GetString());
						}
						// - "account_num_masked"
						else if (__strcmp(strName_Nested, "account_num_masked"sv) == 0)
						{
							AccouuntInfo.account_num_masked	 = value_Nested.GetString();
						}
					}

					response.res_list.push_back(AccouuntInfo);
				}

				// check) 
				CGASSERT_ERROR(response.res_list.size() == res_cnt);
			}
		}
	});

	// 10) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_ACCOUNT_UPDATE_INFO http_api::bank::request(const REQUEST_ACCOUNT_UPDATE_INFO& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/account/update_info
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- fintech_use_num		<fintech_use_num>		AN(24) 핀테크이용번호
	//		- account_alias			"보험료납부계좌"		AN(50) 변경코자하는 계좌별명
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/account/update_info"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// 3) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, "Authorization: Bearer %s"sv }; _request.authorization.c_str();

	// declare)
	char str_account_alias[256]{ 0 };

	// 4) 문자열변환을 수행한다.
	convert_multibyte_to_utf8(str_account_alias, _request.account_alias.c_str());

	// 5) parameter를 설정한다.
	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("fintech_use_num="sv); buf_option.append<TEXT_WEB>(_request.fintech_use_num);
	buf_option.append<TEXT_WEB>("&account_alias="sv); buf_option.append<TEXT_WEB>(str_account_alias);
	buf_option.front<char>(buf_option.size()) = 0;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//vector<std::string_view>	vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_ACCOUNT_UPDATE_INFO response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "fintech_use_num"
			else if (__strcmp(str_name, "fintech_use_num"sv) == 0)
			{
				response.fintech_use_num = value.GetString();
			}
			// - "account_alias"
			else if (__strcmp(str_name, "account_alias"sv) == 0)
			{
				response.account_alias = value.GetString();
			}
		}
	});

	// 9) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_ACCOUNT_CANCEL http_api::bank::request(const REQUEST_ACCOUNT_CANCEL& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/account/cancel
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- scope					[inquire | transfer]	inquire:조회서비스, transfer:출금서비스
	//		- fintech_use_num		"12345678901234567890"	AN(24) 핀테크이용번호
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// check)
	CGASSERT_ERROR(_request.scope==eOAUTH_SCOPE::INQUIRY_ || _request.scope == eOAUTH_SCOPE::TRANSFER_)

	// check)
	THROW_IF(_request.scope!=eOAUTH_SCOPE::INQUIRY_ && _request.scope != eOAUTH_SCOPE::TRANSFER_, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto str_object = "/v1.0/account/update_info"sv;

	// 2) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 3) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// 5) parameter를 설정한다.
	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("scope="sv);			buf_option.append<TEXT_WEB>(to_string<char>(_request.scope));
	buf_option.append<TEXT_WEB>("&fintech_use_num="sv);	buf_option.append<TEXT_WEB>(_request.fintech_use_num);
	buf_option[buf_option.size()] = 0;

	// 5) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view>	vector_accept{ "*/*"sv), "application/json"sv };

	// 6) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_ACCOUNT_CANCEL response;

	// 7) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv)==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "bank_tran_id"
			else if (__strcmp(str_name, "bank_tran_id"sv) == 0)
			{
				response.bank_tran_id = value.GetString();
			}
			// - "bank_tran_date"
			else if (__strcmp(str_name, "bank_tran_date"sv) == 0)
			{
				response.bank_tran_date = from_string_date(value.GetString());
			}
			// - "bank_code_tran"
			else if (__strcmp(str_name, "bank_code_tran"sv) == 0)
			{
				response.bank_code_tran = value.GetString();
			}
			// - "bank_rsp_code"
			else if (__strcmp(str_name, "bank_rsp_code"sv) == 0)
			{
				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_rsp_message"
			else if (__strcmp(str_name, "bank_rsp_message"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
			}
		}
	});

	// 9) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}
http_api::bank::RESPONSE_ACCOUNT_BALANCE http_api::bank::request(const REQUEST_ACCOUNT_BALANCE& _request)
{
	// ------------------------------------------------------------------
	// 
	// 사용자정보조회 API
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		/v1.0/account/balance
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Header
	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
	//
	//   6) Parameters
	//		- fintech_use_num		"12345678901234567890"	AN(24) 핀테크이용번호
	//		- tran_dtime			"20160310101921"		N(14) 요청일시
	//
	// ------------------------------------------------------------------
	// declare) 
	const std::size_t SIZE_BUFFER = 8192;

	// check)
	CGASSERT_ERROR(_request.str_url.empty() == false);

	// check)
	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
	auto prequest_http = make_object<query::http::query>();

	// declare)
	auto buf_object = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	auto size_write = __sprintf(buf_object.data<char>(), SIZE_BUFFER, "/v1.0/account/balance?fintech_use_num=%s&tran_dtime=%04u%02u%02u%02u%02u%02u"sv,
		_request.fintech_use_num.c_str(),
		static_cast<uint32_t>(_request.tran_dtime.t_year),
		static_cast<uint32_t>(_request.tran_dtime.t_month),
		static_cast<uint32_t>(_request.tran_dtime.t_day),
		static_cast<uint32_t>(_request.tran_dtime.t_hour),
		static_cast<uint32_t>(_request.tran_dtime.t_minute),
		static_cast<uint32_t>(_request.tran_dtime.t_second));

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare)
	auto buf_header = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
	size_write = __sprintf(buf_header.data<char>(), SIZE_BUFFER, "Authorization: Bearer %s"sv, _request.authorization.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(size_write < 0, throwable::failure(eRESULT::FAIL));

	// 5) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8"sv, buf_header.data<char>() };

	// declare) 
	shared_buffer buf_option;

	// 6) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*"sv, "text/plain"sv };
	//std::vector<std::string_view> vector_accept{ "*/*"sv, "application/json"sv };

	// 7) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1"sv,
		buf_object.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::bank::RESPONSE_ACCOUNT_BALANCE response;

	// 8) 기본값을 설정...
	response.rsp_code = eRESPONSE_CODE::A0000_;
	response.rsp_message.clear();

	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
	{
		// check) Result가 0이면 실패이므로 끝낸다.
		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
		{
			// - 내부처리 중 Error을 명시
			response.rsp_code = eRESPONSE_CODE::FAIL_;
			response.rsp_message = _msg.status_message;

			// - buffer를 clear한다.
			response.buf_response.clear();

			// Return)
			return;
		}

		// - 마지막을 null로 바꾼다.
		_msg.contents[0].append<char>(0);

		// - buffer를 넣는다.
		response.buf_response = _msg.contents[0];

		// declare) 
		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		// - Parsing한다.
		const auto&	result = json_data.Parse(_msg.contents[0].data<char>());

		// check) Parsing에 실패했으면 false를 리턴한다.
		RETURN_IF(result.IsNull())

		// - 값을 설정한다.
		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		{
			auto str_name = (*iter).name.GetString();
			auto& value = (*iter).value;

			// - rsp_code
			if(__strcmp(str_name, "rsp_code"sv) == 0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(__strcmp(str_name, "rsp_message"sv) == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}
			
			// - "api_tran_id"
			else if (__strcmp(str_name, "api_tran_id"sv) == 0)
			{
				response.api_tran_id = value.GetString();
			}
			// - "api_tran_dtm"
			else if (__strcmp(str_name, "api_tran_dtm"sv) == 0)
			{
				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
			}
			// - "bank_tran_id"
			else if (__strcmp(str_name, "bank_tran_id"sv) == 0)
			{
				response.bank_tran_id = value.GetString();
			}
			// - "bank_tran_date"
			else if (__strcmp(str_name, "bank_tran_date"sv) == 0)
			{
				response.bank_tran_date = from_string_date(value.GetString());
			}
			// - "bank_code_tran"
			else if (__strcmp(str_name, "bank_code_tran"sv) == 0)
			{
				response.bank_code_tran = value.GetString();
			}
			// - "bank_rsp_code"
			else if (__strcmp(str_name, "bank_rsp_code"sv) == 0)
			{
				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE<char>(value.GetString());
			}
			// - "bank_rsp_message"
			else if (__strcmp(str_name, "bank_rsp_message"sv) == 0)
			{
				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
			}
			// - "fintech_use_num"
			else if (__strcmp(str_name, "fintech_use_num"sv) == 0)
			{
				response.fintech_use_num = value.GetString();
			}
			// - "balance_amt"
			else if (__strcmp(str_name, "balance_amt"sv) == 0)
			{
				response.balance_amt = __from_string<int64_t>(value.GetString());
			}
			// - "available_amt"
			else if (__strcmp(str_name, "available_amt"sv) == 0)
			{
				response.available_amt = __from_string<int64_t>(value.GetString());
			}
			// - "account_type"
			else if (__strcmp(str_name, "account_type"sv) == 0)
			{
				response.account_type = get_enum_OAUTH_ACCOUNT_TYPE<char>(value.GetString());
			}
			// - "product_name"
			else if (__strcmp(str_name, "product_name"sv) == 0)
			{
				convert_utf8_to_multibyte(response.product_name, value.GetString());
			}
		}
	});

	// 10) request를 요청한다.
	prequest_http->request();

	// return) 
	return	response;
}


}