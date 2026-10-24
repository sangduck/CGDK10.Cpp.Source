//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                      tutorials query - rdb.coroutine                      *
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

// option) SQL Server에 접속할 것인가? MySQL에 접속할 것인가? 주석처리하면 MySQL용 설정이된다.
#define _SQL_SERVER

#include <iostream>
#include <sstream>
#include <atomic>

// * CGDK header file
#include "cgdk/sdk10/server.query.rdb.h"

// * namespace CGDK
using namespace CGDK;
using namespace std::chrono_literals;


//-----------------------------------------------------------------------------
//
// tutorial.query.09.rdb_coroutine
//
// ------------------------------------------------------------------
//  설명)
//
//  querier와 query의 execute_query()는 'co_await'를 붙이느냐에 따라 다르게
//  동작한다.
//
//  1) 'co_await'를 붙이지 않으면 기존과 동일하게 동작한다.
//     - 쿼리가 완료될 때까지 스레드가 블로킹된다.
//     - 결과를 object_ptr<query>처럼 그대로 사용한다.
//
//  2) 코루틴에서 'co_await'를 붙이면 코루틴으로 동작한다.
//     - 쿼리는 system_executor의 다른 스레드에서 실행된다.
//     - 쿼리가 실행되는 동안 코루틴만 멈추고 스레드는 다른 일을 처리한다.
//     - 쿼리가 끝나면 코루틴이 이어서 실행되고 결과를 돌려받는다.
//     - 쿼리에서 발생한 예외는 'co_await'에서 다시 던져진다.
//
//  주요 함수)
//
//     auto pquery = pquerier->execute_query(...);            기존과 동일 (블로킹)
//     auto pquery = co_await pquerier->execute_query(...);   코루틴으로 실행
//     co_await pquery->execute_query(...);                   query 객체의 쿼리를 코루틴으로 실행
//     pquerier->execute_query_sync(...);                     항상 바로 실행 (블로킹)
//     auto psession = co_await pquerier->co_alloc_session();
//                                                            session을 받아 그 session으로 직접 쿼리한다.
//                                                            (지금 빈 session이 있으면 멈추지 않고 이 스레드에서 이어간다. 8번 예제)
//     auto psession = co_await pquerier->co_post_alloc_session();
//                                                            session이 할당될 때까지 멈추고, 그 session 스레드에서 이어간다.
//     auto pquery = co_await pquerier->co_post_execute_query(...);
//                                                            system_executor에서 실행하고 끝나면 이어간다. ('co_await execute_query()'와 같다)
//     auto pquery = co_await pquerier->co_execute_query(...);
//                                                            지금 빈 session이 있으면 이 스레드에서 바로 실행한다. (멈추지 않는다. 9번 예제)
//
//  주의)
//
//  1) 코루틴 안에서 'co_await' 없이 호출하면 결과를 처음 사용할 때(->, *, get() 등)
//     실행된다. 결과를 사용하지 않으면 그 문장이 끝날 때 실행된다.
//  2) 'co_await'로 실행할 때 쿼리 문자열과 파라미터는 복사되어 전달된다.
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

// - 쿼리 결과의 row들을 출력한다.
int print_result(query::rdb::query_t* _pquery)
{
	int count = 0;

	for (auto result = _pquery->move_first(); result == eRESULT::SUCCESS; result = _pquery->move_next())
	{
		auto value_02 = _pquery->result().get_value<std::string, char*>(2);	// char
		auto value_04 = _pquery->result().get_value<int32_t>(4);				// int

		std::cout << "    row " << ++count << " : " << value_02 << ", " << value_04 << std::endl;
	}

	return count;
}


//-----------------------------------------------------------------------------
// 1. 코루틴 밖에서 사용 - 기존과 동일
//
//    - 코루틴이 아닌 일반 함수에서 execute_query()를 호출하면 기존과 똑같이
//      쿼리가 완료될 때까지 블로킹된다.
//    - 기존 코드는 수정할 필요가 없다.
//-----------------------------------------------------------------------------
void example_1_without_coroutine(query::rdb::querier_t* _pquerier)
{
	// 1) 쿼리를 요청한다. (완료될 때까지 블로킹)
	auto pquery = _pquerier->execute_query("SELECT * FROM test_table"sv);

	// 2) 결과를 읽는다.
	print_result(pquery.get());
}


//-----------------------------------------------------------------------------
// 2. co_await querier->execute_query()
//
//    - 코루틴에서 execute_query() 앞에 'co_await'를 붙이면 쿼리는 다른
//      스레드에서 실행되고, 끝나면 코루틴이 이어서 실행된다.
//    - 'co_await'의 결과는 object_ptr<query>이다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_2_co_await(object_ptr<query::rdb::querier_t> _pquerier)
{
	std::cout << "    coroutine (thread " << thread_id() << ")" << std::endl;

	// 1) 쿼리를 요청하고 결과를 기다린다. (스레드는 블로킹되지 않는다)
	auto pquery = co_await _pquerier->execute_query("SELECT * FROM test_table"sv);

	std::cout << "    resumed   (thread " << thread_id() << ")" << std::endl;

	// 2) 결과를 읽는다.
	print_result(pquery.get());
}


//-----------------------------------------------------------------------------
// 3. 파라미터가 있는 쿼리
//
//    - 파라미터가 있는 execute_query()도 'co_await'를 붙여 사용할 수 있다.
//    - 파라미터는 복사되어 쿼리를 실행하는 스레드로 전달된다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_3_parameter(object_ptr<query::rdb::querier_t> _pquerier)
{
#if defined(_SQL_SERVER)
	auto pquery = co_await _pquerier->execute_query<query::rdb::query_param_t>("{CALL test_procedure (?,?,?)}"sv, 40, "apple"sv, 0.10f);
#else
	auto pquery = co_await _pquerier->execute_query<query::rdb::query_param_t>("CALL test_procedure (?,?,?)"sv, 40, "apple"sv, 0.10f);
#endif

	std::cout << "    test_procedure(40, 'apple', 0.10) done" << std::endl;
}


//-----------------------------------------------------------------------------
// 4. co_await pquery->execute_query()
//
//    - alloc_query()로 할당받은 query 객체의 execute_query()에도 'co_await'를
//      붙일 수 있다.
//    - 결과는 그 query 객체에서 읽는다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_4_query_object(object_ptr<query::rdb::querier_t> _pquerier)
{
	// 1) query 객체를 할당받는다.
	auto pquery = _pquerier->alloc_query();

	// 2) 쿼리를 요청하고 끝날 때까지 기다린다.
	co_await pquery->execute_query("SELECT * FROM test_table"sv);

	// 3) 결과를 읽는다.
	print_result(pquery.get());
}


//-----------------------------------------------------------------------------
// 5. 예외 처리
//
//    - 쿼리에서 발생한 예외는 'co_await'에서 다시 던져진다.
//    - 일반 함수처럼 try/catch로 잡는다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_5_exception(object_ptr<query::rdb::querier_t> _pquerier)
{
	try
	{
		co_await _pquerier->execute_query("SELECT * FROM no_such_table"sv);
	}
	catch (throwable::failure_ex& _e)
	{
		std::cout << "    exception: " << _e.description() << std::endl;
	}
}


//-----------------------------------------------------------------------------
// 6. 코루틴 안에서 'co_await' 없이 사용하기
//
//    - 코루틴 안에서 'co_await' 없이 호출하면 결과를 처음 사용할 때
//      (->, *, get() 등) 그 스레드에서 바로 실행된다. (블로킹)
//    - 결과를 사용하지 않으면 그 문장이 끝날 때 실행된다.
//    - 코루틴 안에서는 가능하면 'co_await'를 붙인다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_6_without_co_await(object_ptr<query::rdb::querier_t> _pquerier)
{
	// 1) 결과를 사용할 때 실행된다.
	auto pquery = _pquerier->execute_query("SELECT * FROM test_table"sv);

	print_result(pquery.get());

	// 2) execute_query_sync()는 항상 바로 실행된다.
	auto pquery_sync = _pquerier->execute_query_sync("SELECT * FROM test_table"sv);

	print_result(pquery_sync.get());

	co_return;
}


//-----------------------------------------------------------------------------
// 7. 여러 쿼리를 동시에 실행하기
//
//    - start()로 시작한 코루틴들은 동시에 실행된다.
//    - 각 코루틴의 쿼리도 서로 다른 스레드에서 동시에 실행된다.
//    - start()로 시작한 코루틴들이 모두 끝날 때까지 기다리려면 coroutine::wait_detached()를
//      호출한다. 프로그램을 끝내거나 querier를 정지하기 전에 호출한다.
//-----------------------------------------------------------------------------
coroutine::task<void> query_job(object_ptr<query::rdb::querier_t> _pquerier, int _id)
{
	auto pquery = co_await _pquerier->execute_query("SELECT * FROM test_table"sv);

	int count = 0;
	for (auto result = pquery->move_first(); result == eRESULT::SUCCESS; result = pquery->move_next())
		++count;

	std::cout << "    job " << _id << " : " << count << " rows" << std::endl;
}


//-----------------------------------------------------------------------------
// 8. co_alloc_session() / co_post_alloc_session()
//
//    - 여러 쿼리를 하나의 session(connection)으로, 또는 하나의 db transaction으로
//      실행하려면 session을 직접 받아서 쓴다. 돌려받은 object_ptr<session_t>를
//      가지고 있는 동안 그 session은 이 코루틴의 것이다. (놓으면 pool로 돌아간다)
//    - co_post_alloc_session() : session이 할당될 때까지 코루틴이 멈추고, 할당되면
//      그 session 스레드 위에서 이어서 실행된다. (이후의 블로킹 쿼리가 system_executor 스레드를 막지 않는다)
//    - co_alloc_session()      : 지금 바로 쓸 수 있는 session이 있으면 멈추지 않고 '이 스레드'에서
//      이어간다. (쿼리도 이 스레드에서 블로킹으로 실행된다) 없으면 co_post_alloc_session()과 같다.
//      요청을 받은 스레드에서 지연 없이 처리를 끝내고 싶을 때 쓴다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_8_session(object_ptr<query::rdb::querier_t> _pquerier, bool _post)
{
	// 1) session을 받는다.
	object_ptr<query::rdb::session_t> psession;

	if (_post)
		psession = co_await _pquerier->co_post_alloc_session();	// session이 생길 때까지 멈추고, session 스레드에서 이어간다.
	else
		psession = co_await _pquerier->co_alloc_session();		// 빈 session이 있으면 멈추지 않고 이 스레드에서 이어간다.

	std::cout << "    session allocated (" << (_post ? "co_post_alloc_session" : "co_alloc_session") << ")" << std::endl;

	// 2) 그 session으로 쿼리한다. (블로킹. session 스레드 또는 이 스레드에서 실행된다)
	auto pquery = psession->alloc_query();
	pquery->execute_query("SELECT * FROM test_table"sv);

	int count = 0;
	for (auto result = pquery->move_first(); result == eRESULT::SUCCESS; result = pquery->move_next())
		++count;

	std::cout << "    " << count << " rows" << std::endl;

	// 3) psession이 사라지면 session은 pool로 돌아간다.
}


//-----------------------------------------------------------------------------
// 9. co_execute_query() / co_post_execute_query()
//
//    - co_post_execute_query() : 쿼리를 system_executor의 다른 스레드에서 실행하고, 끝나면 코루틴이 이어진다.
//      ('co_await pquerier->execute_query()'와 같다. 이 스레드는 기다리지 않는다)
//    - co_execute_query()      : 지금 쓸 수 있는 session이 있으면 '이 스레드'에서 바로 실행한다.
//      (블로킹. 'co_await'해도 멈추지 않는다) 없으면 co_post_execute_query()와 같다.
//-----------------------------------------------------------------------------
coroutine::task<void> example_9_execute_vs_post(object_ptr<query::rdb::querier_t> _pquerier)
{
	// 1) post : 다른 스레드에서 실행되고 끝나면 이어진다.
	auto pquery_post = co_await _pquerier->co_post_execute_query("SELECT * FROM test_table"sv);

	int count_post = 0;
	for (auto result = pquery_post->move_first(); result == eRESULT::SUCCESS; result = pquery_post->move_next())
		++count_post;

	std::cout << "    co_post_execute_query : " << count_post << " rows" << std::endl;

	// 2) execute : 빈 session이 있으면 이 스레드에서 바로 실행된다.
	auto pquery_now = co_await _pquerier->co_execute_query("SELECT * FROM test_table"sv);

	int count_now = 0;
	for (auto result = pquery_now->move_first(); result == eRESULT::SUCCESS; result = pquery_now->move_next())
		++count_now;

	std::cout << "    co_execute_query      : " << count_now << " rows" << std::endl;
}


int main()
{
	// ------------------------------------------------------------------
	// A. querier 객체를 생성하고 시작한다. (query.01.rdb_simple 참고)
	// ------------------------------------------------------------------
	own_ptr<query::rdb::querier_t> pquerier = make_own<query::rdb::querier_t>();

	try
	{
	#if defined(_SQL_SERVER)
		pquerier->start(eDB_PROVIDER::SQL_SERVER, "www.cgcii.co.kr"sv, "test_user"sv, "1234"sv, "test_db"sv); // microsoft SQL server
	#else
		pquerier->start(eDB_PROVIDER::MYSQL, "www.cgcii.co.kr"sv, "test_user"sv, "1234"sv, "test_db"sv); // MySQL
	#endif
	}
	catch (throwable::failure_ex& _e)
	{
		_e >> std::cout;
		return 0;
	}

	object_ptr<query::rdb::querier_t> pquerier_ref(pquerier.get());

	try
	{
		// ------------------------------------------------------------------
		// B. 예제
		//    - 코루틴은 get()으로 실행하고 끝날 때까지 기다린다.
		// ------------------------------------------------------------------
		std::cout << ">> 1. without coroutine" << std::endl;
		example_1_without_coroutine(pquerier.get());

		std::cout << ">> 2. co_await querier->execute_query()" << std::endl;
		example_2_co_await(pquerier_ref).get();

		std::cout << ">> 3. parameter" << std::endl;
		example_3_parameter(pquerier_ref).get();

		std::cout << ">> 4. co_await pquery->execute_query()" << std::endl;
		example_4_query_object(pquerier_ref).get();

		std::cout << ">> 5. exception" << std::endl;
		example_5_exception(pquerier_ref).get();

		std::cout << ">> 6. without co_await in a coroutine" << std::endl;
		example_6_without_co_await(pquerier_ref).get();

		std::cout << ">> 7. queries at the same time" << std::endl;
		for (int i = 1; i <= 4; ++i)
			query_job(pquerier_ref, i).start();

		coroutine::wait_detached();

		std::cout << ">> 8. co_alloc_session() / co_post_alloc_session()" << std::endl;
		example_8_session(pquerier_ref, false).get();
		example_8_session(pquerier_ref, true).get();

		std::cout << ">> 9. co_execute_query() / co_post_execute_query()" << std::endl;
		example_9_execute_vs_post(pquerier_ref).get();
	}
	catch (throwable::failure_ex& _e)
	{
		_e >> std::cout;
	}

	// ------------------------------------------------------------------
	// C. querier를 정지한다.
	// ------------------------------------------------------------------
	pquerier->stop();

	// return)
	return 0;
}
