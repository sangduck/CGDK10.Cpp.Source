//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials query - http.coroutine                      *
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
#include "cgdk/sdk10/server.query.http.h"

// * namespace CGDK
using namespace CGDK;
using namespace std::chrono_literals;


//-----------------------------------------------------------------------------
//
// tutorial.query.11.http_coroutine
//
// ------------------------------------------------------------------
//  설명)
//
//  http querier와 query의 execute_query()는 'co_await'를 붙이느냐에 따라
//  다르게 동작한다.
//
//  1) 'co_await'를 붙이지 않으면 기존과 동일하게 동작한다. (query.07.http 참고)
//     - 응답을 받을 때까지 스레드가 블로킹된다.
//
//  2) 코루틴에서 'co_await'를 붙이면 코루틴으로 동작한다.
//     - 요청을 보내고 코루틴은 멈춘다. 응답을 기다리는 스레드는 없다.
//     - 응답이 도착하면 코루틴이 이어서 실행되고 결과를 돌려받는다.
//
//  주요 함수)
//
//     auto pquery = co_await pquerier->execute_query<query::http::query_t>(url);   querier로 요청
//     co_await pquery->execute_query(url);                                         query 객체로 요청
//     auto pquery = co_await pquerier->co_post_execute_query<query::http::query_t>(url);
//                                                                                  요청을 보내고 응답이 오면 이어간다. ('co_await execute_query()'와 같다)
//     auto pquery = co_await pquerier->co_execute_query<query::http::query_t>(url);
//                                                                                  이 스레드에서 바로 보내고 응답까지 기다린다. (멈추지 않는다. 4번 예제)
//
// ------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// helper) http 응답의 첫 번째 줄(status line)을 출력한다.
//-----------------------------------------------------------------------------
void print_status_line(query::http::query_t* _pquery)
{
	auto buf_head = _pquery->result().buf_head;
	auto status_line = buf_head.extract_text<char>('\n', (char)NULL);

	std::cout << "    " << status_line << std::endl;
}


//-----------------------------------------------------------------------------
// 1. co_await querier->execute_query()
//
//    - 'co_await'의 결과는 object_ptr<query>이다.
//    - 응답이 도착한 후 이어서 실행되므로 바로 결과를 읽을 수 있다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_1_co_await(object_ptr<query::http::querier_t> _pquerier)
{
	auto pquery = co_await _pquerier->execute_query<query::http::query_t>("https://www.google.com"sv);

	print_status_line(pquery.get());
}


//-----------------------------------------------------------------------------
// 2. co_await pquery->execute_query()
//
//    - alloc_query()로 할당받은 query 객체의 execute_query()에도 'co_await'를
//      붙일 수 있다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_2_query_object(object_ptr<query::http::querier_t> _pquerier)
{
	// 1) query 객체를 할당받는다.
	auto pquery = _pquerier->alloc_query<query::http::query_t>();

	// 2) 요청하고 응답을 기다린다.
	co_await pquery->execute_query("https://www.google.com"sv);

	// 3) 결과를 출력한다.
	print_status_line(pquery.get());
}


//-----------------------------------------------------------------------------
// 3. 여러 요청을 동시에 실행하기
//
//    - start()로 시작한 코루틴들은 동시에 실행된다.
//    - 응답을 기다리는 동안 스레드를 붙잡지 않는다.
//    - start()로 시작한 코루틴들이 모두 끝날 때까지 기다리려면 coroutine::wait_detached()를
//      호출한다. 프로그램을 끝내거나 querier를 정지하기 전에 호출한다.
//-----------------------------------------------------------------------------
coroutine::task<void> request_job(object_ptr<query::http::querier_t> _pquerier, std::string _url)
{
	auto pquery = co_await _pquerier->execute_query<query::http::query_t>(_url);

	std::cout << "    " << _url << std::endl;
	print_status_line(pquery.get());
}


//-----------------------------------------------------------------------------
// 4. co_execute_query() / co_post_execute_query()
//
//    - co_post_execute_query() : 요청을 보내고 응답이 오면 코루틴이 이어진다. ('co_await execute_query()'와 같다)
//    - co_execute_query()      : 이 스레드에서 바로 보내고 응답까지 기다린다. (블로킹. 'co_await'해도 멈추지 않는다)
//-----------------------------------------------------------------------------
coroutine::task<void> example_4_execute_vs_post(object_ptr<query::http::querier_t> _pquerier)
{
	// 1) post : 응답이 오면 이어진다.
	auto pquery_post = co_await _pquerier->co_post_execute_query<query::http::query_t>("https://www.google.com"sv);
	print_status_line(pquery_post.get());

	// 2) execute : 이 스레드에서 바로 보내고 응답까지 기다린다.
	auto pquery_now = co_await _pquerier->co_execute_query<query::http::query_t>("https://www.google.com"sv);
	print_status_line(pquery_now.get());
}


int main()
{
	// ------------------------------------------------------------------
	// A. http querier를 생성하고 시작한다.
	// ------------------------------------------------------------------
	auto pquerier = make_object<query::http::querier_t>();

	pquerier->start();

	try
	{
		// ------------------------------------------------------------------
		// B. 예제
		//    - 코루틴은 get()으로 실행하고 끝날 때까지 기다린다.
		// ------------------------------------------------------------------
		std::cout << ">> 1. co_await querier->execute_query()" << std::endl;
		example_1_co_await(pquerier).get();

		std::cout << ">> 2. co_await pquery->execute_query()" << std::endl;
		example_2_query_object(pquerier).get();

		std::cout << ">> 3. requests at the same time" << std::endl;
		request_job(pquerier, "https://www.google.com").start();
		request_job(pquerier, "https://www.naver.com").start();
		request_job(pquerier, "https://www.github.com").start();

		coroutine::wait_detached();

		std::cout << ">> 4. co_execute_query() / co_post_execute_query()" << std::endl;
		example_4_execute_vs_post(pquerier).get();
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
