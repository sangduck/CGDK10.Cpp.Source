#include "pch.h"

namespace CGDK
{


object_ptr<doss::Isession> doss::Ientity::find_session(const sCHECKIN_ID& _cid) const noexcept
{
	// locked)
	{
		// SCOPED_LOCK)
		std::lock_guard cs(const_cast<lock_object&>(m_lockable_entity));

		// 1) find
		auto iter_find = std::find_if(m_list_cid.begin(), m_list_cid.end(), [=](const std::pair<sCHECKIN_ID, Isession*>& _iter) { return _iter.first == _cid; });

		// check) return nullptr if not exist
		RETURN_IF(iter_find == m_list_cid.end(), object_ptr<Isession>());

		// return)
		return	iter_find->second;
	}
}

bool doss::Ientity::complete_transaction(Itransactable* _ptransactable) noexcept
{
	// 1) complete the transaction and get the next queued one
	auto ptransactable_next = process_complete_transaction(_ptransactable);

	// 2) execute the next transaction (as a coroutine : it completes itself and gets the one after it)
	if (ptransactable_next.exist())
	{
		this->process_execute_transaction(std::move(ptransactable_next));
	}

	// return)
	return true;
}

void doss::Ientity::process_execute_transaction(object_ptr<Itransactable>&& _ptransactable) noexcept
{
	// - keep this entity and the transaction alive until the coroutine finishes
	try
	{
		// 1) make the transaction coroutine (the only coroutine frame of this transaction)
		auto task_transaction = m_prepository->co_request_transaction(_ptransactable);

		// 2) start it on the system_executor with a completion (no wrapper coroutine frame)
		//    - the completion runs in final_suspend of the transaction coroutine.
		//    - the entity and the transaction are kept alive by the completion until then.
		task_transaction.start([pself = object_ptr<Ireferenceable>(this), ptransaction = _ptransactable](std::exception_ptr /*_exception*/) mutable
			{
				// - complete the transaction (the next queued transaction is executed)
				//   co_request_transaction never throws out : the result is set already.
				ptransaction->m_pentity->complete_transaction(ptransaction.get());
			});
	}
	catch (...)
	{
		// - failed to even start the coroutine (ex. the executor is stopped / frame allocation failure)
		//   complete the transaction as a failure so the entity does not stall. (and run the next one)
		_ptransactable->_set_result_if_not_set(eRESULT::NOT_READY);
		this->complete_transaction(_ptransactable.get());
	}
}

void doss::Ientity::checkout_all() noexcept
{
	// declare)
	decltype(this->m_list_cid) list_cid;

	// 1) copy
	SCOPED_LOCK(this->m_lockable_entity)
	{
		// check)
		RETURN_IF(this->m_list_cid.empty(), );

		// - copy
		list_cid = this->m_list_cid;
	}

	// 2) checkout all
	for (auto& iter : list_cid)
	{
		try
		{
			// - alloc a transactable
			auto ptransactable = make_object<Itransactable>();

			// - set the values
			ptransactable->m_type = eTRANSACTION_TYPE::CHECKOUT;
			ptransactable->m_prepository = this->m_prepository;
			ptransactable->m_psession = iter.second;
			ptransactable->m_pentity = this;
			ptransactable->m_cid = iter.first;

			// - request transaction
			this->request_transaction(ptransactable);
		}
		catch (...)
		{
		}
	}
}

result_code doss::Ientity::process_request_transaction(Itransactable* _ptransactable)
{
	// check) _ptransactable must not be nullptr
	CGASSERT_FATAL(_ptransactable != nullptr);

	// check) the transaction state must be eRESULT::NOT_DEFINED
	CGASSERT_ERROR(_ptransactable->m_result == eRESULT::NOT_DEFINED);

	// check) m_iter_pending._Ptr must be nullptr
	CGASSERT_ERROR(_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr == nullptr);

	// check) m_iter_pending._Ptr must be nullptr
	THROW_IF(_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr != nullptr, throwable::failure(eRESULT::ALREADY_EXIST));

	// declare)
	bool is_exists;

	// - keep the transactable alive while queued (the entity owns it through the queue)
	object_ptr<Itransactable> ptransactable(_ptransactable);

	SCOPED_LOCK(m_lockable_queue_transaction)
	{
		// 1) is the queue not empty?
		is_exists = !m_queue_command.empty();

		// 2) queue it
		m_queue_command.push_back(ptransactable);

		// 3) set the values
		_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>() = --m_queue_command.end();
		_ptransactable->m_prepository = m_prepository;
		_ptransactable->m_pentity = this;
		_ptransactable->m_rid = generate_request_id();
	}

	// check) another transaction is being processed : this one waits in the queue (PENDED)
	RETURN_IF(is_exists, eRESULT::PENDED);

	// 4) execute the transaction (coroutine : it completes itself and runs the next queued one)
	this->process_execute_transaction(std::move(ptransactable));

	// return) the transaction runs asynchronously
	return eRESULT::PENDED;
}

object_ptr<doss::Itransactable> doss::Ientity::process_complete_transaction(Itransactable* _ptransactable) noexcept
{
	// check) _ptransactable->m_pentity must be this
	CGASSERT_ERROR(_ptransactable->m_pentity == this);

	// check)
	CGASSERT_ERROR(m_prepository.exist());

	// check)
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::COMPLETE && _ptransactable->m_result != eRESULT::NOT_DEFINED && _ptransactable->m_result != eRESULT::REQUEST);

	// 1) complete the transaction on the repository
	m_prepository->process_complete_transaction(_ptransactable);

	// 2) call the completion of the awaiting coroutine (session::co_checkin/co_checkout/co_request)
	if (_ptransactable->m_completion.has_value())
	{
		try
		{
			_ptransactable->m_completion->set_result(_ptransactable->m_result);
		}
		catch (...)
		{
		}
	}

	// 3) call process_entity_transaction_complete of the session (noexcept)
	if (_ptransactable->m_psession.exist())
	{
		_ptransactable->m_psession->process_entity_transaction_complete(_ptransactable);
	}

	// declare)
	object_ptr<Itransactable> ptransactable_next;

	SCOPED_LOCK(m_lockable_queue_transaction)
	{
		// check) the queue was cleared (the repository was destroyed : clear_references())
		if (m_queue_command.empty() || m_queue_command.front() != _ptransactable)
		{
			_ptransactable->m_pentity.reset();
			_ptransactable->m_psession.reset();
			return ptransactable_next;
		}

		// 2) pop front
		m_queue_command.pop_front();
		_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr = nullptr;
		_ptransactable->m_pentity.reset();
		_ptransactable->m_psession.reset();

		// check) return if the queue is empty
		if (m_queue_command.empty() == false)
		{
			// 3) store the next command object if it exists
			ptransactable_next = m_queue_command.front();

			// check) _ptransactable->m_pentity must be this
			CGASSERT_ERROR(ptransactable_next->m_pentity == this);
		}
	}

	// return)
	return ptransactable_next;
}

void doss::Ientity::clear_references() noexcept
{
	// 1) pending transactions (entity <-> transaction)
	circular_list<object_ptr<Itransactable>> queue_command;
	SCOPED_LOCK(m_lockable_queue_transaction)
	{
		queue_command = std::move(m_queue_command);
		m_queue_command.clear();
	}
	for (auto& iter : queue_command)
	{
		iter->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr = nullptr;
		iter->m_pentity.reset();
		iter->m_psession.reset();
	}

	// 2) checked-in sessions (entity <-> session)
	decltype(m_list_cid) list_cid;
	SCOPED_LOCK(m_lockable_entity)
	{
		list_cid.swap(m_list_cid);
	}
}

bool doss::Ientity::process_cancel_command(Itransactable* _ptransactable) noexcept
{
	SCOPED_LOCK(m_lockable_queue_transaction)
	{
		// check) return if the queue is empty
		RETURN_IF(m_queue_command.empty(), false);

		// check) return if already executing
		RETURN_IF(m_queue_command.front() == _ptransactable, false);

		// 1) get the iterator
		auto& iter = _ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>();

		// check) fail if it is the processing transaction
		RETURN_IF(iter == m_queue_command.begin(), false);

		// 2) erase
		m_queue_command.erase(iter);

		// 3) reset
		_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr = nullptr;
		_ptransactable->m_pentity.reset();
		_ptransactable->m_psession.reset();
	}

	// return)
	return true;
}


void doss::Ientity::process_insert_cid_and_remove_existing_cid(Itransactable* _ptransactable)
{
	// declare)
	Isession* psession = _ptransactable->m_psession;
	auto& cid = _ptransactable->m_cid;

	// check)
	CGASSERT_ERROR(cid.exist());

	// declare)
	std::vector<std::pair<sCHECKIN_ID, object_ptr<Isession>>> list_cid_old;

	SCOPED_LOCK(m_lockable_entity)
	{
		// check) throw if the same checkin exists already
		for (auto& iter : m_list_cid)
		{
			THROW_IF(iter.first.id_session == cid.id_session, throwable::failure(eRESULT::ALREADY_EXIST));
		}

		// 1) swap cid ...
		list_cid_old.swap(m_list_cid);

		// 2) insert new
		m_list_cid.push_back({ _ptransactable->m_cid, psession });

		// 3) set pentity on the session
		psession->process_entity_set(_ptransactable);

		// statistics)
		doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkin);
	}

	// 4) force checkout
	for (auto& iter : list_cid_old)
	{
		// - reset entity from session
		auto result = iter.second->process_entity_reset(this, iter.first, true);

		if (result)
		{
			// statistics)
			doss::statistics::repository::decrease(m_prepository->m_statistics.m_statistics_checkin);
			doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkout_forcible);
		}
	}
}
void doss::Ientity::process_insert_cid_posterior(Itransactable* _ptransactable)
{
	// - a posterior checkin : rejected if any checkin exists already
	std::pair pair_new{ _ptransactable->m_cid, _ptransactable->m_psession };

	SCOPED_LOCK(m_lockable_entity)
	{
		// check) rejected if any checkin exists already
		THROW_IF(m_list_cid.empty() == false, throwable::failure(eRESULT::ALREADY_EXIST));

		// 1) set
		pair_new.second->process_entity_set(_ptransactable);

		// statistics)
		doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkin);

		// 2) add new
		m_list_cid.emplace_back(std::move(pair_new));
	}
}
void doss::Ientity::process_insert_cid_duplicate(Itransactable* _ptransactable)
{
	std::pair<sCHECKIN_ID, object_ptr<Isession>> pair_new{ _ptransactable->m_cid, _ptransactable->m_psession };

	// declare)
	object_ptr<Isession> psession_old;

	SCOPED_LOCK(m_lockable_entity)
	{
		// 1) find the same cid
		auto iter = m_list_cid.begin();
		auto iter_end = m_list_cid.end();

		for (; iter != iter_end; ++iter)
		{
			if (iter->first.entity_id()  == pair_new.first.entity_id()
				&& iter->first.object_id()  == pair_new.first.object_id()
				&& iter->first.certify_id() == pair_new.first.certify_id())
			{
				// break)
				break;
			}
		}

		// 2) set
		pair_new.second->process_entity_set(_ptransactable);

		// statistics)
		doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkin);

		// 3-1) not found => add new
		if (iter == m_list_cid.end())
		{
			m_list_cid.emplace_back(pair_new);
		}
		// 3-2) found => update
		else
		{
			psession_old = iter->second;
			iter->second = pair_new.second;
		}

		// 4) remove psession_old if it exists
		if (psession_old.exist())
		{
			// - reset entity from session
			psession_old->process_entity_reset(this, _ptransactable->m_cid, true);

			// statistics)
			doss::statistics::repository::decrease(m_prepository->m_statistics.m_statistics_checkin);
			doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkout_forcible);
		}
	}
}
void doss::Ientity::process_transfer_cid_to_other_session(Itransactable* _ptransactable)
{
	// - transfers a cid : replaces the session of an existing cid with the new one and returns the old session.
	//   (throws NOT_EXIST if the cid does not exist)

	// declare)
	auto cid_new = _ptransactable->m_cid;
	Isession* psession = _ptransactable->m_psession;

	// check)
	CGASSERT_FATAL(psession != nullptr);

	// declare)
	object_ptr<Isession> psession_old;

	SCOPED_LOCK(m_lockable_entity)
	{
		// - find session
		auto iter = m_list_cid.begin();
		auto iter_end = m_list_cid.end();

		for (; iter != iter_end; ++iter)
		{
			if (iter->first.entity_id()  == cid_new.entity_id()
				&& iter->first.object_id()  == cid_new.object_id()
				&& iter->first.certify_id() == cid_new.certify_id())
			{
				// - store old
				psession_old = iter->second;

				// - set
				psession->process_entity_set(_ptransactable);

				// statistics)
				doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkin);

				// - update the value (order matters : process_entity_set may throw)
				iter->second = psession;

				// break)
				break;
			}
		}

		// check) fail if no existing session was checked in
		THROW_IF(psession_old.empty(), throwable::failure(eRESULT::NOT_EXIST));

		// - force checkout the old entity
		auto result = psession_old->process_entity_reset(this, cid_new, true);

		if (result)
		{
			// statistics)
			doss::statistics::repository::decrease(m_prepository->m_statistics.m_statistics_checkin);
			doss::statistics::repository::increase(m_prepository->m_statistics.m_statistics_checkout_forcible);
		}
	}
}
bool doss::Ientity::process_remove_cid(Isession* _psession) noexcept
{
	// check)
	RETURN_IF(_psession == nullptr, false);

	// declare)
	sCHECKIN_ID cid_old;

	// locked)
	{
		// SCOPED_LOCK)
		std::lock_guard cs(m_lockable_entity);

		// 1) find
		auto iter_find = std::find_if(m_list_cid.begin(), m_list_cid.end(), [=](const std::pair<sCHECKIN_ID, object_ptr<Isession>>& _iter) { return _iter.second == _psession; });

		// check)
		RETURN_IF(iter_find == m_list_cid.end(), false);

		// 2) store old
		cid_old = iter_find->first;

		// 3) reset the session cid
		auto result = iter_find->second->process_entity_reset(this, cid_old, false);

		if (result)
		{
			// statistics)
			doss::statistics::repository::decrease(m_prepository->m_statistics.m_statistics_checkin);
		}

		// 4) erase
		m_list_cid.erase(iter_find);

		// return)
		return true;
	}
}
bool doss::Ientity::process_remove_cid(const sCHECKIN_ID& _cid)
{
	// declare)
	object_ptr<Isession> psession;

	{
		// SCOPED_LOCK)
		std::lock_guard cs(m_lockable_entity);

		// 1) find
		auto iter_find = std::find_if(m_list_cid.begin(), m_list_cid.end(), [=](const std::pair<sCHECKIN_ID, object_ptr<Isession>>& _iter) { return _iter.first == _cid; });

		// check)
		RETURN_IF(iter_find == m_list_cid.end(), false);

		// 2) store old
		psession = iter_find->second;

		// 6) reset the session cid
		auto result = iter_find->second->process_entity_reset(this, iter_find->first, false);

		if (result)
		{
			// statistics)
			doss::statistics::repository::decrease(m_prepository->m_statistics.m_statistics_checkin);
		}

		// 3) erase
		m_list_cid.erase(iter_find);

		// return)
		return true;
	}
}

uint64_t doss::Ientity::generate_request_id() noexcept
{
	// 1) generate index
	auto index = (m_index_request.fetch_add(1, std::memory_order_relaxed) + 1);

	// 2) retry if the index is 0
	if (index == 0)
	{
		index = (m_index_request.fetch_add(1, std::memory_order_relaxed) + 1);
	}

	// return)
	return index;
}

}
