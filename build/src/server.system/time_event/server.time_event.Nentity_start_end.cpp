#include "pch.h"

using namespace CGDK;


time_event::Nentity_start_end::Nentity_start_end()
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::START_END);

	// 2) create log
	SCOPED_LOCK(this->m_lockable_plog_result)
	{
		this->m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::Nentity_start_end::Nentity_start_end(const std::string_view _name)
{
	// 1) set time_event_entity type
	this->m_entity_setting.type = static_cast<uint64_t>(eTYPE::START_END);

	// 2) set name
	this->name(_name);

	// 3) create log
	SCOPED_LOCK(this->m_lockable_plog_result)
	{
		this->m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::Nentity_start_end::~Nentity_start_end() noexcept
{
}

void time_event::Nentity_start_end::set_event_start_time(const chrono::time::utc::time_point& _time) noexcept
{
	// 1) set start event time
	this->m_time_event_start = _time;

	// 2) set next time ...
	if (this->m_is_event_start == true)
	{
		this->entity_status().time_next = _time;
	}
}

void time_event::Nentity_start_end::set_event_end_time(const chrono::time::utc::time_point& _time) noexcept
{
	// 1) set start event time
	this->m_time_event_terminate = _time;

	// 2) set next time ...
	if (this->m_is_event_start == false && this->m_is_event_terminate == true)
	{
		this->entity_status().time_next = _time;
	}
}

result_code time_event::Nentity_start_end::_process_starting(context& _context)
{
	// 0) time settings in the context (e.g. JSON : "time_start", "time_end")
	if (const auto& context_find = std::as_const(_context).find("time_start"sv); context_find.is_valid())
	{
		std::string str_value = context_find;
		chrono::time::utc::time_point time_value;

		if (time_event::parse_time_point(str_value, time_value))
			this->set_event_start_time(time_value);
		else
			LOG_WARNING << "(warn) time_event: invalid 'time_start' ["sv << str_value << "]"sv;
	}
	if (const auto& context_find = std::as_const(_context).find("time_end"sv); context_find.is_valid())
	{
		std::string str_value = context_find;
		chrono::time::utc::time_point time_value;

		if (time_event::parse_time_point(str_value, time_value))
			this->set_event_end_time(time_value);
		else
			LOG_WARNING << "(warn) time_event: invalid 'time_end' ["sv << str_value << "]"sv;
	}

	// 1) ...
	this->entity_status().time_next = m_time_event_start;
	this->m_is_event_start = true;
	this->m_is_event_terminate = true;

	// 2) ...
	auto result = this->Ientity::_process_starting(_context);

	// 3) set default setter
	if (this->m_entity_setting.setter == eSETTER::NONE)
	{
		this->m_entity_setting.setter = eSETTER::LOCAL;
	}

	// return)
	return result;
}

result_code time_event::Nentity_start_end::_process_start(context& _context)
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

result_code time_event::Nentity_start_end::_process_stopping() noexcept
{
	// 1) call parents's '_process_stopping'
	auto result = this->CGDK::object::Nstartable::_process_stopping();

	// 2) register to...
	entity_manager::instance()->unregister_entity(this);

	// return)
	return result;
}

result_code time_event::Nentity_start_end::_process_stop() noexcept
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

intptr_t time_event::Nentity_start_end::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check) 
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, 0);

	// 1) get now utc time
	auto time_now = chrono::time::utc::now();

	// 2) start event
	if(this->m_is_event_start)
	{
		// check) 
		CGASSERT_ERROR(time_now >= this->m_time_event_start)

		// - set flag
			this->m_is_event_start = false;

		// - set next time as zero 'm_time_event_terminate'
		this->entity_status().time_next = this->m_time_event_terminate;

		// - append result log
		auto presult_event = this->_append_result_log("process_event_start");

		try
		{
			// statistics) 
			this->entity_status().statistics_try();

			// - call 'process_event_start'
			presult_event->result = this->process_event_start(time_now);

			// statistics) 
			if (presult_event->result == eRESULT::SUCCESS)
			{
				this->entity_status().statistics_succeeded();
			}

			// - transit message
			entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));
		}
		catch(std::exception& /*e*/)
		{
			// - set result as 'eRESULT::EXCEPTION'
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}
		catch(...)
		{
			// - set result as 'eRESULT::EXCEPTION'
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}

		// - append log
		this->_append_result_log_post(presult_event);
	}

	// 3) end event
	if(time_now >= this->m_time_event_terminate)
	{
		// - set flag
		this->m_is_event_terminate = false;

		// - set next time as zero
		this->entity_status().time_next = chrono::time::utc::time_point(chrono::time::duration::zero());

		// - append result log
		auto presult_event = this->_append_result_log("process_event_end");

		try
		{
			// statistics) 
			this->entity_status().statistics_try();

			// - call 'process_event_end'
			presult_event->result = this->process_event_end(time_now);

			// statistics) 
			if (presult_event->result == eRESULT::SUCCESS)
			{
				this->entity_status().statistics_succeeded();
			}

			// - transit message
			entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(static_cast<Ientity*>(this))));
		}
		catch(std::exception& /*e*/)
		{
			// - set result as 'eRESULT::EXCEPTION'
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}
		catch(...)
		{
			// - set result as 'eRESULT::EXCEPTION'
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			this->entity_status().statistics_failed();

			// event log)
			this->LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, std::string_view(this->name()));
		}

		// - append result log
		this->_append_result_log_post(presult_event);

		// - change state to 'eSTATE::DONE'
		this->entity_status().state = eSTATE::DONE;
	}

	// 4) stop on complete
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
