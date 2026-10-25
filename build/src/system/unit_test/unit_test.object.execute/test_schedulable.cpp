#include "pch.h"
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace CGDK
{
	//-----------------------------------------------------------------------------
	// helpers
	//-----------------------------------------------------------------------------
	// - schedulable counting its executions (execution times / threads)
	class schedulable_counter : public schedulable::Nexecutable
	{
	public:
		std::atomic<int>	count{ 0 };
		std::mutex			lock;
		std::vector<double>	times_ms;
		std::set<std::thread::id> threads;
		chrono::tick::time_point tick_start = chrono::tick::now();
		std::atomic<int>	sleep_at{ 0 };			// sleep on the n-th execution
		chrono::tick::duration sleep_duration{};

	protected:
		virtual void on_execute() override
		{
			int n = ++count;
			{
				std::lock_guard g(lock);
				times_ms.push_back(std::chrono::duration<double, std::milli>(chrono::tick::now() - tick_start).count());
				threads.insert(std::this_thread::get_id());
			}
			if (n == sleep_at)
				std::this_thread::sleep_for(sleep_duration);
		}
	};

	static size_t registered_count()
	{
		auto pmanager = system_executor::schedulable_manager();
		std::shared_lock lock(pmanager->get_lockable());
		return pmanager->size();
	}

	//-----------------------------------------------------------------------------
	// tests
	//-----------------------------------------------------------------------------
	// 1) executed at the interval
	TEST(CGDK10_schedulable, schedulable_basic)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(50ms);

		ASSERT_TRUE(system_executor::register_schedulable(p));
		EXPECT_TRUE(p->schedulable_manager().exist());

		api::sleep_for(525ms);
		system_executor::unregister_schedulable(p);

		// - 500ms / 50ms = 10 times
		EXPECT_GE(p->count.load(), 9);
		EXPECT_LE(p->count.load(), 11);
	}

	// 2) not executed after unregister
	TEST(CGDK10_schedulable, schedulable_unregister)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(20ms);

		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(200ms);

		EXPECT_TRUE(system_executor::unregister_schedulable(p));
		EXPECT_TRUE(p->schedulable_manager().empty());
		EXPECT_FALSE(system_executor::unregister_schedulable(p));		// already unregistered

		api::sleep_for(50ms);											// executing one may finish
		int count_after = p->count;
		api::sleep_for(200ms);

		EXPECT_GT(count_after, 0);
		EXPECT_EQ(p->count.load(), count_after);
	}

	// 3) register twice fails / register again after unregister works
	TEST(CGDK10_schedulable, schedulable_register_twice)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(20ms);

		EXPECT_TRUE(system_executor::register_schedulable(p));
		EXPECT_FALSE(system_executor::register_schedulable(p));

		EXPECT_TRUE(system_executor::unregister_schedulable(p));
		EXPECT_TRUE(system_executor::register_schedulable(p));

		int count_before = p->count;
		api::sleep_for(150ms);
		EXPECT_GT(p->count.load(), count_before);

		EXPECT_TRUE(system_executor::unregister_schedulable(p));
	}

	// 4) changing the interval while registered is applied immediately
	TEST(CGDK10_schedulable, schedulable_change_interval)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(1000ms);

		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(100ms);
		EXPECT_EQ(p->count.load(), 0);

		// - 1000ms -> 50ms (no need to wait the remaining 900ms)
		p->execute_interval(50ms);
		api::sleep_for(525ms);
		system_executor::unregister_schedulable(p);

		EXPECT_GE(p->count.load(), 9);
		EXPECT_LE(p->count.load(), 11);
	}

	// 5) executed on the executor of schedulable
	TEST(CGDK10_schedulable, schedulable_custom_executor)
	{
		auto pexecutor = make_own<executor::queue_list_threads>();
		pexecutor->start({ 1 });

		auto p = make_object<schedulable_counter>();
		p->execute_interval(20ms);
		p->executor(pexecutor.get());

		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(300ms);
		system_executor::unregister_schedulable(p);
		api::sleep_for(50ms);

		std::lock_guard g(p->lock);
		EXPECT_GE(p->count.load(), 10);
		EXPECT_EQ(p->threads.size(), 1u);								// the only thread of the executor
		EXPECT_EQ(p->threads.count(std::this_thread::get_id()), 0u);
	}

	// 6) overrun : CATCH_UP - the missed executions are executed immediately (count is guaranteed)
	TEST(CGDK10_schedulable, schedulable_overrun_catch_up)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(50ms);
		p->overrun_policy(eSCHEDULE_OVERRUN::CATCH_UP);
		p->sleep_at = 3;
		p->sleep_duration = 180ms;										// 3rd execution(at 150ms) ends at 330ms

		p->tick_start = chrono::tick::now();
		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(1025ms);
		system_executor::unregister_schedulable(p);

		std::lock_guard g(p->lock);

		// - count : 1000ms / 50ms = 20 (missed 200/250/300 are executed)
		EXPECT_GE(p->times_ms.size(), 19u);

		// - 4th~6th executions are executed in a row right after the 3rd
		ASSERT_GE(p->times_ms.size(), 7u);
		EXPECT_LT(p->times_ms[5] - p->times_ms[3], 15.0);
	}

	// 7) overrun : SKIP - the missed executions are skipped (interval grid is kept)
	TEST(CGDK10_schedulable, schedulable_overrun_skip)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(50ms);
		p->overrun_policy(eSCHEDULE_OVERRUN::SKIP);
		p->sleep_at = 3;
		p->sleep_duration = 180ms;

		p->tick_start = chrono::tick::now();
		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(1025ms);
		system_executor::unregister_schedulable(p);

		std::lock_guard g(p->lock);

		// - count : 20 - 3 (missed 200/250/300 are skipped)
		EXPECT_GE(p->times_ms.size(), 15u);
		EXPECT_LE(p->times_ms.size(), 18u);

		// - 4th execution is at the next interval (350ms), and the interval is kept after
		ASSERT_GE(p->times_ms.size(), 6u);
		EXPECT_GE(p->times_ms[3], 345.0);
		EXPECT_NEAR(p->times_ms[4] - p->times_ms[3], 50.0, 15.0);
	}

	// 8) start with context : "interval"(ms) / "overrun"
	TEST(CGDK10_schedulable, schedulable_start_with_context)
	{
		std::atomic<int> count{ 0 };

		auto p = make_own<schedulable::executable_function<>>();
		p->set_function([&count] { ++count; });

		context setting;
		setting.from_json(R"({ "interval": 30, "overrun": "skip" })");
		p->start(setting);

		EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(p->execute_interval()).count(), 30);
		EXPECT_EQ(p->overrun_policy(), eSCHEDULE_OVERRUN::SKIP);

		api::sleep_for(320ms);
		p.reset();

		EXPECT_GE(count.load(), 8);
		EXPECT_LE(count.load(), 11);
	}

	// 9) start with context : "tick"(chrono::tick count) and "interval" has priority over "tick"
	TEST(CGDK10_schedulable, schedulable_start_with_context_tick)
	{
		{
			auto p = make_own<schedulable::executable_function<>>();
			p->set_function([] {});

			auto ticks = std::chrono::duration_cast<chrono::tick::duration>(std::chrono::milliseconds(70)).count();

			context setting;
			setting["tick"sv] = std::to_string(ticks);
			p->start(setting);

			EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(p->execute_interval()).count(), 70);
		}
		{
			auto p = make_own<schedulable::executable_function<>>();
			p->set_function([] {});

			context setting;
			setting.from_json(R"({ "interval": 40, "tick": 1 })");
			p->start(setting);

			EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(p->execute_interval()).count(), 40);
		}
	}

	// 10) many schedulables : all registered / all unregistered (no pending execution left)
	TEST(CGDK10_schedulable, schedulable_many)
	{
		auto count_registered_base = registered_count();
		auto count_pended_base = system_executor::scheduler()->pended_count();

		std::vector<object_ptr<schedulable_counter>> vector_schedulable;
		for (int i = 0; i < 500; ++i)
		{
			auto p = make_object<schedulable_counter>();
			p->execute_interval(std::chrono::milliseconds(5 + i % 50));
			ASSERT_TRUE(system_executor::register_schedulable(p));
			vector_schedulable.push_back(std::move(p));
		}

		EXPECT_EQ(registered_count(), count_registered_base + 500);

		api::sleep_for(500ms);

		for (auto& iter : vector_schedulable)
			EXPECT_TRUE(system_executor::unregister_schedulable(iter));

		EXPECT_EQ(registered_count(), count_registered_base);

		// - no more execution
		api::sleep_for(100ms);
		int64_t total_1 = 0; for (auto& iter : vector_schedulable) total_1 += iter->count;
		api::sleep_for(200ms);
		int64_t total_2 = 0; for (auto& iter : vector_schedulable) total_2 += iter->count;

		EXPECT_GT(total_1, 0);
		EXPECT_EQ(total_1, total_2);
		EXPECT_EQ(system_executor::scheduler()->pended_count(), count_pended_base);
	}

	// 11) executable_function : start / stop(destroy)
	TEST(CGDK10_schedulable, schedulable_executable_function)
	{
		std::atomic<int> count{ 0 };

		auto p = make_own<schedulable::executable_function<>>();
		p->start({ [&count] { ++count; }, 30ms });

		api::sleep_for(200ms);
		p.reset();

		int count_after = count;
		api::sleep_for(150ms);

		EXPECT_GE(count_after, 5);
		EXPECT_EQ(count.load(), count_after);
	}

	// 12) an interval of (max)() : never executed (now() + interval wrapped to a time in the past : executed at once, again and again)
	TEST(CGDK10_schedulable, schedulable_interval_max_never)
	{
		// - registered with the interval
		auto p = make_object<schedulable_counter>();
		p->execute_interval((chrono::tick::duration::max)());
		ASSERT_TRUE(system_executor::register_schedulable(p));
		api::sleep_for(100ms);
		EXPECT_EQ(p->count.load(), 0);

		// - the interval set while registered
		auto p_2 = make_object<schedulable_counter>();
		p_2->execute_interval(1h);
		ASSERT_TRUE(system_executor::register_schedulable(p_2));
		p_2->execute_interval((chrono::tick::duration::max)());
		api::sleep_for(100ms);
		EXPECT_EQ(p_2->count.load(), 0);

		EXPECT_TRUE(system_executor::unregister_schedulable(p));
		EXPECT_TRUE(system_executor::unregister_schedulable(p_2));
	}

	// 13) schedulable_manager() read while other threads register / unregister the schedulable (it was read without a lock)
	TEST(CGDK10_schedulable, schedulable_manager_read_while_registering)
	{
		auto p = make_object<schedulable_counter>();
		p->execute_interval(1h);

		std::atomic<bool> is_running{ true };
		std::atomic<int> count_read{ 0 };
		std::thread thread_read([&] { while (is_running) { if (p->schedulable_manager().exist()) ++count_read; } });

		for (int i = 0; i < 2000; ++i)
		{
			ASSERT_TRUE(system_executor::register_schedulable(p));
			ASSERT_TRUE(system_executor::unregister_schedulable(p));
		}

		is_running = false;
		thread_read.join();
		EXPECT_TRUE(p->schedulable_manager().empty());
	}
}
