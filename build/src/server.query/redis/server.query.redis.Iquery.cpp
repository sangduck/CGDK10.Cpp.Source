#include "pch.h"
#include <charconv>
#include <cstring>
#include <span>
#include <optional>

using namespace CGDK;


query::redis::Iquery::~Iquery()			
{
}

namespace {
constexpr uint64_t query_phase_mask = 7;
constexpr uint64_t query_idle = 0, query_preparing = 1, query_pending = 2;
constexpr uint64_t query_finishing = 3, query_done = 4, query_resetting = 5;
bool query_busy(uint64_t value) noexcept { const auto phase = value & query_phase_mask; return phase != query_idle && phase != query_done; }
}
void query::redis::Iquery::_wait_request(uint64_t generation, bool notify_executor)
{
    auto current = this->m_request_completion.load(std::memory_order_acquire);
    if ((current & ~query_phase_mask) != generation || !query_busy(current)) return;
    std::optional<scoped_thread_block> thread_block;
    if (notify_executor) thread_block.emplace();
    while ((current & ~query_phase_mask) == generation && query_busy(current)) {
        this->m_request_completion.wait(current, std::memory_order_acquire);
        current = this->m_request_completion.load(std::memory_order_acquire);
    }
}
void query::redis::Iquery::wait()
{
    const auto current = this->m_request_completion.load(std::memory_order_acquire);
    this->_wait_request(current & ~query_phase_mask);
}
uint64_t query::redis::Iquery::_reserve_idle(bool may_wait)
{
    auto current = this->m_request_completion.load(std::memory_order_acquire);
    for (;;) {
        if (query_busy(current)) { THROW_IF(!may_wait, throwable::failure(eRESULT::NOT_READY)); this->_wait_request(current & ~query_phase_mask); current = this->m_request_completion.load(std::memory_order_acquire); continue; }
        if (this->m_request_completion.compare_exchange_weak(current, (current & ~query_phase_mask) | query_resetting, std::memory_order_acq_rel)) return current;
    }
}
void query::redis::Iquery::_release_idle(uint64_t previous) noexcept
{
    this->m_request_completion.store(previous, std::memory_order_release);
    this->m_request_completion.notify_all();
}
uint64_t query::redis::Iquery::_begin_request(bool require_ready)
{
    auto current = this->m_request_completion.load(std::memory_order_acquire);
    for (;;) {
        THROW_IF(query_busy(current), throwable::failure(eRESULT::NOT_READY));
        const auto generation = (current & ~query_phase_mask) + 8;
        if (!this->m_request_completion.compare_exchange_weak(current, generation | query_preparing, std::memory_order_acq_rel)) continue;
        const auto state = this->m_state_query.load(std::memory_order_relaxed);
        if (this->m_psession.empty() || (state != eOBJECT_STATE::READY && (require_ready || state != eOBJECT_STATE::PROCESSED))) {
            this->_release_idle(current);
            throw throwable::failure(eRESULT::NOT_READY);
        }
        this->m_result.reset();
        return generation;
    }
}
void query::redis::Iquery::_publish_request(uint64_t generation) noexcept
{
    this->m_state_query.store(eOBJECT_STATE::REQUESTING, std::memory_order_relaxed);
    this->m_request_completion.store(generation | query_pending, std::memory_order_release);
}
void query::redis::Iquery::_abort_request(uint64_t generation) noexcept
{
    auto expected = generation | query_pending;
    if (!this->m_request_completion.compare_exchange_strong(expected, generation | query_preparing, std::memory_order_acq_rel)) return;
    this->m_function_on_complete = nullptr;
    this->m_state_query.store(eOBJECT_STATE::READY, std::memory_order_relaxed);
    this->_release_idle(generation | query_idle);
}
void query::redis::Iquery::process_set_session(session* session)
{
    const auto previous = this->_reserve_idle();
    if (this->m_psession == session) { this->_release_idle(previous); return; }
    auto old = std::move(this->m_psession);
    this->m_psession = session;
    if (session) session->add_query_reference();
    this->m_state_query.store(eOBJECT_STATE::READY, std::memory_order_relaxed);
    this->_release_idle(previous & ~query_phase_mask);
    if (old.exist()) old->release_query_reference();
}
void query::redis::Iquery::close_query() noexcept
{
    const auto previous = this->_reserve_idle();
    if (this->m_state_query.load(std::memory_order_relaxed) == eOBJECT_STATE::PROCESSED) {
        this->m_result.reset();
        this->m_state_query.store(eOBJECT_STATE::READY, std::memory_order_relaxed);
    }
    this->_release_idle(previous & ~query_phase_mask);
}
void query::redis::Iquery::process_response()
{
    auto current = this->m_request_completion.load(std::memory_order_acquire);
    if ((current & query_phase_mask) != query_pending) return;
    const auto generation = current & ~query_phase_mask;
    if (!this->m_request_completion.compare_exchange_strong(current, generation | query_finishing, std::memory_order_acq_rel)) return;
    auto complete = std::exchange(this->m_function_on_complete, nullptr);
    this->m_state_query.store(eOBJECT_STATE::PROCESSED, std::memory_order_release);
    this->_release_idle(generation | query_done);
    if (complete) complete();
}
void query::redis::Iquery::process_execute_query()
{
    this->process_response();
}
void query::redis::Iquery::process_reset_session() noexcept
{
    const auto previous = this->_reserve_idle();
    auto old = std::move(this->m_psession);
    this->m_result.reset(); this->m_function_on_complete = nullptr;
    this->m_state_query.store(eOBJECT_STATE::NONE, std::memory_order_relaxed);
    this->_release_idle(previous & ~query_phase_mask);
    if (old.exist()) old->release_query_reference();
    this->_process_notify_to_requestor();
}
void query::redis::Iquery::_set_function_on_complete(std::function<void()>&& complete)
{
    const auto previous = this->_reserve_idle(false);
    this->m_function_on_complete = std::move(complete);
    this->_release_idle(previous);
}
void query::redis::Iquery::_process_on_complete()
{
    this->process_response();
}

void query::redis::Iquery::_process_notify_to_requestor() noexcept
{
	// check) requestor가 없으면 끝낸다.
	if(this->m_prequestor.empty())
		return;

	// 1) ProcessCompleteQuery를 호출한다.
	try
	{
		this->m_prequestor->process_notify_complete_query(this);
	}
	catch (...)
	{
	}
		
	// 2) reset requestor
	this->m_prequestor.reset();
}

shared_buffer generate_redis_command(std::string_view command)
{
	std::string_view local[16];
	std::vector<std::string_view> overflow;
	std::size_t count = 0, bytes = 0;
	const auto blank = [](char c) { return c == ' ' || c == '\t'; };
	const auto digits = [](std::size_t n) { std::size_t d = 1; while (n >= 10) { n /= 10; ++d; } return d; };
	const auto add = [&bytes](std::size_t n) {
		THROW_IF(n > (std::numeric_limits<std::size_t>::max)() - bytes, throwable::failure(eRESULT::OVERFLOW_ERROR));
		bytes += n;
	};
	for (std::size_t pos = 0; pos < command.size();)
	{
		while (pos < command.size() && blank(command[pos])) ++pos;
		if (pos == command.size()) break;
		const auto begin = pos;
		while (pos < command.size() && !blank(command[pos])) ++pos;
		auto argument = command.substr(begin, pos - begin);
		if (count < 16) local[count] = argument;
		else overflow.push_back(argument);
		++count;
		add(argument.size());
		add(digits(argument.size()) + 5);
	}
	THROW_IF(count == 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, std::string_view("redis command has no argument")));
	add(digits(count) + 3);
	auto buffer = alloc_shared_buffer(bytes);
	char* out = buffer.data<char>();
	const auto number = [&out, end = out + bytes](std::size_t n) { out = std::to_chars(out, end, n).ptr; };
	*out++ = '*'; number(count); *out++ = '\r'; *out++ = '\n';
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto argument = i < 16 ? local[i] : overflow[i - 16];
		*out++ = '$'; number(argument.size()); *out++ = '\r'; *out++ = '\n';
		std::memcpy(out, argument.data(), argument.size()); out += argument.size();
		*out++ = '\r'; *out++ = '\n';
	}
	CGASSERT_ERROR(static_cast<std::size_t>(out - buffer.data<char>()) == bytes);
	buffer.add_size(bytes);
	return buffer;
}

void query::redis::Iquery::_process_execute_query(std::string_view command)
{
    std::string owned(command);
    const auto generation = this->_begin_request();
    this->_publish_request(generation);
    try {
        CGDK::query::redis::session::REQUEST_QUERY request;
        request.pquery = this;
        scoped_thread_block thread_block;
        this->m_psession->post_command(std::move(request), std::move(owned), false);
        this->_wait_request(generation, false);
    } catch (...) { this->_abort_request(generation); throw; }
}
void query::redis::Iquery::post_execute_query(std::string&& command, std::function<void()>&& response)
{
    this->_process_post_execute_query(command, std::move(response));
}
void query::redis::Iquery::_process_post_execute_query(std::string_view command, std::function<void()>&& response, std::function<void()>&& complete)
{
    std::string owned(command);
    const auto generation = this->_begin_request(false);
    if (complete) this->m_function_on_complete = std::move(complete);
    CGDK::query::redis::session::REQUEST_QUERY request;
    request.pquery = this; request.response = std::move(response);
    request.callback_affinity = querier::t_callback_scope != nullptr;
    this->_publish_request(generation);
    try { this->m_psession->post_command(std::move(request), std::move(owned), true); }
    catch (...) { this->_abort_request(generation); throw; }
}

static shared_buffer generate_redis_batch_impl(std::span<const std::string_view> commands, std::vector<std::size_t>* boundaries)
{
	const auto blank = [](char c) { return c == ' ' || c == '\t'; };
	const auto digits = [](std::size_t n) { std::size_t d = 1; while (n >= 10) { n /= 10; ++d; } return d; };
	const auto add = [](std::size_t& total, std::size_t n) {
		if(n > (std::numeric_limits<std::size_t>::max)() - total)
			throw throwable::failure(eRESULT::OVERFLOW_ERROR);
		total += n;
	};

	// check) 
	if(commands.empty())
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	std::size_t bytes = 0;
	std::size_t local_counts[64];
	std::vector<std::size_t> overflow_counts;
	if (commands.size() > 64) overflow_counts.resize(commands.size() - 64);
	std::size_t index = 0;
	if (boundaries) { boundaries->clear(); boundaries->reserve(commands.size() + 1); boundaries->push_back(0); }
	for (auto command : commands)
	{
		std::size_t count = 0;
		for (std::size_t pos = 0; pos < command.size();)
		{
			while (pos < command.size() && blank(command[pos])) 
				++pos;
			if (pos == command.size()) 
				break;

			const auto start = pos;
			while (pos < command.size() && !blank(command[pos])) 
				++pos;
			const auto length = pos - start;
			add(bytes, length);
			add(bytes, digits(length) + 5);
			++count;
		}

		if(count == 0)
			throw throwable::failure(eRESULT::INVALID_ARGUMENT);

		add(bytes, digits(count) + 3);
		if (index < 64) local_counts[index] = count;
		else overflow_counts[index - 64] = count;
		++index;
	}

	auto buffer = alloc_shared_buffer(bytes);
	char* out = buffer.data<char>();
	const auto number = [&out, end = buffer.data<char>() + bytes](std::size_t n) {
		out = std::to_chars(out, end, n).ptr;
	};
	index = 0;
	for (auto command : commands)
	{
		const auto count = index < 64 ? local_counts[index] : overflow_counts[index - 64];
		++index;
		*out++ = '*'; number(count); *out++ = '\r'; *out++ = '\n';
		for (std::size_t pos = 0; pos < command.size();)
		{
			while (pos < command.size() && blank(command[pos])) 
				++pos;

			if (pos == command.size()) 
				break;

			const auto start = pos;
			while (pos < command.size() && !blank(command[pos]))
				++pos;

			const auto length = pos - start;
			*out++ = '$';
			number(length);
			*out++ = '\r';
			*out++ = '\n';

			std::memcpy(out, command.data() + start, length); out += length;

			*out++ = '\r';
			*out++ = '\n';
		}
		if (boundaries) boundaries->push_back(static_cast<std::size_t>(out - buffer.data()));
	}
	CGASSERT_ERROR(static_cast<std::size_t>(out - buffer.data<char>()) == bytes);
	buffer.add_size(bytes);
	return buffer;
}

query::execute_result<void> query::redis::Iquery::_execute_query_deferred(std::string&& _command)
{
	// - 'co_await' sends the request and resumes the coroutine when the response is received
	auto pquery = object_ptr<Iquery>(this);
	std::string command_sync(_command);
	auto session = this->get_session();
	auto resume = session.exist() ? session->coroutine_resume_policy() : std::pair<object_ptr<Iexecutor>, bool>{nullptr, true};

	return execute_result<void>::deferred(
		[pquery, command = std::move(command_sync)]() { pquery->_process_execute_query(command); },
		[pquery, command = std::move(_command)](std::function<void()>&& _on_complete) { pquery->_process_post_execute_query(command, nullptr, std::move(_on_complete)); }, std::move(resume.first), resume.second);
}


void query::redis::Iquery::_process_post_fresh_query(std::string_view command, std::function<void()>&& response)
{
    this->_process_post_execute_query(command, std::move(response));
}

shared_buffer generate_redis_batch(std::span<const std::string_view> commands)
{
	return generate_redis_batch_impl(commands, nullptr);
}
shared_buffer generate_redis_batch_with_boundaries(std::span<const std::string_view> commands, std::vector<std::size_t>& boundaries)
{
	return generate_redis_batch_impl(commands, &boundaries);
}
