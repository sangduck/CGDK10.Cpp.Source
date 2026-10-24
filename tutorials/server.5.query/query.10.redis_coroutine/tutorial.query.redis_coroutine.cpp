//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials query - redis.coroutine                     *
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

#include <iostream>
#include <atomic>

// * CGDK header file
#include "cgdk/sdk10/server.query.redis.h"

// * namespace CGDK
using namespace CGDK;
using namespace std::chrono_literals;


//-----------------------------------------------------------------------------
//
// tutorial.query.10.redis_coroutine
//
// ------------------------------------------------------------------
//  설명)
//
//  redis querier와 query의 execute_query()는 'co_await'를 붙이느냐에 따라
//  다르게 동작한다.
//
//  1) 'co_await'를 붙이지 않으면 기존과 동일하게 동작한다. (query.06.redis 참고)
//
//  2) 코루틴에서 'co_await'를 붙이면 코루틴으로 동작한다.
//     - 요청을 보내고 코루틴은 멈춘다. 응답을 기다리는 스레드는 없다.
//     - 응답이 도착하면 코루틴이 이어서 실행되고 결과를 돌려받는다.
//
//  주요 함수)
//
//     auto pquery = co_await pquerier->execute_query("get users:1"sv);      querier로 요청
//     auto pquery = co_await pquerier->execute_query<query::redis::query>("get users:{}"sv, 1); 포맷 문자열
//     co_await pquery->execute_query("get users:1"sv);                       query 객체로 요청
//     auto pquery = co_await pquerier->co_post_execute_query("get users:1"sv);  요청을 보내고 응답이 오면 이어간다. ('co_await execute_query()'와 같다)
//     auto pquery = co_await pquerier->co_execute_query("get users:1"sv);       이 스레드에서 바로 보내고 응답까지 기다린다. (멈추지 않는다. 5번 예제)
//     auto pquery = co_await pquerier->co_alloc_query();                        전용 session이 붙은 query 객체 (빈 session이 있으면 멈추지 않는다. 6번 예제)
//     auto pquery = co_await pquerier->co_post_alloc_query();                   전용 session이 붙은 query 객체 (session이 할당될 때까지 멈춘다)
//
//  주의)
//
//  1) 같은 query 객체로 다시 요청하려면 close_query()를 호출한 후 요청한다.
//  2) 연결이 끊어지면 기다리던 요청은 다시 연결된 후 재전송된다.
//
// ------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helper) redis 결과를 출력한다.
//-----------------------------------------------------------------------------
void print_redis_result(const resp_data& _result)
{
	if (_result.is_failed())
	{
		std::cout << "    (failed)" << std::endl;
		return;
	}

	std::cout << "    " << _result.str_value << std::endl;

	for (auto& iter : _result.vector_child)
		print_redis_result(iter);
}


//-----------------------------------------------------------------------------
// 1. co_await querier->execute_query()
//
//    - 'co_await'의 결과는 object_ptr<query>이다.
//    - 응답이 도착한 후 이어서 실행되므로 바로 결과를 읽을 수 있다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_1_co_await(object_ptr<query::redis::querier_t> _pquerier)
{
	// 1) set
	co_await _pquerier->execute_query("set users:1 world"sv);

	// 2) get
	auto pquery = co_await _pquerier->execute_query("get users:1"sv);

	// 3) 결과를 출력한다.
	print_redis_result(pquery->result());
}


//-----------------------------------------------------------------------------
// 2. 포맷 문자열로 요청하기
//
//    - std::format 형식의 문자열과 인자를 넘길 수 있다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_2_format(object_ptr<query::redis::querier_t> _pquerier)
{
	auto pquery = co_await _pquerier->execute_query<query::redis::query>("get users:{}"sv, 1);

	print_redis_result(pquery->result());
}


//-----------------------------------------------------------------------------
// 3. co_await pquery->execute_query()
//
//    - alloc_query()로 query 객체를 할당받아 같은 session으로 여러 번 요청할 수 있다.
//    - 다시 요청하기 전에 close_query()를 호출한다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_3_query_object(object_ptr<query::redis::querier_t> _pquerier)
{
	// 1) query 객체를 할당받는다.
	auto pquery = _pquerier->alloc_query();

	// 2) 첫 번째 요청
	co_await pquery->execute_query("set users:2 alloc_query_test!"sv);
	print_redis_result(pquery->result());

	// 3) 첫 번째 query를 닫는다.
	pquery->close_query();

	// 4) 두 번째 요청
	co_await pquery->execute_query("get users:2"sv);
	print_redis_result(pquery->result());
}


//-----------------------------------------------------------------------------
// 4. 여러 요청을 동시에 실행하기
//
//    - start()로 시작한 코루틴들은 동시에 실행된다.
//    - 응답을 기다리는 동안 스레드를 붙잡지 않으므로 많은 요청을 동시에
//      처리할 수 있다.
//    - start()로 시작한 코루틴들이 모두 끝날 때까지 기다리려면 coroutine::wait_detached()를
//      호출한다. 프로그램을 끝내거나 querier를 정지하기 전에 호출한다.
//-----------------------------------------------------------------------------
coroutine::task<void> request_job(object_ptr<query::redis::querier_t> _pquerier, int _id)
{
	co_await _pquerier->execute_query<query::redis::query>("set users:job{} {}"sv, _id, _id * 10);
	auto pquery = co_await _pquerier->execute_query<query::redis::query>("get users:job{}"sv, _id);

	std::cout << "    job " << _id << " : " << pquery->result().str_value << std::endl;
}


//-----------------------------------------------------------------------------
// 5. co_execute_query() / co_post_execute_query()
//
//    - co_post_execute_query() : 요청을 보내고 응답이 오면 코루틴이 이어진다. ('co_await execute_query()'와 같다)
//    - co_execute_query()      : 지금 쓸 수 있는 session이 있으면 이 스레드에서 바로 보내고 응답까지 기다린다.
//      (블로킹. 'co_await'해도 멈추지 않는다) 없으면 co_post_execute_query()와 같다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_5_execute_vs_post(object_ptr<query::redis::querier_t> _pquerier)
{
	// 1) post : 응답이 오면 이어진다.
	auto pquery_post = co_await _pquerier->co_post_execute_query("get users:1"sv);
	print_redis_result(pquery_post->result());

	// 2) execute : 이 스레드에서 바로 보내고 응답까지 기다린다.
	auto pquery_now = co_await _pquerier->co_execute_query("get users:1"sv);
	print_redis_result(pquery_now->result());
}


//-----------------------------------------------------------------------------
// 6. co_alloc_query() / co_post_alloc_query()
//
//    - 전용 session이 붙은 query 객체를 받는다. 가지고 있는 동안 그 session은 이 query의 것이다.
//      (같은 연결로 여러 명령을 보낼 때. alloc_query()의 코루틴 판)
//    - co_post_alloc_query() : 항상 멈추고, session이 할당되면 executor에서 이어진다.
//    - co_alloc_query()      : 지금 빈 session이 있으면 멈추지 않고 바로 돌려준다. 없으면 co_post_alloc_query()와 같다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_6_alloc_query(object_ptr<query::redis::querier_t> _pquerier)
{
	// 1) 빈 session이 있으면 멈추지 않는다.
	auto pquery = co_await _pquerier->co_alloc_query();

	co_await pquery->execute_query("set users:3 co_alloc_query"sv);
	pquery->close_query();

	co_await pquery->execute_query("get users:3"sv);
	print_redis_result(pquery->result());

	// 2) 항상 멈췄다가 session이 할당되면 이어진다.
	auto pquery_post = co_await _pquerier->co_post_alloc_query();

	co_await pquery_post->execute_query("get users:3"sv);
	print_redis_result(pquery_post->result());
}


int main()
{
	// ------------------------------------------------------------------
	// A. redis querier를 생성하고 시작한다. (ip의 기본값은 'localhost:6379' 이다.)
	// ------------------------------------------------------------------
	auto pquerier = make_object<query::redis::querier_t>();

	pquerier->start();

	try
	{
		// ------------------------------------------------------------------
		// B. 예제
		//    - 코루틴은 get()으로 실행하고 끝날 때까지 기다린다.
		// ------------------------------------------------------------------
		std::cout << ">> 1. co_await querier->execute_query()" << std::endl;
		example_1_co_await(pquerier).get();

		std::cout << ">> 2. format" << std::endl;
		example_2_format(pquerier).get();

		std::cout << ">> 3. co_await pquery->execute_query()" << std::endl;
		example_3_query_object(pquerier).get();

		std::cout << ">> 4. requests at the same time" << std::endl;
		for (int i = 1; i <= 5; ++i)
			request_job(pquerier, i).start();

		coroutine::wait_detached();

		std::cout << ">> 5. co_execute_query() / co_post_execute_query()" << std::endl;
		example_5_execute_vs_post(pquerier).get();

		std::cout << ">> 6. co_alloc_query() / co_post_alloc_query()" << std::endl;
		example_6_alloc_query(pquerier).get();
	}
	catch (throwable::failure& _e)
	{
		std::cout << "exception: " << _e.description() << std::endl;
	}

	// ------------------------------------------------------------------
	// C. querier를 정지한다.
	// ------------------------------------------------------------------
	pquerier->stop();

	// return)
	return 0;
}
