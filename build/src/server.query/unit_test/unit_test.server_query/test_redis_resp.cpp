#include "pch.h"
#include "cgdk/sdk10/server.query.redis.h"
#include <cstring>
#include <string>
#include <vector>

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
	EXPECT_THROW((void)querier->execute_query_sync({}), throwable::failure);
	const std::string_view invalid[] = {"ping", " \t "};
	EXPECT_THROW((void)querier->execute_query_sync(invalid), throwable::failure);
	const std::vector<std::string> strings = {"ping", "echo test"};
	EXPECT_THROW((void)querier->execute_query_sync(strings), throwable::failure);
	EXPECT_THROW((void)querier->execute_query(strings), throwable::failure);
	bool called = false;
	EXPECT_THROW(querier->post_execute_query(strings, [&](auto&&) { called = true; }), throwable::failure);
	EXPECT_FALSE(called);
}

class test_batch_query : public query::redis::Iquery
{
public:
	void prepare(std::size_t count)
	{
		m_state_query = eOBJECT_STATE::REQUESTING;
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
};

class test_batch_querier : public query::redis::querier
{
public:
	using querier::register_session;
	using querier::unregister_session;
};

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
