#include "pch.h"


namespace CGDK
{

api::queue_executable::queue_executable()
{
}

api::queue_executable::~queue_executable() noexcept
{
}

bool api::queue_executable::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable == nullptr, false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.bytes_transfered = _bytes_transferred;
		result.pexecutable = _pexecutable;
		result.completion_key = _completion_key;

		// 2) Post한다.
		{
			// lock)
			std::lock_guard lock(m_lockable_complete);

			// - Queueing한다.
			m_queue_executable.push_back(result);

			// - add_ref한다.
			_pexecutable->add_ref();
		}

		// 3) Signal시킨다.
		m_cv_complete.notify_one();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return)
	return true;
}

bool api::queue_executable::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable.empty(), false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.bytes_transfered = _bytes_transferred;
		result.pexecutable = _pexecutable.get();
		result.completion_key = _completion_key;

		// 2) push 한다.
		{
			// lock)
			std::lock_guard lock(m_lockable_complete);


			// - queueing한다.
			m_queue_executable.push_back(result);

			// - add_ref한다.
			_pexecutable->add_ref();
		}

		// 3) push후 notify
		m_cv_complete.notify_one();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return) 
	return true;
}

uint32_t api::queue_executable::pop_queued_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
{

	CONDITIONAL_LOCK(m_lockable_complete, lock_wait)
	{
		// 1) 개수가 0개면 wait...
		if (m_queue_executable.size() == 0 && _ticks_wait != chrono::tick::duration::zero())
		{
			// - wait한다.
			auto result = m_cv_complete.wait_for(lock_wait, _ticks_wait);

			// check) cv_status.timeout이면 time_out으로 종료된 것이다.
			RETURN_IF(result == std::cv_status::timeout, false);
		}

		// 2) get queued executable
		_result = std::move(m_queue_executable.front());

		// 3) pop
		m_queue_executable.pop_front();
	}

	// return)
	return true;
}

int api::queue_executable::pop_queued_executable(RESULT* _plist_event, int _max_event, chrono::tick::duration _ticks_wait) noexcept
{
	// check) _max_event가 0개면 리턴한다.
	CGASSERT_ERROR(_max_event > 0);

	// check) _max_event가 0이 아니라면 _plist_event는 반드시 nullptr이 아니어야 한다.
	CGASSERT_FATAL(_plist_event != nullptr);

	// declare) 
	int	count = 0;

	CONDITIONAL_LOCK(this->m_lockable_complete, lock_wait)
	{
		// 1) 개수가 0개면 wait...
		if(this->m_queue_executable.size() == 0 && _ticks_wait != chrono::tick::duration::zero())
		{
			// - wait한다.
			auto result = this->m_cv_complete.wait_for(lock_wait, _ticks_wait);

			// check) cv_status.timeout이면 time_out으로 종료된 것이다.
			if(result == std::cv_status::timeout)
				return false;
		}

		// 2) 읽어야 할 개수를 얻어낸다. (Queuing된 개수와 _max_event 중 작은 수)
		count = __select_min(_max_event, static_cast<int>(this->m_queue_executable.size()));

		// 3) count만큼 Queueing된 Executable을 pop해낸다.
		for(int i = 0; i < count; ++i)
		{
			// - queue executable
			_plist_event[i] = std::move(this->m_queue_executable.front());

			// - pop front
			this->m_queue_executable.pop_front();
		}
	}

	// return)
	return count;
}


}