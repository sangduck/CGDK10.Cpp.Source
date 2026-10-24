#include "pch.h"
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace CGDK
{
	// wait until _predicate() is true (timeout : false)
	template <class TPREDICATE>
	static bool wait_until(TPREDICATE&& _predicate, chrono::tick::duration _timeout = 5s)
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

	// 1) system_executor::post - all posted executables are executed
	TEST(CGDK10_executor, system_executor_post)
	{
		std::atomic<int> count{ 0 };

		for (int i = 0; i < 1000; ++i)
			EXPECT_TRUE(system_executor::post(make_executable([&count] { ++count; })));

		EXPECT_TRUE(wait_until([&] { return count == 1000; }));
	}

	// 2) system_executor::post(executable, time) - executed at the time (not before)
	TEST(CGDK10_executor, system_executor_post_at_time)
	{
		std::atomic<int64_t> elapsed_us{ -1 };

		auto tick_start = chrono::tick::now();
		auto tick_execute = tick_start + 50ms;

		system_executor::post(make_executable([&] { elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_start).count(); }), tick_execute);

		ASSERT_TRUE(wait_until([&] { return elapsed_us >= 0; }));

		// - not before the time / not too late (generous for loaded machines)
		EXPECT_GE(elapsed_us.load(), 50000);
		EXPECT_LT(elapsed_us.load(), 50000 + 30000);
	}

	// 2-1) system_executor::post(executable, duration) - executed after the duration from now (not at 'epoch + duration')
	TEST(CGDK10_executor, system_executor_post_after_duration)
	{
		std::atomic<int64_t> elapsed_us{ -1 };

		auto tick_start = chrono::tick::now();

		system_executor::post(make_executable([&] { elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_start).count(); }), chrono::tick::duration(50ms));

		ASSERT_TRUE(wait_until([&] { return elapsed_us >= 0; }));

		// - not before the duration / not too late (generous for loaded machines)
		EXPECT_GE(elapsed_us.load(), 50000);
		EXPECT_LT(elapsed_us.load(), 50000 + 30000);
	}

	// 3) system_executor::post(executable, time) - executed in the order of time
	TEST(CGDK10_executor, system_executor_post_at_order)
	{
		std::mutex lock;
		std::vector<int> order;

		auto tick_now = chrono::tick::now();
		auto post = [&](int _id, chrono::tick::duration _after)
		{
			system_executor::post(make_executable([&, _id] { std::lock_guard g(lock); order.push_back(_id); }), tick_now + _after);
		};

		post(3, 60ms);
		post(1, 20ms);
		post(2, 40ms);

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 3; }));
		EXPECT_EQ(order, (std::vector<int>{ 1, 2, 3 }));
	}

	// 4) executor::scheduler::cancel - the cancelled executable is not executed
	TEST(CGDK10_executor, scheduler_cancel)
	{
		std::atomic<int> count{ 0 };

		auto pscheduler = system_executor::scheduler();
		ASSERT_TRUE(pscheduler.exist());

		auto pexecutable = make_executable([&count] { ++count; });
		pscheduler->post_at(pexecutable, chrono::tick::now() + 100ms);

		EXPECT_TRUE(pscheduler->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler->cancel(pexecutable.get()));	// already cancelled

		api::sleep_for(200ms);
		EXPECT_EQ(count.load(), 0);
	}

	// 4-1) executor::scheduler - many executables due at once are all executed once while another thread post_at() / cancel()
	//      (the due executables are taken out under the lock and posted outside it)
	TEST(CGDK10_executor, scheduler_many_due_with_concurrent_cancel)
	{
		constexpr int COUNT_DUE = 2000;

		auto pscheduler = system_executor::scheduler();
		ASSERT_TRUE(pscheduler.exist());

		std::atomic<int> count_executed{ 0 };
		std::atomic<int> count_cancelled_executed{ 0 };
		std::atomic<int> count_cancel_failed{ 0 };
		std::atomic<bool> is_running{ true };

		// - another thread : post_at() a far executable and cancel() it, repeatedly
		std::thread thread_cancel([&]
			{
				while (is_running)
				{
					auto pexecutable = make_executable([&] { ++count_cancelled_executed; });
					pscheduler->post_at(pexecutable, chrono::tick::now() + 10s);
					if (pscheduler->cancel(pexecutable.get()) == false)
						++count_cancel_failed;
				}
			});

		// - half of them are due at the same time, the other half are spread over 50ms
		auto tick_base = chrono::tick::now() + 50ms;
		for (int i = 0; i < COUNT_DUE; ++i)
			pscheduler->post_at(make_executable([&] { ++count_executed; }), (i & 1) ? tick_base : tick_base + std::chrono::microseconds(i * 25));

		const bool is_all_executed = wait_until([&] { return count_executed.load() == COUNT_DUE; });

		is_running = false;
		thread_cancel.join();

		ASSERT_TRUE(is_all_executed);
		api::sleep_for(50ms);
		EXPECT_EQ(count_executed.load(), COUNT_DUE);
		EXPECT_EQ(count_cancelled_executed.load(), 0);
		EXPECT_EQ(count_cancel_failed.load(), 0);
	}

	// 5) executor::queue_list_threads - post returns true and executes on its own threads
	TEST(CGDK10_executor, queue_list_threads_post)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 2 });

		EXPECT_EQ(pexecutor->reserved_size(), 2u);

		std::mutex lock;
		std::set<std::thread::id> threads;
		std::atomic<int> count{ 0 };

		for (int i = 0; i < 200; ++i)
			EXPECT_TRUE(pexecutor->post(make_executable([&] { { std::lock_guard g(lock); threads.insert(std::this_thread::get_id()); } ++count; })));

		ASSERT_TRUE(wait_until([&] { return count == 200; }));

		std::lock_guard g(lock);
		EXPECT_LE(threads.size(), 2u);
		EXPECT_EQ(threads.count(std::this_thread::get_id()), 0u);
	}

	// 7) print_statistics - all statistics can be printed (with registered objects)
	TEST(CGDK10_executor, print_statistics)
	{
		// - a schedulable / a pended executable / a custom executor
		auto pschedulable = make_own<schedulable::executable_function<>>();
		pschedulable->start({ [] {}, 100ms });

		auto pexecutable = make_executable([] {});
		system_executor::scheduler()->post_at(pexecutable, chrono::tick::now() + 10s);

		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 1 });

		// - print
		executor::manager::print_statistics();
		executor::manager::print_statistics(true);
		factory::manager::print_statistics();
		factory::manager::print_statistics(true);
		system_executor::scheduler()->print_statistics();
		schedulable::manager::print_statistics();
		dump_manager::print_statistics();

		system_executor::scheduler()->cancel(pexecutable.get());
		SUCCEED();
	}

	// 6) executor::queue_list_threads - start with context ("thread_count")
	TEST(CGDK10_executor, queue_list_threads_start_with_context)
	{
		context setting;
		setting.from_json(R"({ "thread_count": 3 })");

		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start(setting);

		EXPECT_EQ(pexecutor->reserved_size(), 3u);
	}

	// 8) executor::queue_list_threads - an executable posted to an idle thread is executed at once
	//    (the waiting thread checks the queue and waits under one lock : a post between them is not lost.
	//     a lost signal waited until the timeout of the wait (1s))
	TEST(CGDK10_executor, queue_list_threads_wakeup)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 1 });

		chrono::tick::duration latency_max = chrono::tick::duration::zero();

		for (int i = 0; i < 50; ++i)
		{
			// - let the thread wait (idle)
			api::sleep_for(2ms);

			std::atomic<bool> is_executed{ false };
			auto tick_post = chrono::tick::now();
			ASSERT_TRUE(pexecutor->post(make_executable([&] { is_executed = true; })));
			ASSERT_TRUE(wait_until([&] { return is_executed.load(); }));

			latency_max = (std::max)(latency_max, chrono::tick::now() - tick_post);
		}

		EXPECT_LT(latency_max, chrono::tick::duration(500ms));
	}

	// 8-1) executor::queue_list_threads - a burst of posts runs on all idle threads at once
	//      (post wakes a thread only when the queue was empty : the other threads slept until the timeout of their wait (1s)
	//       and a burst of blocking executables (ex. odbc queries) ran one by one)
	TEST(CGDK10_executor, queue_list_threads_burst_parallel)
	{
		constexpr int COUNT_THREAD = 4;

		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ COUNT_THREAD });

		chrono::tick::duration latency_max = chrono::tick::duration::zero();

		for (int round = 0; round < 5; ++round)
		{
			// - let all threads wait (idle)
			api::sleep_for(50ms);

			std::atomic<int> count_started{ 0 };
			std::atomic<int> count_done{ 0 };

			// - each executable blocks until all of them are running (or 3s)
			auto tick_post = chrono::tick::now();
			for (int i = 0; i < COUNT_THREAD; ++i)
			{
				ASSERT_TRUE(pexecutor->post(make_executable([&]
					{
						++count_started;
						auto tick_end = chrono::tick::now() + 3s;
						while (count_started.load() < COUNT_THREAD && chrono::tick::now() < tick_end)
							api::sleep_for(1ms);
						++count_done;
					})));
			}

			ASSERT_TRUE(wait_until([&] { return count_started.load() == COUNT_THREAD; }));
			latency_max = (std::max)(latency_max, chrono::tick::now() - tick_post);

			ASSERT_TRUE(wait_until([&] { return count_done.load() == COUNT_THREAD; }));
		}

		EXPECT_LT(latency_max, chrono::tick::duration(500ms));
	}

	// 8-2) executor::queue_list_threads - stop() wakes the waiting threads at once, start() after stop() works again
	//      (stop() did not wake them : each thread saw the stop after the timeout of its wait (1s))
	TEST(CGDK10_executor, queue_list_threads_stop_wakes_waiting_threads)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 4 });

		// - let all threads wait (idle)
		api::sleep_for(50ms);

		auto tick_stop = chrono::tick::now();
		pexecutor->stop();
		EXPECT_LT(chrono::tick::now() - tick_stop, chrono::tick::duration(300ms));

		// - start again : the threads wait for a post again (not cancelled)
		pexecutor->start({ 2 });
		api::sleep_for(50ms);

		std::atomic<bool> is_executed{ false };
		auto tick_post = chrono::tick::now();
		ASSERT_TRUE(pexecutor->post(make_executable([&] { is_executed = true; })));
		ASSERT_TRUE(wait_until([&] { return is_executed.load(); }));
		EXPECT_LT(chrono::tick::now() - tick_post, chrono::tick::duration(300ms));

		tick_stop = chrono::tick::now();
		pexecutor->stop();
		EXPECT_LT(chrono::tick::now() - tick_stop, chrono::tick::duration(300ms));
	}

	// 9) executor::strand::post_at - executed at the time (not before), in the order of the time
	TEST(CGDK10_executor, strand_post_at_order)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		std::mutex lock;
		std::vector<int> order;
		auto tick_start = chrono::tick::now();
		std::atomic<int64_t> elapsed_first_ms{ -1 };

		auto push = [&](int _value) { std::lock_guard g(lock); if (order.empty()) elapsed_first_ms = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::tick::now() - tick_start).count(); order.push_back(_value); };

		EXPECT_TRUE(pstrand->post_at(make_executable([&] { push(60); }), tick_start + 60ms));
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { push(20); }), tick_start + 20ms));
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { push(40); }), tick_start + 40ms));

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 3; }));

		std::lock_guard g(lock);
		EXPECT_EQ(order, (std::vector<int>{ 20, 40, 60 }));
		EXPECT_GE(elapsed_first_ms.load(), 15);
	}

	// 11) executor::io_context - post moves the reference to the completion (no leak, no double release)
	TEST(CGDK10_executor, io_context_post_reference)
	{
		auto pexecutor = system_executor::executor();
		ASSERT_TRUE(pexecutor.exist());

		std::atomic<int> count{ 0 };
		auto pexecutable = make_executable([&count] { ++count; });
		const auto count_reference = pexecutable->reference_count();

		// - posted from several threads (the same executable)
		std::vector<std::thread> threads;
		for (int t = 0; t < 4; ++t)
			threads.emplace_back([&] { for (int i = 0; i < 500; ++i) EXPECT_TRUE(pexecutor->post(pexecutable)); });
		for (auto& iter : threads)
			iter.join();

		ASSERT_TRUE(wait_until([&] { return count == 2000; }));
		EXPECT_TRUE(wait_until([&] { return pexecutable->reference_count() == count_reference; }));

		// - system_executor::post (raw pointer of the system executor)
		for (int i = 0; i < 100; ++i)
			EXPECT_TRUE(system_executor::post(object_ptr<Iexecutable>(pexecutable)));
		ASSERT_TRUE(wait_until([&] { return count == 2100; }));
		EXPECT_TRUE(wait_until([&] { return pexecutable->reference_count() == count_reference; }));
	}

	// - messageable for the transmitter tests
	class test_messageable : public Imessageable
	{
	public:
			std::atomic<int>	count{ 0 };
			eRESULT				result = eRESULT::BYPASS;
	protected:
		virtual	result_code		process_message(sMESSAGE& /*_msg*/) override { ++this->count; return result_code(this->result); }
	};
	class test_transmitter : public Nmessage_transmitter
	{
	};

	// 12) Nmessage_transmitter - transmits in order until processed, detach / reset release the messageables
	TEST(CGDK10_executor, message_transmitter_attach_detach)
	{
		auto ptransmitter = make_object<test_transmitter>();
		auto pmessageable_1 = make_object<test_messageable>();
		auto pmessageable_2 = make_object<test_messageable>();
		const auto count_reference = pmessageable_1->reference_count();

		EXPECT_TRUE(ptransmitter->register_messageable(pmessageable_1));
		EXPECT_TRUE(ptransmitter->register_messageable(pmessageable_2));
		EXPECT_FALSE(ptransmitter->register_messageable(pmessageable_1));		// - already attached

		// - not processed (BYPASS) : all
		sMESSAGE msg;
		ptransmitter->transmit_message(msg);
		EXPECT_EQ(pmessageable_1->count, 1);
		EXPECT_EQ(pmessageable_2->count, 1);

		// - processed by the first : the second is not called
		pmessageable_1->result = eRESULT::DONE;
		ptransmitter->transmit_message(msg);
		EXPECT_EQ(pmessageable_1->count, 2);
		EXPECT_EQ(pmessageable_2->count, 1);

		// - detach
		EXPECT_TRUE(ptransmitter->unregister_messageable(pmessageable_1.get()));
		EXPECT_FALSE(ptransmitter->unregister_messageable(pmessageable_1.get()));
		EXPECT_EQ(pmessageable_1->reference_count(), count_reference);
		ptransmitter->transmit_message(msg);
		EXPECT_EQ(pmessageable_1->count, 2);
		EXPECT_EQ(pmessageable_2->count, 2);

		// - reset
		EXPECT_EQ(ptransmitter->reset_message_transmitter(), 1);
		EXPECT_EQ(pmessageable_2->reference_count(), count_reference);
		ptransmitter->transmit_message(msg);
		EXPECT_EQ(pmessageable_2->count, 2);
	}

	// 13) Nmessage_transmitter - attach / detach while other threads transmit
	//     (a detached messageable is released only after the transmits using it are finished : no use after free)
	TEST(CGDK10_executor, message_transmitter_detach_while_transmitting)
	{
		auto ptransmitter = make_object<test_transmitter>();
		auto pmessageable_fixed = make_object<test_messageable>();
		ptransmitter->register_messageable(pmessageable_fixed);

		std::atomic<bool> is_running{ true };
		std::vector<std::thread> threads;
		for (int t = 0; t < 4; ++t)
			threads.emplace_back([&] { sMESSAGE msg; while (is_running) ptransmitter->transmit_message(msg); });

		for (int i = 0; i < 2000; ++i)
		{
			auto pmessageable = make_object<test_messageable>();
			ptransmitter->register_messageable(pmessageable);
			ptransmitter->unregister_messageable(pmessageable.get());
		}

		// - the loop above can end before the threads transmit once (2ms on a loaded machine)
		EXPECT_TRUE(wait_until([&] { return pmessageable_fixed->count.load() > 0; }));

		is_running = false;
		for (auto& iter : threads)
			iter.join();

		EXPECT_GT(pmessageable_fixed->count, 0);
		EXPECT_EQ(ptransmitter->reset_message_transmitter(), 1);
	}

	// 10) executor::strand::post_at - a past time is executed at once
	TEST(CGDK10_executor, strand_post_at_past)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		std::atomic<bool> is_executed{ false };
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { is_executed = true; }), chrono::tick::now() - 1s));
		EXPECT_TRUE(wait_until([&] { return is_executed.load(); }, 1s));
	}
}
