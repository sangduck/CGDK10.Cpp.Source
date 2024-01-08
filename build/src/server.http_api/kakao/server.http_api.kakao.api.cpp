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

// 1) KAKAO 사용자
http_api::kakao::RESPONSE_AUTHORIZE http_api::kakao::request(const REQUEST_AUTHORIZE& _request)
{
	// ------------------------------------------------------------------
	// 
	// KAKAO 인증 - 코드 받기
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://kauth.kakao.com/oauth/authorize
	//	 3) HTTP Method		GET
	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
	//   5) Parameters
	//		- client_id			(필수)	앱 생성시 발급 받은 REST API 키.
	//		- redirect_uri		(필수)	코드가 리다이렉트 된 URI. 설정>일반>웹>사이트 도메인에서 설정한 각각의 도메인에 설정>일반>웹>Redirect Path를 붙인 URI
	//		- response_type		(필수)	code 문자열 값으로 고정
	//		- state						Cross-site request Forgery 공격을 보호하기 위한 임의의 고유한 문자열, 리다이렉트시 해당값이 전달됨.
	//		- encode_state				state가 encode되어 있는지에 대한 유무(boolean). true(y또는 1)일 경우 state 전달시 decoded state로 전달.
	//									false(n또는0)일 경우 별도의 decode를 하지 않고 state를 그대로 전달. default false
	//
	//   요청)	GET /oauth/authorize?client_id={app_key}&redirect_uri={redirect_uri}&response_type=code HTTP/1.1 
	//			Host: kauth.kakao.com
	//
	//   결과)	HTTP / 1.1 302 Found
	//				Content - Length: 0
	//				Location : {redirect_uri} ? code = { authorize_code }
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
	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));

	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
	//auto	sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data()), SIZE_BUFFER, "/oauth/authorize?client_id=%s&redirect_uri=%s&response_type=code&state=%s", _request.client_id.c_str(), _request.redirect_uri.c_str(), _request.state.c_str());
	auto sizeWrite = __sprintf(bufObject.data<char>(), SIZE_BUFFER, "/oauth/authorize?client_id=%s&redirect_uri=%s&response_type=code", _request.client_id.c_str(), _request.redirect_uri.c_str());

	// check) sprintf가 오류가 났으면 Exception을 던진다.
	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::GET_;

	// declare) 
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };

	// declare) 
	shared_buffer buf_option;

	// 5) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };

	// 6) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		"HTTP/1.1",
		bufObject.data<char>(),
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::kakao::RESPONSE_AUTHORIZE response;

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

		//// declare) 
		//rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;

		//// - Parsing한다.
		//const auto&	result = json_data.Parse(_msg.contents[0].data_);

		//// check) Parsing에 실패했으면 false를 리턴한다.
		//RETURN_IF(result.IsNull())

		//// - 값을 설정한다.
		//for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
		//{
		//	auto	str_name	 = (*iter).name.GetString();
		//	auto&	value	 = (*iter).value;

		//	// - rsp_code
		//	if(strcmp(str_name, "rsp_code")==0)
		//	{
		//		response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
		//	}
		//	// - rsp_message
		//	else if(strcmp(str_name, "rsp_message") == 0)
		//	{
		//		convert_utf8_to_multibyte(response.rsp_message, value.GetString());
		//	}

		//	//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		//}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// 9) 완료를 기다린다.
	prequest_http->wait();

	// return) 
	return	response;
}
http_api::kakao::RESPONSE_TOKEN http_api::kakao::request(const REQUEST_TOKEN& _request)
{
	// ------------------------------------------------------------------
	// 
	// KAKAO 인증 - 사용자 토큰 받기
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://kauth.kakao.com/oauth/token
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
	//   5) Parameters
	//		- grant_type		(필수)	authorization_code로 고정
	//		- client_id			(필수)	앱 생성시 발급 받은 REST API키.
	//		- redirect_uri		(필수)	코드가 리다이렉트 된 URI. 설정>일반>웹>사이트 도메인에서 설정된 각각의 도메인에 설정>일반>웹>Redirect Path를 붙인 URI
	//		- code				(필수)	위 코드 받기에서 발급 받은 인증된 코드
	//		- client_secret				설정>일반>Client Secret에서 생성된 client_secret 코드 ACTIVE 상태인 경우 필수로 설정해야 함.
	//
	//	요청)	curl -v -X POST https://kauth.kakao.com/oauth/token 
	//			 -d 'grant_type=authorization_code' 
	//			 -d 'client_id={app_key}' 
	//			 -d 'redirect_uri={redirect_uri}' 
	//			 -d 'code={authorize_code}'	
	//
	//	결과)	HTTP/1.1 200 OK
	//			Content-Type: application/json;charset=UTF-8
	//			{
	//				"access_token":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
	//				"token_type":"bearer",
	//				"refresh_token":"yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy",
	//				"expires_in":43199,
	//				"scope":"Basic_Profile"
	//			}
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
	auto str_object = "/oauth/2.0/token";

	// 3) HTTP Method를 설정한다.
	auto http_method = eHTTP_METHOD::POST_;

	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };

	// 5) parameter를 설정한다.
	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
	buf_option.append<TEXT_WEB>("code=");			buf_option.append<TEXT_WEB>(_request.code);
	buf_option.append<TEXT_WEB>("&client_id=");		buf_option.append<TEXT_WEB>(_request.client_id);
	buf_option.append<TEXT_WEB>("&redirect_uri=");	buf_option.append<TEXT_WEB>(_request.redirect_uri);
	buf_option.append<TEXT_WEB>("&grant_type=authorization_code");

	// 6) Options
	if(_request.client_secret.empty()==false)
	{
		buf_option.append<TEXT_WEB>("&client_secret=");	buf_option.append<TEXT_WEB>(_request.client_secret);
	}
	buf_option[buf_option.size()] = 0;

	// 7) Accept 받을 것을 설정.
	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };

	// 8) request를 Set한다.
	prequest_http->set_request(
		http_method,
		_request.str_url,
		443,
		//800,
		"HTTP/1.1",
		str_object,
		std::string_view(),
		std::move(vector_header),
		buf_option,
		std::move(vector_accept),
		1		// 0: normal, 1:ssl
	);

	// declare) 
	http_api::kakao::RESPONSE_TOKEN response;

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
			if(strcmp(str_name, "rsp_code")==0)
			{
				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
			}
			// - rsp_message
			else if(strcmp(str_name, "rsp_message") == 0)
			{
				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
			}

			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
		}
	});

	// 8) request를 요청한다.
	prequest_http->request();

	// 9) 완료를 기다린다.
	prequest_http->wait();

	// return) 
	return response;
}
http_api::kakao::RESPONSE_LOGOUT http_api::kakao::request(const REQUEST_LOGOUT& /*_request*/)
{
	// ------------------------------------------------------------------
	// 
	// KAKAO 인증 - 로그아웃
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://kapi.kakao.com/v1/user/logout
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
	//   5) Parameters
	//		- Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxx
	//
	//	요청)	curl -v -X POST https://kapi.kakao.com/v1/user/logout 
	//			 -H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	//
	//	결과)	HTTP / 1.1 200 OK
	//				Content - Type: application / json; charset = UTF - 8
	//			{
	//				"id":123456789
	//			}
	//
	// ------------------------------------------------------------------

	// declare) 
	http_api::kakao::RESPONSE_LOGOUT response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_SIGNUP http_api::kakao::request(const REQUEST_SIGNUP& /*_request*/)
{
	// ------------------------------------------------------------------
	// 
	// KAKAO 인증 - 앱연결
	//
	// 요청 메시지 URL
	//   
	//   2) HTTP URL		https://kapi.kakao.com/v1/user/signup
	//	 3) HTTP Method		POST
	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
	//   5) Parameters
	//		- Authorization: Bearer {access_token}
	//	    - Content-Type: application/x-www-form-urlencoded;charset=utf-8
	//
	//	요청)	curl - v - X POST https ://kapi.kakao.com/v1/user/signup 
	//				-H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" 
	//				--data-urlencode 'properties={"age":"23", "gender":"female"}'
	//
	//	결과)	HTTP / 1.1 200 OK
	//			Content-Type: application/json;charset=UTF-8
	//			{
	//				"id":123456789
	//			}
	//
	// ------------------------------------------------------------------
	// declare) 
	http_api::kakao::RESPONSE_SIGNUP response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_UNLINK http_api::kakao::request(const REQUEST_UNLINK& /*_request*/)
{
	// ------------------------------------------------------------------
	// 
	// KAKAO 인증 - 앱연결해제
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
	http_api::kakao::RESPONSE_UNLINK response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_ME http_api::kakao::request(const REQUEST_ME& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_ME response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_UPDATE_PROFILE http_api::kakao::request(const REQUEST_UPDATE_PROFILE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_UPDATE_PROFILE response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_IDS http_api::kakao::request(const REQUEST_IDS& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_IDS	response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_ACCESS_TOKEN_INFO http_api::kakao::request(const REQUEST_ACCESS_TOKEN_INFO& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_ACCESS_TOKEN_INFO response;

	// return) 
	return response;
}

// 2) KAKAO Talk
http_api::kakao::RESPONSE_TAKL_PROFILE http_api::kakao::request(const REQUEST_TALK_PROFILE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_TAKL_PROFILE	response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_TAKL_SEND http_api::kakao::request(const REQUEST_TALK_SEND& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_TAKL_SEND response;

	// return) 
	return response;
}

// 3) KAKAO Pay
http_api::kakao::RESPONSE_PAY_READY http_api::kakao::request(const REQUEST_PAY_READY& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_READY response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_APPROVE http_api::kakao::request(const REQUEST_PAY_APPROVE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_APPROVE response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_SUBSCRIPTION http_api::kakao::request(const REQUEST_PAY_SUBSCRIPTION& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_SUBSCRIPTION	response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_CANCEL http_api::kakao::request(const REQUEST_PAY_CANCEL& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_CANCEL response;

	// return) 
	return response;
}

// 3) KAKAO Pay
http_api::kakao::RESPONSE_PAY_ORDER http_api::kakao::request(const REQUEST_PAY_ORDER& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_ORDER response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_ORDERS http_api::kakao::request(const REQUEST_PAY_ORDERS& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_ORDERS response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_STATUS http_api::kakao::request(const REQUEST_PAY_STATUS& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_STATUS response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PAY_INACTIVE http_api::kakao::request(const REQUEST_PAY_INACTIVE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PAY_INACTIVE	response;

	// return) 
	return response;
}

// 4) KAKAO Story
http_api::kakao::RESPONSE_STORY_ISSTORYUSER http_api::kakao::request(const REQUEST_STORY_ISSTORYUSER& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_ISSTORYUSER response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_PROFILE http_api::kakao::request(const REQUEST_STORY_PROFILE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_PROFILE response;
	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_NOTE http_api::kakao::request(const REQUEST_STORY_NOTE& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_NOTE response;

	// return) 
	return response;
}

// 5) KAKAO Story
http_api::kakao::RESPONSE_STORY_UPLOAD http_api::kakao::request(const REQUEST_STORY_UPLOAD& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_UPLOAD	response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_PHOTO http_api::kakao::request(const REQUEST_STORY_PHOTO& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_PHOTO response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_LINKINFO http_api::kakao::request(const REQUEST_STORY_LINKINFO& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_LINKINFO response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_LINK http_api::kakao::request(const REQUEST_STORY_LINK& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_LINK response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_MYSTORY http_api::kakao::request(const REQUEST_STORY_MYSTORY& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_MYSTORY response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_MYSTORIES http_api::kakao::request(const REQUEST_STORY_MYSTORIES& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_MYSTORIES response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_STORY_DELETE_MYSTROY http_api::kakao::request(const REQUEST_STORY_DELETE_MYSTROY& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_STORY_DELETE_MYSTROY	response;

	// return) 
	return response;
}

// 6) KAKAO Push
http_api::kakao::RESPONSE_PUSH_REGISTER http_api::kakao::request(const REQUEST_PUSH_REGISTER& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PUSH_REGISTER response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PUSH_TOKENS http_api::kakao::request(const REQUEST_PUSH_TOKENS& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PUSH_TOKENS response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PUSH_DEREGISTER http_api::kakao::request(const REQUEST_PUSH_DEREGISTER& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PUSH_DEREGISTER response;

	// return) 
	return response;
}
http_api::kakao::RESPONSE_PUSH_GCM_PUSH_FAIL http_api::kakao::request(const REQUEST_PUSH_GCM_PUSH_FAIL& /*_request*/)
{
	// declare) 
	http_api::kakao::RESPONSE_PUSH_GCM_PUSH_FAIL response;

	// return) 
	return response;
}



//
//
//http_api::kakao::RESPONSE_TOKEN http_api::kakao::request(const REQUEST_TOKEN& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - 사용자로그인연결 요청(3-legged)
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/KAKAO/2.0/token
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- code						<authorization_code>		사용자인증 성공 후 전송되는 authorization_code
//	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
//	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
//	//		- redirect_uri				<callback_uri>				Access Token을 전달받을 callback_uri
//	//		- grant_type				고정값:authorization_code	Access Token을 허용하기 위한 값
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t	SIZE_BUFFER = 8192;
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto	str_object	 = "/KAKAO/2.0/token";
//
//	// 3) HTTP Method를 설정한다.
//	auto	http_method	 = eHTTP_METHOD::POST_;
//
//	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("code=");				buf_option.append<TEXT_WEB>(_request.code);
//	buf_option.append<TEXT_WEB>("&client_id=");			buf_option.append<TEXT_WEB>(_request.client_id);
//	buf_option.append<TEXT_WEB>("&client_secret=");		buf_option.append<TEXT_WEB>(_request.client_secret);
//	buf_option.append<TEXT_WEB>("&redirect_uri=");		buf_option.append<TEXT_WEB>(_request.redirect_uri);
//	buf_option.append<TEXT_WEB>("&grant_type=authorization_code");
//	buf_option.front<char>(buf_option.size_)	 = 0;
//
//	// 6) Accept 받을 것을 설정.
//	std::vector<std::string_view>	vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view>	vector_accept{ "*/*", "application/json" };
//
//	// 7) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		//800,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TOKEN	response;
//
//	// 8) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parse한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - "rsp_code"
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - "rsp_message"
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "access_token"
//			else if(strcmp(str_name, "access_token") == 0)
//			{
//				response.access_token	 = value.GetString();
//			}
//			// - "token_type"
//			else if (strcmp(str_name, "token_type") == 0)
//			{
//				response.token_type		 = get_enum_OAUTH_TOKEN_TYPE(value.GetString());
//			}
//			// - "expires_in"
//			else if (strcmp(str_name, "expires_in") == 0)
//			{
//				response.expires_in		 = value.GetInt64();
//			}
//			// - "refresh_token"
//			else if (strcmp(str_name, "refresh_token") == 0)
//			{
//				response.refresh_token	 = value.GetString();
//			}
//			// - "scope"
//			else if (strcmp(str_name, "scope") == 0)
//			{
//				response.scope			 = get_enum_OAUTH_SCOPE(value.GetString());
//			}
//			// - "user_seq_no"
//			else if (strcmp(str_name, "user_seq_no") == 0)
//			{
//				response.user_seq_no	 = value.GetString();
//			}
//
//			// "user_ci"
//			else if (strcmp(str_name, "user_ci") == 0)
//			{
//				response.user_ci		 = value.GetString();
//			}
//			// "user_name"
//			else if (strcmp(str_name, "user_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.user_name, value.GetString());
//			}
//			// "user_info"
//			else if (strcmp(str_name, "user_info") == 0)
//			{
//				response.user_info		 = value.GetString();
//			}
//			// "user_gender"
//			else if (strcmp(str_name, "user_gender") == 0)
//			{
//				response.user_gender	 = get_enum_OAUTH_GENDER(value.GetString());
//			}
//			// "user_cell_no"
//			else if (strcmp(str_name, "user_cell_no") == 0)
//			{
//				response.user_cell_no	 = value.GetString();
//			}
//			// "user_email"
//			else if (strcmp(str_name, "user_email") == 0)
//			{
//				response.user_email		 = value.GetString();
//			}
//		}
//	});
//
//	// 5) request를 요청한다.
//	prequest_http->request();
//
//	// 12) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TOKEN2 http_api::kakao::request(const REQUEST_TOKEN2& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - 이용기관 Access Token 요청 (2-legged)
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/KAKAO/2.0/token
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
//	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
//	//		- grant_type				고정값:client_credentials	2-legged 인증을 위한 인증방식 지정
//	//		- scope						고정값:oob					Access Token 권한 범위
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t	SIZE_BUFFER = 8192;
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto	str_object	 = "/KAKAO/2.0/token";
//
//	// 3) HTTP Method를 설정한다.
//	auto	http_method	 = eHTTP_METHOD::POST_;
//
//	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("client_id=");			buf_option.append<TEXT_WEB>(_request.client_id);
//	buf_option.append<TEXT_WEB>("&client_secret=");		buf_option.append<TEXT_WEB>(_request.client_secret);
//	buf_option.append<TEXT_WEB>("&grant_type=client_credentials");
//	buf_option.append<TEXT_WEB>("&scope=oob");
//	buf_option.front<char>(buf_option.size_)	 = 0;
//
//	// 6) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 7) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		//800,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TOKEN2	response;
//
//	// 8) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// 1) Parse한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// 2) 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "access_token"
//			else if(strcmp(str_name, "access_token") == 0)
//			{
//				response.access_token	 = value.GetString();
//			}
//			// - "token_type"
//			else if (strcmp(str_name, "token_type") == 0)
//			{
//				response.token_type		 = get_enum_OAUTH_TOKEN_TYPE(value.GetString());
//			}
//			// - "expires_in"
//			else if (strcmp(str_name, "expires_in") == 0)
//			{
//				response.expires_in		 = value.GetInt64();
//			}
//			// - "scope"
//			else if (strcmp(str_name, "scope") == 0)
//			{
//				response.scope			 = get_enum_OAUTH_SCOPE(value.GetString());
//			}
//			// - "client_use_code"
//			else if (strcmp(str_name, "client_use_code") == 0)
//			{
//				response.client_use_code = value.GetString();
//			}
//		}
//	});
//
//	// 5) request를 요청한다.
//	prequest_http->request();
//
//	// 12) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TOKEN_REFRESH http_api::kakao::request(const REQUEST_TOKEN_REFRESH& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - Login Access Token 갱신(refresh_token 사용)
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/KAKAO/2.0/token
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- client_id					<client_id>					오픈플랫폼에서 발급한 이용기관 앱의 Client ID
//	//		- client_secret				<client_secret>				오픈플랫폼에서 발급한 이용기관 앱의 Client Secret
//	//		- refresh_token				<refresh_token>				login 토큰 발급 시 받은 access_token 갱신용 token
//	//		- scope						고정값						Access Token 권한 범위
//	//									[login|inquiry|transfer]
//	//		- grant_type				고정값:refresh_token		Access Token을 허용하기 위한 값
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t	SIZE_BUFFER = 8192;
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto str_object = "/oauth/2.0/token";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("client_id=");			buf_option.append<TEXT_WEB>(_request.client_id);
//	buf_option.append<TEXT_WEB>("&client_secret=");	buf_option.append<TEXT_WEB>(_request.client_secret);
//	buf_option.append<TEXT_WEB>("&refresh_token=");	buf_option.append<TEXT_WEB>(_request.refresh_token);
//	buf_option.append<TEXT_WEB>("&scope=");			buf_option.append<TEXT_WEB>(to_string<char>(_request.scope));
//	buf_option.append<TEXT_WEB>("&grant_type=refresh_token");
//	buf_option.front<char>(buf_option.size_)	 = 0;
//
//	// 6) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 7) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		//800,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TOKEN_REFRESH	response;
//
//	// 8) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 9) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// 1) Parse한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// 2) 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "access_token"
//			else if (strcmp(str_name, "access_token") == 0)
//			{
//				response.access_token	 = value.GetString();
//			}
//			// - "token_type"
//			else if (strcmp(str_name, "token_type") == 0)
//			{
//				response.token_type		 = get_enum_OAUTH_TOKEN_TYPE(value.GetString());
//			}
//			// - "expires_in"
//			else if (strcmp(str_name, "expires_in") == 0)
//			{
//				response.expires_in		 = value.GetInt64();
//			}
//			// - "scope"
//			else if (strcmp(str_name, "scope") == 0)
//			{
//				response.scope			 = get_enum_OAUTH_SCOPE(value.GetString());
//			}
//			// - "user_seq_no"
//			else if (strcmp(str_name, "user_seq_no") == 0)
//			{
//				response.user_seq_no	 = value.GetString();
//			}
//		}
//	});
//
//	// 5) request를 요청한다.
//	prequest_http->request();
//
//	// 12) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//
//inline time::local::time_point from_string_date_time_millsec(const char* _string)
//{
//	// declare) 
//	time::local::time_point	temp;
//
//	// check)
//	RETURN_IF(_string == nullptr, temp);
//
//	// check)
//	RETURN_IF(_string[0] == 0, temp);
//
//	// 1) Year
//	temp.set_year		((_string[ 0]-'0')*1000 + (_string[ 1]-'0')*100 + (_string[ 2]-'0')*10+ (_string[ 3]-'0'));
//
//	// 2) Month
//	temp.set_month		((_string[ 4]-'0')*10 + (_string[ 5]-'0'));
//
//	// 3) Day
//	temp.set_day		((_string[ 6]-'0')*10 + (_string[ 7]-'0'));
//
//	// 4) Hour
//	temp.set_hour		((_string[ 8]-'0')*10 + (_string[ 9]-'0'));
//
//	// 5) Minute
//	temp.set_minute		((_string[10]-'0')*10 + (_string[11]-'0'));
//
//	// 6) Second
//	temp.set_second		((_string[12]-'0')*10 + (_string[13]-'0'));
//
//	// 7) millisecond
//	temp.set_millisecond((_string[14]-'0')*100 + (_string[15]-'0')*10 + (_string[16]-'0'));
//
//	// return) 
//	return	temp;
//}
//
//inline time::local::time_point from_string_date_time(const char* _string)
//{
//	// declare) 
//	time::local::time_point	temp;
//
//	// check)
//	RETURN_IF(_string == nullptr, temp);
//
//	// check)
//	RETURN_IF(_string[0] == 0, temp);
//
//	// 1) Year
//	temp.set_year		((_string[ 0]-'0')*1000 + (_string[ 1]-'0')*100 + (_string[ 2]-'0')*10+ (_string[ 3]-'0'));
//
//	// 2) Month
//	temp.set_month		((_string[ 4]-'0')*10 + (_string[ 5]-'0'));
//
//	// 3) Day
//	temp.set_day		((_string[ 6]-'0')*10 + (_string[ 7]-'0'));
//
//	// 4) Hour
//	temp.set_hour		((_string[ 8]-'0')*10 + (_string[ 9]-'0'));
//
//	// 5) Minute
//	temp.set_minute		((_string[10]-'0')*10 + (_string[11]-'0'));
//
//	// 6) Second
//	temp.set_second		((_string[12]-'0')*10 + (_string[13]-'0'));
//
//	// 7) millisecond
//	temp.set_millisecond(0);
//
//	// return) 
//	return	temp;
//}
//
//inline time::local::time_point from_string_date(const char* _string)
//{
//	// declare) 
//	time::local::time_point	temp;
//
//	// check)
//	RETURN_IF(_string == nullptr, temp);
//
//	// check)
//	RETURN_IF(_string[0] == 0, temp);
//
//	// 1) Year
//	temp.set_year		((_string[0]-'0')*1000 + (_string[1]-'0')*100 + (_string[2]-'0')*10 + (_string[3]-'0'));
//
//	// 2) Month
//	temp.set_month		((_string[4]-'0')*10 + (_string[5]-'0'));
//
//	// 3) Day
//	temp.set_day		((_string[6]-'0')*10 + (_string[7]-'0'));
//
//	// 4) Hour
//	temp.set_hour(0);
//
//	// 5) Minute
//	temp.set_minute(0);
//
//	// 6) Second
//	temp.set_second(0);
//
//	// 7) millisecond
//	temp.set_millisecond(0);
//
//	// return) 
//	return	temp;
//}
//
//inline time::local::time_point from_string_time(const char* _string)
//{
//	// declare) 
//	time::local::time_point	temp;
//
//	// check)
//	RETURN_IF(_string == nullptr, temp);
//
//	// check)
//	RETURN_IF(_string[0] == 0, temp);
//
//	// 1) Year
//	temp.set_year(0);
//
//	// 2) Month
//	temp.set_month(0);
//
//	// 3) Day
//	temp.set_day(0);
//
//	// 4) Hour
//	temp.set_hour((_string[0] - '0') * 10 + (_string[1] - '0'));
//
//	// 5) Minute
//	temp.set_minute((_string[2] - '0') * 10 + (_string[3] - '0'));
//
//	// 6) Second
//	temp.set_second((_string[4] - '0') * 10 + (_string[5] - '0'));
//
//	// 7) millisecond
//	temp.set_millisecond(0);
//
//	// return) 
//	return	temp;
//}
//
//http_api::kakao::RESPONSE_USER_ME http_api::kakao::request(const REQUEST_USER_ME& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/user/me
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- user_seq_no			"U123456789"			AN(10) 사용자일련번호
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t	SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto bufObject	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto	sizeWrite	 = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/v1.0/user/me?user_seq_no=%s", _request.user_seq_no.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto	http_method	 = eHTTP_METHOD::GET_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type:application/x-www-form-urlencoded;charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) }; 
//
//	// declare) 
//	shared_buffer	buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_USER_ME	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt	 = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//
//			// - "user_seq_no"
//			else if (strcmp(str_name, "user_seq_no") == 0)
//			{
//				response.user_seq_no = value.GetString();
//			}
//			// - "user_ci"
//			else if (strcmp(str_name, "user_ci") == 0)
//			{
//				response.user_ci = value.GetString();
//			}
//			// - "user_name"
//			else if (strcmp(str_name, "user_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.user_name, value.GetString());
//			}
//			// - "res_cnt"
//			else if (strcmp(str_name, "res_cnt") == 0)
//			{
//				res_cnt	 = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_USER_ME::ACCOUNT_INFO	AccouuntInfo;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested	 = iterNest->name.GetString();
//						auto&	value_Nested	 = iterNest->value;
//
//						// - "fintech_use_num"
//						if (strcmp(strName_Nested, "fintech_use_num") == 0)
//						{
//							AccouuntInfo.fintech_use_num = value_Nested.GetString();
//						}
//						// - "account_alias"
//						else if (strcmp(strName_Nested, "account_alias") == 0)
//						{
//							convert_utf8_to_multibyte(AccouuntInfo.account_alias, value_Nested.GetString());
//						}
//						// - "bank_code_std"
//						else if (strcmp(strName_Nested, "bank_code_std") == 0)
//						{
//							AccouuntInfo.bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_code_sub"
//						else if (strcmp(strName_Nested, "bank_code_sub") == 0)
//						{
//							AccouuntInfo.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_name"
//						else if (strcmp(strName_Nested, "bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(AccouuntInfo.bank_name, value_Nested.GetString());
//						}
//						// - "account_num_masked"
//						else if (strcmp(strName_Nested, "account_num_masked") == 0)
//						{
//							AccouuntInfo.account_num_masked = value_Nested.GetString();
//						}
//						// - "account_holder_name"
//						else if (strcmp(strName_Nested, "account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(AccouuntInfo.account_holder_name, value_Nested.GetString());
//						}
//						// - "account_type"
//						else if (strcmp(strName_Nested, "account_type") == 0)
//						{
//							AccouuntInfo.account_type = value_Nested.GetString();
//						}
//						// - "inquiry_agree_yn"
//						else if (strcmp(strName_Nested, "inquiry_agree_yn") == 0)
//						{
//							AccouuntInfo.inquiry_agree_yn = get_enum_OAUTH_YESNO(value_Nested.GetString());
//						}
//						// - "inquiry_agree_dtime"
//						else if (strcmp(strName_Nested, "inquiry_agree_dtime") == 0)
//						{
//							AccouuntInfo.inquiry_agree_dtime = from_string_date_time(value_Nested.GetString());
//						}
//						// - "transfer_agree_yn"
//						else if (strcmp(strName_Nested, "transfer_agree_yn") == 0)
//						{
//							AccouuntInfo.transfer_agree_yn = get_enum_OAUTH_YESNO(value_Nested.GetString());
//						}
//						// - "transfer_agree_dtime"
//						else if (strcmp(strName_Nested, "transfer_agree_dtime") == 0)
//						{
//							AccouuntInfo.transfer_agree_dtime = from_string_date_time(value_Nested.GetString());
//						}
//					}
//
//					response.res_list.push_back(AccouuntInfo);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_UNLINK http_api::kakao::request(const REQUEST_UNLINK& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/unlink
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- client_user_code		"F001234560"			AN(10) 이용기관코드
//	//		- user_seq_no			"U123456789"			AN(10) 사용자일련번호
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object = "/oauth/2.0/unlink";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	//    Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) }; 
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("{client_use_code: ");	buf_option.append<TEXT_WEB>(_request.client_use_code);
//	buf_option.append<TEXT_WEB>(", user_seq_no: ");		buf_option.append<TEXT_WEB>(_request.user_seq_no);
//	buf_option.append<TEXT_WEB>("}");
//	buf_option.front<char>(buf_option.size_) = 0;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_UNLINK	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "user_seq_no"
//			else if (strcmp(str_name, "user_seq_no") == 0)
//			{
//				response.user_seq_no = value.GetString();
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_REGISTER_ACCOUNT http_api::kakao::request(const REQUEST_REGISTER_ACCOUNT& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - 사용자로그인연결 요청(3-legged): 구버전
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/authorize
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- response_type			고정값: code			KAKAO 2.0 인정 요청시 반환되는 형태
//	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
//	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
//	//		- scope					고정값:login			Access Token 권한 범위
//	//								(login | inquiry | transfer)
//	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
//	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/oauth/2.0/register_account?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&client_info=%s", _request.client_id.c_str(), _request.redirect_uri.c_str(), to_string<char>(_request.scope), _request.client_info.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::GET_;
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_REGISTER_ACCOUNT	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT http_api::kakao::request(const REQUEST_AUTHORIZE_ACCOUNT& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - 계좌등록확인 요청 (3-legged): 구버전
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://openapi.open.openform.or.kr/oauth/2.0/authorize_account
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- response_type			고정값: code			KAKAO 2.0 인정 요청시 반환되는 형태
//	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
//	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
//	//		- scope					[inquiry | transfer]	Access Token 권한 범위
//	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
//	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
//	//
//	// ------------------------------------------------------------------
//	// check)
//	CGASSERT_ERROR(_request.scope==eOAUTH_SCOPE::INQUIRY_ || _request.scope == eOAUTH_SCOPE::TRANSFER_)
//
//	// check)
//	THROW_IF(_request.scope!=eOAUTH_SCOPE::INQUIRY_ && _request.scope != eOAUTH_SCOPE::TRANSFER_, throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/oauth/2.0/authorize_account?response_type=code&client_id=%s&redirect_uri=%s&scope=%s&client_info=%s", _request.client_id.c_str(), _request.redirect_uri.c_str(), to_string<char>(_request.scope), _request.client_info.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::GET_;
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_FIRST http_api::kakao::request(const REQUEST_AUTHORIZE_ACCOUNT2_FIRST& /*_request*/)
//{
//	// declare) 
//	http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_FIRST	response;
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_REAUTH http_api::kakao::request(const REQUEST_AUTHORIZE_ACCOUNT2_REAUTH& /*_request*/)
//{
//	// declare) 
//	http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_REAUTH	response;
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_BYPASS http_api::kakao::request(const REQUEST_AUTHORIZE_ACCOUNT2_BYPASS& /*_request*/)
//{
//	// declare) 
//	http_api::kakao::RESPONSE_AUTHORIZE_ACCOUNT2_BYPASS	response;
//
//	// return) 
//	return	response;
//}
//
//
//
//http_api::kakao::RESPONSE_ACCOUNT_LIST http_api::kakao::request(const REQUEST_ACCOUNT_LIST& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// KAKAO 인증 - 계좌등록확인 요청 (3-legged): 구버전
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://openapi.open.openform.or.kr/v1.0/account/list
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- response_type			고정값: code			KAKAO 2.0 인정 요청시 반환되는 형태
//	//		- client_id				<client_id>				오픈플랫폼에서 발급한 이용기관 협의 Client ID
//	//		- redirect_uri			<callback_uri>			사용자로그인연결이 성공되면 이용기관으로 연결되는 URL
//	//		- scope					[inquiry | transfer]	Access Token 권한 범위
//	//		- client_info			<client_info>			이용기관이 셋팅하는 임의의 정보
//	//								(Max: 256bytes)			Callback 호출 시 그대로 전달됨
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/v1.0/account/list?user_seq_no=%s&include_cancel_yn=%s&sort_order=%s", _request.user_seq_no.c_str(), to_string<char>(_request.include_cancel_yn), to_string<char>(_request.sort_order));
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto		http_method	 = eHTTP_METHOD::GET_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_ACCOUNT_LIST	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt	 = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "user_name"
//			else if (strcmp(str_name, "user_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.user_name, value.GetString());
//			}
//			// - "res_cnt"
//			else if (strcmp(str_name, "res_cnt") == 0)
//			{
//				res_cnt	 = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_ACCOUNT_LIST::ACCOUNT_INFO	AccouuntInfo;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested	 = iterNest->name.GetString();
//						auto&	value_Nested	 = iterNest->value;
//
//						// - "fintech_use_num"
//						if (strcmp(strName_Nested, "fintech_use_num") == 0)
//						{
//							AccouuntInfo.fintech_use_num	 = value_Nested.GetString();
//						}
//						// - "account_alias"
//						else if (strcmp(strName_Nested, "account_alias") == 0)
//						{
//							AccouuntInfo.account_alias		 = value_Nested.GetString();
//						}
//						// - "bank_code_std"
//						else if (strcmp(strName_Nested, "bank_code_std") == 0)
//						{
//							AccouuntInfo.bank_code_std		 = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_code_sub"
//						else if (strcmp(strName_Nested, "bank_code_sub") == 0)
//						{
//							AccouuntInfo.bank_code_sub		 = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_name"
//						else if (strcmp(strName_Nested, "bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(AccouuntInfo.bank_name, value_Nested.GetString());
//						}
//						// - "account_num_masked"
//						else if (strcmp(strName_Nested, "account_num_masked") == 0)
//						{
//							AccouuntInfo.account_num_masked	 = value_Nested.GetString();
//						}
//					}
//
//					response.res_list.push_back(AccouuntInfo);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_ACCOUNT_UPDATE_INFO http_api::kakao::request(const REQUEST_ACCOUNT_UPDATE_INFO& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/account/update_info
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- fintech_use_num		<fintech_use_num>			AN(24) 핀테크이용번호
//	//		- account_alias			"보험료납부계좌"			AN(50) 변경코자하는 계좌별명
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t	SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto		str_object	 = "/v1.0/account/update_info";
//
//	// 3) HTTP Method를 설정한다.
//	auto		http_method	 = eHTTP_METHOD::POST_;
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", "Authorization: Bearer %s" }; _request.Authorization.c_str();
//
//	// declare)
//	char	strAccountAlias[256];
//
//	// 5) 문자열변환을 수행한다.
//	convert_multibyte_to_utf8(strAccountAlias, _request.account_alias.c_str());
//
//	// 6) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("fintech_use_num=");		buf_option.append<TEXT_WEB>(_request.fintech_use_num);
//	buf_option.append<TEXT_WEB>("&account_alias=");		buf_option.append<TEXT_WEB>(strAccountAlias);
//	buf_option.front<char>(buf_option.size_)	 = 0;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_ACCOUNT_UPDATE_INFO	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "fintech_use_num"
//			else if (strcmp(str_name, "fintech_use_num") == 0)
//			{
//				response.fintech_use_num = value.GetString();
//			}
//			// - "account_alias"
//			else if (strcmp(str_name, "account_alias") == 0)
//			{
//				response.account_alias = value.GetString();
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_ACCOUNT_CANCEL http_api::kakao::request(const REQUEST_ACCOUNT_CANCEL& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/account/cancel
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- scope					[inquire | transfer]	inquire:조회서비스, transfer:출금서비스
//	//		- fintech_use_num		"12345678901234567890"	AN(24) 핀테크이용번호
//	//
//	// ------------------------------------------------------------------
//	// check)
//	CGASSERT_ERROR(_request.scope==eOAUTH_SCOPE::INQUIRY_ || _request.scope == eOAUTH_SCOPE::TRANSFER_)
//
//	// check)
//	THROW_IF(_request.scope!=eOAUTH_SCOPE::INQUIRY_ && _request.scope != eOAUTH_SCOPE::TRANSFER_, throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object = "/v1.0/account/update_info";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// 5) parameter를 설정한다.
//	shared_buffer	buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("scope=");				buf_option.append<TEXT_WEB>(to_string<char>(_request.scope));
//	buf_option.append<TEXT_WEB>("&fintech_use_num=");	buf_option.append<TEXT_WEB>(_request.fintech_use_num);
//	buf_option.front<char>(buf_option.size_)	 = 0;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_ACCOUNT_CANCEL	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "bank_tran_id"
//			else if (strcmp(str_name, "bank_tran_id") == 0)
//			{
//				response.bank_tran_id = value.GetString();
//			}
//			// - "bank_tran_date"
//			else if (strcmp(str_name, "bank_tran_date") == 0)
//			{
//				response.bank_tran_date = from_string_date(value.GetString());
//			}
//			// - "bank_code_tran"
//			else if (strcmp(str_name, "bank_code_tran") == 0)
//			{
//				response.bank_code_tran = value.GetString();
//			}
//			// - "bank_rsp_code"
//			else if (strcmp(str_name, "bank_rsp_code") == 0)
//			{
//				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_rsp_message"
//			else if (strcmp(str_name, "bank_rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 12) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_ACCOUNT_BALANCE http_api::kakao::request(const REQUEST_ACCOUNT_BALANCE& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/account/balance
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- fintech_use_num		"12345678901234567890"	AN(24) 핀테크이용번호
//	//		- tran_dtime			"20160310101921"		N(14) 요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/v1.0/account/balance?fintech_use_num=%s&tran_dtime=%04u%02u%02u%02u%02u%02u", 
//		_request.fintech_use_num.c_str(),
//		_request.tran_dtime.get_year  <uint32_t>(),
//		_request.tran_dtime.get_month <uint32_t>(),
//		_request.tran_dtime.get_day   <uint32_t>(),
//		_request.tran_dtime.get_hour  <uint32_t>(),
//		_request.tran_dtime.get_minute<uint32_t>(),
//		_request.tran_dtime.get_second<uint32_t>());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method	 = eHTTP_METHOD::GET_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_ACCOUNT_BALANCE	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//			
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "bank_tran_id"
//			else if (strcmp(str_name, "bank_tran_id") == 0)
//			{
//				response.bank_tran_id = value.GetString();
//			}
//			// - "bank_tran_date"
//			else if (strcmp(str_name, "bank_tran_date") == 0)
//			{
//				response.bank_tran_date = from_string_date(value.GetString());
//			}
//			// - "bank_code_tran"
//			else if (strcmp(str_name, "bank_code_tran") == 0)
//			{
//				response.bank_code_tran = value.GetString();
//			}
//			// - "bank_rsp_code"
//			else if (strcmp(str_name, "bank_rsp_code") == 0)
//			{
//				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_rsp_message"
//			else if (strcmp(str_name, "bank_rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
//			}
//			// - "fintech_use_num"
//			else if (strcmp(str_name, "fintech_use_num") == 0)
//			{
//				response.fintech_use_num = value.GetString();
//			}
//			// - "balance_amt"
//			else if (strcmp(str_name, "balance_amt") == 0)
//			{
//				response.balance_amt = __from_string<int64_t>(value.GetString());
//			}
//			// - "available_amt"
//			else if (strcmp(str_name, "available_amt") == 0)
//			{
//				response.available_amt = __from_string<int64_t>(value.GetString());
//			}
//			// - "account_type"
//			else if (strcmp(str_name, "account_type") == 0)
//			{
//				response.account_type = get_enum_OAUTH_ACCOUNT_TYPE(value.GetString());
//			}
//			// - "product_name"
//			else if (strcmp(str_name, "product_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.product_name, value.GetString());
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_ACCOUNT_TRANSACTION_LIST http_api::kakao::request(const REQUEST_ACCOUNT_TRANSACTION_LIST& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/account/transaction_list
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization			Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- fintech_use_num			"12345678901234567890"	AN(24) 핀테크이용번호
//	//		- inquiry_type				"A"						A(1) 조회구분콛드 A:All, I:입금, O:입금
//	//		- from_date					"20160310101921"		N(14) 조회시작일자
//	//		- to_date					"20160310101921"		N(14) 조회종료일자
//	//		- sort_order				"D"						A(1) 정렬순서 D:Descending, A:Ascending
//	//		- page_index				"00001"					N(5) 페이지번호
//	//		- tran_dtime				"20160310101921"		N(14) 요청일시
//	//		- before_inquiry_trace_info	"123"					AN(20) 직전 조회 추적 정보
//	//		- list_tran_seqno			"123"					N(10) 최종 거래내역 순번
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) 시간을 얻는다.
//	auto&	from_time	 = _request.from_time;
//	auto&	to_time		 = _request.to_time;
//	auto&	tran_dtime	 = _request.tran_dtime;
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto	sizeWrite	 = __sprintf(reinterpret_cast<char*>(bufObject.data_), SIZE_BUFFER, "/v1.0/account/transaction_list?fintech_use_num=%s&inquiry_type=%s&from_date=%04u%02u%02u&to_date=%04u%02u%02u&sort_order=%s&page_index=%05u&tran_dtime=%04u%02u%02u%02u%02u%02u&befor_inquiry_trace_info=%llu&list_tran_seqno=%llu",
//		_request.fintech_use_num.c_str(),
//		to_string<char>(_request.inquiry_type),
//		from_time.get_year<uint32_t>(), from_time.get_month<uint32_t>(), from_time.get_day<uint32_t>(),
//		to_time.get_year<uint32_t>(), to_time.get_month<uint32_t>(), to_time.get_day<uint32_t>(),
//		to_string<char>(_request.sort_order),
//		_request.page_index,
//		tran_dtime.get_year<uint32_t>(), tran_dtime.get_month<uint32_t>(), tran_dtime.get_day<uint32_t>(), tran_dtime.get_hour<uint32_t>(), tran_dtime.get_minute<uint32_t>(), tran_dtime.get_second<uint32_t>(),
//		_request.before_inquiry_trace_info,
//		_request.list_tran_seqno
//	);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto	http_method	 = eHTTP_METHOD::GET_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		reinterpret_cast<char*>(bufObject.data_),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_ACCOUNT_TRANSACTION_LIST	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare) 
//		int	page_record_cnt = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "bank_tran_id"
//			else if (strcmp(str_name, "bank_tran_id") == 0)
//			{
//				response.bank_tran_id = value.GetString();
//			}
//			// - "bank_tran_date"
//			else if (strcmp(str_name, "bank_tran_date") == 0)
//			{
//				response.bank_tran_date = from_string_date(value.GetString());
//			}
//			// - "bank_code_tran"
//			else if (strcmp(str_name, "bank_code_tran") == 0)
//			{
//				response.bank_code_tran = value.GetString();
//			}
//			// - "bank_rsp_code"
//			else if (strcmp(str_name, "bank_rsp_code") == 0)
//			{
//				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_rsp_message"
//			else if (strcmp(str_name, "bank_rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
//			}
//			// - "fintech_use_num"
//			else if (strcmp(str_name, "fintech_use_num") == 0)
//			{
//				response.fintech_use_num = value.GetString();
//			}
//			// - "balance_amt"
//			else if (strcmp(str_name, "balance_amt") == 0)
//			{
//				response.balance_amt = __from_string<int64_t>(value.GetString());
//			}
//			// - "page_index_use_yn"
//			else if (strcmp(str_name, "page_index_use_yn") == 0)
//			{
//				response.page_index_use_yn = get_enum_OAUTH_YESNO(value.GetString());
//			}
//			// - "page_index"
//			else if (strcmp(str_name, "page_index") == 0)
//			{
//				response.page_index = __from_string<int64_t>(value.GetString());
//			}
//			// - "total_record_cnt"
//			else if (strcmp(str_name, "total_record_cnt") == 0)
//			{
//				response.total_record_cnt = __from_string<int64_t>(value.GetString());
//			}
//			// - "page_record_cnt"
//			else if (strcmp(str_name, "page_record_cnt") == 0)
//			{
//				page_record_cnt = __from_string<int>(value.GetString());
//			}
//			// - "next_page_yn"
//			else if (strcmp(str_name, "next_page_yn") == 0)
//			{
//				response.next_page_yn = get_enum_OAUTH_YESNO(value.GetString());
//			}
//			// - "befor_inquiry_trace_info"
//			else if (strcmp(str_name, "befor_inquiry_trace_info") == 0)
//			{
//				response.befor_inquiry_trace_info = value.GetString();
//			}
//			// - "list_tran_seqno"
//			else if (strcmp(str_name, "list_tran_seqno") == 0)
//			{
//				response.list_tran_seqno = value.GetString();
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_ACCOUNT_TRANSACTION_LIST::TRANSACTION	transaction;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested	 = iterNest->name.GetString();
//						auto&	value_Nested	 = iterNest->value;
//
//						// - "tran_date"
//						if (strcmp(strName_Nested, "tran_date") == 0)
//						{
//							transaction.tran_date = from_string_date(value_Nested.GetString());
//						}
//						// - "tran_time"
//						else if (strcmp(strName_Nested, "tran_time") == 0)
//						{
//							transaction.tran_time = from_string_time(value_Nested.GetString());
//						}
//						// - "inout_type"
//						else if (strcmp(strName_Nested, "inout_type") == 0)
//						{
//							// declare) 
//							static_string<64>	tempString;
//
//							// - 문자열 변환
//							convert_utf8_to_multibyte(tempString, value_Nested.GetString());
//
//							// - "access_token"
//							if (__strcmp(tempString.c_str(), "입금") == 0)
//							{
//								transaction.inout_type = eOAUTH_INOUT_TYPE::IN_;
//							}
//							// - "token_type"
//							else if (__strcmp(tempString.c_str(), "출금") == 0)
//							{
//								transaction.inout_type = eOAUTH_INOUT_TYPE::OUT_;
//							}
//							else
//							{
//								transaction.inout_type = eOAUTH_INOUT_TYPE::NOT_DEFINED_;
//							}
//						}
//						// - "tran_type"
//						else if (strcmp(strName_Nested, "tran_type") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.tran_type, value_Nested.GetString());
//						}
//						// - "print_content"
//						else if (strcmp(strName_Nested, "print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
//						}
//						// - "tran_amt"
//						else if (strcmp(strName_Nested, "tran_amt") == 0)
//						{
//							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "after_balance_amt"
//						else if (strcmp(strName_Nested, "after_balance_amt") == 0)
//						{
//							transaction.after_balance_amt = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "branch_name"
//						else if (strcmp(strName_Nested, "branch_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.branch_name, value_Nested.GetString());
//						}
//					}
//
//					response.res_list.push_back(transaction);
//				}
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TRANSFER_WITHDRAW http_api::kakao::request(const REQUEST_TRANSFER_WITHDRAW& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/transfer/withdraw
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- dps_print_content			"입금계좌인자내역"		AH(20) 입금계좌인자내역
//	//		- fintech_use_num			"12345678901234567890"	AN(24) 핀테크이용번호
//	//		- tran_amt					"10000"					N(12) 거래금액
//	//		- tran_dtime				"20160310101921"		N(14) 요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object = "/v1.0/transfer/withdraw";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// 5) parameter를 설정한다.
//	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// declare)
//	char strDpsPrintContent[256];
//
//	// 6) 문자열변환을 수행한다.
//	convert_multibyte_to_utf8(strDpsPrintContent, _request.dps_print_content.c_str());
//
//	// 7) Option
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"dps_print_content\": \"%s\", \"fintech_use_num\": \"%s\", \"tran_amt\": \"%lld\", \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		strDpsPrintContent,
//		_request.fintech_use_num.c_str(),
//		_request.tran_amt,
//		_request.tran_dtime.get_year<uint32_t>(), _request.tran_dtime.get_month<uint32_t>(), _request.tran_dtime.get_day<uint32_t>(), _request.tran_dtime.get_hour<uint32_t>(), _request.tran_dtime.get_minute<uint32_t>(), _request.tran_dtime.get_second<uint32_t>()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 8) Option의 길이를 저장한다.
//	buf_option.size_ = sizeWrite;
//
//	// 9) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 10) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TRANSFER_WITHDRAW	response;
//
//	// 11) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 12) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "dps_bank_code_std"
//			else if (strcmp(str_name, "dps_bank_code_std") == 0)
//			{
//				response.dps_bank_code_std = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "dps_bank_code_sub"
//			else if (strcmp(str_name, "dps_bank_code_sub") == 0)
//			{
//				response.dps_bank_code_sub = __from_string<int64_t>(value.GetString());
//			}
//			// - "dps_bank_name"
//			else if (strcmp(str_name, "dps_bank_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.dps_bank_name, value.GetString());
//			}
//			// - "dps_account_num_masked"
//			else if (strcmp(str_name, "dps_account_num_masked") == 0)
//			{
//				response.dps_account_num_masked = value.GetString();
//			}
//			// - "dps_print_content"
//			else if (strcmp(str_name, "dps_print_content") == 0)
//			{
//				convert_utf8_to_multibyte(response.dps_print_content, value.GetString());
//			}
//			// - "dps_account_holder_name"
//			else if (strcmp(str_name, "dps_account_holder_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.dps_account_holder_name, value.GetString());
//			}
//			// - "bank_tran_id"
//			else if (strcmp(str_name, "bank_tran_id") == 0)
//			{
//				response.bank_tran_id = value.GetString();
//			}
//			// - "bank_tran_date"
//			else if (strcmp(str_name, "bank_tran_date") == 0)
//			{
//				response.bank_tran_date = from_string_date(value.GetString());
//			}
//			// - "bank_code_tran"
//			else if (strcmp(str_name, "bank_code_tran") == 0)
//			{
//				response.bank_code_tran = value.GetString();
//			}
//			// - "bank_rsp_code"
//			else if (strcmp(str_name, "bank_rsp_code") == 0)
//			{
//				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_rsp_message"
//			else if (strcmp(str_name, "bank_rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
//			}
//			// - "fintech_use_num"
//			else if (strcmp(str_name, "fintech_use_num") == 0)
//			{
//				response.fintech_use_num = value.GetString();
//			}
//			// - "account_alias"
//			else if (strcmp(str_name, "account_alias") == 0)
//			{
//				convert_utf8_to_multibyte(response.account_alias, value.GetString());
//			}
//			// - "bank_code_std"
//			else if (strcmp(str_name, "bank_code_std") == 0)
//			{
//				response.bank_code_std = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_code_sub"
//			else if (strcmp(str_name, "bank_code_sub") == 0)
//			{
//				response.bank_code_sub = __from_string<int64_t>(value.GetString());
//			}
//			// - "bank_name"
//			else if (strcmp(str_name, "bank_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_name, value.GetString());
//			}
//			// - "account_num_masked"
//			else if (strcmp(str_name, "account_num_masked") == 0)
//			{
//				response.account_num_masked = value.GetString();
//			}
//			// - "print_content"
//			else if (strcmp(str_name, "print_content") == 0)
//			{
//				convert_utf8_to_multibyte(response.print_content, value.GetString());
//			}
//			// - "account_holder_name"
//			else if (strcmp(str_name, "account_holder_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.account_holder_name, value.GetString());
//			}
//			// - "tran_amt"
//			else if (strcmp(str_name, "tran_amt") == 0)
//			{
//				response.tran_amt = __from_string<int64_t>(value.GetString());
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TRANSFER_DEPOSIT http_api::kakao::request(const REQUEST_TRANSFER_DEPOSIT& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/transfer/deposit
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- ws_pass_pharase			"ASDFQWERZXCVTYUI"	AN(64) 이용기관 입금이체용 암호문구
//	//		- ws_print_content			"출금계좌인자내역"		AH(20) 출금계좌인자내역
//	//		- req_cnt					"25"					N(5)   입금요청건수
//	//		- req_list					<object>				       입금요청목록
//	//		- --tran_no					"1"					N(5)   거래순번
//	//		- --fintech_use_num			"1234567890123456"	N(24)  핀테크이용번호
//	//		- --print_content			"입금계좌인자내역"		AH(20) 입금계좌인자내역
//	//		- --tran_amt				"10000"				N(12) 거래금액
//	//		- tran_dtime				"20160310101921"		N(14) 요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto str_object = "/v1.0/transfer/deposit";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) }; 
//
//	// 5) parameter를 설정한다.
//	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// declare) 
//	char	strPrintContent[256];
//
//	// 6) 문자열변환을 수행한다.
//	convert_multibyte_to_utf8(strPrintContent, _request.wd_print_content.c_str());
//
//	// - ...
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"wd_pass_phrase\": \"%s\", \"wd_print_content\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": [",
//		_request.wd_pass_phrase.c_str(),
//		strPrintContent,
//		_request.req_list.size()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - 길이를 추가한다.
//	buf_option.size_ += sizeWrite;
//
//	// - req_list를 추가한다.
//	for(auto iter: _request.req_list)
//	{
//		// - 문자열변환을 수행한다.
//		convert_multibyte_to_utf8(strPrintContent, iter.print_content.c_str());
//
//		// - 뒷부분을 추가한다.
//		auto	sizeWrite2 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER - buf_option.size_, "{ \"tran_no\": \"%d\", \"fintech_use_num\": \"%s\", \"print_content\": \"%s\", \"tran_amt\": \"%lld\"}",
//			iter.tran_no,					// tran_no
//			iter.fintech_use_num.c_str(),	// fintech_use_num
//			strPrintContent,				// print_content
//			iter.tran_amt					// tran_amt
//		);
//
//		// check) sprintf가 오류가 났으면 Exception을 던진다.
//		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));
//
//		// - 길이를 추가한다.
//		buf_option.size_ += sizeWrite2;
//	}
//
//	// - 뒷부분을 추가한다.
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER- buf_option.size_, "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		_request.tran_dtime.get_year<uint32_t>(), _request.tran_dtime.get_month<uint32_t>(), _request.tran_dtime.get_day<uint32_t>(), _request.tran_dtime.get_hour<uint32_t>(), _request.tran_dtime.get_minute<uint32_t>(), _request.tran_dtime.get_second<uint32_t>()
//		);
//
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 6) Option의 길이를 더한다.
//	buf_option.size_	+= sizeWrite;
//
//	// 7) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 8) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TRANSFER_DEPOSIT	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt	 = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "wd_bank_code_std"
//			else if (strcmp(str_name, "wd_bank_code_std") == 0)
//			{
//				response.wd_bank_code_std = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "wd_bank_code_sub"
//			else if (strcmp(str_name, "wd_bank_code_sub") == 0)
//			{
//				response.wd_bank_code_sub = __from_string<int64_t>(value.GetString());
//			}
//			// - "wd_bank_name"
//			else if (strcmp(str_name, "wd_bank_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_bank_name, value.GetString());
//			}
//			// - "wd_account_num_masked"
//			else if (strcmp(str_name, "wd_account_num_masked") == 0)
//			{
//				response.wd_account_num_masked = value.GetString();
//			}
//			// - "wd_print_content"
//			else if (strcmp(str_name, "wd_print_content") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_print_content, value.GetString());
//			}
//			// - "wd_account_holder_name"
//			else if (strcmp(str_name, "wd_account_holder_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_account_holder_name, value.GetString());
//			}
//			else if(strcmp(str_name, "res_cnt")==0)
//			{
//				res_cnt	 = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_TRANSFER_DEPOSIT::TRANSACTION	transaction;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested = iterNest->name.GetString();
//						auto&	value_Nested = iterNest->value;
//
//						// - "tran_no"
//						if (strcmp(strName_Nested, "tran_no") == 0)
//						{
//							transaction.tran_no = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_tran_id"
//						else if (strcmp(strName_Nested, "bank_tran_id") == 0)
//						{
//							transaction.bank_tran_id = value_Nested.GetString();
//						}
//						// - "bank_tran_date"
//						else if (strcmp(strName_Nested, "bank_tran_date") == 0)
//						{
//							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
//						}
//						// - "bank_code_tran"
//						else if (strcmp(strName_Nested, "bank_code_tran") == 0)
//						{
//							transaction.bank_code_tran = value_Nested.GetString();
//						}
//						// - "bank_rsp_code"
//						else if (strcmp(strName_Nested, "bank_rsp_code") == 0)
//						{
//							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_rsp_message"
//						else if (strcmp(strName_Nested, "bank_rsp_message") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
//						}
//						// - "fintech_use_num"
//						else if (strcmp(strName_Nested, "fintech_use_num") == 0)
//						{
//							transaction.fintech_use_num = value_Nested.GetString();
//						}
//						// - "account_alias"
//						else if (strcmp(strName_Nested, "account_alias") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.account_alias, value_Nested.GetString());
//						}
//						// - "bank_code_std"
//						else if (strcmp(strName_Nested, "bank_code_std") == 0)
//						{
//							transaction.bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_code_sub"
//						else if (strcmp(strName_Nested, "bank_code_sub") == 0)
//						{
//							transaction.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_name"
//						else if (strcmp(strName_Nested, "bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_name, value_Nested.GetString());
//						}
//						// - "account_num_masked"
//						else if (strcmp(strName_Nested, "account_num_masked") == 0)
//						{
//							transaction.account_num_masked = value_Nested.GetString();
//						}
//						// - "print_content"
//						else if (strcmp(strName_Nested, "print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
//						}
//						// - "account_holder_name"
//						else if (strcmp(strName_Nested, "account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.account_holder_name, value_Nested.GetString());
//						}
//						// - "tran_amt"
//						else if (strcmp(strName_Nested, "tran_amt") == 0)
//						{
//							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
//						}
//					}
//
//					response.res_list.push_back(transaction);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TRANSFER_DEPOSIT2 http_api::kakao::request(const REQUEST_TRANSFER_DEPOSIT2& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/transfer/deposit2
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- ws_pass_pharase			"ASDFQWERZXCVTYUI"	AN(64) 이용기관 입금이체용 암호문구
//	//		- ws_print_content			"출금계좌인자내역"		AH(20) 출금계좌인자내역
//	//		- req_cnt					"25"					N(5)   입금요청건수
//	//		- req_list					<object>				       입금요청목록
//	//		- --tran_no					"1"					N(5)   거래순번
//	//		- --bank_code_std			"097"					AN(3)  입금은행.표준코드
//	//		- --account_num				"001230000678"		AN(16) 계좌번호
//	//		- --account_holder_name		"홍길동"				AH(20) 입금계좌예금주명
//	//		- --print_content			"입금계좌인자내역"		AH(20) 입금계좌인자내역
//	//		- --tran_amt				"10000"				N(12) 거래금액
//	//		- tran_dtime				"20160310101921"		N(14) 요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object	 = "/v1.0/transfer/deposit2";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method	 = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	//    Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// declare) 
//	char strWdPrintContent[256];
//
//	// 6) 문자열변환을 수행한다.
//	convert_multibyte_to_utf8(strWdPrintContent, _request.wd_print_content.c_str());
//
//	// - ...
//	sizeWrite = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"wd_pass_phrase\": \"%s\", \"wd_print_content\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": [",
//		_request.wd_pass_phrase.c_str(),
//		strWdPrintContent,
//		_request.req_list.size()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - 길이를 추가한다.
//	buf_option.size_ += sizeWrite;
//
//
//	// -) req_list를 추가한다.
//	for(auto iter: _request.req_list)
//	{
//		// declare) 
//		char	strAccountHolderName[256];
//		char	strPrintContent[256];
//
//		// 6) 문자열변환을 수행한다.
//		convert_multibyte_to_utf8(strAccountHolderName, iter.account_holder_name.c_str());
//		convert_multibyte_to_utf8(strPrintContent, iter.print_content.c_str());
//
//		// - 뒷부분을 추가한다.
//		sizeWrite = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER - buf_option.size_, "{ \"tran_no\": \"%d\", \"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"account_holder_name\": \"%s\", \"print_content\": \"%s\", \"tran_amt\": \"%lld\"}",
//			iter.tran_no,							// tran_no
//			static_cast<int>(iter.bank_code_std),	// bank_code_std
//			iter.accoount_num.c_str(),				// account_num
//			strAccountHolderName,					// account_holder_name
//			strPrintContent,						// print_content
//			iter.tran_amt							// tran_amt
//		);
//
//		// check) sprintf가 오류가 났으면 Exception을 던진다.
//		THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//		// - 길이를 추가한다.
//		buf_option.size_ += sizeWrite;
//	}
//
//	// - 뒷부분을 추가한다.
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER- buf_option.size_, "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		_request.tran_dtime.get_year<uint32_t>(), _request.tran_dtime.get_month<uint32_t>(), _request.tran_dtime.get_day<uint32_t>(), _request.tran_dtime.get_hour<uint32_t>(), _request.tran_dtime.get_minute<uint32_t>(), _request.tran_dtime.get_second<uint32_t>()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 6) Option의 길이를 저장한다.
//	buf_option.size_	+= sizeWrite;
//
//	// 7) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 8) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TRANSFER_DEPOSIT2	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "wd_bank_code_std"
//			else if (strcmp(str_name, "wd_bank_code_std") == 0)
//			{
//				response.wd_bank_code_std = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "wd_bank_code_sub"
//			else if (strcmp(str_name, "wd_bank_code_sub") == 0)
//			{
//				response.wd_bank_code_sub = __from_string<int64_t>(value.GetString());
//			}
//			// - "wd_bank_name"
//			else if (strcmp(str_name, "wd_bank_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_bank_name, value.GetString());
//			}
//			// - "wd_account_num_masked"
//			else if (strcmp(str_name, "wd_account_num_masked") == 0)
//			{
//				response.wd_account_num_masked = value.GetString();
//			}
//			// - "wd_print_content"
//			else if (strcmp(str_name, "wd_print_content") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_print_content, value.GetString());
//			}
//			// - "wd_account_holder_name"
//			else if (strcmp(str_name, "wd_account_holder_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.wd_account_holder_name, value.GetString());
//			}
//			else if (strcmp(str_name, "res_cnt") == 0)
//			{
//				res_cnt = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_TRANSFER_DEPOSIT2::TRANSACTION	transaction;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested = iterNest->name.GetString();
//						auto&	value_Nested = iterNest->value;
//
//						// - "tran_no"
//						if (strcmp(strName_Nested, "tran_no") == 0)
//						{
//							transaction.tran_no = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_tran_id"
//						else if (strcmp(strName_Nested, "bank_tran_id") == 0)
//						{
//							transaction.bank_tran_id = value_Nested.GetString();
//						}
//						// - "bank_tran_date"
//						else if (strcmp(strName_Nested, "bank_tran_date") == 0)
//						{
//							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
//						}
//						// - "bank_code_tran"
//						else if (strcmp(strName_Nested, "bank_code_tran") == 0)
//						{
//							transaction.bank_code_tran = value_Nested.GetString();
//						}
//						// - "bank_rsp_code"
//						else if (strcmp(strName_Nested, "bank_rsp_code") == 0)
//						{
//							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_rsp_message"
//						else if (strcmp(strName_Nested, "bank_rsp_message") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
//						}
//						// - "bank_code_std"
//						else if (strcmp(strName_Nested, "bank_code_std") == 0)
//						{
//							transaction.bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_code_sub"
//						else if (strcmp(strName_Nested, "bank_code_sub") == 0)
//						{
//							transaction.bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_name"
//						else if (strcmp(strName_Nested, "bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_name, value_Nested.GetString());
//						}
//						// - "account_num"
//						else if (strcmp(strName_Nested, "account_num") == 0)
//						{
//							transaction.account_num = value_Nested.GetString();
//						}
//						// - "account_num_masked"
//						else if (strcmp(strName_Nested, "account_num_masked") == 0)
//						{
//							transaction.account_num_masked = value_Nested.GetString();
//						}
//						// - "print_content"
//						else if (strcmp(strName_Nested, "print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.print_content, value_Nested.GetString());
//						}
//						// - "account_holder_name"
//						else if (strcmp(strName_Nested, "account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.account_holder_name, value_Nested.GetString());
//						}
//						// - "tran_amt"
//						else if (strcmp(strName_Nested, "tran_amt") == 0)
//						{
//							transaction.tran_amt = value_Nested.GetString();
//						}
//					}
//
//					response.res_list.push_back(transaction);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TRANSFER_RESULT http_api::kakao::request(const REQUEST_TRANSFER_RESULT& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/transfer/result
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- check_type				"1"					AN(1) 1:출금이체, 2:입금이체
//	//		- req_cnt					"25"					N(5)   입금요청건수
//	//		- req_list					<object>				       입금요청목록
//	//		- --tran_no					"1"					N(5)   거래순번
//	//		- --org_bank_tran_id		"123456712345"		AN(20) 원거래, 거래고유번호(참가기관)
//	//		- --org_bank_tran_date		"20160310"			N(8)   원거래 거래일자(참가기관)
//	//		- --org_tran_amt			"10000"				N(12)  원거래 거래금액
//	//		- tran_dtime				"20160310101921"		N(14)  요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object = "/v1.0/transfer/result";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// 5) parameter를 설정한다.
//	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// - ...
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"check_type\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": [",
//		to_string<char>(_request.check_type),
//		_request.req_list.size()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - 길이를 추가한다.
//	buf_option.size_ += sizeWrite;
//
//
//	// -) req_list를 추가한다.
//	for(auto iter: _request.req_list)
//	{
//		const time::local::time_point&	org_bank_tran_date	 = iter.org_bank_tran_date;
//
//		// - 뒷부분을 추가한다.
//		auto	sizeWrite2 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER - buf_option.size_, "{ \"tran_no\": \"%d\", \"org_bank_tran_id\": \"%s\", \"org_bank_tran_date\": \"%04u%02u%02u\", \"org_tran_amt\": \"%lld\"}",
//			iter.tran_no,						// tran_no
//			iter.org_bank_tran_id.c_str(),		// org_bank_tran_id
//			org_bank_tran_date.get_year<uint32_t>(), org_bank_tran_date.get_month<uint32_t>(), org_bank_tran_date.get_day<uint32_t>(),	// org_bank_tran_date
//			iter.org_tran_amt					// org_tran_amt
//		);
//
//		// check) sprintf가 오류가 났으면 Exception을 던진다.
//		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));
//
//		// - 길이를 추가한다.
//		buf_option.size_ += sizeWrite2;
//	}
//
//	// - 뒷부분을 추가한다.
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER- buf_option.size_, "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		_request.tran_dtime.get_year<uint32_t>(), _request.tran_dtime.get_month<uint32_t>(), _request.tran_dtime.get_day<uint32_t>(), _request.tran_dtime.get_hour<uint32_t>(), _request.tran_dtime.get_minute<uint32_t>(), _request.tran_dtime.get_second<uint32_t>()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 6) Option의 길이를 저장한다.
//	buf_option.size_	+= sizeWrite;
//
//	// 7) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 8) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TRANSFER_RESULT	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			else if (strcmp(str_name, "res_cnt") == 0)
//			{
//				res_cnt = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_TRANSFER_RESULT::TRANSACTION	transaction;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested = iterNest->name.GetString();
//						auto&	value_Nested = iterNest->value;
//
//						// - "tran_no"
//						if (strcmp(strName_Nested, "tran_no") == 0)
//						{
//							transaction.tran_no = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_tran_id"
//						else if (strcmp(strName_Nested, "bank_tran_id") == 0)
//						{
//							transaction.bank_tran_id = value_Nested.GetString();
//						}
//						// - "bank_tran_date"
//						else if (strcmp(strName_Nested, "bank_tran_date") == 0)
//						{
//							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
//						}
//						// - "bank_code_tran"
//						else if (strcmp(strName_Nested, "bank_code_tran") == 0)
//						{
//							transaction.bank_code_tran = value_Nested.GetString();
//						}
//						// - "bank_rsp_code"
//						else if (strcmp(strName_Nested, "bank_rsp_code") == 0)
//						{
//							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_rsp_message"
//						else if (strcmp(strName_Nested, "bank_rsp_message") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
//						}
//						// - "wd_bank_code_std"
//						else if (strcmp(strName_Nested, "wd_bank_code_std") == 0)
//						{
//							transaction.wd_bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "wd_bank_code_sub"
//						else if (strcmp(strName_Nested, "wd_bank_code_sub") == 0)
//						{
//							transaction.wd_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "wd_bank_name"
//						else if (strcmp(strName_Nested, "wd_bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_bank_name, value_Nested.GetString());
//						}
//						// - "wd_fintech_use_num"
//						else if (strcmp(strName_Nested, "wd_fintech_use_num") == 0)
//						{
//							transaction.wd_fintech_use_num = value_Nested.GetString();
//						}
//						// - "wd_account_num_masked"
//						else if (strcmp(strName_Nested, "wd_account_num_masked") == 0)
//						{
//							transaction.wd_account_num_masked = value_Nested.GetString();
//						}
//						// - "wd_print_content"
//						else if (strcmp(strName_Nested, "wd_print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_print_content, value_Nested.GetString());
//						}
//						// - "wd_account_holder_name"
//						else if (strcmp(strName_Nested, "wd_account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_account_holder_name, value_Nested.GetString());
//						}
//						// - "dps_bank_code_std"
//						else if (strcmp(strName_Nested, "dps_bank_code_std") == 0)
//						{
//							transaction.dps_bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "dps_bank_code_sub"
//						else if (strcmp(strName_Nested, "dps_bank_code_sub") == 0)
//						{
//							transaction.dps_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "dps_bank_name"
//						else if (strcmp(strName_Nested, "dps_bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_bank_name, value_Nested.GetString());
//						}
//						// - "dps_fintech_use_num"
//						else if (strcmp(strName_Nested, "dps_fintech_use_num") == 0)
//						{
//							transaction.dps_fintech_use_num = value_Nested.GetString();
//						}
//						// - "dps_account_num_masked"
//						else if (strcmp(strName_Nested, "dps_account_num_masked") == 0)
//						{
//							transaction.dps_account_num_masked = value_Nested.GetString();
//						}
//						// - "dps_print_content"
//						else if (strcmp(strName_Nested, "dps_print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_print_content, value_Nested.GetString());
//						}
//						// - "dps_account_holder_name"
//						else if (strcmp(strName_Nested, "dps_account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_account_holder_name, value_Nested.GetString());
//						}
//						// - "tran_amt"
//						else if (strcmp(strName_Nested, "tran_amt") == 0)
//						{
//							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
//						}
//					}
//
//					response.res_list.push_back(transaction);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_TRANSFER_RECHECK http_api::kakao::request(const REQUEST_TRANSFER_RECHECK& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/transfer/recheck
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- check_type				"1"					AN(1) 1:출금이체, 2:입금이체
//	//		- req_cnt					"25"					N(5)   입금요청건수
//	//		- req_list					<object>				       입금요청목록
//	//		-   tran_no					"1"					N(5)   거래순번
//	//		-   org_bank_tran_dtime		"20160725100010"		AN(14) 원거래요청일시
//	//		-   org_req_gubun			"1"					AN(1)  원거래요청계좌지정방식구분 1:핀테크이ㅣ용번호, 2:계좌번호
//	//		-   print_content			"입금계좌인자내역"		AH(20)  원거래 입출금계좌인자내역
//	//		-   fintech_use_num			"1234567890123456"	AN(24)  핀테크이용번호
//	//		-   org_tran_amt			"10000"				N(12)  원거래 거래금액
//	//		- tran_dtime				"20160310101921"		N(14)  요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto	prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER	 = 8192;
//
//	// declare)
//	auto str_object	 = "/v1.0/transfer/recheck";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method	 = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	//    Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) };
//
//	// 5) parameter를 설정한다.
//	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// - ...
//	sizeWrite = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"check_type\": \"%s\", \"req_cnt\": \"%llu\", \"req_list\": [",
//		to_string<char>(_request.check_type),
//		_request.req_list.size()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - 길이를 추가한다.
//	buf_option.size_ += sizeWrite;
//
//	// - req_list를 추가한다.
//	for(auto iter: _request.req_list)
//	{
//		// declare) 
//		char	strPrintContent[256];
//
//		// - 문자열변환을 수행한다.
//		convert_multibyte_to_utf8(strPrintContent, iter.print_content.c_str());
//
//		// - 
//		const auto&		org_tran_dtime	 = iter.org_tran_dtime;
//
//		// - 뒷부분을 추가한다.
//		auto	sizeWrite2 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER - buf_option.size_, "{ \"tran_no\": \"%d\", \"org_tran_dtime\": \"%04u%02u%02u%02u%02u%02u\", \"org_req_gubun\": \"%s\", \"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"print_content\": \"%s\", \"fintech_use_num\": \"%s\", \"org_tran_amt\": \"%lld\"}",
//			iter.tran_no,					// tran_no
//			org_tran_dtime.get_year<uint32_t>(), org_tran_dtime.get_month<uint32_t>(), org_tran_dtime.get_day<uint32_t>(), org_tran_dtime.get_hour<uint32_t>(), org_tran_dtime.get_minute<uint32_t>(), org_tran_dtime.get_second<uint32_t>(),
//			to_string<char>(iter.org_req_gubun),		// org_req_gubun
//			iter.bank_code_std,				// bank_code_std
//			iter.accoount_num.c_str(),		// account_num
//			strPrintContent,				// print_content
//			iter.fintech_use_num.c_str(),	// fintech_use_num
//			iter.org_tran_amt				// org_tran_amt
//		);
//
//		// check) sprintf가 오류가 났으면 Exception을 던진다.
//		THROW_IF(sizeWrite2 < 0, throwable::failure(eRESULT::FAIL));
//
//		// - 길이를 추가한다.
//		buf_option.size_ += sizeWrite2;
//	}
//
//	// - 뒷부분을 추가한다.
//	sizeWrite	 = __sprintf(reinterpret_cast<char*>(buf_option.data_ + buf_option.size_), SIZE_BUFFER- buf_option.size_, "], \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		_request.tran_dtime.get_year<uint32_t>(), _request.tran_dtime.get_month<uint32_t>(), _request.tran_dtime.get_day<uint32_t>(), _request.tran_dtime.get_hour<uint32_t>(), _request.tran_dtime.get_minute<uint32_t>(), _request.tran_dtime.get_second<uint32_t>()
//		);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 6) Option의 길이를 저장한다.
//	buf_option.size_	+= sizeWrite;
//
//	// 7) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 8) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_TRANSFER_RECHECK	response;
//
//	// 9) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// declare)
//		size_t	res_cnt = 0;
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			else if (strcmp(str_name, "res_cnt") == 0)
//			{
//				res_cnt = __from_string<size_t>(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_TRANSFER_RECHECK::TRANSACTION	transaction;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested = iterNest->name.GetString();
//						auto&	value_Nested = iterNest->value;
//
//						// - "tran_no"
//						if (strcmp(strName_Nested, "tran_no") == 0)
//						{
//							transaction.tran_no = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "bank_tran_id"
//						else if (strcmp(strName_Nested, "bank_tran_id") == 0)
//						{
//							transaction.bank_tran_id = value_Nested.GetString();
//						}
//						// - "bank_tran_date"
//						else if (strcmp(strName_Nested, "bank_tran_date") == 0)
//						{
//							transaction.bank_tran_date = from_string_date(value_Nested.GetString());
//						}
//						// - "bank_code_tran"
//						else if (strcmp(strName_Nested, "bank_code_tran") == 0)
//						{
//							transaction.bank_code_tran = value_Nested.GetString();
//						}
//						// - "bank_rsp_code"
//						else if (strcmp(strName_Nested, "bank_rsp_code") == 0)
//						{
//							transaction.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "bank_rsp_message"
//						else if (strcmp(strName_Nested, "bank_rsp_message") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.bank_rsp_message, value_Nested.GetString());
//						}
//						// - "wd_bank_code_std"
//						else if (strcmp(strName_Nested, "wd_bank_code_std") == 0)
//						{
//							transaction.wd_bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "wd_bank_code_sub"
//						else if (strcmp(strName_Nested, "wd_bank_code_sub") == 0)
//						{
//							transaction.wd_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "wd_bank_name"
//						else if (strcmp(strName_Nested, "wd_bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_bank_name, value_Nested.GetString());
//						}
//						// - "wd_fintech_use_num"
//						else if (strcmp(strName_Nested, "wd_fintech_use_num") == 0)
//						{
//							transaction.wd_fintech_use_num = value_Nested.GetString();
//						}
//						// - "wd_account_num_masked"
//						else if (strcmp(strName_Nested, "wd_account_num_masked") == 0)
//						{
//							transaction.wd_account_num_masked = value_Nested.GetString();
//						}
//						// - "wd_print_content"
//						else if (strcmp(strName_Nested, "wd_print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_print_content, value_Nested.GetString());
//						}
//						// - "wd_account_holder_name"
//						else if (strcmp(strName_Nested, "wd_account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.wd_account_holder_name, value_Nested.GetString());
//						}
//						// - "dps_bank_code_std"
//						else if (strcmp(strName_Nested, "dps_bank_code_std") == 0)
//						{
//							transaction.dps_bank_code_std = get_enum_OAUTH_BANK_CODE(value_Nested.GetString());
//						}
//						// - "dps_bank_code_sub"
//						else if (strcmp(strName_Nested, "dps_bank_code_sub") == 0)
//						{
//							transaction.dps_bank_code_sub = __from_string<int64_t>(value_Nested.GetString());
//						}
//						// - "dps_bank_name"
//						else if (strcmp(strName_Nested, "dps_bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_bank_name, value_Nested.GetString());
//						}
//						// - "dps_fintech_use_num"
//						else if (strcmp(strName_Nested, "dps_fintech_use_num") == 0)
//						{
//							transaction.dps_fintech_use_num = value_Nested.GetString();
//						}
//						// - "dps_account_num_masked"
//						else if (strcmp(strName_Nested, "dps_account_num_masked") == 0)
//						{
//							transaction.dps_account_num_masked = value_Nested.GetString();
//						}
//						// - "dps_print_content"
//						else if (strcmp(strName_Nested, "dps_print_content") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_print_content, value_Nested.GetString());
//						}
//						// - "dps_account_holder_name"
//						else if (strcmp(strName_Nested, "dps_account_holder_name") == 0)
//						{
//							convert_utf8_to_multibyte(transaction.dps_account_holder_name, value_Nested.GetString());
//						}
//						// - "tran_amt"
//						else if (strcmp(strName_Nested, "tran_amt") == 0)
//						{
//							transaction.tran_amt = __from_string<int64_t>(value_Nested.GetString());
//						}
//					}
//
//					response.res_list.push_back(transaction);
//				}
//
//				// check) 
//				CGASSERT_ERROR(response.res_list.size() == res_cnt);
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_INQUIRY_REAL_NAME http_api::kakao::request(const REQUEST_INQUIRY_REAL_NAME& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 사용자정보조회 API
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/inquiry/realname
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	//   6) Parameters
//	//		- bank_code_std				"097"					AN(3)  개설기관 표준코드
//	//		- account_num				"1234567890123123"	AN(16) 개좌번호
//	//		- account_holder_info		"8801012"				AN(7)  예금주 인증정보 YYYYMMDD(6) + 식별정보(1)
//	//		- tran_dtime				"20160310101921"		N(14)  요청일시
//	//
//	// ------------------------------------------------------------------
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// declare)
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// declare)
//	auto str_object = "/v1.0/inquiry/real_name";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	//    Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/json; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) }; 
//
//	// 5) parameter를 설정한다.
//	auto buf_option = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// Decalre) 
//	auto& tran_dtime = _request.tran_dtime;
//
//	// - ...
//	sizeWrite = __sprintf(reinterpret_cast<char*>(buf_option.data_), SIZE_BUFFER, "{\"bank_code_std\": \"%03d\", \"account_num\": \"%s\", \"account_holder_info\": \"%s\", \"tran_dtime\": \"%04u%02u%02u%02u%02u%02u\"}",
//		_request.bank_code_std,
//		_request.accoount_num.c_str(),
//		_request.accoount_holder_info.c_str(),
//		tran_dtime.get_year<uint32_t>(), tran_dtime.get_month<uint32_t>(), tran_dtime.get_day<uint32_t>(), tran_dtime.get_hour<uint32_t>(), tran_dtime.get_minute<uint32_t>(), tran_dtime.get_second<uint32_t>()
//	);
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// - 길이를 추가한다.
//	buf_option.size_  = sizeWrite;
//
//	// 7) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 8) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_INQUIRY_REAL_NAME	response;
//
//	// 9) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 10) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//
//			// - "bank_tran_id"
//			else if (strcmp(str_name, "bank_tran_id") == 0)
//			{
//				response.bank_tran_id = value.GetString();
//			}
//			// - "bank_tran_date"
//			else if (strcmp(str_name, "bank_tran_date") == 0)
//			{
//				response.bank_tran_date = from_string_date(value.GetString());
//			}
//			// - "bank_code_tran"
//			else if (strcmp(str_name, "bank_code_tran") == 0)
//			{
//				response.bank_code_tran = value.GetString();
//			}
//			// - "bank_rsp_code"
//			else if (strcmp(str_name, "bank_rsp_code") == 0)
//			{
//				response.bank_rsp_code = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_rsp_message"
//			else if (strcmp(str_name, "bank_rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_rsp_message, value.GetString());
//			}
//			// - "bank_code_std"
//			else if (strcmp(str_name, "bank_code_std") == 0)
//			{
//				response.bank_code_std = get_enum_OAUTH_BANK_CODE(value.GetString());
//			}
//			// - "bank_code_sub"
//			else if (strcmp(str_name, "bank_code_sub") == 0)
//			{
//				response.bank_code_sub = __from_string<int64_t>(value.GetString());
//			}
//			// - "bank_name"
//			else if (strcmp(str_name, "bank_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.bank_name, value.GetString());
//			}
//			// - "account_num"
//			else if (strcmp(str_name, "account_num") == 0)
//			{
//				response.account_num = value.GetString();
//			}
//			// - "account_holder_info"
//			else if (strcmp(str_name, "account_holder_info") == 0)
//			{
//				response.account_holder_info = value.GetString();
//			}
//			// - "account_holder_name"
//			else if (strcmp(str_name, "account_holder_name") == 0)
//			{
//				convert_utf8_to_multibyte(response.account_holder_name, value.GetString());
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}
//
//http_api::kakao::RESPONSE_BANK_STATUS http_api::kakao::request(const REQUEST_BANK_STATUS& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// 참가기관상태조회
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		/v1.0/bank/status
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Header
//	//		- Authorization				Bearer<access_token>	오픈플랫폼으로부터 전송받은 access_token을 HTTP Header에 추가 [scope=login]
//	//
//	// ------------------------------------------------------------------
//	// declare)
//	const size_t	SIZE_BUFFER = 8192;
//
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto str_object = "/v1.0/bank/status";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::GET_;
//
//	// declare)
//	auto bufHeader = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 4) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto sizeWrite = __sprintf(reinterpret_cast<char*>(bufHeader.data_), SIZE_BUFFER, "Authorization: Bearer %s", _request.Authorization.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite < 0, throwable::failure(eRESULT::FAIL));
//
//	// 4) Header를 설정한다 - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8", reinterpret_cast<char*>(bufHeader.data_) }; 
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//	//std::vector<std::string_view> vector_accept{ "*/*", "application/json" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		str_object,
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::kakao::RESPONSE_BANK_STATUS	response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->SetResponse([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if(_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code	 = eRESPONSE_CODE::FAIL_;
//			response.rsp_message = _msg.status_message;
//
//			// - buffer를 clear한다.
//			response.buf_response.clear();
//
//			// Return)
//			return;
//		}
//
//		// - 마지막을 null로 바꾼다.
//		_msg.contents[0].append<char>(0);
//
//		// - buffer를 넣는다.
//		response.buf_response = _msg.contents[0];
//
//		// declare) 
//		rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		// - Parsing한다.
//		const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto	str_name	 = (*iter).name.GetString();
//			auto&	value	 = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//			}
//
//			// - "api_tran_id"
//			else if (strcmp(str_name, "api_tran_id") == 0)
//			{
//				response.api_tran_id = value.GetString();
//			}
//			// - "api_tran_dtm"
//			else if (strcmp(str_name, "api_tran_dtm") == 0)
//			{
//				response.api_tran_dtm = from_string_date_time_millsec(value.GetString());
//			}
//			// - "res_list"
//			else if (strcmp(str_name, "res_list") == 0)
//			{
//				for (auto iterRes = value.Begin(); iterRes != value.End(); ++iterRes)
//				{
//					// declare) 
//					http_api::kakao::RESPONSE_BANK_STATUS::BANK_STATUS	bankStatus;
//
//					for (auto iterNest = iterRes->MemberBegin(); iterNest != iterRes->MemberEnd(); ++iterNest)
//					{
//						auto	strName_Nested = iterNest->name.GetString();
//						auto&	value_Nested = iterNest->value;
//
//						// - "bank_code_std"
//						if (strcmp(strName_Nested, "bank_code_std") == 0)
//						{
//							bankStatus.bank_code_std = __from_string<int>(value_Nested.GetString());
//						}
//						// - "bank_name"
//						else if (strcmp(strName_Nested, "bank_name") == 0)
//						{
//							convert_utf8_to_multibyte(bankStatus.bank_name, value_Nested.GetString());
//						}
//						// - "bank_status"
//						else if (strcmp(strName_Nested, "bank_status") == 0)
//						{
//							bankStatus.bank_status = get_enum_OAUTH_BANK_STATUS(value_Nested.GetString());
//						}
//					}
//
//					// - ...
//					response.res_list.push_back(bankStatus);
//				}
//			}
//		}
//	});
//
//	// 8) request를 요청한다.
//	prequest_http->request();
//
//	// 9) 완료를 기다린다.
//	prequest_http->wait();
//
//	// return) 
//	return	response;
//}


}