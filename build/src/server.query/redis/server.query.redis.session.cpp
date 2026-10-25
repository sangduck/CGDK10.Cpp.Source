#include "pch.h"

using namespace CGDK;


query::redis::session::session() : 
	m_query_reference(0)
{
	this->minimum_message_buffer_size (        64*1024);
	this->maximum_message_buffer_size ( 64 * 1024*1024);
}

query::redis::session::~session() noexcept
{
}

void query::redis::session::add_query_reference()
{
	// declare)
	object_ptr<querier> pquerier;

	// - the reference count and 'm_pquerier' are changed together in 'm_cs_queue_query' (process_detach_querier())
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// check) not the first one
		RETURN_IF((this->m_query_reference++) != 0);

		pquerier = this->m_pquerier;
	}

	// statistics) the session is in use from the first query attached (dedicated or shared) to the last one released
	if (pquerier.exist())
		pquerier->statistics_on_alloc_session();
}
void query::redis::session::release_query_reference() noexcept
{
	// declare)
	object_ptr<querier> pquerier;

	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// check) not the last one
		RETURN_IF((--this->m_query_reference) != 0);

		pquerier = this->m_pquerier;
	}

	// check) unregistered (the free is counted by the querier when it is detached)
	RETURN_IF(pquerier.empty());

	// statistics)
	pquerier->statistics_on_free_session();

	// - back to the querier
	pquerier->free_session(this);
}

object_ptr<query::redis::querier> query::redis::session::process_attach_querier(querier* _pquerier) noexcept
{
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		return std::exchange(this->m_pquerier, _pquerier);
	}
}

object_ptr<query::redis::querier> query::redis::session::process_detach_querier(bool& _is_in_use) noexcept
{
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// - in use : its last release is not counted any more (no querier)
		_is_in_use = this->m_query_reference != 0;

		return std::move(this->m_pquerier);
	}
}

bool query::redis::session::execute_query(REQUEST_QUERY&&_query)
{
	// check) ... 
	CGASSERT_ERROR(_query.pquery.exist());

	// check) ...
	THROW_IF(_query.pquery.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// - queued and sent holding 'm_cs_queue_query' : the requests are sent in the order of the queue
	//   (the responses are matched to the queue in order by process_message(). on_connect() resends them holding it too)
	std::unique_lock<std::recursive_mutex> lock_queue(this->m_cs_queue_query);

	// declare) (empty if unregistered)
	object_ptr<querier> pquerier = this->m_pquerier;

	const auto count_requests = _query.count_batch ? _query.count_batch : 1;
	THROW_IF(_query.count_batch && (pquerier.empty() || this->socket_state() != eSOCKET_STATE::ESTABLISHED), throwable::failure(eRESULT::CANCEL));
	if (pquerier.exist())
		for (std::size_t i = 0; i < count_requests; ++i) pquerier->statistics_on_query_try();

	// declare)
	bool result = true;
	bool is_queued = false;
	auto buf_request = _query.buf_request;

	try
	{
		// 1) Query를 먼저 Queuing한다
		// - already holding m_cs_queue_query; do not acquire it again through push_query().
		this->m_queue_query.push_back(std::move(_query));
		is_queued = true;

		// 2) send를 전송한다.
		result = this->send(buf_request);

		// check)
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL_TO_OPERATION));
	}
	catch (...)
	{
		// declare)
		REQUEST_QUERY request_rollback;

		// - rollback : not sent. the request is taken out of the queue (the last one : queued holding the lock)
		if (is_queued && this->m_queue_query.empty() == false && this->m_queue_query.back().buf_request.data() == buf_request.data())
		{
			request_rollback = std::move(this->m_queue_query.back());
			this->m_queue_query.pop_back();
		}

		// - unlock (the request is released out of the lock)
		lock_queue.unlock();

		// statistics)
		if (pquerier.exist())
			for (std::size_t i = 0; i < count_requests; ++i) pquerier->statistics_on_query_fail();

		// reraise)
		throw;
	}

	// return)
	return true;
}

void query::redis::session::cancel_query_all() noexcept
{
	// declare)
	object_ptr<querier>	pquerier;

	// locked)
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		pquerier = this->m_pquerier;
	}

	// 1) cancel (statistics: the querier it is registered to)
	this->process_cancel_query_all(pquerier.get());
}

void query::redis::session::process_cancel_query_all(querier* _pquerier) noexcept
{
	// declare)
	circular_list<REQUEST_QUERY> list_query;

	// locked)
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// 1) move
		list_query = std::move(this->m_queue_query);
	}

	// 2) Complete outside the lock. Batch callbacks retain received replies; singles keep their legacy cancellation policy.
	for(auto& iter: list_query)
	{
		if (iter.count_batch && _pquerier != nullptr)
			for (auto i = iter.received_batch; i < iter.count_batch; ++i) _pquerier->statistics_on_query_fail();
		try
		{
			// - set result as ...
			if (!iter.count_batch) iter.pquery->m_result.type = eRESP_DATA_TYPE::NONE_;

			// - process response
			iter.pquery->process_response();
			if (iter.count_batch && iter.response) iter.response();
		}
		catch (...)
		{
		}

		// statistics) 
		if (_pquerier != nullptr && !iter.count_batch)
			_pquerier->statistics_on_query_fail();
	}
}

void query::redis::session::push_query(REQUEST_QUERY _request_infoy)
{
	// locked) 
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// 1) Queuing한다.
		this->m_queue_query.push_back(std::move(_request_infoy));
	}
}

query::redis::session::REQUEST_QUERY query::redis::session::pop_query() noexcept
{
	// locked) 
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// check) 
		RETURN_IF(this->m_queue_query.empty(), REQUEST_QUERY());

		// 1) 제일 앞에 것을 얻는다.
		auto query_info = std::move(this->m_queue_query.front());

		// 2) 하나를 빼낸다.
		this->m_queue_query.pop_front();

		// return) 
		return query_info;
	}
}

void query::redis::session::on_request_connect([[maybe_unused]] const net::sockaddr& _paddrPeer) 
{
}

void query::redis::session::on_connect()
{
	// 1) socket tunning parameter
	this->minimum_message_buffer_size(64 * 1024);
	this->maximum_message_buffer_size(64 * 1024 * 1024);

	// locked) 
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// 1) 접속 시 Queuing된 데이터들을 모두 전송한다.
		for (auto& iter : this->m_queue_query)
		{
			// 2) send를 전송한다.
			this->send(iter.pquery->m_buffer_query);
		}
	}
}

void query::redis::session::on_fail_connect([[maybe_unused]] uint64_t _reason)
{
}

void query::redis::session::on_disconnect([[maybe_unused]] uint64_t _reason)
{
	// Never replay a batch after a broken connection: some commands may have run
	// even if their replies were lost. Keep the legacy retry policy for singles.
	circular_list<REQUEST_QUERY> canceled;
	object_ptr<querier> pquerier;
	{
		std::unique_lock<std::recursive_mutex> lock(this->m_cs_queue_query);
		pquerier = this->m_pquerier;
		// Detached sessions are canceled by process_close_session(), which owns their statistics.
		if (pquerier.empty()) return;
		for (auto it = this->m_queue_query.begin(); it != this->m_queue_query.end();)
		{
			if (it->count_batch)
			{
				canceled.push_back(std::move(*it));
				it = this->m_queue_query.erase(it);
			}
			else ++it;
		}
	}
	for (auto& request : canceled)
	{
		if (pquerier.exist())
			for (auto i = request.received_batch; i < request.count_batch; ++i)
				pquerier->statistics_on_query_fail();
		try
		{
			request.pquery->process_response();
			if (request.response) request.response();
		}
		catch (...) {}
	}
}

result_code query::redis::session::process_message(sMESSAGE& _msg)
{
	// 1) process message
	if(_msg.message == eMESSAGE::SYSTEM::NETWORK_RESP)
	{
		// declare) 
		REQUEST_QUERY query_info;
		bool partial_batch = false;
		const bool response_failed = static_cast<sMESSAGE_RESP&>(_msg).data.type == eRESP_DATA_TYPE::ERROR_;
		object_ptr<querier>	pquerier;

		// 1) poping queued query (an unregistered session also completes the request. statistics: only when registered)
		SCOPED_LOCK(this->m_cs_queue_query)
		{
			// - pop and capture the querier under one lock (callbacks run after unlocking).
			if (this->m_queue_query.empty() == false)
			{
				auto& front = this->m_queue_query.front();
				if (front.count_batch)
				{
					front.pquery->m_result.vector_child[front.received_batch++] = std::move(static_cast<sMESSAGE_RESP&>(_msg).data);
					partial_batch = front.received_batch != front.count_batch;
				}
				if (!partial_batch)
				{
					query_info = std::move(front);
					this->m_queue_query.pop_front();
				}
			}
			pquerier = this->m_pquerier;
		}

		if (partial_batch)
		{
			if (pquerier.exist())
			{
				if (response_failed) pquerier->statistics_on_query_fail();
				else pquerier->statistics_on_query_success();
			}
			return eRESULT::BYPASS;
		}

		// check) return if pquery is empty
		RETURN_IF(query_info.pquery.empty(), eRESULT::BYPASS);

		// declare) 
		auto& msg_redis = (sMESSAGE_RESP&)_msg;

		// - result를 설정한다.
		if (!query_info.count_batch)
			query_info.pquery->m_result = std::move(msg_redis.data);
		else
		{
			// Publish command statistics before the completion callback can stop
			// the querier or wake a caller inspecting the counters.
			if (pquerier.exist())
			{
				if (response_failed) pquerier->statistics_on_query_fail();
				else pquerier->statistics_on_query_success();
			}
			try
			{
				query_info.pquery->process_response();
				if (query_info.response) query_info.response();
			}
			catch (...) {}
			return eRESULT::BYPASS;
		}

		// - 완료 시...
		try
		{
			// - call response function
			if (query_info.response != nullptr)
			{
				query_info.response();
			}

			// - response한다.
			query_info.pquery->process_response();

			// statistics) 
			if (!response_failed)
			{
				if (pquerier.exist())
					pquerier->statistics_on_query_success();
			}
			else
			{
				if (pquerier.exist())
					pquerier->statistics_on_query_fail();
			}
		}
		catch (...)
		{
			if (pquerier.exist())
				pquerier->statistics_on_query_fail();
		}
	}
	
	// return) 
	return eRESULT::BYPASS;
}
