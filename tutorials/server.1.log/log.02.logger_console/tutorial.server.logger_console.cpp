#include "cgdk/sdk10/server.log.h"

#include <iostream>
#if defined(_WIN32)
	#include <conio.h>
#endif

using namespace CGDK;

#if defined(__cpp_lib_format)
	using std::format;
#else
	using fmt::format;
#endif

int main()
{
	// ------------------------------------------------------------------
	// 설명)
	//
	//  Console창에 LOG를 남길 수도 있다.
	//  간단히 [로그 객체]를 콘솔창용으로 생성하면 된다.
	//  
	//    CCGLogConsole 이 콘솔창 출력용 [로그 객체]이다.
	//
	//  콘솔창 출력 용 [로그 객체]는 특별한 초기화가 필요 없다.
	//
	// ------------------------------------------------------------------
	// ------------------------------------------------------------------
	// 1. 먼저 로그를 남길 대상을 결정하는 대상객체를 생성한다.
	// ------------------------------------------------------------------
	// 1) 콘솔창용 [로그 객체]인 'CCGLogConsole'를 생성한다.
	auto plogger = make_own<logger_console>();

	// 2) 시작한다.
	plogger->initialize();

	// view) 
	std::cout << "create LOG object"sv << std::endl;


	// ------------------------------------------------------------------
	// 2. LOG()함수를 사용하여 로그를 출력한다.
	//    (로그 대상 객체를 plogger로 한다.)
	// ------------------------------------------------------------------

	// 1) 다음과 같이 Log를 출력할 수 있다.
	LOG(plogger, log_type_t::info)		<< log_level_t::normal() << "[A] This paragraph is log"sv;

	// 2) 로그 종류(log_type_t)와 로그 레벨(log_level)에 따라 다른 색상을 출력한다.
	LOG(plogger, log_type_t::info)		<< log_level_t::normal() << "[A-1] INFO Important Log"sv;
	LOG(plogger, log_type_t::info)		<< log_level_t::higher() << "[A-2] Important INFO Important Log"sv;
	LOG(plogger, log_type_t::progress)  << log_level_t::normal() << "[A-3] PROGRESS Log (1)"sv;
	LOG(plogger, log_type_t::progress)	<< log_continue_t()		 << "[A-4] PROGRESS Log (2)"sv;
	LOG(plogger, log_type_t::progress)	<< log_level_t::higher() << "[A-5] Important PROGRESS Log"sv;
	LOG(plogger, log_type_t::error)		<< log_level_t::normal() << "[A-6] ERROR Log"sv;
	LOG(plogger, log_type_t::fatal)		<< log_level_t::normal() << "[A-7] FATAL Log"sv;
	LOG(plogger, log_type_t::warning)	<< log_level_t::normal() << "[A-8] WARNING Log"sv;
	LOG(plogger, log_type_t::debug)		<< log_level_t::normal() << "[A-9] DEBUG Log"sv;
	LOG(plogger, log_type_t::user)		<< log_level_t::normal() << "[A-10] USER Log"sv;
	LOG(plogger, log_type_t::system)	<< log_level_t::normal() << "[A-11] SYSTEM Log"sv;

	// declare) 
	std::wstring_view temp_string(L"\uD14C\uC2A4\uD2B8 \uBB38\uC790\uC5F4."sv);	// "테스트 문자열."

	// 3) 파라미터가 있는 fmt::format
	LOG(plogger, log_type_t::info) << CGDK::format("[B-1] \uC774 \uBB38\uC7A5\uC740 \uBA40\uD2F0\uBC14\uC774\uD2B8 \uD55C\uAE00 \uBB38\uC790\uC5F4 \uB85C\uADF8 [{}]"sv, "STRING"sv);	// "[B-1] 이 문장은 멀티바이트 한글 문자열 로그 [{}]"
	LOG(plogger, log_type_t::info) << format(L"[B-2] \uC774 \uBB38\uC7A5\uC740 \uC720\uB2C8\uCF54\uB4DC \uD55C\uAE00 \uBB38\uC790\uC5F4 \uB85C\uADF8 [{}]"sv, L"STRING"sv);	// "[B-2] 이 문장은 유니코드 한글 문자열 로그 [{}]"
	LOG(plogger, log_type_t::info) << CGDK::format("[B-3] fmt\uB97C \uC0AC\uC6A9\uD55C \uB85C\uADF8 \uCD9C\uB825 {}"sv, 10);	// "[B-3] fmt를 사용한 로그 출력 {}"
	LOG(plogger, log_type_t::info) << format(L"[B-4] fmt\uB97C \uC0AC\uC6A9\uD55C \uB85C\uADF8 \uCD9C\uB825 {} - {}"sv, 10, L"STRING"sv);	// "[B-4] fmt를 사용한 로그 출력 {} - {}"
	LOG(plogger, log_type_t::info) << format(L"[B-5] fmt\uB97C \uC0AC\uC6A9\uD55C \uB85C\uADF8 \uCD9C\uB825 {} - {}"sv, 10, temp_string);	// "[B-5] fmt를 사용한 로그 출력 {} - {}"
	LOG(plogger, log_type_t::info) << format(L"[B-6] fmt\uB97C \uC0AC\uC6A9\uD55C \uB85C\uADF8 \uCD9C\uB825 {0} - {1} - {0}"sv, 10, temp_string);	// "[B-6] fmt를 사용한 로그 출력 {0} - {1} - {0}"

	// 4) LOG_MESSAGE() 객체를 임시로 생성해 operator <<를 사용해 로그 메시지를 남기고 LOG_END()를 << 함으로써 로그를 출력한다.
	LOG(plogger, log_type_t::debug) << "[C-1] This is log"sv << " X: "sv << 10 << " Y: "sv << float(10.0f) << L" Z: "sv << temp_string;

	// 5) 다중 라인 출력
	LOG(plogger, log_type_t::info) << L"[C-2] \uCCAB\uC9F8\uC904 ["sv << L"STRING"sv << "]"sv << log_endl	// "[C-2] 첫째줄 ["
								  << L"[C-3] \uB450\uC9F8\uC904 "sv << 10 << log_endl	// "[C-3] 둘째 줄 "
								  << L"[C-4] \uB9C8\uC9C0\uB9C9\uC904"sv;	// "[C-4] 마지막줄"


	// ------------------------------------------------------------------
	// 3. Performance test
	// ------------------------------------------------------------------
	// 1) get time
	auto tick_start = chrono::tick::now();

	for (int i = 0; i < 10000; ++i)
	{
		LOG(plogger, log_type_t::info) << "test log output to console"sv;
	}

	// 2) get time
	auto tick_diff = chrono::tick::now() - tick_start;

	// 3) 시간을 출력한다.
	std::cout << "elapsed time: "sv << std::chrono::duration_cast<std::chrono::milliseconds>(tick_diff).count() << "ms"sv << std::endl;


	// return) 끝.
	return 0;
}
