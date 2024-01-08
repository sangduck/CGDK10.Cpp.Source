#include "pch.h"

namespace CGDK
{

void get_parameter_execute_interval (schedulable::Iexecutable* _pschedulable, const context& _context)
{
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
	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
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
	if (_context.type() == eCONTEXT_VALUE_TYPE::BUFFER_)
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