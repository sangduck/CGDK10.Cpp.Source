#include "pch.h"

namespace CGDK
{

int Nmessage_transmitter::process_reset_message_transmitter() noexcept
{
	// declare)
	std::shared_ptr<const CONTAINER_MESSAGEABLE> parray_messageable;

	// 1) swap
	SCOPED_UNIQUE_LOCK(this->m_lockable_array_messageable)
	{
		parray_messageable = std::move(this->m_parray_messageable);
		this->m_parray_messageable.reset();
	}

	// declare)
	const int count_messageable = parray_messageable ? static_cast<int>(parray_messageable->size()) : 0;

	// 2) release (messageable은 transmit 중인 snapshot이 모두 끝나면 해제된다)
	this->release(count_messageable);

	// return)
	return count_messageable;
}

bool Nmessage_transmitter::process_attach_messageable_front(object_ptr<Imessageable>&& _pmessageable)
{
	return this->_process_attach_messageable(std::move(_pmessageable), true);
}

bool Nmessage_transmitter::process_attach_messageable_back(object_ptr<Imessageable>&& _pmessageable)
{
	return this->_process_attach_messageable(std::move(_pmessageable), false);
}

bool Nmessage_transmitter::_process_attach_messageable(object_ptr<Imessageable>&& _pmessageable, bool _is_front)
{
	// check)
	if(_pmessageable.empty())
		return false;

	// declare)
	std::shared_ptr<const CONTAINER_MESSAGEABLE> parray_old;

	SCOPED_UNIQUE_LOCK(this->m_lockable_array_messageable)
	{
		// - 새 목록을 만든다. (이전 snapshot은 transmit 중일 수 있으므로 바꾸지 않는다)
		auto parray_new = (this->m_parray_messageable) ? std::make_shared<CONTAINER_MESSAGEABLE>(*this->m_parray_messageable) : std::make_shared<CONTAINER_MESSAGEABLE>();

		// check)
		CGASSERT_ERROR(parray_new->size() < MAX_MEDIATOR);

		// check) return if already exist
		if(std::find(parray_new->begin(), parray_new->end(), _pmessageable) != parray_new->end())
			return false;

		// 1) add
		if (_is_front)
			parray_new->push_front(std::move(_pmessageable));
		else
			parray_new->push_back(std::move(_pmessageable));

		// 2) replace (이전 snapshot은 lock 밖에서 해제한다)
		parray_old = std::move(this->m_parray_messageable);
		this->m_parray_messageable = std::move(parray_new);

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
		return this->reset_message_transmitter() != 0;

	// declare)
	std::shared_ptr<const CONTAINER_MESSAGEABLE> parray_old;

	try
	{
		SCOPED_UNIQUE_LOCK(m_lockable_array_messageable)
		{
			// check)
			if(!this->m_parray_messageable)
				return false;

			// 1) find
			auto iter = std::find(this->m_parray_messageable->begin(), this->m_parray_messageable->end(), _pmessageable);

			// check) return if not exist
			if(iter == this->m_parray_messageable->end())
				return false;

			// 2) 그것만 뺀 새 목록을 만든다.
			auto parray_new = std::make_shared<CONTAINER_MESSAGEABLE>();
			for (auto iter_copy = this->m_parray_messageable->begin(); iter_copy != this->m_parray_messageable->end(); ++iter_copy)
			{
				if (iter_copy != iter)
					parray_new->push_back(*iter_copy);
			}

			// 3) replace (이전 snapshot(과 messageable)은 lock 밖에서, transmit 중인 것이 모두 끝나면 해제된다)
			parray_old = std::move(this->m_parray_messageable);
			this->m_parray_messageable = parray_new->empty() ? nullptr : std::move(parray_new);
		}
	}
	catch (...)
	{
		// - 메모리가 부족하면 detach하지 못한다.
		return false;
	}

	// 4) release self
	this->release();

	// return)
	return true;
}

result_code Nmessage_transmitter::process_transmit_message(sMESSAGE& _msg)
{
	// declare)
	std::shared_ptr<const CONTAINER_MESSAGEABLE> parray_messageable;

	// 1) snapshot을 얻는다. (messageable의 참조 count는 건드리지 않는다)
	SCOPED_SHARED_LOCK(this->m_lockable_array_messageable)
	{
		parray_messageable = this->m_parray_messageable;
	}

	// check)
	RETURN_IF(!parray_messageable, result_code(eRESULT::BYPASS));

	// 2) call 'request_process_messagel' all of messageable
	for(auto& iter : *parray_messageable)
	{
		// - call 'request_process_messagel'
		const auto result = iter->request_process_message(_msg);

		// check) return if processed
		if(result != eRESULT::BYPASS)
			return result;
	}

	// return)
	return result_code(eRESULT::BYPASS);
}


}