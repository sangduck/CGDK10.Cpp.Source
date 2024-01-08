#include "pch.h"

namespace CGDK
{
	

object_ptr<doss::Isession> doss::Ientity::find_session(const sCHECKIN_ID& _cid) const noexcept
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(const_cast<lockable<>&>(m_lockable_entity));

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
	// 1) complete command
	auto ptransactable_next = process_complete_transaction(_ptransactable);

	// 2) execute net transactable
	while (ptransactable_next.exist())
	{
		// - call 'process_request_transaction'
		auto result = m_prepository->process_request_transaction(ptransactable_next);

		// check) return if result is eRESULT::PENDED
		BREAK_IF(result == eRESULT::PENDED);

		// - process complete command and get next command
		ptransactable_next = process_complete_transaction(ptransactable_next);
	}

	// return) 
	return true;
}

void doss::Ientity::checkout_all() noexcept
{
	// declare) 
	std::vector<std::pair<sCHECKIN_ID, object_ptr<Isession>>> list_cid;

	// 1) copy
	scoped_lock(m_lockable_entity)
	{
		list_cid = m_list_cid;
	}

	// check)
	RETURN_IF(list_cid.empty());

	// 2) checkout all
	for (auto& iter : m_list_cid)
	{
		// - Transactable을 생성한다.
		auto ptransactable = make_object<Itransactable>();

		// - 값을 설정한다.
		ptransactable->m_type = eTRANSACTION_TYPE::CHECKOUT;
		ptransactable->m_prepository = m_prepository;
		ptransactable->m_psession = iter.second;
		ptransactable->m_pentity = this;
		ptransactable->m_cid = iter.first;

		// - request transaction
		request_transaction(ptransactable);
	}
}

result_code doss::Ientity::process_request_transaction(Itransactable* _ptransactable)
{
	// check) _ptransactable이 nullptr이면안됀다.
	CGASSERT_ERROR(_ptransactable != nullptr);

	// check) _ptransactable의 Transaction state는 반드시 eRESULT::NOT_DEFINED상태여야 한다.
	CGASSERT_ERROR(_ptransactable->m_result == eRESULT::NOT_DEFINED);

	// check) m_iter_pending._Ptr이 nullptr이어야만 한다.
	CGASSERT_ERROR(_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr == nullptr);

	// check) m_iter_pending._Ptr이 nullptr이어야만 한다.
	THROW_IF(_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr != nullptr, throwable::failure(eRESULT::ALEADY_EXIST));

	// declare)
	bool is_exists;

	scoped_lock(m_lockable_queue_transaction)
	{
		// 1) is queue empty?
		is_exists = !m_queue_command.empty();

		// 2) queing first
		m_queue_command.push_back(_ptransactable);

		// 3) set value
		_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>() = --m_queue_command.end();
		_ptransactable->m_prepository = m_prepository;
		_ptransactable->m_pentity = this;
		_ptransactable->m_rid = generate_request_id();
	}

	// check) return eRESULT::PENDED, if empty
	RETURN_IF(is_exists, eRESULT::PENDED);

	// 4) TransactionRequest 호출
	auto result = m_prepository->process_request_transaction(_ptransactable);

	// 5) Pending이 아니라면 바로 완료 처리를 한다.(계속적 Recursion으로 불안정)
	if (result != eRESULT::PENDED)
	{
		complete_transaction(_ptransactable);
	}

	// return)
	return result;
}

object_ptr<doss::Itransactable> doss::Ientity::process_complete_transaction(Itransactable* _ptransactable) noexcept
{
	// check) _ptransactable의 m_pTransactor는 this여야만 한다.
	CGASSERT_ERROR(_ptransactable->m_pentity == this);

	// check)
	CGASSERT_ERROR(m_prepository.exist());

	// check) 
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::COMPLETE && _ptransactable->m_result != eRESULT::NOT_DEFINED && _ptransactable->m_result != eRESULT::REQUEST);

	// 1) OnComplete 수행
	m_prepository->process_complete_transaction(_ptransactable);

	// declare)
	object_ptr<Itransactable> ptransactable_next;

	scoped_lock(m_lockable_queue_transaction)
	{
		// check) 
		CGASSERT_ERROR(m_queue_command.empty() == false);

		// check)
		CGASSERT_ERROR(m_queue_command.front() == _ptransactable);

		// 2) pop front
		m_queue_command.pop_front();
		_ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr = nullptr;
		_ptransactable->m_pentity.reset();
		_ptransactable->m_psession.reset();

		// check) return if queue is empty
		if (m_queue_command.empty() == false)
		{
			// 3) stor next command object if exist
			ptransactable_next = m_queue_command.front();

			// check) _ptransactable의 m_pTransactor는 this여야만 한다.
			CGASSERT_ERROR(ptransactable_next->m_pentity == this);
		}
	}

	// return)
	return ptransactable_next;
}

bool doss::Ientity::process_cancel_command(Itransactable* _ptransactable) noexcept
{
	scoped_lock(m_lockable_queue_transaction)
	{
		// check) return if queue is empty
		RETURN_IF(m_queue_command.empty(), false);

		// check) return if aleady executing
		RETURN_IF(m_queue_command.front() == _ptransactable, false);

		// 1) get iterator
		auto& iter = _ptransactable->m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>();

		// check) fail if processing transataction
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

	scoped_lock(m_lockable_entity)
	{
		// check) 이미 동일한 checkin이 존재하면 exception을 던진다.
		for (auto& iter : m_list_cid)
		{
			THROW_IF(iter.first.id_session == cid.id_session, throwable::failure(eRESULT::ALEADY_EXIST));
		}

		// 1) swap  cid ...
		list_cid_old.swap(m_list_cid);

		// 2) insert new
		m_list_cid.push_back({ _ptransactable->m_cid, psession });

		// 3) psession에 pentity를 설정한다.
		psession->process_entity_set(_ptransactable);

		// statistics) 
		++m_prepository->m_statistics.m_statistics_checkin;
	}

	// 4) force checkout
	for (auto& iter : list_cid_old)
	{
		// - reset entity from session
		auto result = iter.second->process_entity_reset(this, iter.first, true);

		if (result)
		{
			// statistics) 
			--m_prepository->m_statistics.m_statistics_checkin;
			++m_prepository->m_statistics.m_statistics_checkout_forcible;
		}
	}
}
void doss::Ientity::process_insert_cid_posterior(Itransactable* _ptransactable)
{
	// 설명) 후순위 checkin
	//       - 기존에 checin된 것이 있으면 무조건 거부된다.
	std::pair pair_new{ _ptransactable->m_cid, _ptransactable->m_psession };

	scoped_lock(m_lockable_entity)
	{
		// check) 기존에 존재하는 ehcekin이 있으면 무조건 거부된다.
		THROW_IF(m_list_cid.empty() == false, throwable::failure(eRESULT::ALEADY_EXIST));

		// 1) 설정한다.
		pair_new.second->process_entity_set(_ptransactable);

		// statistics) 
		++m_prepository->m_statistics.m_statistics_checkin;

		// 2) add new
		m_list_cid.emplace_back(std::move(pair_new));
	}
}
void doss::Ientity::process_insert_cid_duplicate(Itransactable* _ptransactable)
{
	std::pair<sCHECKIN_ID, object_ptr<Isession>> pair_new{ _ptransactable->m_cid, _ptransactable->m_psession };

	// declare) 
	object_ptr<Isession> psession_old;

	scoped_lock(m_lockable_entity)
	{
		// 1) 동일한 cid가 존재하는지 확인한다.
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

		// 2) 설정한다.
		pair_new.second->process_entity_set(_ptransactable);  // pair_new.first, this);

		// statistics) 
		++m_prepository->m_statistics.m_statistics_checkin;

		// 3-1) 존재하지 않을 경우 => 신규로 추가한다.
		if (iter == m_list_cid.end())
		{
			m_list_cid.emplace_back(pair_new);
		}
		// 3-2) 존재하는 경우 => 업데이트 한다.
		else
		{
			psession_old = iter->second;
			iter->second = pair_new.second;
		}

		// 4) psession_old가 존재하면 제거한다.
		if (psession_old.exist())
		{
			// - reset entity from session
			psession_old->process_entity_reset(this, _ptransactable->m_cid, true);

			// statistics) 
			--m_prepository->m_statistics.m_statistics_checkin;
			++m_prepository->m_statistics.m_statistics_checkout_forcible;
		}
	}
}
void doss::Ientity::process_transfer_cid_to_other_session(Itransactable* _ptransactable)
{
	// 설명) cid 이전 처리
	//  - 기존에 존재하는 cid를 찾아 _new의 session로 대체하고 기존의 psession을 리턴해준다.
	//  - 만약 기존의 cid를 찾지 못했을 경우 nullptr을 리턴한다.

	// declare)
	auto cid_new = _ptransactable->m_cid;
	Isession* psession = _ptransactable->m_psession;

	// check) 
	CGASSERT_ERROR(psession != nullptr);

	// declare) 
	object_ptr<Isession> psession_old;

	scoped_lock(m_lockable_entity)
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

				// - 설정한다.
				psession->process_entity_set(_ptransactable);

				// statistics) 
				++m_prepository->m_statistics.m_statistics_checkin;

				// - update value (순서주의 - process_entity_set에서 exception을 던질 수 있으므로...)
				iter->second = psession;

				// break) 
				break;
			}
		}

		// check) 기존 session이 empty()면 기존에 checkin된 session이 없었다는 것이므로 실패다.
		THROW_IF(psession_old.empty(), throwable::failure(eRESULT::NOT_EXIST));

		// - force checkout old entity
		auto result = psession_old->process_entity_reset(this, cid_new, true);

		if (result)
		{
			// statistics) 
			--m_prepository->m_statistics.m_statistics_checkin;
			++m_prepository->m_statistics.m_statistics_checkout_forcible;
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
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_entity);

		// 1) find
		auto iter_find = std::find_if(m_list_cid.begin(), m_list_cid.end(), [=](const std::pair<sCHECKIN_ID, object_ptr<Isession>>& _iter) { return _iter.second == _psession; });

		// check)
		RETURN_IF(iter_find == m_list_cid.end(), false);

		// 2) store old
		cid_old = iter_find->first;

		// 3) reset session cid
		auto result = iter_find->second->process_entity_reset(this, cid_old, false);

		if (result)
		{
			// statistics) 
			--m_prepository->m_statistics.m_statistics_checkin;
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
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(m_lockable_entity);

		// 1) find
		auto iter_find = std::find_if(m_list_cid.begin(), m_list_cid.end(), [=](const std::pair<sCHECKIN_ID, object_ptr<Isession>>& _iter) { return _iter.first == _cid; });

		// check)
		RETURN_IF(iter_find == m_list_cid.end(), false);

		// 2) store old
		psession = iter_find->second;

		// 6) reset session cid
		auto result = iter_find->second->process_entity_reset(this, iter_find->first, false);

		if (result)
		{
			// statistics) 
			--m_prepository->m_statistics.m_statistics_checkin;
		}

		// 3) erase
		m_list_cid.erase(iter_find);

		// return)
		return true;
	}
}

uint64_t doss::Ientity::generate_request_id() noexcept
{
	// 1) generate indexf
	auto index = ++m_index_request;

	// 2) retry if index is 0
	if (index == 0)
	{
		index = ++m_index_request;
	}

	// return)
	return index;
}


}