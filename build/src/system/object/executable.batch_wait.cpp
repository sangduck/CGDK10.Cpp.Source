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

	// declare) 
	CONDITIONAL_LOCK(m_lockable_complete, lock_wait)
	{
		// 1) wait한다.
		auto result = m_cv_complete.wait_for(lock_wait, _ticks_wait);

		// check) Timeout이면 false를 리턴한다.
		if(result == std::cv_status::timeout)
			return false;
	}

	// return) 성공!
	return true;
}

void executable::batch_wait::process_complete_execution()
{
	m_cv_complete.notify_one();
}



}