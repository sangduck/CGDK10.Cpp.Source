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
//-----------------------------------------------------------------------------
// doss::entity
//
//
//
//-----------------------------------------------------------------------------
namespace CGDK
{

//-----------------------------------------------------------------------------
// doss::entity_transactable<TTRANACTION, TSESSION>
//
//  1) serializes the transactions of an entity.
//     - request_transaction() executes the transaction at once when no
//       transaction is being processed, or queues it. (m_queue_transaction)
//     - when a transaction finishes, the next queued one is executed.
//  2) coroutine-based: when TTRANACTION::process_transaction() returns
//     coroutine::task<result_code>, the whole transaction runs as a single
//     coroutine started on the system_executor. (task::start with a completion.
//     no wrapper coroutine frame is made)
//     all its asynchronous phases (db, server confirm, ...) are awaited with
//     'co_await' inside the coroutine. when the coroutine finishes, the
//     transaction is completed automatically. (no eRESULT::PENDED any more)
//  3) legacy: when process_transaction() returns result_code, it runs
//     synchronously as before. (eRESULT::PENDED + complete_transaction())
//
//-----------------------------------------------------------------------------
template <class TTRANACTION, class TSESSION>
class doss::entity_transactable :
	virtual public				Ireferenceable
{
public:
			typedef TTRANACTION	transaction_t;
			typedef TSESSION	session_t;
			typedef typename TSESSION::session_id_t session_id_t;

public:
			result_code			request_transaction(object_ptr<transaction_t>&& _ptransaction);
			void				complete_transaction() noexcept;

			void				register_client_session(object_ptr<session_t>&& _psession);
			void				unregister_client_session(session_id_t _session_id) noexcept;

			auto				size() const noexcept { SCOPED_LOCK(this->m_cs_queue_transaction) { return this->m_queue_transaction.size(); } }
			auto				client_session_now() noexcept { SCOPED_LOCK(this->m_cs_vector_client_info) { return (this->m_vector_client_session.empty() == false) ? this->m_vector_client_session.front() : object_ptr<session_t>();} }
			auto&				client_session() noexcept { return this->m_vector_client_session; }
			const auto&			statistics() const noexcept { return this->m_statistics; }

			auto&				lockable_transaction_queue() noexcept { return this->m_cs_queue_transaction; }
			auto&				get_transaction_queue() noexcept { return this->m_queue_transaction; }
			auto				get_transaction_now() noexcept { SCOPED_LOCK(this->m_cs_queue_transaction) { return this->m_ptransaction_now; } }

			struct sSTATISTICS
			{
				std::atomic<uint64_t> transaction_request{ 0 };
				std::atomic<uint64_t> transaction_success{ 0 };
				std::atomic<uint64_t> transaction_fail{ 0 };
			};
			int					m_idx_list = -1;

protected:
	// - executes a transaction. (coroutine: started on the system_executor / legacy: runs here)
	virtual void				process_execute_transaction(object_ptr<transaction_t>&& _ptransaction);
	// - legacy synchronous execution. (used only when process_transaction() returns result_code)
	virtual result_code			process_request_transaction(transaction_t* _transaction);
	// - called when a transaction could not even be started. (ex. out of memory)
	//   no retry: cleans up the current/queued transactions and closes the client connections. (fail fast)
	virtual void				process_abort_transactions() noexcept;

	virtual void				process_on_session_register(session_t* /*_psession*/) {}
	virtual void				process_on_session_unresigter(session_t* /*_psession*/) noexcept {}

	virtual result_code			process_on_add_transaction(transaction_t* /*_ptransaction*/) { return eRESULT::SUCCESS; }
	virtual void				process_on_remove_transaction(transaction_t* /*_ptransaction*/) noexcept {}

protected:
			lock_object			m_cs_queue_transaction;
			circular_list<object_ptr<TTRANACTION>> m_queue_transaction;
			object_ptr<TTRANACTION> m_ptransaction_now;
			size_t				m_max_transaction_queue = 8;
			bool				m_flag_transaction_prosses = false;
			sSTATISTICS			m_statistics;

			lock_object			m_cs_vector_client_info;
			lock_object			m_cs_client_register;
			std::vector<object_ptr<session_t>> m_vector_client_session;
};

}

#include "cgdk/sdk10/server.service/doss/server.service.doss.entity_transactable.inl"
