#include "pch.h"

namespace CGDK
{

int Nmessage_transmitter::process_reset_message_transmitter() noexcept
{
	// declare) 
	static_vector<object_ptr<Imessageable>, MAX_MEDIATOR> vector_messageable;

	// 1) swap
	scoped_unique_lock(this->m_lockable_array_messageable)
	{
		vector_messageable = std::move(this->m_array_messageable);
	}

	// declare)
	const int count_messageable = static_cast<int>(vector_messageable.size());

	// 2) release
	this->release(count_messageable);

	// return)
	return count_messageable;
}

bool Nmessage_transmitter::process_attach_messageable_front(object_ptr<Imessageable>&& _pmessageable)
{
	// check) 
	RETURN_IF(_pmessageable.empty(), false);

	scoped_unique_lock(this->m_lockable_array_messageable)
	{
		// check)
		CGASSERT_ERROR(this->m_array_messageable.size() < MAX_MEDIATOR);

		// check) return if aleady exist
		RETURN_IF(std::find(this->m_array_messageable.begin(), this->m_array_messageable.end(), _pmessageable) != this->m_array_messageable.end(), false);

		// 2) add
		this->m_array_messageable.push_front(std::move(_pmessageable));

		// 3) add reference count
		this->add_ref();
	}

	// return) 
	return true;
}

bool Nmessage_transmitter::process_attach_messageable_back(object_ptr<Imessageable>&& _pmessageable)
{
	// check) 
	RETURN_IF(_pmessageable.empty(), false);

	scoped_unique_lock(this->m_lockable_array_messageable)
	{
		// check) 
		CGASSERT_ERROR(this->m_array_messageable.size() < MAX_MEDIATOR);

		// check) return if aleady exist
		RETURN_IF(std::find(this->m_array_messageable.begin(), this->m_array_messageable.end(), _pmessageable) != this->m_array_messageable.end(), false);

		// 2) add messageable
		this->m_array_messageable.push_back(std::move(_pmessageable));

		// 3) add reference count
		this->add_ref();
	}

	// return) 
	return true;
}

bool Nmessage_transmitter::process_detach(Imessageable* _pmessageable) noexcept
{
	// check) 
	if(_pmessageable == nullptr)
	{
		// - remove all
		return this->reset_message_transmitter() != 0;
	}

	// declare)
	object_ptr<Imessageable> pmessageable;

	scoped_unique_lock(m_lockable_array_messageable)
	{
		// 1) find
		auto iter = std::find(this->m_array_messageable.begin(), this->m_array_messageable.end(), _pmessageable);

		// check) return if not exist
		RETURN_IF(iter == this->m_array_messageable.end(), false);

		// 2) move
		pmessageable = std::move(*iter);

		// 3) erase
		this->m_array_messageable.erase(iter);
	}
	
	// 4) release self
	this->release();

	// return) 
	return true;
}

result_code Nmessage_transmitter::process_transmit_message(sMESSAGE& _msg)
{
	// declare)
	static_vector<object_ptr<Imessageable>, MAX_MEDIATOR> array_messageable;

	// 1) copy messageable list for temporary
	scoped_shared_lock(this->m_lockable_array_messageable)
	{
		array_messageable = this->m_array_messageable;
	}

	// 2) call 'request_process_messagel' all of messageable
	for(auto& iter:array_messageable)
	{
		// - call 'request_process_messagel'
		const auto result = iter->request_process_message(_msg);

		// check) return if processed
		RETURN_IF(result != eRESULT::BYPASS, result);
	}
	
	// return) 
	return result_code(eRESULT::BYPASS);
}


}