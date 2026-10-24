#include "pch.h"


int main(int /*argc*/, char* /*argv*/[])
{
	// 1) [콘솔 로거] 객체를 생성한다.
	auto plogger_console = make_own<logger_console>();

	// 2) [콘솔 로거] 객체를 초기화한다.
	plogger_console->initialize();

	// 3) [콘솔 로거] 객체를 등록한다.
	service::get_logger()->register_log_targetable(plogger_console);

	try
	{
		// 4) [서비스] 객체를 생성한다.
		g_pservice = make_own<service>();

		// declare)
		context context_start;

		// 5) [설정 파일] 읽기
		context_start.from_json(api::file::read_file(CONFIGURE_FILE));

		// 6) [서비스] '초기화'한다.
		g_pservice->initialize(context_start);

		// 임시) [서비스] '시작'은 요청은 [컨트롤 서버]에서 시작 메시지를 받았을 때 할 수 있다.
		g_pservice->start(context_start);

		// 7) 종료 될때까지 대기한다. (1초 단위로 ctrl-X키의 입력을 확인한다.)
		while (g_pservice->wait_destroy(1s) == false)
		{
			// check) ctrl-X를 입력하면 종료한다.
			if (_kbhit())
			{
				auto ch = _getch();

				if (ch == 24)
				{
					break;
				}
				else if (ch == 'A' || ch == 'a')
				{
					factory::manager::print_statistics();
				}
			}
		}
	}
	catch (...)
	{
	}

	// 7) [서비스] 객체를 종료한다.
	g_pservice.destroy();

	// return) 
	return 0;
}
