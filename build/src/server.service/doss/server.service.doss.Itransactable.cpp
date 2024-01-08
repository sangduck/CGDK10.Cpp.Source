#include "pch.h"

namespace CGDK
{

void doss::Itransactable::reset() noexcept
{
	scoped_lock(m_lockable_state)
	{
		// check)
		CGASSERT_ERROR(m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr == nullptr);

		// 1) Command는 NONE으로
		m_type = eTRANSACTION_TYPE::NONE;
		m_option = 0;

		// 2) Request 정보를 초기화한다.
		m_rid_source.reset();
		m_rid.reset();
		m_buf_request.clear();
		m_f_request = nullptr;

		// 3) Completion 정보를 초기화한다.
		m_result = eRESULT::NOT_DEFINED;
		m_buf_result.clear();
		m_f_completion = nullptr;

		// 4) 기타...
		m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>() = nullptr; 
		m_prepository.reset();
		m_psession.reset();
		m_pentity.reset();
		m_cid.reset();
	}
}

void doss::Itransactable::set_result(result_code _result, const shared_buffer& _buffer)
{
	scoped_lock(m_lockable_state)
	{
		// check) 이미 Result 정보가 설정된 상태에서 다시 Result를 설정했을 경우!
		CGASSERT_ERROR(m_result == eRESULT::REQUEST);

		// check) 이미 Result 정보가 설정되어 있으면 Exception을 던진다. (이중 Result 설정 오류를 방지하기 위해...)
		THROW_IF(m_result != eRESULT::REQUEST, throwable::failure(eRESULT::EXPIRED))

		// 1) Result를 써넣는다.
		m_result = _result;

		// 2) Result를 써넣는다.
		m_buf_result = _buffer;
	}
}

void doss::Itransactable::set_result(result_code _result)
{
	scoped_lock(m_lockable_state)
	{
		// check) 이미 Result 정보가 설정된 상태에서 다시 Result를 설정했을 경우!
		CGASSERT_ERROR(m_result == eRESULT::REQUEST);

		// check) 이미 설정되어 있으면 Exception을 던진다.
		THROW_IF(m_result != eRESULT::REQUEST, throwable::failure(eRESULT::EXPIRED))

		// 1) Result를 써넣는다.
		m_result = _result;

		// 2) 버퍼는 초기화 한다.
		m_buf_result.clear();
	}
}

bool doss::Itransactable::cancel() noexcept
{
	// declare) 
	object_ptr<Ientity>	pentity;

	// 1) ..
	scoped_lock(m_lockable_state)
	{
		pentity = m_pentity;
	}

	// check) 
	RETURN_IF(m_pentity.empty(), false);

	// 2) cancel
	return pentity->cancel_transaction(this);
}

void doss::Itransactable::complete_transaction() noexcept
{
	// declare) 
	object_ptr<Ientity>	pentity;

	// 1) get entity object
	scoped_lock(m_lockable_state)
	{
		pentity = m_pentity;
	}

	// check) return if entity is empty
	RETURN_IF(pentity.empty());

	// 2) complete command
	pentity->complete_transaction(this);
}

void doss::Itransactable::complete_transaction(result_code _result) noexcept
{
	// check) Result가 REQUEST나 COMPLETE면 안됀다.
	CGASSERT_ERROR(_result != eRESULT::REQUEST);
	CGASSERT_ERROR(_result != eRESULT::COMPLETE);

	// declare) 
	object_ptr<Ientity>	pentity;

	scoped_lock(m_lockable_state)
	{
		// check) eRESULT::REQUEST 상태여야지만 한다.
		CGASSERT_ERROR(m_result == eRESULT::REQUEST)

		// check) 이미 설정되어 있으면 Exception을 던진다.
		RETURN_IF(m_result != eRESULT::REQUEST)

		// 1 )get entity
		pentity = m_pentity;

		// check) return if pentity is empty
		RETURN_IF(pentity.empty());

		// 2) store result value
		m_result = _result;
	}

	// 4) complete command
	complete_transaction();
}

void doss::Itransactable::complete_transaction(result_code _result, const shared_buffer& _buffer) noexcept
{
	// check) Result가 REQUEST나 COMPLETE면 안됀다.
	CGASSERT_ERROR(_result != eRESULT::REQUEST);
	CGASSERT_ERROR(_result != eRESULT::COMPLETE);

	// declare) 
	object_ptr<Ientity>	pentity;

	scoped_lock(m_lockable_state)
	{
		// check) eRESULT::REQUEST 상태여야지만 한다.
		CGASSERT_ERROR(m_result == eRESULT::REQUEST)

		// check) 이미 설정되어 있으면 Exception을 던진다.
		RETURN_IF(m_result != eRESULT::REQUEST)

		// 1 )get entity
		pentity = m_pentity;

		// check) return if pentity is empty
		RETURN_IF(pentity.empty());

		// 2) store result value
		m_result = _result;

		// 3) store result buffer
		m_buf_result = _buffer;
	}

	// 4) complete command
	complete_transaction();
}

intptr_t doss::Itransactable::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check)
	CGASSERT_ERROR(m_prepository.exist());

	// 1) process_transaction
	auto result = m_prepository->process_transaction(this);

	// 2)
	if (result != eRESULT::PENDED)
	{
		complete_transaction();
	}

	// return) 
	return 0;
}


}