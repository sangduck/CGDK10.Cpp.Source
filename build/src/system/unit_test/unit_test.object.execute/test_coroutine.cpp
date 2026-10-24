#include "pch.h"
#include <atomic>
#include <memory>
#include <thread>

//-----------------------------------------------------------------------------
// coroutine::task - how it is started and where it runs (auto-detected)
//
//  - post()    : the task is posted (or a function is wrapped and posted). nothing of it runs before post() returns.
//  - execute() : the task runs right here until it suspends for the first time,
//                   then it continues on the system_executor. (never nested, never on a strand)
//  - stopped executor : nothing runs inline. start()/post() throw NOT_READY and
//                   a coroutine waiting to be resumed on a stopped executor is cancelled.
//-----------------------------------------------------------------------------
namespace CGDK
{
	// wait until _predicate() is true (timeout : false)
	template <class TPREDICATE>
	static bool wait_until_true(TPREDICATE&& _predicate, chrono::tick::duration _timeout = 5s)
	{
		auto tick_end = chrono::tick::now() + _timeout;
		while (!_predicate())
		{
			if (chrono::tick::now() > tick_end)
				return false;
			api::sleep_for(1ms);
		}
		return true;
	}

	namespace
	{
		// - records where a coroutine runs: before and after its first real suspension
		struct sRUN_RECORD
		{
			std::atomic<bool>	started{ false };
			std::atomic<bool>	finished{ false };
			std::thread::id		thread_before;		// the thread of the first segment
			std::thread::id		thread_after;		// the thread after the first suspension
		};

		// - suspends once (co_sleep_for) and records the threads
		coroutine::task<void> run_and_suspend(sRUN_RECORD* _precord)
		{
			_precord->thread_before = std::this_thread::get_id();
			_precord->started = true;

			co_await co_sleep_for(50ms);		// the first real suspension

			_precord->thread_after = std::this_thread::get_id();
			_precord->finished = true;
		}

		// - never suspends
		coroutine::task<void> run_without_suspend(sRUN_RECORD* _precord)
		{
			_precord->thread_before = std::this_thread::get_id();
			_precord->started = true;
			_precord->finished = true;
			co_return;
		}

		// - keeps '_pholder' in the frame: its use_count tells whether the frame is alive
		coroutine::task<void> hold_only(std::shared_ptr<int> _pholder, std::atomic<bool>* _pstarted)
		{
			*_pstarted = true;
			(void)_pholder;
			co_return;
		}

		// - keeps '_pholder' in the frame and suspends for a while
		coroutine::task<void> hold_and_sleep(std::shared_ptr<int> _pholder, std::atomic<bool>* _psuspending, std::atomic<bool>* _presumed)
		{
			*_psuspending = true;
			co_await co_sleep_for(200ms);
			*_presumed = true;
			(void)_pholder;
		}

		coroutine::task<int> add_values(int _a, int _b)
		{
			co_return _a + _b;
		}

		coroutine::task<int> fail_task()
		{
			throw throwable::failure(eRESULT::FAIL, 0, "fail_task"sv);
			co_return 0;
		}

		// - a request-response with a timeout (like Nsocket_server::co_request_response)
		//   the response is posted at a time point, the timeout is posted with a duration (system_executor::post(executable, duration))
		//   the first one wins and sets the result (1: response, 0: timeout)
		coroutine::task<void> request_with_timeout(chrono::tick::duration _response_after, chrono::tick::duration _timeout, std::atomic<int>* _presult, std::atomic<int64_t>* _pelapsed_us)
		{
			coroutine::async_result<int> result;
			auto pclaimed = std::make_shared<std::atomic<bool>>(false);
			auto complete = [result, pclaimed](int _value) { if (!pclaimed->exchange(true)) result.set_result(_value); };

			auto tick_start = chrono::tick::now();

			system_executor::post(make_executable([complete] { complete(1); }), tick_start + _response_after);
			system_executor::post(make_executable([complete] { complete(0); }), _timeout);

			auto value = co_await result;

			*_pelapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_start).count();
			*_presult = value;
		}

		// - calls execute() inside a coroutine and records whether the inner task ran inline on this thread
		coroutine::task<void> outer_executes_inner(sRUN_RECORD* _pinner, std::atomic<bool>* _pran_inline)
		{
			system_executor::execute(run_without_suspend(_pinner));

			// - inline: it already ran on this very thread. posted: it runs on another thread (this one is busy here)
			*_pran_inline = _pinner->started.load() && _pinner->thread_before == std::this_thread::get_id();
			co_return;
		}
	}

	// 1) get() - the result and the exception of the task
	TEST(CGDK10_coroutine, get_returns_result)
	{
		EXPECT_EQ(add_values(2, 3).get(), 5);
	}

	TEST(CGDK10_coroutine, get_rethrows_exception)
	{
		EXPECT_THROW(fail_task().get(), throwable::failure);
	}

	// 2) system_executor::post - the task runs on an executor thread, never on the calling thread
	TEST(CGDK10_coroutine, post_runs_on_executor)
	{
		sRUN_RECORD record;

		system_executor::post(run_and_suspend(&record));

		ASSERT_TRUE(wait_until_true([&] { return record.finished.load(); }));
		EXPECT_NE(record.thread_before, std::this_thread::get_id());
		EXPECT_NE(record.thread_after, std::this_thread::get_id());

		coroutine::wait_detached();
	}

	// 3) system_executor::execute - the first segment runs right here, the rest on the executor
	TEST(CGDK10_coroutine, execute_runs_here_until_first_suspend)
	{
		sRUN_RECORD record;

		system_executor::execute(run_and_suspend(&record));

		// - the first segment ran on this thread before execute() returned
		EXPECT_TRUE(record.started.load());
		EXPECT_EQ(record.thread_before, std::this_thread::get_id());

		// - the rest runs on an executor thread after the suspension
		ASSERT_TRUE(wait_until_true([&] { return record.finished.load(); }));
		EXPECT_NE(record.thread_after, std::this_thread::get_id());

		coroutine::wait_detached();
	}

	// 4) execute - a task that never suspends is finished when execute() returns
	TEST(CGDK10_coroutine, execute_without_suspend_finishes_at_once)
	{
		sRUN_RECORD record;

		system_executor::execute(run_without_suspend(&record));

		EXPECT_TRUE(record.finished.load());
		EXPECT_EQ(record.thread_before, std::this_thread::get_id());

		// - the frame is already released: returns at once
		coroutine::wait_detached();
	}

	// 5) execute inside a coroutine - posted, not nested (no recursion)
	TEST(CGDK10_coroutine, execute_inside_coroutine_is_posted)
	{
		sRUN_RECORD inner;
		std::atomic<bool> ran_inline{ true };

		outer_executes_inner(&inner, &ran_inline).get();

		EXPECT_FALSE(ran_inline.load());
		ASSERT_TRUE(wait_until_true([&] { return inner.finished.load(); }));

		coroutine::wait_detached();
	}

	// 6) task::execute on a strand - posted, not run here (a strand never runs its coroutines on a foreign thread)
	TEST(CGDK10_coroutine, execute_on_strand_is_posted)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		sRUN_RECORD record;

		run_without_suspend(&record).execute(object_ptr<Iexecutor>(pstrand.get()));

		ASSERT_TRUE(wait_until_true([&] { return record.finished.load(); }));
		EXPECT_NE(record.thread_before, std::this_thread::get_id());

		coroutine::wait_detached();
		pstrand->stop();
	}

	// 7) start()/post() on a stopped executor - throws NOT_READY, nothing runs, the frame is destroyed
	TEST(CGDK10_coroutine, start_on_stopped_executor_throws_and_destroys_frame)
	{
		auto pstrand = make_own<executor::strand>();		// not started: it refuses every post

		auto pholder = std::make_shared<int>(0);
		std::atomic<bool> started{ false };

		EXPECT_THROW(hold_only(pholder, &started).start(object_ptr<Iexecutor>(pstrand.get())), throwable::failure);
		EXPECT_THROW(pstrand->post(hold_only(pholder, &started)), throwable::failure);

		// - never run, and the frames (with their copies of 'pholder') are destroyed
		EXPECT_FALSE(started.load());
		EXPECT_EQ(pholder.use_count(), 1);

		// - the cancelled coroutines are not counted as detached any more: returns at once
		coroutine::wait_detached();
	}

	// 8) a coroutine waiting to be resumed on a stopped executor - cancelled, never run inline
	TEST(CGDK10_coroutine, resume_on_stopped_executor_is_cancelled)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		auto pholder = std::make_shared<int>(0);
		std::atomic<bool> suspending{ false };
		std::atomic<bool> resumed{ false };

		// 1) the coroutine suspends in co_sleep_for(200ms) on the strand
		pstrand->post(hold_and_sleep(pholder, &suspending, &resumed));

		ASSERT_TRUE(wait_until_true([&] { return suspending.load(); }));
		api::sleep_for(50ms);

		// 2) the strand is stopped while the coroutine sleeps: it refuses to resume it
		pstrand->stop();

		// 3) when the time comes the coroutine is cancelled: the rest never runs, the frame is destroyed
		EXPECT_TRUE(wait_until_true([&] { return pholder.use_count() == 1; }));
		EXPECT_FALSE(resumed.load());

		coroutine::wait_detached();
	}

	// 9) co_await async_result with a timeout posted by duration - the response (50ms) arrives before the timeout (1s)
	//    (regression: system_executor::post(executable, duration) scheduled at 'epoch + duration' = the past,
	//     so the timeout fired at once and the co_await returned the empty timeout result without waiting)
	TEST(CGDK10_coroutine, async_result_waits_for_response_before_duration_timeout)
	{
		std::atomic<int> result{ -1 };
		std::atomic<int64_t> elapsed_us{ -1 };

		system_executor::post(request_with_timeout(50ms, 1s, &result, &elapsed_us));

		ASSERT_TRUE(wait_until_true([&] { return result.load() >= 0; }));
		EXPECT_EQ(result.load(), 1);				// the response, not the timeout
		EXPECT_GE(elapsed_us.load(), 50000);		// it really waited for the response
		EXPECT_LT(elapsed_us.load(), 1000000);

		api::sleep_for(1100ms);						// let the (losing) timeout run before the frame is gone
		coroutine::wait_detached();
	}

	// 10) the timeout posted by duration fires after the duration, not before (the response is too late : 1s, the timeout : 100ms)
	TEST(CGDK10_coroutine, async_result_duration_timeout_fires_after_duration)
	{
		std::atomic<int> result{ -1 };
		std::atomic<int64_t> elapsed_us{ -1 };

		system_executor::post(request_with_timeout(1s, 100ms, &result, &elapsed_us));

		ASSERT_TRUE(wait_until_true([&] { return result.load() >= 0; }));
		EXPECT_EQ(result.load(), 0);				// the timeout
		EXPECT_GE(elapsed_us.load(), 100000);		// not before the duration
		EXPECT_LT(elapsed_us.load(), 1000000);		// before the (late) response

		api::sleep_for(1100ms);						// let the (losing) response run before the frame is gone
		coroutine::wait_detached();
	}

	// 11) post(owner, task) - the owner is kept alive until the task finishes. no wrapper coroutine frame (the task is the only detached frame)
	TEST(CGDK10_coroutine, post_owner_kept_alive_without_wrapper_frame)
	{
		coroutine::wait_detached();

		auto powner = make_executable([] {});
		auto pholder = std::make_shared<int>(0);
		std::atomic<bool> suspending{ false };
		std::atomic<bool> resumed{ false };

		auto count_ref = powner->reference_count();

		system_executor::post(powner, hold_and_sleep(pholder, &suspending, &resumed));

		ASSERT_TRUE(wait_until_true([&] { return suspending.load(); }));
		EXPECT_EQ(coroutine::count_detached(), std::size_t(1));	// the task itself only
		EXPECT_EQ(powner->reference_count(), count_ref + 1);		// held by the task

		coroutine::wait_detached();
		EXPECT_TRUE(resumed.load());
		EXPECT_EQ(pholder.use_count(), 1);
		EXPECT_EQ(powner->reference_count(), count_ref);			// released with the frame
	}

	// 12) post(owner, task, completion) - the completion gets the exception of the task while the owner is still held
	TEST(CGDK10_coroutine, post_owner_completion_gets_exception)
	{
		coroutine::wait_detached();

		auto powner = make_executable([] {});
		auto count_ref = powner->reference_count();
		std::atomic<int> result{ -1 };				// 0: no exception, 1: throwable::failure, 2: another exception
		std::atomic<int> count_ref_in_completion{ 0 };

		system_executor::post(powner, fail_task(), [&, powner_raw = powner.get()](std::exception_ptr _exception)
		{
			count_ref_in_completion = powner_raw->reference_count();
			try { if (_exception) std::rethrow_exception(_exception); result = 0; }
			catch (const throwable::failure&) { result = 1; }
			catch (...) { result = 2; }
		});

		ASSERT_TRUE(wait_until_true([&] { return result.load() >= 0; }));
		EXPECT_EQ(result.load(), 1);
		EXPECT_EQ(count_ref_in_completion.load(), count_ref + 1);	// still held when the completion runs

		coroutine::wait_detached();
		EXPECT_EQ(powner->reference_count(), count_ref);
	}

	// 13) execute(owner, task, completion) - a task that never suspends: finished and completed before execute() returns
	TEST(CGDK10_coroutine, execute_owner_completion_called_inline)
	{
		auto powner = make_executable([] {});
		sRUN_RECORD record;
		bool completed = false;

		system_executor::execute(powner, run_without_suspend(&record), [&](std::exception_ptr _exception) { completed = (_exception == nullptr); });

		EXPECT_TRUE(record.finished.load());
		EXPECT_TRUE(completed);
		EXPECT_EQ(record.thread_before, std::this_thread::get_id());

		coroutine::wait_detached();
	}

	//-----------------------------------------------------------------------------
	// performance (printed. the asserts are only sanity checks)
	//-----------------------------------------------------------------------------
	namespace
	{
		// - awaits '_count' child tasks
		coroutine::task<int64_t> await_children(int _count)
		{
			int64_t sum = 0;
			for (int i = 0; i < _count; ++i)
				sum += co_await add_values(i, 1);
			co_return sum;
		}

		// - a task which finishes at once
		coroutine::task<void> empty_task()
		{
			co_return;
		}

		// - suspends and is resumed by the scheduler '_count' times (1us: the round trip of the scheduler, not the sleep itself)
		coroutine::task<void> sleep_repeatedly(int _count)
		{
			for (int i = 0; i < _count; ++i)
				co_await co_sleep_for(1us);
		}

		template <class TFUNCTION>
		double measure_ns_per_op(int _count, TFUNCTION&& _function)
		{
			auto begin = std::chrono::steady_clock::now();
			_function();
			auto elapsed = std::chrono::steady_clock::now() - begin;
			return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count()) / _count;
		}
	}

	TEST(CGDK10_coroutine_perf, await_child_task)
	{
		constexpr int COUNT = 300000;

		auto ns = measure_ns_per_op(COUNT, [&] { EXPECT_EQ(await_children(COUNT).get(), int64_t(COUNT) + int64_t(COUNT - 1) * COUNT / 2); });
		std::printf("[  perf    ] co_await child task      : %8.1f ns/op (%d)\n", ns, COUNT);
		EXPECT_LT(ns, 20000.0);
	}

	TEST(CGDK10_coroutine_perf, post_with_owner)
	{
		constexpr int COUNT = 100000;

		auto powner = make_executable([] {});

		auto ns = measure_ns_per_op(COUNT, [&]
		{
			for (int i = 0; i < COUNT; ++i)
				system_executor::post(powner, empty_task());
			coroutine::wait_detached();
		});
		std::printf("[  perf    ] post(owner, task)     : %8.1f ns/op (%d)\n", ns, COUNT);
		EXPECT_LT(ns, 50000.0);
	}

	TEST(CGDK10_coroutine_perf, post_with_completion)
	{
		constexpr int COUNT = 100000;

		std::atomic<int> completed{ 0 };

		auto ns = measure_ns_per_op(COUNT, [&]
		{
			for (int i = 0; i < COUNT; ++i)
				system_executor::post(empty_task(), [&](std::exception_ptr) { ++completed; });
			coroutine::wait_detached();
		});
		std::printf("[  perf    ] post(task, completion): %8.1f ns/op (%d)\n", ns, COUNT);
		EXPECT_EQ(completed.load(), COUNT);
		EXPECT_LT(ns, 50000.0);
	}

	TEST(CGDK10_coroutine_perf, co_sleep_for_tick)
	{
		constexpr int COUNT = 2000;

		auto ns = measure_ns_per_op(COUNT, [&] { sleep_repeatedly(COUNT).get(); });
		std::printf("[  perf    ] co_sleep_for(1us)        : %8.1f ns/op (%d)\n", ns, COUNT);
		EXPECT_LT(ns, 20000000.0);
	}
}
