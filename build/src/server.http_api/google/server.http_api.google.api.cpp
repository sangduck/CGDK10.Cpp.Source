#include "pch.h"

namespace CGDK
{

//// 1) google 사용자
//http_api::google::RESPONSE_AUTHORIZE http_api::google::request(const REQUEST_AUTHORIZE& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// google 인증 - 코드 받기
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://kauth.google.com/oauth/authorize
//	//	 3) HTTP Method		GET
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded; charset=UTF-8"
//	//   5) Parameters
//	//		- client_id			(필수)	앱 생성시 발급 받은 REST API 키.
//	//		- redirect_uri		(필수)	코드가 리다이렉트 된 URI. 설정>일반>웹>사이트 도메인에서 설정한 각각의 도메인에 설정>일반>웹>Redirect Path를 붙인 URI
//	//		- response_type		(필수)	code 문자열 값으로 고정
//	//		- state						Cross-site request Forgery 공격을 보호하기 위한 임의의 고유한 문자열, 리다이렉트시 해당값이 전달됨.
//	//		- encode_state				state가 encode되어 있는지에 대한 유무(boolean). true(y또는 1)일 경우 state 전달시 decoded state로 전달.
//	//									false(n또는0)일 경우 별도의 decode를 하지 않고 state를 그대로 전달. default false
//	//
//	//   요청)	GET /oauth/authorize?client_id={app_key}&redirect_uri={redirect_uri}&response_type=code HTTP/1.1 
//	//			Host: kauth.google.com
//	//
//	//   결과)	HTTP / 1.1 302 Found
//	//				Content - Length: 0
//	//				Location : {redirect_uri} ? code = { authorize_code }
//	//
//	// ------------------------------------------------------------------
//	// declare) 
//	const std::size_t SIZE_BUFFER = 8192;
//
//	// check)
//	CGASSERT_ERROR(_request.str_url.empty() == false);
//
//	// check)
//	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//	
//	// declare)
//	auto bufObject = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	//auto	sizeWrite = __sprintf(reinterpret_cast<char*>(bufObject.data()), SIZE_BUFFER, "/oauth/authorize?client_id=%s&redirect_uri=%s&response_type=code&state=%s", _request.client_id.c_str(), _request.redirect_uri.c_str(), _request.state.c_str());
//	auto sizeWrite = __sprintf(bufObject.data<char>(), SIZE_BUFFER, "/oauth/authorize?client_id=%s&redirect_uri=%s&response_type=code", _request.client_id.c_str(), _request.redirect_uri.c_str());
//
//	// check) sprintf가 오류가 났으면 Exception을 던진다.
//	THROW_IF(sizeWrite<0, throwable::failure(eRESULT::FAIL));
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::GET_;
//
//	// declare) 
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// declare) 
//	shared_buffer buf_option;
//
//	// 5) Accept 받을 것을 설정.
//	std::vector<std::string_view> vector_accept{ "*/*", "text/plain" };
//
//	// 6) request를 Set한다.
//	prequest_http->set_request(
//		http_method,
//		_request.str_url,
//		443,
//		"HTTP/1.1",
//		bufObject.data<char>(),
//		std::string_view(),
//		std::move(vector_header),
//		buf_option,
//		std::move(vector_accept),
//		1		// 0: normal, 1:ssl
//	);
//
//	// declare) 
//	http_api::google::RESPONSE_AUTHORIZE response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code = eRESPONSE_CODE::FAIL_;
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
//		//// declare) 
//		//rapidjson::GenericDocument<rapidjson::UTF8<char>> json_data;
//
//		//// - Parsing한다.
//		//const auto&	result = json_data.Parse(_msg.contents[0].data_);
//
//		//// check) Parsing에 실패했으면 false를 리턴한다.
//		//RETURN_IF(result.IsNull())
//
//		//// - 값을 설정한다.
//		//for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		//{
//		//	auto	str_name	 = (*iter).name.GetString();
//		//	auto&	value	 = (*iter).value;
//
//		//	// - rsp_code
//		//	if(strcmp(str_name, "rsp_code")==0)
//		//	{
//		//		response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//		//	}
//		//	// - rsp_message
//		//	else if(strcmp(str_name, "rsp_message") == 0)
//		//	{
//		//		convert_utf8_to_multibyte(response.rsp_message, value.GetString());
//		//	}
//
//		//	//AddJSONMember<char, rapidjson::UTF8<char>>(*this, iter);
//		//}
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
//http_api::google::RESPONSE_TOKEN http_api::google::request(const REQUEST_TOKEN& _request)
//{
//	// ------------------------------------------------------------------
//	// 
//	// google 인증 - 사용자 토큰 받기
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://kauth.google.com/oauth/token
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
//	//   5) Parameters
//	//		- grant_type		(필수)	authorization_code로 고정
//	//		- client_id			(필수)	앱 생성시 발급 받은 REST API키.
//	//		- redirect_uri		(필수)	코드가 리다이렉트 된 URI. 설정>일반>웹>사이트 도메인에서 설정된 각각의 도메인에 설정>일반>웹>Redirect Path를 붙인 URI
//	//		- code				(필수)	위 코드 받기에서 발급 받은 인증된 코드
//	//		- client_secret				설정>일반>Client Secret에서 생성된 client_secret 코드 ACTIVE 상태인 경우 필수로 설정해야 함.
//	//
//	//	요청)	curl -v -X POST https://kauth.google.com/oauth/token 
//	//			 -d 'grant_type=authorization_code' 
//	//			 -d 'client_id={app_key}' 
//	//			 -d 'redirect_uri={redirect_uri}' 
//	//			 -d 'code={authorize_code}'	
//	//
//	//	결과)	HTTP/1.1 200 OK
//	//			Content-Type: application/json;charset=UTF-8
//	//			{
//	//				"access_token":"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
//	//				"token_type":"bearer",
//	//				"refresh_token":"yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy",
//	//				"expires_in":43199,
//	//				"scope":"Basic_Profile"
//	//			}
//	//
//	// ------------------------------------------------------------------
//	// declare) 
//	const size_t SIZE_BUFFER = 8192;
//
//	// check)
//	CGASSERT_ERROR(_request.str_url.empty() == false);
//
//	// check)
//	THROW_IF(_request.str_url.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));
//
//	// 1) CGWeb::CHTTPRequest 객체를 생성한다.
//	auto prequest_http = make_object<query::http::query>();
//
//	// 2) HTTP URL을 설정한다.(URL중 Path부분만...)
//	auto str_object = "/oauth/2.0/token";
//
//	// 3) HTTP Method를 설정한다.
//	auto http_method = eHTTP_METHOD::POST_;
//
//	// 4) Header를 설정한다. - Content_Type를 설정한다. (Header부분에 Content_Type이 들어간다.)
//	std::vector<std::string_view> vector_header{ "Content-Type: application/x-www-form-urlencoded; charset=UTF-8" };
//
//	// 5) parameter를 설정한다.
//	auto buf_option	 = alloc_shared_buffer(SIZE_BUFFER * sizeof(char));
//	buf_option.append<TEXT_WEB>("code=");			buf_option.append<TEXT_WEB>(_request.code);
//	buf_option.append<TEXT_WEB>("&client_id=");		buf_option.append<TEXT_WEB>(_request.client_id);
//	buf_option.append<TEXT_WEB>("&redirect_uri=");	buf_option.append<TEXT_WEB>(_request.redirect_uri);
//	buf_option.append<TEXT_WEB>("&grant_type=authorization_code");
//
//	// 6) Options
//	if(_request.client_secret.empty()==false)
//	{
//		buf_option.append<TEXT_WEB>("&client_secret=");	buf_option.append<TEXT_WEB>(_request.client_secret);
//	}
//	buf_option[buf_option.size()] = 0;
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
//	http_api::google::RESPONSE_TOKEN response;
//
//	// 7) 기본값을 설정...
//	response.rsp_code = eRESPONSE_CODE::A0000_;
//	response.rsp_message.clear();
//
//	// 8) CGWeb::CHTTPRequest 객체에 응답이후 처리할 내용을 Lambda함수를 사용해 설정해준다.
//	prequest_http->set_function([&](sMESSAGE_HTTP_RESPONSE& _msg)
//	{
//		// check) Result가 0이면 실패이므로 끝낸다.
//		if (_msg.status_code != eHTTP_STATUS_CODE::OK_)
//		{
//			// - 내부처리 중 Error을 명시
//			response.rsp_code = eRESPONSE_CODE::FAIL_;
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
//		const auto&	result = json_data.Parse(_msg.contents[0].data());
//
//		// check) Parsing에 실패했으면 false를 리턴한다.
//		RETURN_IF(result.IsNull())
//
//		// - 값을 설정한다.
//		for (auto iter = json_data.MemberBegin(); iter != json_data.MemberEnd(); ++iter)
//		{
//			auto str_name = (*iter).name.GetString();
//			auto& value = (*iter).value;
//
//			// - rsp_code
//			if(strcmp(str_name, "rsp_code")==0)
//			{
//				response.rsp_code = get_value_RESPONSE_CODE(value.GetString());
//			}
//			// - rsp_message
//			else if(strcmp(str_name, "rsp_message") == 0)
//			{
//				response.rsp_message = convert_utf8_to_multibyte(value.GetString());
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
//	return response;
//}
//http_api::google::RESPONSE_LOGOUT http_api::google::request(const REQUEST_LOGOUT& /*_request*/)
//{
//	// ------------------------------------------------------------------
//	// 
//	// google 인증 - 로그아웃
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://kapi.google.com/v1/user/logout
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
//	//   5) Parameters
//	//		- Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxx
//	//
//	//	요청)	curl -v -X POST https://kapi.google.com/v1/user/logout 
//	//			 -H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
//	//
//	//	결과)	HTTP / 1.1 200 OK
//	//				Content - Type: application / json; charset = UTF - 8
//	//			{
//	//				"id":123456789
//	//			}
//	//
//	// ------------------------------------------------------------------
//
//	// declare) 
//	http_api::google::RESPONSE_LOGOUT response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_SIGNUP http_api::google::request(const REQUEST_SIGNUP& /*_request*/)
//{
//	// ------------------------------------------------------------------
//	// 
//	// google 인증 - 앱연결
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://kapi.google.com/v1/user/signup
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/json;charset=UTF-8"
//	//   5) Parameters
//	//		- Authorization: Bearer {access_token}
//	//	    - Content-Type: application/x-www-form-urlencoded;charset=utf-8
//	//
//	//	요청)	curl - v - X POST https ://kapi.google.com/v1/user/signup 
//	//				-H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" 
//	//				--data-urlencode 'properties={"age":"23", "gender":"female"}'
//	//
//	//	결과)	HTTP / 1.1 200 OK
//	//			Content-Type: application/json;charset=UTF-8
//	//			{
//	//				"id":123456789
//	//			}
//	//
//	// ------------------------------------------------------------------
//	// declare) 
//	http_api::google::RESPONSE_SIGNUP response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_UNLINK http_api::google::request(const REQUEST_UNLINK& /*_request*/)
//{
//	// ------------------------------------------------------------------
//	// 
//	// google 인증 - 앱연결해제
//	//
//	// 요청 메시지 URL
//	//   
//	//   2) HTTP URL		https://kapi.google.com/v1/user/unlink HTTP/1.1
//	//	 3) HTTP Method		POST
//	//	 4) Content-Type	"Content-Type: application/x-www-form-urlencoded;charset=utf-8"
//	//   5) Parameters
//	//		- Authorization: googleAK {admin_key}
//	//	    - Content-Type: application/x-www-form-urlencoded;charset=utf-8
//	//
//	//   6) ...
//	//			POST /v1/user/unlink HTTP/1.1
//	//			Host: kapi.google.com
//	//			Authorization : Bearer{ access_token }
//	//
//	//			POST /v1/user/unlink HTTP/1.1
//	//			Host: kapi.google.com
//	//			Authorization: googleAK {admin_key}
//	//
//	//
//	//	요청1)	curl -v -X POST https://kapi.google.com/v1/user/unlink 
//	//			  -H "Authorization: Bearer xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
//	//
//	//	요청)	curl -v -X POST https://kapi.google.com/v1/user/unlink 
//	//			  -H "Authorization: googleAK kkkkkkkkkkkkkkkkkkkkkkkkkkkkkk" 
//	//			  -d 'target_id_type=user_id&target_id=123456789'
//	//
//	//	결과)	HTTP/1.1 200 OK
//	//			Content-Type: application/json;charset=UTF-8
//	//			{
//	//			    "id":123456789
//	//			}
//	//
//	// ------------------------------------------------------------------
//	// declare) 
//	http_api::google::RESPONSE_UNLINK response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_ME http_api::google::request(const REQUEST_ME& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_ME response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_UPDATE_PROFILE http_api::google::request(const REQUEST_UPDATE_PROFILE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_UPDATE_PROFILE response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_IDS http_api::google::request(const REQUEST_IDS& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_IDS	response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_ACCESS_TOKEN_INFO http_api::google::request(const REQUEST_ACCESS_TOKEN_INFO& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_ACCESS_TOKEN_INFO response;
//
//	// return) 
//	return response;
//}
//
//// 2) google Talk
//http_api::google::RESPONSE_TAKL_PROFILE http_api::google::request(const REQUEST_TALK_PROFILE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_TAKL_PROFILE	response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_TAKL_SEND http_api::google::request(const REQUEST_TALK_SEND& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_TAKL_SEND response;
//
//	// return) 
//	return response;
//}
//
//// 3) google Pay
//http_api::google::RESPONSE_PAY_READY http_api::google::request(const REQUEST_PAY_READY& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_READY response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_APPROVE http_api::google::request(const REQUEST_PAY_APPROVE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_APPROVE response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_SUBSCRIPTION http_api::google::request(const REQUEST_PAY_SUBSCRIPTION& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_SUBSCRIPTION	response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_CANCEL http_api::google::request(const REQUEST_PAY_CANCEL& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_CANCEL response;
//
//	// return) 
//	return response;
//}
//
//// 3) google Pay
//http_api::google::RESPONSE_PAY_ORDER http_api::google::request(const REQUEST_PAY_ORDER& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_ORDER response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_ORDERS http_api::google::request(const REQUEST_PAY_ORDERS& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_ORDERS response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_STATUS http_api::google::request(const REQUEST_PAY_STATUS& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_STATUS response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PAY_INACTIVE http_api::google::request(const REQUEST_PAY_INACTIVE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PAY_INACTIVE	response;
//
//	// return) 
//	return response;
//}
//
//// 4) google Story
//http_api::google::RESPONSE_STORY_ISSTORYUSER http_api::google::request(const REQUEST_STORY_ISSTORYUSER& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_ISSTORYUSER response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_PROFILE http_api::google::request(const REQUEST_STORY_PROFILE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_PROFILE response;
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_NOTE http_api::google::request(const REQUEST_STORY_NOTE& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_NOTE response;
//
//	// return) 
//	return response;
//}
//
//// 5) google Story
//http_api::google::RESPONSE_STORY_UPLOAD http_api::google::request(const REQUEST_STORY_UPLOAD& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_UPLOAD	response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_PHOTO http_api::google::request(const REQUEST_STORY_PHOTO& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_PHOTO response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_LINKINFO http_api::google::request(const REQUEST_STORY_LINKINFO& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_LINKINFO response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_LINK http_api::google::request(const REQUEST_STORY_LINK& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_LINK response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_MYSTORY http_api::google::request(const REQUEST_STORY_MYSTORY& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_MYSTORY response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_MYSTORIES http_api::google::request(const REQUEST_STORY_MYSTORIES& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_MYSTORIES response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_STORY_DELETE_MYSTROY http_api::google::request(const REQUEST_STORY_DELETE_MYSTROY&/*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_STORY_DELETE_MYSTROY	response;
//
//	// return) 
//	return response;
//}
//
//// 6) google Push
//http_api::google::RESPONSE_PUSH_REGISTER http_api::google::request(const REQUEST_PUSH_REGISTER& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PUSH_REGISTER response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PUSH_TOKENS 	http_api::google::request(const REQUEST_PUSH_TOKENS& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PUSH_TOKENS response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PUSH_DEREGISTER http_api::google::request(const REQUEST_PUSH_DEREGISTER& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PUSH_DEREGISTER response;
//
//	// return) 
//	return response;
//}
//http_api::google::RESPONSE_PUSH_GCM_PUSH_FAIL http_api::google::request(const REQUEST_PUSH_GCM_PUSH_FAIL& /*_request*/)
//{
//	// declare) 
//	http_api::google::RESPONSE_PUSH_GCM_PUSH_FAIL response;
//
//	// return) 
//	return response;
//}


}