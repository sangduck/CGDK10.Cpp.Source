#include "pch.h"

namespace CGDK
{

bool schedulable::Iexecutable::process_on_register()
{
	// 1) set next tick time
	this->m_tick_next = chrono::tick::now() + this->m_tick_diff_execute;

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