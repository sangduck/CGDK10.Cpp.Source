#include "pch.h"

namespace CGDK
{

executable::batch_wait::~batch_wait() noexcept
{
}

bool executable::batch_wait::wait_execute_completion(chrono::tick::duration _ticks_wait)
{
	// check) Queued된 개수가 0개이면 바로 끝낸다.
	if(executing_count() == 0)
		return true;

	// - this thread blocks (a CGDK thread : scoped_thread_block. the slot of runnext and the deferred sends go out : a worker
	//   waiting for the executables it posted would never see them run)
	std::optional<scoped_thread_block> thread_block;

	if (api::get_current_thread() != nullptr)
		thread_block.emplace();

	// lock)
	std::unique_lock<std::mutex> lock_wait(this->m_lockable_complete);

	// 1) wait한다. (the count is checked under the lock : the last completion notifies under it. (max)() : no timeout, the
	//    wait_for of the maximum overflowed)
	const auto is_completed = [this]() { return this->executing_count() == 0; };

	if (_ticks_wait == (chrono::tick::duration::max)())
	{
		this->m_cv_complete.wait(lock_wait, is_completed);

		// return) 성공!
		return true;
	}

	// return) false : timeout
	return this->m_cv_complete.wait_for(lock_wait, _ticks_wait, is_completed);
}

void executable::batch_wait::process_complete_execution()
{
	// - under the lock : a waiter between its check and its wait does not miss it
	std::lock_guard<std::mutex> lock(this->m_lockable_complete);

	this->m_cv_complete.notify_all();
}



}