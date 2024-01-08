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
	scoped_lock(this->m_cs_pexecutor)
	{
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

		// declar) 

		// 2) clear quque
		for (;;)
		{
			scoped_lock(this->m_cs_queue_executable)
			{
				if (this->m_is_processing == true)
				{
					std::this_thread::sleep_for(1ms);
					continue;
				}

				queue_executable = std::move(this->m_queue_executable);
				queue_executable_at = std::move(this->m_queue_executable_at);
				break;
			}
		}

		// 3) executal
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
	scoped_lock(this->m_cs_queue_executable)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, false);

		// - make executable item
		EXECUTABLE executable_temp;
		executable_temp.pexecutable = std::move(_pexecutable);
		executable_temp.result = 0;
		executable_temp.param = 0;

		// - 일단 queueing
		this->m_queue_executable.push_back(std::move(executable_temp));

		// check) processing 중이면 queuing만하고 끝냄.
		RETURN_IF(this->m_is_processing == true, true);

		// - set flag true
		this->m_is_processing = true;
	}

	// 2) get executor
	auto pexecutor = this->process_get_executor();

	// check) 
	CGASSERT_ERROR(pexecutor.exist());

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
		CGASSERT_ERROR(pexecutor.exist())

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

	if(_tick_execute < chrono::tick::now())
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, false);

		// 1) make sEXECUTABLE_AT
		EXECUTABLE_AT executable_at;
		executable_at.executable.pexecutable = std::move(_pexecutable);
		executable_at.executable.result = 0;
		executable_at.executable.param = 0;
		executable_at.tick = _tick_execute;

		// 2) find position
		if (this->m_queue_executable_at.empty())
		{
			this->m_queue_executable_at.emplace_back(std::move(executable_at));
		}
		else
		{
			// - find position
			auto iter = this->m_queue_executable_at.end();
			auto iter_begin = this->m_queue_executable_at.begin();

			do
			{
				auto iter_pre = iter;
				--iter_pre;

				if (iter_pre->tick < _tick_execute)
					break;

				iter = iter_pre;
			} while (iter != iter_begin);

			// - qeueing
			this->m_queue_executable_at.emplace(iter, std::move(executable_at));
		}

		// 2) 비동기 걸기
		system_executor::post(make_object<executor_post_at>(this), _tick_execute);
	}
	else
	{
		this->process_post(std::move(_pexecutable));
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
	scoped_lock(this->m_cs_queue_executable)
	{
		// check) processing 중이면 queuing만하고 끝냄.
		RETURN_IF(this->m_queue_executable.empty(), intptr_t(0));

		// check)
		CGASSERT_ERROR(this->m_is_processing == true)

		// - get queued executable 
		queue_executable = std::move(this->m_queue_executable);
	}

	// 2) execute excutable
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
	 scoped_lock(this->m_cs_queue_executable)
	 {
		 // check)
		 RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING,);

		// - get executable
		if (this->m_is_processing)
		{
			// - queueing for immediately execute
			for (auto iter = this->m_queue_executable_at.begin(), iter_end = this->m_queue_executable_at.end(); iter != iter_end && iter->tick > tick_now; ++iter, this->m_queue_executable_at.pop_front())
				this->m_queue_executable.push_back(std::move(iter->executable));

			// return)
			return;
		}
		else
		{
			// - queuing to vector_execute
			for (auto iter = this->m_queue_executable_at.begin(), iter_end = this->m_queue_executable_at.end(); iter!= iter_end && iter->tick > tick_now; ++iter, this->m_queue_executable_at.pop_front())
				vector_executable.push_back(std::move(iter->executable));

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
	scoped_lock(this->m_cs_queue_executable)
	{
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
	CGASSERT_ERROR(pexecutor.exist());

	// 3) 비동기 걸기
	pexecutor->post(this);
}


}