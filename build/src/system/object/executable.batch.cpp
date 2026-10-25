#include "pch.h"

namespace CGDK
{

executable::Nbatch::~Nbatch() noexcept
{
	// lock)
	std::lock_guard cs(this->m_lockable_list_executable);

	CGASSERT_ERROR(this->m_list_executable.empty());
}

size_t executable::Nbatch::executing_count() const noexcept
{
	// return) the requested ones not completed yet (the size of the list reached 0 when the last one was taken, before it ran)
	return this->m_count_executing.load(std::memory_order_acquire);
}

void executable::Nbatch::_complete_one() noexcept
{
	// check) not the last one
	if (this->m_count_executing.fetch_sub(1, std::memory_order_acq_rel) != 1)
		return;

	try
	{
		this->process_complete_execution();
	}
	catch (...)
	{
	}
}

void executable::Nbatch::queue_executable(object_ptr<Iexecutable>&& _pexecutable)
{
	// lock)
	std::lock_guard cs(this->m_lockable_list_executable);

	// 1) queue executable
	this->m_list_executable.push_back(std::move(_pexecutable));
}

void executable::Nbatch::clear() noexcept
{
	// lock)
	std::lock_guard cs(this->m_lockable_list_executable);

	// 1) clear
	this->m_list_executable.clear();
}

bool executable::Nbatch::request_execute(Iexecutor* _pexecutor)
{
	// declare) 
	size_t iSize = 0;

	// 1) 개수를 얻어둔다.
	{
		// lock)
		std::lock_guard cs(this->m_lockable_list_executable);

		// - get size
		iSize = m_list_executable.size();

		// - counted before the posts (one may complete before the next is posted)
		this->m_count_executing.fetch_add(iSize, std::memory_order_acq_rel);
	}

	// 2) Executable의 개수만큼 executor를 실행시킨다.
	//    (a post refused : that one is completed here. it would never run)
	bool is_posted = true;

	for(size_t i=0; i<iSize; ++i)
	{
		bool result = false;

		try
		{
			result = _pexecutor->post(this);
		}
		catch (...)
		{
		}

		if (result == false)
		{
			is_posted = false;
			this->_complete_one();
		}
	}

	// return)
	return is_posted;
}

intptr_t executable::Nbatch::process_execute(intptr_t _return, size_t _option)
{
	// declare) the execution is completed at the end (also when it throws)
	struct COMPLETE_ONE
	{
		Nbatch* pbatch;
		~COMPLETE_ONE() noexcept { pbatch->_complete_one(); }
	} complete_one{ this };

	object_ptr<Iexecutable> pexecutable;

	{
		// lock)
		std::lock_guard cs(this->m_lockable_list_executable);

		// 1) 하나를 줄인다. (cleared after the request : none)
		if (this->m_list_executable.empty() == false)
		{
			pexecutable = std::move(this->m_list_executable.front());
			this->m_list_executable.pop_front();
		}
	}

	// check)
	if(pexecutable.empty())
		return intptr_t(1);

	// 3) 실행한다.
	pexecutable->process_execute(_return, _option);

	// return)
	return intptr_t(0);
}

void executable::Nbatch::on_final_release() noexcept
{
	clear();
}


}