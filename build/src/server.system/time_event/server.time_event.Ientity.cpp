#include "pch.h"

using namespace CGDK;


time_event::sENTITY_SETTING _get_parameter_entity_setting(context& _context)
{
	// declare) 
	time_event::sENTITY_SETTING entity_setting;

	for (auto& iter : _context.map_nodes)
	{
		if (iter.first == "type")
		{
			entity_setting.type = (uint64_t)iter.second;
		}
		else if (iter.first == "setter")
		{
			entity_setting.setter = static_cast<time_event::eSETTER>((int)iter.second);
		}
		else if (iter.first == "level")
		{
			entity_setting.level = (int)iter.second;
		}
	}

	// return)
	return entity_setting;
}

//-----------------------------------------------------------------------------
// time settings in a context
//-----------------------------------------------------------------------------
static bool _is_space(char _ch) noexcept { return _ch == ' ' || _ch == '\t' || _ch == '\r' || _ch == '\n'; }
static bool _is_digit(char _ch) noexcept { return _ch >= '0' && _ch <= '9'; }
static bool _is_alpha(char _ch) noexcept { return (_ch >= 'a' && _ch <= 'z') || (_ch >= 'A' && _ch <= 'Z'); }
static std::string_view _trim(std::string_view _string) noexcept
{
	while (_string.empty() == false && _is_space(_string.front())) _string.remove_prefix(1);
	while (_string.empty() == false && _is_space(_string.back())) _string.remove_suffix(1);
	return _string;
}
static bool _equal_no_case(std::string_view _lhs, std::string_view _rhs) noexcept
{
	if (_lhs.size() != _rhs.size())
		return false;
	for (size_t i = 0; i < _lhs.size(); ++i)
	{
		auto l = _lhs[i]; if (l >= 'A' && l <= 'Z') l = static_cast<char>(l - 'A' + 'a');
		auto r = _rhs[i]; if (r >= 'A' && r <= 'Z') r = static_cast<char>(r - 'A' + 'a');
		if (l != r) return false;
	}
	return true;
}

bool time_event::parse_duration(std::string_view _string, chrono::time::duration& _duration) noexcept
{
	// declare)
	auto str = _trim(_string);
	auto total = std::chrono::duration<double>::zero();
	size_t pos = 0;

	// check)
	if (str.empty())
		return false;

	// 1) "<number>[unit] <number>[unit] ..."
	while (pos < str.size())
	{
		// - number (integer or decimal)
		size_t pos_begin = pos;
		while (pos < str.size() && (_is_digit(str[pos]) || str[pos] == '.')) ++pos;

		// check) no number
		if (pos == pos_begin)
			return false;

		std::string str_number(str.substr(pos_begin, pos - pos_begin));
		char* pend = nullptr;
		double value = std::strtod(str_number.c_str(), &pend);

		// check) invalid number
		if (pend == str_number.c_str() || *pend != '\0')
			return false;

		// - unit (none : seconds)
		while (pos < str.size() && _is_space(str[pos])) ++pos;
		size_t pos_unit = pos;
		while (pos < str.size() && _is_alpha(str[pos])) ++pos;
		auto unit = str.substr(pos_unit, pos - pos_unit);

		double seconds;
		if (unit.empty() || _equal_no_case(unit, "s") || _equal_no_case(unit, "sec") || _equal_no_case(unit, "second") || _equal_no_case(unit, "seconds"))
			seconds = 1.0;
		else if (_equal_no_case(unit, "ms") || _equal_no_case(unit, "msec"))
			seconds = 0.001;
		else if (_equal_no_case(unit, "m") || _equal_no_case(unit, "min") || _equal_no_case(unit, "minute") || _equal_no_case(unit, "minutes"))
			seconds = 60.0;
		else if (_equal_no_case(unit, "h") || _equal_no_case(unit, "hour") || _equal_no_case(unit, "hours"))
			seconds = 3600.0;
		else if (_equal_no_case(unit, "d") || _equal_no_case(unit, "day") || _equal_no_case(unit, "days"))
			seconds = 86400.0;
		else
			return false;

		total += std::chrono::duration<double>(value * seconds);

		// - next
		while (pos < str.size() && _is_space(str[pos])) ++pos;
	}

	// return)
	_duration = std::chrono::duration_cast<chrono::time::duration>(total);
	return true;
}

bool time_event::parse_time_point(std::string_view _string, chrono::time::utc::time_point& _time_point) noexcept
{
	// declare)
	auto str = _trim(_string);

	// check)
	if (str.empty())
		return false;

	// 1) "now", "now + <duration>", "now - <duration>"
	if (str.size() >= 3 && _equal_no_case(str.substr(0, 3), "now"))
	{
		auto time_now = chrono::time::utc::now();
		auto rest = _trim(str.substr(3));

		// - "now"
		if (rest.empty())
		{
			_time_point = time_now;
			return true;
		}

		// check) +/-
		if (rest.front() != '+' && rest.front() != '-')
			return false;

		// - duration
		chrono::time::duration duration;
		if (parse_duration(rest.substr(1), duration) == false)
			return false;

		_time_point = (rest.front() == '+') ? chrono::time::utc::time_point(time_now + duration) : chrono::time::utc::time_point(time_now - duration);
		return true;
	}

	// 2) date time string ("YYYY-MM-DD hh:mm:ss.fffffffff")
	auto date_time_parsed = chrono::from_string_to_date_time(std::string(str));

	// check) not a date time (the fields not parsed are 0 : an invalid string has month 0 and day 0)
	if (date_time_parsed.t_month < 1 || date_time_parsed.t_month > 12
	 || date_time_parsed.t_day < 1 || date_time_parsed.t_day > 31
	 || date_time_parsed.t_hour > 23 || date_time_parsed.t_minute > 59 || date_time_parsed.t_second > 60)
		return false;

	auto time_point = chrono::from_date_time<chrono::time::utc::time_point>(date_time_parsed);

	// return)
	_time_point = time_point;
	return true;
}

time_event::sEVENT_SETTING _get_parameter_event_setting(std::string_view _name, context& _context)
{
	// declare) 
	time_event::sEVENT_SETTING event_setting;

	// 1) set name
	event_setting.name = _name;

	// 2) ...
	for (auto& iter : _context.map_nodes)
	{
		if (iter.first == "type")
		{
			event_setting.type = static_cast<time_event::eTYPE>((int)iter.second);
		}
		else if (iter.first == "option")
		{
			event_setting.option = (uint64_t)iter.second;
		}
		else if (iter.first == "time_execute")
		{
			std::string str_temp = iter.second;
			chrono::time::utc::time_point time_execute;
			if (time_event::parse_time_point(str_temp, time_execute))
				event_setting.time_execute = time_execute;
		}
		else if (iter.first == "time_interval")
		{
			std::string str_temp = iter.second;
			chrono::time::duration time_interval;
			if (time_event::parse_duration(str_temp, time_interval))
				event_setting.time_interval = time_interval;
		}
		else if (iter.first == "count_times")
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
	auto iter = _context.map_nodes.begin();
	auto iter_end = _context.map_nodes.end();

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

		SCOPED_LOCK(this->m_lockable_plog_result)
		{
			// check) 
			RETURN_IF(this->m_plog_result.empty());

			// check) 
			RETURN_IF(this->m_plog_result->sub_plog().empty());

			// 1) 제일 마지막의 SubEvent객체를 얻어 온다.
			auto presult_event = m_plog_result->sub_plog().back();

			// 2) alloc LOG_RECORD object
			plog_sub = make_object<LOG_RECORD>();

			// 3) set log info
			plog_sub->type = _type;
			plog_sub->level = _level;
			plog_sub->set_message(_message, _char_code);
			plog_sub->time_occurred = presult_event->time_occurred;

			// 3) add LOG_RECORD object
			presult_event->add_sub_log(object_ptr<LOG_RECORD>(plog_sub));
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
		SCOPED_LOCK(this->m_lockable_plog_result)
		{
			// 1) allocate new LOG_RECORD object
			pevent_result = make_object<LOG_RECORD>();

			// 2) set log info
			pevent_result->type = log_type_t::undefined;
			pevent_result->level = log_level_t::normal();
			pevent_result->set_message(_message);
			pevent_result->time_occurred = chrono::time::utc::now();

			// 4) 새로운 Result 객체를 추가한다.
			this->m_plog_result->add_sub_log(object_ptr<LOG_RECORD>(pevent_result));

			// check) 보관하는 Result의 객체가 한계를 넘을 경우 제일 앞의 Result를 하나 제거한다.
			if (this->m_plog_result->sub_plog().size() > 256)
			{
				this->m_plog_result->remove_front_sub_log();
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

	SCOPED_LOCK(this->m_lockable_plog_result)
	{
		// check) ...
		CGASSERT_ERROR(_plog_result->buf_message.size() > sizeof(COUNT_T));

		// 1)  reserve 
		_plog_result->buf_message.reserve(_plog_result->buf_message.size() + 64);

		// 2)remove 'NULL'
		_plog_result->buf_message.subtract(sizeof(char));

		// 3) ...
		_plog_result->buf_message.append_text(" ["sv);
		_plog_result->buf_message.append_text(to_string(_plog_result->result.value<eRESULT>()));
		_plog_result->buf_message.append_text("]"sv);

		// 4) ...
		_plog_result->buf_message.append<char>(0);

		// 5) message
		_plog_result->buf_message.front<COUNT_T>() = static_cast<COUNT_T>(_plog_result->buf_message.size() - sizeof(COUNT_T));
	}
}

