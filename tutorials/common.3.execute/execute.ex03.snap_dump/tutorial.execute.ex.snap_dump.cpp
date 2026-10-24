//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials execute - ex.snap dump                      *
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
// tutorial.execute.ex03.snap dump
//
//  설명)
//  snap_dump()함수를 사용하면 process를 멈추지 않고 현재 상태 그대로의 dump를 남길 수 있다.
//  crash가 아닌 상황, 예를 들어 deadlock이나 무한 루프로 thread가 hang이 걸렸을 때
//  그 순간 모든 thread의 call stack을 확인하는 데 사용한다.
//
//  1) snap_dump()                            '<dump path>/snap_...' 이름으로 남긴다.
//  2) snap_dump("file name")                 지정한 파일 이름으로 남긴다.
//  3) snap_dump("file name", eDUMP_CONTEXT)  dump의 종류(MINI/DETAIL/FULL)를 지정한다.
//
//  Windows는 MiniDumpWriteDump로 '.dmp' 파일을 남기고,
//  Linux는 gcore로 core 파일('<file name>.<pid>')을 남긴다.
//  (Linux는 gdb(gcore)가 설치되어 있어야 하며 eDUMP_CONTEXT는 무시된다.)
//
//-----------------------------------------------------------------------------
// 1) hang 상태를 흉내내기 위한 thread 함수
std::atomic<bool> g_is_run{ true };
std::atomic<bool> g_is_hang{ false };

void hang_function()
{
	// - deadlock이나 무한 루프에 빠진 것처럼 계속 돈다.
	g_is_hang = true;
	while (g_is_run)
		api::sleep_for(10ms);
}

int main()
{
	// 1) dump 파일을 남길 경로를 설정한다.
	//    (폴더가 없으면 dump를 남길 때 중간 폴더까지 자동으로 생성된다.)
	std::filesystem::path path_dump = "dumps/snap";
	dump_manager::instance()->set_dump_path(path_dump.string());

	// 2) hang 걸린 thread를 하나 만든다.
	auto pthread = make_object<api::thread>();
	pthread->begin_thread(hang_function);

	while (!g_is_hang)
		api::sleep_for(10ms);

	// 3) 기본 이름으로 snap dump를 남긴다. ('<dump path>/snap_...')
	auto result_1 = snap_dump();
	std::cout << "[1] snap_dump()                       : " << (result_1 ? "ok" : "fail") << std::endl;

	// 4) 파일 이름을 지정해서 snap dump를 남긴다.
	auto str_file = (path_dump / "snap_named.dmp").string();
	auto result_2 = snap_dump(str_file);
	std::cout << "[2] snap_dump(\"" << str_file << "\") : " << (result_2 ? "ok" : "fail") << std::endl;

	// 5) dump의 종류를 지정해서 snap dump를 남긴다. (FULL: 전체 memory 포함)
	auto str_file_full = (path_dump / "snap_full.dmp").string();
	auto result_3 = snap_dump(str_file_full, eDUMP_CONTEXT::FULL);
	std::cout << "[3] snap_dump(\"" << str_file_full << "\", FULL) : " << (result_3 ? "ok" : "fail") << std::endl;

	// 6) snap dump는 process를 멈추지 않는다. thread는 계속 동작 중이다.
	g_is_run = false;
	pthread->wait();
	std::cout << "    (thread is still alive and stopped normally)" << std::endl;

	// 7) 남겨진 dump 파일들
	std::cout << "[dump files in '" << path_dump.string() << "']" << std::endl;
	for (auto& iter : std::filesystem::directory_iterator(path_dump))
		if (iter.path().filename().string().rfind("snap_", 0) == 0)
			std::cout << "   " << iter.path().filename().string() << " (" << iter.file_size() << " bytes)" << std::endl;

	// return)
	return 0;
}
