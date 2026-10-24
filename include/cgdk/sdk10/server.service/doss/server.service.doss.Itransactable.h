//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / release 2017.01.03                        *
//*                                                                           *
//*                 Distributed Object Storage System Classes                 *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
// Itransactable
//
//  - a transaction of an entity. the whole transaction runs as a single
//    coroutine (Irepository::co_request_transaction) started by the entity
//    on the system_executor. (one transaction at a time per entity)
//  - the result is set by set_result()/co_request_transaction and delivered
//    1) to the session hooks (Isession::process_entity_transaction_complete)
//    2) to the awaiting coroutine of session::co_checkin/co_checkout/co_request
//       (m_completion : coroutine::async_result<result_code>)
//  - repository::remote awaits the response of the depot with m_response.
//    (the response message of the depot resolves it)
//-----------------------------------------------------------------------------
class doss::Itransactable :
	virtual public				Ireferenceable
{
public:
			Itransactable() noexcept {}
	virtual ~Itransactable() noexcept {}

public:
			void				reset() noexcept;
			bool				cancel() noexcept;

			void				set_result(result_code _result, const shared_buffer& _buffer);
			void				set_result(result_code _result);
			// - sets the result if it was not set (the transaction is completed with the result code returned by the repository)
			void				_set_result_if_not_set(result_code _result) noexcept;

public:
			eTRANSACTION_TYPE	m_type = eTRANSACTION_TYPE::NONE;
			uint64_t			m_option = 0;
			lock_object			m_lockable_state;

			sREQUEST_ID			m_rid_source;
			sREQUEST_ID			m_rid;
			shared_buffer		m_buf_request;

			result_code			m_result = eRESULT::NOT_DEFINED;
			shared_buffer		m_buf_result;

			object_ptr<Irepository> m_prepository;
			object_ptr<Isession> m_psession;
			object_ptr<Ientity>	m_pentity;
			sCHECKIN_ID			m_cid;

	// - the completion awaited by session::co_checkin/co_checkout/co_request
	//   (created by them before the request. resolved with m_result when the transaction finishes)
			std::optional<coroutine::async_result<result_code>> m_completion;

	// - the response of the depot awaited by repository::remote::co_request_transaction
	//   (created before the request message is sent. resolved by the response message : {result, result buffer})
			std::optional<coroutine::async_result<std::pair<result_code, shared_buffer>>> m_response;

protected:
			CGDK::flexible<sizeof(circular_list<int>::iterator)> m_iter_pending;
			friend class Ientity;
};

namespace doss
{
	inline object_ptr< doss::Itransactable> make_command(eTRANSACTION_TYPE _type)
	{
		// 1) alloc transaction
		auto ptransactable = make_object<doss::Itransactable>();

		// 2) set command
		ptransactable->m_type = _type;

		// return)
		return ptransactable;
	}
}


}
