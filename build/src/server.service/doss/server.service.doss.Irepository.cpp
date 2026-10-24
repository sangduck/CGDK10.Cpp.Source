#include "pch.h"

namespace CGDK
{

bool doss::Irepository::wait()
{
	// 1) wait
	auto pthread_now = api::get_current_thread();

	// 2) notify  blocked
	pthread_now->notify_blocked();

	// 3) wait
	try
	{
		SCOPED_LOCK_NAMED(m_lockable_wait, cs)
		{
			// check)
			RETURN_IF(get_object_state() == eOBJECT_STATE::ESTABLISHED, true);

			// - wait util complete
			m_cv_complete.wait(cs);
		}
	}
	catch (...)
	{
		// - notify  blocked
		pthread_now->notify_unblocked();

		// reraise)
		throw;
	}

	// 6) notify unblocked
	pthread_now->notify_unblocked();

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction(object_ptr<Itransactable> _ptransactable)
{
	// declare)
	result_code	result = eRESULT::NOT_READY;

	// check)
	if (get_object_state() < eOBJECT_STATE::STOP_PENDING)
		co_return result;

	// 1) set the state to eRESULT::REQUEST
	_ptransactable->m_result = eRESULT::REQUEST;

	// 2) call process_entity_transaction_request of the session object (noexcept)
	if (_ptransactable->m_psession.exist())
	{
		_ptransactable->m_psession->process_entity_transaction_request(_ptransactable.get());
	}

	try
	{
		// check)
		THROW_IF(get_object_state() != eOBJECT_STATE::ESTABLISHED, throwable::failure(eRESULT::NOT_READY));

		switch (_ptransactable->m_type)
		{
		// co_request_transaction_checkin
		case	eTRANSACTION_TYPE::CHECKIN:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkin_request);
				result = co_await co_request_transaction_checkin(_ptransactable);
				break;

		// co_request_transaction_command
		case	eTRANSACTION_TYPE::COMMAND:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_command_request);
				result = co_await co_request_transaction_command(_ptransactable);
				break;

		// co_request_transaction_update
		case	eTRANSACTION_TYPE::UPDATE:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_update_request);
				result = co_await co_request_transaction_update(_ptransactable);
				break;

		// co_request_transaction_checkout
		case	eTRANSACTION_TYPE::CHECKOUT:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkout_request);
				result = co_await co_request_transaction_checkout(_ptransactable);
				break;

		// co_request_transaction_create
		case	eTRANSACTION_TYPE::CREATE:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_create_request);
				result = co_await co_request_transaction_create(_ptransactable);
				break;

		// co_request_transaction_remove
		case	eTRANSACTION_TYPE::REMOVE:
				doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_remove_request);
				result = co_await co_request_transaction_remove(_ptransactable);
				break;

		default:
				CGASSERT_ERROR(false);
		}
	}
	catch (throwable::failure & _e)
	{
		// - set result
		result = _e.reason();
	}
	catch (...)
	{
		// - set result
		result = eRESULT::EXCEPTION;
	}

	// - the result code of the transaction is stored on the transactable (the entity completes it)
	_ptransactable->_set_result_if_not_set(result);

	// return)
	co_return result;
}

bool doss::Irepository::process_complete_transaction(Itransactable* _ptransactable) noexcept
{
	// declare)
	bool result = false;

	// 1) process command
	switch (_ptransactable->m_type)
	{
	// process_complete_transaction_checkin
	case	eTRANSACTION_TYPE::CHECKIN:
			result = process_complete_transaction_checkin(_ptransactable);
			break;

	// process_complete_transaction_command
	case	eTRANSACTION_TYPE::COMMAND:
			result = process_complete_transaction_command(_ptransactable);
			break;

	// process_complete_transaction_update
	case	eTRANSACTION_TYPE::UPDATE:
			result = process_complete_transaction_update(_ptransactable);
			break;

	// process_complete_transaction_checkout
	case	eTRANSACTION_TYPE::CHECKOUT:
			result = process_complete_transaction_checkout(_ptransactable);
			break;

	// process_complete_transaction_create
	case	eTRANSACTION_TYPE::CREATE:
			result = process_complete_transaction_create(_ptransactable);
			break;

	// process_complete_transaction_remove
	case	eTRANSACTION_TYPE::REMOVE:
			result = process_complete_transaction_remove(_ptransactable);
			break;

	default:
			CGASSERT_ERROR(false);
	}

	// return)
	return	result;
}

void doss::Irepository::process_fail_transaction(Itransactable* /*_ptransactable*/) noexcept
{
}

void doss::Irepository::process_cancel_checkin(Ientity* _pentity, sCHECKIN_ID _cid) noexcept
{
	_pentity->process_remove_cid(_cid);
}


coroutine::task<result_code> doss::Irepository::co_request_transaction_checkin(object_ptr<Itransactable> _ptransactable)
{
	// check) '_ptransactable->m_pentity' must exist
	CGASSERT_ERROR(_ptransactable->m_pentity.exist());

	// declare)
	result_code result;

	if (_ptransactable->m_cid.id_entity.exist())
	{
		// check)
		CGASSERT_ERROR(_ptransactable->m_cid.id_entity == _ptransactable->m_pentity->m_eid);

		// 1) generate new cid
		_ptransactable->m_cid = process_generate_new_cid(_ptransactable->m_cid);

		// 2) call 'on_request_checkin'
		result = co_await on_request_checkin(_ptransactable);
	}
	else
	{
		// - the same cid is checked in already : a cancel (m_psession->process_entity_transaction_request made the cid empty)
		_ptransactable->set_result(eRESULT::ALREADY_DONE);

		// - set result
		result = eRESULT::ALREADY_DONE;
	}

	// return)
	co_return result;
}

bool doss::Irepository::process_complete_transaction_checkin(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::NOT_DEFINED);

	// check)
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::PENDED);

	// declare)
	auto pentity = _ptransactable->m_pentity;

	if(_ptransactable->m_result == eRESULT::SUCCESS)
	{
		// 1) generate new cid
		sCHECKIN_ID	cid_new = _ptransactable->m_cid;

		// check) the cid must be valid
		CGASSERT_ERROR(cid_new.valid() == true);

		// check) this must not happen
		RETURN_IF(cid_new.empty(), false);

		SCOPED_LOCK(m_lockable_map_entity)
		{
			try
			{
				// 2) the login option
				auto option_login = eTRANSACTION_OPTION::EXCLUSIVE_CHECKIN;

 				// 3) process by the login option
				switch (option_login)
				{
				// case A) exclusive checkin : one checkin per eid (every other session is force-checked-out)
				case	eTRANSACTION_OPTION::EXCLUSIVE_CHECKIN:
						pentity->process_insert_cid_and_remove_existing_cid(_ptransactable);
						break;

				// case B) reject a new checkin if one exists already
				case	eTRANSACTION_OPTION::REJECT_IF_ALREADY_EXIST:
						pentity->process_insert_cid_posterior(_ptransactable);
						break;

				// case C) allow duplicated checkin : the existing same cid is removed
				case	eTRANSACTION_OPTION::ALLOW_DUPLICATED_CHECKIN:
						pentity->process_insert_cid_duplicate(_ptransactable);
						break;

				// case D) transfer checkin : replaces the existing checkin (the cid must exist : NOT_EXIST otherwise)
				case	eTRANSACTION_OPTION::TRANSFER_CHECKIN:
						pentity->process_transfer_cid_to_other_session(_ptransactable);
						break;

				// default) undefined
				default:
						throw throwable::failure(eRESULT::EXCEPTION);
						break;
				}
			}
			catch (throwable::failure & _e)
			{
				// - set result
				_ptransactable->m_result = _e.reason();

				// - rollback command
				process_fail_transaction(_ptransactable);
			}
			catch (...)
			{
				// - set result
				_ptransactable->m_result = eRESULT::EXCEPTION;

				// - rollback command
				process_fail_transaction(_ptransactable);
			}
		}
	}

	// 7) ...
	if (_ptransactable->m_result == eRESULT::SUCCESS)
	{
		// statistics)
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkin_success);

		// !note) do not decrease the lock count on success
	}
	else
	{
		// statistics)
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkin_fail);
	}

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction_command(object_ptr<Itransactable> _ptransactable)
{
	// 1) do not proceed if the cid is empty
	if (_ptransactable->m_cid.empty())
	{
		_ptransactable->set_result(eRESULT::NOT_READY);

		// return)
		co_return eRESULT::NOT_READY;
	}

	// 2) call 'on_request_command' and return
	co_return co_await on_request_command(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_command(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_FATAL(_ptransactable != nullptr);

	// statistics)
	if (_ptransactable->m_result == eRESULT::SUCCESS)
	{
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_command_success);
	}
	else
	{
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_command_fail);
	}

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction_update(object_ptr<Itransactable> _ptransactable)
{
	co_return co_await on_request_update(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_update(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_FATAL(_ptransactable != nullptr);

	// statistics) (update result : counted in command statistics before)
	if(_ptransactable->m_result == eRESULT::SUCCESS)
	{
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_update_success);
	}
	else
	{
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_update_fail);
	}

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction_checkout(object_ptr<Itransactable> _ptransactable)
{
	// check)
	THROW_IF(_ptransactable->m_cid.empty(), throwable::failure(eRESULT::NOT_READY));

	// 1) copy pentity value temporary
	auto* pentity = _ptransactable->m_pentity.get();

	// check)
	CGASSERT_ERROR(pentity != nullptr);

	// check) return if pentity is nullptr
	if (pentity == nullptr)
		co_return eRESULT::NOT_EXIST;

	SCOPED_LOCK(m_lockable_map_entity)
	{
		// 2) remove cid
		auto is_success = pentity->process_remove_cid(_ptransactable->m_psession.get());

		// check)
		THROW_IF(is_success == false, throwable::failure(eRESULT::NOT_EXIST));
	}

	// return)
	co_return co_await on_request_checkout(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_checkout(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_ERROR(_ptransactable != nullptr);

	// check) return if _ptransactable is nullptr
	RETURN_IF(_ptransactable == nullptr, false);

	// statistics)
	if (_ptransactable->m_result == eRESULT::SUCCESS)
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkout_success);
	else
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_checkout_fail);

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction_create(object_ptr<Itransactable> _ptransactable)
{
	// 1) set result - must
	_ptransactable->set_result(eRESULT::SUCCESS);

	// return)
	co_return eRESULT::SUCCESS;
}

bool doss::Irepository::process_complete_transaction_create(Itransactable* _ptransactable) noexcept
{
	// check)
	RETURN_IF(_ptransactable == nullptr, false);

	// statistics)
	if (_ptransactable->m_result == eRESULT::SUCCESS)
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_create_success);
	else
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_create_fail);

	// return)
	return true;
}

coroutine::task<result_code> doss::Irepository::co_request_transaction_remove(object_ptr<Itransactable> _ptransactable)
{
	// check)
	auto* pentity = _ptransactable->m_pentity.get();
	THROW_IF(pentity == nullptr, throwable::failure(eRESULT::NOT_EXIST));

	// check) checked in (used by sessions) : check out first
	THROW_IF(pentity->get_checkin_count() != 0, throwable::failure(eRESULT::INVALID_STATE));

	// 1) set result - must (removed from this repository on completion)
	_ptransactable->set_result(eRESULT::SUCCESS);

	// return)
	co_return eRESULT::SUCCESS;
}

bool doss::Irepository::process_complete_transaction_remove(Itransactable* _ptransactable) noexcept
{
	// check)
	RETURN_IF(_ptransactable == nullptr, false);

	// check) failed
	RETURN_IF(_ptransactable->m_result != eRESULT::SUCCESS, true);

	// 1) get entity
	auto* pentity = _ptransactable->m_pentity.get();

	// check)
	RETURN_IF(pentity == nullptr, false);

	// check) not removable now : checked in again or other transactions are queued after this one
	//  (they would be processed on an entity which is no more in this repository)
	RETURN_IF(pentity->get_checkin_count() != 0 || pentity->get_queued_transaction_count() > 1, false);

	// 2) remove from this repository
	if (this->process_entity_remove(pentity))
		doss::statistics::repository::increase(Irepository::m_statistics.m_statistics_remove_complete);

	// return)
	return true;
}

sCHECKIN_ID doss::Irepository::process_generate_new_cid(sCHECKIN_ID& _cid_request) noexcept
{
	// static)
	static std::atomic<uint64_t> static_id_checkin_generate = 0;

	// declare)
	sCHECKIN_ID	cid;

	// 1) set...
	cid.id_entity	 = sENTITY_ID{ get_repository_id(), _cid_request.entity_id().object_id() };
	cid.id_session	 = _cid_request.id_session;
	cid.id_object	 = (static_id_checkin_generate.fetch_add(1, std::memory_order_relaxed) + 1); if(cid.id_object.empty()) { cid.id_object = (static_id_checkin_generate.fetch_add(1, std::memory_order_relaxed) + 1); }
	cid.key_certify	 = (static_cast<uint64_t>(rand()) << 32) | rand();

	if (cid.id_object.empty())
	{
		cid.id_object = (static_id_checkin_generate.fetch_add(1, std::memory_order_relaxed) + 1);
	}

	// return)
	return cid;
}

}
