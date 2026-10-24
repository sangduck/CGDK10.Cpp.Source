#pragma once

namespace CGDK
{

template <class TTRANACTION, class TSESSION>
inline result_code doss::entity_transactable<TTRANACTION, TSESSION>::request_transaction(object_ptr<transaction_t>&& _ptransaction)
{
	// check)
	CGASSERT_ERROR(_ptransaction.exist());

	// statistics)
	this->m_statistics.transaction_request.fetch_add(1, std::memory_order_relaxed);

	// 1) pre transaction request
	try
	{
		// - set request time
		_ptransaction->time_request = chrono::time::utc::now();

		// -
		auto result = this->process_on_add_transaction(_ptransaction.get());

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result));
	}
	catch (...)
	{
		// -
		_ptransaction->time_begin = chrono::time::utc::now();
		_ptransaction->time_complete = _ptransaction->time_begin;

		// reraise)
		throw;
	}

	SCOPED_LOCK(this->m_cs_queue_transaction)
	{
		// 2) queuing only if a transaction is being processed
		if (this->m_flag_transaction_prosses == true)
		{
			// check) queue 상한을 넘으면 거부한다. (client의 과도한 요청 또는 pipeline 지연)
			if (this->m_queue_transaction.size() >= this->m_max_transaction_queue)
				throw throwable::failure(eRESULT::FULL, 0, "transaction queue is full"sv);

			// - queuing transaction (executed by complete_transaction() of the running one)
			this->m_queue_transaction.push_back(std::move(_ptransaction));

			// return)
			return eRESULT::SUCCESS;
		}

		// 3) set processing flag and set current transaction
		this->m_flag_transaction_prosses = true;

		// 4) set current transaction
		this->m_ptransaction_now = _ptransaction;
	}

	// 5) execute the transaction (coroutine: started on the system_executor / legacy: runs here)
	this->process_execute_transaction(std::move(_ptransaction));

	// return)
	return eRESULT::SUCCESS;
}

template <class TTRANACTION, class TSESSION>
inline void doss::entity_transactable<TTRANACTION, TSESSION>::process_execute_transaction(object_ptr<transaction_t>&& _ptransaction)
{
	// statistics) set transaction begin time
	_ptransaction->time_begin = chrono::time::utc::now();

	try
	{
#if defined(__cpp_impl_coroutine)
		if constexpr (coroutine::is_task_v<std::remove_cvref_t<decltype(std::declval<transaction_t&>().process_transaction())>>)
		{
			// 1) make the transaction coroutine (the only coroutine frame of this transaction)
			auto task_transaction = _ptransaction->process_transaction();

			// 2) start it on the system_executor with a completion (no wrapper coroutine frame)
			//    - the entity and the transaction are kept alive by the completion until the coroutine finishes.
			//    - the completion runs in final_suspend of the transaction coroutine.
			task_transaction.start([this, pself = object_ptr<Ireferenceable>(this), ptransaction = std::move(_ptransaction)](std::exception_ptr _exception)
				{
					// statistics) an exception escaping from the transaction coroutine is counted as a failure
					if (_exception == nullptr)
						this->m_statistics.transaction_success.fetch_add(1, std::memory_order_relaxed);
					else
						this->m_statistics.transaction_fail.fetch_add(1, std::memory_order_relaxed);

					// - complete the transaction (the next queued transaction is executed)
					this->complete_transaction();
				});
		}
		else
#endif
		{
			// legacy) a synchronous transaction returning result_code
			auto result = this->process_request_transaction(_ptransaction.get());

			// check) if transaction is pended, it completes itself later (complete_transaction())
			if (result == eRESULT::PENDED)
				return;

			// statistics)
			if (result <= eRESULT::DONE)
				this->m_statistics.transaction_success.fetch_add(1, std::memory_order_relaxed);
			else
				this->m_statistics.transaction_fail.fetch_add(1, std::memory_order_relaxed);

			// - complete transaction
			this->complete_transaction();
		}
	}
	catch (...)
	{
		// - failed to even start the transaction. (ex. coroutine frame allocation failure)
		//   do not retry: clean up the pipeline, close the client connections and rethrow.
		//   (otherwise 'm_flag_transaction_prosses' stays true and the entity stalls forever)
		this->process_abort_transactions();

		// reraise)
		throw;
	}
}

template <class TTRANACTION, class TSESSION>
inline void doss::entity_transactable<TTRANACTION, TSESSION>::process_abort_transactions() noexcept
{
	// declare)
	object_ptr<TTRANACTION> ptransaction_now;
	std::vector<object_ptr<TTRANACTION>> vector_queued;

	// declare) abort된 transaction을 요청한 session (hook이 transaction을 건드리기 전에 먼저 잡아둔다.)
	object_ptr<session_t> psession_requestor;

	// 1) take the current/queued transactions out and reset the pipeline
	SCOPED_LOCK(this->m_cs_queue_transaction)
	{
		// - take the current transaction
		ptransaction_now = std::move(this->m_ptransaction_now);

		// - 요청 session을 잡아둔다.
		if (ptransaction_now.exist())
			psession_requestor = ptransaction_now->psession;

		// - take the queued transactions
		while (this->m_queue_transaction.empty() == false)
		{
			vector_queued.push_back(std::move(this->m_queue_transaction.front()));
			this->m_queue_transaction.pop_front();
		}

		// - reset the processing flag
		this->m_flag_transaction_prosses = false;
	}

	// 2) finish the current transaction (remove hook)
	if (ptransaction_now.exist())
	{
		try
		{
			ptransaction_now->time_complete = chrono::time::utc::now();
			this->process_on_remove_transaction(ptransaction_now);
		}
		catch (...) {}
	}

	// 3) finish the queued transactions (remove hook. they were 'added' by process_on_add_transaction)
	for (auto& iter_transaction : vector_queued)
	{
		try
		{
			iter_transaction->time_begin = chrono::time::utc::now();
			iter_transaction->time_complete = iter_transaction->time_begin;
			this->process_on_remove_transaction(iter_transaction);
		}
		catch (...) {}
	}

	// statistics) aborted transactions are counted as failures
	this->m_statistics.transaction_fail.fetch_add((ptransaction_now.exist() ? 1 : 0) + vector_queued.size(), std::memory_order_relaxed);

	// 4) close the client connection (fail fast. the client reconnects and recovers)
	//    - 단, abort된 transaction을 요청한 session이 '현재 account의 session'과 동일할 때만 끊는다.
	//      (이미 재접속으로 session이 교체된 경우 새 접속은 끊지 않는다.
	//       server에서 발생한 transaction(psession이 없는 경우)도 접속을 끊지 않는다.)
	if (psession_requestor.exist())
	{
		// declare) 현재 session
		object_ptr<session_t> psession_now;

		// - get current session
		SCOPED_LOCK(this->m_cs_vector_client_info)
		{
			if (this->m_vector_client_session.empty() == false)
				psession_now = this->m_vector_client_session.front();
		}

		// check) 요청 session == 현재 session일 때만 종료한다.
		//        (closesocket이 아니라 disconnect로 종료한다 - 직전에 보낸 에러 메시지가 전달된 후 끊기도록)
		if (psession_now.exist() && psession_now == psession_requestor)
		{
			try { psession_now->disconnect(); } catch (...) {}
		}
	}
}

template <class TTRANACTION, class TSESSION>
inline void doss::entity_transactable<TTRANACTION, TSESSION>::complete_transaction() noexcept
{
	// 1) finish the current transaction
	{
		// declare)
		object_ptr<TTRANACTION> ptransaction_now;

		SCOPED_LOCK(this->m_cs_queue_transaction)
		{
			// - move current transaction to local variable
			ptransaction_now = std::move(this->m_ptransaction_now);

			// check) if current transaction is nullptr, break
			CGASSERT_ERROR(ptransaction_now);

			// - set complete time
			if (ptransaction_now.exist())
				ptransaction_now->time_complete = chrono::time::utc::now();
		}

		// check) double-complete: 실행 중인 transaction이 없었다면 queue를 진행하지 않는다.
		//        (진행하면 실행 중인 다른 transaction과 겹쳐 직렬화가 깨진다. 실제 완료 시점에 queue가 처리된다.)
		if (ptransaction_now.empty())
			return;

		// - process on remove transaction
		this->process_on_remove_transaction(ptransaction_now);
	}

	// declare)
	object_ptr<TTRANACTION> ptransaction_next;

	// 2) check if there is a transaction in the queue, if not, set processing flag to false and return
	SCOPED_LOCK(this->m_cs_queue_transaction)
	{
		// check) if there is no transaction in the queue, set processing flag to false and return
		if (this->m_queue_transaction.empty())
		{
			// - set processing flag to false
			this->m_flag_transaction_prosses = false;

			// return)
			return;
		}

		// - set current transaction to the first transaction in the queue and pop it from the queue
		ptransaction_next = std::move(this->m_queue_transaction.front());

		// - set current transaction
		this->m_ptransaction_now = ptransaction_next;

		// - pop the first transaction from the queue
		this->m_queue_transaction.pop_front();
	}

	// 3) execute the next transaction (coroutine: posted to the system_executor, no recursion)
	try
	{
		this->process_execute_transaction(std::move(ptransaction_next));
	}
	catch (...)
	{
		// - failed to start the next transaction: process_execute_transaction already aborted
		//   the pipeline and closed the client connections. (noexcept here - not propagated)
	}
}

template <class TTRANACTION, class TSESSION>
inline void doss::entity_transactable<TTRANACTION, TSESSION>::register_client_session(object_ptr<session_t>&& _psession)
{

	object_ptr<session_t> psession_old;

	CGASSERT_ERROR(_psession.exist());

	THROW_IF(_psession.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "'_client_session.psocket' is nullptr"sv));

	SCOPED_LOCK(this->m_cs_client_register)
	{
		SCOPED_LOCK(this->m_cs_vector_client_info)
		{

			auto iter_already_exist = std::find(this->m_vector_client_session.begin(), this->m_vector_client_session.end(), _psession);

			if (iter_already_exist != this->m_vector_client_session.end())
				return;

			if (this->m_vector_client_session.empty() == false)
			{
				psession_old = std::move(this->m_vector_client_session.front());
				this->m_vector_client_session.erase(this->m_vector_client_session.begin());
			}

			this->m_vector_client_session.push_back(_psession);
		}

		if (psession_old.exist())
		{
			this->process_on_session_unresigter(std::move(psession_old));
		}

		this->process_on_session_register(_psession);
	}
}

template <class TTRANACTION, class TSESSION>
inline void doss::entity_transactable<TTRANACTION, TSESSION>::unregister_client_session(session_id_t _session_id) noexcept
{
	// declare)
	object_ptr<session_t> psession;

	SCOPED_LOCK(this->m_cs_vector_client_info)
	{
		// 1) find session by session_id
		auto iter_find = std::find_if(this->m_vector_client_session.begin(), m_vector_client_session.end(), [=](const auto& _iter) { return _iter->session_id == _session_id; });

		// check) if not found, return
		if(iter_find == this->m_vector_client_session.end())
			return;

		// 2) move session and erase from vector
		psession = std::move(*iter_find);

		// 3) erase session from vector
		this->m_vector_client_session.erase(iter_find);
	}

	// 4) process on session unregister
	this->process_on_session_unresigter(psession);
}

template <class TTRANACTION, class TSESSION>
inline result_code doss::entity_transactable<TTRANACTION, TSESSION>::process_request_transaction(transaction_t* _transaction)
{
	// 1) process transaction (legacy synchronous execution)
	if constexpr (std::is_same_v<std::remove_cvref_t<decltype(std::declval<transaction_t&>().process_transaction())>, result_code>)
		return _transaction->process_transaction();
	else
		return eRESULT::FAIL;
}

}
