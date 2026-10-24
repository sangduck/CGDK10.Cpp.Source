#include "pch.h"

#ifdef _WIN32
namespace CGDK
{

updatable::executable_batch::executable_batch(size_t _count_thread) noexcept
{
	this->process_initialize(_count_thread);
}

updatable::executable_batch::~executable_batch() noexcept
{
	this->process_destroy();
}

bool updatable::executable_batch::process_initialize(size_t _count_thread)
{
	// 1) iThread가 AUTO면 Core수에 따라 Thread개수를 결정한다.
	if(_count_thread == 0)
	{
		_count_thread = api::get_core_count();
	}

	// 2) 최대 동시 실행 개수를 설정한다.
	this->m_max_thread_count = _count_thread;

	// return)
	return true;
}

bool updatable::executable_batch::process_destroy() noexcept
{
	return true;
}

void updatable::executable_batch::queue_updatable(object::Iupdatable* _pupdatable)
{
	this->m_list_updatable.push_back(_pupdatable);
}

result_code updatable::executable_batch::process_update(context& _context)
{
	// check) 
	RETURN_IF(this->m_list_updatable.empty(), eRESULT::SUCCESS);
	
	// 1) Rendering Contex 저장
	this->m_context = _context;

	// 2)
	this->m_count = this->m_max_thread_count;
	
	// 3) default executor를 얻는다.
	auto pexecutor = system_executor::executor();

	// 4) Executable의 개수만큼 executor를 실행을 건다.
	for(size_t i = 0; i < this->m_max_thread_count; ++i)
	{
		pexecutor->post(this);
	}

	// 5) 모두 끝날 때까지 기다린다.
	//    (조건식으로 기다린다 : 먼저 끝나서 notify를 놓치거나(lost wakeup) 이유 없이 깨어나도(spurious) 맞게 동작한다.)
	{
		std::unique_lock lock(this->m_lockable_complete);
		this->m_cv_complete.wait(lock, [this] { return this->m_count.load(std::memory_order_acquire) == 0; });
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}

intptr_t updatable::executable_batch::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// declare) 
	for(;;)
	{
		// declare)
		object_ptr<object::Iupdatable> pUpdatable;

		{
			// lock)
			std::lock_guard lock(this->m_lockable_list_updatable);

			// check) 더 이상 없으면 그냥 끝~
			BREAK_IF(this->m_list_updatable.empty());

			// 1) Pop을 하나 해온다.
			pUpdatable = this->m_list_updatable.front();
			this->m_list_updatable.pop_front();
		}

		// 2) 실행한다.
		pUpdatable->process_update(m_context);
	}

	// 3) 모든 Executable이 실행되면 Signal시킨다.
	//    (lock 안에서 notify한다 : 기다리는 쪽이 조건을 확인한 뒤 wait에 들어가기 전에 notify가 지나가지 않도록)
	if(this->m_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
	{
		std::lock_guard lock(this->m_lockable_complete);
		this->m_cv_complete.notify_one();
	}

	// return) Success!
	return 0;
}


}
#endif