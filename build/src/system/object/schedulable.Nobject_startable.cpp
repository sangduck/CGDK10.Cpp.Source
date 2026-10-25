#include "pch.h"

namespace CGDK
{

result_code schedulable::Ninitializable_startable::_process_initialize(context& _context)
{
	// 1) call 'on_initialize'
	this->on_initialize(_context);

	//return)
	return result_code(eRESULT::SUCCESS);
}

result_code schedulable::Ninitializable_startable::_process_start(context& _context)
{
	// 1) on_start
	this->on_start(_context);

	// 2) executor를  설정한다.
	if (this->has_executor() == false)
		this->executor(system_executor::executor());

	// 3) register
	system_executor::register_schedulable(this);

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code schedulable::Ninitializable_startable::_process_stopping() noexcept
{
	// 1) unregister
	system_executor::unregister_schedulable(this);

	try
	{
		// 2) call 'on_stopping'
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code schedulable::Ninitializable_startable::_process_destroying() noexcept
{
	// 1) reset executor (under m_lockable_pexecutor : executor() reads it from other threads. released outside the lock)
	object_ptr<Iexecutor> pexecutor_old;
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pexecutor);

		pexecutor_old = std::move(this->m_pexecutor);
	}
	pexecutor_old.reset();

	try
	{
		// 2) destroy
		this->on_destroying();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}


}