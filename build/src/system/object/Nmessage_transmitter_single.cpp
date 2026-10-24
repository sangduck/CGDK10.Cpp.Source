#include "pch.h"

namespace CGDK
{

bool Nmessage_transmitter_single::process_attach(object_ptr<Imessageable>&& _pmessageable)
{
	// check) 
	CGASSERT_ERROR(_pmessageable.exist());

	// check) _pmessageable이 nullptr이면 안 된다!
	if(_pmessageable.empty())
		return false;

	// declare) 
	object_ptr<Imessageable> pmessageable_pre;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmessageable);

		// check) 
		if(this->m_pmessageable == _pmessageable)
			return false;

		// 1) Pre
		pmessageable_pre = std::move(this->m_pmessageable);

		// 2) Set Messageable
		this->m_pmessageable = std::move(_pmessageable);
	}

	// return) 
	return true;
}

bool Nmessage_transmitter_single::process_detach(Imessageable* _pmessageable) noexcept
{
	// check) 
	if(_pmessageable == nullptr)
		return this->reset_message_transmitter() != 0;
	
	// declare) 
	object_ptr<Imessageable> pmessageable;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmessageable);

		// check) 
		if(this->m_pmessageable != _pmessageable)
			return false;

		// 2) reset
		pmessageable = std::move(this->m_pmessageable);
	}

	// return) 
	return true;
}

int Nmessage_transmitter_single::process_reset_message_transmitter() noexcept
{
	// declare) 
	object_ptr<Imessageable> pmessageable;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmessageable);

		// check) 
		if(this->m_pmessageable.empty())
			return 0;

		// 1) 값을 임시로 복사
		pmessageable = std::move(this->m_pmessageable);
	}

	// return)
	return 1;
}

result_code Nmessage_transmitter_single::process_transmit_message(sMESSAGE& _msg)
{
	// 1) mediator를 얻는다.
	object_ptr<Imessageable> pmessageable;

	// 2) 복사
	{
		// lock)
		std::lock_guard lock(this->m_lockable_pmessageable);

		// - copy
		pmessageable = this->m_pmessageable;
	}

	// check) pmessageable이 nullptr이면 그냥 끝낸다.
	if(pmessageable.empty())
		return result_code(eRESULT::BYPASS);

	// 3) Message를 transmit_message한다.
	return pmessageable->request_process_message(_msg);
}


}