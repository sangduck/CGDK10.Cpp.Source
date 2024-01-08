#include "pch.h"

using namespace CGDK;


time_event::sENTITY_SETTING _get_parameter_entity_setting(context& _context)
{
	// declare) 
	time_event::sENTITY_SETTING entity_setting;

	for (auto& iter : _context.map_node)
	{
		if (iter.first == "type"sv)
		{
			entity_setting.type = (uint64_t)iter.second;
		}
		else if (iter.first == "setter"sv)
		{
			entity_setting.setter = static_cast<time_event::eSETTER>((int)iter.second);
		}
		else if (iter.first == "level"sv)
		{
			entity_setting.level = (int)iter.second;
		}
	}

	// return)
	return entity_setting;
}

time_event::sEVENT_SETTING _get_parameter_event_setting(std::string_view _name, context& _context)
{
	// declare) 
	time_event::sEVENT_SETTING event_setting;

	// 1) set name
	event_setting.name = _name;

	// 2) ...
	for (auto& iter : _context.map_node)
	{
		if (iter.first == "type"sv)
		{
			event_setting.type = static_cast<time_event::eTYPE>((int)iter.second);
		}
		else if (iter.first == "option"sv)
		{
			event_setting.option = (uint64_t)iter.second;
		}
		else if (iter.first == "time_execute"sv)
		{
			event_setting.time_execute = from_string<chrono::time::utc::time_point>((std::string)iter.second);
		}
		else if (iter.first == "time_interval"sv)
		{
			event_setting.time_interval = chrono::time::duration(__from_string<uint64_t>((std::string)iter.second));
		}
		else if (iter.first == "count_times"sv)
		{
			event_setting.count_times = (int)iter.second;
		}
	}

	// return)
	return event_setting;
}

std::vector<time_event::sEVENT_SETTING> _get_parameter_event_setting_list(context& _context)
{
	// declare) 
	std::vector<time_event::sEVENT_SETTING> vector_event_setting;

	// 1) ...
	auto iter = _context.map_node.begin();
	auto iter_end = _context.map_node.end();

	for (;iter != iter_end; ++iter)
	{
		// 2) parsing event_setting
		auto event_setting = _get_parameter_event_setting(iter->first, iter->second);

		// 3) push_back event_setting
		vector_event_setting.push_back(event_setting);
	}

	// return)
	return vector_event_setting;
}

void time_event::Ientity::set_next_time(chrono::time::utc::time_point _time_next) noexcept
{
	// check)
	RETURN_IF(_time_next == m_entity_status.time_next);

	// 1) change
	m_entity_status.time_next = _time_next;

	// 2) notify
	process_notify_update_next_time();
}

void time_event::Ientity::process_notify_update_next_time()
{
	// check) 
	RETURN_IF(m_entity_status.state == eSTATE::RUN, );

	// 1) notify to manager
	time_event::entity_manager::instance()->process_notify_next_time_changed(this);
}

void time_event::Ientity::LOG_EVENT(Ilogger* _plog_target, log_type_t _type, log_level_t _level, eCHARACTER_SET _char_code, std::string_view _message) noexcept
{
	// check) _format shouldn't be nullptr
	CGASSERT_ERROR(_message.empty() == false);

	// check) _format shouldn't be nullptr
	RETURN_IF(_message.empty());

	try
	{
		//  declare) 
		object_ptr<LOG_RECORD> plog_sub;

		scoped_lock(m_plog_result)
		{
			// check) 
			RETURN_IF(m_plog_result.empty());

			// check) 
			RETURN_IF(m_plog_result->sub_plog.empty());

			// 1) 제일 마지막의 SubEvent객체를 얻어 온다.
			auto presult_event = m_plog_result->sub_plog.back();

			// 2) alloc LOG_RECORD object
			plog_sub = make_object<LOG_RECORD>();

			// 3) set log info
			plog_sub->type = _type;
			plog_sub->level = _level;
			plog_sub->set_message(_message, _char_code);
			plog_sub->time_occure = presult_event->time_occure;

			// 3) add LOG_RECORD object
			presult_event->sub_plog.push_back(plog_sub);
		}

		// Log)
		_log(_plog_target, plog_sub);
	}
	catch (...)
	{
	}
}

object_ptr<LOG_RECORD> time_event::Ientity::_append_result_log(std::string_view _message) noexcept
{
	// check) _message shouldn't be nullptr
	CGASSERT_ERROR(_message.empty() == false);

	// check) _message shouldn't be nullptr
	RETURN_IF(_message.empty(), object_ptr<LOG_RECORD>());

	// declare)
	object_ptr<LOG_RECORD> pevent_result;

	try
	{
		scoped_lock(m_plog_result)
		{
			// 1) allocate new LOG_RECORD object
			pevent_result = make_object<LOG_RECORD>();

			// 2) set log info
			pevent_result->type = log_type_t::undefined;
			pevent_result->level = log_level_t::normal();
			pevent_result->set_message(_message);
			pevent_result->time_occure = chrono::time::utc::now();

			// 4) 새로운 Result 객체를 추가한다.
			m_plog_result->sub_plog.push_back(pevent_result);

			// check) 보관하는 Result의 객체가 한계를 넘을 경우 제일 앞의 Result를 하나 제거한다.
			if (m_plog_result->sub_plog.size() > 256)
			{
				m_plog_result->sub_plog.pop_front();
			}
		}
	}
	catch (...)
	{
	}

	// return) 
	return pevent_result;
}

void time_event::Ientity::_append_result_log_post(LOG_RECORD* _plog_result)
{
	// check) _plog_result shouldn't be nullptr
	CGASSERT_ERROR(_plog_result != nullptr);

	// check) _plog_result shouldn't be nullptr
	RETURN_IF(_plog_result == nullptr);

	scoped_lock(m_plog_result)
	{
		// check) ...
		CGASSERT_ERROR(_plog_result->buf_message.size() > sizeof(COUNT_T));

		// 1)  reserve 
		_plog_result->buf_message.reserve(_plog_result->buf_message.size() + 64);

		// 2)remove 'NULL'
		_plog_result->buf_message.subtract(sizeof(char));

		// 3) ...
		_plog_result->buf_message.append_text(" ["sv);
		_plog_result->buf_message.append_text(to_string<char>(_plog_result->result.value<eRESULT>()));
		_plog_result->buf_message.append_text("]"sv);

		// 4) ...
		_plog_result->buf_message.append<char>(0);

		// 5) message
		_plog_result->buf_message.front<COUNT_T>() = static_cast<COUNT_T>(_plog_result->buf_message.size() - sizeof(COUNT_T));
	}
}

