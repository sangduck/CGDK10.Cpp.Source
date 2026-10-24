//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                         tutorials execute - lock                          *
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
// 1) CGDK header file
#include "cgdk/sdk10/system.object.h"

// 2) header files
#include <iostream>

// 3) namespace
using namespace CGDK;

int main()
{
	//-----------------------------------------------------------------------------
	//
	// 1. lockable 객체
	//    - lock_object은 lock객체로 std::recursive_lock
	//
	//-----------------------------------------------------------------------------
	{
		// - lockable<>
		lock_object x;

		// - lock한다.
		x.lock();

		// - 처리...

		// - unlock한다.
		x.unlock();
	}

	//-----------------------------------------------------------------------------
	//
	// 2. SCOPED_LOCK
	//    - SCOPED_LOCK(cs)를 사용하면 RAII를 사용해 lock과 unlock을 자동 처리할 수 있다.
	//
	//-----------------------------------------------------------------------------
	// 1) 기본 SCOPED_LOCK
	{
		std::mutex x;

		// SCOPED_LOCK을 사용해서 RAII를 사용한 자동 lock 해제 처리를 할 수 있다.
		SCOPED_LOCK(x)
		{
			std::cout << "locked"sv << std::endl;
		}
	}

	// 2) 다중 SCOPED_LOCK
	{
		std::mutex x;
		std::mutex y;

		SCOPED_LOCK(x)
		{
			SCOPED_LOCK(y)
			{
				std::cout << "locked"sv << std::endl;
			}
		}
	}

	// 3) named를 사용해 lock에 이름을 부여할 수 있다.
	//    std::conditional_variable를 사용하는 경우나 블록을 빠져나가지 않고 lock을
	//    풀어야 할 필요가 이름을 정해줄 필요가 있을 수 있다.
	{
		std::mutex x;

		SCOPED_LOCK_NAMED(x, x_scoped)
		{
			std::cout << "before lock released"sv << std::endl;

			// 여기 이후로는 lock을 풀어버린다.
			SCOPED_LOCK_LEAVE(x_scoped);

			std::cout << "after lock released"sv << std::endl;
		}
	}


	//-----------------------------------------------------------------------------
	//
	// 3. conditional variable
	//    - 다중 스레드에서 동기화 처리를 위해 동기적으로 signal을 전달하기 위해
	//      std::condition_variable을 사용한다.
	//    - 먼저 lock을 건 후 그 lock안에서 완료를 통보할 처리를 비동기적으로 요청한 후
	//    - std::condition_variable의 wait() 등을 호출해 signal이 전달되기 전까지 블로킹한다.
	//      이때 wait()함수의 인자로 전달되는 lock 객체는 unlock된다.
	//      주의) wait에 들어가기 전 notify_one()혹은 notify_all()을 할 경우 전달되지 않는다.
	//    - std::condition_variable의 notify_one() 혹은 notify_all()이 호출되면 
	//      인자로 전달된 lock객체를 다시 lock한후 wait()함수의 블로킹이 해제된다.
	//    - std::promise, std::future이 가능한 곳이라면 그것을 사용하는 것이 더 편리할 수 있다.
	//
	//-----------------------------------------------------------------------------
	// declare) 
	lock_object lockable_wait;
	std::condition_variable_any cv_complete;

	// - 실행할 함수...
	auto f_process = [&]()
	{
		std::cout << "'f_process' executed!!"sv << '\n';

		SCOPED_LOCK(lockable_wait)
		{
			std::cout << "'cv_complete' notify one!!"sv << '\n';

			cv_complete.notify_one();
		}
	};

	SCOPED_LOCK_NAMED(lockable_wait, cs)
	{
		std::cout << "'lockable_wait' locked!"sv << '\n';

		// - 비동기 호출한다. (반드시 lock 안에서 실행해야 한다.)
		system_executor::post(make_executable(f_process));

		// - wait를 하면 lock을 해제한다.
		cv_complete.wait(cs);
	}

	// 출력) 
	std::cout << "completed!!"sv << '\n';

	// return)
	return 0;
}
