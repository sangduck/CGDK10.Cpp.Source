//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket mail classes                        *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{

//-----------------------------------------------------------------------------
// 1. email definitions
//-----------------------------------------------------------------------------
// 1) SMTP 진행 상태
enum class net::email::eSTATE
{
	NONE			 = 0,
	REQUESTED		 = 1,	// 접속 중
	GREETING		 = 2,	// 서버 인사(220) 대기
	SEND_EHLO		 = 3,	// EHLO 응답(250) 대기
	SEND_STARTTLS	 = 4,	// STARTTLS 응답(220) 대기
	SEND_EHLO_TLS	 = 5,	// TLS 시작 후 EHLO 응답(250) 대기
	SEND_AUTH		 = 6,	// AUTH PLAIN / XOAUTH2 응답(235) 대기
	SEND_NAME		 = 7,	// AUTH LOGIN 사용자 이름 요청(334) 대기
	SEND_PASSWORD	 = 8,	// AUTH LOGIN 비밀번호 요청(334) / 결과(235) 대기
	SEND_MAIL_FROM	 = 9,	// MAIL FROM 응답(250) 대기
	SEND_RCPT_TO	 = 10,	// RCPT TO 응답(250) 대기
	REQUEST_DATA	 = 11,	// DATA 응답(354) 대기
	SEND_DATA		 = 12,	// 본문 전송 후 응답(250) 대기
	SEND_QUIT		 = 13,	// QUIT 응답(221) 대기
	DONE			 = 14,
};

// 2) 접속 보안
enum class net::email::eSECURITY
{
	NONE			 = 0,	// 암호화하지 않는다. (25. 내부 relay용)
	SSL				 = 1,	// 접속하자마자 TLS (implicit TLS. 465)
	STARTTLS		 = 2,	// 평문으로 접속한 후 STARTTLS로 TLS를 시작한다. (587/25. 서버가 지원하지 않으면 실패한다.)
};

// 3) 인증 방식 (EHLO 응답의 AUTH에서 고른다.)
enum class net::email::eAUTH_TYPE
{
	NONE			 = 0,
	PLAIN			 = 1,	// AUTH PLAIN
	LOGIN			 = 2,	// AUTH LOGIN
	XOAUTH2			 = 3,	// AUTH XOAUTH2 (oauth2_token)
};

// 4) 인증 정보
struct net::email::sAUTH
{
	std::string		name;
	std::string		password;
	std::string		oauth2_token;	// (XOAUTH2. gmail/outlook 등의 OAuth2 access token. 있으면 password 대신 쓴다.)

	[[nodiscard]] bool empty() const noexcept { return name.empty() && password.empty() && oauth2_token.empty(); }
	void clear() noexcept { name.clear(); password.clear(); oauth2_token.clear(); }
};

// 5) 보낼 메일
//    - from/from_name이 비어 있으면 smtp 설정의 "from"/"from_name"을 쓴다.
//    - auth가 비어 있으면 smtp 설정의 "auth"를 쓴다.
//    - subject, from_name, text는 UTF-8이다. (subject/from_name은 RFC 2047, 본문은 base64로 보낸다.)
//    - complete는 성공/실패에 관계없이 한 번 호출된다. (io thread에서 호출되므로 오래 걸리는 일을 하지 않는다.)
struct net::email::sREQUEST
{
public:
	std::string		from;
	std::string		from_name;
	std::string		to;
	std::vector<std::string> cc_list;
	std::vector<std::string> bcc_list;
	std::string		subject;
	shared_buffer	text;
	std::string		content_type{ "text/plain" };	// ("text/plain" 또는 "text/html". charset은 UTF-8)
	sAUTH			auth;

	std::function<void(const sRESULT&)> complete;

public:
	void clear() noexcept
	{
		from.clear();
		from_name.clear();
		to.clear();
		cc_list.clear();
		bcc_list.clear();
		subject.clear();
		text.clear();
		content_type = "text/plain";
		auth.clear();
		complete = std::function<void(const sRESULT&)>();
	}
};

// 6) 결과
//    - result : SUCCESS / FAIL_TO_CONNECT / TIME_OUT / INVALID_PASSWORD(인증 실패) / NOT_SUPPORT(STARTTLS/AUTH 미지원)
//               / FAIL(서버가 거부. reply_code, vector_result 참고) / CANCEL(중지) / ...
//    - reply_code : 마지막 서버 응답 코드 (예: 250, 535, 550)
//    - vector_result : 서버 응답 줄 (인증 정보는 남기지 않는다.)
struct net::email::sRESULT
{
public:
	net::email::sREQUEST request;

	chrono::time::utc::time_point send_time;
	eRESULT			result = eRESULT::SUCCESS;
	eSTATE			state = eSTATE::NONE;
	eAUTH_TYPE		auth_type = eAUTH_TYPE::NONE;
	int				reply_code = 0;
	std::string		message;
	std::vector<std::string> vector_result;

public:
	void clear() noexcept
	{
		request.clear();
		result = eRESULT::SUCCESS;
		state = eSTATE::NONE;
		auth_type = eAUTH_TYPE::NONE;
		reply_code = 0;
		message.clear();
		vector_result.clear();
	}
};


}
