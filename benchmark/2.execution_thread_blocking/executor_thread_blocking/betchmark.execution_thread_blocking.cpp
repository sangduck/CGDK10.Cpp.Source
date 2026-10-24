#include <iostream>
#include "cgdk/sdk10/system.object.h"

#define TEST_COUNT 10000

using namespace CGDK;

void foo()
{
    std::cout << "foo executed" << std::endl;
}

int main()
{
    // 1) executor에 실행을 등록한다.
    auto tick_now = chrono::tick::now();
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        // - 스케쥴러에 실행할 함수와 시간을 등록한다.
        CGDK::system_executor::post(CGDK::make_executable(foo), tick_now);

        // - 1초마다 1개씩
        tick_now = tick_now + 1s;
    }

    // 2) 종료를 기다린다.
    getchar();
}
