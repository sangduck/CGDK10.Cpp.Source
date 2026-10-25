#include "pch.h"

using namespace CGDK;


time_event::Nentity_iteration::Nentity_iteration()
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::ITERATION);

	// 2) create log
	SCOPED_LOCK(this->m_lockable_plog_result)
	{
		this->m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::Nentity_iteration::Nentity_iteration(const std::string_view _name)
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::ITERATION);

	// 2) set name
	this->name(_name);

	// 3) create log
	SCOPED_LOCK(this->m_lockable_plog_result)
	{
		this->m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::Nentity_iteration::~Nentity_iteration() noexcept
{
}

void time_event::Nentity_iteration::set_event_time(const chrono::time::utc::time_point& _time) noexcept
{
	this->entity_status().time_next = _time;
}

void time_event::Nentity_iteration::set_event_interval(const chrono::time::duration& _duration) noexcept
{
	this->m_time_diff_interval = _duration;
}

void time_event::Nentity_iteration::set_event_times(int _times) noexcept
{
	this->m_times = _times;
}

result_code time_event::Nentity_iteration::_process_starting(context& _context)
{
	// 0) time settings in the context (e.g. JSON : "time_execute", "time_interval", "count_times")
	if (const auto& context_find = std::as_const(_context).find("time_execute"sv); context_find.is_valid())
	{
		std::string str_value = context_find;
		chrono::time::utc::time_point time_value;

		if (time_event::parse_time_point(str_value, time_value))
			this->set_event_time(time_value);
		else
			LOG_WARNING << "(warn) time_event: invalid 'time_execute' ["sv << str_value << "]"sv;
	}
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

	// 1) 
	auto result = this->Ientity::_process_starting(_context);

	// 2) set default setter
	if (this->m_entity_setting.setter == eSETTER::NONE)
	{
		this->m_entity_setting.setter = eSETTER::LOCAL;
	}

	// return)
	return result;
}

result_code time_event::Nentity_iteration::_process_start(context& _context)
{
	// 1) call parents's '_process_start'
	this->CGDK::object::Nstartable::_process_start(_context);

	// 2) register to...
	auto result = entity_manager::instance()->register_entity(this);

	// 3) log
	auto presult_event = this->_append_result_log("start time_event entity");
	presult_event->result = (result) ? eRESULT::SUCCESS : eRESULT::FAIL;
	this->_append_result_log_post(presult_event);

	// return)
	//  - result_code : 0 (SUCCESS) is success (returning the bool as it is means a failure)
	return (result) ? result_code(eRESULT::SUCCESS) : result_code(eRESULT::FAIL);
}

result_code time_event::Nentity_iteration::_process_stopping() noexcept
{
	// 1) call parents's '_process_stopping'
	auto result = this->CGDK::object::Nstartable::_process_stopping();

	// 2) register to...
	entity_manager::instance()->unregister_entity(this);

	// return)
	return result;
}

result_code time_event::Nentity_iteration::_process_stop() noexcept
{
	// 1) call parents's '_process_stop'
	auto result = this->CGDK::object::Nstartable::_process_stop();

	// 2) log
	auto presult_event = this->_append_result_log("stop time_event entity");
	presult_event->result = eRESULT::SUCCESS;
	this->_append_result_log_post(presult_event);

	// return)
	return result;
}

intptr_t time_event::Nentity_iteration::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check) 
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, 0);

	// 1) 현재시간과 다음 시간을 구한다. 
	auto time_now = chrono::time::utc::now();
	auto time_next = this->entity_status().time_next;

	while(time_now >= time_next)
	{
		// log) result log
		auto presult_event = this->_append_result_log("process_event_iteration");

		// check)
		CGASSERT_ERROR(presult_event.exist());

		try
		{
			// statistics) 
			this->m_entity_status.statistics_try();

			// - call 'process_event_iteration'
			presult_event->result = this->process_event_iteration(time_now);

			// statistics) 
			if(presult_event->result == eRESULT::SUCCESS)
			{
				this->entity_status().statistics_succeeded();
			}
		}
		catch(std::exception& e)
		{
			// - 결과를 Exception으로 설정
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
			// - 결과를 Exception으로 설정
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}

		// log) result log
		this->_append_result_log_post(presult_event);

		// 5) transit message
		entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));

		// 6) m_times을 1 줄인다.
		if(this->m_times > 0)
		{
			--this->m_times;

			if(this->m_times == 0)
			{
				// - DONE
				this->entity_status().state = eSTATE::DONE;

				// break)
				break;
			}
		}

		// check) Interval이 모두 0이면 안 된다!
		CGASSERT_ERROR(this->m_time_diff_interval != chrono::time::duration::zero());

		// check) unlimited (times <= 0) with no interval (<= 0) : executed once and done (it ran again and again in this loop
		//        without an end : the next time never passed time_now)
		if (this->m_times <= 0 && this->m_time_diff_interval <= chrono::time::duration::zero())
		{
			this->entity_status().state = eSTATE::DONE;
			break;
		}

		// 7) next time을 새로 설정한다. (interval만큼을 더한다.)
		time_next += this->m_time_diff_interval;

		// 8) next time을 설정한다.
		this->entity_status().time_next = time_next;
	}

	// 9) 완료되면 stop를 한다.
	if (this->entity_status().state != eSTATE::DONE)
	{
		entity_manager::instance()->_reschedule_time_entity(this);
	}
	else
	{
		// - stop
		this->stop();
	}

	// return)
	return intptr_t(0);
}
