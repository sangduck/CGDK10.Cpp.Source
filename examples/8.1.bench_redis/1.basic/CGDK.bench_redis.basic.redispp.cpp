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
// * CGDK header file
//#include "sw/redis++/redis++.h"

// * CGDK
//using namespace sw::redis;


void test_basic_redispp()
{
 //   try
	//{
 // //      // Redis 클라이언트 초기화
	//	//redis->set("", "");

	//	//redis::client client("localhost", 6379);

 // //      // 데이터 저장
 // //      client.set("my_key", "Hello, Redis++!");

 // //      // 데이터 조회
 // //      std::string value;
 // //      client.get("my_key", value);
 // //      std::cout << "Value for key 'my_key': " << value << std::endl;
 //   }
	//catch (const std::exception& e)
	//{
 //       std::cerr << "Error: " << e.what() << std::endl;
 //   }

	////-----------------------------------------------------------------
	//// 1. redis querier의 초기화.
	////-----------------------------------------------------------------
	//{
	//	// 1) redis querier를 생성한다.
	//	auto pquerier = make_object<query::redis::querier_t>();

	//	// 2) redis querier를 시작한다. (ip의 기본값은 'localhost:6379' 이다.)
	//	pquerier->start();

	//	try
	//	{
	//		// 1) 연결해서 쿼리를 한다.
	//		auto pquery = pquerier->execute_query("set foo bar"sv);

	//	}
	//	catch (...)
	//	{
	//		std::cout << "redis query failed!"sv << '\n';
	//	}

	//	try
	//	{
	//		// 1) 연결해서 또 쿼리를 한다.
	//		pquerier->post_execute_query("get users:1"sv, [=](auto /*_pquery*/)
	//			{
	//			});
	//	}
	//	catch (...)
	//	{
	//		std::cout << "redis query failed!"sv << '\n';
	//	}
	//}
}
