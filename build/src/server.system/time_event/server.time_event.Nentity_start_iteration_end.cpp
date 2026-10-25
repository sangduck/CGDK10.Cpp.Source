#include "pch.h"

using namespace CGDK;


time_event::Nentity_start_iteration_end::Nentity_start_iteration_end()
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::START_ITERATION_END);
}

time_event::Nentity_start_iteration_end::Nentity_start_iteration_end(const std::string_view _name) : time_event::Nentity_start_end(_name)
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::START_ITERATION_END);
}

time_event::Nentity_start_iteration_end::~Nentity_start_iteration_end() noexcept
{
}

void time_event::Nentity_start_iteration_end::set_event_interval(const chrono::time::duration& _duration) noexcept
{
	this->m_time_diff_interval = _duration;
}

void time_event::Nentity_start_iteration_end::set_event_times(int _times) noexcept
{
	this->m_times = _times;
}

result_code time_event::Nentity_start_iteration_end::_process_starting(context& _context)
{
	// 1) time settings in the context (e.g. JSON : "time_interval", "count_times" / "time_start", "time_end" in Nentity_start_end)
	if (const auto& context_find = std::as_const(_context).find("time_interval"sv); context_find.is_valid())
	{
		std::string str_value = context_find;
		chrono::time::duration duration_value;

		if (time_event::parse_duration(str_value, duration_value))
			this->set_event_interval(duration_value);
		else
			LOG_WARNING << "(warn) time_event: invalid 'time_interval' ["sv << str_value << "]"sv;
	}
	if (const auto& context_find = std::as_const(_context).find("count_times"sv); context_find.is_valid())
	{
		this->set_event_times(static_cast<int>(context_find));
	}

	// 2) 
	this->m_times_remained = -1;

	// 3) 
	Nentity_start_end::_process_starting(_context);

	// 4) set default setter
	if (this->m_entity_setting.setter == eSETTER::NONE)
	{
		this->m_entity_setting.setter = eSETTER::LOCAL;
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code time_event::Nentity_start_iteration_end::_process_start(context& _context)
{
	// 1) call parents's '_process_start'
	//    - not Nentity_start_end::_process_start() : it registers this entity too (registered twice)
	this->CGDK::object::Nstartable::_process_start(_context);

	// 2) register to...
	auto result = entity_manager::instance()->register_entity(this);

	// 3) log
	auto presult_event = this->_append_result_log("start time_event entity");
	presult_event->result = (result) ? eRESULT::SUCCESS : eRESULT::FAIL;
	this->_append_result_log_post(presult_event);

	// return)
	return (result) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
}

result_code time_event::Nentity_start_iteration_end::_process_stopping() noexcept
{
	// 1) call parents's '_process_stopping'
	//    - not Nentity_start_end::_process_stopping() : it unregisters this entity too (unregistered twice)
	this->CGDK::object::Nstartable::_process_stopping();

	// 2) register to...
	entity_manager::instance()->unregister_entity(this);

	// return)
	return eRESULT::SUCCESS;
}

result_code time_event::Nentity_start_iteration_end::_process_stop() noexcept
{
	// 1) call parents's '_process_stop'
	//    - not Nentity_start_end::_process_stop() : it appends the stop log too (logged twice)
	this->CGDK::object::Nstartable::_process_stop();

	// 2) log
	auto presult_event = this->_append_result_log("stop time_event entity");
	presult_event->result = eRESULT::SUCCESS;
	this->_append_result_log_post(presult_event);

	// return)
	return eRESULT::SUCCESS;
}

void time_event::Nentity_start_iteration_end::process_execute_start(const chrono::time::utc::time_point& _time_now)
{
	// check) 
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	// check) 
	CGASSERT_ERROR(this->m_time_event_start <= _time_now);

	// 1) start했으므로 false로 설정한다.
	this->m_is_event_start = false;

	// 2) append result log
	auto presult_event = this->_append_result_log("process_event_start");

	try
	{
		// statistics) 
		this->entity_status().statistics_try();

		// 3) (핵심) process_event_start함수를 호출한다.
		presult_event->result = this->process_event_start(_time_now);

		// statistics) 
		if (presult_event->result == eRESULT::SUCCESS)
		{
			this->entity_status().statistics_succeeded();
		}
	}
	catch(std::exception& /*e*/)
	{
		// - 결과를 Exception으로 설정
		presult_event->result = eRESULT::EXCEPTION;

		// statistics) 
		this->entity_status().statistics_failed();

		// event log)
		this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
	}
	catch(...)
	{
		// - 결과를 Exception으로 설정
		presult_event->result = eRESULT::EXCEPTION;

		// statistics) 
		this->entity_status().statistics_failed();

		// event log)
		this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
	}

	// 4) append result log
	this->_append_result_log_post(presult_event);

	// 5) transit message
	entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));

	// check) Interval이 모두 0이면 안 된다!
	CGASSERT_ERROR(this->m_time_diff_interval != chrono::time::duration::zero());

	// 6) Interval만큼을 더해 다음 시간을 설정한다.
	auto time_next = this->m_time_event_start + this->m_time_diff_interval;

	// 7) check terminated or not
	if (time_next < this->m_time_event_terminate)
	{
		// - set next time
		this->entity_status().time_next = time_next;

		// - reset m_times_remained
		this->m_times_remained = m_times;
	}
	else
	{
		// - set next time
		this->entity_status().time_next = this->m_time_event_terminate;

		// - reset m_times_remained
		this->m_times_remained = 0;
	}
}

void time_event::Nentity_start_iteration_end::process_execute_iteration(const chrono::time::utc::time_point& _time_now)
{
	// check) 
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	while (_time_now >= this->entity_status().time_next)
	{
		// log) 
		auto presult_event = this->_append_result_log("process_event_iteration");

		// check)
		CGASSERT_ERROR(presult_event.exist());

		try
		{
			// statistics) 
			this->entity_status().statistics_try();

			// 1) call 'process_event_iteration'
			presult_event->result = this->process_event_iteration(_time_now);

			// statistics) 
			if(presult_event->result == eRESULT::SUCCESS)
			{
				this->entity_status().statistics_succeeded();
			}
		}
		catch(std::exception& e)
		{
			// - set result as eRESULT::EXCEPTION
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));

			// - process exception
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}
		catch(...)
		{
			// - set result as eRESULT::EXCEPTION
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}

		// log) result log
		this->_append_result_log_post(presult_event);

		// 2) transit message
		entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));

		// 3) m_times을 1 줄인다.
		if(this->m_times_remained > 0)
		{
			--this->m_times_remained;

			// check) break if times is 0
			if (this->m_times_remained == 0)
			{
				// break)
				break;
			}
		}

		// check) Interval이 모두 0이면 안 된다!
		CGASSERT_ERROR(m_time_diff_interval != chrono::time::duration::zero());

		// check) unlimited (times <= 0) with no interval (<= 0) : the iteration is executed once, then waits the end time
		//        (it ran again and again in this loop without an end : the next time never passed _time_now)
		if (this->m_times_remained <= 0 && this->m_time_diff_interval <= chrono::time::duration::zero())
		{
			this->m_times_remained = 0;
			break;
		}

		// 4) set next - Interval만큼을 더해 다음 시간을 설정한다.
		this->entity_status().time_next += this->m_time_diff_interval;

		// 5) check terminated or not
		if (this->entity_status().time_next >= this->m_time_event_terminate)
		{
			// - set next time as terminate time
			this->entity_status().time_next = this->m_time_event_terminate;

			// - reset m_times_remained
			this->m_times_remained = 0;

			// break)
			break;
		}
	}
}

void time_event::Nentity_start_iteration_end::process_execute_end(const chrono::time::utc::time_point& _time_now)
{
	// check) 
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);

	// 1) 종료처리했으므로 false로 설정한다.
	this->m_is_event_terminate = false;

	// 2) 새로운 Result를 추가한다.
	auto presult_event = this->_append_result_log("process_event_end");

	try
	{
		// statistics) 
		this->entity_status().statistics_try();

		// 3) (핵심) process_event_end함수를 호출한다.
		presult_event->result = this->process_event_end(_time_now);

		// statistics) 
		if (presult_event->result == eRESULT::SUCCESS)
		{
			this->entity_status().statistics_succeeded();
		}
	}
	catch(std::exception& /*_e*/)
	{
		// - 결과를 Exception으로 설정
		presult_event->result = eRESULT::EXCEPTION;

		// statistics) 
		this->entity_status().statistics_failed();

		// event log)
		this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
	}
	catch(...)
	{
		// - 결과를 Exception으로 설정
		presult_event->result = eRESULT::EXCEPTION;

		// statistics) 
		this->entity_status().statistics_failed();

		// event log)
		this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
	}

	// 4) Result를 써넣는다.
	this->_append_result_log_post(presult_event);
	
	// 5) Message 날리기..
	entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));

	// 6) stop
	this->stop();

	// 7) set event state
	this->entity_status().state = eSTATE::DONE;

	// 8) reset event time
	this->entity_status().time_next = chrono::time::utc::time_point(chrono::time::duration::zero());
}

intptr_t time_event::Nentity_start_iteration_end::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// 1) get now time
	auto time_now = chrono::time::utc::now();

	// 2) execute 'process_execute_start'
	if(this->m_is_event_start)
	{
		this->process_execute_start(time_now);
	}

	// 3) execute 'process_execute_iteration' if remained count is not 0, 
	if(this->m_times_remained != 0)
	{
		this->process_execute_iteration(time_now);
	}

	// 3-1) no more iteration (count completed or 0) : wait until the end time
	//      (time_next is not advanced at the last iteration : it would be executed again and again until the end time)
	if (this->m_is_event_start == false && this->m_times_remained == 0 && this->m_is_event_terminate)
	{
		this->entity_status().time_next = this->m_time_event_terminate;
	}

	// 4) terminate time - execute 'process_execute_end'
	if(this->m_is_event_terminate)
	{
		if (time_now >= this->m_time_event_terminate)
		{
			this->process_execute_end(time_now);
		}
	}

	// 5) retry or stop
	if (this->entity_status().state != eSTATE::DONE)
	{
		entity_manager::instance()->_reschedule_time_entity(this);
	}
	else
	{
		this->stop();
	}

	// return)
	return intptr_t(0);
}
