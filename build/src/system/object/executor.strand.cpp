#include "pch.h"
#include "platform.h"

namespace CGDK
{

namespace
{
// - 이 thread 가 지금 실행 중인 strand 들 (strand 의 executor 가 다른 strand 이면 중첩된다)
struct STRAND_EXECUTING
{
	const executor::strand*	pstrand = nullptr;
	bool					is_destroyed = false;
	STRAND_EXECUTING*		pprev = nullptr;
};
thread_local STRAND_EXECUTING* t_pstrand_executing = nullptr;

class strand_executing_scope
{
public:
	strand_executing_scope(const executor::strand* _pstrand) noexcept { this->m_executing.pstrand = _pstrand; this->m_executing.pprev = t_pstrand_executing; t_pstrand_executing = &this->m_executing; }
	~strand_executing_scope() noexcept { t_pstrand_executing = this->m_executing.pprev; }
private:
	STRAND_EXECUTING m_executing;
};

STRAND_EXECUTING* find_strand_executing(const executor::strand* _pstrand) noexcept
{
	for (auto* p = t_pstrand_executing; p != nullptr; p = p->pprev)
	{
		if (p->pstrand == _pstrand)
			return p;
	}
	return nullptr;
}
}


executor::strand::strand() noexcept
{
}

executor::strand::~strand() noexcept
{
}

result_code executor::strand::_process_initialize(context& _context)
{
	// 1) call 'on_initialize'
	this->on_initialize(_context);

	// 2) 
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pexecutor);

		if (this->m_pexecutor.empty())
		{
			this->m_pexecutor = system_executor::executor();
		}
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}

result_code executor::strand::_process_destroying() noexcept
{
	try
	{
		// 1) call 'on_destroying'
		this->on_destroying();

		// declare) 
		std::vector<EXECUTABLE> queue_executable;
		decltype(this->m_queue_executable_at) queue_executable_at;

		// 2) 이 strand 의 executable 안에서 destroy 했으면 기다리지 않는다.
		//    (처리 중인 것은 이 thread 다 : 기다리면 끝나지 않는다. 남은 executable 은 지금 executable 이 끝난 뒤 이 thread 가 순서대로 실행한다)
		if (auto* pexecuting = find_strand_executing(this); pexecuting != nullptr)
		{
			pexecuting->is_destroyed = true;

			// lock)
			std::lock_guard lock(this->m_cs_queue_executable);

			queue_executable_at = std::move(this->m_queue_executable_at);
		}
		else
		{
			// 3) clear queue (다른 thread 가 처리 중이면 끝날 때까지 기다린다 : 기다리는 동안은 락을 잡지 않는다)
			//    - waits only while a thread executes the queue (m_is_executing), not while the strand is only posted : the posted execution
			//      may never run while this thread waits (it can be in the runnext slot of this thread, or queued on an executor whose only
			//      thread is this one). the queue is taken and executed here, the posted execution finds it empty and ends.
			//    - the runnext slot of this thread is spilled while it waits (an executable the executing one waits for is run by the others)
			auto tick_yield_end = chrono::tick::now() + 2ms;
			bool is_waiting = false;
			for (;;)
			{
				{
					// lock)
					std::lock_guard lock(this->m_cs_queue_executable);

					if (this->m_is_executing == false)
					{
						queue_executable = std::move(this->m_queue_executable);
						queue_executable_at = std::move(this->m_queue_executable_at);
						break;
					}
				}

				// - spill the runnext slot (once)
				if (is_waiting == false)
				{
					is_waiting = true;
					executor::io_context::runnext_suspend();
				}

				// - 처음 2ms 는 yield (windows 의 sleep_for(1ms) 는 ~15ms 일 수 있다)
				if (chrono::tick::now() < tick_yield_end)
					std::this_thread::yield();
				else
					std::this_thread::sleep_for(1ms);
			}

			// - the runnext slot again
			if (is_waiting)
				executor::io_context::runnext_resume();
		}

		// 4) execute
		for (auto& iter : queue_executable)
		{
			try
			{
				iter.pexecutable->process_execute(iter.result, iter.param);
			}
			catch (...)
			{
			}
		}

		// 5) reset pended executable
		queue_executable_at.clear();

		// 6) reset
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}

	// return) 
	return result_code(eRESULT::SUCCESS);
}

bool executor::strand::process_post(object_ptr<Iexecutable>&& _pexecutable)
{
	// check)
	CGASSERT_ERROR(_pexecutable.exist());

	// declare)
	Iexecutable* pexecutable_raw = _pexecutable.get();

	// 1) queuing한다.
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// check)
		if(this->get_object_state() != eOBJECT_STATE::RUNNING)
			return false;

		// - make executable item
		EXECUTABLE executable_temp;
		executable_temp.pexecutable = std::move(_pexecutable);
		executable_temp.result = 0;
		executable_temp.param = 0;

		// - 일단 queueing
		this->m_queue_executable.push_back(std::move(executable_temp));

		// check) processing 중이면 queuing만하고 끝냄.
		if(this->m_is_processing == true)
			return true;

		// - set flag true
		this->m_is_processing = true;
	}

	// 2) post this strand to its executor
	if (this->_post_to_executor())
		return true;

	// 3) refused (the executor stopped, or no executor) : nothing will execute the queue. the executable just queued is taken back
	//    (unless destroy() took the queue and executed it meanwhile) and the ones queued meanwhile wait for the next post or destroy()
	//    (m_is_processing stayed true : a post would only queue and destroy() would wait forever)
	object_ptr<Iexecutable> pexecutable_back;
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// - not posted
		this->m_is_processing = false;

		// - take back the one just queued (from the back : the same executable may be queued twice)
		for (auto iter = this->m_queue_executable.rbegin(); iter != this->m_queue_executable.rend(); ++iter)
		{
			if (iter->pexecutable.get() == pexecutable_raw)
			{
				pexecutable_back = std::move(iter->pexecutable);
				this->m_queue_executable.erase(std::next(iter).base());
				break;
			}
		}
	}

	// return) false : not queued (released here, outside the lock)
	return pexecutable_back.empty();
}

bool executor::strand::_post_to_executor() noexcept
{
	try
	{
		// 1) get executor
		auto pexecutor = this->process_get_executor();

		// check) no executor (the system executor is destroyed)
		if (pexecutor.empty())
			return false;

		// 2) post (false : the executor is stopped)
		return pexecutor->post(object_ptr<Iexecutable>(this));
	}
	catch (...)
	{
		return false;
	}
}

class executor::strand::executor_post_at : virtual public Iexecutable
{
public:
	executor_post_at(executor::strand* _pexecutor, chrono::tick::time_point _tick_wake) noexcept : pexecutor(_pexecutor), tick_wake(_tick_wake) {}
	virtual ~executor_post_at() noexcept {}
	virtual	intptr_t process_execute(intptr_t, size_t)
	{
		// check)
		CGASSERT_FATAL(pexecutor.exist())

		// - call 'process_execute_at'
		pexecutor->process_execute_post_at(this->tick_wake);

		// - reset pexecutor
		pexecutor.reset();

		// return)
		return intptr_t(0);
	}

	object_ptr<executor::strand> pexecutor;
	chrono::tick::time_point tick_wake;
};

namespace
{
	// - removes one wake time (a wake fired or was refused)
	void _erase_tick_wake(std::multiset<chrono::tick::time_point>& _set, chrono::tick::time_point _tick) noexcept
	{
		auto iter = _set.find(_tick);
		if (iter != _set.end())
			_set.erase(iter);
	}
}

bool executor::strand::process_post_at(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute)
{
	// check)
	CGASSERT_ERROR(_pexecutable.exist());

	// 1) 이미 지난 시각이면 바로 실행한다.
	if (_tick_execute <= chrono::tick::now())
		return this->process_post(std::move(_pexecutable));

	// declare)
	Iexecutable* pexecutable_raw = _pexecutable.get();
	bool is_wake = false;

	// 2) 미래 시각이면 시각 순서로 queuing한다.
	//    - multimap : O(log N), the same time after the ones queued before (in the order of the posts)
	//    - a wake is posted to the scheduler only when no pending wake is at or before this time (one wake for the earliest)
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// check)
		if(this->get_object_state() != eOBJECT_STATE::RUNNING)
			return false;

		// - make executable item
		EXECUTABLE executable_temp;
		executable_temp.pexecutable = std::move(_pexecutable);
		executable_temp.result = 0;
		executable_temp.param = 0;

		// - queueing
		this->m_queue_executable_at.emplace(_tick_execute, std::move(executable_temp));

		// - a wake at this time is needed?
		if (this->m_set_tick_wake.empty() || *this->m_set_tick_wake.begin() > _tick_execute)
		{
			this->m_set_tick_wake.insert(_tick_execute);
			is_wake = true;
		}
	}

	// check) an earlier wake is pending : it posts the next one
	if (is_wake == false)
		return true;

	// 3) post a wake at the time (the wake only moves the executables of the time to the queue : they are executed on the executor of the strand)
	if (_post_wake(_tick_execute))
		return true;

	// 4) refused (the scheduler is stopped) : the executable just queued is taken back (unless a wake moved it to the queue meanwhile)
	object_ptr<Iexecutable> pexecutable_back;
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		auto range = this->m_queue_executable_at.equal_range(_tick_execute);
		for (auto iter = range.second; iter != range.first;)
		{
			--iter;
			if (iter->second.pexecutable.get() == pexecutable_raw)
			{
				pexecutable_back = std::move(iter->second.pexecutable);
				this->m_queue_executable_at.erase(iter);
				break;
			}
		}
	}

	// return) false : not queued (released here, outside the lock)
	return pexecutable_back.empty();
}

bool executor::strand::_post_wake(chrono::tick::time_point _tick_wake) noexcept
{
	// 1) post the wake to the scheduler
	try
	{
		if (system_executor::post(make_object<executor_post_at>(this, _tick_wake), _tick_wake))
			return true;
	}
	catch (...)
	{
	}

	// 2) refused : not pending
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		_erase_tick_wake(this->m_set_tick_wake, _tick_wake);
	}

	// return)
	return false;
}

bool executor::strand::process_execute(chrono::tick::duration /*_ticks_wait*/, intptr_t /*_option*/)
{
	return false;
}

 intptr_t executor::strand::process_execute(intptr_t /*_result*/, size_t /*_param*/)
{
	// declare) 
	std::vector<EXECUTABLE> queue_executable;

	// 1) get queued executable
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// check) destroy() took the queue while this execution was only posted : it ends here
		if(this->m_queue_executable.empty())
		{
			this->m_is_processing = false;
			return intptr_t(0);
		}

		// check)
		CGASSERT_ERROR(this->m_is_processing == true)

		// - executing (destroy() waits for it from now)
		this->m_is_executing = true;

		// - get queued executable
		queue_executable = std::move(this->m_queue_executable);
	}

	// declare)
	strand_executing_scope scope_executing(this);

	// 2) execute executable
	for (auto& iter : queue_executable)
	{
		// - execute
		try
		{
			// - execute
			iter.pexecutable->process_execute(iter.result, iter.param);
		}
		catch (...)
		{
		}
	}

	// 3) post next
	this->process_post_next();

	// return) 
	return intptr_t(0);
}

void executor::strand::process_execute_post_at(chrono::tick::time_point _tick_wake)
{
	// declare)
	chrono::tick::time_point tick_wake_next{};
	bool is_wake_next = false;
	bool is_post = false;

	// 1) get tick now
	auto tick_now = chrono::tick::now();

	// 2) move the executables of the time to the queue (they are executed on the executor of the strand, in the order of the queue)
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// - this wake is not pending any more
		_erase_tick_wake(this->m_set_tick_wake, _tick_wake);

		// check)
		if(this->get_object_state() != eOBJECT_STATE::RUNNING)
			return;

		// - the ones of the time (in the order of the time)
		bool is_moved = false;
		while (this->m_queue_executable_at.empty() == false && this->m_queue_executable_at.begin()->first <= tick_now)
		{
			auto iter = this->m_queue_executable_at.begin();
			this->m_queue_executable.push_back(std::move(iter->second));
			this->m_queue_executable_at.erase(iter);
			is_moved = true;
		}

		// - the wake of the next one (unless an earlier one is pending)
		if (this->m_queue_executable_at.empty() == false)
		{
			auto tick_next = this->m_queue_executable_at.begin()->first;

			if (this->m_set_tick_wake.empty() || *this->m_set_tick_wake.begin() > tick_next)
			{
				this->m_set_tick_wake.insert(tick_next);
				tick_wake_next = tick_next;
				is_wake_next = true;
			}
		}

		// - post the strand (processing : the thread executing it executes them in process_post_next())
		if (is_moved && this->m_is_processing == false)
		{
			this->m_is_processing = true;
			is_post = true;
		}
	}

	// 3) the wake of the next one (refused : the scheduler is stopped. the rest wait for destroy())
	if (is_wake_next)
		this->_post_wake(tick_wake_next);

	// 4) post this strand to its executor (refused : the queue waits for the next post or destroy())
	if (is_post && this->_post_to_executor() == false)
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		this->m_is_processing = false;
	}
}

 object_ptr<Iexecutor> executor::strand::process_get_executor()
{
	// 1) executor 얻기...
	auto pexecutor = this->executor();

	// 2) get default executor
	if (pexecutor.empty())
		pexecutor = system_executor::executor();

	// return) 
	return pexecutor;
}

void executor::strand::process_post_next()
{
	// 1) 이 strand 의 executable 안에서 destroy 됐으면 남은 executable 을 이 thread 에서 순서대로 실행하고 끝낸다.
	if (auto* pexecuting = find_strand_executing(this); pexecuting != nullptr && pexecuting->is_destroyed)
	{
		for (;;)
		{
			// declare)
			std::vector<EXECUTABLE> queue_executable;

			{
				// lock)
				std::lock_guard lock(this->m_cs_queue_executable);

				if (this->m_queue_executable.empty())
				{
					// - set flag false
					this->m_is_processing = false;
					this->m_is_executing = false;

					// return)
					return;
				}

				queue_executable = std::move(this->m_queue_executable);
			}

			for (auto& iter : queue_executable)
			{
				try
				{
					iter.pexecutable->process_execute(iter.result, iter.param);
				}
				catch (...)
				{
				}
			}
		}
	}

	// 2) post next?
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// - the execution of this thread ended (from here destroy() takes the queue itself)
		this->m_is_executing = false;

		if (this->m_queue_executable.empty() || this->get_object_state() != eOBJECT_STATE::RUNNING)
		{
			// - set flag false
			this->m_is_processing = false;

			// return)
			return;
		}
	}

	// 3) post this strand to its executor again
	if (this->_post_to_executor())
		return;

	// 4) refused (the executor stopped, or no executor) : not posted. the queued executables wait for the next post or destroy()
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		this->m_is_processing = false;
	}
}


}