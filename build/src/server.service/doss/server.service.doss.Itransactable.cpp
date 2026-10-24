#include "pch.h"

namespace CGDK
{

void doss::Itransactable::reset() noexcept
{
	SCOPED_LOCK(m_lockable_state)
	{
		// check)
		CGASSERT_ERROR(m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>()._Ptr == nullptr);

		// 1) set type NONE
		m_type = eTRANSACTION_TYPE::NONE;
		m_option = 0;

		// 2) reset the request
		m_rid_source.reset();
		m_rid.reset();
		m_buf_request.clear();

		// 3) reset the result
		m_result = eRESULT::NOT_DEFINED;
		m_buf_result.clear();

		// 4) reset the awaiters (a reused transactable gets new ones)
		m_completion.reset();
		m_response.reset();

		// 5) ...
		m_iter_pending.get<circular_list<object_ptr<Itransactable>>::iterator>() = nullptr;
		m_prepository.reset();
		m_psession.reset();
		m_pentity.reset();
		m_cid.reset();
	}
}

void doss::Itransactable::set_result(result_code _result, const shared_buffer& _buffer)
{
	SCOPED_LOCK(m_lockable_state)
	{
		// check) the result must be set only once
		CGASSERT_ERROR(m_result == eRESULT::REQUEST);

		// check) throw if the result was set already (no double set)
		THROW_IF(m_result != eRESULT::REQUEST, throwable::failure(eRESULT::EXPIRED))

		// 1) set the result
		m_result = _result;

		// 2) set the result buffer
		m_buf_result = _buffer;
	}
}

void doss::Itransactable::_set_result_if_not_set(result_code _result) noexcept
{
	SCOPED_LOCK(m_lockable_state)
	{
		// check) already set
		if (m_result != eRESULT::REQUEST && m_result != eRESULT::NOT_DEFINED)
			return;

		// 1) not a final result (still requesting / not defined) => FAIL
		if (_result == eRESULT::REQUEST || _result == eRESULT::COMPLETE || _result == eRESULT::NOT_DEFINED)
			_result = eRESULT::FAIL;

		// 2) set
		m_result = _result;
		m_buf_result.clear();
	}
}

void doss::Itransactable::set_result(result_code _result)
{
	SCOPED_LOCK(m_lockable_state)
	{
		// check) the result must be set only once
		CGASSERT_ERROR(m_result == eRESULT::REQUEST);

		// check) throw if the result was set already (no double set)
		THROW_IF(m_result != eRESULT::REQUEST, throwable::failure(eRESULT::EXPIRED))

		// 1) set the result
		m_result = _result;

		// 2) clear the result buffer
		m_buf_result.clear();
	}
}

bool doss::Itransactable::cancel() noexcept
{
	// declare)
	object_ptr<Ientity>	pentity;

	// 1) ..
	SCOPED_LOCK(m_lockable_state)
	{
		pentity = m_pentity;
	}

	// check)
	RETURN_IF(pentity.empty(), false);

	// 2) cancel
	return pentity->cancel_transaction(this);
}

}
