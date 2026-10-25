#include "pch.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
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

	// 14) entity - unregister_event() removes a registered event (the condition was inverted : never removed)
	TEST(CGDK10_time_event, entity_unregister_event)
	{
		auto pentity = make_own<time_event::entity>();

		auto events_1 = time_event::make_event_iteration(chrono::time::utc::now() + 1h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		auto events_2 = time_event::make_event_iteration(chrono::time::utc::now() + 2h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		time_event::Ievent* pevent_1 = events_1[0].get();
		time_event::Ievent* pevent_2 = events_2[0].get();

		pentity->register_event(std::move(events_1[0]));
		pentity->register_event(std::move(events_2[0]));
		ASSERT_EQ(pentity->get_event_count(), 2u);
		EXPECT_EQ(pentity->get_front_next_time(), pevent_1->event_status().time_next);

		// - the front is removed : the next one becomes the front
		auto time_2 = pevent_2->event_status().time_next;
		pentity->unregister_event(pevent_1);
		EXPECT_EQ(pentity->get_event_count(), 1u);
		EXPECT_EQ(pentity->get_front_next_time(), time_2);
		EXPECT_EQ(pentity->entity_status().time_next, time_2);

		pentity->unregister_event(pevent_2);
		EXPECT_EQ(pentity->get_event_count(), 0u);
	}

	// 14-1) entity - unregister_event() of an event not registered (it erased end() : undefined behavior)
	TEST(CGDK10_time_event, entity_unregister_event_not_registered)
	{
		auto pentity = make_own<time_event::entity>();

		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 1h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		pentity->register_event(std::move(events[0]));

		auto pother = make_own<time_event::event>();
		pentity->unregister_event(pother.get());
		EXPECT_EQ(pentity->get_event_count(), 1u);
	}

	// 14-2) entity - an event unregistered while it is executing is not pushed again
	TEST(CGDK10_time_event, entity_unregister_event_while_executing)
	{
		std::atomic<int> count{ 0 };
		time_event::Ievent* pevent_raw = nullptr;

		auto pentity = make_own<time_event::entity>();
		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 20ms, 20ms, 0, [&](chrono::time::utc::time_point) -> result_code
		{
			++count;
			pentity->unregister_event(pevent_raw);
			return eRESULT::SUCCESS;
		});
		pevent_raw = events[0].get();
		pentity->register_event(std::move(events[0]));
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return count.load() >= 1; }));
		api::sleep_for(200ms);

		EXPECT_EQ(count.load(), 1);
		EXPECT_EQ(pentity->get_event_count(), 0u);

		pentity->stop();
	}

	// - entities far in the future : the entity tested is pushed after them (deep in the heap of the manager)
	static std::vector<own_ptr<time_event::entity_iteration<>>> make_far_entities(int _count)
	{
		std::vector<own_ptr<time_event::entity_iteration<>>> entities;
		for (int i = 0; i < _count; ++i)
		{
			auto pentity = make_own<time_event::entity_iteration<>>();
			pentity->set_event_time(chrono::time::utc::now() + 1h);
			pentity->set_event_interval(1h);
			pentity->set_event_function([](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
			pentity->start();
			entities.push_back(std::move(pentity));
		}
		return entities;
	}

	// 15) entity - an earlier event registered on an entity deep in the heap is executed in time
	//     (register_event() wrote time_next without moving the entity in the heap : executed after the front (1h))
	TEST(CGDK10_time_event, entity_register_earlier_event_deep_in_heap)
	{
		auto entities = make_far_entities(31);

		std::atomic<int> count{ 0 };
		auto pentity = make_own<time_event::entity>();
		auto events_far = time_event::make_event_iteration(chrono::time::utc::now() + 2h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		pentity->register_event(std::move(events_far[0]));
		pentity->start();
		ASSERT_TRUE(is_registered(pentity.get()));

		// - an earlier event
		auto events = time_event::make_event_once(chrono::time::utc::now() + 50ms, [&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->register_event(std::move(events[0]));

		EXPECT_TRUE(wait_until([&] { return count.load() == 1; }, 3s));

		pentity->stop();
		for (auto& iter : entities)
			iter->stop();
	}

	// 16) Ientity::set_next_time() - an earlier time on a registered entity deep in the heap is executed in time
	//     (it was written without moving the entity in the heap and the manager was not notified : state RUN)
	TEST(CGDK10_time_event, entity_set_next_time_deep_in_heap)
	{
		auto entities = make_far_entities(31);

		std::atomic<int> count{ 0 };
		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() + 2h);
		pentity->set_event_interval(1h);
		pentity->set_event_times(1);
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->start();
		ASSERT_TRUE(is_registered(pentity.get()));

		pentity->set_next_time(chrono::time::utc::now() + 50ms);

		EXPECT_TRUE(wait_until([&] { return count.load() == 1; }, 3s));

		pentity->stop();
		for (auto& iter : entities)
			iter->stop();
	}

	// 17) entity_manager - stop right after start does not hang (the stop flag was set out of the queue lock and
	//     the thread could miss the wakeup between the check and the wait : on_stopping() waited forever)
	TEST(CGDK10_time_event, manager_stop_right_after_start)
	{
		std::atomic<bool> done{ false };

		std::thread thread_test([&]
		{
			for (int i = 0; i < 200; ++i)
			{
				time_event::entity_manager::reset_instance();
				time_event::entity_manager::initialize_instance();
			}
			done = true;
		});

		if (!wait_until([&] { return done.load(); }, 60s))
		{
			ADD_FAILURE() << "entity_manager stop hung";
			std::fflush(stdout);
			std::_Exit(1);
		}
		thread_test.join();

		EXPECT_TRUE(time_event::entity_manager::instance().exist());
	}

	// 18) sENTITY_STATUS::reset_next_time() resets time_next (it reset time_last_failed)
	TEST(CGDK10_time_event, entity_status_reset_next_time)
	{
		time_event::sENTITY_STATUS status;
		auto time_now = chrono::time::utc::now();
		status.time_next = time_now;
		status.time_last_failed = time_now;

		status.reset_next_time();

		EXPECT_EQ(status.time_next, chrono::time::utc::time_point(chrono::time::duration::zero()));
		EXPECT_EQ(status.time_last_failed, time_now);
	}

	// 19) Ievent::unregister_event() from its own process_event() (m_pentity was never set : it did nothing)
	TEST(CGDK10_time_event, event_unregister_itself)
	{
		std::atomic<int> count{ 0 };
		time_event::Ievent* pevent_raw = nullptr;

		auto pentity = make_own<time_event::entity>();
		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 20ms, 20ms, 0, [&](chrono::time::utc::time_point) -> result_code
		{
			++count;
			pevent_raw->unregister_event();
			return eRESULT::SUCCESS;
		});
		pevent_raw = events[0].get();
		pentity->register_event(std::move(events[0]));
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return count.load() >= 1; }));
		api::sleep_for(200ms);

		EXPECT_EQ(count.load(), 1);
		EXPECT_EQ(pentity->get_event_count(), 0u);

		pentity->stop();
	}

	// 20) Ievent::set_next_time() - an earlier time on a registered event is executed in time
	//     (written without the entity lock and without rebuilding its heap, the entity was not notified : executed late)
	TEST(CGDK10_time_event, event_set_next_time_earlier)
	{
		auto entities = make_far_entities(31);

		std::atomic<int> count{ 0 };
		auto pentity = make_own<time_event::entity>();
		auto events_a = time_event::make_event_iteration(chrono::time::utc::now() + 2h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		auto events_b = time_event::make_event_once(chrono::time::utc::now() + 3h, [&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		time_event::Ievent* pevent_b = events_b[0].get();
		pentity->register_event(std::move(events_a[0]));
		pentity->register_event(std::move(events_b[0]));
		pentity->start();

		pevent_b->set_next_time(chrono::time::utc::now() + 50ms);

		EXPECT_TRUE(wait_until([&] { return count.load() == 1; }, 3s));

		pentity->stop();
		for (auto& iter : entities)
			iter->stop();
	}

	// 21) Ievent::set_next_time() from its own process_event() is kept (the interval was added to it)
	TEST(CGDK10_time_event, event_set_next_time_while_executing)
	{
		std::atomic<int> count{ 0 };
		time_event::Ievent* pevent_raw = nullptr;

		auto pentity = make_own<time_event::entity>();
		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 20ms, 1h, 0, [&](chrono::time::utc::time_point) -> result_code
		{
			if (++count == 1)
				pevent_raw->set_next_time(chrono::time::utc::now() + 50ms);
			return eRESULT::SUCCESS;
		});
		pevent_raw = events[0].get();
		pentity->register_event(std::move(events[0]));
		pentity->start();

		EXPECT_TRUE(wait_until([&] { return count.load() >= 2; }, 3s));

		pentity->stop();
	}

	// 22) an event kept after its entity is destroyed : unregister_event() / set_next_time() do nothing (m_pentity cleared)
	TEST(CGDK10_time_event, event_entity_pointer_cleared)
	{
		object_ptr<time_event::Ievent> pevent_kept;
		{
			auto pentity = make_own<time_event::entity>();
			auto events = time_event::make_event_iteration(chrono::time::utc::now() + 1h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
			pevent_kept = events[0].get();
			pentity->register_event(std::move(events[0]));
		}
		pevent_kept->unregister_event();
		pevent_kept->set_next_time(chrono::time::utc::now());

		auto pentity = make_own<time_event::entity>();
		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 1h, 1h, 0, [](chrono::time::utc::time_point) -> result_code { return eRESULT::SUCCESS; });
		pentity->register_event(std::move(events[0]));
		pentity->unregister_event_all();
		EXPECT_EQ(pentity->get_event_count(), 0u);
	}

	// 23) unlimited with no interval (count 0, interval 0) : executed once and done (it ran again and again without an end)
	TEST(CGDK10_time_event, event_unlimited_without_interval)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity>();
		auto events = time_event::make_event_iteration(chrono::time::utc::now() + 10ms, chrono::time::duration::zero(), 0, [&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->register_event(std::move(events[0]));
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return count.load() >= 1; }));
		api::sleep_for(200ms);
		EXPECT_EQ(count.load(), 1);
		EXPECT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		pentity->stop();
	}

	TEST(CGDK10_time_event, entity_iteration_unlimited_without_interval)
	{
		std::atomic<int> count{ 0 };

		auto pentity = make_own<time_event::entity_iteration<>>();
		pentity->set_event_time(chrono::time::utc::now() + 10ms);
		pentity->set_event_interval(chrono::time::duration::zero());
		pentity->set_event_function([&](chrono::time::utc::time_point) -> result_code { ++count; return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return count.load() >= 1; }));
		api::sleep_for(200ms);
		EXPECT_EQ(count.load(), 1);
		EXPECT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		pentity->stop();
	}

	TEST(CGDK10_time_event, entity_start_iteration_end_unlimited_without_interval)
	{
		std::mutex lock;
		std::vector<int> calls;

		auto time_now = chrono::time::utc::now();
		auto pentity = make_own<time_event::entity_start_iteration_end<>>();
		pentity->set_event_start_time(time_now + 20ms);
		pentity->set_event_end_time(time_now + 200ms);
		pentity->set_event_interval(chrono::time::duration::zero());
		pentity->set_start_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(1); return eRESULT::SUCCESS; });
		pentity->set_iteration_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); if (calls.size() < 100) calls.push_back(2); return eRESULT::SUCCESS; });
		pentity->set_end_function([&](chrono::time::utc::time_point) -> result_code { std::lock_guard g(lock); calls.push_back(3); return eRESULT::SUCCESS; });
		pentity->start();

		ASSERT_TRUE(wait_until([&] { return pentity->entity_status().state.load() == time_event::eSTATE::DONE; }));

		std::lock_guard g(lock);
		EXPECT_EQ(calls, (std::vector<int>{ 1, 2, 3 }));
	}
}
