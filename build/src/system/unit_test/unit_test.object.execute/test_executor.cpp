#include "pch.h"
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#if defined(__linux__)
#include <sched.h>
#endif
#include <string>
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

	// 4-2) executor::scheduler - an earlier post_at wakes the sleeping thread, a later one does not delay it
	TEST(CGDK10_executor, scheduler_earlier_post_wakes)
	{
		auto pscheduler = make_own<executor::scheduler>();
		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

		std::atomic<int64_t> elapsed_us_a{ -1 };
		std::atomic<int64_t> elapsed_us_b{ -1 };
		auto tick_start = chrono::tick::now();

		// - the thread sleeps until a far one, then an earlier one (a) and a later one than (a) (b) are posted
		auto pfar = make_executable([] {});
		pscheduler->post_at(pfar, tick_start + 10s);
		api::sleep_for(20ms);
		pscheduler->post_at(make_executable([&] { elapsed_us_a = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_start).count(); }), tick_start + 50ms);
		pscheduler->post_at(make_executable([&] { elapsed_us_b = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_start).count(); }), tick_start + 80ms);

		ASSERT_TRUE(wait_until([&] { return elapsed_us_a >= 0 && elapsed_us_b >= 0; }));

		// - not before the time / not too late (generous for loaded machines)
		EXPECT_GE(elapsed_us_a.load(), 50000);
		EXPECT_LT(elapsed_us_a.load(), 50000 + 30000);
		EXPECT_GE(elapsed_us_b.load(), 80000);
		EXPECT_LT(elapsed_us_b.load(), 80000 + 30000);

		EXPECT_TRUE(pscheduler->cancel(pfar.get()));
		pscheduler->stop();
	}

	// 4-3) executor::scheduler::cancel - cancel at any position of the heap keeps the order of the rest
	TEST(CGDK10_executor, scheduler_cancel_keeps_order)
	{
		constexpr int COUNT = 400;

		// - one thread executor (a multi thread executor may run the adjacent ones in any order)
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 1 });

		auto pscheduler = make_own<executor::scheduler>();
		pscheduler->executor(pexecutor);
		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

		std::mutex lock;
		std::vector<int> order;
		std::vector<object_ptr<Iexecutable>> vector_executable;

		// - post in a shuffled time order (100ms ~ 100ms + COUNT x 100us)
		auto tick_base = chrono::tick::now() + 100ms;
		for (int i = 0; i < COUNT; ++i)
		{
			const int id = (i * 7919) % COUNT;
			auto pexecutable = make_executable([&, id] { std::lock_guard g(lock); order.push_back(id); });
			vector_executable.push_back(pexecutable);
			pscheduler->post_at(pexecutable, tick_base + std::chrono::microseconds(id * 100));
		}
		EXPECT_EQ(pscheduler->pended_count(), static_cast<size_t>(COUNT));

		// - cancel the ids that are multiple of 3
		int count_cancelled = 0;
		for (int i = 0; i < COUNT; ++i)
		{
			const int id = (i * 7919) % COUNT;
			if (id % 3 == 0)
			{
				EXPECT_TRUE(pscheduler->cancel(vector_executable[i].get()));
				EXPECT_FALSE(pscheduler->cancel(vector_executable[i].get()));
				++count_cancelled;
			}
		}
		EXPECT_EQ(pscheduler->pended_count(), static_cast<size_t>(COUNT - count_cancelled));

		// - the rest are executed in the order of the time
		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == static_cast<size_t>(COUNT - count_cancelled); }));
		api::sleep_for(20ms);

		std::vector<int> expected;
		for (int id = 0; id < COUNT; ++id)
			if (id % 3 != 0)
				expected.push_back(id);

		std::lock_guard g(lock);
		EXPECT_EQ(order, expected);

		// - executed ones can not be cancelled
		for (auto& iter : vector_executable)
			EXPECT_FALSE(pscheduler->cancel(iter.get()));

		pscheduler->stop();
		pexecutor->stop();
	}

	// 4-4) executor::scheduler::cancel - an executable posted twice is cancelled one by one / re-posted after execution
	TEST(CGDK10_executor, scheduler_cancel_posted_twice)
	{
		auto pscheduler = make_own<executor::scheduler>();
		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

		std::atomic<int> count{ 0 };
		auto pexecutable = make_executable([&count] { ++count; });

		// - posted twice (with another one between) : cancel twice
		auto pother = make_executable([] {});
		pscheduler->post_at(pexecutable, chrono::tick::now() + 10s);
		pscheduler->post_at(pother, chrono::tick::now() + 5s);
		pscheduler->post_at(pexecutable, chrono::tick::now() + 1s);
		EXPECT_TRUE(pscheduler->cancel(pexecutable.get()));
		EXPECT_TRUE(pscheduler->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler->cancel(pexecutable.get()));
		EXPECT_EQ(pscheduler->pended_count(), 1u);

		// - posted twice : one is executed, the other is cancelled
		pscheduler->post_at(pexecutable, chrono::tick::now() + 20ms);
		pscheduler->post_at(pexecutable, chrono::tick::now() + 10s);
		ASSERT_TRUE(wait_until([&] { return count == 1; }));
		EXPECT_TRUE(pscheduler->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler->cancel(pexecutable.get()));

		// - re-posted after execution : cancelled
		pscheduler->post_at(pexecutable, chrono::tick::now() + 10s);
		EXPECT_TRUE(pscheduler->cancel(pexecutable.get()));

		// - cancel all : cancel() of each fails
		pscheduler->post_at(pexecutable, chrono::tick::now() + 10s);
		pscheduler->cancel();
		EXPECT_FALSE(pscheduler->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler->cancel(pother.get()));

		api::sleep_for(50ms);
		EXPECT_EQ(count.load(), 1);

		pscheduler->stop();
	}

	// 4-5) executor::scheduler::post_at - refused (false) when not running : not started / stopped (it was dropped and true returned)
	TEST(CGDK10_executor, scheduler_post_at_refused_when_not_running)
	{
		auto pscheduler = make_own<executor::scheduler>();
		std::atomic<int> count{ 0 };

		EXPECT_FALSE(pscheduler->post_at(make_executable([&count] { ++count; }), chrono::tick::now() + 10ms));

		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);
		EXPECT_TRUE(pscheduler->post_at(make_executable([&count] { ++count; }), chrono::tick::now() + 10ms));
		ASSERT_TRUE(wait_until([&] { return count == 1; }));

		pscheduler->stop();
		EXPECT_FALSE(pscheduler->post_at(make_executable([&count] { ++count; }), chrono::tick::now() + 10ms));
		EXPECT_EQ(pscheduler->pended_count(), 0u);
	}

	// 4-6) executor::scheduler::cancel - the cancelled executables are released outside the queue lock : a destructor may post_at() /
	//      cancel() on the same scheduler (it deadlocked on the lock held by cancel)
	TEST(CGDK10_executor, scheduler_cancel_releases_outside_lock)
	{
		struct REENTER
		{
			executor::scheduler* pscheduler;
			~REENTER()
			{
				auto pexecutable = make_executable([] {});
				auto praw = pexecutable.get();
				pscheduler->post_at(std::move(pexecutable), chrono::tick::now() + 10s);
				pscheduler->cancel(praw);
			}
		};

		auto pscheduler = make_own<executor::scheduler>();
		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

		std::atomic<bool> is_done{ false };
		std::thread thread_test([&]
			{
				// - cancel(executable) : the last reference
				{
					auto pexecutable = make_executable([preenter = std::make_shared<REENTER>(REENTER{ pscheduler.get() })] {});
					auto praw = pexecutable.get();
					pscheduler->post_at(std::move(pexecutable), chrono::tick::now() + 10s);
					pscheduler->cancel(praw);
				}

				// - cancel() : all
				pscheduler->post_at(make_executable([preenter = std::make_shared<REENTER>(REENTER{ pscheduler.get() })] {}), chrono::tick::now() + 10s);
				pscheduler->cancel();

				is_done = true;
			});

		const bool is_finished = wait_until([&] { return is_done.load(); });

		if (is_finished)
			thread_test.join();
		else
			thread_test.detach();

		ASSERT_TRUE(is_finished);
		EXPECT_EQ(pscheduler->pended_count(), 0u);

		pscheduler->stop();
	}

	// 4-7) executor::scheduler::cancel - an executable pending in two schedulers is cancelled in both (the first one did not find it :
	//      its position was overwritten by the second)
	TEST(CGDK10_executor, scheduler_cancel_in_two_schedulers)
	{
		auto pscheduler_a = make_own<executor::scheduler>();
		auto pscheduler_b = make_own<executor::scheduler>();
		ASSERT_EQ(pscheduler_a->start(), eRESULT::SUCCESS);
		ASSERT_EQ(pscheduler_b->start(), eRESULT::SUCCESS);

		std::atomic<int> count{ 0 };
		auto pexecutable = make_executable([&count] { ++count; });

		// - a then b, cancelled in a then b
		pscheduler_a->post_at(pexecutable, chrono::tick::now() + 10s);
		pscheduler_b->post_at(pexecutable, chrono::tick::now() + 10s);
		EXPECT_TRUE(pscheduler_a->cancel(pexecutable.get()));
		EXPECT_TRUE(pscheduler_b->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler_a->cancel(pexecutable.get()));
		EXPECT_FALSE(pscheduler_b->cancel(pexecutable.get()));

		// - a then b, executed in b, cancelled in a
		pscheduler_a->post_at(pexecutable, chrono::tick::now() + 10s);
		pscheduler_b->post_at(pexecutable, chrono::tick::now() + 10ms);
		ASSERT_TRUE(wait_until([&] { return count == 1; }));
		EXPECT_TRUE(pscheduler_a->cancel(pexecutable.get()));
		EXPECT_EQ(pscheduler_a->pended_count(), 0u);
		EXPECT_EQ(pscheduler_b->pended_count(), 0u);

		pscheduler_a->stop();
		pscheduler_b->stop();
	}

	// 4-8) executor::scheduler::stop - joins the thread every time (Release : the 1000 loops without a sleep ended before the thread did,
	//      FAIL without the join), also while it sleeps for a far one
	TEST(CGDK10_executor, scheduler_stop_joins)
	{
		for (int i = 0; i < 200; ++i)
		{
			auto pscheduler = make_own<executor::scheduler>();
			ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

			if (i & 1)
				pscheduler->post_at(make_executable([] {}), chrono::tick::now() + 10s);

			ASSERT_EQ(pscheduler->stop(), eRESULT::SUCCESS);
			ASSERT_EQ(pscheduler->get_existing_thread_count(), 0u);
		}
	}

	// 4-9) executor::scheduler::spin_wait - the lateness of the timer is spun away (windows high resolution timer : 0.2 ~ 0.6ms late)
	TEST(CGDK10_executor, scheduler_spin_wait)
	{
		// - one thread executor : the lateness of the scheduler, not of a busy executor
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 1 });

		auto pscheduler = make_own<executor::scheduler>();
		pscheduler->executor(pexecutor);
		ASSERT_EQ(pscheduler->start(), eRESULT::SUCCESS);

		const auto measure = [&](chrono::tick::duration _spin) -> int64_t
		{
			pscheduler->spin_wait(_spin);

			std::vector<int64_t> vector_late;
			for (int i = 0; i < 20; ++i)
			{
				std::atomic<int64_t> late_us{ -1 };
				const auto tick_at = chrono::tick::now() + 10ms;
				pscheduler->post_at(make_executable([&late_us, tick_at] { late_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_at).count(); }), tick_at);
				EXPECT_TRUE(wait_until([&] { return late_us.load() != -1; }));
				EXPECT_GE(late_us.load(), 0);
				vector_late.push_back(late_us.load());
			}

			std::sort(vector_late.begin(), vector_late.end());
			return vector_late[vector_late.size() / 2];
		};

		const auto late_default = measure(chrono::tick::duration::zero());
		const auto late_spin = measure(std::chrono::milliseconds(2));

		printf("    scheduler lateness (median) : default %lldus, spin_wait(2ms) %lldus\n", static_cast<long long>(late_default), static_cast<long long>(late_spin));
		EXPECT_EQ(pscheduler->spin_wait(), std::chrono::duration_cast<chrono::tick::duration>(std::chrono::milliseconds(2)));
		EXPECT_LT(late_spin, 300);

		pscheduler->stop();
		pexecutor->stop();
	}

	// 4-10) executable::batch_wait - waits until every executable ran (it returned when the last one was taken out, before it ran,
	//       and could miss the wakeup), (max)() waits without a timeout
	TEST(CGDK10_executor, batch_wait_waits_for_all)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 4 });

		for (int round = 0; round < 2; ++round)
		{
			auto pbatch = make_object<executable::batch_wait>();
			std::atomic<int> count{ 0 };

			for (int i = 0; i < 40; ++i)
				pbatch->queue_executable(make_executable([&count] { std::this_thread::sleep_for(std::chrono::milliseconds(5)); ++count; }));

			EXPECT_TRUE(pbatch->request_execute(pexecutor.get()));

			if (round == 0)
				EXPECT_TRUE(pbatch->wait_execute_completion(10s));
			else
				EXPECT_TRUE(pbatch->wait_execute_completion());

			EXPECT_EQ(count.load(), 40);
			EXPECT_EQ(pbatch->executing_count(), 0u);
		}

		pexecutor->stop();
	}

	// 4-11) runnext - an executable a worker posts before it blocks (scoped_thread_block) is run by another worker meanwhile
	//       (it stayed in the slot of the blocked thread : a wait for it never ended)
	TEST(CGDK10_executor, runnext_post_then_block)
	{
		auto pexecutor = executor::make_io_context_threads("test runnext block", 2);
		ASSERT_TRUE(pexecutor.exist());

		std::atomic<bool> is_ran{ false };
		std::atomic<int> result{ -1 };

		pexecutor->post(make_executable([&]
			{
				pexecutor->post(make_executable([&] { is_ran = true; }));

				scoped_thread_block thread_block;
				result = wait_until([&] { return is_ran.load(); }, 3s) ? 1 : 0;
			}));

		ASSERT_TRUE(wait_until([&] { return result.load() != -1; }, 10s));
		EXPECT_EQ(result.load(), 1);

		pexecutor->stop();
	}

	// 4-12) runnext - an executor of one worker runs the posts of its handler in order (the slot let the last one run first)
	TEST(CGDK10_executor, runnext_one_worker_keeps_order)
	{
		auto pexecutor = executor::make_io_context_threads("test runnext order", 1);
		ASSERT_TRUE(pexecutor.exist());

		std::mutex lock;
		std::vector<int> order;

		pexecutor->post(make_executable([&]
			{
				for (int i = 0; i < 8; ++i)
					pexecutor->post(make_executable([&, i] { std::lock_guard g(lock); order.push_back(i); }));
			}));

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 8; }));

		std::lock_guard g(lock);
		for (int i = 0; i < 8; ++i)
			EXPECT_EQ(order[i], i);

		pexecutor->stop();
	}

	// 4-13) runnext - a chain of executables posting the next one runs to the end on an executor of several workers
	TEST(CGDK10_executor, runnext_chain)
	{
		auto pexecutor = executor::make_io_context_threads("test runnext chain", 4);
		ASSERT_TRUE(pexecutor.exist());

		struct CHAIN : public Iexecutable
		{
			Iexecutor* pexecutor = nullptr;
			std::atomic<int> count{ 0 };
			virtual intptr_t process_execute(intptr_t, size_t) override { if (++count < 100000) pexecutor->post(object_ptr<Iexecutable>(this)); return 0; }
		};

		auto pchain = make_object<CHAIN>();
		pchain->pexecutor = pexecutor.get();
		pexecutor->post(object_ptr<Iexecutable>(pchain.get()));

		ASSERT_TRUE(wait_until([&] { return pchain->count.load() == 100000; }, 20s));

		pexecutor->stop();
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

	// 8-3) executor::queue_list_threads - refuses the posts after stop() (they were queued and never executed : a batch_wait on a stopped
	//      executor waited forever), accepts them again after start()
	TEST(CGDK10_executor, queue_list_threads_refuses_after_stop)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 2 });
		pexecutor->stop();

		std::atomic<int> count{ 0 };
		EXPECT_FALSE(pexecutor->post(make_executable([&] { ++count; })));

		// - batch_wait : the refused posts are counted as done
		auto pbatch = make_object<executable::batch_wait>();
		for (int i = 0; i < 4; ++i)
			pbatch->queue_executable(make_executable([&count] { ++count; }));
		pbatch->request_execute(pexecutor.get());
		EXPECT_TRUE(pbatch->wait_execute_completion(2s));
		EXPECT_EQ(count.load(), 0);

		// - start again
		pexecutor->start({ 2 });
		EXPECT_TRUE(pexecutor->post(make_executable([&] { ++count; })));
		EXPECT_TRUE(wait_until([&] { return count.load() == 1; }));
		pexecutor->stop();
	}

	// 8-5) extra threads - every worker blocks at once (scoped_thread_block) : one extra thread for each blocked thread above the threshold
	//      (the core count). an extra thread was counted active only when it ran : the blocks before that woke a thread that was not
	//      waiting and began none
	template <class TEXECUTOR, class FCOUNT>
	static void test_extra_threads(TEXECUTOR& _pexecutor, int _thread_count, bool _is_chain, FCOUNT&& _existing_count)
	{
		const int threshold = static_cast<int>(api::get_core_count());
		const int extra_expected = _thread_count - threshold;

		// - shared with the executables (they may still run when this returns)
		struct STATE
		{
			std::atomic<int> count_blocked{ 0 };
			std::atomic<int> count_extra_run{ 0 };
			std::atomic<bool> is_release{ false };
		};
		auto pstate = std::make_shared<STATE>();

		// - released on every return (an ASSERT failure left the workers blocked : stop() never returned)
		struct RELEASE { std::shared_ptr<STATE> p; ~RELEASE() { p->is_release = true; } } release{ pstate };

		// - all at once, or a chain : each one posts the next one before it blocks
		auto ppost = std::make_shared<std::function<void(int)>>();
		auto* pexecutor = _pexecutor.get();
		*ppost = [pstate, pexecutor, _thread_count, _is_chain, wppost = std::weak_ptr<std::function<void(int)>>(ppost)](int _index)
		{
			pexecutor->post(make_executable([pstate, _thread_count, _is_chain, wppost, _index]
				{
					if (_is_chain && _index + 1 < _thread_count)
						if (auto p = wppost.lock())
							(*p)(_index + 1);

					scoped_thread_block thread_block;
					++pstate->count_blocked;
					while (!pstate->is_release)
						std::this_thread::sleep_for(1ms);
				}));
		};
		if (_is_chain)
			(*ppost)(0);
		else
			for (int i = 0; i < _thread_count; ++i)
				(*ppost)(i);

		ASSERT_TRUE(wait_until([&] { return pstate->count_blocked.load() == _thread_count; }, 10s)) << "blocked : " << pstate->count_blocked.load() << " / " << _thread_count;

		// - the extra threads run the posts meanwhile
		for (int i = 0; i < extra_expected; ++i)
			_pexecutor->post(make_executable([pstate] { ++pstate->count_extra_run; }));
		EXPECT_TRUE(wait_until([&] { return pstate->count_extra_run.load() == extra_expected; }, 5s));

		EXPECT_TRUE(wait_until([&] { return static_cast<int>(_existing_count()) >= _thread_count + extra_expected; }, 3s)) << "threads : " << _existing_count() << " (expected at least " << _thread_count + extra_expected << ")";
	}
	TEST(CGDK10_executor, queue_list_threads_extra_threads)
	{
		const int thread_count = static_cast<int>(api::get_core_count()) + 4;

		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ thread_count });

		test_extra_threads(pexecutor, thread_count, false, [&] { return pexecutor->get_existing_thread_count(); });

		pexecutor->stop();
	}
	TEST(CGDK10_executor, io_context_threads_extra_threads)
	{
		const int thread_count = static_cast<int>(api::get_core_count()) + 4;

		auto pexecutor = executor::make_io_context_threads("test extra threads", thread_count);
		ASSERT_TRUE(pexecutor.exist());

		test_extra_threads(pexecutor, thread_count, true, [&] { return pexecutor->existing_thread_count(); });

		pexecutor->stop();
	}

	// 8-6) io_context - the blocking executables posted all at once : a worker takes several of them in one batch (epoll / io_uring / iocp),
	//      the first one blocks and hands the rest of its batch to the other workers (they waited behind it : 22 of 36 blocked on epoll)
	TEST(CGDK10_executor, io_context_threads_extra_threads_batch)
	{
		const int thread_count = static_cast<int>(api::get_core_count()) + 4;

		auto pexecutor = executor::make_io_context_threads("test extra threads batch", thread_count);
		ASSERT_TRUE(pexecutor.exist());

		test_extra_threads(pexecutor, thread_count, false, [&] { return pexecutor->existing_thread_count(); });

		pexecutor->stop();
	}

	// 8-4) executor::queue_list::stop - an executable throwing in execute_all() does not end the process (noexcept : std::terminate),
	//      the others are executed
	TEST(CGDK10_executor, queue_list_execute_all_catches)
	{
		auto pexecutor = make_own<executor::queue_list>();
		pexecutor->start();

		std::atomic<int> count{ 0 };
		EXPECT_TRUE(pexecutor->post(make_executable([&] { ++count; })));
		EXPECT_TRUE(pexecutor->post(make_executable([]() -> int { throw std::runtime_error("test"); })));
		EXPECT_TRUE(pexecutor->post(make_executable([&] { ++count; })));

		pexecutor->stop();
		EXPECT_EQ(count.load(), 2);
		EXPECT_FALSE(pexecutor->post(make_executable([&] { ++count; })));
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

	// 15) executor::strand::destroy - in an executable of the strand itself : returns at once (it waited for itself forever),
	//     the rest of the executables are executed in order after the executable
	TEST(CGDK10_executor, strand_destroy_inside_executable)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		std::atomic<int> state{ 0 };
		std::mutex lock;
		std::string order;
		auto push = [&](char _value) { std::lock_guard g(lock); order += _value; };

		// - the first waits for the posts of '2' and '3' (a post after the destroy is refused : '2' and '3' were lost when the first ran before them)
		std::atomic<bool> is_posted{ false };
		EXPECT_TRUE(pstrand->post(make_executable([&]
		{
			while (!is_posted)
				std::this_thread::yield();
			pstrand->post(make_executable([&] { push('4'); }));
			state = 1;
			pstrand->destroy();
			push('1');
			state = 2;
		})));
		EXPECT_TRUE(pstrand->post(make_executable([&] { push('2'); })));
		EXPECT_TRUE(pstrand->post(make_executable([&] { push('3'); })));
		is_posted = true;

		ASSERT_TRUE(wait_until([&] { return state == 2; }));
		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 4; }));

		std::lock_guard g(lock);
		EXPECT_EQ(order, "1234");
		EXPECT_FALSE(pstrand->post(make_executable([] {})));
	}

	// 16) executor::strand::destroy - while another thread executes the strand : the destroying thread does not hold the lock while it waits
	//     (it slept 1ms holding the lock : post() blocked and the executing thread could not finish for up to seconds)
	TEST(CGDK10_executor, strand_destroy_while_processing)
	{
		auto pstrand = make_own<executor::strand>();
		pstrand->start();

		std::atomic<bool> is_started{ false };
		std::atomic<bool> is_ended{ false };
		std::atomic<int64_t> post_in_executable_us{ -1 };
		pstrand->post(make_executable([&]
		{
			is_started = true;
			std::this_thread::sleep_for(50ms);
			auto tick_post = chrono::tick::now();
			pstrand->post(make_executable([] {}));
			post_in_executable_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_post).count();
			std::this_thread::sleep_for(50ms);
			is_ended = true;
		}));
		ASSERT_TRUE(wait_until([&] { return is_started.load(); }));

		std::atomic<bool> is_destroyed{ false };
		std::atomic<int64_t> post_max_us{ 0 };
		std::thread thread_post([&]
		{
			while (!is_destroyed)
			{
				auto tick_post = chrono::tick::now();
				pstrand->post(make_executable([] {}));
				auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(chrono::tick::now() - tick_post).count();
				if (elapsed_us > post_max_us)
					post_max_us = elapsed_us;
			}
		});

		auto tick_destroy = chrono::tick::now();
		pstrand->destroy();
		auto elapsed_destroy = chrono::tick::now() - tick_destroy;
		is_destroyed = true;
		thread_post.join();

		EXPECT_TRUE(is_ended.load());
		EXPECT_LT(elapsed_destroy, chrono::tick::duration(1s));
		EXPECT_LT(post_in_executable_us.load(), 100'000);
		EXPECT_LT(post_max_us.load(), 100'000);
	}

	// - runs _function on a detached thread and waits for it up to _timeout (false : still running. the thread is left : it may never end)
	template <class TFUNCTION>
	static bool run_with_timeout(TFUNCTION&& _function, chrono::tick::duration _timeout)
	{
		auto pis_done = std::make_shared<std::atomic<bool>>(false);
		std::thread([pis_done, function = std::forward<TFUNCTION>(_function)]() mutable { function(); *pis_done = true; }).detach();
		return wait_until([&] { return pis_done->load(); }, _timeout);
	}

	// 16-1) executor::strand - the executor of the strand refuses the post (stopped) : post() returns false and destroy() returns
	//       (the refused post was ignored : m_is_processing stayed true, every later post only queued and destroy() waited forever)
	TEST(CGDK10_executor, strand_post_refused_by_executor)
	{
		auto pexecutor = executor::make_io_context_threads("test strand refused", 1);
		ASSERT_TRUE(pexecutor.exist());
		pexecutor->stop();

		auto pstrand = make_object<executor::strand>();
		pstrand->executor(object_ptr<Iexecutor>(pexecutor.get()));
		pstrand->start();

		std::atomic<int> count{ 0 };
		EXPECT_FALSE(pstrand->post(make_executable([&] { ++count; })));
		EXPECT_FALSE(pstrand->post(make_executable([&] { ++count; })));

		// - a working executor again : the posts are executed
		auto pexecutor_2 = executor::make_io_context_threads("test strand refused 2", 1);
		pstrand->executor(object_ptr<Iexecutor>(pexecutor_2.get()));
		EXPECT_TRUE(pstrand->post(make_executable([&] { ++count; })));
		EXPECT_TRUE(wait_until([&] { return count.load() == 1; }, 3s));

		// - destroy
		pstrand->executor(object_ptr<Iexecutor>(pexecutor.get()));
		ASSERT_TRUE(run_with_timeout([pstrand] { pstrand->destroy(); }, 5s));
		EXPECT_EQ(count.load(), 1);

		pexecutor_2->stop();
	}

	// 16-2) executor::strand::destroy - by a handler of the executor of the strand (not of the strand) right after a post : the strand is
	//       only posted (in the runnext slot of this thread, or queued behind this handler on an executor of one thread). destroy()
	//       executes the queue itself (it waited for an execution that could not start while it waited : forever)
	static void test_strand_destroy_posted(int _thread_count)
	{
		auto pexecutor = executor::make_io_context_threads("test strand destroy posted", _thread_count);
		ASSERT_TRUE(pexecutor.exist());

		auto pstrand = make_object<executor::strand>();
		pstrand->executor(object_ptr<Iexecutor>(pexecutor.get()));
		pstrand->start();

		auto pcount = std::make_shared<std::atomic<int>>(0);
		auto pis_done = std::make_shared<std::atomic<bool>>(false);

		pexecutor->post(make_executable([pstrand, pcount, pis_done]
			{
				for (int i = 0; i < 3; ++i)
					pstrand->post(make_executable([pcount] { ++*pcount; }));
				pstrand->destroy();
				*pis_done = true;
			}));

		ASSERT_TRUE(wait_until([&] { return pis_done->load(); }, 5s)) << "destroy() never returned (thread count " << _thread_count << ")";
		EXPECT_EQ(pcount->load(), 3);
		EXPECT_FALSE(pstrand->post(make_executable([] {})));

		pexecutor->stop();
	}
	TEST(CGDK10_executor, strand_destroy_posted_one_thread)
	{
		test_strand_destroy_posted(1);
	}
	TEST(CGDK10_executor, strand_destroy_posted_runnext)
	{
		test_strand_destroy_posted(4);
	}

	// 16-3) executor::strand::destroy - many threads post and destroy at once : every accepted executable runs once, none after destroy()
	//       returned, never two at a time
	TEST(CGDK10_executor, strand_destroy_while_posting)
	{
		for (int round = 0; round < 50; ++round)
		{
			auto pstrand = make_object<executor::strand>();
			pstrand->start();

			std::atomic<int> accepted{ 0 };
			std::atomic<int> executed{ 0 };
			std::atomic<int> concurrent{ 0 };
			std::atomic<int> concurrent_max{ 0 };
			std::atomic<bool> is_destroyed{ false };
			std::atomic<int> executed_after{ 0 };

			std::vector<std::thread> threads;
			for (int t = 0; t < 4; ++t)
				threads.emplace_back([&]
				{
					for (int i = 0; i < 2000; ++i)
					{
						if (pstrand->post(make_executable([&]
							{
								auto now = ++concurrent;
								if (now > concurrent_max) concurrent_max = now;
								if (is_destroyed) ++executed_after;
								++executed;
								--concurrent;
							})))
							++accepted;
					}
				});

			std::this_thread::sleep_for(std::chrono::microseconds(200 * (round % 5)));
			pstrand->destroy();
			is_destroyed = true;

			for (auto& iter : threads)
				iter.join();

			EXPECT_EQ(executed.load(), accepted.load());
			EXPECT_EQ(executed_after.load(), 0);
			EXPECT_LE(concurrent_max.load(), 1);
		}
	}

	// 16-4) executor::strand::post_at - executed on the executor of the strand (it was executed on the thread the scheduler posted the
	//       wake to : the system executor)
	TEST(CGDK10_executor, strand_post_at_runs_on_strand_executor)
	{
		auto pexecutor = executor::make_io_context_threads("test strand post_at executor", 1);
		ASSERT_TRUE(pexecutor.exist());

		// - the thread of the executor
		std::atomic<std::thread::id> id_executor{};
		pexecutor->post(make_executable([&] { id_executor = std::this_thread::get_id(); }));
		ASSERT_TRUE(wait_until([&] { return id_executor.load() != std::thread::id(); }));

		auto pstrand = make_object<executor::strand>();
		pstrand->executor(object_ptr<Iexecutor>(pexecutor.get()));
		pstrand->start();

		std::atomic<std::thread::id> id_post_at{};
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { id_post_at = std::this_thread::get_id(); }), chrono::tick::now() + 20ms));
		ASSERT_TRUE(wait_until([&] { return id_post_at.load() != std::thread::id(); }, 3s));
		EXPECT_EQ(id_post_at.load(), id_executor.load());

		pstrand->destroy();
		pexecutor->stop();
	}

	// 16-5) executor::strand::post_at - one wake in the scheduler for the earliest time (one per post_at() before), the same time in the
	//       order of the posts
	TEST(CGDK10_executor, strand_post_at_one_wake)
	{
		auto pstrand = make_object<executor::strand>();
		pstrand->start();

		auto pscheduler = system_executor::scheduler();
		ASSERT_TRUE(pscheduler.exist());

		std::mutex lock;
		std::vector<int> order;

		const auto count_pended = pscheduler->pended_count();
		const auto tick_base = chrono::tick::now() + 50ms;

		for (int i = 0; i < 300; ++i)
			EXPECT_TRUE(pstrand->post_at(make_executable([&, i] { std::lock_guard g(lock); order.push_back(i); }), tick_base + (i % 3) * 10ms));

		const auto count_pended_posted = pscheduler->pended_count();
		EXPECT_LE(count_pended_posted, count_pended + 3);

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 300; }, 5s));

		std::vector<int> expected;
		for (int r = 0; r < 3; ++r)
			for (int i = r; i < 300; i += 3)
				expected.push_back(i);

		{
			std::lock_guard g(lock);
			EXPECT_EQ(order, expected);
		}

		pstrand->destroy();
	}

	// 16-6) executor::strand::post_at - an earlier post_at after a later one : both at their times, the earlier first
	TEST(CGDK10_executor, strand_post_at_earlier_after_later)
	{
		auto pstrand = make_object<executor::strand>();
		pstrand->start();

		std::mutex lock;
		std::vector<int> order;
		std::atomic<int64_t> elapsed_ms[2] = { -1, -1 };
		auto tick_start = chrono::tick::now();
		auto push = [&](int _value) { std::lock_guard g(lock); elapsed_ms[_value] = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::tick::now() - tick_start).count(); order.push_back(_value); };

		EXPECT_TRUE(pstrand->post_at(make_executable([&] { push(1); }), tick_start + 80ms));
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { push(0); }), tick_start + 20ms));

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 2; }, 3s));

		std::lock_guard g(lock);
		EXPECT_EQ(order, (std::vector<int>{ 0, 1 }));
		EXPECT_GE(elapsed_ms[0].load(), 15);
		EXPECT_LT(elapsed_ms[0].load(), 70);
		EXPECT_GE(elapsed_ms[1].load(), 75);
	}

	// 16-7) executor::strand::post_at - destroyed before the time : not executed, the strand is released
	TEST(CGDK10_executor, strand_post_at_destroyed_before_time)
	{
		auto pstrand = make_object<executor::strand>();
		pstrand->start();

		std::atomic<bool> is_executed{ false };
		EXPECT_TRUE(pstrand->post_at(make_executable([&] { is_executed = true; }), chrono::tick::now() + 50ms));
		pstrand->destroy();

		std::this_thread::sleep_for(150ms);
		EXPECT_FALSE(is_executed.load());
		EXPECT_FALSE(pstrand->post_at(make_executable([] {}), chrono::tick::now() + 10ms));
	}

	// 14) make_executable(lambda) - the captures are released with the executable (not kept in the pool until it is reused)
	TEST(CGDK10_executor, make_executable_releases_captures)
	{
		auto pcaptured = std::make_shared<int>(0);
		std::atomic<int> count{ 0 };

		// - posted : released when executed
		for (int i = 0; i < 100; ++i)
			EXPECT_TRUE(system_executor::post(make_executable([pcaptured, &count] { ++count; })));
		EXPECT_TRUE(wait_until([&] { return count == 100 && pcaptured.use_count() == 1; }));

		// - not executed : released with the last reference
		{
			auto pexecutable = make_executable([pcaptured, &count] { ++count; });
			EXPECT_EQ(pcaptured.use_count(), 2);
		}
		EXPECT_EQ(pcaptured.use_count(), 1);

		// - the same executable posted twice : the captures live until the last execution
		count = 0;
		{
			auto pexecutable = make_executable([pcaptured, &count] { ++count; });
			EXPECT_TRUE(system_executor::post(pexecutable));
			EXPECT_TRUE(system_executor::post(pexecutable));
		}
		EXPECT_TRUE(wait_until([&] { return count == 2 && pcaptured.use_count() == 1; }));
	}

	// 15) make_executable - a move-only capture, the return value is the result of process_execute()
	TEST(CGDK10_executor, make_executable_move_only_and_result)
	{
		std::atomic<int> value{ 0 };
		auto pvalue = std::make_unique<int>(7);
		EXPECT_TRUE(system_executor::post(make_executable([pvalue = std::move(pvalue), &value] { value = *pvalue; })));
		EXPECT_TRUE(wait_until([&] { return value == 7; }));

		auto pexecutable = make_executable([] { return 42; });
		EXPECT_EQ(pexecutable->process_execute(0, 0), 42);
		auto pexecutable_void = make_executable([&value] { value = 9; });
		EXPECT_EQ(pexecutable_void->process_execute(0, 0), 0);
		EXPECT_EQ(value.load(), 9);
	}

#if defined(_WIN32)
	// 17) api::get_core_count - not more than the CPUs of the affinity of this process (it was hardware_concurrency() : the CPUs of
	//     the machine. run with "start /affinity 3" to see it)
	TEST(CGDK10_executor, core_count_follows_affinity)
	{
		DWORD_PTR mask_process = 0;
		DWORD_PTR mask_system = 0;
		ASSERT_TRUE(::GetProcessAffinityMask(::GetCurrentProcess(), &mask_process, &mask_system));

		size_t count_affinity = 0;
		for (auto mask = static_cast<uint64_t>(mask_process); mask != 0; mask &= mask - 1)
			++count_affinity;

		printf("    core count %zu (affinity %zu, hardware_concurrency %u)\n", api::get_core_count(), count_affinity, std::thread::hardware_concurrency());
		EXPECT_GE(api::get_core_count(), 1u);
		if (count_affinity != 0 && ::GetActiveProcessorGroupCount() == 1)
			EXPECT_LE(api::get_core_count(), count_affinity);
	}
#endif

#if defined(__linux__)
	// 17) api::get_core_count - not more than the CPUs this process may run on (affinity : taskset, cpuset)
	//     (it was hardware_concurrency() : the CPUs of the machine. run with "taskset -c 0-1" to see it)
	TEST(CGDK10_executor, core_count_follows_affinity)
	{
		cpu_set_t set_cpu;
		CPU_ZERO(&set_cpu);
		ASSERT_EQ(::sched_getaffinity(0, sizeof(set_cpu), &set_cpu), 0);

		EXPECT_GE(api::get_core_count(), 1u);
		EXPECT_LE(api::get_core_count(), static_cast<size_t>(CPU_COUNT(&set_cpu)));
	}

	// 18) api::Nthread::force_raise_exception - not supported on linux : false, the thread is not signaled
	//     (it sent SIGUSR2 with a throwing handler installed only for a moment : the signal often came after the old handler was back,
	//     the default action of SIGUSR2 terminates the process)
	TEST(CGDK10_executor, force_raise_exception_linux_does_nothing)
	{
		auto pexecutor = executor::make_io_context_threads("test force raise", 1);
		ASSERT_TRUE(pexecutor.exist());

		std::atomic<api::Nthread*> pthread{ nullptr };
		object_ptr<api::Nthread> pthread_hold;
		std::atomic<bool> is_ended{ false };
		std::atomic<bool> is_thrown{ false };

		pexecutor->post(make_executable([&]
			{
				pthread_hold = api::get_current_thread();
				pthread = pthread_hold.get();
				try
				{
					for (int i = 0; i < 30; ++i)
						std::this_thread::sleep_for(10ms);
				}
				catch (...)
				{
					is_thrown = true;
				}
				is_ended = true;
			}));

		ASSERT_TRUE(wait_until([&] { return pthread.load() != nullptr; }));

		EXPECT_FALSE(pthread.load()->force_raise_exception("test"));

		ASSERT_TRUE(wait_until([&] { return is_ended.load(); }));
		EXPECT_FALSE(is_thrown.load());

		pthread_hold.reset();
		pexecutor->stop();
	}
#endif
}
