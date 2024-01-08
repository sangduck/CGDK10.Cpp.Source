#include "pch.h"

using namespace CGDK;


time_event::entity::entity() noexcept
{
	// 1) set entity type
	m_entity_setting.type = static_cast<uint64_t>(eTYPE::ENTITY);

	// 2) create log
	scoped_lock(m_plog_result)
	{
		m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::entity::entity(const std::string_view _name, int64_t _type)
{
	// 1) set name
	set_name(_name);
	
	// 2) entity setting
	entity_setting().type = _type;

	// 3) create log
	scoped_lock(m_plog_result)
	{
		m_plog_result = make_object<LOG_RECORD>();
	}
}

time_event::entity::~entity() noexcept
{
}


time_event::entity& time_event::entity::operator = (own_ptr<Ievent>&& _pevent)
{
	scoped_lock(m_vector_event)
	{
		// 1) unregister all 
		unregister_event_all();

		// 2) set 
		m_queue_event.push_back(_pevent);
		m_vector_event.push_back(std::move(_pevent));

		// 3) reset next time
		if (m_queue_event.empty() == false)
		{
			entity_status().time_next = m_queue_event.front()->event_status().time_next;
		}
		else
		{
			entity_status().reset_next_time();
		}
	}

	// return)
	return *this;
}

time_event::entity& time_event::entity::operator = (std::vector<own_ptr<Ievent>>&& _vector_event)
{
	scoped_lock(m_vector_event)
	{
		// 1) unregister all 
		unregister_event_all();

		// 2) register
		for (auto& iter : _vector_event)
		{
			register_event(std::move(iter));
		}

		// 3) clear
		_vector_event.clear();
	}

	// return)
	return *this;
}

time_event::entity& time_event::entity::operator += (std::vector<own_ptr<Ievent>>&& _vector_event)
{
	// 1) register
	for (auto& iter : _vector_event)
	{
		register_event(std::move(iter));
	}

	// 2) clear
	_vector_event.clear();

	// return) 
	return *this;
}

time_event::entity& time_event::entity::operator += (own_ptr<Ievent>&& _pevent)
{
	// 1) register
	register_event(std::move(_pevent));

	// return) 
	return *this;
}

time_event::entity& time_event::entity::operator -= (Ievent* _pevent)
{
	// 1)
	unregister_event(_pevent);

	// return) 
	return *this;
}

void time_event::entity::process_notify_next_time_changed(time_event::Ievent* _pevent)
{
	scoped_lock(m_vector_event)
	{
		// check)
		RETURN_IF(m_queue_event.empty());

		if (m_queue_event.at(0) == _pevent)
		{
			// - update next tiime
			m_entity_status.time_next = _pevent->event_status().time_next;

			// - notify
			time_event::entity_manager::instance()->process_notify_next_time_changed(this);
		}
	}
}

void time_event::entity::register_event(own_ptr<time_event::Ievent>&& _pevent)
{
	// 1) craete log object
	auto presult_event = _append_result_log("register event");

	try
	{
		// declare) 
		bool is_notify_to = false;

		scoped_lock(m_vector_event)
		{
			// 1) find event
			auto iter_find = std::find(m_vector_event.begin(), m_vector_event.end(), _pevent);

			// check) aleady exisit?
			THROW_IF(iter_find != m_vector_event.end(), throwable::failure(eRESULT::ALEADY_EXIST));

			// 2) preset
			_pevent->event_status().count_remained = _pevent->event_setting().count_times;
			_pevent->event_status().time_next = _pevent->event_setting().time_execute;

			// 3) reset
			_pevent->process_reset();

			// 4) add to queue
			_push_heap(_pevent);

			// check) 
			CGASSERT_ERROR(m_queue_event.empty() == false);

			// 5) notify_one
			if (_pevent == m_queue_event[0])
			{
				// - update next tiime
				m_entity_status.time_next = _pevent->event_status().time_next;

				// - 
				is_notify_to = true;
			}

			// 6) add pevent
			try
			{
				m_vector_event.push_back(std::move(_pevent));
			}
			catch (...)
			{
				_process_erase_event_from_heap(_pevent);

				// reraise) 
				throw;
			}
		}

		// 8) notify
		if (is_notify_to)
		{
			// - notify
			time_event::entity_manager::instance()->process_notify_next_time_changed(this);
		}

		// 9) apply log 
		_append_result_log_post(presult_event);

		// 10) notify message
		time_event::entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER, 0, reinterpret_cast<uintptr_t>(this)));
	}
	catch (throwable::failure& _e)
	{
		// - set log result
		presult_event->result = _e.reason();

		// - apply log 
		_append_result_log_post(presult_event);
	}
	catch (...)
	{
		// - set log result
		presult_event->result = eRESULT::EXCEPTION;

		// - apply log 
		_append_result_log_post(presult_event);
	}
}

void time_event::entity::unregister_event(time_event::Ievent* _pevent) noexcept
{
	// check)
	CGASSERT_ERROR(_pevent != nullptr);

	// check)
	RETURN_IF(_pevent == nullptr);

	// declare) 
	bool is_notify_to = false;

	// 1) craete log object
	auto presult_event = _append_result_log("unregister event");

	scoped_lock(m_vector_event)
	{
		// 2) find event
		auto iter_find = std::find(m_vector_event.begin(), m_vector_event.end(), _pevent);

		// check) 
		if (iter_find == m_vector_event.end())
		{
			// 3) erase entity
			m_vector_event.erase(iter_find);

			// 4) reorder heap
			is_notify_to = _process_erase_event_from_heap(_pevent);

			// 5) update next tiime
			if (is_notify_to == true)
			{
				m_entity_status.time_next = _pevent->event_status().time_next;
			}

			// 6) ...
			if (m_vector_event.empty())
			{
				// - set state as time_event::eSTATE::DONE
				entity_status().state = time_event::eSTATE::DONE;
			}

			// 7) set result to log
			presult_event->result = eRESULT::SUCCESS;
		}
		else
		{
			// - set result to log
			presult_event->result = eRESULT::NOT_EXIST;
		}
	}

	// 8) apply log 
	_append_result_log_post(presult_event);

	// 9) notify message
	time_event::entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER, 0, reinterpret_cast<uintptr_t>(this)));

	// 10) notify to entity_manager
	if (is_notify_to)
	{
		// - notify
		time_event::entity_manager::instance()->process_notify_next_time_changed(this);
	}
}

void time_event::entity::unregister_event_all() noexcept
{
	scoped_lock(m_vector_event)
	{
		// 1) clear
		m_vector_event.clear();
		m_queue_event.clear();

		// 2) ...
		entity_status().reset_next_time();
	}
}

bool _compare_event_function(time_event::Ievent* _left, time_event::Ievent* _right) noexcept
{
	// check)
	CGASSERT_ERROR(_left != nullptr)

	// check)
	CGASSERT_ERROR(_right != nullptr)

	return _left->event_status().time_next > _right->event_status().time_next;
}

intptr_t time_event::entity::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check) 
	RETURN_IF(get_object_state() != eOBJECT_STATE::RUNNING, 0);

	// 1) get now utc time
	auto time_now = chrono::time::utc::now();

	scoped_lock(m_vector_event)
	{
		// check) 
		CGASSERT_ERROR(m_queue_event.empty() == false);

		// check) 
		CGASSERT_ERROR(time_now >= m_queue_event.front()->event_status().time_next);
	}

	for(;;)
	{
		// declare) 
		object_ptr<time_event::Ievent> pevent_front;

		scoped_lock(m_vector_event)
		{
			// check) 
			if (m_queue_event.empty())
			{
				// - set state as time_event::eSTATE::DONE
				entity_status().state = time_event::eSTATE::DONE;

				// break)
				break;
			}

			// 2) get front entity
			pevent_front = m_queue_event.front();

			// check) 
			BREAK_IF(time_now < pevent_front->event_status().time_next);

			// 3) pop entity
			_pop_heap();
		}

		// 5) add log
		auto presult_event = _append_result_log("process_execute_function");

		try
		{
			// statistics) 
			entity_status().statistics_try();

			// 6) run 'process_execute'
			presult_event->result = pevent_front->process_event(time_now);

			// statistics) 
			if (presult_event->result == eRESULT::SUCCESS)
			{
				entity_status().statistics_succeeded();
			}

			// 7) set complete
			if (presult_event->result == eRESULT::COMPLETE)
			{
				// statistics) 
				entity_status().statistics_succeeded();

				// - set state 
				entity_status().state = time_event::eSTATE::DONE;
			}
			else if (presult_event->result == eRESULT::CANCEL)
			{
				// statistics) 
				entity_status().statistics_failed();

				// - set state 
				entity_status().state = time_event::eSTATE::DONE;
			}
		}
		catch (std::exception& e)
		{
			// - 결과를 Exception으로 설정
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			entity_status().statistics_failed();

			// event log)
			LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, name());

			// Exception 처리.
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}
		catch (...)
		{
			// - 결과를 Exception으로 설정
			presult_event->result = eRESULT::EXCEPTION;

			// statistics) 
			entity_status().statistics_failed();

			// event log)
			LOG_EVENT(nullptr, log_type_t::fatal, log_level_t::normal(), "exception [{}]"sv, name());
		}

		// 8) Result를 써넣는다.
		_append_result_log_post(presult_event);

		// 9) Message 날리기..
		time_event::entity_manager::instance()->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE, 0, reinterpret_cast<uintptr_t>(this)));

		// 10) [남은 실행 횟수]를 줄인다.
		if (pevent_front->event_status().count_remained > 0)
		{
			--pevent_front->event_status().count_remained;
		}

		// 11) [남은 실행 횟수]가 0이 아니면 다시 추가한다.
		if (pevent_front->event_setting().count_times == 0 
		 || pevent_front->event_status().count_remained != 0)
		{
			// - 실행한 Event에 대해 Interval만큼 증가시킨다.
			pevent_front->event_status().time_next += pevent_front->event_setting().time_interval;
			pevent_front->event_status().time_last_executed = time_now;

			scoped_lock(m_vector_event)
			{
				// - push heap
				_push_heap(pevent_front);
			}
		}

		// 12) set...
		if (m_queue_event.empty() == false)
		{
			// - set entity next time
			entity_status().time_next = m_queue_event.front()->event_status().time_next;
		}
	}

	// 13) stop if all done (state == eSTATE::DONE)
	if (entity_status().state != eSTATE::DONE)
	{
		entity_manager::instance()->_push_time_entity(this);
	}
	else
	{
		stop();
	}

	// return)
	return 0;
}

void time_event::entity::_process_start_request(context& _context)
{
	// 1) call parents's '_process_start_request'
	CGDK::object::Nstartable::_process_start_request(_context);
}

time_event::sENTITY_SETTING _get_parameter_entity_setting(context& _context);
time_event::sEVENT_SETTING _get_parameter_event_setting(std::string_view _name, context& _context);
std::vector<time_event::sEVENT_SETTING> _get_parameter_event_setting_list(context& _context);

result_code time_event::entity::_process_starting(context& _context)
{
	// 1) get param (entity setting)
	auto entity_setting = _get_parameter_entity_setting(_context);

	// 2) get param (event settings)
	auto vector_event_setting = _get_parameter_event_setting_list(_context);

	// 3) call parents's '_process_starting'
	CGDK::object::Nstartable::_process_starting(_context);

	// 4) set default setter
	if (m_entity_setting.setter == eSETTER::NONE)
	{
		m_entity_setting.setter = eSETTER::LOCAL;
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code time_event::entity::_process_start(context& _context)
{
	// 1) call parents's '_process_start'
	CGDK::object::Nstartable::_process_start(_context);

	// 2) register to...
	auto result = CGDK::time_event::entity_manager::instance()->register_entity(this);

	// 3) log
	auto presult_event = _append_result_log("start time_event entity");
	presult_event->result = (result) ? eRESULT::SUCCESS : eRESULT::FAIL;
	_append_result_log_post(presult_event);

	// check) 
	THROW_IF(result == false, throwable::failure(eRESULT::EXCEPTION));

	// return)
	return eRESULT::SUCCESS;
}

void time_event::entity::_process_stop_request() noexcept
{
	// 1) call parents's '_process_stop_request'
	CGDK::object::Nstartable::_process_stop_request();
}

result_code time_event::entity::_process_stopping() noexcept
{
	// 1) call parents's '_process_stopping'
	auto result = CGDK::object::Nstartable::_process_stopping();

	// 2) register to...
	CGDK::time_event::entity_manager::instance()->unregister_entity(this);

	// return)
	return result; 
}

result_code time_event::entity::_process_stop() noexcept
{
	// 1) call parents's '_process_stop'
	auto result = CGDK::object::Nstartable::_process_stop();

	// 2) log
	auto presult_event = _append_result_log("stop time_event entity");
	presult_event->result = eRESULT::SUCCESS;
	_append_result_log_post(presult_event);

	// return)
	return result;
}

void time_event::entity::_process_pause_request()
{
	Npausable::_process_pause_request();
}

void time_event::entity::_process_pauseing()
{
	Npausable::_process_pauseing();
}

void time_event::entity::_process_pause()
{
	Npausable::_process_pause();
}

void time_event::entity::_process_resume_request()
{
	Npausable::_process_resume_request();
}

void time_event::entity::_process_resuming()
{
	Npausable::_process_resuming();
}

void time_event::entity::_process_resume()
{
	Npausable::_process_resume();
}

void time_event::entity::_pop_heap() noexcept
{
	// 1) Pop-head한다.
	std::pop_heap(m_queue_event.begin(), m_queue_event.end(), _compare_event_function);

	// 2) 큐의 크기를 하나 줄인다.
	m_queue_event.pop_back();
}

void time_event::entity::_push_heap(time_event::Ievent* _pevent)
{
	// 1) 새로운 [Scheduled Event 객체]를 추가한다.
	m_queue_event.emplace_back(_pevent);

	// 2) Priority heap을 생성
	std::push_heap(m_queue_event.begin(), m_queue_event.end(), _compare_event_function);
}

bool time_event::entity::_process_erase_event_from_heap(time_event::Ievent* _pevent) noexcept
{
	// check)
	CGASSERT_ERROR(_pevent != nullptr);

	// check)
	RETURN_IF(_pevent == nullptr, false);

	// check)
	CGASSERT_ERROR(m_queue_event.empty() == false);

	// check)
	RETURN_IF(m_queue_event.empty(), false);

	if (m_queue_event.size() > 1)
	{
		// decalre)
		bool is_time_next_changed = false;

		// 1) find _pevent
		auto iter_find_queue = std::find(m_queue_event.begin(), m_queue_event.end(), _pevent);

		// check)
		CGASSERT_ERROR(iter_find_queue != m_queue_event.end());

		// check) not exist?
		RETURN_IF(iter_find_queue == m_queue_event.end(), false);

		// 2) is first?
		if (iter_find_queue == m_queue_event.begin())
		{
			is_time_next_changed = true;
		}

		// 2) erase event object from queue
		m_queue_event.erase(iter_find_queue);

		// 3) regeneration heap
		std::make_heap(m_queue_event.begin(), m_queue_event.end(), _compare_event_function);

		// 4) reset next time
		if (is_time_next_changed == true)
		{
			entity_status().time_next = m_queue_event.front()->event_status().time_next;
		}

		// return)
		return is_time_next_changed;
	}
	else
	{
		// check)
		CGASSERT_ERROR(m_queue_event[0] == _pevent)

		// - clear
		m_queue_event.erase(m_queue_event.begin());

		// - reset next time
		entity_status().reset_next_time();

		// return)
		return true;
	}
}
