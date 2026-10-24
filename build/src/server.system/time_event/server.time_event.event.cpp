#include "pch.h"

namespace CGDK
{

void time_event::event::process_reset()
{
}

result_code time_event::event::process_event(chrono::time::utc::time_point _time_now)
{
	// check) 
	RETURN_IF(m_event_function == nullptr, 0);
	
	// 1) call event_function
	return m_event_function(_time_now);
}

std::vector<own_ptr<time_event::Ievent>> time_event::make_event_once(chrono::time::utc::time_point _time_execute, EVENT_FUNCTION&& _event_function)
{
	// declare)
	std::vector<own_ptr<time_event::Ievent>> vector_events;

	// 1) once
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// - set event setting values
		time_event::sEVENT_SETTING event_setting;
		event_setting.name = "iteration";
		event_setting.type = time_event::eTYPE::NONE;
		event_setting.option = 0;
		event_setting.time_execute = _time_execute;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = 1;

		// - set ...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = _time_execute;
		pevent->set_event_function(std::move(_event_function));

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// return)
	return vector_events;
}

std::vector<own_ptr<time_event::Ievent>> time_event::make_event_iteration(chrono::time::utc::time_point _time_first, chrono::time::duration _duration, int _count, EVENT_FUNCTION&& _event_function)
{
	// declare)
	std::vector<own_ptr<time_event::Ievent>> vector_events;

	// 1) iteration
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// - set event setting values
		time_event::sEVENT_SETTING event_setting;
		event_setting.name = "iteration";
		event_setting.type = eTYPE::NONE;
		event_setting.option = 0;
		event_setting.time_execute = _time_first;
		event_setting.time_interval = _duration;
		event_setting.count_times = _count;

		// - set ...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = _time_first;
		pevent->set_event_function(std::move(_event_function));

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// return)
	return vector_events;
}

std::vector<own_ptr<time_event::Ievent>> time_event::make_event_start_stop(chrono::time::utc::time_point _time_start, chrono::time::utc::time_point _time_end, int _count, EVENT_FUNCTION&& _event_function)
{
	// declare)
	std::vector<own_ptr<time_event::Ievent>> vector_events;

	// 1) start
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// declare)
		time_event::sEVENT_SETTING event_setting;
		event_setting.name = "start";
		event_setting.type = eTYPE::NONE;
		event_setting.option = 0;
		event_setting.time_execute = _time_start;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = _count;

		// - set...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = event_setting.time_execute;
		pevent->set_event_function(_event_function);

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// 2) stop
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// declare)
		sEVENT_SETTING event_setting;
		event_setting.name = "stop";
		event_setting.type = eTYPE::NONE;
		event_setting.time_execute = _time_end;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = _count;

		// - set...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = event_setting.time_execute;
		pevent->set_event_function(std::move(_event_function));

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// return)
	return vector_events;
}

std::vector<own_ptr<time_event::Ievent>> time_event::make_event_start_iteration_stop(chrono::time::utc::time_point _time_start, chrono::time::duration _duration, int _count, chrono::time::utc::time_point _time_end, EVENT_FUNCTION&& _event_function)
{
	// declare)
	std::vector<own_ptr<time_event::Ievent>> vector_events;

	// 1) start
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// declare)
		time_event::sEVENT_SETTING event_setting;
		event_setting.name = "start";
		event_setting.type = eTYPE::NONE;
		event_setting.option = 0;
		event_setting.time_execute = _time_start;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = _count;

		// - set...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = event_setting.time_execute;
		pevent->set_event_function(_event_function);

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// 2) iteration
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// declare)
		time_event::sEVENT_SETTING event_setting;
		event_setting.name = "iteration";
		event_setting.type = eTYPE::NONE;
		event_setting.option = 0;
		event_setting.time_execute = _time_start + _duration;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = _count;

		// - set...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = event_setting.time_execute;
		pevent->set_event_function(_event_function);

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// 3) stop
	{
		// - create event object
		auto pevent = make_own<time_event::event>();

		// declare)
		sEVENT_SETTING event_setting;
		event_setting.name = "stop";
		event_setting.type = eTYPE::NONE;
		event_setting.time_execute = _time_end;
		event_setting.time_interval = chrono::time::duration::zero();
		event_setting.count_times = _count;

		// - set...
		pevent->event_setting() = event_setting;
		pevent->event_status().time_next = event_setting.time_execute;
		pevent->set_event_function(std::move(_event_function));

		// - push
		vector_events.push_back(std::move(pevent));
	}

	// return)
	return vector_events;
}









































}