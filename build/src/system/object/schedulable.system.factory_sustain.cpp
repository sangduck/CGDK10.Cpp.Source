#include "pch.h"

namespace CGDK
{

#ifdef _WIN32
#if !defined(_MSC_VER)
BOOL GetProcessMemoryInfo(CGHANDLE /*Process*/, PPROCESS_MEMORY_COUNTERS ppsmemCounters, uint32_t cb)
{
	// 1) Linux용은 그냥 Default...
	ppsmemCounters->PrivateUsage = 0;

	// return)
	return TRUE;
}
#endif
#endif

schedulable::system::factory_sustain::factory_sustain() noexcept :
	Nschedulable()
{
	// 1) set object system
	this->set_object_type(eOBJECT_TYPE::SCHEDULABLE_FACTORY_GARBAGE);

	// 2) set schedulable type
	this->m_statistics_schedulable.type = eSCHEDULABLE_TYPE::SYSTEM;
}

schedulable::system::factory_sustain::~factory_sustain() noexcept
{
	// check) 
	CGASSERT_ERROR(this->m_pmanager_factory.empty());
}

bool schedulable::system::factory_sustain::process_on_register()
{
	// 1) set factory manager 
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager_factory);

		this->m_pmanager_factory = factory::manager::instance();
	}

	// 2) set next tick time
	this->m_tick_next = chrono::tick::now();

	// return)
	return true;
}

void schedulable::system::factory_sustain::process_on_unregister()
{
	// lock)
	std::lock_guard lock(this->m_lockable_pmanager_factory);

	// 1) reset factory manager
	this->m_pmanager_factory.reset();
}

bool schedulable::system::factory_sustain::process_schedule()
{
	// 1) 현재의 Tick을 얻는다.
	auto tick_now = chrono::tick::now();

	// check) Next Tick보다 현재 tick이 크면 그냥 되돌린다.
	if(tick_now < this->m_tick_next)
		return false;

	// declare) 
	object_ptr<factory::manager> pmanager_factory;

	// 2) factory_manager를 복사한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmanager_factory);

		pmanager_factory = this->m_pmanager_factory;
	}

	// check) 
	if(pmanager_factory.empty())
		return true;

	// 3) sustain을 수행한다.
	pmanager_factory->sustain();

	// 4) Tick을 Update한다.
	this->process_advance_next_tick();

	// return)
	return true;
}


}