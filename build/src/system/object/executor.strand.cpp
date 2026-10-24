#include "pch.h"

namespace CGDK
{


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
		std::list<EXECUTABLE_AT> queue_executable_at;

		// declare) 

		// 2) clear queue
		for (;;)
		{
			// lock)
			std::lock_guard lock(this->m_cs_queue_executable);

			if (this->m_is_processing == true)
			{
				std::this_thread::sleep_for(1ms);
				continue;
			}

			queue_executable = std::move(this->m_queue_executable);
			queue_executable_at = std::move(this->m_queue_executable_at);
			break;
		}

		// 3) execute
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

		// 4) reset pended executable
		queue_executable_at.clear();

		// 5) reset
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

	// 2) get executor
	auto pexecutor = this->process_get_executor();

	// check) 
	CGASSERT_FATAL(pexecutor.exist());

	// 3) 비동기 걸기
	pexecutor->post(this);

	// return) 종료~
	return true;
}

class executor::strand::executor_post_at : virtual public Iexecutable
{
public:
	executor_post_at(executor::strand* _pexecutor) noexcept : pexecutor(_pexecutor) {}
	virtual ~executor_post_at() noexcept {}
	virtual	intptr_t process_execute(intptr_t, size_t)
	{
		// check)
		CGASSERT_FATAL(pexecutor.exist())

		// - call 'process_execute_at'
		pexecutor->process_execute_post_at();

		// - reset pexecutor
		pexecutor.reset();

		// return)
		return intptr_t(0);
	}

	object_ptr<executor::strand> pexecutor;
};

bool executor::strand::process_post_at(object_ptr<Iexecutable>&& _pexecutable, chrono::tick::time_point _tick_execute)
{
	// check)
	CGASSERT_ERROR(_pexecutable.exist());

	// 1) 이미 지난 시각이면 바로 실행한다.
	if (_tick_execute <= chrono::tick::now())
		return this->process_post(std::move(_pexecutable));

	// 2) 미래 시각이면 시각 순서로 queuing한다.
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		// check)
		if(this->get_object_state() != eOBJECT_STATE::RUNNING)
			return false;

		// - make sEXECUTABLE_AT
		EXECUTABLE_AT executable_at;
		executable_at.executable.pexecutable = std::move(_pexecutable);
		executable_at.executable.result = 0;
		executable_at.executable.param = 0;
		executable_at.tick = _tick_execute;

		// - find position (뒤에서부터 : 시각이 같으면 먼저 넣은 것이 먼저 실행된다)
		auto iter = this->m_queue_executable_at.end();
		while (iter != this->m_queue_executable_at.begin())
		{
			auto iter_pre = std::prev(iter);

			if (iter_pre->tick <= _tick_execute)
				break;

			iter = iter_pre;
		}

		// - queueing
		this->m_queue_executable_at.emplace(iter, std::move(executable_at));
	}

	// 3) 그 시각에 실행되도록 건다.
	return system_executor::post(make_object<executor_post_at>(this), _tick_execute);
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

		// check) processing 중이면 queuing만하고 끝냄.
		if(this->m_queue_executable.empty())
			return intptr_t(0);

		// check)
		CGASSERT_ERROR(this->m_is_processing == true)

		// - get queued executable 
		queue_executable = std::move(this->m_queue_executable);
	}

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

 void executor::strand::process_execute_post_at()
 {
	 // declare)
	 std::vector<EXECUTABLE> vector_executable;

	 // 1) get tick now
	 auto tick_now = chrono::tick::now();

	 // 2) pop
	 {
		 // lock)
		 std::lock_guard lock(this->m_cs_queue_executable);

		 // check)
		 if(this->get_object_state() != eOBJECT_STATE::RUNNING)
			 return;

		// - get executable
		if (this->m_is_processing)
		{
			// - queueing for immediately execute (시각이 된 것만 : 처리 중인 thread가 process_post_next()에서 이어서 실행한다)
			while (this->m_queue_executable_at.empty() == false && this->m_queue_executable_at.front().tick <= tick_now)
			{
				this->m_queue_executable.push_back(std::move(this->m_queue_executable_at.front().executable));
				this->m_queue_executable_at.pop_front();
			}

			// return)
			return;
		}
		else
		{
			// - queuing to vector_execute (시각이 된 것만)
			while (this->m_queue_executable_at.empty() == false && this->m_queue_executable_at.front().tick <= tick_now)
			{
				vector_executable.push_back(std::move(this->m_queue_executable_at.front().executable));
				this->m_queue_executable_at.pop_front();
			}

			// - set flag true
			this->m_is_processing = true;
		}
	 }

	 // 3) execute executable
	for (auto& iter : vector_executable)
	{
		try
		{
			iter.pexecutable->process_execute(iter.result, iter.param);
		}
		catch (...)
		{
		}
	}

	 // 4) post next
	 this->process_post_next();
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
	// 1) post next?
	{
		// lock)
		std::lock_guard lock(this->m_cs_queue_executable);

		if (this->m_queue_executable.empty() || this->get_object_state() != eOBJECT_STATE::RUNNING)
		{
			// - set flag false
			this->m_is_processing = false;

			// return)
			return;
		}
	}

	// 2) get executor
	auto pexecutor = this->process_get_executor();

	// check) 
	CGASSERT_FATAL(pexecutor.exist());

	// 3) 비동기 걸기
	pexecutor->post(this);
}


}