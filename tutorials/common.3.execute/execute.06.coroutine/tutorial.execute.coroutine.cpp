//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                       tutorials execute - coroutine                       *
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
//    - coroutine은 "system.object.h"에 포함되어 있다. (C++20 이상)
#include "cgdk/sdk10/system.object.h"

// 2) header files
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <future>
#include <atomic>
#include <stdexcept>

// 3) namespace
using namespace CGDK;
using namespace std::chrono_literals;


//-----------------------------------------------------------------------------
//
// tutorial.06.coroutine
//
// ------------------------------------------------------------------
//  설명)
//
//  콜백 방식의 비동기 처리는 처리 흐름이 여러 함수로 나뉘고, 중간 상태를
//  멤버 변수로 따로 보관해야 하기 때문에 코드가 복잡해진다.
//
//  C++20 coroutine을 사용하면 비동기 처리를 일반 함수처럼 순서대로 작성할 수
//  있다. 'co_await'를 만나면 코루틴은 멈추고 스레드는 executor로 돌아가 다른
//  일을 처리한다. 결과가 준비되면 코루틴은 멈췄던 다음 줄부터 이어서 실행된다.
//
//  CGDK의 coroutine은 다음 규칙으로 동작한다.
//
//  1) 코루틴 함수는 반환형이 coroutine::task<T>이고 co_return으로 값을 돌려준다.
//  2) 코루틴 함수는 호출하는 것만으로는 실행되지 않는다.(지연 시작)
//     다른 코루틴에서 'co_await'로 호출하면 실행되고, 그 결과를 돌려받는다.
//  3) 'co_await f'처럼 일반 함수를 co_await하면 f()는 system_executor의 다른
//     스레드에서 실행되고, 끝나면 코루틴은 그 결과를 받아 이어서 실행된다.
//  4) main() 같은 일반 함수에서는 'co_await'를 쓸 수 없다. 진입 코루틴 하나만
//     foo().get()으로 실행하고, 나머지는 그 안에서 'co_await'로 호출한다.
//
//  주요 함수)
//
//     coroutine::task<T>          코루틴 함수의 반환형
//     co_await foo()              코루틴 foo()를 호출하고 결과를 받는다.
//     co_await f                  일반 함수 f()를 system_executor에서 실행하고 결과를 받는다.
//     co_await co_sleep_for(t)    스레드를 막지 않고 정해진 시간 동안 기다린다.
//     co_await co_sleep_until(t)  스레드를 막지 않고 정해진 시각까지 기다린다.
//     foo().get()                 일반 함수에서 코루틴을 실행하고 끝날 때까지 기다려 결과를 받는다.
//     foo().start()               일반 함수에서 코루틴을 실행하고 기다리지 않는다.
//     executor->post(task)        executor를 지정해서 코루틴을 시작하고 기다리지 않는다. (자동 감지)
//     system_executor::post(task)
//                                 system_executor에서 코루틴을 시작하고 기다리지 않는다. (자동 감지)
//     system_executor::execute(task)
//                                 호출한 스레드에서 코루틴을 바로 실행하고, 첫 'co_await'에서 멈춘 뒤부터는
//                                 system_executor에서 이어간다. (9-3 예제, 자동 감지)
//
//  주의)
//
//  1) 코루틴 함수의 인자는 참조(&) 대신 값으로 받는다.
//     첫 'co_await' 이후에는 호출한 쪽의 지역 변수가 이미 사라졌을 수 있다.
//  2) 멤버 함수 코루틴은 'this'를 원시 포인터로 가지고 있다.
//     코루틴이 끝날 때까지 객체가 살아 있어야 한다.
//  3) 'co_await' 중에 lock을 잡고 있으면 안 된다.
//     재개되는 스레드가 달라질 수 있기 때문이다.
//  4) main() 같은 일반 함수에서는 'co_await'를 쓸 수 없다.
//  5) 'co_await f'의 f는 일반 함수여야 한다. f() 안에서 'co_await'를 쓰면
//     f가 코루틴이 되어 컴파일 오류가 난다.
//
// ------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helper)
//-----------------------------------------------------------------------------
std::string thread_id()
{
	std::ostringstream os;
	os << std::this_thread::get_id();
	return os.str();
}

long long elapsed_ms(chrono::tick::time_point _tick_start)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(chrono::tick::now() - _tick_start).count();
}

// - 시간이 걸리는 작업(쿼리, 파일 읽기 등)을 흉내낸다.
//   예제에서는 'co_await [] { slow_work(10ms); };'처럼 system_executor의 다른
//   스레드에서 실행하고, 끝나면 코루틴이 이어서 실행된다. (4번 예제 참고)
void slow_work(std::chrono::milliseconds _duration)
{
	std::this_thread::sleep_for(_duration);
}


//-----------------------------------------------------------------------------
// 1. 기본 함수 코루틴
//
//    - 반환형을 coroutine::task<T>로 하고 return 대신 co_return을 쓴다.
//    - 반환값이 없으면 coroutine::task<void>로 하고 co_return은 생략할 수 있다.
//    - 다른 코루틴의 결과는 'co_await'로 받는다.
//-----------------------------------------------------------------------------
// 1) 값을 돌려주는 코루틴
coroutine::task<int> add(int _a, int _b)
{
	co_return _a + _b;
}

// 2) 기다린 후 값을 돌려주는 코루틴
coroutine::task<int> multiply_later(int _a, int _b)
{
	// - 시간이 걸리는 작업을 다른 스레드에서 실행하고 끝나면 이어서 실행된다.
	co_await [] { slow_work(50ms); };

	co_return _a * _b;
}

// 3) 반환값이 없는 코루틴
coroutine::task<void> print_message(std::string _message)
{
	co_await [] { slow_work(10ms); };

	std::cout << "    message: " << _message << std::endl;
}

coroutine::task<void> example_1_basic()
{
	// 1) 코루틴을 호출하고 결과를 받는다.
	int result = co_await add(1, 2);
	std::cout << "    add(1, 2) = " << result << std::endl;

	// 2) 기다리는 코루틴도 일반 함수처럼 호출한다.
	result = co_await multiply_later(3, 4);
	std::cout << "    multiply_later(3, 4) = " << result << std::endl;

	// 3) 반환값이 없는 코루틴
	co_await print_message("hello coroutine");

	// 4) 람다도 코루틴이 될 수 있다. (반환형을 반드시 적는다)
	auto square = [](int _value) -> coroutine::task<int>
		{
			co_await [] { slow_work(10ms); };
			co_return _value * _value;
		};

	result = co_await square(5);
	std::cout << "    square(5) = " << result << std::endl;
}


//-----------------------------------------------------------------------------
// 2. 멤버 함수 코루틴
//
//    - 멤버 함수도 반환형만 coroutine::task<T>로 하면 코루틴이 된다.
//    - 멤버 변수를 그대로 쓸 수 있고, 다른 멤버 함수 코루틴을 호출할 수 있다.
//    - const 멤버 함수와 static 멤버 함수도 코루틴이 될 수 있다.
//    - 코루틴은 'this'를 원시 포인터로 가지고 있으므로 코루틴이 끝날 때까지
//      객체가 살아 있어야 한다. 여기서는 호출하는 코루틴의 지역 변수이므로
//      'co_await'가 끝날 때까지 살아 있다.
//-----------------------------------------------------------------------------
class account
{
public:
	explicit account(std::string _name, int _balance) : m_name(std::move(_name)), m_balance(_balance) {}

	// 1) 멤버 변수를 사용하는 코루틴
	coroutine::task<int> deposit(int _amount)
	{
		// - 저장 처리를 흉내낸다.
		co_await [] { slow_work(10ms); };

		this->m_balance += _amount;

		co_return this->m_balance;
	}

	// 2) 실패하면 예외를 던지는 코루틴
	coroutine::task<int> withdraw(int _amount)
	{
		co_await [] { slow_work(10ms); };

		if (_amount > this->m_balance)
			throw std::runtime_error(this->m_name + ": not enough balance");

		this->m_balance -= _amount;

		co_return this->m_balance;
	}

	// 3) 다른 멤버 함수 코루틴을 호출하는 코루틴
	//    - withdraw()가 던진 예외를 여기서 try/catch로 잡는다.
	//    - 잔액이 부족하면 월세를 내지 않고 현재 잔액을 돌려준다.
	coroutine::task<int> pay_month(int _salary, int _rent)
	{
		co_await this->deposit(_salary);

		try
		{
			co_await this->withdraw(_rent);
		}
		catch (const std::exception& _e)
		{
			std::cout << "    pay_month: rent is not paid ('" << _e.what() << "')" << std::endl;
		}

		co_return this->m_balance;
	}

	// 4) const 멤버 함수 코루틴
	coroutine::task<std::string> report() const
	{
		co_await [] { slow_work(10ms); };

		co_return this->m_name + " = " + std::to_string(this->m_balance);
	}

	// 5) static 멤버 함수 코루틴
	static coroutine::task<int> interest(int _balance, int _percent)
	{
		co_return _balance * _percent / 100;
	}

private:
	std::string		m_name;
	int				m_balance;
};

coroutine::task<void> example_2_member()
{
	// - 이 코루틴이 끝날 때까지 살아 있는 객체
	account my_account("my_account", 100);

	// 1) 멤버 함수 코루틴 호출
	int balance = co_await my_account.deposit(50);
	std::cout << "    deposit(50)        -> " << balance << std::endl;

	// 2) 다른 멤버 함수 코루틴을 호출하는 멤버 함수 코루틴
	balance = co_await my_account.pay_month(300, 120);
	std::cout << "    pay_month(300,120) -> " << balance << std::endl;

	//    - 잔액이 부족하면 withdraw()의 예외를 pay_month()가 잡는다.
	//      예외는 여기까지 전달되지 않는다.
	balance = co_await my_account.pay_month(10, 100000);
	std::cout << "    pay_month(10,100000) -> " << balance << std::endl;

	// 3) static 멤버 함수 코루틴
	int interest = co_await account::interest(balance, 10);
	std::cout << "    interest(10%)      -> " << interest << std::endl;

	// 4) const 멤버 함수 코루틴
	std::cout << "    report()           -> " << co_await my_account.report() << std::endl;

	// 5) 멤버 함수 코루틴의 예외
	try
	{
		co_await my_account.withdraw(10000);
	}
	catch (const std::exception& _e)
	{
		std::cout << "    withdraw(10000)    -> exception '" << _e.what() << "'" << std::endl;
	}
}


//-----------------------------------------------------------------------------
// 3. 여러 번 co_await 하기
//
//    - 'co_await'는 일반 함수 호출처럼 몇 번이든 순서대로 쓸 수 있다.
//    - 코루틴이 다른 코루틴을, 그 코루틴이 또 다른 코루틴을 호출할 수 있다.
//    - 반복문, 조건문 안에서도 쓸 수 있다.
//    - 순서대로 'co_await'한 시간은 더해진다.
//-----------------------------------------------------------------------------
coroutine::task<std::string> load_name(int _id)
{
	co_await [] { slow_work(30ms); };

	co_return "user_" + std::to_string(_id);
}

coroutine::task<int> load_level(int _id)
{
	co_await [] { slow_work(20ms); };

	co_return _id * 10;
}

// - 두 코루틴을 호출해서 결과를 합치는 코루틴
coroutine::task<std::string> load_profile(int _id)
{
	auto name = co_await load_name(_id);
	auto level = co_await load_level(_id);

	co_return name + " (level " + std::to_string(level) + ")";
}

coroutine::task<void> example_3_multiple()
{
	auto tick_start = chrono::tick::now();

	// 1) 순서대로 여러 번 (30ms + 20ms)
	auto name = co_await load_name(1);
	auto level = co_await load_level(1);
	std::cout << "    sequential: " << name << ", level " << level << " (" << elapsed_ms(tick_start) << "ms)" << std::endl;

	// 2) 중첩 호출 - example_3_multiple -> load_profile -> load_name, load_level
	std::cout << "    nested    : " << co_await load_profile(2) << std::endl;

	// 3) 반복문 안에서
	int total = 0;

	for (int id = 1; id <= 3; ++id)
		total += co_await load_level(id);

	std::cout << "    loop      : total level = " << total << std::endl;

	// 4) 조건문 안에서
	if (co_await load_level(5) >= 50)
		std::cout << "    condition : level of 5 is 50 or more" << std::endl;

	// 5) 식 안에서
	//    - 한 식 안의 두 'co_await'의 실행 순서는 정해져 있지 않다.
	//      순서가 중요하면 문장을 나눠서 쓴다.
	int sum = co_await add(1, 2) + co_await add(3, 4);
	std::cout << "    expression: add(1,2) + add(3,4) = " << sum << std::endl;

	std::cout << "    total " << elapsed_ms(tick_start) << "ms" << std::endl;
}

//-----------------------------------------------------------------------------
// 4. co_await 함수 - 일반 함수를 system_executor에서 실행하고 결과 받기
//
//    - 'co_await f'처럼 인자가 없는 함수(람다, 함수, 함수 포인터, std::function)를
//      co_await하면 f()는 system_executor의 다른 스레드에서 실행된다.
//    - f()가 끝나면 코루틴이 이어서 실행되고 'co_await f'의 값은 f()의 반환값이 된다.
//    - 인자가 필요한 함수는 람다로 묶는다.
//    - f()에서 던진 예외는 'co_await f'에서 다시 던져진다.
//    - 코루틴 함수 foo는 'co_await foo'가 아니라 'co_await foo()'로 호출한다.
//-----------------------------------------------------------------------------
int heavy_calculation(int _n)
{
	int sum = 0;

	for (int i = 1; i <= _n; ++i)
		sum += i;

	return sum;
}

void load_table()
{
	std::cout << "    load_table()        (thread " << thread_id() << ")" << std::endl;
}

coroutine::task<void> example_4_function()
{
	std::cout << "    coroutine           (thread " << thread_id() << ")" << std::endl;

	// 1) 람다 - 반환값을 받는다.
	int sum = co_await []
		{
			std::cout << "    in function         (thread " << thread_id() << ")" << std::endl;
			return heavy_calculation(100);
		};

	std::cout << "    sum(1..100) = " << sum << std::endl;

	// 2) 인자가 필요한 함수는 람다로 묶는다.
	int n = 1000;
	sum = co_await [n] { return heavy_calculation(n); };

	std::cout << "    sum(1..1000) = " << sum << std::endl;

	// 3) 함수 (반환값 없음)
	co_await load_table;

	// 4) 예외
	try
	{
		co_await [] { throw std::runtime_error("failed in function"); };
	}
	catch (const std::exception& _e)
	{
		std::cout << "    caught: '" << _e.what() << "'" << std::endl;
	}
}


//-----------------------------------------------------------------------------
// 5. 예외 처리
//
//    - 코루틴에서 던진 예외는 그 코루틴을 'co_await'한 곳으로 전달된다.
//    - 일반 함수처럼 try/catch로 잡는다.
//    - 끝까지 잡지 않은 예외는 진입 코루틴을 실행한 get()에서 다시 던져진다.
//-----------------------------------------------------------------------------
coroutine::task<int> may_fail(bool _fail)
{
	co_await [] { slow_work(10ms); };

	if (_fail)
		throw std::runtime_error("may_fail() failed");

	co_return 1;
}

coroutine::task<int> call_may_fail()
{
	// - 여기서 잡지 않으므로 호출한 쪽으로 다시 전달된다.
	co_return co_await may_fail(true);
}

coroutine::task<void> example_5_exception()
{
	// 1) 자식 코루틴의 예외를 잡는다.
	try
	{
		co_await may_fail(true);
	}
	catch (const std::exception& _e)
	{
		std::cout << "    caught in coroutine: '" << _e.what() << "'" << std::endl;
	}

	// 2) 여러 단계를 거쳐 전달된 예외를 잡는다.
	try
	{
		co_await call_may_fail();
	}
	catch (const std::exception& _e)
	{
		std::cout << "    caught through call_may_fail(): '" << _e.what() << "'" << std::endl;
	}
}


//-----------------------------------------------------------------------------
// 6. co_sleep_for() / co_sleep_until() - 스레드를 막지 않고 기다리기
//
//    - co_sleep_for(시간)은 정해진 시간 동안, co_sleep_until(시각)은 정해진
//      시각까지 기다린다.
//    - 기다리는 동안 코루틴만 멈추고 스레드는 다른 일을 처리한다.
//      시간이 되면 코루틴이 이어서 실행된다.
//    - 'co_await [] { std::this_thread::sleep_for(...); }'는 그동안 다른 스레드
//      하나를 붙잡는다. 단순히 기다릴 때는 co_sleep_for()를 쓴다.
//    - co_sleep_until()은 chrono::tick의 시각과 다른 clock(system_clock 등)의
//      시각을 모두 받는다.
//    - 0 이하의 시간이나 이미 지난 시각이면 멈추지 않고 바로 이어서 실행된다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_6_sleep()
{
	auto tick_start = chrono::tick::now();

	// 1) 정해진 시간 동안 기다린다.
	co_await co_sleep_for(100ms);
	std::cout << "    co_sleep_for(100ms)            -> " << elapsed_ms(tick_start) << "ms" << std::endl;

	// 2) 정해진 시각까지 기다린다. (chrono::tick)
	co_await co_sleep_until(tick_start + 200ms);
	std::cout << "    co_sleep_until(start + 200ms)  -> " << elapsed_ms(tick_start) << "ms" << std::endl;

	// 3) 다른 clock의 시각도 쓸 수 있다. (std::chrono::system_clock)
	co_await co_sleep_until(std::chrono::system_clock::now() + 100ms);
	std::cout << "    co_sleep_until(system + 100ms) -> " << elapsed_ms(tick_start) << "ms" << std::endl;

	// 4) 반복문에서 일정한 간격으로 실행한다.
	for (int i = 1; i <= 3; ++i)
	{
		co_await co_sleep_for(50ms);
		std::cout << "    tick " << i << "                         -> " << elapsed_ms(tick_start) << "ms" << std::endl;
	}
}


//-----------------------------------------------------------------------------
// 8. start() - 기다리지 않고 시작하기
//
//    - foo().get()은 코루틴이 끝날 때까지 기다리지만, foo().start()는 시작만
//      하고 바로 돌아온다. 코루틴은 system_executor에서 따로 실행된다.
//    - 서버에서 메시지를 받았을 때처럼 결과를 기다리지 않고 처리를 시작할 때 쓴다.
//    - 여러 코루틴을 start()하면 동시에 실행된다.
//    - 끝나면 코루틴 프레임은 스스로 해제된다. 잡지 않은 예외는 로그로 남는다.
//    - 코루틴 안에서도 start()로 기다리지 않는 작업을 시작할 수 있다.
//    - 캡처가 있는 람다 코루틴을 임시 객체로 start()하면 안 된다.
//      (식이 끝나면 람다가 사라지는데 코루틴은 아직 실행 중이다)
//
//    * 예제는 main()에 있다.
//    - start()로 시작한 코루틴들이 모두 끝날 때까지 기다리려면 coroutine::wait_detached()를
//      호출한다. 프로그램을 끝내거나 querier를 정지하기 전에 호출한다.
//-----------------------------------------------------------------------------
coroutine::task<void> background_job(int _id, std::chrono::milliseconds _duration)
{
	co_await co_sleep_for(_duration);

	std::cout << "    job " << _id << " finished (" << _duration.count() << "ms)" << std::endl;
}


//-----------------------------------------------------------------------------
// 9. post() - 기다리지 않고 executor에서 시작하기 (자동 감지)
//
//    - executor->post(func_or_task)는 start()처럼 기다리지 않고 시작하되, 코루틴을
//      그 executor에 묶는다. 'co_await' 이후에도 항상 그 executor에서 재개된다.
//      일반 함수면 자동으로 coroutine으로 래핑된다.
//    - executor::strand에 묶으면 같은 strand의 코루틴들은 동시에 실행되지 않으므로
//      lock 없이 상태를 다룰 수 있다.
//    - executor->post(owner, func_or_task)는 코루틴이 끝날 때까지 owner 객체를 살려 둔다.
//      멤버 함수 코루틴을 시작할 때 객체를 owner로 넘긴다.
//    - executor->post(func_or_task, on_complete)는 코루틴이 끝나면 완료 함수를 호출한다.
//      예외가 있으면 std::exception_ptr로 전달된다.
//    - system_executor::post()는 system_executor에서 시작한다.
//        system_executor::post(func_or_task)                   (start()와 같다)
//        system_executor::post(owner, func_or_task)            owner를 끝날 때까지 살려 둔다.
//        system_executor::post(func_or_task, on_complete)      끝나면 완료 함수를 호출한다.
//        system_executor::post(owner, func_or_task, on_complete)
//      system_executor에서는 'co_await' 이후 아무 스레드에서나 재개되고 여러
//      코루틴이 동시에 실행되므로 공유하는 데이터는 lock이나 atomic으로 보호한다.
//    - post()로 시작한 코루틴도 coroutine::wait_detached()로 끝날 때까지
//      기다릴 수 있다.
//
//    * 예제는 main()에 있다.
//-----------------------------------------------------------------------------
// 1) strand에서 lock 없이 카운터를 증가시키는 코루틴
int g_count = 0;	// lock 없이 사용한다. (같은 strand에서만 접근한다)

coroutine::task<void> add_count(int _times)
{
	for (int i = 0; i < _times; ++i)
	{
		++g_count;

		// - 가끔 기다려서 다른 코루틴과 번갈아 실행되게 한다.
		if ((i % 1000) == 999)
			co_await co_sleep_for(1ms);
	}
}

// 2) 멤버 함수 코루틴을 가진 객체 (owner)
class session : virtual public Ireferenceable
{
public:
	virtual ~session() noexcept
	{
		std::cout << "    [session] destroyed" << std::endl;
	}

	coroutine::task<void> process_login(std::string _name)
	{
		co_await co_sleep_for(100ms);

		std::cout << "    [session] " << _name << " logged in" << std::endl;
	}
};

// 3) strand 없이 카운터를 증가시키는 코루틴
//    - system_executor의 여러 스레드에서 동시에 실행되므로 atomic을 사용한다.
std::atomic<int> g_count_atomic{ 0 };

coroutine::task<void> add_count_atomic(int _times)
{
	for (int i = 0; i < _times; ++i)
	{
		++g_count_atomic;

		// - 가끔 기다려서 다른 코루틴과 번갈아 실행되게 한다.
		if ((i % 1000) == 999)
			co_await co_sleep_for(1ms);
	}
}


// 4) 어느 스레드에서 실행되는지 보여주는 코루틴 (co_post / co_execute 비교용)
coroutine::task<void> show_thread(std::string _name)
{
	std::cout << "    [" << _name << "] before co_await : thread " << thread_id() << std::endl;

	co_await co_sleep_for(10ms);

	std::cout << "    [" << _name << "] after  co_await : thread " << thread_id() << std::endl;
}

// 5) 'co_await'가 없는 코루틴 (co_execute()가 돌아올 때 이미 끝나 있다)
coroutine::task<void> print_now(std::string _message)
{
	std::cout << "    " << _message << " (thread " << thread_id() << ")" << std::endl;
	co_return;
}


//-----------------------------------------------------------------------------
// tutorial_coroutine()
//
//    - 예제 코루틴들을 함수 앞에 'co_await'만 붙여서 차례로 호출한다.
//    - main()은 코루틴이 될 수 없으므로(C++ 표준) 이 코루틴을 대신 진입점으로 쓴다.
//-----------------------------------------------------------------------------
coroutine::task<void> tutorial_coroutine()
{
	try
	{
		// 1. 기본 함수 코루틴
		std::cout << ">> 1. basic" << std::endl;
		co_await example_1_basic();

		// 2. 멤버 함수 코루틴
		std::cout << ">> 2. member function" << std::endl;
		co_await example_2_member();

		// 3. 여러 번 co_await 하기
		std::cout << ">> 3. multiple co_await" << std::endl;
		co_await example_3_multiple();

		// 4. co_await 함수
		std::cout << ">> 4. co_await function" << std::endl;
		co_await example_4_function();

		// 5. 예외 처리
		std::cout << ">> 5. exception" << std::endl;
		co_await example_5_exception();

		// 6. co_sleep_for / co_sleep_until
		std::cout << ">> 6. co_sleep_for / co_sleep_until" << std::endl;
		co_await example_6_sleep();

		// 7. 잡지 않은 예외는 호출한 쪽으로 전달된다.
		std::cout << ">> 7. uncaught exception" << std::endl;
		co_await may_fail(true);
	}
	catch (const std::exception& _e)
	{
		std::cout << "    exception from tutorial_coroutine(): '" << _e.what() << "'" << std::endl;
	}
}

int main()
{
	// ------------------------------------------------------------------
	// 1. ~ 7. get()
	//    - main()에서는 'co_await'를 쓸 수 없다.
	//      진입 코루틴 tutorial_coroutine() 하나만 get()으로 실행하고
	//      끝날 때까지 기다린다.
	// ------------------------------------------------------------------
	tutorial_coroutine().get();

	// ------------------------------------------------------------------
	// 8. start()
	//    - 세 작업을 기다리지 않고 시작한다.
	//      동시에 실행되므로 짧은 작업이 먼저 끝난다.
	// ------------------------------------------------------------------
	std::cout << ">> 8. start()" << std::endl;
	{
		auto tick_start = chrono::tick::now();

		background_job(1, 300ms).start();
		background_job(2, 100ms).start();
		background_job(3, 200ms).start();

		std::cout << "    3 jobs started (main continues)" << std::endl;

		// - 예제에서는 끝날 때까지 기다린다.
		coroutine::wait_detached();

		std::cout << "    all finished in " << elapsed_ms(tick_start) << "ms (concurrently)" << std::endl;
	}

	// ------------------------------------------------------------------
	// 9. executor->post() (자동 감지)
	// ------------------------------------------------------------------
	std::cout << ">> 9. executor->post()" << std::endl;
	{
		// - strand를 준비한다.
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		// 1) strand에 묶기
		//    - 두 코루틴이 같은 strand에서 번갈아 실행되며 lock 없이 카운터를 증가시킨다.
		pstrand->post(add_count(5000));
		pstrand->post(add_count(5000));

		coroutine::wait_detached();

		std::cout << "    count = " << g_count << " (expected 10000)" << std::endl;

		// 2) owner
		//    - 참조를 바로 놓아도 process_login()이 끝날 때까지 session은 살아 있다.
		auto psession = make_object<session>();
		pstrand->post(psession.get(), psession->process_login("user_1"));

		psession.reset();
		std::cout << "    released the reference of the session" << std::endl;

		coroutine::wait_detached();

		// 3) 완료 함수
		//    - 코루틴이 끝나면 호출된다. 예외가 있으면 exception_ptr로 전달된다.
		pstrand->post(may_fail(true), [](std::exception_ptr _exception)
			{
				try
				{
					if (_exception)
						std::rethrow_exception(_exception);

					std::cout << "    completed" << std::endl;
				}
				catch (const std::exception& _e)
				{
					std::cout << "    completed with an exception '" << _e.what() << "'" << std::endl;
				}
			});

		coroutine::wait_detached();

		// - strand를 정지한다.
		pstrand->stop();
	}

	// ------------------------------------------------------------------
	// 9-2. system_executor::post() - system_executor에서 시작하기 (자동 감지)
	// ------------------------------------------------------------------
	std::cout << ">> 9-2. system_executor::post()" << std::endl;
	{
		// 1) system_executor::post(task)
		//    - 여러 코루틴이 system_executor의 스레드들에서 동시에 실행된다.
		//      (공유하는 카운터는 atomic이다)
		for (int i = 0; i < 4; ++i)
			system_executor::post(add_count_atomic(2500));

		coroutine::wait_detached();

		std::cout << "    count = " << g_count_atomic << " (expected 10000)" << std::endl;

		// 2) system_executor::post(owner, task)
		//    - 참조를 바로 놓아도 process_login()이 끝날 때까지 session은 살아 있다.
		auto psession = make_object<session>();
		system_executor::post(psession, psession->process_login("user_2"));

		psession.reset();
		std::cout << "    released the reference of the session" << std::endl;

		coroutine::wait_detached();

		// 3) system_executor::post(task, on_complete)
		system_executor::post(may_fail(false), [](std::exception_ptr _exception)
			{
				std::cout << (_exception ? "    completed with an exception" : "    completed") << std::endl;
			});

		coroutine::wait_detached();
	}

	// ------------------------------------------------------------------
	// 9-3. system_executor::execute() - 호출한 스레드에서 바로 시작하기 (자동 감지)
	//    - post()는 코루틴을 executor에 넘기고 바로 돌아온다. (본문은 executor 스레드에서 실행된다)
	//    - execute()는 호출한 스레드에서 본문을 바로 실행하고, 첫 'co_await'에서 멈춘 뒤부터는
	//      system_executor에서 이어간다. 'co_await'가 없으면 돌아올 때 이미 끝나 있다.
	//      (요청을 받은 스레드에서 지연 없이 처리를 시작하고 싶을 때 쓴다)
	//    - 일반 함수면 즉시 실행된다. coroutine::task면 co_execute처럼 동작한다.
	//    - 코루틴 안에서 부르면 중첩되지 않도록 post()처럼 동작한다. strand는 항상 post된다.
	// ------------------------------------------------------------------
	std::cout << ">> 9-3. system_executor::execute()" << std::endl;
	{
		std::cout << "    main thread : " << thread_id() << std::endl;

		// 1) post : 'before'도 다른 스레드에서 출력된다.
		system_executor::post(show_thread("post"));
		coroutine::wait_detached();

		// 2) execute : 'before'는 main 스레드에서, 'after'는 system_executor 스레드에서 출력된다.
		system_executor::execute(show_thread("execute"));
		std::cout << "    execute() returned (the coroutine is waiting in co_sleep_for)" << std::endl;
		coroutine::wait_detached();

		// 3) 'co_await'가 없는 코루틴은 execute()가 돌아올 때 이미 끝나 있다.
		system_executor::execute(print_now("no co_await: finished before execute() returns"));
		std::cout << "    execute() returned" << std::endl;
	}

	// ------------------------------------------------------------------
	// 9-4. 멈춘 executor
	//    - executor가 정지되어 코루틴을 받지 못하면 코루틴은 다른 스레드에서 대신 실행되지 않는다.
	//      post()/start()는 eRESULT::NOT_READY 예외를 던지고 코루틴은 한 줄도 실행되지 않는다. (frame은 해제된다)
	//    - 'co_await'로 멈춰 있는 동안 executor가 정지되면 그 코루틴은 취소된다.
	//      (나머지 본문은 실행되지 않고 frame이 해제되어 지역 변수가 정리된다)
	// ------------------------------------------------------------------
	std::cout << ">> 9-4. stopped executor" << std::endl;
	{
		// - 시작하지 않은 strand는 post를 받지 않는다.
		auto pstrand = make_own<executor::strand>();

		try
		{
			pstrand->post(print_message("never printed"));
		}
		catch (const throwable::failure& _e)
		{
			std::cout << "    post() on a stopped executor: exception '" << _e.description() << "'" << std::endl;
		}
	}

	// return)
	return	0;
}