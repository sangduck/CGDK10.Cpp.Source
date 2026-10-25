#include "pch.h"

namespace CGDK
{

bool schedulable::Iexecutable::process_on_register()
{
	// 1) set next tick time ((max)() when it overflows : an interval of (max)() is never. it wrapped to a time in the past)
	auto tick_now = chrono::tick::now();
	this->m_tick_next = (this->m_tick_diff_execute > chrono::tick::duration::zero() && tick_now > (chrono::tick::max)() - this->m_tick_diff_execute) ? (chrono::tick::max)() : tick_now + this->m_tick_diff_execute;

	// 2) default executor
	if (this->has_executor() == false)
		this->executor(system_executor::executor());

	// return)
	return true;
}

void schedulable::Iexecutable::process_on_unregister()
{
	this->m_tick_next = (chrono::tick::max)();
}

bool schedulable::Iexecutable::process_schedule()
{
	// check) 
	if(chrono::tick::now() < this->m_tick_next)
		return false;

	// 1) execute
	try
	{
		this->process_execute(0, 0);
	}
	catch (...)
	{
	}

	// 2) set next tick
	this->process_advance_next_tick();

	// return)
	return true;
}


}