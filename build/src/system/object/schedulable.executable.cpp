#include "pch.h"

namespace CGDK
{

void get_parameter_execute_interval (schedulable::Iexecutable* _pschedulable, const context& _context)
{
	// - "overrun" ("catch_up" / "skip")
	if (const auto& value_overrun = _context.find("overrun"sv, "overrun_policy"sv); value_overrun.is_valid())
	{
		std::string str_value = value_overrun;

		if (CGDK::__stricmp(str_value.c_str(), "skip") == 0)
			_pschedulable->overrun_policy(eSCHEDULE_OVERRUN::SKIP);
		else if (CGDK::__stricmp(str_value.c_str(), "catch_up") == 0 || CGDK::__stricmp(str_value.c_str(), "catchup") == 0)
			_pschedulable->overrun_policy(eSCHEDULE_OVERRUN::CATCH_UP);
		else
		{
			// log) 
			LOG_WARNING << "  (!) invalid parameter['overrun']: '"sv << str_value << "' (ignored)"sv;
		}

		// log) 
		LOG_INFO << "  + parameter['overrun']: "sv << to_string(_pschedulable->overrun_policy());
	}

	// - "interval" (milliseconds)
	if (const auto& value_interval = _context.find("interval"sv, "interval_ms"sv); value_interval.is_valid())
	{
		std::string str_value = value_interval;

		int64_t interval_ms = -1;
		try { interval_ms = std::stoll(str_value); } catch (...) {}

		if (interval_ms > 0)
		{
			// - set interval
			_pschedulable->execute_interval(std::chrono::milliseconds(interval_ms));

			// log) 
			LOG_INFO << "  + parameter['interval']: "sv << interval_ms << "ms"sv;
		}
		else
		{
			// log) 
			LOG_WARNING << "  (!) invalid parameter['interval']: '"sv << str_value << "' (ignored)"sv;
		}

		// return) 'interval' has priority over 'tick'
		return;
	}

	// - "tick" (chrono::tick::duration count)
	// - "tick"키에 대응하는 값을 얻는다.
	auto pvalue_tick = _context["tick"sv];

	// - "tick"키에 대응하는 값을 얻었다면...
	if (pvalue_tick.is_valid())
	{
		// - get tick value
		chrono::tick::duration tick_gap{ (chrono::tick::duration::rep)pvalue_tick };

		// - set tick
		_pschedulable->execute_interval(tick_gap);

		// log) 
		LOG_INFO << "  + parameter['tick']: "sv << tick_gap.count();
	}
}

result_code schedulable::executable::_process_initialize(context& _context)
{
	// declare) 
	START_PARAMETER parameter;

	// 1A) 
	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - set interval
		if (parameter.tick != chrono::tick::duration::zero())
		{
			this->execute_interval(parameter.tick);
		}
	}
	// 1B)
	else
	{
		get_parameter_execute_interval(this, _context);
	}

	// 2) ...
	return this->schedulable::Ninitializable_startable::_process_initialize(_context);
}

result_code schedulable::executable::_process_start(context& _context)
{
	// declare) 
	START_PARAMETER parameter;

	// 1A) 
	if (_context.type() == context::value_type_t::buffer_)
	{
		// - get parameter
		parameter = _context.v.value_buffer.front<START_PARAMETER>();

		// - set interval
		if (parameter.tick != chrono::tick::duration::zero())
		{
			this->execute_interval(parameter.tick);
		}
	}
	// 1B)
	else
	{
		get_parameter_execute_interval(this, _context);
	}

	// 2) ...
	return this->schedulable::Ninitializable_startable::_process_start(_context);
}


}