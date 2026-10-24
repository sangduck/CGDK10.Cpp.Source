//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials execute - ex.crash dump                     *
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
// 1) standard header file
#include <filesystem>
#include <future>

// 2) CGDK header file
#include "cgdk/sdk10/system.object.h"

// 3) header files
#include <iostream>

// 4) namespace
using namespace CGDK;


//-----------------------------------------------------------------------------
//
// tutorial.execute.ex02.crash dump
//
//  설명)
//  CGDK의 thread(api::thread)나 executor에서 실행 중인 코드에서 crash(Access Violation,
//  Divide by zero 등)가 발생하면 dump_manager가 dump 파일('crash_...')을 남긴 후
//  throwable::system_exception 예외를 던진다.
//  따라서 try-catch로 이 예외를 잡으면 process가 죽지 않고 계속 동작할 수 있다.
//  (stack unwinding이 되므로 지역 객체의 소멸자도 정상적으로 호출된다.)
//
//  1) dump 파일은 dump_manager::set_dump_path()로 설정한 경로에 남는다.(기본값 ".")
//  2) enable_exception_handling(true/false)로 crash 처리 여부를 설정할 수 있다.
//  3) Windows는 같은 위치(주소)에서 다시 발생한 crash는 dump를 다시 남기지 않고
//     발생 횟수(DUMP_INFO::count_times)만 증가시킨다.
//     (Linux는 매번 gcore로 core 파일('crash_....<pid>')을 남기며 발생 정보는 기록하지 않는다.)
//
//  주의)
//  - Release 빌드에서만 crash를 예외로 받을 수 있다.
//    (Windows는 /EHa, Linux는 -fnon-call-exceptions 옵션으로 빌드되어야 한다.)
//    Debug 빌드(/EHsc)는 crash가 나면 process가 그대로 종료된다.
//  - Windows는 CGDK의 thread(api::thread, executor의 thread)에서만 동작한다.
//    main thread나 std::thread에서는 예외로 변환되지 않는다.
//
//-----------------------------------------------------------------------------
// 1) crash를 발생시키는 함수들 (volatile: 최적화로 제거되지 않도록)
static volatile int* volatile g_pnull = nullptr;
static volatile int g_zero = 0;

void crash_access_violation()
{
	*g_pnull = 1;
}

int crash_divide_by_zero()
{
	return 10 / g_zero;
}

// 2) 소멸자 호출을 확인하기 위한 객체
struct local_object
{
	~local_object() { std::cout << "     - local_object destructed (stack unwound)" << std::endl; }
};

// 3) 남겨진 crash dump 파일의 수
size_t count_crash_dump(const std::filesystem::path& _path)
{
	size_t count = 0;
	if (!std::filesystem::exists(_path))
		return count;
	for (auto& iter : std::filesystem::directory_iterator(_path))
		if (iter.path().filename().string().rfind("crash_", 0) == 0)
			++count;
	return count;
}

// 4) crash를 발생시키고 예외를 잡는다.
void run_crash(bool _is_divide_by_zero)
{
	try
	{
		local_object temp;

		if (_is_divide_by_zero)
			std::cout << crash_divide_by_zero() << std::endl;
		else
			crash_access_violation();

		std::cout << "     - not crashed?" << std::endl;
	}
	catch (const throwable::system_exception& _e)
	{
	#if defined(_WIN32)
		std::cout << "  (!) caught system_exception (code: 0x" << std::hex << _e.m_exception_record.ExceptionCode << std::dec << ")" << std::endl;
	#elif defined(__linux__)
		std::cout << "  (!) caught system_exception (signal: " << _e.m_sig << ")" << std::endl;
	#endif
	}
	catch (...)
	{
		std::cout << "  (!) caught unknown exception" << std::endl;
	}
}

int main()
{
#if defined(_DEBUG)
	// check) Debug 빌드에서는 crash를 예외로 받을 수 없다.
	std::cout << "(!) Debug build : crash is not converted to exception. run the Release build." << std::endl;
	return 0;
#else
	// 1) dump 파일을 남길 경로를 설정한다.
	//    (폴더가 없으면 dump를 남길 때 중간 폴더까지 자동으로 생성된다.)
	std::filesystem::path path_dump = "dumps/crash";
	dump_manager::instance()->set_dump_path(path_dump.string());

	// 2) crash 처리를 켠다.(Release 빌드는 기본값이 true)
	enable_exception_handling(true);

	// 3) system_executor를 시작한다.(Linux는 이때 signal handler가 설치된다.)
	system_executor::instance();

	// 4) api::thread에서 Access Violation
	std::cout << "[1] access violation on api::thread" << std::endl;
	{
		auto pthread = make_object<api::thread>();
		pthread->begin_thread([] { run_crash(false); });
		pthread->wait();
	}
	std::cout << "     dump count: " << count_crash_dump(path_dump) << std::endl;

	// 5) api::thread에서 Divide by zero
	std::cout << "[2] divide by zero on api::thread" << std::endl;
	{
		auto pthread = make_object<api::thread>();
		pthread->begin_thread([] { run_crash(true); });
		pthread->wait();
	}
	std::cout << "     dump count: " << count_crash_dump(path_dump) << std::endl;

	// 6) executor(system_executor)에서 Access Violation
	//    - [1]과 같은 위치에서 crash가 발생하므로 Windows는 dump를 다시 남기지 않는다.
	std::cout << "[3] access violation on system_executor (same crash address as [1])" << std::endl;
	{
		std::promise<void> promise_done;
		system_executor::post(make_executable([&promise_done] { run_crash(false); promise_done.set_value(); }));
		promise_done.get_future().wait();
	}
	std::cout << "     dump count: " << count_crash_dump(path_dump) << std::endl;

	// 7) 남겨진 dump 파일들
	std::cout << "[dump files in '" << path_dump.string() << "']" << std::endl;
	for (auto& iter : std::filesystem::directory_iterator(path_dump))
		if (iter.path().filename().string().rfind("crash_", 0) == 0)
			std::cout << "   " << iter.path().filename().string() << " (" << iter.file_size() << " bytes)" << std::endl;

	// 8) crash 발생 정보 (위치별 발생 횟수)
	std::cout << "[crash info]" << std::endl;
	for (auto& iter : dump_manager::instance()->status_snap())
		std::cout << "   times: " << iter.count_times << "  dump: " << iter.dump_filename << std::endl;

	// return)
	return 0;
#endif
}
