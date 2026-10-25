#include "pch.h"
#include "platform.h"

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
	if(pmanager.empty())
		return;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager);

		// 1) register factory manager
		auto result = pmanager->process_register_executor(this);

		// check) 
		if(result == false)
			return;

		// 2) set manager
		this->m_pmanager = pmanager;
	}
}

void Iexecutor::process_unregister_executor() noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_manager);

	// check) 
	if(this->m_pmanager.empty())
		return;

	// 1) unregister executor
	this->m_pmanager->process_unregister_executor(this);

	// 2) reset manager
	this->m_pmanager.reset();
}

void Iexecutor::process_idle() noexcept
{
	// 1) tls garbage collection
	if (factory::manager::m_s_sustain_serial_now != factory::manager::m_s_sustain_serial)
		factory::manager::sustain_tls();
}

scoped_thread_block::scoped_thread_block()
{
	// - send defer : sends the sends deferred in a group lock before blocking, and does not defer them until unblocked.
	//   (a sync query sends its request and waits for the response here : a deferred request would never be answered)
	send_defer::suspend();

	// - runnext : the slot of this thread goes to the shared queue and is not used until unblocked (an executable posted before
	//   or while blocking is run by the other workers : a sync query waiting for one would never be answered)
	executor::io_context::runnext_suspend();

	// 1) get thread
	auto current_thread = api::get_current_thread();

	// check) 
	if (current_thread == nullptr)
	{
		assert(false);
		return;
	}

	// 2) get executor
	auto pexecutor = current_thread->m_pexecutor;

	// check) 
	if (pexecutor == nullptr)
		pexecutor = system_executor::executor();

	// 3) set executor
	this->m_pexecutor = pexecutor;

	// 4) block thread
	pexecutor->thread_blocked();
}

scoped_thread_block::~scoped_thread_block() noexcept
{
	// - send defer
	send_defer::resume();

	// - runnext
	executor::io_context::runnext_resume();

	// 1) get executor
	auto pexecutor = this->m_pexecutor;

	// check) 
	if (pexecutor == nullptr)
		return;

	// 2) unblock thread
	pexecutor->thread_unblocked();
}



}
