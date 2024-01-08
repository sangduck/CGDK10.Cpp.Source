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
		scoped_lock_named(m_lockable_wait, cs)
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

result_code doss::Irepository::process_request_transaction(Itransactable* _ptransactable)
{
	// 1) post execute
	auto is_success = system_executor::post(_ptransactable);

	// return)
	return (is_success) ? eRESULT::PENDED : eRESULT::FAIL;
}

result_code doss::Irepository::process_transaction(Itransactable* _ptransactable)
{
	// declare)
	result_code	result = eRESULT::NOT_READY;

	// check) 
	RETURN_IF(get_object_state() < eOBJECT_STATE::STOP_PENDING, result);

	// 1) 상태를 eRESULT::REQUEST로 변경한다.
	_ptransactable->m_result = eRESULT::REQUEST;

	// 2) call process_entity_transaction_complete of session object
	if (_ptransactable->m_psession.exist())
	{
		_ptransactable->m_psession->process_entity_transaction_request(_ptransactable); // noexcept
	}

	// 3) call m_f_request fuction if m_f_request is not nullptr
	if (_ptransactable->m_f_request != nullptr)
	{
		// m_f_request를 호출한다. (여기서 실패시 바로 실패)
		result = _ptransactable->m_f_request(_ptransactable);

		// check) 여기서 Pending이어서는 안됀다.
		CGASSERT_ERROR(result != eRESULT::PENDED);

		// check) SUCCESS가 아닐 경우는 리턴한다.(여기서는 PENDING도 해서는 안됀다.
		RETURN_IF(result != eRESULT::SUCCESS, eRESULT::INVALID_STATE);
	}

	try
	{
		// check) 
		THROW_IF(get_object_state() != eOBJECT_STATE::ESTABLISHED, throwable::failure(eRESULT::NOT_READY));

		switch (_ptransactable->m_type)
		{
		// process_request_transaction_checkin
		case	eTRANSACTION_TYPE::CHECKIN:
				{
					// statistics) 
					++Irepository::m_statistics.m_statistics_checkin_request;

					result = process_request_transaction_checkin(_ptransactable);
				}
				break;

		// process_request_transaction_command
		case	eTRANSACTION_TYPE::COMMAND:
				{
					// statistics)
					++Irepository::m_statistics.m_statistics_command_request;

					result = process_request_transaction_command(_ptransactable);
				}
				break;

		// process_request_transaction_update
		case	eTRANSACTION_TYPE::UPDATE:
				{
					// statistics)
					++Irepository::m_statistics.m_statistics_update_request;

					result = process_request_transaction_update(_ptransactable);
				}
				break;

		// process_request_transaction_checkout
		case	eTRANSACTION_TYPE::CHECKOUT:
				{
					// statistics)
					++Irepository::m_statistics.m_statistics_checkout_request;

					result = process_request_transaction_checkout(_ptransactable);
				}
				break;

		// process_request_transaction_create
		case	eTRANSACTION_TYPE::CREATE:
				{
					// statistics)
					++Irepository::m_statistics.m_statistics_create_request;

					result = process_request_transaction_create(_ptransactable);
				}
				break;

		// process_request_transaction_remove
		case	eTRANSACTION_TYPE::REMOVE:
				{
					// statistics)
					++Irepository::m_statistics.m_statistics_remove_request;

					result = process_request_transaction_remove(_ptransactable);
				}
				break;

		default:
				CGASSERT_ERROR(false);
		}
	}
	catch (throwable::failure & _e)
	{
		// - set result
		result = _e.reason();

		// - set result
		_ptransactable->set_result(result);
	}
	catch (...)
	{
		// - set result
		result = eRESULT::EXCEPTION;

		// - set result
		_ptransactable->set_result(result);
	}

	// check)
	CGASSERT_ERROR(result != eRESULT::NOT_DEFINED);

	// return)
	return	result;
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

	default:
			CGASSERT_ERROR(false);
	}

	// 2) _fCompletion함수를 호출한다.
	if (_ptransactable->m_f_completion != nullptr)
	{
		try
		{
			_ptransactable->m_f_completion(_ptransactable);
		}
		catch (...)
		{
		}
	}

	// 3) call process_entity_transaction_complete of session object
	if (_ptransactable->m_psession.exist())
	{
		_ptransactable->m_psession->process_entity_transaction_complete(_ptransactable); // noexcept
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


result_code doss::Irepository::process_request_transaction_checkin(Itransactable* _ptransactable)
{
	// check) '_ptransactable' must not be nullptr
	CGASSERT_ERROR(_ptransactable != nullptr);

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
		result = on_request_checkin(_ptransactable);
	}
	else
	{
		// 설명) 이미 동일한 cid로 checkin이 이미 되어 있다면 이것은 취소
		//       만약 cid가 동일하다면 m_psession->process_entity_transaction_request에서 cid가 empty가 되어 온다.
		_ptransactable->set_result(eRESULT::ALEADY_DONE);

		// - set result
		result = eRESULT::ALEADY_DONE;;
	}

	// return)
	return result;
}

bool doss::Irepository::process_complete_transaction_checkin(Itransactable* _ptransactable) noexcept
{
	// check) 
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::NOT_DEFINED);
	 
	// check) 
	CGASSERT_ERROR(_ptransactable->m_result != eRESULT::PENDED);

	// declare) 
	auto pentity = _ptransactable->m_pentity;

	// declare)
	object_ptr<Isession> psession_old;

	if(_ptransactable->m_result == eRESULT::SUCCESS)
	{
		// 1) generate new cid
		sCHECKIN_ID	cid_new = _ptransactable->m_cid;

		// check) CID가 제대로 되었는 지 확인한다.
		CGASSERT_ERROR(cid_new.valid() == true);

		// check) 이런 일은 있을 수 없다.
		RETURN_IF(cid_new.empty(), false);

		scoped_lock(m_lockable_map_entity)
		{
			try
			{
				// 2) get login option
				//auto option_login = eTRANSACTION_OPTION::REJECT_IF_ALEADY_EXIST;
				//auto option_login = eTRANSACTION_OPTION::TRANSFER_CHECKIN;
				auto option_login = eTRANSACTION_OPTION::EXCLUSIVE_CHECKIN;

 				// 3) process by login option
				switch (option_login)
				{
				// case A) 배타적 checkin
				//         - 하다의 eid에 하나의 checkin만을 허락한다.
				//         - 따라서 eid에 checkin한 다른 모든 session은 강제 checkout처리가 ㄷ ㅚㄴ다.
				case	eTRANSACTION_OPTION::EXCLUSIVE_CHECKIN:
						pentity->process_insert_cid_and_remove_existing_cid(_ptransactable);
						break;

				// case B) 신규 checkin 금지
				//       - entity에 기존의 checkin이 존재하면 checkin이 금지된다.
				case	eTRANSACTION_OPTION::REJECT_IF_ALEADY_EXIST:
						pentity->process_insert_cid_posterior(_ptransactable);
						break;

				// case C) 증벅 checkin 허용
				//         - 하나의 eid에 여러개의 checkin을 허용한다.
				//         - 따라서 eid에 checkin한 것 중에 동일한 cid가 있을 경우 기존 cid는 제거한다.
				case	eTRANSACTION_OPTION::ALLOW_DUPLICATED_CHECKIN:
						pentity->process_insert_cid_duplicate(_ptransactable);
						break;

				// case D) checkin 이전 
				//         - 기존 checkin을 대채한다. - 기존 접속을 checkout시킨다.
				//         - 반드시 해당 cid와 동일한 cid가 존재햐야지만 진행된다.
				//         - 해당 cid가 존재하지 않으면 eRESULT::NOT_EXIST 결과를 낸다.
				//         - 기존 cid에 해당하는 psession을 리턴해 주며 새로운 psession으로 대체된다.
				case	eTRANSACTION_OPTION::TRANSFER_CHECKIN:
						pentity->process_transfer_cid_to_other_session(_ptransactable);
						break;

				// default) 정의되지 않은 처리다. 이런 처리는 있을 수 없다.
				default:
						throw throwable::failure(eRESULT::EXCEPTION);
						break;
				}
			}
			catch (throwable::failure & _e)
			{
				// - set result
				_ptransactable->m_result = _e.reason();

				// - rollbakc command
				process_fail_transaction(_ptransactable);
			}
			catch (...)
			{
				// - set result
				_ptransactable->m_result = eRESULT::EXCEPTION;

				// - rollbakc command
				process_fail_transaction(_ptransactable);
			}
		}
	}

	// 7) ...
	if (_ptransactable->m_result == eRESULT::SUCCESS)
	{
		// statistics) 
		++Irepository::m_statistics.m_statistics_checkin_success;

		// !주의) success일 경우 lock_count를 줄이지 않는다.
	}
	else
	{
		// statistics) 
		++Irepository::m_statistics.m_statistics_checkin_fail;
	}

	// return) 
	return true;
}

result_code doss::Irepository::process_request_transaction_command(Itransactable* _ptransactable)
{
	// 1) cid가 empty일 경우 진행하지 않는다.
	if (_ptransactable->m_cid.empty())
	{
		_ptransactable->set_result(eRESULT::NOT_READY);

		// return)
		return eRESULT::NOT_READY;
	}

	// 2) call 'on_request_command' and return
	return on_request_command(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_command(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_ERROR(_ptransactable != nullptr);

	// statistics) 
	if (_ptransactable->m_result == eRESULT::SUCCESS)
	{
		++Irepository::m_statistics.m_statistics_command_success;
	}
	else
	{
		++Irepository::m_statistics.m_statistics_command_fail;
	}

	// return)
	return true;
}

result_code doss::Irepository::process_request_transaction_update(Itransactable* _ptransactable)
{
	return on_request_update(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_update(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_ERROR(_ptransactable != nullptr);

	// statistics) 
	if(_ptransactable->m_result == eRESULT::SUCCESS)
	{
		++Irepository::m_statistics.m_statistics_command_success;
	}
	else
	{
		++Irepository::m_statistics.m_statistics_command_fail;
	}

	// return)
	return true;
}

result_code doss::Irepository::process_request_transaction_checkout(Itransactable* _ptransactable)
{
	// check) 
	THROW_IF(_ptransactable->m_cid.empty(), throwable::failure(eRESULT::NOT_READY));

	// 1) copy pentity value temporary
	auto* pentity = _ptransactable->m_pentity.get();

	// check) 
	CGASSERT_ERROR(pentity != nullptr);

	// check) return if pentity is nullptr
	RETURN_IF(pentity == nullptr, false);

	scoped_lock(m_lockable_map_entity)
	{
		// 2) remove cid 
		auto is_success = pentity->process_remove_cid(_ptransactable->m_psession);

		// check) 
		THROW_IF(is_success == false, throwable::failure(eRESULT::NOT_EXIST));
	}

	// return) 
	return on_request_checkout(_ptransactable);
}

bool doss::Irepository::process_complete_transaction_checkout(Itransactable* _ptransactable) noexcept
{
	// check)
	CGASSERT_ERROR(_ptransactable != nullptr);

	// check) return if _ptransactable is nullptr
	RETURN_IF(_ptransactable == nullptr, false);

	// statistics) 
	if (_ptransactable->m_result == eRESULT::SUCCESS)
		++Irepository::m_statistics.m_statistics_checkout_success;
	else
		++Irepository::m_statistics.m_statistics_checkout_fail;

	// return) 
	return true;
}

result_code doss::Irepository::process_request_transaction_create(Itransactable* _ptransactable)
{
	// 1) set result - must 
	_ptransactable->set_result(eRESULT::SUCCESS);

	// return)
	return eRESULT::SUCCESS;
}

bool doss::Irepository::process_complete_transaction_create(Itransactable* /*_ptransactable*/) noexcept
{
	// return) 
	return true;
}

result_code doss::Irepository::process_request_transaction_remove(Itransactable* _ptransactable)
{
	// 1) set result - must 
	_ptransactable->set_result(eRESULT::SUCCESS);

	// return)
	return eRESULT::SUCCESS;
}

bool doss::Irepository::process_complete_transaction_remove(Itransactable* /*_ptransactable*/) noexcept
{
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
	cid.id_object	 = ++static_id_checkin_generate; if(cid.id_object.empty()) { cid.id_object = ++static_id_checkin_generate; }
	cid.key_certify	 = (static_cast<uint64_t>(rand()) << 32) | rand();

	if (cid.id_object.empty())
	{
		cid.id_object = ++static_id_checkin_generate;
	}

	// return) 
	return cid;
}


}