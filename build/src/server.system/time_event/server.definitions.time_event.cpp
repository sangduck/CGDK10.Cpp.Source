#include "pch.h"

using namespace CGDK;


std::string_view CGDK::to_string(time_event::eSETTER _setter, bool _upper_case) noexcept
{
	// declare) 
	std::string_view str_result;

	if (_upper_case)
	{
		switch (_setter)
		{
		case	time_event::eSETTER::SYSTEM:			str_result = "SYSTEM";				break;
		case	time_event::eSETTER::LOCAL:				str_result = "LOCAL";				break;
		case	time_event::eSETTER::ADMIN:				str_result = "ADMIN";				break;
		default:										str_result = "-";					break;
		};
	}
	else
	{
		switch (_setter)
		{
		case	time_event::eSETTER::SYSTEM:			str_result = "system";				break;
		case	time_event::eSETTER::LOCAL:				str_result = "local";				break;
		case	time_event::eSETTER::ADMIN:				str_result = "admin";				break;
		default:										str_result = "-";					break;
		};
	}

	// return) 
	return str_result;
}

std::string_view CGDK::to_string(time_event::eTYPE _state, bool _upper_case) noexcept
{
	std::string_view str_result;

	if (_upper_case)
	{
		switch (_state)
		{
		case	time_event::eTYPE::ONCE:				str_result = "ONCE";				break;
		case	time_event::eTYPE::ITERATION:			str_result = "ITERATION";			break;
		case	time_event::eTYPE::START_END:			str_result = "START_END";			break;
		case	time_event::eTYPE::START_ITERATION_END:	str_result = "START_ITERATION_END";	break;
		case	time_event::eTYPE::ENTITY:				str_result = "ENTITY";				break;
		case	time_event::eTYPE::CUSTOM:				str_result = "CUSTOM";				break;
		default:										str_result = "N/A";					break;
		};
	}
	else
	{
		switch (_state)
		{
		case	time_event::eTYPE::ONCE:				str_result = "once";				break;
		case	time_event::eTYPE::ITERATION:			str_result = "iteration";			break;
		case	time_event::eTYPE::START_END:			str_result = "start_end";			break;
		case	time_event::eTYPE::START_ITERATION_END:	str_result = "start_iteration_end";	break;
		case	time_event::eTYPE::ENTITY:				str_result = "entity";				break;
		case	time_event::eTYPE::CUSTOM:				str_result = "custom";				break;
		default:										str_result = "n/a";					break;
		};
	}

	// return)
	return str_result;
}

std::string_view CGDK::to_string(time_event::eSTATE _state, bool _upper_case) noexcept
{
	std::string_view str_result;

	if (_upper_case)
	{
		switch (_state)
		{
		case	time_event::eSTATE::WAIT:				str_result = "WAIT";				break;
		case	time_event::eSTATE::RUN:				str_result = "RUN";					break;
		case	time_event::eSTATE::DONE:				str_result = "DONE";				break;
		default:										str_result = "-";					break;
		};
	}
	else
	{
		switch (_state)
		{
		case	time_event::eSTATE::WAIT:				str_result = "wait";				break;
		case	time_event::eSTATE::RUN:				str_result = "run";					break;
		case	time_event::eSTATE::DONE:				str_result = "Done";				break;
		default:										str_result = "-";					break;
		};
	}

	// return) 
	return str_result;
}

CGDK::time_event::sEVENT_SETTING::sEVENT_SETTING(const sEVENT_SETTING& _copy) noexcept :
	name(_copy.name),
	type(_copy.type),
	option( _copy.option),
	time_execute(_copy.time_execute),
	time_interval(_copy.time_interval),
	count_times(_copy.count_times)
{}

CGDK::time_event::sEVENT_SETTING& CGDK::time_event::sEVENT_SETTING::operator = (const CGDK::time_event::sEVENT_SETTING& _rhs) noexcept
{
	name			 = _rhs.name;
	type			 = _rhs.type;
	option			 = _rhs.option;
	time_execute	 = _rhs.time_execute;
	time_interval	 = _rhs.time_interval;
	count_times		 = _rhs.count_times;

	return *this;
}

CGDK::time_event::sEVENT_STATUS::sEVENT_STATUS(const sEVENT_STATUS& _copy) noexcept :
	id(_copy.id),
	state(_copy.state.load()),
	time_last_executed(_copy.time_last_executed),
	time_next(_copy.time_next),
	count_remained(_copy.count_remained.load())
{
}

CGDK::time_event::sEVENT_STATUS& CGDK::time_event::sEVENT_STATUS::operator = (const CGDK::time_event::sEVENT_STATUS& _rhs) noexcept
{
	id					 = _rhs.id;
	state				 = _rhs.state.load();
	time_last_executed	 = _rhs.time_last_executed;
	time_next			 = _rhs.time_next;
	count_remained.store(_rhs.count_remained);

	return *this;
}

void CGDK::time_event::sEVENT_STATUS::reset() noexcept
{
	id					 = 0;
	state				 = eSTATE::NONE;
	time_last_executed	 = chrono::time::utc::time_point(chrono::time::duration::zero());
	time_next			 = chrono::time::utc::time_point(chrono::time::duration::zero());
	count_remained.store(0);
}

CGDK::time_event::sENTITY_SETTING::sENTITY_SETTING(const CGDK::time_event::sENTITY_SETTING& _copy) noexcept :
	name(_copy.name),
	type(_copy.type),
	setter(_copy.setter),
	level(_copy.level)
{}

CGDK::time_event::sENTITY_SETTING& CGDK::time_event::sENTITY_SETTING::operator=(const sENTITY_SETTING& _rhs) noexcept
{
	name				 = _rhs.name;
	type				 = _rhs.type;
	setter				 = _rhs.setter;
	level				 = _rhs.level;
		
	return *this;
}

CGDK::time_event::sENTITY_STATUS::sENTITY_STATUS(const CGDK::time_event::sENTITY_STATUS& _copy) noexcept :
	id(_copy.id),
	state(_copy.state.load()),
	time_setup(_copy.time_setup),
	time_last_try(_copy.time_last_try),
	time_last_succeeded(_copy.time_last_succeeded),
	time_last_failed(_copy.time_last_failed),
	time_next(_copy.time_next),
	count_try(_copy.count_try.load()),
	count_succeeded(_copy.count_succeeded.load()),
	count_failed(_copy.count_failed.load())
{}

CGDK::time_event::sENTITY_STATUS& CGDK::time_event::sENTITY_STATUS::operator=(const sENTITY_STATUS& _rhs) noexcept
{
	id					 = _rhs.id;
	state				 = _rhs.state.load();
	time_setup			 = _rhs.time_setup;
	time_last_try		 = _rhs.time_last_try;
	time_last_succeeded	 = _rhs.time_last_succeeded;
	time_last_failed	 = _rhs.time_last_failed;
	time_next			 = _rhs.time_next;
	count_try.store(_rhs.count_try);
	count_succeeded.store(_rhs.count_succeeded);
	count_failed.store(_rhs.count_failed);
		
	return *this;
}

void CGDK::time_event::sENTITY_STATUS::reset() noexcept
{
	id					 = 0;
	state				 = eSTATE::NONE;
	time_setup			 = chrono::time::utc::time_point(chrono::time::duration::zero());
	time_last_try		 = chrono::time::utc::time_point(chrono::time::duration::zero());
	time_last_succeeded	 = chrono::time::utc::time_point(chrono::time::duration::zero());
	time_last_failed	 = chrono::time::utc::time_point(chrono::time::duration::zero());
	time_next			 = chrono::time::utc::time_point(chrono::time::duration::zero());
	count_try.store(0);
	count_succeeded.store(0);
	count_failed.store(0);
}
