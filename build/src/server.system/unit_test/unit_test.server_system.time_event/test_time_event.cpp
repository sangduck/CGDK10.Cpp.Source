#include "pch.h"
#include <atomic>
#include <mutex>
#include <stdexcept>
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

	static bool is_registered(time_event::Ientity* _pentity)
	{
		return time_event::entity_manager::instance()->is_exist(_pentity);
	}

	// 1) parse_duration - "<number>[unit] ..." (no unit : seconds)
	TEST(CGDK10_time_event, parse_duration)
	{
		chrono::time::duration d;

		ASSERT_TRUE(time_event::parse_duration("10", d));			EXPECT_EQ(d, chrono::time::duration(10s));
		ASSERT_TRUE(time_event::parse_duration("1.5s", d));			EXPECT_EQ(d, chrono::time::duration(1500ms));
		ASSERT_TRUE(time_event::parse_duration("100ms", d));		EXPECT_EQ(d, chrono::time::duration(100ms));
		ASSERT_TRUE(time_event::parse_duration("2m", d));			EXPECT_EQ(d, chrono::time::duration(120s));
		ASSERT_TRUE(time_event::parse_duration("1h 30min", d));		EXPECT_EQ(d, chrono::time::duration(5400s));
		ASSERT_TRUE(time_event::parse_duration("1 day", d));		EXPECT_EQ(d, chrono::time::duration(86400s));
		ASSERT_TRUE(time_event::parse_duration("  3 SEC  ", d));	EXPECT_EQ(d, chrono::time::duration(3s));

		EXPECT_FALSE(time_event::parse_duration("", d));
		EXPECT_FALSE(time_event::parse_duration("abc", d));
		EXPECT_FALSE(time_event::parse_duration("5x", d));
		EXPECT_FALSE(time_event::parse_duration("1..2", d));
	}

	// 2) parse_time_point - "now", "now +/- <duration>"
	TEST(CGDK10_time_event, parse_time_point)
	{
		chrono::time::utc::time_point t;

		auto time_before = chrono::time::utc::now();
		ASSERT_TRUE(time_event::parse_time_point("now", t));
		EXPECT_GE(t, time_before);
		EXPECT_LE(t, chrono::time::utc::now());

		ASSERT_TRUE(time_event::parse_time_point("now + 10s", t));
		EXPECT_GE(t, time_before + 10s);
		EXPECT_LE(t, chrono::time::utc::now() + 10s);

		ASSERT_TRUE(time_event::parse_time_point("NOW - 1m", t));
		EXPECT_LE(t, chrono::time::utc::now() - 60s);

		EXPECT_FALSE(time_event::parse_time_point("", t));
		EXPECT_FALSE(time_event::parse_time_point("now * 3", t));
		EXPECT_FALSE(time_event::parse_time_point("now + abc", t));
	}

	// 2-1) parse_time_point - a date time string
	TEST(CGDK10_time_event, parse_time_point_date_time)
	{
		chrono::time::utc::time_point t;

		// - valid : the same as from_string()
		ASSERT_TRUE(time_event::parse_time_point("2026-10-08 12:34:56", t));
		EXPECT_EQ(t, from_string<chrono::time::utc::time_point>(std::string("2026-10-08 12:34:56")));

		ASSERT_TRUE(time_event::parse_time_point("2026-10-08", t));
		EXPECT_EQ(t, from_string<chrono::time::utc::time_point>(std::string("2026-10-08 00:00:00")));

		// - invalid (the time point is not changed)
		auto t_before = t;
		EXPECT_FALSE(time_event::parse_time_point("not a time", t));
		EXPECT_FALSE(time_event::parse_time_point("2026", t));
		EXPECT_FALSE(time_event::parse_time_point("2026-13-01 00:00:00", t));
		EXPECT_FALSE(time_event::parse_time_point("2026-10-00 00:00:00", t));
		EXPECT_FALSE(time_event::parse_time_point("2026-10-08 24:00:00", t));
		EXPECT_FALSE(time_event::parse_time_point("2026-10-08 12:60:00", t));
		EXPECT_EQ(t, t_before);
	}

	// 3) to_string(eSTATE)
	TEST(CGDK10_time_event, to_string_state)
	{
		EXPECT_EQ(to_string(time_event::eSTATE::WAIT), "WAIT");
		EXPECT_EQ(to_string(time_event::eSTATE::RUN), "RUN");
		EXPECT_EQ(to_string(time_event::eSTATE::DONE), "DONE");
	}

	// 4) entity_iteration - executed 'times' times at the interval and then done/unregistered
	TEST(CGDK10_time_event, entity_iteration_times)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->name("test.iteration");
		pentity->set_event_time(chrono::time::utc::now() + 50ms);
		pentity->set_event_interval(50ms);
		pentity->set_event_times(3);
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });

		auto time_start = chrono::time::utc::now();
		pentity->start();
		EXPECT_TRUE(is_registered(pentity.get()));
		EXPECT_EQ(pentity->entity_status().state.load(), time_event::eSTATE::RUN);

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));
		auto elapsed = chrono::time::utc::now() - time_start;

		// - 3 times (50ms, 100ms, 150ms)
		EXPECT_EQ(count.load(), 3);
		EXPECT_EQ(pentity->entity_status().count_try.load(), 3);
		EXPECT_EQ(pentity->entity_status().count_succeeded.load(), 3);
		EXPECT_EQ(pentity->entity_status().count_failed.load(), 0);
		EXPECT_GE(elapsed, chrono::time::duration(150ms));

		// - no more execution / unregistered
		api::sleep_for(150ms);
		EXPECT_EQ(count.load(), 3);
		EXPECT_TRUE(wait_until([&] { return !is_registered(pentity.get()); }));
	}

	// 5) entity_iteration - stop() unregisters and no more execution
	TEST(CGDK10_time_event, entity_iteration_stop)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() + 20ms);
		pentity->set_event_interval(20ms);
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return count >= 2; }));

		pentity->stop();
		EXPECT_FALSE(is_registered(pentity.get()));

		api::sleep_for(30ms);
		auto count_stopped = count.load();
		api::sleep_for(100ms);
		EXPECT_EQ(count.load(), count_stopped);
	}

	// 6) entity_iteration - exception in the event function is counted as failed
	TEST(CGDK10_time_event, entity_iteration_exception)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() + 10ms);
		pentity->set_event_interval(10ms);
		pentity->set_event_times(2);
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; throw std::runtime_error("test exception"); });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		EXPECT_EQ(count.load(), 2);
		EXPECT_EQ(pentity->entity_status().count_try.load(), 2);
		EXPECT_EQ(pentity->entity_status().count_succeeded.load(), 0);
		EXPECT_EQ(pentity->entity_status().count_failed.load(), 2);
	}

	// 7) entity_iteration - the time already passed is executed at once (catch up)
	TEST(CGDK10_time_event, entity_iteration_catch_up)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() - 1s);
		pentity->set_event_interval(100ms);
		pentity->set_event_times(5);
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->start();

		// - 5 times (all past) are executed without waiting the interval
		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }, 300ms));
		EXPECT_EQ(count.load(), 5);
	}

	// 8) entity_start_end - start at the start time and end at the end time
	TEST(CGDK10_time_event, entity_start_end)
	{
		std::mutex lock;
		std::vector<std::pair<int, chrono::time::utc::time_point>> calls;

		auto time_now = chrono::time::utc::now();
		auto time_start = time_now + 50ms;
		auto time_end = time_now + 150ms;

		auto pentity = make_own<time_event::entity_start_end<>>();
		pentity->set_event_start_time(time_start);
		pentity->set_event_end_time(time_end);
		pentity->set_start_function([&](chrono::time::utc::time_point _t) -> result_code { std::lock_guard g(lock); calls.emplace_back(1, _t); return eRESULT::SUCCESS; });
		pentity->set_end_function([&](chrono::time::utc::time_point _t) -> result_code { std::lock_guard g(lock); calls.emplace_back(2, _t); return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		std::lock_guard g(lock);
		ASSERT_EQ(calls.size(), 2u);
		EXPECT_EQ(calls[0].first, 1);
		EXPECT_EQ(calls[1].first, 2);
		EXPECT_GE(calls[0].second, time_start);
		EXPECT_GE(calls[1].second, time_end);
		EXPECT_EQ(pentity->entity_status().count_try.load(), 2);
		EXPECT_EQ(pentity->entity_status().count_succeeded.load(), 2);
		EXPECT_TRUE(wait_until([&] { return !is_registered(pentity.get()); }));
	}

	// 9) entity_start_iteration_end - start -> iterations (at the interval) -> end
	TEST(CGDK10_time_event, entity_start_iteration_end)
	{
		std::mutex lock;
		std::vector<int> calls;

		auto time_now = chrono::time::utc::now();
		auto time_start = time_now + 50ms;
		auto time_end = time_now + 200ms;

		auto pentity = make_own<time_event::entity_start_iteration_end<>>();
		pentity->set_event_start_time(time_start);
		pentity->set_event_end_time(time_end);
		pentity->set_event_interval(30ms);
		pentity->set_start_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(1); return eRESULT::SUCCESS; });
		pentity->set_iteration_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(2); return eRESULT::SUCCESS; });
		pentity->set_end_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(3); return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		// - start(50ms) / iteration(80, 110, 140, 170ms) / end(200ms)
		std::lock_guard g(lock);
		EXPECT_EQ(calls, (std::vector<int>{ 1, 2, 2, 2, 2, 3 }));
	}

	// 9-1) entity_start_iteration_end - waits until the end time after the last iteration ('times' completed or 0)
	//      (it was executed again and again until the end time : the next time was not advanced at the last iteration)
	class counting_start_iteration_end : public time_event::entity_start_iteration_end<>
	{
	public:
		std::atomic<int> count_execute{ 0 };

	protected:
		virtual intptr_t process_execute(intptr_t _return, size_t _param) override
		{
			++this->count_execute;
			return time_event::entity_start_iteration_end<>::process_execute(_return, _param);
		}
	};

	static void test_start_iteration_end_times(int _times, const std::vector<int>& _expected_calls)
	{
		std::mutex lock;
		std::vector<int> calls;

		auto time_now = chrono::time::utc::now();

		auto pentity = make_own<counting_start_iteration_end>();
		pentity->set_event_start_time(time_now + 30ms);
		pentity->set_event_end_time(time_now + 330ms);
		pentity->set_event_interval(20ms);
		pentity->set_event_times(_times);
		pentity->set_start_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(1); return eRESULT::SUCCESS; });
		pentity->set_iteration_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(2); return eRESULT::SUCCESS; });
		pentity->set_end_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(3); return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		{
			std::lock_guard g(lock);
			EXPECT_EQ(calls, _expected_calls);
		}

		// - start / iterations / end (+ margin) : not executed repeatedly while waiting the end time
		EXPECT_LE(pentity->count_execute.load(), static_cast<int>(_expected_calls.size()) + 4);
	}

	TEST(CGDK10_time_event, entity_start_iteration_end_times)
	{
		// - start(30ms) / iteration(50, 70ms) / end(330ms)
		test_start_iteration_end_times(2, { 1, 2, 2, 3 });
	}

	TEST(CGDK10_time_event, entity_start_iteration_end_times_zero)
	{
		// - start(30ms) / end(330ms)
		test_start_iteration_end_times(0, { 1, 3 });
	}

	// 10) entity_manager - entities are executed in the order of time
	TEST(CGDK10_time_event, manager_execute_order)
	{
		std::mutex lock;
		std::vector<int> order;
		std::vector<own_ptr<time_event::entity_iteration<>>> entities;

		auto time_base = chrono::time::utc::now() + 100ms;

		// - registered in shuffled order
		//   (40ms apart : the wait of the manager can be late up to the timer resolution (about 15.6ms on Windows)
		//    and entities popped together are executed concurrently on the system executor)
		for (int i : { 5, 2, 8, 0, 7, 3, 9, 1, 6, 4 })
		{
			auto pentity = make_own<time_event::entity_iteration<>>();
			pentity->set_event_time(time_base + std::chrono::milliseconds(i * 40));
			pentity->set_event_times(1);
			pentity->set_event_function([&, i](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); order.push_back(i); return eRESULT::SUCCESS; });
			pentity->start();
			entities.push_back(std::move(pentity));
		}

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 10; }));

		std::lock_guard g(lock);
		EXPECT_EQ(order, (std::vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
	}

	// 11) entity_manager - the order is kept after some entities are unregistered (heap removal)
	TEST(CGDK10_time_event, manager_execute_order_after_unregister)
	{
		std::mutex lock;
		std::vector<int> order;
		std::vector<own_ptr<time_event::entity_iteration<>>> entities(16);

		auto time_base = chrono::time::utc::now() + 200ms;

		// - this sequence broke the heap with the old _process_detach() (moved to the right child without its position)
		//   => 7 was executed before 6
		for (int i : { 2, 11, 9, 1, 0, 8, 4, 7, 5, 6, 12, 15, 14, 10, 3, 13 })
		{
			auto pentity = make_own<time_event::entity_iteration<>>();
			pentity->set_event_time(time_base + std::chrono::milliseconds(i * 40));
			pentity->set_event_times(1);
			pentity->set_event_function([&, i](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); order.push_back(i); return eRESULT::SUCCESS; });
			pentity->start();
			entities[i] = std::move(pentity);
		}

		// - unregister some entities in the middle of the heap
		for (int i : { 15, 12, 0, 5 })
		{
			entities[i]->stop();
			EXPECT_FALSE(is_registered(entities[i].get()));
		}

		ASSERT_TRUE(wait_until([&] { std::lock_guard g(lock); return order.size() == 12; }));
		api::sleep_for(50ms);

		std::lock_guard g(lock);
		EXPECT_EQ(order, (std::vector<int>{ 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 13, 14 }));
	}

	// 11-1) entity_manager - an id is assigned on register (unique / kept on re-register) and found by it
	TEST(CGDK10_time_event, manager_entity_id)
	{
		auto pmanager = time_event::entity_manager::instance();

		auto make_entity = []
		{
			auto pentity = make_own<time_event::entity_iteration<>>();
			pentity->set_event_time(chrono::time::utc::now() + 1h);
			pentity->set_event_interval(1h);
			pentity->set_event_function([](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
			return pentity;
		};

		auto pentity_a = make_entity();
		auto pentity_b = make_entity();
		pentity_a->start();
		pentity_b->start();

		auto id_a = pentity_a->entity_status().id;
		auto id_b = pentity_b->entity_status().id;
		EXPECT_NE(id_a, 0u);
		EXPECT_NE(id_b, 0u);
		EXPECT_NE(id_a, id_b);

		// - find_event
		EXPECT_EQ(pmanager->find_event(id_a).get(), static_cast<time_event::Ientity*>(pentity_a.get()));
		EXPECT_EQ(pmanager->find_event(id_b).get(), static_cast<time_event::Ientity*>(pentity_b.get()));

		// - unregister_entity(id)
		auto punregistered = pmanager->unregister_entity(id_a);
		EXPECT_EQ(punregistered.get(), static_cast<time_event::Ientity*>(pentity_a.get()));
		EXPECT_FALSE(is_registered(pentity_a.get()));
		EXPECT_TRUE(pmanager->find_event(id_a).empty());

		// - the id is kept on re-register
		pentity_a->start();
		EXPECT_TRUE(is_registered(pentity_a.get()));
		EXPECT_EQ(pentity_a->entity_status().id, id_a);

		pentity_a->stop();
		pentity_b->stop();
	}

	// 12) entity_manager - register / unregister
	TEST(CGDK10_time_event, manager_register_unregister)
	{
		auto pmanager = time_event::entity_manager::instance();
		ASSERT_TRUE(pmanager.exist());

		auto size_before = pmanager->size();

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() + 1h);
		pentity->set_event_interval(1h);
		pentity->set_event_function([](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		pentity->start();

		EXPECT_TRUE(pmanager->is_exist(pentity.get()));
		EXPECT_EQ(pmanager->size(), size_before + 1);
		EXPECT_FALSE(pmanager->register_entity(object_ptr<time_event::Ientity>(pentity.get())));	// already registered

		EXPECT_TRUE(pmanager->unregister_entity(pentity.get()));
		EXPECT_FALSE(pmanager->is_exist(pentity.get()));
		EXPECT_EQ(pmanager->size(), size_before);
		EXPECT_FALSE(pmanager->unregister_entity(pentity.get()));								// already unregistered
	}

	// 13) print_statistics - with registered entities
	TEST(CGDK10_time_event, print_statistics)
	{
		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->name("test.print_statistics");
		pentity->set_event_time(chrono::time::utc::now() + 1h);
		pentity->set_event_interval(1h);
		pentity->set_event_function([](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		pentity->start();

		time_event::entity_manager::print_statistics();

		pentity->stop();
		SUCCEED();
	}
}
