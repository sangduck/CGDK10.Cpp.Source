//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                      tutorials query - redis.simple                       *
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
#include <chrono>
#include <iostream>
#ifdef _WIN32
	#include <conio.h>
#endif

#include "cgdk/sdk10/server.query.redis.h"
using namespace CGDK;


void test_basic_cgdk()
{
	// - output
	std::cout << "result: " << std::endl;


	// 1) create redis querier
	auto pquerier = make_object<query::redis::querier_t>();

	// 2) start redis querier
	pquerier->start();

	std::atomic<int64_t> count{ 0 };

	std::chrono::system_clock::duration result_duration;

	// 3) test query
	{
		// - get start time
		auto time_start = std::chrono::system_clock::now();

		for(int i=0; i< 1000000; ++i)
		{
			// - set query
			//auto pquery = pquerier->execute_query("set foo bar"sv);

			// - get query
			//auto pquery = pquerier->execute_query("get foo"sv);

			//pquery->wait();

			// - request query
			pquerier->post_execute_query("set foo bar"sv, [&](auto /*_pquery*/)
				{
					++count;
				});

			//pquerier->post_execute_query("get foo"sv, [&](auto /*_pquery*/)
			//	{
			//		++count;
			//	});
		}

		// - get start time
		result_duration = std::chrono::system_clock::now() - time_start;

		// - output
		std::cout << "result: " << std::chrono::duration_cast<std::chrono::milliseconds>(result_duration).count() << std::endl;
		std::cout << "request: " << pquerier->statistics_get_query_try() << std::endl;
		std::cout << "complete: " << pquerier->statistics_get_query_success() + pquerier->statistics_get_query_fail() << std::endl;
	}

	_getch();

	// - output
	std::cout << "result: " << std::chrono::duration_cast<std::chrono::milliseconds>(result_duration).count() << "   count:" << count.load() << std::endl;
}
