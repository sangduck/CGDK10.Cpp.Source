#include "pch.h"

using namespace CGDK;

// declare)
struct AUTO_RELEASE_EXECUTABLE
{
public:
	Iexecutable*		m_pexecutable;
	api::Nthread*		m_pthread_current;

public:
	explicit AUTO_RELEASE_EXECUTABLE(object_ptr<Iexecutable>&& _pexecutable) : m_pexecutable(std::move(_pexecutable)), m_pthread_current(api::get_current_thread())
	{
		// check) m_pthread_current가 null이면 안된다.
		CGASSERT_ERROR(m_pthread_current != nullptr);

		// statistics)
		m_pthread_current->statistics_executing_begin();
	}

	~AUTO_RELEASE_EXECUTABLE()
	{
		reset();
	}

	void	reset()
	{
		// check) m_pexecutable가 nullptr이면 그냥 리턴한다.
		RETURN_IF(m_pexecutable == nullptr);

		// statistics)
		m_pthread_current->statistics_executing_end();

		// 1) m_pexecutable을 release한다.
		m_pexecutable->release();

		// 2) m_pexecutable을 nullptr로 설정한다.
		m_pexecutable = nullptr;
	}
};

executor::queue_list::queue_list() noexcept
{
	// 1) set object type
	this->set_object_type(eOBJECT_TYPE::EXECUTOR_QUEUED);

	// 2) register to factory manager
	this->process_register_executor();
}

executor::queue_list::queue_list(std::string_view _name) :
	Nexecutor(_name)
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

	// 2) Initalizable이 있으면 initialize한다.
	if(pinitializable != nullptr)
	{
		pinitializable->initialize(_context);
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

result_code executor::queue_list::stop() noexcept
{
	// 1) Queued된 것을 일단 모두 실행한다.
	this->execute_all();

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool executor::queue_list::process_post(object_ptr<Iexecutable>&& _pexecutable)
{
	// check) _pexecutable가 nullptr이면 Post를 던지지 않는다.
	RETURN_IF(_pexecutable.empty(), false);

	// declare)
	bool is_queued = false;

	// 1) Post한다.
	scoped_lock(this->m_queue_executable)
	{
		// - 완전히 비어 있었다면 Thread를 signal시킨다.
		is_queued = this->m_queue_executable.empty();

		// - Queuning한다.
		this->m_queue_executable.push_back(std::move(_pexecutable));
	}

	// 2) Queuing을 했다면 Signal시킨다.
	if (is_queued)
	{
		this->m_cv_complete.notify_one();
	}

	// return)
	return	false;
}

object_ptr<Iexecutable> executor::queue_list::pop_queued_executable() noexcept
{
	// declare) 
	object_ptr<Iexecutable>	pexecutable;

	// check) Queueing된 것이 없으면 continue
	RETURN_IF(this->m_queue_executable.empty(), pexecutable);

	scoped_lock(this->m_queue_executable)
	{
		// check) Queueing된 것이 없으면 continue
		if(this->m_queue_executable.empty()==false)
		{
			pexecutable = std::move(this->m_queue_executable.front());
			this->m_queue_executable.pop_front();
		}
	}

	// return)
	return pexecutable;
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
		RETURN_IF(_ticks_wait == chrono::tick::duration::zero(), false);

		conditional_lock(this->m_lock_complete, lock_wait)
		{
			// 2) _ticks_wait만큼 기다린다.
			auto result = this->m_cv_complete.wait_for(lock_wait, _ticks_wait);

			// check) timeout이면 여기서 false를 리턴한다.
			if (result == std::cv_status::timeout)
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
		RETURN_IF(pexecutable.empty(), false);
	}

	// 4) Releaser을 생성!
	AUTO_RELEASE_EXECUTABLE	auto_executable(std::move(pexecutable));

	// 5) on_execute함수를 호출한다.
	auto_executable.m_pexecutable->process_execute(1, 0);

	// 6) reset
	auto_executable.reset();

	// statistics)
	++this->m_statistics.now.count_execute;

	// return) 성공~
	return true;
}

void executor::queue_list::execute_all() noexcept
{
	CONTAINER_EXECUTABLE_QUEUE tempQeue;

	scoped_lock(this->m_queue_executable)
	{
		tempQeue.swap(this->m_queue_executable);
	}

	for(auto& iter: tempQeue)
	{
		// 1) Releaser으 생성!
		AUTO_RELEASE_EXECUTABLE	auto_executable(std::move(iter));

		// 2) on_execute함수를 호출한다.
		auto_executable.m_pexecutable->process_execute(1, 0);

		// 3) reset
		auto_executable.reset();
	}
}
