#include "pch.h"

using namespace CGDK;

//-----------------------------------------------------------------
// declare)
struct AUTO_RELEASE_EXECUTABLE
{
public:
	object_ptr<Iexecutable> m_pexecutable;
	api::Nthread* m_pthread_current;

#if defined(_ENABLE_DEBUG_NEW)
	char				m_debugInfo_pexecutable[sizeof(Iexecutable)];
	Iexecutable*		m_debugInfo_pexecutable_Debug;
	const char*			m_debug_info_filename;
	int					m_debug_info_line;
#endif

public:
	explicit AUTO_RELEASE_EXECUTABLE(object_ptr<Iexecutable>&& _pexecutable) :
		m_pexecutable(std::move(_pexecutable)),
		m_pthread_current(api::get_current_thread())
#if defined(_ENABLE_DEBUG_NEW)
		, m_debugInfo_pexecutable{}
		, m_debugInfo_pexecutable_Debug(reinterpret_cast<Iexecutable*>(m_debugInfo_pexecutable))
		, m_debug_info_filename(m_pexecutable->m_debug_info_filename)
		, m_debug_info_line(m_pexecutable->m_debug_info_line)
#endif
	{
		// check) m_pthread_current가 null이면 안된다.
		CGASSERT_FATAL(this->m_pthread_current != nullptr);

#if defined(_ENABLE_DEBUG_NEW)
		// check) ....
		CGASSERT_ERROR(sizeof(Iexecutable) > 32);

		// - 제일 앞  32Byte만 복사한다. virtual table 정도만 복사해 놓기 위해...
		memcpy(this->m_debugInfo_pexecutable, reinterpret_cast<void*>(m_pexecutable.get()), 32);
#endif

		// statistics)
		this->m_pthread_current->statistics_executing_begin();
	}

	~AUTO_RELEASE_EXECUTABLE()
	{
		this->reset();
	}

	void reset() noexcept
	{
		// check) m_pexecutable가 nullptr이면 그냥 리턴한다.
		if(this->m_pexecutable.empty())
			return;

		// statistics)
		this->m_pthread_current->statistics_executing_end();

		// 1) m_pexecutable을 release한다.
		this->m_pexecutable.reset();
	}
};
//-----------------------------------------------------------------

executor::queue_list::queue_list() noexcept
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_QUEUED);

	// 2) register to factory manager
	this->process_register_executor();
}

executor::queue_list::~queue_list() noexcept
{
	// 1) stop queue_list
	this->executor::queue_list::stop();

	// 2) unregister to factory manager
	this->process_unregister_executor();
}

result_code executor::queue_list::start(context& _context)
{
	// 1) Initializable
	auto* pinitializable = dynamic_cast<object::Iinitializable*>(this);

	// 2) Initializable이 있으면 initialize한다.
	if(pinitializable != nullptr)
	{
		pinitializable->initialize(_context);
	}

	// 3) 이전 stop()의 대기 취소를 푼다.
	this->cancel_wait(false);

	// 4) accept the posts again
	{
		// lock)
		std::lock_guard lock(this->m_lockable_queue_executable);

		this->m_is_accepting = true;
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::queue_list::stop() noexcept
{
	// 1) refuse the posts from now (an Nbatch / batch_wait counts a refused post as done : it waited forever for an executable queued
	//    after the threads ended). the queue only shrinks from here
	{
		// lock)
		std::lock_guard lock(this->m_lockable_queue_executable);

		this->m_is_accepting = false;
	}

	// 2) Queued된 것을 일단 모두 실행한다.
	this->execute_all();

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool executor::queue_list::process_post(object_ptr<Iexecutable>&& _pexecutable)
{
	// check) _pexecutable가 nullptr이면 Post를 던지지 않는다.
	if(_pexecutable.empty())
		return false;

	// declare)
	bool is_queued = false;

	// 1) Post한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_queue_executable);

		// check) stopped
		if (this->m_is_accepting == false)
			return false;

		// - 완전히 비어 있었다면 Thread를 signal시킨다.
		is_queued = this->m_queue_executable.empty();

		// - Queueing한다.
		this->m_queue_executable.push_back(std::move(_pexecutable));
	}

	// 2) Queuing을 했다면 Signal시킨다.
	//    - m_lockable_complete를 한 번 잡았다 놓는다. 기다리는 쪽은 이 lock 안에서 queue를 확인하고 wait에 들어가므로
	//      확인과 wait 사이에 notify가 끼어들어 신호를 잃어버리는 일이 없다.
	if (is_queued)
	{
		{ std::lock_guard lock_complete(this->m_lockable_complete); }
		this->m_cv_complete.notify_one();
	}

	// return)
	return	true;
}

object_ptr<Iexecutable> executor::queue_list::pop_queued_executable() noexcept
{
	// declare) 
	object_ptr<Iexecutable>	pexecutable;
	bool is_remained = false;

	// - queue는 다른 thread가 lock 안에서 바꾸므로 lock 밖에서 empty()를 읽지 않는다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_queue_executable);

		// check) Queueing된 것이 없으면 continue
		if(this->m_queue_executable.empty()==false)
		{
			pexecutable = std::move(this->m_queue_executable.front());
			this->m_queue_executable.pop_front();

			// - 꺼낸 뒤에도 남아 있는가?
			is_remained = (this->m_queue_executable.empty() == false);
		}
	}

	// - 꺼낸 뒤에도 queue에 남아 있으면 기다리는 thread를 하나 더 깨운다.
	//   process_post()는 queue가 비어 있다가 찰 때만 깨우므로, 한꺼번에 들어온 일을 깨어난 thread 하나가 혼자 처리하고
	//   나머지 thread는 wait의 timeout(queue_list_threads는 1초)까지 잠들어 있었다. (blocking query가 몰리면 하나씩 처리됨)
	//   꺼낸 thread가 다음 thread를 깨우므로 남은 일 수만큼 차례로 깬다. 기다리는 thread가 없으면 notify는 아무 일도 하지 않는다.
	if (is_remained)
	{
		{ std::lock_guard lock_complete(this->m_lockable_complete); }
		this->m_cv_complete.notify_one();
	}

	// return)
	return pexecutable;
}

void executor::queue_list::cancel_wait(bool _is_cancel) noexcept
{
	// 1) 기다리는 쪽은 m_lockable_complete 안에서 확인하고 wait에 들어가므로 lock 안에서 바꾼다. (신호를 잃지 않는다)
	{
		std::lock_guard lock_complete(this->m_lockable_complete);
		this->m_is_wait_cancelled = _is_cancel;
	}

	// 2) 기다리고 있는 thread를 모두 깨운다.
	if (_is_cancel)
		this->m_cv_complete.notify_all();
}

bool executor::queue_list::process_execute(chrono::tick::duration _ticks_wait, intptr_t /*_option*/)
{
	// declare) 
	object_ptr<Iexecutable>	pexecutable;

	// 1) 읽어낸다.
	pexecutable = this->pop_queued_executable();

	// check) Queue에서 꺼내오지 못했으면 wait만큼 기다린다.
	if (pexecutable.empty())
	{
		// check) wait하지 않으면 바로 false를 리턴한다.
		if(_ticks_wait == chrono::tick::duration::zero())
			return false;

		CONDITIONAL_LOCK(this->m_lockable_complete, lock_wait)
		{
			// 2) _ticks_wait만큼 기다린다. (queue가 차 있으면 기다리지 않는다. lock 안에서 확인하므로 notify를 잃지 않는다)
			//    - cancel_wait(true) 뒤에는 기다리지 않는다. (stop()이 기다리는 thread를 바로 끝낼 수 있게)
			auto is_queued = this->m_cv_complete.wait_for(lock_wait, _ticks_wait, [this]() { if (this->m_is_wait_cancelled) return true; std::lock_guard lock(this->m_lockable_queue_executable); return this->m_queue_executable.empty() == false; });

			// check) timeout이면 여기서 false를 리턴한다.
			if (is_queued == false)
			{
				// - process idle
				this->process_idle();

				// return)
				return false;
			}
		}

		// 3) Queuing된 Executable을 읽어낸다.
		pexecutable = this->pop_queued_executable();

		// check) 비어 있다면 false를 리턴한다.
		if(pexecutable.empty())
			return false;
	}

	// 4) Releaser을 생성!
	AUTO_RELEASE_EXECUTABLE	auto_executable(std::move(pexecutable));

	// 5) on_execute함수를 호출한다.
	auto_executable.m_pexecutable->process_execute(1, 0);

	// 6) reset
	auto_executable.reset();

	// statistics)
	this->m_statistics.now.count_execute.fetch_add(1, std::memory_order_relaxed);

	// return) 성공~
	return true;
}

void executor::queue_list::execute_all() noexcept
{
	CONTAINER_EXECUTABLE_QUEUE tempQeue;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_queue_executable);

		// - swap
		tempQeue.swap(this->m_queue_executable);
	}

	for(auto& iter: tempQeue)
	{
		// - an exception of one executable does not end the others (noexcept : it called std::terminate at the stop())
		try
		{
			// 1) Releaser으 생성!
			AUTO_RELEASE_EXECUTABLE	auto_executable(std::move(iter));

			// 2) on_execute함수를 호출한다.
			auto_executable.m_pexecutable->process_execute(1, 0);

			// 3) reset
			auto_executable.reset();
		}
		catch (...)
		{
		}
	}
}
