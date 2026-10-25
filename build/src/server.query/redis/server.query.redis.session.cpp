#include "pch.h"
#include <exception>
#include <optional>

using namespace CGDK;

shared_buffer generate_redis_command(std::string_view);
shared_buffer generate_redis_batch(std::span<const std::string_view>);
shared_buffer generate_redis_batch_with_boundaries(std::span<const std::string_view>, std::vector<std::size_t>&);

bool do_process_resp_parsing(resp_data& _resp_data, shared_buffer& _buffer);


query::redis::session::session() : 
	m_query_reference(0)
{
	this->send_cork_enabled(false);
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

	// - the reference count and 'm_pquerier' are changed together in 'm_cs_queue_encoding' (process_detach_querier())
	SCOPED_LOCK(this->m_cs_queue_encoding)
	{
		// check) not the first one
		if ((this->m_query_reference.fetch_add(1, std::memory_order_acq_rel) & reference_mask) != 0)
			return;

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

	SCOPED_LOCK(this->m_cs_queue_encoding)
	{
		// check) not the last one
		if ((this->m_query_reference.fetch_sub(1, std::memory_order_acq_rel) & reference_mask) != 1)
			return;

		pquerier = this->m_pquerier;
	}

	// check) unregistered (the free is counted by the querier when it is detached)
	if (pquerier.empty())
		return;

	// statistics)
	pquerier->statistics_on_free_session();

	// - back to the querier
	pquerier->free_session(this);
}

object_ptr<query::redis::querier> query::redis::session::process_attach_querier(querier* _pquerier) noexcept
{
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		std::lock_guard<std::mutex> staging_lock(this->m_cs_queue_encoding);
		return std::exchange(this->m_pquerier, _pquerier);
	}
}

object_ptr<query::redis::querier> query::redis::session::process_detach_querier(bool& _is_in_use) noexcept
{
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// - in use : its last release is not counted any more (no querier)
		std::lock_guard<std::mutex> staging_lock(this->m_cs_queue_encoding);
        _is_in_use = this->query_reference_count() != 0;
		return std::move(this->m_pquerier);
	}
}

void query::redis::session::send_query_locked(REQUEST_QUERY& request)
{
	auto buffer = request.buf_request;
	bool queued = false;
	try
	{
		this->m_queue_send.push_back(std::move(request));

		queued = true;

		if (this->m_queue_send.back().count_batch)
			this->m_pending_batch_queries.fetch_add(1, std::memory_order_relaxed);

		if(!this->send(buffer))
			throw throwable::failure(eRESULT::FAIL_TO_OPERATION);
	}
	catch (...)
	{
		if (queued && !this->m_queue_send.empty() && this->m_queue_send.back().buf_request.data() == buffer.data())
		{
			request = std::move(this->m_queue_send.back());
			this->m_queue_send.pop_back();
			if (request.count_batch) this->m_pending_batch_queries.fetch_sub(1, std::memory_order_relaxed);
		}
		throw;
	}
}

std::exception_ptr query::redis::session::drain_send_locked(std::optional<circular_list<REQUEST_QUERY>>& canceled) noexcept
{
	try
	{
		while (!this->m_queue_deferred.empty())
		{
			auto& request = this->m_queue_deferred.front();
			const auto bytes = request.buf_request.size();
			this->send_query_locked(request);
			this->m_queue_deferred.pop_front();
			this->m_pending_send_bytes -= bytes;
			this->m_pending_send_count.fetch_sub(1, std::memory_order_relaxed);
		}
		return {};
	}
	catch (...)
	{
		canceled.emplace(std::move(this->m_queue_deferred));
		this->m_pending_send_bytes = 0;
		this->m_pending_send_count.store(0, std::memory_order_relaxed);
		return std::current_exception();
	}
}

void query::redis::session::finish_canceled(REQUEST_QUERY& request) noexcept
{
	// check) request must not be empty
	if (request.pquery.empty())
		return;

	if (request.count_batch)
	{
		request.pquery->m_result.type = eRESP_DATA_TYPE::ARRAY_;

		try
		{
			request.pquery->m_result.vector_child.resize(request.count_batch); 
		}
		catch (...)
		{
		}
	}
	else
	{
		request.pquery->m_result.type = eRESP_DATA_TYPE::NONE_;
	}

	try
	{
		request.pquery->process_response(); 
	}
	catch (...)
	{
	}

	try
	{
		if (request.response) request.response(); 
	}
	catch (...)
	{
	}
}
void query::redis::session::cancel_unsent(circular_list<REQUEST_QUERY>& canceled, querier* owner) noexcept
{
	std::size_t count = 0;
	for (auto& request : canceled) if (request.pquery.exist()) count += request.count_batch ? request.count_batch - request.received_batch : 1;
	if (count && owner) owner->statistics_on_query_fail(count);
	for (auto& request : canceled) finish_canceled(request);
}
void query::redis::session::cancel_staged(std::span<ENCODING_REQUEST> canceled, querier* owner) noexcept
{
    std::size_t count = 0;
    for (auto& entry : canceled) if (entry.pquery.exist()) count += entry.count_batch ? entry.count_batch : 1;
    if (count && owner) owner->statistics_on_query_fail(count);
    for (auto& entry : canceled) finish_canceled(entry);
}

bool query::redis::session::execute_query(REQUEST_QUERY&& request, bool allow_send_queue, bool count_statistics)
{
	// check) request must not be empty
	if (request.pquery.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	if (this->m_encode_on_session && count_statistics)
		return this->submit_encoding(std::move(request));

	object_ptr<querier> owner;
	std::unique_lock<std::recursive_mutex> lock(this->m_cs_queue_query);
	owner = this->m_pquerier;
	const auto count = request.count_batch ? request.count_batch : 1;
	THROW_IF(request.count_batch && (owner.empty() || this->socket_state() != eSOCKET_STATE::ESTABLISHED), throwable::failure(eRESULT::CANCEL));
	const bool grouping = this->m_enable_send_defer && send_defer::has_grouping_context(static_cast<const net::io::Isocket*>(this));
	const bool eligible = this->m_enable_auto_send && this->m_request_send_window != 0 && allow_send_queue && !request.count_batch &&
		!grouping && this->m_pending_batch_queries.load(std::memory_order_relaxed) == 0 && owner.exist() && this->socket_state() == eSOCKET_STATE::ESTABLISHED && request.buf_request.size() < 64 * 1024;
	// Keep normal sends free of the deferred-send completion machinery.
	if (this->m_queue_deferred.empty() && (!eligible || this->m_queue_send.size() < this->m_request_send_window))
	{
		if (count_statistics && owner.exist())
			owner->statistics_on_query_try(count);

		try
		{
			this->send_query_locked(request);
		}
		catch (...)
		{
			lock.unlock();
			if (count_statistics && owner.exist() && request.pquery.exist()) owner->statistics_on_query_fail(count);
			throw;
		}
		lock.unlock();

		if (!this->m_enable_send_defer)
			this->process_flush_deferred_send();

		return true;
	}
	std::optional<circular_list<REQUEST_QUERY>> canceled;
	std::optional<send_defer::auto_scope> group;
	std::exception_ptr error;
	bool did_send = false;
	if (eligible && this->m_queue_send.size() >= this->m_request_send_window)
	{
		const auto bytes = request.buf_request.size();
		this->m_queue_deferred.push_back(std::move(request));
		this->m_pending_send_bytes += bytes;
		this->m_pending_send_count.fetch_add(1, std::memory_order_relaxed);
		if (count_statistics && owner.exist()) owner->statistics_on_query_try();
		// Bound the unsent queue without waiting indefinitely for a reply.
		if (this->m_pending_send_count.load(std::memory_order_relaxed) >= 256 || this->m_pending_send_bytes >= 64 * 1024)
		{
			group.emplace();
			did_send = true;
			// Accepted requests complete asynchronously even if their eventual send fails.
			this->drain_send_locked(canceled);
		}
	}
	else
	{
		did_send = true;
		if (!this->m_queue_deferred.empty())
		{
			// check) grouping context
			if (!grouping) 
				group.emplace();

			error = this->drain_send_locked(canceled);
		}
		if (!error)
		{
			// check) the querier must be attached and the socket must be established
			if (count_statistics && owner.exist())
				owner->statistics_on_query_try(count);

			try
			{
				this->send_query_locked(request); 
			}
			catch (...)
			{
				error = std::current_exception();
				if (count_statistics && owner.exist() && request.pquery.exist())
					owner->statistics_on_query_fail(count);
			}
		}
	}
	lock.unlock();
	group.reset();

	if (!this->m_enable_send_defer && did_send)
		this->process_flush_deferred_send();

	if (canceled)
		this->cancel_unsent(*canceled, owner.get());

	if (error)
		std::rethrow_exception(error);

	return true;
}

std::size_t query::redis::session::process_packet(shared_buffer& buffer, const net::sockaddr& address)
{
	const auto received_before = this->statistics_get_received_message();
	const auto count = this->net::io::packetable::Nresp::process_packet(buffer, address);
	if (this->m_pending_send_count.load(std::memory_order_relaxed) && this->statistics_get_received_message() != received_before)
	{
		std::optional<circular_list<REQUEST_QUERY>> canceled;
		object_ptr<querier> owner;
		{
			std::optional<send_defer::auto_scope> group;
			std::unique_lock<std::recursive_mutex> lock(this->m_cs_queue_query);
			owner = this->m_pquerier;

			if (!send_defer::has_grouping_context(static_cast<const net::io::Isocket*>(this)))
				group.emplace();

			this->drain_send_locked(canceled);
		}
		if (!this->m_enable_send_defer)
			this->process_flush_deferred_send();

		if (canceled)
			this->cancel_unsent(*canceled, owner.get());
	}
    return count;
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
    this->drain_response_callbacks(true);
	// declare)
	circular_list<REQUEST_QUERY> list_query;
	circular_list<REQUEST_QUERY> unsent;
	circular_list<REQUEST_QUERY> staged;

	// locked)
	SCOPED_LOCK(this->m_cs_queue_query)
	{
		// 1) move
		list_query.swap(this->m_queue_send);
		unsent = std::move(this->m_queue_deferred);
		{
            std::lock_guard<std::mutex> staging_lock(this->m_cs_queue_encoding);
            staged = std::move(this->m_queue_query);
            
            this->m_encoding_scheduled = false; this->m_encoding_defer_owner = nullptr; ++this->m_encoding_epoch;
        }
		this->m_pending_send_count.store(0, std::memory_order_relaxed);
		this->m_pending_send_bytes = 0;
		this->m_pending_batch_queries.store(0, std::memory_order_relaxed);
	}

	this->cancel_unsent(unsent, _pquerier);
	this->cancel_staged(staged, _pquerier);

	// 2) Complete outside the lock. Batch callbacks retain received replies; singles keep their legacy cancellation policy.
	for(auto& iter: list_query)
	{
        if (iter.pquery.empty())
			continue;

		if (iter.count_batch && _pquerier != nullptr)
			_pquerier->statistics_on_query_fail(iter.count_batch - iter.received_batch);

		try
		{
			// - set result as ...
			if (!iter.count_batch) iter.pquery->m_result.type = eRESP_DATA_TYPE::NONE_;

			// - process response
			iter.pquery->process_response();

			if ((iter.count_batch || this->m_encode_on_session) && iter.response) 
				iter.response();
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
		const bool is_batch = _request_infoy.count_batch != 0;
		this->m_queue_send.push_back(std::move(_request_infoy));

		if (is_batch)
			this->m_pending_batch_queries.fetch_add(1, std::memory_order_relaxed);
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
		for (auto& iter : this->m_queue_send)
		{
            if (iter.pquery.empty())
				continue;

			// 2) send를 전송한다.
			this->send(iter.buf_request);
		}
	}

	if (!this->m_enable_send_defer)
		this->process_flush_deferred_send();
}

void query::redis::session::on_fail_connect([[maybe_unused]] uint64_t _reason)
{
}

void query::redis::session::on_disconnect([[maybe_unused]] uint64_t _reason)
{
	// Never replay a batch after a broken connection: some commands may have run
	// even if their replies were lost. Keep the legacy retry policy for singles.
	circular_list<REQUEST_QUERY> canceled;
	circular_list<REQUEST_QUERY> unsent;
	circular_list<REQUEST_QUERY> staged;
	object_ptr<querier> pquerier;
	{
		std::unique_lock<std::recursive_mutex> lock(this->m_cs_queue_query);

		pquerier = this->m_pquerier;

		// Detached sessions are canceled by process_close_session(), which owns their statistics.
		if (pquerier.empty())
			return;

		unsent = std::move(this->m_queue_deferred);
		{
            std::lock_guard<std::mutex> staging_lock(this->m_cs_queue_encoding);
            staged = std::move(this->m_queue_query);
            
            this->m_encoding_scheduled = false; this->m_encoding_defer_owner = nullptr; ++this->m_encoding_epoch;
        }
		this->m_pending_send_count.store(0, std::memory_order_relaxed);
		this->m_pending_send_bytes = 0;
		for (auto it = this->m_queue_send.begin(); it != this->m_queue_send.end();)
		{
			if (it->count_batch)
			{
				canceled.push_back(std::move(*it));
				it = this->m_queue_send.erase(it);
				this->m_pending_batch_queries.fetch_sub(1, std::memory_order_relaxed);
			}
			else ++it;
		}
	}
	this->cancel_unsent(unsent, pquerier.get());
	this->cancel_staged(staged, pquerier.get());
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
			if (!this->m_queue_send.empty())
			{
				auto& front = this->m_queue_send.front();

				if (front.count_batch)
				{
					front.pquery->m_result.vector_child[front.received_batch++] = std::move(static_cast<sMESSAGE_RESP&>(_msg).data);
					partial_batch = front.received_batch != front.count_batch;
				}
				if (!partial_batch)
				{
					query_info = std::move(front);
					this->m_queue_send.pop_front();
					if (query_info.count_batch) this->m_pending_batch_queries.fetch_sub(1, std::memory_order_relaxed);
				}
			}
			pquerier = this->m_pquerier;
		}

		if (partial_batch)
		{
			if (pquerier.exist())
			{
				if (response_failed)
					pquerier->statistics_on_query_fail();
				else
					pquerier->statistics_on_query_success();
			}

			// return) 
			return eRESULT::BYPASS;
		}

		// check) return if pquery is empty
		if (query_info.pquery.empty())
			return eRESULT::BYPASS;

		// declare) 
		auto& msg_redis = (sMESSAGE_RESP&)_msg;
		querier::callback_scope response_scope(pquerier.get(), this, !query_info.count_batch && query_info.callback_affinity);

		// - result를 설정한다.
		if (!query_info.count_batch)
			query_info.pquery->m_result = std::move(msg_redis.data);
		else
		{
			// Publish command statistics before the completion callback can stop
			// the querier or wake a caller inspecting the counters.
			if (pquerier.exist())
			{
				if (response_failed)
					pquerier->statistics_on_query_fail();

				else
					pquerier->statistics_on_query_success();
			}

			try
			{
				this->complete_response(std::move(query_info), pquerier.get(), false);
			}
			catch (...)
			{
			}

			// return)
			return eRESULT::BYPASS;
		}

		// - 완료 시...
        // Account for the response before a posted completion can observe it.
        if (pquerier.exist()) {
            if (response_failed) pquerier->statistics_on_query_fail();
            else pquerier->statistics_on_query_success();
        }
        const auto may_reuse = query_info.callback_affinity;
        this->complete_response(std::move(query_info), pquerier.get(), may_reuse);

	}
	
	// return) 
	return eRESULT::BYPASS;
}

std::size_t query::redis::session::process_resp_message(sMESSAGE_RESP& msg, shared_buffer& remaining)
{
	// Derived sessions retain their per-message virtual hooks and ordering.
	if (!this->m_enable_batch_receive || this->m_pending_batch_queries.load(std::memory_order_relaxed) == 0)
		return net::io::packetable::Nresp::process_resp_message(msg, remaining);

	return this->process_batch_responses(msg, remaining);
}

std::size_t query::redis::session::process_batch_responses(sMESSAGE_RESP& msg, shared_buffer& remaining)
{
	REQUEST_QUERY completed;
	object_ptr<querier> owner;
	std::exception_ptr parse_error;
	std::size_t delivered = 0;
	{
		std::unique_lock<std::recursive_mutex> lock(this->m_cs_queue_query);

		if (this->m_queue_send.empty() || !this->m_queue_send.front().count_batch)
		{
			lock.unlock();
			return net::io::packetable::Nresp::process_resp_message(msg, remaining);
		}
		auto& front = this->m_queue_send.front();
		owner = this->m_pquerier;
		std::size_t success = 0, failed = 0;
		resp_data next;
		resp_data* value = &msg.data;
		for (;;)
		{
			if (value->type == eRESP_DATA_TYPE::ERROR_) ++failed;
			else ++success;
			front.pquery->m_result.vector_child[front.received_batch++] = std::move(*value);
			++delivered;

			if (front.received_batch == front.count_batch || delivered == 256 || remaining.empty())
				break;

			const auto type = static_cast<eRESP_DATA_TYPE>(*remaining.data<char>());

			if (type == eRESP_DATA_TYPE::PING_)
				break;

			next.reset();
			next.type = type;
			auto following = remaining;
			try
			{
				// Commit only complete replies. The normal parser retains partial state.
				if (!do_process_resp_parsing(next, following)) break;
			}
			catch (...)
			{
				parse_error = std::current_exception();
				break;
			}
			remaining = std::move(following);
			value = &next;
		}

		// Publish before cancellation can account for the still-pending replies.
		if (owner.exist())
		{
			if (success)
				owner->statistics_on_query_success(success);

			if (failed)
				owner->statistics_on_query_fail(failed);
		}
		if (front.received_batch == front.count_batch)
		{
			completed = std::move(front);
			this->m_queue_send.pop_front();
			this->m_pending_batch_queries.fetch_sub(1, std::memory_order_relaxed);
		}
	}
	if (completed.pquery.exist())
	{
		querier::callback_scope response_scope(owner.get(), this, false);
		try
		{
			this->complete_response(std::move(completed), owner.get(), false);
		}
		catch (...) 
		{
		}
	}

	if (parse_error)
		std::rethrow_exception(parse_error);

	return delivered;
}
namespace {
void validate_redis_input(std::string_view command)
{
    if(command.find_first_not_of(" \t") == std::string_view::npos)
        throw throwable::failure(eRESULT::INVALID_ARGUMENT);
}
}
bool query::redis::session::post_command(REQUEST_QUERY&& request, std::string command, bool allow_send_queue)
{
	// check) request must not be empty
    if (request.pquery.empty())
        throw throwable::failure(eRESULT::INVALID_ARGUMENT);

    validate_redis_input(command);
    if (!this->m_encode_on_session) {
        request.buf_request = generate_redis_command(command);
        return this->execute_query(std::move(request), allow_send_queue);
    }
    request.input = std::move(command);
    return this->submit_encoding(std::move(request));
}
bool query::redis::session::post_batch(REQUEST_QUERY&& request, std::vector<std::string> commands)
{
	// check) request must not be empty 
    if (request.pquery.empty() || commands.empty() || request.count_batch != commands.size())
        throw throwable::failure(eRESULT::INVALID_ARGUMENT);

    for (const auto& command : commands) validate_redis_input(command);
    if (!this->m_encode_on_session) {
        std::vector<std::string_view> views(commands.begin(), commands.end());
        request.buf_request = generate_redis_batch(views);
        request.pquery->m_result.type = eRESP_DATA_TYPE::ARRAY_;
        request.pquery->m_result.vector_child.resize(request.count_batch);
        return this->execute_query(std::move(request));
    }
    request.input = std::move(commands);
    return this->submit_encoding(std::move(request));
}

class query::redis::session::encoding_executable : public Iexecutable
{
public:
    object_ptr<session> target;
    object_ptr<querier> owner;
    std::size_t epoch;
    encoding_executable(session* value, object_ptr<querier> q, std::size_t e) : target(value), owner(std::move(q)), epoch(e) {}
    ~encoding_executable() noexcept override { if (target.exist()) target->cancel_encoding_epoch(epoch, owner.get()); }
    intptr_t process_execute(intptr_t, std::size_t) override {
        auto hold = std::move(target); auto q = std::move(owner);
        hold->drain_encoding(epoch, std::move(q)); return 0;
    }
};

bool query::redis::session::submit_encoding(ENCODING_REQUEST&& entry)
{
    object_ptr<querier> owner;
    bool defer = false, run = false;
    std::size_t epoch = 0;
    {
        std::lock_guard<std::mutex> lock(this->m_cs_queue_encoding);
        owner = this->m_pquerier;

		// check) the querier must be attached and the socket must be established
        if (owner.empty() || this->socket_state() != eSOCKET_STATE::ESTABLISHED || this->io_context().empty())
            throw throwable::failure(eRESULT::CANCEL);

        const auto count = entry.count_batch ? entry.count_batch : 1;
        this->m_queue_query.push_back(std::move(entry));
        owner->statistics_on_query_try(count);
        if (this->m_encoding_defer_owner == nullptr && this->m_enable_send_defer && send_defer::has_grouping_context(static_cast<const net::io::Isocket*>(this))) {
            this->m_encoding_defer_owner = send_defer::token(); defer = true;
        }
        if (!this->m_encoding_scheduled && this->m_encoding_defer_owner == nullptr) {
            this->m_encoding_scheduled = true; epoch = ++this->m_encoding_epoch; run = true;
        }
    }
    if (defer) {
        if (!send_defer::is_deferring(static_cast<const net::io::Isocket*>(this)) ||
            !send_defer::push(static_cast<Isend_deferrable*>(this), object_ptr<Ireferenceable>(this)))
            this->process_flush_deferred_send();
    }
    if (run) this->schedule_encoding(epoch, std::move(owner));
    return true;
}
void query::redis::session::schedule_encoding(std::size_t epoch, object_ptr<querier> owner) noexcept
{
    try {
        auto task = make_object<encoding_executable>(this, owner, epoch);
        auto io = this->io_context();
        if (io.empty() || !io->post(object_ptr<Iexecutable>(task))) this->cancel_encoding_epoch(epoch, owner.get());
    } catch (...) { this->cancel_encoding_epoch(epoch, owner.get()); }
}
void query::redis::session::cancel_encoding_epoch(std::size_t epoch, querier* owner) noexcept
{
    circular_list<REQUEST_QUERY> canceled;
    {
        std::lock_guard<std::mutex> lock(this->m_cs_queue_encoding);
        if (!this->m_encoding_scheduled || this->m_encoding_epoch != epoch) return;
        canceled = std::move(this->m_queue_query); 
        this->m_encoding_scheduled = false; this->m_encoding_defer_owner = nullptr; ++this->m_encoding_epoch;
    }
    this->cancel_staged(canceled, owner);
}
void query::redis::session::process_flush_deferred_send() noexcept
{
    object_ptr<querier> owner; std::size_t epoch = 0;
    {
        std::lock_guard<std::mutex> lock(this->m_cs_queue_encoding);
        if (this->m_encoding_defer_owner == send_defer::token()) {
            this->m_encoding_defer_owner = nullptr;
            if (!this->m_encoding_scheduled && !this->m_queue_query.empty()) {
                this->m_encoding_scheduled = true; epoch = ++this->m_encoding_epoch; owner = this->m_pquerier;
            }
        }
    }
    if (epoch) {
        if (get_current_executor().get() == this->io_context().get()) this->drain_encoding(epoch, std::move(owner));
        else this->schedule_encoding(epoch, std::move(owner));
    }
    this->net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send();
}
void query::redis::session::cancel_staged(circular_list<REQUEST_QUERY>& canceled, querier* owner) noexcept
{
    std::size_t count = 0;
    for (auto& entry : canceled) if (entry.pquery.exist()) count += entry.count_batch ? entry.count_batch : 1;
    if (count && owner) owner->statistics_on_query_fail(count);
    for (auto& entry : canceled) finish_canceled(entry);
}
void query::redis::session::drain_encoding(std::size_t epoch, object_ptr<querier> owner) noexcept
{
    std::vector<REQUEST_QUERY> group;
    try {
        group.reserve(256);
        {
            std::lock_guard<std::mutex> queue_lock(this->m_cs_queue_encoding);
            if (!this->m_encoding_scheduled || this->m_encoding_epoch != epoch) return;
            while (!this->m_queue_query.empty() && group.size() < 256) {
                group.push_back(std::move(this->m_queue_query.front())); this->m_queue_query.pop_front();
            }
        }
        if (!group.empty()) {
            std::vector<std::string_view> commands;
            std::size_t count = 0;
            bool raw = true;
            for (const auto& entry : group) {
                count += entry.count_batch ? entry.count_batch : 1;
                raw = raw && !std::holds_alternative<std::monostate>(entry.input);
            }
            shared_buffer encoded;
            if (raw) {
                commands.reserve(count);
                for (auto& entry : group) {
                    if (auto command = std::get_if<std::string>(&entry.input)) commands.emplace_back(*command);
                    else for (const auto& command : std::get<std::vector<std::string>>(entry.input)) commands.emplace_back(command);
                }
                std::vector<std::size_t> offsets;
                encoded = generate_redis_batch_with_boundaries(commands, offsets);
                std::size_t index = 0;
                for (auto& entry : group) {
                    const auto next = index + (entry.count_batch ? entry.count_batch : 1);
                    entry.buf_request = encoded ^ buffer_view(encoded.data() + offsets[index], offsets[next] - offsets[index]);
                    index = next;
                }
            } else {
                std::size_t bytes = 0;
                for (auto& entry : group) {
                    if (auto command = std::get_if<std::string>(&entry.input)) entry.buf_request = generate_redis_command(*command);
                    else if (auto batch = std::get_if<std::vector<std::string>>(&entry.input)) {
                        std::vector<std::string_view> views(batch->begin(), batch->end()); entry.buf_request = generate_redis_batch(views);
                    }
                    bytes += entry.buf_request.size();
                }
                encoded = alloc_shared_buffer(bytes);
                for (const auto& entry : group) encoded.append(entry.buf_request.size(), entry.buf_request.data());
            }
            for (auto& entry : group) {
                entry.input = std::monostate{};
                if (entry.count_batch) {
                    entry.pquery->m_result.type = eRESP_DATA_TYPE::ARRAY_;
                    entry.pquery->m_result.vector_child.resize(entry.count_batch);
                }
            }
            std::unique_lock<std::recursive_mutex> send_lock(this->m_cs_queue_query);

            THROW_IF(this->m_encoding_epoch != epoch || this->m_pquerier.get() != owner.get(), throwable::failure(eRESULT::CANCEL));

            std::size_t published = 0;
            try {
                for (auto& entry : group) {
                    this->m_queue_send.push_back(std::move(entry)); ++published;
                    if (this->m_queue_send.back().count_batch) this->m_pending_batch_queries.fetch_add(1, std::memory_order_relaxed);
                }
                THROW_IF(!this->send(encoded), throwable::failure(eRESULT::FAIL_TO_OPERATION));
            } catch (...) {
                auto restore = group.begin() + published;
                while (published--) {
                    --restore; *restore = std::move(this->m_queue_send.back()); this->m_queue_send.pop_back();
                    if (restore->count_batch) this->m_pending_batch_queries.fetch_sub(1, std::memory_order_relaxed);
                }
                throw;
            }
            send_lock.unlock();
            if (!this->m_enable_send_defer) this->net::io::sendable::Ntcp_async_gather_buffered::process_flush_deferred_send();
        }
    } catch (...) { this->cancel_staged(std::span<REQUEST_QUERY>(group), owner.get()); }
    // Only queued work controls the next transmission; outstanding replies do not gate it.
    bool again = false;
    {
        std::lock_guard<std::mutex> queue_lock(this->m_cs_queue_encoding);
        if (this->m_encoding_epoch == epoch && this->m_encoding_scheduled) {
            again = !this->m_queue_query.empty() && this->m_encoding_defer_owner == nullptr;
            if (!again) this->m_encoding_scheduled = false;
        }
    }
    if (again) this->schedule_encoding(epoch, std::move(owner));
}

class query::redis::session::response_executable : public Iexecutable
{
public:
    object_ptr<session> target;
    explicit response_executable(session* value) : target(value) {}
    ~response_executable() noexcept override { if (target.exist()) target->drain_response_callbacks(true); }
    intptr_t process_execute(intptr_t, std::size_t) override {
        auto hold = std::move(target); hold->drain_response_callbacks(); return 0;
    }
};
void query::redis::session::invoke_response(COMPLETED_RESPONSE& response) noexcept
{
    if (response.request.pquery.empty()) return;
    querier::callback_scope scope(response.owner.get(), this, response.may_reuse);
    try { response.request.pquery->process_response(); } catch (...) {}
    try { if (response.request.response) response.request.response(); } catch (...) {}
}
void query::redis::session::complete_response(REQUEST_QUERY&& request, querier* owner, bool may_reuse) noexcept
{
    // Inline dispatch needs no additional owner reference or queue record.
    if (!this->m_post_response || !request.response || this->m_response_executor.empty()) {
        querier::callback_scope scope(owner, this, may_reuse);
        try { request.pquery->process_response(); } catch (...) {}
        try { if (request.response) request.response(); } catch (...) {}
        return;
    }
    COMPLETED_RESPONSE completed{std::move(request), object_ptr<querier>(owner), may_reuse};
    bool schedule = false;
    try {
        std::unique_lock<std::mutex> lock(this->m_cs_response_queue);

        if (this->m_queue_response.size() >= 65536)
		{
            lock.unlock();
			this->drain_response_callbacks(true);
			lock.lock();
        }
        this->m_queue_response.push_back(std::move(completed));

        if (!this->m_response_scheduled)
		{
			this->m_response_scheduled = true; 
			schedule = true;
		}
    } 
	catch (...)
	{
		this->drain_response_callbacks(true);
		this->invoke_response(completed);

		return;
	}

    if (schedule)
		this->schedule_response_callbacks();
}
void query::redis::session::schedule_response_callbacks() noexcept
{
    try 
	{
        auto task = make_object<response_executable>(this);
        if (!this->m_response_executor->post(object_ptr<Iexecutable>(task)))
			this->drain_response_callbacks(true);
    } 
	catch (...)
	{
		this->drain_response_callbacks(true);
	}
}
void query::redis::session::drain_response_callbacks(bool all) noexcept
{
    std::unique_lock<std::recursive_mutex> processing(this->m_cs_response_processing);
    std::size_t count = 0;
    for (;;) {
        COMPLETED_RESPONSE response;
        {
            std::lock_guard<std::mutex> lock(this->m_cs_response_queue);
            if (this->m_queue_response.empty()) { this->m_response_scheduled = false; return; }
            if (!all && count == 256) break;
            response = std::move(this->m_queue_response.front()); this->m_queue_response.pop_front();
        }
        this->invoke_response(response); ++count;
    }
    processing.unlock();
    this->schedule_response_callbacks();
}

void query::redis::session::close_shared_routes() noexcept
{
    auto access = this->m_shared_route_access.fetch_or(shared_routes_closed, std::memory_order_acq_rel) | shared_routes_closed;
    while (access != shared_routes_closed) {
        this->m_shared_route_access.wait(access, std::memory_order_acquire);
        access = this->m_shared_route_access.load(std::memory_order_acquire);
    }
}
