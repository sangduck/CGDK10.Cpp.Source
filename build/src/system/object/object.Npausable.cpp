#include "pch.h"

namespace CGDK
{


result_code object::Npausable::pause()
{
	// 1) 
	this->_process_pause_request();

	// 2) service상태를 pause의 Pending상태로 바꾼다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::PAUSE_PENDING);

	// check) NONE 상태가 아니라면 그냥 끝낸다.
	RETURN_IF(is_changed == false, eRESULT::NOT_ALLOWED);

	// 3) 훅함수를 호출한다.
	this->process_pause();

	// 4) 상태를 Paused상태로 바꾼다.
	this->set_object_state(eOBJECT_STATE::PAUSED);

	// return) 
	return result_code(eRESULT::SUCCESS);
}

result_code object::Npausable::resume()
{
	// 1) request resume
	this->_process_resume_request();

	// 2) Status를 eOBJECT_STATE::INITIALIZE_PENDING로 설정한다.
	bool is_changed = this->set_object_state_if(eOBJECT_STATE::PAUSED, eOBJECT_STATE::CONTINUE_PENDING);

	// check) NONE 상태가 아니라면 그냥 끝낸다.
	RETURN_IF(is_changed == false, eRESULT::NOT_ALLOWED);

	// 3) 훅함수를 호출한다.
	this->process_resume();

	// 4) 상태를 Running상태로 바꾼다.
	this->set_object_state(eOBJECT_STATE::RUNNING);

	// return) 
	return result_code(eRESULT::SUCCESS);
}

void object::Npausable::process_pause()
{
	// 1) on_starting 함수를 호출한다.
	this->_process_pauseing();

	// declare)
	auto iter = this->m_container.begin();
	auto iter_end = this->m_container.end();

	try
	{
		// 3) 모든 자식 객체를 start한다.
		for (; iter != iter_end; ++iter)
		{
			object::pause(*iter);
		}

		// 3) on_start 함수를 호출한다.
		this->_process_pause();
	}
	catch (...)
	{
		// - start했던 자식 객체를 Roll-back한다.
		for (auto iter_rollback = this->m_container.begin(); iter_rollback != iter; ++iter_rollback)
			object::resume(*iter_rollback);

		// - on_stop 함수를 호출한다.(on_starting()함수와 짝함수이므로 호출된다.)
		try
		{
			this->_process_resume();
		}
		catch (...)
		{
		}

		// reraise) 
		throw;
	}
}

void object::Npausable::process_resume()
{
	// 1) on_stopping 함수를 호출한다.
	try
	{
		this->_process_resuming();
	}
	catch (...)
	{
	}

	// 2) Childe를 stop한다.
	for (auto riter = this->m_container.rbegin(); riter != this->m_container.rend(); ++riter)
	{
		object::resume(*riter);
	}

	// 3) on_stop 함수를 호출한다.
	try
	{
		this->_process_resume();
	}
	catch (...)
	{
	}
}


}