#include "pch.h"
#include "cgdk/sdk10/server.query.redis.h"
#include <cstring>
#include <string>
#include <vector>
#include <future>
#include <thread>

CGDK::shared_buffer generate_redis_command(std::string_view command);
CGDK::shared_buffer generate_redis_batch(std::span<const std::string_view> commands);

TEST(CGDK10_redis_resp, encoding_preserves_argument_boundaries_and_binary_bytes)
{
	for (std::size_t count : {1u, 16u, 17u, 64u, 128u})
	{
		std::string command = " \t ";
		std::string expected = "*" + std::to_string(count) + "\r\n";
		for (std::size_t i = 0; i < count; ++i)
		{
			std::string argument = "arg" + std::to_string(i);
			if (i + 1 == count)
			{
				argument.append(70000, 'x');
				argument.push_back('\0');
				argument += "tail";
			}
			command += argument;
			command += " \t  ";
			expected += "$" + std::to_string(argument.size()) + "\r\n" + argument + "\r\n";
		}
		auto encoded = generate_redis_command(command);
		EXPECT_EQ(std::string_view(encoded.data<char>(), encoded.size()), expected);
	}
	EXPECT_THROW(generate_redis_command(" \t  "), CGDK::throwable::failure);
}

TEST(CGDK10_redis_resp, batch_encoding_preserves_command_counts_at_storage_boundaries)
{
	for (std::size_t count : {1u, 64u, 65u, 1000u})
	{
		std::vector<std::string> commands;
		commands.reserve(count);
		std::string expected;
		for (std::size_t i = 0; i < count; ++i)
		{
			if (i % 2 == 0)
			{
				commands.emplace_back(" \t ping  ");
				expected += "*1\r\n$4\r\nping\r\n";
			}
			else
			{
				auto value = std::to_string(i);
				commands.push_back("echo\t" + value);
				expected += "*2\r\n$4\r\necho\r\n$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
			}
		}
		std::vector<std::string_view> views(commands.begin(), commands.end());
		auto encoded = generate_redis_batch(views);
		EXPECT_EQ(std::string_view(encoded.data<char>(), encoded.size()), expected);
		views.back() = " \t";
		EXPECT_THROW(generate_redis_batch(views), CGDK::throwable::failure);
	}
}
// Temporary strings must reach the stopped-querier check, rather than recurse
// through the std::string&& forwarding overload forever.
namespace CGDK
{
TEST(CGDK10_redis_resp, temporary_command_on_stopped_querier)
{
	auto querier = make_object<query::redis::querier_t>();
	EXPECT_THROW(querier->post_execute_query(std::string("ping"),
		[](CGDK::query::redis::query*) {}), CGDK::throwable::failure);
	EXPECT_THROW(querier->post_execute_query<CGDK::query::redis::query>(std::string("ping"),
		[](CGDK::query::redis::query*) {}), CGDK::throwable::failure);
}

TEST(CGDK10_redis_resp, batch_rejects_empty_invalid_and_stopped)
{
	auto querier = make_object<query::redis::querier_t>();
	bool called = false;
	auto response = [&](auto&&) { called = true; };
	EXPECT_THROW(querier->post_execute_query({}, response), throwable::failure);
	const std::string_view invalid[] = {"ping", " \t "};
	EXPECT_THROW(querier->post_execute_query(invalid, response), throwable::failure);
	const std::string_view valid[] = {"ping", "echo test"};
	EXPECT_THROW(querier->post_execute_query(valid, response), throwable::failure);
	EXPECT_FALSE(called);
}

TEST(CGDK10_redis_resp, sync_batch_rejects_empty_invalid_and_stopped)
{
	auto querier = make_object<query::redis::querier_t>();
	EXPECT_THROW((void)querier->execute_query({}), throwable::failure);
	const std::string_view invalid[] = {"ping", " \t "};
	EXPECT_THROW((void)querier->execute_query(invalid), throwable::failure);
	const std::vector<std::string> strings = {"ping", "echo test"};
	EXPECT_THROW((void)querier->execute_query(strings), throwable::failure);
	EXPECT_THROW((void)querier->execute_query(strings), throwable::failure);
	bool called = false;
	EXPECT_THROW(querier->post_execute_query(strings, [&](auto&&) { called = true; }), throwable::failure);
	EXPECT_FALSE(called);
}

class test_batch_query : public query::redis::Iquery
{
public:
	void finish() { process_response(); }
	void prepare(std::size_t count)
	{
		m_state_query = eOBJECT_STATE::REQUESTING;
        m_request_completion.store(10, std::memory_order_relaxed);
		m_result.type = eRESP_DATA_TYPE::ARRAY_;
		m_result.vector_child.resize(count);
	}
};

class test_batch_session : public query::redis::session
{
public:
	using session::push_query;
	void reply(eRESP_DATA_TYPE type, std::string_view value)
	{
		sMESSAGE_RESP message;
		message.data.type = type;
		message.data.str_value = value;
		process_message(message);
	}
	void simulate_disconnect() { on_disconnect(0); }
	void simulate_connect() { on_connect(); }
};

class test_batch_querier : public query::redis::querier
{
public:
	using querier::register_session;
	using querier::unregister_session;
	using querier::post_alloc_session;
    void publish_routes() { open_fixed_routes(m_sessions_registered); }
    void close_routes() { close_fixed_routes(); }
    bool try_dedicated(query::redis::Iquery* query) { return _try_alloc_session(query); }
	std::unique_lock<std::recursive_mutex> lock_manager() { return std::unique_lock<std::recursive_mutex>(m_lockable_query); }
	std::size_t queued() { std::lock_guard<std::recursive_mutex> lock(m_lockable_query); return m_queue_query.size(); }
};

class test_receive_batch_session : public test_batch_session
{
public:
	std::vector<std::size_t> groups;
	void feed(std::string_view data)
	{
		auto buffer = alloc_shared_buffer(pending.size() + data.size() + 16);
		if (pending.size()) buffer.append(pending.size(), pending.data());
		buffer.append(data.size(), data.data());
		std::memset(buffer.data() + buffer.size(), 0, 16);
		this->net::io::packetable::Nresp::process_packet(buffer, net::sockaddr());
		pending = buffer.size() ? shared_buffer(buffer) : shared_buffer();
	}
	std::size_t batches_pending() const { return m_pending_batch_queries.load(); }
	std::uint64_t received_messages() const { return statistics_get_received_message(); }
protected:
	std::size_t process_resp_message(sMESSAGE_RESP& msg, shared_buffer& remaining) override
	{
		auto count = process_batch_responses(msg, remaining);
		groups.push_back(count);
		return count;
	}
	shared_buffer pending;
};

TEST(CGDK10_redis_resp, receive_batch_groups_mixed_replies_and_preserves_following_query)
{
	constexpr std::size_t count = 2500;
	auto session = make_object<test_receive_batch_session>();
	auto owner = make_object<test_batch_querier>();
	owner->register_session(session);
	owner->statistics_on_query_try(count + 1);
	auto batch = make_object<test_batch_query>(); batch->prepare(count);
	auto single = make_object<test_batch_query>(); single->prepare(0);
	std::size_t failed = 0;
	std::string wire;
	for (std::size_t i = 0; i < count; ++i)
	{
		if (i % 17 == 0) { wire += "-ERR item\r\n"; ++failed; }
		else if (i % 17 == 1) wire += "$-1\r\n";
		else if (i % 17 == 2) wire += "*2\r\n+item\r\n:123\r\n";
		else if (i % 17 == 3) wire += "$0\r\n\r\n";
		else { auto v = std::to_string(i); wire += "$" + std::to_string(v.size()) + "\r\n" + v + "\r\n"; }
	}
	wire += "$4\r\ntail\r\n";
	int calls = 0;
	query::redis::session::REQUEST_QUERY request;
	request.pquery = batch; request.count_batch = count;
	request.response = [&] {
		++calls;
		EXPECT_EQ(owner->statistics_get_query_success(), count - failed);
		EXPECT_EQ(owner->statistics_get_query_fail(), failed);
		EXPECT_EQ(owner->statistics_get_query_processing(), 1u);
	};
	session->push_query(std::move(request));
	query::redis::session::REQUEST_QUERY tail;
	tail.pquery = single; tail.response = [&] { ++calls; };
	session->push_query(std::move(tail));
	session->feed(wire);
	ASSERT_EQ(calls, 2);
	ASSERT_EQ(batch->result().vector_child.size(), count);
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto& value = batch->result()[static_cast<int>(i)];
		if (i % 17 == 0) EXPECT_EQ(value.type, eRESP_DATA_TYPE::ERROR_);
		else if (i % 17 == 1 || i % 17 == 3) EXPECT_TRUE(value.str_value.empty());
		else if (i % 17 == 2) { ASSERT_EQ(value.vector_child.size(), 2u); EXPECT_EQ(value.vector_child[1].str_value, "123"); }
		else EXPECT_EQ(value.str_value, std::to_string(i));
	}
	EXPECT_EQ(single->result().str_value, "tail");
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	EXPECT_EQ(owner->statistics_get_query_success(), count - failed + 1);
	EXPECT_EQ(session->batches_pending(), 0u);
	EXPECT_EQ(session->received_messages(), count + 1);
	EXPECT_GT(*std::max_element(session->groups.begin(), session->groups.end()), 1u);
	owner->unregister_session(session);
}

TEST(CGDK10_redis_resp, receive_batch_partial_bulk_and_parse_failure_keep_statistics)
{
	for (bool malformed : {false, true})
	{
		auto session = make_object<test_receive_batch_session>();
		auto owner = make_object<test_batch_querier>(); owner->register_session(session);
		auto query = make_object<test_batch_query>(); query->prepare(2);
		owner->statistics_on_query_try(2);
		int calls = 0;
		query::redis::session::REQUEST_QUERY request;
		request.pquery = query; request.count_batch = 2; request.response = [&] { ++calls; };
		session->push_query(std::move(request));
		if (malformed)
		{
			EXPECT_THROW(session->feed("+first\r\n!invalid\r\n"), throwable::failure);
			EXPECT_EQ(owner->statistics_get_query_success(), 1u);
			EXPECT_EQ(owner->statistics_get_query_processing(), 1u);
			session->cancel_query_all();
			EXPECT_EQ(owner->statistics_get_query_fail(), 1u);
			EXPECT_EQ(query->result()[1].type, eRESP_DATA_TYPE::NONE_);
		}
		else
		{
			session->feed("+first\r\n$8\r\npar");
			EXPECT_EQ(calls, 0);
			EXPECT_EQ(owner->statistics_get_query_success(), 1u);
			EXPECT_EQ(owner->statistics_get_query_processing(), 1u);
			session->feed("tial!\r\n");
			EXPECT_EQ(query->result()[1].str_value, "partial!");
			EXPECT_EQ(owner->statistics_get_query_success(), 2u);
			EXPECT_EQ(session->received_messages(), 2u);
		}
		EXPECT_EQ(query->result()[0].str_value, "first");
		EXPECT_EQ(calls, 1);
		EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
		EXPECT_EQ(session->batches_pending(), 0u);
		owner->unregister_session(session);
	}
}

TEST(CGDK10_redis_resp, receive_batch_completion_is_outside_queue_lock)
{
	auto session = make_object<test_receive_batch_session>();
	auto owner = make_object<test_batch_querier>(); owner->register_session(session);
	auto query = make_object<test_batch_query>(); query->prepare(4);
	owner->statistics_on_query_try(4);
	std::promise<void> finished;
	auto ready = finished.get_future();
	std::thread worker;
	query::redis::session::REQUEST_QUERY request;
	request.pquery = query; request.count_batch = 4;
	request.response = [&] {
		EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
		worker = std::thread([&] { session->cancel_query_all(); finished.set_value(); });
		EXPECT_EQ(ready.wait_for(std::chrono::seconds(1)), std::future_status::ready);
	};
	session->push_query(std::move(request));
	session->feed("+1\r\n+2\r\n+3\r\n+4\r\n");
	if (worker.joinable()) worker.join();
	EXPECT_EQ(owner->statistics_get_query_success(), 4u);
	EXPECT_EQ(session->batches_pending(), 0u);
	owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, batch_preserves_order_errors_and_following_query)
{
	auto session = make_object<test_batch_session>();
	auto batch = make_object<test_batch_query>();
	batch->prepare(3);
	int calls = 0;
	query::redis::session::REQUEST_QUERY request;
	request.pquery = batch;
	request.count_batch = 3;
	request.response = [&] { ++calls; };
	session->push_query(std::move(request));
	session->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "first");
	session->reply(eRESP_DATA_TYPE::ERROR_, "ERR test");
	EXPECT_EQ(calls, 0);
	session->reply(eRESP_DATA_TYPE::INTEGER_, "3");
	ASSERT_EQ(calls, 1);
	ASSERT_EQ(batch->result().vector_child.size(), 3u);
	EXPECT_EQ(batch->result().vector_child[0].str_value, "first");
	EXPECT_EQ(batch->result().vector_child[1].type, eRESP_DATA_TYPE::ERROR_);
	EXPECT_EQ(batch->result().vector_child[2].str_value, "3");
	auto next = make_object<test_batch_query>();
	next->prepare(0);
	query::redis::session::REQUEST_QUERY single;
	single.pquery = next;
	single.response = [&] { ++calls; };
	session->push_query(std::move(single));
	session->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "next");
	EXPECT_EQ(calls, 2);
	EXPECT_EQ(next->result().str_value, "next");
	session->cancel_query_all();
	EXPECT_EQ(calls, 2);
}

TEST(CGDK10_redis_resp, batch_partial_cancel_and_disconnect_complete_once)
{
	for (bool disconnect : {false, true})
	{
		auto session = make_object<test_batch_session>();
		auto owner = make_object<test_batch_querier>();
		if (disconnect)
		{
			owner->register_session(session);
			for (int i = 0; i < 3; ++i) owner->statistics_on_query_try();
		}
		auto batch = make_object<test_batch_query>();
		batch->prepare(3);
		int calls = 0;
		query::redis::session::REQUEST_QUERY request;
		request.pquery = batch;
		request.count_batch = 3;
		request.response = [&] { ++calls; };
		session->push_query(std::move(request));
		session->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "first");
		if (disconnect) session->simulate_disconnect();
		else session->cancel_query_all();
		EXPECT_EQ(calls, 1);
		EXPECT_EQ(batch->result().vector_child[0].str_value, "first");
		EXPECT_EQ(batch->result().vector_child[1].type, eRESP_DATA_TYPE::NONE_);
		EXPECT_EQ(batch->result().vector_child[2].type, eRESP_DATA_TYPE::NONE_);
		session->cancel_query_all();
		EXPECT_EQ(calls, 1);
		if (disconnect)
		{
			EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
			owner->unregister_session(session);
		}
	}
}
}

//-----------------------------------------------------------------------------
// RESP parser (net::io::packetable::Nresp)
//
//  - the data is fed in pieces like the receivable does (unprocessed bytes are kept and the next piece is appended)
//  - split at every byte position : no read past the received data, the same replies in the same order
//  - parse errors and exceptions of process_message() are not swallowed (the socket is closed)
//-----------------------------------------------------------------------------
namespace CGDK
{
	// - "+OK" / ":12" / "-ERR x" / "$hello" / "$(nil)" / "*2[...,...]"
	static std::string to_text(const resp_data& _data)
	{
		switch (_data.type)
		{
		case	eRESP_DATA_TYPE::SIMPLE_STRING_:	return "+" + std::string(_data.str_value);
		case	eRESP_DATA_TYPE::ERROR_:			return "-" + std::string(_data.str_value);
		case	eRESP_DATA_TYPE::INTEGER_:			return ":" + std::string(_data.str_value);
		case	eRESP_DATA_TYPE::BULK_STRING_:		return "$" + std::string(_data.str_value);
		case	eRESP_DATA_TYPE::ARRAY_:
				{
					std::string text = "*" + std::to_string(_data.vector_child.size()) + "[";
					for (std::size_t i = 0; i < _data.vector_child.size(); ++i)
					{
						if (i != 0)
							text += ",";
						text += to_text(_data.vector_child[i]);
					}
					return text + "]";
				}
		default:									return "?";
		}
	}

	class test_resp_session : public query::redis::session
	{
	public:
				std::vector<std::string> replies;
				bool	throw_in_message = false;

				void	feed(std::string_view _data)
		{
			auto buf = alloc_shared_buffer(this->m_pending.size() + _data.size() + 16);
			if (this->m_pending.size() != 0)
				buf.append(this->m_pending.size(), this->m_pending.data());
			buf.append(_data.size(), _data.data());

			// - fill the space after the data : a read past the received data finds '\n' (not the debug heap fill)
			std::memset(buf.data() + buf.size(), '\n', 16);

			this->net::io::packetable::Nresp::process_packet(buf, net::sockaddr());

			// - unprocessed bytes
			this->m_pending = (buf.size() != 0) ? shared_buffer(buf) : shared_buffer();
		}

				void	feed_split(std::string_view _data, std::size_t _size)
		{
			for (std::size_t pos = 0; pos < _data.size(); pos += _size)
				this->feed(_data.substr(pos, _size));
		}

	protected:
		virtual	result_code process_message(sMESSAGE& _msg) override
		{
			if (_msg.message == eMESSAGE::SYSTEM::NETWORK_RESP)
			{
				if (this->throw_in_message)
					throw throwable::failure(eRESULT::FAIL);

				this->replies.push_back(to_text(static_cast<sMESSAGE_RESP&>(_msg).data));
			}
			return eRESULT::DONE;
		}

				shared_buffer m_pending;
	};

	static const std::string RESP_REPLIES =
		"+OK\r\n"
		":1234\r\n"
		":-56\r\n"
		"-ERR bad thing\r\n"
		"$5\r\nhello\r\n"
		"$0\r\n\r\n"
		"$-1\r\n"
		"*3\r\n$3\r\nfoo\r\n:42\r\n*2\r\n+a\r\n$-1\r\n"
		"*0\r\n"
		"*-1\r\n"
		"$12\r\nline\r\nbreak!\r\n"
		"+LAST\r\n";

	static const std::vector<std::string> RESP_EXPECTED =
	{
		"+OK", ":1234", ":-56", "-ERR bad thing", "$hello", "$", "$", "*3[$foo,:42,*2[+a,$]]", "*0[]", "*0[]", "$line\r\nbreak!", "+LAST"
	};

	TEST(CGDK10_redis_resp, text_scan_preserves_split_and_vector_boundaries)
	{
		for (std::size_t size : {0u, 2u, 3u, 4u, 15u, 16u, 27u, 28u, 29u, 30u, 31u, 32u, 33u, 59u, 60u, 61u, 62u, 63u, 64u, 65u, 127u, 128u, 129u, 4096u})
		{
			for (char prefix : {'+', '-'})
			{
				const std::string body(size, 'x');
				const auto wire = std::string(1, prefix) + body + "\r\n";
				for (auto split : {std::size_t(1), wire.size() - 2, wire.size() - 1, wire.size()})
				{
					auto session = make_own<test_resp_session>();
					if (split) session->feed(std::string_view(wire).substr(0, split));
					if (split < wire.size())
					{
						EXPECT_TRUE(session->replies.empty());
						session->feed(std::string_view(wire).substr(split));
					}
					ASSERT_EQ(session->replies.size(), 1u);
					EXPECT_EQ(session->replies[0], std::string(1, prefix) + body);
				}
			}
		}
	}

	TEST(CGDK10_redis_resp, text_scan_rejects_first_bad_cr_and_keeps_embedded_zero)
	{
		const std::string body(4096, 'x');
		{
			auto session = make_own<test_resp_session>();
			EXPECT_THROW(session->feed("+" + body + "\rX\r\n"), throwable::failure);
		}
		{
			auto session = make_own<test_resp_session>();
			std::string binary = body;
			binary[31] = '\0';
			session->feed("+OK\r\n+" + binary + "\r\n+LAST\r\n");
			ASSERT_EQ(session->replies.size(), 3u);
			EXPECT_EQ(session->replies[0], "+OK");
			EXPECT_EQ(session->replies[1], "+" + binary);
			EXPECT_EQ(session->replies[2], "+LAST");
		}
	}
	TEST(CGDK10_redis_resp, replies_in_one_piece)
	{
		auto psession = make_own<test_resp_session>();
		ASSERT_NO_THROW(psession->feed(RESP_REPLIES));
		EXPECT_EQ(psession->replies, RESP_EXPECTED);
	}

	// - a null bulk string ("$-1\r\n") skipped the first 2 bytes of the next reply
	TEST(CGDK10_redis_resp, null_bulk_string_then_reply)
	{
		auto psession = make_own<test_resp_session>();
		ASSERT_NO_THROW(psession->feed("$-1\r\n+OK\r\n$-1\r\n:7\r\n"));
		EXPECT_EQ(psession->replies, (std::vector<std::string>{ "$", "+OK", "$", ":7" }));
	}

	// - split into two at every position
	TEST(CGDK10_redis_resp, split_at_every_position)
	{
		for (std::size_t pos = 1; pos < RESP_REPLIES.size(); ++pos)
		{
			auto psession = make_own<test_resp_session>();
			ASSERT_NO_THROW(psession->feed(std::string_view(RESP_REPLIES).substr(0, pos))) << "split at " << pos;
			ASSERT_NO_THROW(psession->feed(std::string_view(RESP_REPLIES).substr(pos))) << "split at " << pos;
			EXPECT_EQ(psession->replies, RESP_EXPECTED) << "split at " << pos;
		}
	}

	// - fed byte by byte (and in pieces of 2, 3, 7 bytes)
	TEST(CGDK10_redis_resp, split_in_pieces)
	{
		for (std::size_t size : { std::size_t(1), std::size_t(2), std::size_t(3), std::size_t(7) })
		{
			auto psession = make_own<test_resp_session>();
			for (std::size_t pos = 0; pos < RESP_REPLIES.size(); pos += size)
				ASSERT_NO_THROW(psession->feed(std::string_view(RESP_REPLIES).substr(pos, size))) << "piece size " << size << " at " << pos;
			EXPECT_EQ(psession->replies, RESP_EXPECTED) << "piece size " << size;
		}
	}

	// - a parse error is thrown (the socket is closed) : it was swallowed and the partial state stayed
	TEST(CGDK10_redis_resp, parse_error_is_thrown)
	{
		for (std::string_view data : { "?what\r\n"sv, "+OK\rX"sv, "*1a\r\n"sv, "*\r\n"sv, "$3\r\nabcXY"sv, "*1\r\n?\r\n"sv, "$99999999999999999999\r\n"sv, "$5\rX"sv })
		{
			auto psession = make_own<test_resp_session>();
			EXPECT_ANY_THROW(psession->feed(data)) << data;
		}
	}

	// - an exception of process_message() is thrown (it was swallowed : the next replies were matched to the wrong queries)
	TEST(CGDK10_redis_resp, exception_in_process_message_is_thrown)
	{
		auto psession = make_own<test_resp_session>();
		psession->throw_in_message = true;
		EXPECT_ANY_THROW(psession->feed("+OK\r\n+NEXT\r\n"));
	}
}

namespace CGDK {
class test_send_window_session : public test_batch_session
{
public:
	test_send_window_session() { m_enable_auto_send = true; socket_state(eSOCKET_STATE::ESTABLISHED); }
	std::vector<std::string> sent;
	bool fail_send = false;
	int forced_flushes = 0;
	bool verify_flush_lock = false;
	void configure_send_defer(bool enabled) { m_enable_send_defer = enabled; }
	void configure_window(std::size_t n) { m_request_send_window = n; }
    std::unique_lock<std::mutex> hold_admission() { return std::unique_lock<std::mutex>(m_cs_queue_encoding); }
    bool dedicated() const { return m_is_dedicated.load(); }
	std::size_t pending() const { return m_pending_send_count.load(); }
	std::size_t lock_probe() { std::lock_guard<std::recursive_mutex> lock(m_cs_queue_query); return pending(); }
	const void* socket_identity() const { return static_cast<const net::io::Isocket*>(this); }
	void feed(std::string_view text)
	{
		auto buffer = alloc_shared_buffer(pending_bytes.size() + text.size() + 16);
		if (pending_bytes.size()) buffer.append(pending_bytes.size(), pending_bytes.data());
		buffer.append(text.size(), text.data());
		process_packet(buffer, net::sockaddr());
		pending_bytes = buffer.size() ? shared_buffer(buffer) : shared_buffer();
	}
protected:
	shared_buffer pending_bytes;
	void process_flush_deferred_send() noexcept override
	{
		++forced_flushes;
		if (verify_flush_lock)
		{
			auto lock_check = std::async(std::launch::async, [this] { return lock_probe(); });
			EXPECT_EQ(lock_check.wait_for(std::chrono::seconds(2)), std::future_status::ready);
		}
	}
	bool process_send(shared_buffer&& buffer, std::size_t, uint64_t) override
	{
		if (fail_send) return false;
		sent.emplace_back(buffer.data<char>(), buffer.size());
		return true;
	}
};
static object_ptr<test_batch_query> window_request(test_send_window_session* session, int id,
	bool individual = true, std::function<void()> callback = {}, std::size_t batch = 0)
{
	auto query = make_object<test_batch_query>();
	query->prepare(batch);
	query::redis::session::REQUEST_QUERY request;
	request.pquery = query;
	request.buf_request = generate_redis_command("ECHO " + std::to_string(id));
	request.response = std::move(callback);
	request.count_batch = batch;
	try { session->execute_query(std::move(request), individual); }
    catch (...) { query->finish(); throw; }
	return query;
}
TEST(CGDK10_redis_resp, individual_window_flushes_on_complete_reply_and_matches_fifo)
{
	auto session = make_object<test_send_window_session>();
	auto owner = make_object<test_batch_querier>(); owner->register_session(session);
	std::vector<object_ptr<test_batch_query>> queries;
	int callbacks = 0;
	for (int i = 0; i < 20; ++i) queries.push_back(window_request(session.get(), i, true, [&] { ++callbacks; }));
	EXPECT_EQ(session->sent.size(), 8u); EXPECT_EQ(session->pending(), 12u);
	session->feed("+");
	EXPECT_EQ(session->sent.size(), 8u);
	// Preserve incomplete input in the same way as the receivable.
	session->feed("0\r\n");
	EXPECT_EQ(session->sent.size(), 20u); EXPECT_EQ(session->pending(), 0u);
	for (int i = 1; i < 20; ++i) session->feed("+" + std::to_string(i) + "\r\n");
	EXPECT_EQ(callbacks, 20);
	for (int i = 0; i < 20; ++i) EXPECT_EQ(queries[i]->result().str_value, std::to_string(i));
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, individual_window_preserves_batch_sync_defer_auto_and_cork_order)
{
	for (int mode = 0; mode < 5; ++mode)
	{
		auto session = make_object<test_send_window_session>();
		auto owner = make_object<test_batch_querier>(); owner->register_session(session);
		for (int i = 0; i < 10; ++i) window_request(session.get(), i);
		EXPECT_EQ(session->pending(), 2u);
		if (mode == 0) window_request(session.get(), 10, false);
		if (mode == 1) window_request(session.get(), 10, false, {}, 1);
		if (mode == 2) { send_defer::scope group; for (int i = 10; i < 30; ++i) window_request(session.get(), i); }
		if (mode == 3) { send_defer::auto_scope group; for (int i = 10; i < 30; ++i) window_request(session.get(), i); }
		if (mode == 4)
		{
			const auto previous = send_defer::begin_cork(session->socket_identity());
			for (int i = 10; i < 30; ++i) window_request(session.get(), i);
			send_defer::end_cork(previous);
		}
		EXPECT_EQ(session->pending(), 0u);
		const auto n = mode < 2 ? 11u : 30u;
		ASSERT_EQ(session->sent.size(), n);
		for (unsigned i = 0; i < n; ++i)
		{
			auto expected = generate_redis_command("ECHO " + std::to_string(i));
			EXPECT_EQ(session->sent[i], std::string(expected.data<char>(), expected.size()));
		}
		session->cancel_query_all(); owner->unregister_session(session);
	}
}
TEST(CGDK10_redis_resp, individual_window_bounds_pending_and_completes_failed_unsent_once)
{
	for (int mode = 0; mode < 4; ++mode)
	{
		auto session = make_object<test_send_window_session>();
		auto owner = make_object<test_batch_querier>(); owner->register_session(session);
		int canceled = 0;
		for (int i = 0; i < 8; ++i) window_request(session.get(), i);
		for (int i = 8; i < 12; ++i) window_request(session.get(), i, true, [&] {
			++canceled;
			auto unlocked = std::async(std::launch::async, [&] { return session->lock_probe(); });
			EXPECT_EQ(unlocked.wait_for(std::chrono::seconds(2)), std::future_status::ready);
		});
		if (mode == 0) session->cancel_query_all();
		if (mode == 1) session->simulate_disconnect();
		if (mode == 2) { session->fail_send = true; session->feed("+OK\r\n"); }
		if (mode == 3)
		{
			session->fail_send = true;
			EXPECT_THROW(window_request(session.get(), 12, false), throwable::failure);
		}
		EXPECT_EQ(canceled, 4); EXPECT_EQ(session->pending(), 0u);
		session->cancel_query_all(); EXPECT_EQ(canceled, 4);
		EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
		owner->unregister_session(session);
	}
	auto session = make_object<test_send_window_session>();
	auto owner = make_object<test_batch_querier>(); owner->register_session(session);
	for (int i = 0; i < 264; ++i) window_request(session.get(), i);
	EXPECT_EQ(session->pending(), 0u); EXPECT_EQ(session->sent.size(), 264u);
	session->cancel_query_all(); owner->unregister_session(session);
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, send_defer_option_bypasses_external_scopes_and_keeps_internal_window)
{
	EXPECT_TRUE(query::redis::querier::START_PARAMETER().enable_send_defer);
	auto default_session = make_object<test_send_window_session>();
	EXPECT_TRUE(default_session->is_send_defer_enabled());
	for (int mode = 0; mode < 3; ++mode)
	{
		auto session = make_object<test_send_window_session>();
		session->configure_send_defer(false);
		session->verify_flush_lock = true;
		EXPECT_FALSE(session->is_send_defer_enabled());
		auto owner = make_object<test_batch_querier>(); owner->register_session(session);
		auto check = [&] {
			for (int i = 0; i < 20; ++i) window_request(session.get(), i);
			EXPECT_EQ(session->sent.size(), 8u);
			EXPECT_EQ(session->pending(), 12u);
			EXPECT_EQ(session->forced_flushes, 8);
			session->feed("+OK\r\n");
			EXPECT_EQ(session->sent.size(), 20u);
			EXPECT_EQ(session->pending(), 0u);
			EXPECT_EQ(session->forced_flushes, 9);
			window_request(session.get(), 20, false, {}, 2);
			EXPECT_EQ(session->sent.size(), 21u);
			EXPECT_EQ(session->forced_flushes, 10);
			for (int i = 0; i < 21; ++i)
			{
				auto expected = generate_redis_command("ECHO " + std::to_string(i));
				EXPECT_EQ(session->sent[i], std::string(expected.data<char>(), expected.size()));
			}
		};
		if (mode == 0) { send_defer::scope group; check(); }
		if (mode == 1) { send_defer::auto_scope group; check(); }
		if (mode == 2)
		{
			const auto previous = send_defer::begin_cork(session->socket_identity());
			check();
			send_defer::end_cork(previous);
		}
		session->cancel_query_all();
		EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
		owner->unregister_session(session);
	}
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, callback_post_reuses_session_without_querier_manager_lock)
{
	auto owner = make_object<test_batch_querier>();
	auto source = make_object<test_send_window_session>(), other = make_object<test_send_window_session>();
	owner->register_session(source); owner->register_session(other);
	owner->set_object_state(eOBJECT_STATE::RUNNING);
	auto initial = make_object<test_batch_query>(); initial->set_session(source); initial->prepare(0);
	int callbacks = 0;
	query::redis::session::REQUEST_QUERY request;
	request.pquery = initial;
	request.callback_affinity = true;
	request.response = [&] { owner->post_execute_query(std::string_view("ECHO next"), [&](auto* reply) {
		EXPECT_EQ(reply->result().str_value, "next"); ++callbacks;
	}); };
	owner->statistics_on_query_try(); source->push_query(std::move(request));
	auto held = owner->lock_manager();
	auto replied = std::async(std::launch::async, [&] { source->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "first"); });
	EXPECT_EQ(replied.wait_for(std::chrono::seconds(2)), std::future_status::ready);
	held.unlock(); replied.get();
	EXPECT_EQ(source->sent.size(), 1u); EXPECT_TRUE(other->sent.empty());
	source->feed("+next\r\n"); EXPECT_EQ(callbacks, 1);
	initial->reset_session();
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->set_object_state(eOBJECT_STATE::NONE);
	owner->unregister_session(source); owner->unregister_session(other);
}
TEST(CGDK10_redis_resp, callback_fast_path_preserves_dedicated_waiter_fifo)
{
	auto owner = make_object<test_batch_querier>(); auto session = make_object<test_send_window_session>();
	owner->register_session(session); owner->set_object_state(eOBJECT_STATE::RUNNING);
	auto initial = make_object<test_batch_query>(); initial->set_session(session); initial->prepare(0);
	auto dedicated = make_object<test_batch_query>();
	bool acquired = false; int callbacks = 0;
	query::redis::session::REQUEST_QUERY request; request.pquery = initial;
	request.response = [&] {
		owner->post_alloc_session(dedicated.get(), [&](query::redis::Iquery*) { acquired = true; });
		owner->post_execute_query(std::string_view("ECHO queued"), [&](auto*) { ++callbacks; });
	};
	owner->statistics_on_query_try(); session->push_query(std::move(request));
	session->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "first");
	EXPECT_FALSE(acquired); EXPECT_EQ(owner->queued(), 2u); EXPECT_TRUE(session->sent.empty());
	initial->reset_session();
	EXPECT_TRUE(acquired); EXPECT_EQ(owner->queued(), 1u); EXPECT_TRUE(session->sent.empty());
	dedicated->reset_session();
	EXPECT_EQ(owner->queued(), 0u); EXPECT_EQ(session->sent.size(), 1u);
	session->feed("+queued\r\n"); EXPECT_EQ(callbacks, 1);
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, shared_callback_post_does_not_adopt_dedicated_session)
{
	auto owner = make_object<test_batch_querier>();
	auto shared = make_object<test_send_window_session>(), exclusive = make_object<test_send_window_session>();
	owner->register_session(shared); owner->register_session(exclusive); owner->set_object_state(eOBJECT_STATE::RUNNING);
	auto initial = owner->alloc_query<test_batch_query>(); ASSERT_EQ(initial->get_session().get(), exclusive.get());
	initial->prepare(0);
	query::redis::session::REQUEST_QUERY request; request.pquery = initial;
	request.response = [&] { owner->post_execute_query(std::string_view("PING"), [](auto*) {}); };
	owner->statistics_on_query_try(); exclusive->push_query(std::move(request));
	exclusive->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "first");
	EXPECT_TRUE(exclusive->sent.empty()); EXPECT_EQ(shared->sent.size(), 1u);
	shared->feed("+PONG\r\n"); initial->reset_session();
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(shared); owner->unregister_session(exclusive);
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, callback_seeds_balance_chains_then_keep_each_connection)
{
	auto owner = make_object<test_batch_querier>();
	std::vector<object_ptr<test_send_window_session>> sessions;
	for (int i = 0; i < 4; ++i) { auto s = make_object<test_send_window_session>(); owner->register_session(s); sessions.push_back(s); }
	owner->set_object_state(eOBJECT_STATE::RUNNING);
	std::vector<object_ptr<test_batch_query>> seeds;
	for (int i = 0; i < 4; ++i)
	{
		auto initial = make_object<test_batch_query>(); initial->set_session(sessions[0]); initial->prepare(0); seeds.push_back(initial);
		query::redis::session::REQUEST_QUERY request; request.pquery = initial;
		request.response = [&] {
			owner->post_execute_query(std::string_view("ECHO home"), [&](auto*) {
				owner->post_execute_query(std::string_view("ECHO stable"), [](auto*) {});
			});
		};
		owner->statistics_on_query_try(); sessions[0]->push_query(std::move(request));
	}
	for (int i = 0; i < 4; ++i) sessions[0]->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "seed");
	for (auto& session : sessions) EXPECT_EQ(session->sent.size(), 1u);
	for (auto& session : sessions) session->feed("+home\r\n");
	for (auto& session : sessions) { EXPECT_EQ(session->sent.size(), 2u); session->feed("+stable\r\n"); }
	for (auto& seed : seeds) seed->reset_session();
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->set_object_state(eOBJECT_STATE::NONE);
	for (auto& session : sessions) owner->unregister_session(session);
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, posts_from_one_seed_callback_preserve_connection_and_order)
{
	auto owner = make_object<test_batch_querier>();
	auto source = make_object<test_send_window_session>(), other = make_object<test_send_window_session>();
	owner->register_session(source); owner->register_session(other); owner->set_object_state(eOBJECT_STATE::RUNNING);
	auto initial = make_object<test_batch_query>(); initial->set_session(source); initial->prepare(0);
	query::redis::session::REQUEST_QUERY request; request.pquery = initial;
	request.response = [&] {
		owner->post_execute_query(std::string_view("ECHO first"), [](auto*) {});
		owner->post_execute_query(std::string_view("ECHO second"), [](auto*) {});
	};
	owner->statistics_on_query_try(); source->push_query(std::move(request));
	source->reply(eRESP_DATA_TYPE::SIMPLE_STRING_, "seed");
	EXPECT_TRUE(other->sent.empty()); ASSERT_EQ(source->sent.size(), 2u);
	for (int i = 0; i < 2; ++i)
	{
		auto expected = generate_redis_command(i ? "ECHO second" : "ECHO first");
		EXPECT_EQ(source->sent[i], std::string(expected.data<char>(), expected.size()));
	}
	source->feed("+first\r\n+second\r\n"); initial->reset_session();
	EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
	owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(source); owner->unregister_session(other);
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, configurable_request_window_handles_six_ten_and_disabled)
{
	EXPECT_EQ(query::redis::querier::START_PARAMETER().request_send_window, 8u);
	for (std::size_t window : {0u, 6u, 8u, 10u})
	{
		auto session = make_object<test_send_window_session>(); session->configure_window(window);
		auto owner = make_object<test_batch_querier>(); owner->register_session(session);
		EXPECT_EQ(session->request_send_window(), window);
		for (int i = 0; i < 20; ++i) window_request(session.get(), i);
		EXPECT_EQ(session->sent.size(), window ? window : 20u);
		EXPECT_EQ(session->pending(), window ? 20u - window : 0u);
		session->feed("+OK\r\n"); EXPECT_EQ(session->sent.size(), 20u); EXPECT_EQ(session->pending(), 0u);
		session->cancel_query_all(); EXPECT_EQ(owner->statistics_get_query_processing(), 0u); owner->unregister_session(session);
	}
}
struct test_executor_probe
{
	Iexecutor* observed = nullptr;
	bool await_ready() const noexcept { return false; }
	template<class P> bool await_suspend(std::coroutine_handle<P> h) noexcept { observed = h.promise().executor().get(); return false; }
	Iexecutor* await_resume() const noexcept { return observed; }
};
static coroutine::task<void> test_query_executor(object_ptr<Iexecutor> preferred, bool preserve,
	object_ptr<Iexecutor> override_executor, std::promise<Iexecutor*>& result)
{
	auto operation = query::execute_result<object_ptr<query::redis::query>>::deferred([] { return object_ptr<query::redis::query>(make_object<query::redis::query>()); },
		[](object_ptr<query::redis::query>& value, std::function<void()>&& done) { value = make_object<query::redis::query>(); done(); }, preferred, preserve);
	auto moved = std::move(operation);
	auto value = override_executor.exist() ? co_await std::move(moved).resume_on(override_executor) : co_await std::move(moved);
	if (value.empty()) throw std::runtime_error("bad execute_result value");
	auto observed = co_await test_executor_probe{};
	// Exercise the void specialization and its lvalue override too.
	auto empty = query::execute_result<void>::deferred([] {}, [](std::function<void()>&& done) { done(); }, preferred, preserve);
	co_await empty.resume_on(coroutine::default_executor());
	if ((co_await test_executor_probe{}) != coroutine::default_executor().get()) throw std::runtime_error("void executor override failed");
	result.set_value(observed);
}
TEST(CGDK10_redis_resp, execute_result_executor_defaults_overrides_and_move)
{
	auto io = executor::make_io_context_threads("query executor test", 1);
	auto other = executor::make_io_context_threads("query caller test", 1);
	auto run = [&](object_ptr<Iexecutor> bound, object_ptr<Iexecutor> preferred, bool preserve,
		object_ptr<Iexecutor> override_executor, Iexecutor* expected) {
		std::promise<Iexecutor*> result; auto ready = result.get_future();
		test_query_executor(preferred, preserve, override_executor, result).start(bound, [&](std::exception_ptr error) {
			if (error) result.set_exception(error);
		});
		ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
		EXPECT_EQ(ready.get(), expected); coroutine::wait_detached();
	};
	run(nullptr, io, true, nullptr, io.get());
	run(other, io, true, nullptr, other.get());
	run(other, io, false, nullptr, io.get());
	run(nullptr, io, true, other, other.get());
	run(other, nullptr, false, nullptr, other.get());
	dynamic_pointer_cast<object::Istartable>(io)->stop(); dynamic_pointer_cast<object::Istartable>(other)->stop();
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, reconnect_replays_the_queued_request_buffers_in_order)
{
	auto session = make_object<test_send_window_session>();
	auto owner = make_object<test_batch_querier>(); owner->register_session(session);
	auto query = make_object<test_batch_query>(); query->prepare(0);
	for (auto command : {"ECHO first", "ECHO second"})
	{
		query::redis::session::REQUEST_QUERY request;
		request.pquery = query; request.buf_request = generate_redis_command(command);
		session->push_query(std::move(request)); owner->statistics_on_query_try();
	}
	session->simulate_disconnect(); session->simulate_connect();
	ASSERT_EQ(session->sent.size(), 2u);
	for (int i = 0; i < 2; ++i)
	{
		auto expected = generate_redis_command(i ? "ECHO second" : "ECHO first");
		EXPECT_EQ(session->sent[i], std::string(expected.data<char>(), expected.size()));
	}
	session->cancel_query_all(); EXPECT_EQ(owner->statistics_get_query_processing(), 0u); owner->unregister_session(session);
}
}

namespace CGDK {
class test_encoding_session : public test_send_window_session
{
public:
    std::promise<void> sent_ready;
    std::promise<void> second_send_ready;
    std::atomic<int> send_calls{0};
    void configure(object_ptr<executor::Iio_context> io) { io_context(io); m_encode_on_session = true; }
    std::size_t staged() { std::lock_guard<std::mutex> lock(m_cs_queue_encoding); return m_queue_query.size(); }
protected:
    void process_flush_deferred_send() noexcept override { session::process_flush_deferred_send(); }
    bool process_send(shared_buffer&& buffer, std::size_t n, uint64_t flag) override {
        auto result = test_send_window_session::process_send(std::move(buffer), n, flag);
        const auto count = ++send_calls;
        if (count == 1) sent_ready.set_value();
        else if (count == 2) second_send_ready.set_value();
        return result;
    }
};
TEST(CGDK10_redis_resp, session_encodes_owned_scalar_and_batch_together_before_one_send)
{
    auto io = executor::make_io_context_threads("redis encoder", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto single = make_object<test_batch_query>(); single->prepare(0);
    auto batch = make_object<test_batch_query>(); batch->prepare(0);
    auto ready = session->sent_ready.get_future(); int callbacks = 0;
    {
        send_defer::scope defer;
        query::redis::session::REQUEST_QUERY a; a.pquery = single; a.response = [&] { ++callbacks; };
        std::string command = "ECHO owned";
        session->post_command(std::move(a), command, true); command.assign(1000, 'x');
        query::redis::session::REQUEST_QUERY b; b.pquery = batch; b.count_batch = 2; b.response = [&] { ++callbacks; };
        std::vector<std::string> commands{"ECHO first", "ECHO second"};
        session->post_batch(std::move(b), commands); commands.clear();
        EXPECT_EQ(session->staged(), 2u); EXPECT_TRUE(session->sent.empty());
    }
    ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    session->feed("+owned\r\n+first\r\n+second\r\n");
    ASSERT_EQ(session->sent.size(), 1u);
    std::string expected;
    for (auto text : {"ECHO owned", "ECHO first", "ECHO second"}) { auto b = generate_redis_command(text); expected.append(b.data<char>(), b.size()); }
    EXPECT_EQ(session->sent[0], expected); EXPECT_EQ(callbacks, 2);
    EXPECT_EQ(single->result().str_value, "owned"); EXPECT_EQ(batch->result()[0].str_value, "first"); EXPECT_EQ(batch->result()[1].str_value, "second");
    EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
TEST(CGDK10_redis_resp, session_cancels_raw_queue_once_before_encoding)
{
    auto io = executor::make_io_context_threads("redis encoder cancel", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    int callbacks = 0;
    {
        send_defer::scope defer;
        auto query = make_object<test_batch_query>(); query->prepare(0);
        query::redis::session::REQUEST_QUERY request; request.pquery = query; request.count_batch = 2;
        request.response = [&] { ++callbacks; };
        session->post_batch(std::move(request), {"PING", "PING"});
        session->cancel_query_all(); session->cancel_query_all();
        EXPECT_EQ(query->result().size(), 2u); EXPECT_EQ(query->result()[0].type, eRESP_DATA_TYPE::NONE_);
        EXPECT_EQ(callbacks, 1); EXPECT_TRUE(session->sent.empty());
    }
    EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
TEST(CGDK10_redis_resp, session_encoder_send_failure_completes_all_requests_once)
{
    auto io = executor::make_io_context_threads("redis encoder failure", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io); session->fail_send = true;
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    std::promise<void> completed; auto ready = completed.get_future(); std::atomic<int> callbacks{0};
    {
        send_defer::scope defer;
        for (int i = 0; i < 3; ++i) {
            auto query = make_object<test_batch_query>(); query->prepare(0);
            query::redis::session::REQUEST_QUERY request; request.pquery = query;
            request.response = [&] { if (++callbacks == 3) completed.set_value(); };
            session->post_command(std::move(request), "PING", true);
        }
    }
    ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(callbacks, 3); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    session->cancel_query_all(); EXPECT_EQ(callbacks, 3);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, session_encoder_rejected_executor_completes_accepted_request)
{
    auto io = executor::make_io_context_threads("redis stopped encoder", 1);
    dynamic_pointer_cast<object::Istartable>(io)->stop();
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto query = make_object<test_batch_query>(); query->prepare(0);
    query::redis::session::REQUEST_QUERY request; request.pquery = query;
    int callbacks = 0; request.response = [&] { ++callbacks; };
    session->post_command(std::move(request), "PING", true);
    EXPECT_EQ(callbacks, 1); EXPECT_TRUE(session->sent.empty()); EXPECT_EQ(query->result().type, eRESP_DATA_TYPE::NONE_);
    EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    session->cancel_query_all(); EXPECT_EQ(callbacks, 1); owner->unregister_session(session);
}
}

namespace CGDK {
class test_atomic_query : public query::redis::Iquery
{
public:
    using Iquery::_begin_request;
    using Iquery::_publish_request;
    using Iquery::_wait_request;
    using Iquery::process_response;
    void set_value(std::string_view value) { m_result.type = eRESP_DATA_TYPE::SIMPLE_STRING_; m_result.str_value = value; }
};
TEST(CGDK10_redis_resp, atomic_completion_publishes_result_and_notifies_all_waiters_once)
{
    auto session = make_object<test_send_window_session>();
    auto query = make_object<test_atomic_query>(); query->set_session(session);
    std::atomic<int> callbacks{0}; query->_set_function_on_complete([&] { ++callbacks; });
    auto generation = query->_begin_request(); query->_publish_request(generation);
    std::promise<void> started; auto start = started.get_future();
    auto waiter = std::async(std::launch::async, [&] { started.set_value(); query->wait(); return query->result().str_value == "published"; });
    start.wait();
    auto another = std::async(std::launch::async, [&] { query->wait(); return query->result().str_value == "published"; });
    query->set_value("published");
    std::vector<std::thread> completers;
    for (int i = 0; i < 8; ++i) completers.emplace_back([&] { query->process_response(); });
    for (auto& thread : completers) thread.join();
    EXPECT_TRUE(waiter.get()); EXPECT_TRUE(another.get()); EXPECT_EQ(callbacks, 1);
    query->wait(); // Completion before waiting must not lose the notification.
    query->process_response(); EXPECT_EQ(callbacks, 1);
    query->close_query(); EXPECT_EQ(query->get_object_state(), eOBJECT_STATE::READY);
}
TEST(CGDK10_redis_resp, old_completion_generation_does_not_wait_for_a_reused_query)
{
    auto session = make_object<test_send_window_session>(); auto query = make_object<test_atomic_query>(); query->set_session(session);
    auto first = query->_begin_request(); query->_publish_request(first); query->process_response(); query->close_query();
    auto second = query->_begin_request(); query->_publish_request(second);
    EXPECT_NE(first, second);
    query->_wait_request(first, false);
    EXPECT_EQ(query->get_object_state(), eOBJECT_STATE::REQUESTING);
    EXPECT_THROW(query->_begin_request(), throwable::failure);
    query->process_response(); query->wait();
}
TEST(CGDK10_redis_resp, callback_can_close_and_repost_same_query_without_query_mutex)
{
    auto session = make_object<test_send_window_session>();
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto query = make_object<test_atomic_query>(); query->set_session(session); int callbacks = 0;
    query->post_execute_query(std::string_view("ECHO first"), [&] {
        EXPECT_EQ(query->result().str_value, "first"); ++callbacks;
        query->close_query();
        query->post_execute_query(std::string_view("ECHO second"), [&] { EXPECT_EQ(query->result().str_value, "second"); ++callbacks; });
    });
    session->feed("+first\r\n"); session->feed("+second\r\n");
    EXPECT_EQ(callbacks, 2); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    query->reset_session(); owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, circular_send_pipelines_before_prior_replies_and_completes_each_reply_immediately)
{
    auto io = executor::make_io_context_threads("redis circular pipeline", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto active = make_object<test_batch_query>(); active->prepare(2);
    query::redis::session::REQUEST_QUERY first; first.pquery = active; first.count_batch = 2;
    auto sent = session->sent_ready.get_future(); auto second = session->second_send_ready.get_future();
    session->post_batch(std::move(first), {"ECHO first", "ECHO last"});
    ASSERT_EQ(sent.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    std::vector<object_ptr<test_batch_query>> waiting; int callbacks = 0;
    {
        send_defer::scope defer;
        for (int i = 0; i < 3; ++i) {
            auto query = make_object<test_batch_query>(); query->prepare(0); waiting.push_back(query);
            query::redis::session::REQUEST_QUERY request; request.pquery = query; request.response = [&] { ++callbacks; };
            session->post_command(std::move(request), "ECHO queued" + std::to_string(i), true);
        }
    }
    // No reply has arrived: the next request group must already be sent.
    ASSERT_EQ(second.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(active->get_object_state(), eOBJECT_STATE::REQUESTING);
    EXPECT_EQ(session->send_calls, 2); EXPECT_EQ(session->staged(), 0u);
    std::string expected;
    for (int i = 0; i < 3; ++i) { auto encoded = generate_redis_command("ECHO queued" + std::to_string(i)); expected.append(encoded.data<char>(), encoded.size()); }
    EXPECT_EQ(session->sent[1], expected);
    session->feed("+first\r\n"); EXPECT_EQ(active->get_object_state(), eOBJECT_STATE::REQUESTING);
    session->feed("+last\r\n+queued0\r\n");
    EXPECT_EQ(callbacks, 1); EXPECT_EQ(waiting[0]->result().str_value, "queued0");
    EXPECT_EQ(waiting[1]->get_object_state(), eOBJECT_STATE::REQUESTING);
    session->feed("+queued1\r\n"); EXPECT_EQ(callbacks, 2);
    session->feed("+queued2\r\n"); EXPECT_EQ(callbacks, 3);
    EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
TEST(CGDK10_redis_resp, circular_send_cancel_completes_active_and_pending_once)
{
    auto io = executor::make_io_context_threads("redis vector cancel", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto sent = session->sent_ready.get_future(); std::atomic<int> callbacks{0};
    std::optional<send_defer::scope> defer;
    for (int i = 0; i < 6; ++i) {
        auto query = make_object<test_batch_query>(); query->prepare(0);
        query::redis::session::REQUEST_QUERY request; request.pquery = query; request.response = [&] { ++callbacks; };
        session->post_command(std::move(request), "PING", true);
        if (i == 0) { ASSERT_EQ(sent.wait_for(std::chrono::seconds(5)), std::future_status::ready); defer.emplace(); }
    }
    EXPECT_EQ(session->staged(), 5u); session->cancel_query_all(); session->cancel_query_all(); defer.reset();
    EXPECT_EQ(callbacks, 6); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, pending_circular_queue_keeps_send_defer_scope_after_prior_reply_completes)
{
    auto io = executor::make_io_context_threads("redis pending defer", 1);
    auto session = make_object<test_encoding_session>(); session->configure(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    auto sent = session->sent_ready.get_future(); auto second = session->second_send_ready.get_future();
    auto first_query = make_object<test_batch_query>(); first_query->prepare(0);
    query::redis::session::REQUEST_QUERY first; first.pquery = first_query;
    session->post_command(std::move(first), "PING", true);
    ASSERT_EQ(sent.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    {
        send_defer::scope defer;
        auto next_query = make_object<test_batch_query>(); next_query->prepare(0);
        query::redis::session::REQUEST_QUERY next; next.pquery = next_query;
        session->post_command(std::move(next), "PING", true);
        session->feed("+PONG\r\n");
        EXPECT_EQ(session->send_calls, 1); EXPECT_EQ(session->staged(), 1u);
    }
    ASSERT_EQ(second.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    session->feed("+PONG\r\n"); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, fixed_routes_choose_without_manager_or_session_mutex)
{
    auto owner = make_object<test_batch_querier>();
    auto a = make_object<test_send_window_session>(), b = make_object<test_send_window_session>();
    owner->register_session(a); owner->register_session(b); owner->set_object_state(eOBJECT_STATE::RUNNING); owner->publish_routes();
    auto manager = owner->lock_manager(); auto first = a->hold_admission(); auto second = b->hold_admission();
    auto request = std::async(std::launch::async, [&] { return owner->alloc_shared_query<test_batch_query>(); });
    const auto status = request.wait_for(std::chrono::seconds(2));
    EXPECT_EQ(status, std::future_status::ready);
    first.unlock(); second.unlock(); manager.unlock(); auto query = request.get();
    EXPECT_TRUE(query->get_session().get() == a.get() || query->get_session().get() == b.get()); query.reset();
    owner->close_routes(); owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(a); owner->unregister_session(b);
}
TEST(CGDK10_redis_resp, closed_fixed_routes_fail_without_entering_manager_and_reopen_with_new_sessions)
{
    auto owner = make_object<test_batch_querier>(); auto old = make_object<test_send_window_session>();
    owner->register_session(old); owner->set_object_state(eOBJECT_STATE::RUNNING); owner->publish_routes();
    auto held = owner->alloc_shared_query<test_batch_query>(); EXPECT_EQ(held->get_session().get(), old.get());
    owner->close_routes();
    auto manager = owner->lock_manager();
    auto rejected = std::async(std::launch::async, [&] {
        EXPECT_THROW((void)owner->alloc_shared_query(), throwable::failure);
        EXPECT_THROW(owner->post_execute_query(std::string_view("PING"), [](auto*) {}), throwable::failure);
        EXPECT_THROW(owner->post_execute_query({"PING", "PING"}, [](auto&&) {}), throwable::failure);
    });
    EXPECT_EQ(rejected.wait_for(std::chrono::seconds(2)), std::future_status::ready); manager.unlock(); rejected.get();
    owner->unregister_session(old); held.reset();
    auto fresh = make_object<test_send_window_session>(); owner->register_session(fresh); owner->publish_routes();
    auto query = owner->alloc_shared_query<test_batch_query>(); EXPECT_EQ(query->get_session().get(), fresh.get()); query.reset();
    owner->close_routes(); owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(fresh);
}
TEST(CGDK10_redis_resp, fixed_routes_do_not_take_a_dedicated_session)
{
    auto owner = make_object<test_batch_querier>(); auto a = make_object<test_send_window_session>(), b = make_object<test_send_window_session>();
    owner->register_session(a); owner->register_session(b); owner->set_object_state(eOBJECT_STATE::RUNNING); owner->publish_routes();
    auto exclusive = owner->alloc_query<test_batch_query>();
    for (int i = 0; i < 200; ++i) {
        auto shared = owner->alloc_shared_query<test_batch_query>(); EXPECT_NE(shared->get_session().get(), exclusive->get_session().get());
    }
    exclusive.reset(); owner->close_routes(); owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(a); owner->unregister_session(b);
}
TEST(CGDK10_redis_resp, fixed_shared_and_dedicated_admission_are_atomic_under_contention)
{
    auto owner = make_object<test_batch_querier>(); auto session = make_object<test_send_window_session>();
    owner->register_session(session); owner->set_object_state(eOBJECT_STATE::RUNNING); owner->publish_routes();
    std::atomic<int> errors{0}; std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) threads.emplace_back([&] {
        for (int j = 0; j < 500; ++j) {
            auto query = owner->alloc_shared_query<test_batch_query>();
            if (session->dedicated()) ++errors;
        }
    });
    threads.emplace_back([&] {
        for (int j = 0; j < 2000; ++j) {
            auto query = make_object<test_batch_query>();
            if (owner->try_dedicated(query.get()) && !session->dedicated()) ++errors;
        }
    });
    for (auto& thread : threads) thread.join(); EXPECT_EQ(errors, 0);
    owner->close_routes(); owner->set_object_state(eOBJECT_STATE::NONE); owner->unregister_session(session);
}
}

namespace CGDK {
class test_post_response_session : public test_send_window_session
{
public:
    void configure_response(object_ptr<Iexecutor> executor) { m_post_response = true; m_response_executor = std::move(executor); }
    std::size_t pending_callbacks() { std::lock_guard<std::mutex> lock(m_cs_response_queue); return m_queue_response.size(); }
};
TEST(CGDK10_redis_resp, synchronous_completion_stays_inline_when_response_post_is_enabled)
{
    auto io = executor::make_io_context_threads("sync inline response", 1);
    std::promise<void> entered, release; auto start = entered.get_future(); auto unblock = release.get_future();
    io->post([&] { entered.set_value(); unblock.wait(); }); start.wait();
    auto session = make_object<test_post_response_session>(); session->configure_response(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    for (std::size_t batch : {std::size_t(0), std::size_t(3)}) {
        auto query = window_request(session.get(), 0, false, {}, batch);
        session->feed(batch ? "+OK\r\n+OK\r\n+OK\r\n" : "+OK\r\n");
        EXPECT_EQ(query->get_object_state(), eOBJECT_STATE::PROCESSED);
        EXPECT_EQ(session->pending_callbacks(), 0u);
    }
    release.set_value(); dynamic_pointer_cast<object::Istartable>(io)->stop(); owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, posted_responses_return_from_receive_before_user_callbacks_and_own_results)
{
    auto io = executor::make_io_context_threads("response dispatch test", 1);
    std::promise<void> entered, release; auto start = entered.get_future(); auto ready = release.get_future();
    io->post([&] { entered.set_value(); ready.wait(); }); start.wait();
    auto session = make_object<test_post_response_session>(); session->configure_response(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    std::atomic<int> count{0}; std::promise<void> complete; auto complete_ready = complete.get_future();
    std::vector<object_ptr<test_batch_query>> queries;
    for (int i = 0; i < 3; ++i) {
        auto query = make_object<test_batch_query>(); query->prepare(0); queries.push_back(query);
        query::redis::session::REQUEST_QUERY request; request.pquery = query;
        request.response = [&, i, query] { EXPECT_EQ(query->result().str_value, std::to_string(i)); if (++count == 3) complete.set_value(); };
        request.buf_request = generate_redis_command("ECHO " + std::to_string(i)); session->execute_query(std::move(request));
    }
    session->feed("+0\r\n+1\r\n+2\r\n");
    EXPECT_EQ(count, 0); EXPECT_EQ(session->pending_callbacks(), 3u); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    release.set_value(); ASSERT_EQ(complete_ready.wait_for(std::chrono::seconds(5)), std::future_status::ready); EXPECT_EQ(count, 3);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
TEST(CGDK10_redis_resp, posted_responses_preserve_order_across_dispatch_batches)
{
    auto io = executor::make_io_context_threads("ordered response dispatch", 2);
    auto session = make_object<test_post_response_session>(); session->configure_response(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session);
    std::atomic<int> next{0}, active{0}, errors{0}; std::promise<void> complete; auto ready = complete.get_future();
    for (int i = 0; i < 600; ++i) {
        auto query = make_object<test_batch_query>(); query->prepare(0);
        query::redis::session::REQUEST_QUERY request; request.pquery = query;
        request.response = [&, i] { if (active.fetch_add(1) != 0) ++errors; if (next.fetch_add(1) != i) ++errors; active.fetch_sub(1); if (i == 599) complete.set_value(); };
        owner->statistics_on_query_try(); session->push_query(std::move(request));
    }
    std::string replies; for (int i = 0; i < 600; ++i) replies += "+OK\r\n";
    session->feed(replies); ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(errors, 0); EXPECT_EQ(next, 600); EXPECT_EQ(owner->statistics_get_query_processing(), 0u);
    owner->unregister_session(session); dynamic_pointer_cast<object::Istartable>(io)->stop();
}
TEST(CGDK10_redis_resp, posted_response_executor_rejection_falls_back_once)
{
    auto io = executor::make_io_context_threads("stopped response dispatch", 1); dynamic_pointer_cast<object::Istartable>(io)->stop();
    auto session = make_object<test_post_response_session>(); session->configure_response(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session); int count = 0;
    auto query = window_request(session.get(), 0, false, [&] { ++count; });
    session->feed("+OK\r\n"); EXPECT_EQ(count, 1); EXPECT_EQ(query->get_object_state(), eOBJECT_STATE::PROCESSED);
    session->cancel_query_all(); EXPECT_EQ(count, 1); owner->unregister_session(session);
}
TEST(CGDK10_redis_resp, stop_drains_queued_posted_callbacks_before_releasing_requests)
{
    auto io = executor::make_io_context_threads("cancel response dispatch", 1);
    std::promise<void> entered, release; auto start = entered.get_future(); auto unblock = release.get_future();
    io->post([&] { entered.set_value(); unblock.wait(); }); start.wait();
    auto session = make_object<test_post_response_session>(); session->configure_response(io);
    auto owner = make_object<test_batch_querier>(); owner->register_session(session); int count = 0;
    auto query = window_request(session.get(), 0, false, [&] { ++count; });
    session->feed("+OK\r\n"); EXPECT_EQ(count, 0);
    session->cancel_query_all(); EXPECT_EQ(count, 1); EXPECT_EQ(query->result().str_value, "OK");
    release.set_value(); dynamic_pointer_cast<object::Istartable>(io)->stop(); EXPECT_EQ(count, 1); owner->unregister_session(session);
}
}

namespace CGDK {
TEST(CGDK10_redis_resp, redis_session_does_not_use_native_send_cork)
{
    auto session = make_object<test_send_window_session>();
    EXPECT_FALSE(session->send_cork_enabled());
    EXPECT_FALSE(query::redis::querier::START_PARAMETER().post_response);
}
}
