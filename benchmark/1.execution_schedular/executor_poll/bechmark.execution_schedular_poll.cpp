#include <iostream>
#include <chrono>
#include <list>
#include <thread>

#define TEST_COUNT 10000

void foo()
{
    std::cout << "foo executed" << std::endl;
}

std::list<std::chrono::steady_clock::time_point> list_job;
void f_poll()
{
    auto tick_pre = std::chrono::steady_clock::now();

    while (list_job.empty() == false)
    {
        auto tick_now = std::chrono::steady_clock::now();

        if (tick_pre + std::chrono::milliseconds(1) < tick_now)
        {
            auto iter = list_job.begin();
            while(iter!= list_job.end())
            {
                if (tick_now > *iter)
                {
                    foo();
                    iter = list_job.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }

            tick_pre += std::chrono::milliseconds(1);
        }
    }
}

int main()
{
    // 1) executor에 실행을 등록한다.
    auto tick_now = std::chrono::steady_clock::now();
    for (int i = 0; i < TEST_COUNT; ++i)
    {
        list_job.push_back(tick_now);
        tick_now = tick_now + std::chrono::seconds(1);
    }

    // 2) 시작!
    std::thread temp(f_poll);

    // 3) 종료를 기다린다.
    temp.join();
}
