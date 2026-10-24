//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                       tutorials socket - mail(SMTP)                       *
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
//-----------------------------------------------------------------------------
//
// tutorial.socket.mail
//
//  net::email::smtp로 메일을 보내는 방법
//
//  1. net::email::smtp
//     - start(context)로 SMTP 서버와 보안 방식, 인증 정보를 설정한다.
//     - send(sREQUEST)로 메일을 보낸다. send()는 바로 리턴하고 결과는
//       sREQUEST::complete(또는 on_email_sent())로 한 번 전달된다.
//     - 메일 하나마다 접속해서 보내고 끊는다. 동시에 'max_concurrent_socket'개까지
//       접속하고 나머지는 queue에 넣어 차례로 보낸다.
//     - stop()을 호출하면 보내지 않은 메일은 CANCEL로 알린다.
//
//  2. 보안 ("security")
//     - "ssl"      : 접속하자마자 TLS (465)
//     - "starttls" : 평문으로 접속한 후 STARTTLS (587). 서버가 지원하지 않으면 보내지 않는다.
//     - "none"     : 암호화하지 않는다. (내부 relay 서버용. 인증 정보는 보낼 수 없다.)
//     - 서버 인증서는 system CA 저장소로 검증하고 "host_name"과 비교한다.
//
//  3. 인증 ("auth")
//     - 서버가 지원하는 방식 중에서 XOAUTH2(oauth2_token이 있을 때) > PLAIN > LOGIN 순으로 고른다.
//     - gmail/outlook은 계정 비밀번호 대신 '앱 비밀번호'나 OAuth2 token을 사용한다.
//
//  4. context 설정
//
//       {
//           "remote_endpoint": { "address": "smtp.gmail.com", "port": 465 },
//           "security": "ssl",
//           "email_server_domain": "mydomain.com",     // EHLO에 쓰는 이 서버의 이름
//           "from": "noreply@mydomain.com",            // 기본 보내는 사람
//           "from_name": "CGDK",
//           "auth": { "name": "...", "password": "..." },
//           "timeout": 30
//       }
//
//  실행) tutorial.socket.mail <smtp server> <port> <security> <user> <from> <to>
//     - 비밀번호는 환경변수 SMTP_PASSWORD로 넘긴다. (명령줄에 남지 않도록)
//     - ex) set SMTP_PASSWORD=app-password
//           tutorial.socket.mail smtp.gmail.com 465 ssl me@gmail.com me@gmail.com you@example.com
//
//-----------------------------------------------------------------------------
#include "cgdk/sdk10/net.socket.mail.h"

#include <iostream>
#include <cstdlib>
#include <future>

using namespace CGDK;

// - UTF-8 문자열을 메일 본문 buffer로 만든다. (byte 그대로 넣는다.)
static shared_buffer make_text(std::string_view _text)
{
	auto buf = alloc_shared_buffer(_text.size() + 1);
	buf.append(_text.size(), _text.data());
	return buf;
}


// - 환경변수 (없으면 빈 문자열)
static std::string get_env(const char* _name)
{
#if defined(_WIN32)
	char* value = nullptr;
	std::size_t size = 0;
	if (_dupenv_s(&value, &size, _name) != 0 || value == nullptr)
		return std::string();
	std::string result(value);
	free(value);
	return result;
#else
	const char* value = std::getenv(_name);
	return (value != nullptr) ? std::string(value) : std::string();
#endif
}


//-----------------------------------------------------------------------------
// 1. smtp
//    - on_email_sent()를 재정의하면 모든 메일의 결과를 한 곳에서 받을 수 있다.
//      (기본 구현은 sREQUEST::complete를 호출한다.)
//-----------------------------------------------------------------------------
class tutorial_smtp : public net::email::smtp
{
protected:
	// (v) 메일을 보내기 시작할 때
	virtual void on_email_sending(const net::email::sREQUEST& _request) override
	{
		std::cout << " > sending to '"sv << _request.to << "' ("sv << _request.subject << ")"sv << std::endl;
	}

	// (v) 메일의 결과 (io thread에서 호출되므로 오래 걸리는 일을 하지 않는다.)
	virtual void on_email_sent(const net::email::sRESULT& _result) override
	{
		if (_result.result == eRESULT::SUCCESS)
			std::cout << " @ sent to '"sv << _result.request.to << "' : "sv << _result.message << std::endl;
		else
			std::cout << " @ fail to send to '"sv << _result.request.to << "' (result: 0x"sv << std::hex << static_cast<uint32_t>(_result.result) << std::dec << ", reply: "sv << _result.reply_code << ") "sv << _result.message << std::endl;

		// - 서버와 주고받은 내용 (인증 정보와 본문은 남지 않는다.)
		for (const auto& iter : _result.vector_result)
			std::cout << "     "sv << iter << std::endl;

		// - complete를 호출한다.
		net::email::smtp::on_email_sent(_result);
	}
};


//-----------------------------------------------------------------------------
// 2. main
//-----------------------------------------------------------------------------
int main(int _argc, char* _argv[])
{
	// check)
	if (_argc < 7)
	{
		std::cout << "usage) tutorial.socket.mail <smtp server> <port> <security: ssl|starttls|none> <user> <from> <to>" << std::endl;
		std::cout << "       password: environment variable SMTP_PASSWORD" << std::endl;
		return 0;
	}

	// declare)
	std::string server = _argv[1];
	int port = std::atoi(_argv[2]);
	std::string security = _argv[3];
	std::string user = _argv[4];
	std::string from = _argv[5];
	std::string to = _argv[6];
	std::string password = get_env("SMTP_PASSWORD");

	// 1) smtp 객체를 만든다.
	auto psmtp = make_own<tutorial_smtp>();

	// 2) 설정해서 시작한다.
	context context_setting;
	context_setting["remote_endpoint"]["address"] = server;
	context_setting["remote_endpoint"]["port"] = port;
	context_setting["security"] = security;
	context_setting["email_server_domain"] = "tutorial.cgdk";
	context_setting["from"] = from;
	context_setting["from_name"] = "CGDK 튜토리얼";
	context_setting["timeout"] = 30;

	if (!password.empty() && security != "none")
	{
		context_setting["auth"]["name"] = user;
		context_setting["auth"]["password"] = password;
	}

	try
	{
		psmtp->start(context_setting);
	}
	catch (throwable::failure& _e)
	{
		std::cout << " @ fail to start smtp (" << _e.description() << ")" << std::endl;
		return 1;
	}

	// 3) text 메일을 보낸다. (결과는 complete로 한 번 전달된다.)
	std::promise<eRESULT> promise_text;
	{
		net::email::sREQUEST request;
		request.to = to;
		request.subject = "CGDK 메일 튜토리얼 - text";
		request.text = make_text("안녕하세요.\r\nCGDK net::email::smtp로 보낸 메일입니다.\r\n");
		request.complete = [&promise_text](const net::email::sRESULT& _result) { promise_text.set_value(_result.result); };

		psmtp->send(std::move(request));
	}

	// 4) html 메일을 보낸다. (받는 사람을 여럿 지정할 수 있다: to, cc_list, bcc_list)
	std::promise<eRESULT> promise_html;
	{
		net::email::sREQUEST request;
		request.to = to;
		request.subject = "CGDK 메일 튜토리얼 - html";
		request.content_type = "text/html";
		request.text = make_text("<html><body><h3>CGDK</h3><p>인증 코드: <b>123456</b></p></body></html>");
		request.complete = [&promise_html](const net::email::sRESULT& _result) { promise_html.set_value(_result.result); };

		psmtp->send(std::move(request));
	}

	// 5) 결과를 기다린다.
	auto result_text = promise_text.get_future().get();
	auto result_html = promise_html.get_future().get();

	std::cout << " - text: " << ((result_text == eRESULT::SUCCESS) ? "success" : "fail") << ", html: " << ((result_html == eRESULT::SUCCESS) ? "success" : "fail") << std::endl;

	// 6) 정지한다.
	psmtp->stop();

	// return)
	return (result_text == eRESULT::SUCCESS && result_html == eRESULT::SUCCESS) ? 0 : 1;
}
