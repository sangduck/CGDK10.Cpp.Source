#include "pch.h"

namespace CGDK
{

void Iexecutor::process_reset_statistics() noexcept
{
	// 1) reset object statistics
	this->m_statistics.now.count_execute = 0;
}

void Iexecutor::process_register_executor()
{
	// declare) 
	object_ptr<executor::manager> pmanager = executor::manager::instance();

	// check) pmanager
	CGASSERT_ERROR(pmanager.exist());

	// check)
	RETURN_IF(pmanager.empty());

	scoped_lock(this->m_lock_manager)
	{
		// 1) register factory manager
		auto result = pmanager->process_register_executor(this);

		// check) 
		RETURN_IF(result == false);

		// 2) set manager
		this->m_pmanager = pmanager;
	}
}

void Iexecutor::process_unregister_executor() noexcept
{
	scoped_lock(this->m_lock_manager)
	{
		// check) 
		RETURN_IF(this->m_pmanager.empty());

		// 1) unregister executor
		this->m_pmanager->process_unregister_executor(this);

		// 2) reset manager
		this->m_pmanager.reset();
	}
}

void Iexecutor::process_idle() noexcept
{
	// 1) tls garbage collection
	if (factory::manager::m_s_sustain_serial_now != factory::manager::m_s_sustain_serial)
	{
		factory::manager::sustain_tls();
	}
}


}
