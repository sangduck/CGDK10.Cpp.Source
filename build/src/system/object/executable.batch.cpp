#include "pch.h"

namespace CGDK
{

executable::Nbatch::~Nbatch() noexcept
{
	CGASSERT_ERROR(this->m_list_executable.empty());
}

size_t executable::Nbatch::executing_count() const noexcept
{
	scoped_lock(this->m_list_executable)
	{
		return this->m_list_executable.size();
	}
}

void executable::Nbatch::queue_executable(object_ptr<Iexecutable>&& _pexecutable)
{
	scoped_lock(this->m_list_executable)
	{
		this->m_list_executable.push_back(std::move(_pexecutable));
	}
}

void executable::Nbatch::clear() noexcept
{
	scoped_lock(this->m_list_executable)
	{
		this->m_list_executable.clear();
	}
}

bool executable::Nbatch::request_execute(Iexecutor* _pexecutor)
{
	// declare) 
	size_t iSize = 0;

	// 1) 갯수를 얻어둔다.
	scoped_lock(m_list_executable)
	{
		iSize = m_list_executable.size();
	}

	// 2) Executable의 갯수만큼 executor를 실행시킨다.
	for(size_t i=0; i<iSize; ++i)
	{
		_pexecutor->post(this);
	}

	// return) 
	return true;
}

intptr_t executable::Nbatch::process_execute(intptr_t _return, size_t _option)
{
	// declare) 
	bool is_empty = false;

	object_ptr<Iexecutable> pexecutable;

	scoped_lock(this->m_list_executable)
	{
		// 1) 하나를 줄인다.
		pexecutable = this->m_list_executable.front();
		this->m_list_executable.pop_front();

		// 2) Empty인지 확인한다.
		is_empty = this->m_list_executable.empty();
	}

	// check) 
	RETURN_IF(pexecutable.empty(), intptr_t(1));

	// 3) 실행한다.
	pexecutable->process_execute(_return, _option);

	// 4) 모든 Executable이 실행되면 Signal시킨다.
	if(is_empty)
	{
		process_complete_execution();
	}

	// return) 
	return intptr_t(0);
}

void executable::Nbatch::on_final_release() noexcept
{
	clear();
}


}