//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                       tutorials execute - ex.setting                      *
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
#include <atomic>

// 2) CGDK header file
#include "cgdk/sdk10/system.object.h"

// 3) header files
#include <iostream>

// 4) namespace
using namespace CGDK;


//-----------------------------------------------------------------------------
//
// tutorial.execute.ex04.setting
//
//  설명)
//  실행 관련 객체들의 설정은 json 설정 파일(context)로 지정할 수 있다.
//  (이 예제는 'setting.json'을 읽어 사용한다.)
//
//  1) system_executor   : system_executor::initialize_instance(context)
//     "system_executor": {
//        "thread_count": 4,                 // 스레드 개수 (숫자 또는 "auto")
//        "enable_thread_observer": true,    // 스레드 감시(hang 감지) 사용 여부
//        "dump": { ... }                    // dump 설정 (아래 4) 참조)
//     }
//
//  2) executor          : executor::queue_list_threads의 start(context)
//     { "thread_count": 2 }
//
//  3) schedulable       : schedulable::executable_function<> 등의 start(context)
//     { "interval": 300 }                   // 실행 간격 (ms)
//     { "tick": 300000000 }                 // 실행 간격 (chrono::tick 단위. 'interval'이 우선한다.)
//     { "overrun": "catch_up" }             // 실행이 늦어져 다음 실행 시각이 지났을 때: "catch_up"(밀린 실행을 바로 실행, 횟수 보장) / "skip"(밀린 실행은 건너뛰고 다음 간격에 실행)
//
//  4) dump              : system_executor의 "dump" 항목 또는 dump_manager::initialize_instance(context)
//     "dump": {
//        "enable": true,                    // crash가 발생했을 때 dump를 남길지 여부
//        "path": "dumps/setting",           // dump 경로 (폴더가 없으면 dump를 남길 때 생성된다.)
//        "count_limit": 10,                 // 남길 crash dump의 최대 개수 (0이면 제한 없음)
//        "dump_type": "mini"                // "mini" / "detail" / "full" / "default"
//     }
//
//  !) service는 start(context)할 때 "system_executor" 항목과 최상위의 "dump" 항목을
//     자동으로 적용한다.
//
//-----------------------------------------------------------------------------
const char* to_string_dump_type(eDUMP_CONTEXT _type) noexcept
{
	switch (_type)
	{
	case eDUMP_CONTEXT::MINI:	return "mini";
	case eDUMP_CONTEXT::DETAIL:	return "detail";
	case eDUMP_CONTEXT::FULL:	return "full";
	default:					return "unknown";
	}
}

int main()
{
	// ------------------------------------------------------------------
	// 0. json 설정 파일을 읽는다.
	// ------------------------------------------------------------------
	if (std::filesystem::exists("setting.json") == false)
	{
		std::cout << "(!) 'setting.json' is not found (run in the tutorial folder)" << std::endl;
		return 0;
	}

	context setting;
	setting.from_json(api::file::read_file("setting.json"sv));

	// ------------------------------------------------------------------
	// 1. system_executor
	//    - "system_executor" 항목으로 스레드 개수, 스레드 감시, dump를 설정한다.
	//    - system_executor는 처음 사용될 때 기본값으로 시작되므로
	//      다른 실행 객체를 사용하기 전에 가장 먼저 초기화해야 한다.
	// ------------------------------------------------------------------
	{
		system_executor::initialize_instance(setting);

		std::cout << "1. system_executor" << std::endl;
		std::cout << "   thread_count : " << system_executor::thread_count() << std::endl;
	}

	// ------------------------------------------------------------------
	// 2. executor
	//    - 직접 생성한 executor도 start(context)로 스레드 개수를 설정할 수 있다.
	// ------------------------------------------------------------------
	{
		// 1) executor를 생성하고 설정으로 시작한다.
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start(context(setting.find("executor"sv)));

		std::cout << "2. executor" << std::endl;
		std::cout << "   thread_count : " << pexecutor->reserved_size() << std::endl;

		// 2) executor에서 작업을 실행해 본다.
		std::atomic<int> done{ 0 };
		for (int i = 0; i < 100; ++i)
			pexecutor->post(make_executable([&done] { ++done; }));

		while (done < 100)
			api::sleep_for(1ms);

		std::cout << "   executed     : " << done << " jobs" << std::endl;

		// !) own_ptr이므로 블록이 끝나면 destroy된다.
	}

	// ------------------------------------------------------------------
	// 3. schedulable
	//    - start(context)의 "interval"(ms)로 실행 간격을 설정한다.
	// ------------------------------------------------------------------
	{
		std::atomic<int> count_execute{ 0 };

		// 1) schedulable을 생성하고 실행할 함수를 설정한다.
		auto pschedulable = make_own<schedulable::executable_function<>>();
		pschedulable->set_function([&count_execute] { ++count_execute; });

		// 2) 설정으로 시작한다.
		context context_schedulable(setting.find("schedulable"sv));
		pschedulable->start(context_schedulable);

		// 3) 설정된 실행 간격
		auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(pschedulable->execute_interval());
		auto overrun = pschedulable->overrun_policy();

		// 4) 1초 동안 실행되게 둔다.
		api::sleep_for(1s);
		pschedulable.reset();

		std::cout << "3. schedulable" << std::endl;
		std::cout << "   interval     : " << interval.count() << "ms" << std::endl;
		std::cout << "   overrun      : " << to_string(overrun) << std::endl;
		std::cout << "   executed     : " << count_execute << " times in 1 sec" << std::endl;
	}

	// ------------------------------------------------------------------
	// 4. dump
	//    - "system_executor"의 "dump" 항목이 1.에서 적용되었다.
	//    - dump를 남기면 설정된 경로에 남는다.
	// ------------------------------------------------------------------
	{
		auto pdump_manager = dump_manager::instance();

		std::cout << "4. dump" << std::endl;
		std::cout << "   enable       : " << (pdump_manager->is_exception_handling() ? "true" : "false") << std::endl;
		std::cout << "   path         : " << pdump_manager->get_dump_path() << std::endl;
		std::cout << "   count_limit  : " << pdump_manager->get_dump_count_limit() << std::endl;
		std::cout << "   dump_type    : " << to_string_dump_type(pdump_manager->get_dump_type()) << std::endl;

		// - snap dump를 남겨 경로를 확인한다.
		auto result = snap_dump();
		std::cout << "   snap_dump    : " << (result ? "ok" : "fail") << std::endl;

		std::filesystem::path path_dump(pdump_manager->get_dump_path());
		if (std::filesystem::exists(path_dump))
		{
			for (auto& iter : std::filesystem::directory_iterator(path_dump))
				std::cout << "      " << iter.path().filename().string() << " (" << iter.file_size() << " bytes)" << std::endl;
		}
	}

	// return)
	return 0;
}
