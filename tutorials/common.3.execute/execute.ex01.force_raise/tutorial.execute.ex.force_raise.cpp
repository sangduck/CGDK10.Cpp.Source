//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials execute - ex.force raise                    *
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
#if defined(_WIN32)
#include <conio.h>
#endif

// 2) CGDK header file
#include "cgdk/sdk10/system.object.h"

// 3) header files
#include <iostream>

// 4) namespace
using namespace CGDK;


//-----------------------------------------------------------------------------
//
// tutorial.06.ex.force raise
//
//  설명)
//  force_raise()함수로 특정 Thread를 강제로 exception을 발생시킬 수 있다.
//  이 때 스레드에 throwable::force_raise 예외를 던져 켜 Rollback시키다.
//  데드락이나 무한루프로 Thead의 Hang이 걸린 경우 이을 사용해 해결할 수 있다.
//
//-----------------------------------------------------------------------------
void thread_function()
{
	std::cout << ">> thread started..." << std::endl;

	try 
	{
		while (true)
		{
			//Work forever...
			api::sleep_for(1s);
		}
	}
	catch (throwable::force_raise& /*e*/) 
	{
		std::cout << "(!) force raised" << std::endl;
	}
	catch (...)
	{
		std::cout << "(!) exception" << std::endl;
	}

	std::cout << "<< thread stopped" << std::endl;
}

int main()
{
	// 1) thread 생성한다.
	auto pthread = make_object<api::thread>();

	// 2) thread 시작
	pthread->begin_thread(thread_function);

	// 3) 아무 키를 입력하면 force raising
	_getch();

	// 4) force raise excetpion
	pthread->force_raise_exception();

	// 4) 아무 키를 입력하면 종료
	_getch();

	// return)
	return	0;
}
