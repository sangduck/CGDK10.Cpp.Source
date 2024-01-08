#include "pch.h"

namespace CGDK
{


schedulable::system::exception_observer::exception_observer() noexcept
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::SCHEDULABLE_EXCEPTION_OBSERVER);

	// 2) set schedulable type
	this->m_statistics_schedulable.type = eSCHEDULABLE_TYPE::SYSTEM;
}

schedulable::system::exception_observer::~exception_observer() noexcept
{
	// check) 
	CGASSERT_ERROR(this->m_pmanager_dump.empty());
}

bool schedulable::system::exception_observer::process_on_register()
{
	// 1) set dump manager
	scoped_lock(this->m_pmanager_dump)
	{
		this->m_pmanager_dump = dump_manager::instance();
	}

	// 2) set next tick time
	this->m_tick_next = chrono::tick::now();

	// return)
	return true;
}

void schedulable::system::exception_observer::process_on_unregister()
{
	// 1) reset dump manager
	scoped_lock(this->m_pmanager_dump)
	{
		this->m_pmanager_dump.reset();
	}
}

bool schedulable::system::exception_observer::process_schedule()
{
	// 1) 현재의 Tick을 얻는다.
	auto tick_now = chrono::tick::now();

	// check) Next Tick보다 현재 tick이 크면 그냥 되돌린다.
	RETURN_IF(tick_now < this->m_tick_next, false);

	// declare) 
	object_ptr<dump_manager> pmanager_exception;

	// 2) Pool Manager를 복사한다.
	scoped_lock(this->m_pmanager_dump)
	{
		pmanager_exception = this->m_pmanager_dump;
	}

	// check) 
	RETURN_IF(pmanager_exception.empty(), true);

	// 3) Object Pool을 sustain한다.
	pmanager_exception->dispatch_exception_message_all();

	// 4) Tick을 Update한다.
	this->m_tick_next = this->m_tick_next + this->m_tick_diff_execute;

	// return)
	return true;
}


}