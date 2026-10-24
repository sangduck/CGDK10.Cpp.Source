#include "pch.h"
#include <cstdlib>
#include <random>

//-----------------------------------------------------------------------------
// http / websocket parser (net::io::Nhttp_server, net::io::Nhttp_client)
//
//  - the data is fed in pieces like the receivable does (unprocessed bytes are kept and the next piece is appended)
//  - split at every byte, pipelined requests, Content-Length / chunked body, size limits
//  - websocket frames : 7/16/64 bit length, masked, split, ping
//-----------------------------------------------------------------------------
namespace CGDK
{

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------
// - feeds the bytes like the receivable (keeps the unprocessed bytes)
template <class TSOCKET>
class Nfeeder : public TSOCKET
{
public:
	// - the receivable runs only on a connected socket (the first request/response moves ESTABLISHED -> CERTIFIED)
			Nfeeder() { this->socket_state(eSOCKET_STATE::ESTABLISHED); }

			void				feed(std::string_view _data)
	{
		auto buf = alloc_shared_buffer(this->m_pending.size() + _data.size() + 16);
		if (this->m_pending.size() != 0)
			buf.append(this->m_pending.size(), this->m_pending.data());
		buf.append(_data.size(), _data.data());

		this->process_packet(buf, net::sockaddr());

		// - unprocessed bytes
		this->m_pending = (buf.size() != 0) ? shared_buffer(buf) : shared_buffer();
	}

	// - feeds in pieces of _size bytes
			void				feed_split(std::string_view _data, std::size_t _size)
	{
		for (std::size_t pos = 0; pos < _data.size(); pos += _size)
			this->feed(_data.substr(pos, _size));
	}

	[[nodiscard]] std::size_t	pending_size() const noexcept { return this->m_pending.size(); }

protected:
			shared_buffer		m_pending;
};

class test_http_server : public Nfeeder<net::socket::http_server>
{
public:
			std::vector<std::string> requests;			// - "METHOD path|body"
			std::vector<std::string> websocket_messages;

			void				set_websocket() { this->m_msg_http_request.protocol = eHTTP_PROTOCOL::WEBSOCKET_; }

protected:
	virtual void				on_connect() override {}
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& _msg) override
	{
		std::string body;
		for (const auto& iter : _msg.contents)
			body.append(reinterpret_cast<const char*>(iter.data()), iter.size());

		std::string_view method = (_msg.method == eHTTP_METHOD::GET_) ? "GET"sv : (_msg.method == eHTTP_METHOD::POST_) ? "POST"sv : "OTHER"sv;
		this->requests.push_back(std::string(method) + " " + std::string(reinterpret_cast<const char*>(_msg.path.data()), _msg.path.size()) + "|" + body);
		return eRESULT::DONE;
	}
	virtual result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& _msg) override
	{
		this->websocket_messages.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		return eRESULT::DONE;
	}
};

class test_http_client : public Nfeeder<net::socket::http_client>
{
public:
			std::vector<std::string> responses;			// - "status|body" (completed)
			std::vector<std::string> chunks;			// - chunks of a chunked response (not completed. buf_message)
			std::vector<std::string> websocket_messages;

			void				set_websocket() { this->m_msg_http_response.protocol = eHTTP_PROTOCOL::WEBSOCKET_; }

protected:
	virtual result_code			on_message(sMESSAGE_HTTP_RESPONSE& _msg) override
	{
		if (_msg.is_completed == false)
		{
			this->chunks.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
			return eRESULT::DONE;
		}

		std::string body(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		if (_msg.contents.empty() == false)
		{
			body.clear();
			for (const auto& iter : _msg.contents)
				body.append(reinterpret_cast<const char*>(iter.data()), iter.size());
		}

		this->responses.push_back(std::to_string(static_cast<int>(_msg.status_code)) + "|" + body);
		return eRESULT::DONE;
	}
	virtual result_code			on_message(sMESSAGE_HTTP_WEBSOCKET& _msg) override
	{
		this->websocket_messages.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		return eRESULT::DONE;
	}
};

// - websocket frame (client -> server : masked. _is_fin false : a fragment)
static std::string make_frame(uint8_t _opcode, std::string_view _payload, bool _is_masked = true, bool _is_fin = true)
{
	std::string frame;
	frame.push_back(static_cast<char>((_is_fin ? 0x80 : 0x00) | _opcode));

	const uint8_t mask_bit = _is_masked ? 0x80 : 0x00;
	if (_payload.size() < 126)
	{
		frame.push_back(static_cast<char>(mask_bit | _payload.size()));
	}
	else if (_payload.size() <= 0xffff)
	{
		frame.push_back(static_cast<char>(mask_bit | 126));
		frame.push_back(static_cast<char>((_payload.size() >> 8) & 0xff));
		frame.push_back(static_cast<char>(_payload.size() & 0xff));
	}
	else
	{
		frame.push_back(static_cast<char>(mask_bit | 127));
		for (int i = 7; i >= 0; --i)
			frame.push_back(static_cast<char>((static_cast<uint64_t>(_payload.size()) >> (i * 8)) & 0xff));
	}

	if (_is_masked)
	{
		const char mask[4] = { 0x12, 0x34, 0x56, 0x78 };
		frame.append(mask, 4);
		for (std::size_t i = 0; i < _payload.size(); ++i)
			frame.push_back(static_cast<char>(_payload[i] ^ mask[i % 4]));
	}
	else
	{
		frame.append(_payload);
	}
	return frame;
}

//-----------------------------------------------------------------------------
// server : request
//-----------------------------------------------------------------------------
TEST(http_parser, server_simple_get)
{
	auto psocket = make_own<test_http_server>();
	psocket->feed("GET /api/instances HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n");

	ASSERT_EQ(psocket->requests.size(), 1u);
	EXPECT_EQ(psocket->requests[0], "GET /api/instances|");
	EXPECT_EQ(psocket->pending_size(), 0u);
}

TEST(http_parser, server_split_at_every_byte)
{
	const std::string request = "GET /webpage_host_info.htm HTTP/1.1\r\nHost: 127.0.0.1:22052\r\nUser-Agent: test\r\n\r\n";

	for (std::size_t size = 1; size < request.size(); ++size)
	{
		auto psocket = make_own<test_http_server>();
		psocket->feed_split(request, size);

		ASSERT_EQ(psocket->requests.size(), 1u) << "piece size " << size;
		EXPECT_EQ(psocket->requests[0], "GET /webpage_host_info.htm|") << "piece size " << size;
	}
}

TEST(http_parser, server_large_header_split)
{
	// - header longer than the first receive buffer (1KB) arrives in several pieces
	const std::string request = "GET /metrics HTTP/1.1\r\nX-Big: " + std::string(10 * 1024, 'y') + "\r\n\r\n";

	auto psocket = make_own<test_http_server>();
	psocket->feed_split(request, 1000);

	ASSERT_EQ(psocket->requests.size(), 1u);
	EXPECT_EQ(psocket->requests[0], "GET /metrics|");
}

TEST(http_parser, server_body)
{
	const std::string request = "POST /login HTTP/1.1\r\nHost: x\r\nContent-Length: 11\r\n\r\nkey=web-key";

	for (std::size_t size : { std::size_t(1), std::size_t(7), request.size() })
	{
		auto psocket = make_own<test_http_server>();
		psocket->feed_split(request, size);

		ASSERT_EQ(psocket->requests.size(), 1u) << "piece size " << size;
		EXPECT_EQ(psocket->requests[0], "POST /login|key=web-key") << "piece size " << size;
	}

	// - header name is case insensitive
	auto psocket = make_own<test_http_server>();
	psocket->feed("POST /login HTTP/1.1\r\ncontent-length: 3\r\n\r\nabc");
	ASSERT_EQ(psocket->requests.size(), 1u);
	EXPECT_EQ(psocket->requests[0], "POST /login|abc");
}

TEST(http_parser, server_pipelined)
{
	// - two requests in one receive
	auto psocket = make_own<test_http_server>();
	psocket->feed("GET /a HTTP/1.1\r\nHost: x\r\n\r\nPOST /b HTTP/1.1\r\nContent-Length: 2\r\n\r\nhiGET /c HTTP/1.1\r\n\r\n");

	ASSERT_EQ(psocket->requests.size(), 3u);
	EXPECT_EQ(psocket->requests[0], "GET /a|");
	EXPECT_EQ(psocket->requests[1], "POST /b|hi");
	EXPECT_EQ(psocket->requests[2], "GET /c|");
	EXPECT_EQ(psocket->pending_size(), 0u);
}

TEST(http_parser, server_chunked)
{
	const std::string request = "POST /c HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n";

	for (std::size_t size : { request.size(), std::size_t(1), std::size_t(5) })
	{
		auto psocket = make_own<test_http_server>();
		psocket->feed_split(request, size);

		ASSERT_EQ(psocket->requests.size(), 1u) << "piece size " << size;
		EXPECT_EQ(psocket->requests[0], "POST /c|hello world") << "piece size " << size;
	}
}

TEST(http_parser, server_limits)
{
	// - header (with terminator)
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(1024, 1024);
		EXPECT_ANY_THROW(psocket->feed("GET / HTTP/1.1\r\nX-Big: " + std::string(2000, 'y') + "\r\n\r\n"));
	}

	// - header (no terminator yet)
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(1024, 1024);
		EXPECT_ANY_THROW(psocket->feed("GET / HTTP/1.1\r\nX-Big: " + std::string(2000, 'y')));
	}

	// - body (Content-Length)
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(1024, 100);
		EXPECT_ANY_THROW(psocket->feed("POST / HTTP/1.1\r\nContent-Length: 101\r\n\r\n"));
	}

	// - body (chunked total)
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(1024, 10);
		EXPECT_ANY_THROW(psocket->feed("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n8\r\n12345678\r\n8\r\n12345678\r\n0\r\n\r\n"));
	}

	// - within the limits
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(1024, 100);
		EXPECT_NO_THROW(psocket->feed("POST / HTTP/1.1\r\nContent-Length: 100\r\n\r\n" + std::string(100, 'x')));
		EXPECT_EQ(psocket->requests.size(), 1u);
	}
}

//-----------------------------------------------------------------------------
// server : websocket frame
//-----------------------------------------------------------------------------
TEST(http_parser, websocket_frames)
{
	for (std::size_t length : { std::size_t(5), std::size_t(125), std::size_t(126), std::size_t(300), std::size_t(70000) })
	{
		const std::string payload(length, 'a');

		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->feed(make_frame(0x1, payload));

		ASSERT_EQ(psocket->websocket_messages.size(), 1u) << "length " << length;
		EXPECT_EQ(psocket->websocket_messages[0], payload) << "length " << length;
		EXPECT_EQ(psocket->pending_size(), 0u) << "length " << length;
	}
}

TEST(http_parser, websocket_split)
{
	// - two frames (16 bit length) split at every byte
	const std::string data = make_frame(0x1, std::string(200, 'x')) + make_frame(0x2, "binary");

	for (std::size_t size : { std::size_t(1), std::size_t(2), std::size_t(3), std::size_t(7), std::size_t(64) })
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->feed_split(data, size);

		ASSERT_EQ(psocket->websocket_messages.size(), 2u) << "piece size " << size;
		EXPECT_EQ(psocket->websocket_messages[0], std::string(200, 'x')) << "piece size " << size;
		EXPECT_EQ(psocket->websocket_messages[1], "binary") << "piece size " << size;
	}
}

TEST(http_parser, websocket_limit)
{
	auto psocket = make_own<test_http_server>();
	psocket->set_websocket();
	psocket->set_http_limits(1024, 1000);
	EXPECT_ANY_THROW(psocket->feed(make_frame(0x2, std::string(1001, 'x'))));
}

TEST(http_parser, websocket_ping_is_not_a_message)
{
	auto psocket = make_own<test_http_server>();
	psocket->set_websocket();
	psocket->feed(make_frame(0x9, "ping") + make_frame(0x1, "after"));

	ASSERT_EQ(psocket->websocket_messages.size(), 1u);
	EXPECT_EQ(psocket->websocket_messages[0], "after");
}

//-----------------------------------------------------------------------------
// client : response
//-----------------------------------------------------------------------------
TEST(http_parser, client_response)
{
	const std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\ncontent-length: 5\r\n\r\nhello";

	for (std::size_t size : { response.size(), std::size_t(1), std::size_t(10) })
	{
		auto psocket = make_own<test_http_client>();
		psocket->feed_split(response, size);

		ASSERT_EQ(psocket->responses.size(), 1u) << "piece size " << size;
		EXPECT_EQ(psocket->responses[0], "200|hello") << "piece size " << size;
	}
}

TEST(http_parser, client_large_header)
{
	const std::string response = "HTTP/1.1 200 OK\r\nX-Big: " + std::string(5000, 'y') + "\r\nContent-Length: 2\r\n\r\nok";

	auto psocket = make_own<test_http_client>();
	psocket->feed_split(response, 1000);

	ASSERT_EQ(psocket->responses.size(), 1u);
	EXPECT_EQ(psocket->responses[0], "200|ok");
}

TEST(http_parser, client_limits)
{
	// - the client delivers a parse error as a response (413) and disconnects
	auto psocket = make_own<test_http_client>();
	psocket->set_http_limits(1024, 100);
	psocket->feed("HTTP/1.1 200 OK\r\nContent-Length: 101\r\n\r\n");
	ASSERT_EQ(psocket->responses.size(), 1u);
	EXPECT_EQ(psocket->responses[0].substr(0, 4), "413|");

	auto psocket2 = make_own<test_http_client>();
	psocket2->set_http_limits(1024, 100);
	psocket2->feed("HTTP/1.1 200 OK\r\nX-Big: " + std::string(2000, 'y') + "\r\n\r\n");
	ASSERT_EQ(psocket2->responses.size(), 1u);
	EXPECT_EQ(psocket2->responses[0].substr(0, 4), "413|");
}

//-----------------------------------------------------------------------------
// websocket fragments (continuation frames)
//-----------------------------------------------------------------------------
TEST(http_parser, websocket_fragments)
{
	// - text "hel" + continuation "lo " + (ping) + continuation(FIN) "world" -> one message
	const std::string data = make_frame(0x1, "hel", true, false) + make_frame(0x0, "lo ", true, false) + make_frame(0x9, "p") + make_frame(0x0, "world", true, true) + make_frame(0x2, "next");

	for (std::size_t size : { data.size(), std::size_t(1), std::size_t(3) })
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->feed_split(data, size);

		ASSERT_EQ(psocket->websocket_messages.size(), 2u) << "piece size " << size;
		EXPECT_EQ(psocket->websocket_messages[0], "hello world") << "piece size " << size;
		EXPECT_EQ(psocket->websocket_messages[1], "next") << "piece size " << size;
	}

	// - continuation without the first frame is dropped
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->feed(make_frame(0x0, "orphan", true, true) + make_frame(0x1, "ok"));
		ASSERT_EQ(psocket->websocket_messages.size(), 1u);
		EXPECT_EQ(psocket->websocket_messages[0], "ok");
	}

	// - the total size of the fragments is limited
	{
		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->set_http_limits(1024, 100);
		EXPECT_ANY_THROW(psocket->feed(make_frame(0x2, std::string(60, 'a'), true, false) + make_frame(0x0, std::string(60, 'b'), true, true)));
	}
}

TEST(http_parser, client_websocket_fragments)
{
	// - server -> client : not masked
	auto psocket = make_own<test_http_client>();
	psocket->set_websocket();
	psocket->feed(make_frame(0x1, "ab", false, false) + make_frame(0x0, "cd", false, true) + make_frame(0x1, "single", false));

	ASSERT_EQ(psocket->websocket_messages.size(), 2u);
	EXPECT_EQ(psocket->websocket_messages[0], "abcd");
	EXPECT_EQ(psocket->websocket_messages[1], "single");
}

//-----------------------------------------------------------------------------
// client : chunked response
//-----------------------------------------------------------------------------
TEST(http_parser, client_chunked)
{
	const std::string response = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6;ext=1\r\n world\r\n0\r\nX-Trailer: t\r\n\r\n";

	for (std::size_t size : { response.size(), std::size_t(1), std::size_t(4) })
	{
		auto psocket = make_own<test_http_client>();
		psocket->feed_split(response, size);

		// - each chunk (is_completed == false) and the whole body at the end (is_completed == true)
		ASSERT_EQ(psocket->chunks.size(), 2u) << "piece size " << size;
		EXPECT_EQ(psocket->chunks[0], "hello") << "piece size " << size;
		EXPECT_EQ(psocket->chunks[1], " world") << "piece size " << size;
		ASSERT_EQ(psocket->responses.size(), 1u) << "piece size " << size;
		EXPECT_EQ(psocket->responses[0], "200|hello world") << "piece size " << size;
	}

	// - two responses (chunked + normal) in one receive
	auto psocket = make_own<test_http_client>();
	psocket->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nok\r\n0\r\n\r\nHTTP/1.1 404 Not Found\r\nContent-Length: 2\r\n\r\nno");
	ASSERT_EQ(psocket->responses.size(), 2u);
	EXPECT_EQ(psocket->responses[0], "200|ok");
	EXPECT_EQ(psocket->responses[1], "404|no");

	// - chunked total size is limited (413)
	auto psocket2 = make_own<test_http_client>();
	psocket2->set_http_limits(1024, 10);
	psocket2->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n8\r\n12345678\r\n8\r\n12345678\r\n0\r\n\r\n");
	ASSERT_FALSE(psocket2->responses.empty());
	EXPECT_EQ(psocket2->responses.back().substr(0, 4), "413|");
}


//-----------------------------------------------------------------------------
// request line / status line (found by the fuzz test : short lines were read out of range)
//-----------------------------------------------------------------------------
TEST(http_parser, server_methods)
{
	for (std::string_view method : { "GET"sv, "POST"sv, "PUT"sv, "PURGE"sv, "PATCH"sv, "PROPFIND"sv, "COPY"sv, "DELETE"sv, "HEAD"sv, "OPTIONS"sv, "LINK"sv, "LOCK"sv, "UNLINK"sv, "UNLOCK"sv, "VIEW"sv, "TRACE"sv })
	{
		auto psocket = make_own<test_http_server>();
		EXPECT_NO_THROW(psocket->feed(std::string(method) + " /x HTTP/1.1\r\n\r\n")) << method;
		EXPECT_EQ(psocket->requests.size(), 1u) << method;
	}

	// - short or unknown method : 400 (exception)
	for (std::string_view line : { "G"sv, "GE"sv, "GET"sv, "P"sv, "PO"sv, "PU"sv, "PUR"sv, "C"sv, "COP"sv, "OPTION"sv, "X /x HTTP/1.1"sv, "get /x HTTP/1.1"sv, "GETX /x HTTP/1.1"sv })
	{
		auto psocket = make_own<test_http_server>();
		EXPECT_ANY_THROW(psocket->feed(std::string(line) + "\r\n\r\n")) << line;
		EXPECT_TRUE(psocket->requests.empty()) << line;
	}
}

TEST(http_parser, client_status_line)
{
	// - short protocol : an error response (not read out of range. the client converts the parse error to an error response)
	for (std::string_view line : { "H 200 OK"sv, "HT 200 OK"sv, "HTT 200 OK"sv, "XTTP/1.1 200 OK"sv })
	{
		auto psocket = make_own<test_http_client>();
		EXPECT_NO_THROW(psocket->feed(std::string(line) + "\r\nContent-Length: 0\r\n\r\n")) << line;
		ASSERT_EQ(psocket->responses.size(), 1u) << line;
		EXPECT_NE(psocket->responses[0].substr(0, 4), "200|") << line;
	}

	// - case insensitive (protocol, header names)
	auto psocket = make_own<test_http_client>();
	psocket->feed("http/1.1 200 OK\r\ntransfer-encoding: Chunked\r\n\r\n2\r\nok\r\n0\r\n\r\n");
	ASSERT_EQ(psocket->responses.size(), 1u);
	EXPECT_EQ(psocket->responses[0], "200|ok");
}

//-----------------------------------------------------------------------------
// random input (fuzz)
//
//  - valid inputs are changed at random (bit/byte change, insert, delete, truncate) or random bytes,
//    fed in random pieces
//  - no crash, no hang, the bytes kept by the parser are limited by set_http_limits
//    (an exception is the invalid input : the socket is closed)
//  - the websocket upgrade request is not used (the handshake needs the acceptor : e2e)
//  - CGDK_FUZZ_ITERATIONS : the iterations of each kind (default 2000) / CGDK_FUZZ_SEED : the seed (default 20261011)
//-----------------------------------------------------------------------------
namespace
{
	std::size_t fuzz_env(const char* _name, std::size_t _default)
	{
#if defined(_MSC_VER)
		char* value = nullptr;
		std::size_t length = 0;
		if (_dupenv_s(&value, &length, _name) != 0 || value == nullptr)
			return _default;
		std::unique_ptr<char, decltype(&free)> holder(value, &free);
#else
		const char* value = std::getenv(_name);
#endif
		return (value != nullptr && *value != '\0') ? static_cast<std::size_t>(std::strtoull(value, nullptr, 10)) : _default;
	}

	std::string fuzz_mutate(std::mt19937& _rng, std::string _data)
	{
		const auto count = 1 + _rng() % 4;
		for (std::size_t i = 0; i < count; ++i)
		{
			switch (_rng() % 6)
			{
			case 0:	if (_data.empty() == false) _data[_rng() % _data.size()] ^= static_cast<char>(1 << (_rng() % 8)); break;
			case 1:	if (_data.empty() == false) _data[_rng() % _data.size()] = static_cast<char>(_rng() % 256); break;
			case 2:	_data.insert(_data.begin() + static_cast<std::ptrdiff_t>(_data.empty() ? 0 : _rng() % _data.size()), static_cast<char>(_rng() % 256)); break;
			case 3:	if (_data.empty() == false) _data.erase(_rng() % _data.size(), 1 + _rng() % 8); break;
			case 4:	if (_data.empty() == false) _data.resize(_rng() % _data.size()); break;
			case 5:	_data += _data.substr(0, _rng() % (_data.size() + 1)); break;	// - repeats a part
			}
		}
		return _data;
	}

	std::string fuzz_random_bytes(std::mt19937& _rng, std::size_t _max_size)
	{
		std::string data(_rng() % (_max_size + 1), '\0');
		for (auto& iter : data)
			iter = static_cast<char>(_rng() % 256);
		return data;
	}

	// - feeds in random pieces (1~64 bytes). stops at the exception (the socket is closed)
	//   returns false if the bytes kept by the parser are more than _max_pending
	template <class TSOCKET>
	bool fuzz_feed(std::mt19937& _rng, TSOCKET& _socket, const std::string& _data, std::size_t _max_pending)
	{
		for (std::size_t pos = 0; pos < _data.size();)
		{
			const std::size_t size = 1 + _rng() % 64;
			try
			{
				_socket.feed(std::string_view(_data).substr(pos, size));
			}
			catch (...)
			{
				return true;
			}
			if (_socket.pending_size() > _max_pending)
				return false;
			pos += size;
		}
		return true;
	}

	constexpr std::size_t FUZZ_MAX_HEADER = 1024;
	constexpr std::size_t FUZZ_MAX_BODY = 4096;
	constexpr std::size_t FUZZ_MAX_PENDING = FUZZ_MAX_HEADER + FUZZ_MAX_BODY + 64 + 16;	// - header + body + a piece + frame header
}

TEST(http_fuzz, server_request)
{
	const auto iterations = fuzz_env("CGDK_FUZZ_ITERATIONS", 2000);
	std::mt19937 rng(static_cast<uint32_t>(fuzz_env("CGDK_FUZZ_SEED", 20261011)));

	const std::vector<std::string> seeds =
	{
		"GET /api/instances HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n",
		"POST /login HTTP/1.1\r\nContent-Length: 11\r\nContent-Type: text/plain\r\n\r\nhello world",
		"POST /a HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n",
		"GET /a HTTP/1.1\r\nHost: x\r\n\r\nPOST /b HTTP/1.1\r\nContent-Length: 2\r\n\r\nhiGET /c HTTP/1.1\r\n\r\n",
		"GET / HTTP/1.1\r\nX-Long: " + std::string(900, 'y') + "\r\n\r\n",
	};

	for (std::size_t i = 0; i < iterations; ++i)
	{
		const auto data = (i % 8 == 7) ? fuzz_random_bytes(rng, 512) : fuzz_mutate(rng, seeds[rng() % seeds.size()]);

		auto psocket = make_own<test_http_server>();
		psocket->set_http_limits(FUZZ_MAX_HEADER, FUZZ_MAX_BODY);
		ASSERT_TRUE(fuzz_feed(rng, *psocket, data, FUZZ_MAX_PENDING)) << "iteration " << i << " pending " << psocket->pending_size();
	}
}

TEST(http_fuzz, server_websocket)
{
	const auto iterations = fuzz_env("CGDK_FUZZ_ITERATIONS", 2000);
	std::mt19937 rng(static_cast<uint32_t>(fuzz_env("CGDK_FUZZ_SEED", 20261011)) + 1);

	const std::vector<std::string> seeds =
	{
		make_frame(0x1, "hello"),
		make_frame(0x2, std::string(300, 'b')),
		make_frame(0x1, "ab", true, false) + make_frame(0x0, "cd", true, false) + make_frame(0x0, "ef"),
		make_frame(0x9, "ping") + make_frame(0x1, "after"),
		make_frame(0xA, "pong") + make_frame(0x8, "\x03\xe8"),
		make_frame(0x1, std::string(3000, 'x')),
	};

	for (std::size_t i = 0; i < iterations; ++i)
	{
		const auto data = (i % 8 == 7) ? fuzz_random_bytes(rng, 512) : fuzz_mutate(rng, seeds[rng() % seeds.size()]);

		auto psocket = make_own<test_http_server>();
		psocket->set_websocket();
		psocket->set_http_limits(FUZZ_MAX_HEADER, FUZZ_MAX_BODY);
		ASSERT_TRUE(fuzz_feed(rng, *psocket, data, FUZZ_MAX_PENDING)) << "iteration " << i << " pending " << psocket->pending_size();
	}
}

TEST(http_fuzz, client_response)
{
	const auto iterations = fuzz_env("CGDK_FUZZ_ITERATIONS", 2000);
	std::mt19937 rng(static_cast<uint32_t>(fuzz_env("CGDK_FUZZ_SEED", 20261011)) + 2);

	const std::vector<std::string> seeds =
	{
		"HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello",
		"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n6\r\n world\r\n0\r\n\r\n",
		"HTTP/1.1 404 Not Found\r\nContent-Length: 2\r\n\r\nnoHTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n",
		"HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n\r\n",
		"HTTP/1.1 200 OK\r\nX-Long: " + std::string(900, 'y') + "\r\nContent-Length: 1\r\n\r\nz",
	};

	for (std::size_t i = 0; i < iterations; ++i)
	{
		const auto data = (i % 8 == 7) ? fuzz_random_bytes(rng, 512) : fuzz_mutate(rng, seeds[rng() % seeds.size()]);

		auto psocket = make_own<test_http_client>();
		psocket->set_http_limits(FUZZ_MAX_HEADER, FUZZ_MAX_BODY);
		ASSERT_TRUE(fuzz_feed(rng, *psocket, data, FUZZ_MAX_PENDING)) << "iteration " << i << " pending " << psocket->pending_size();
	}
}

TEST(http_fuzz, client_websocket)
{
	const auto iterations = fuzz_env("CGDK_FUZZ_ITERATIONS", 2000);
	std::mt19937 rng(static_cast<uint32_t>(fuzz_env("CGDK_FUZZ_SEED", 20261011)) + 3);

	const std::vector<std::string> seeds =
	{
		make_frame(0x1, "hello", false),
		make_frame(0x2, std::string(300, 'b'), false),
		make_frame(0x1, "ab", false, false) + make_frame(0x0, "cd", false),
		make_frame(0x9, "ping", false) + make_frame(0x1, "after", false),
	};

	for (std::size_t i = 0; i < iterations; ++i)
	{
		const auto data = (i % 8 == 7) ? fuzz_random_bytes(rng, 512) : fuzz_mutate(rng, seeds[rng() % seeds.size()]);

		auto psocket = make_own<test_http_client>();
		psocket->set_websocket();
		psocket->set_http_limits(FUZZ_MAX_HEADER, FUZZ_MAX_BODY);
		ASSERT_TRUE(fuzz_feed(rng, *psocket, data, FUZZ_MAX_PENDING)) << "iteration " << i << " pending " << psocket->pending_size();
	}
}

}

//-----------------------------------------------------------------------------
// net::http_request::set_request : Content-Length from the body, the body as it is
//-----------------------------------------------------------------------------
namespace CGDK
{
	TEST(CGDK10_http_request, content_length_and_body)
	{
		auto prequest = make_object<net::http_request>();

		const std::string body = "{\"text\":\"hello\"}";
		const std::vector<std::string_view> headers{ "Content-Type: application/json"sv, "content-length: 999"sv, "Connection: close"sv };
		const std::vector<std::string_view> accept;
		prequest->set_request(eHTTP_METHOD::POST_, "127.0.0.1"sv, 8080, "HTTP/1.1"sv, "/api/post"sv, ""sv, headers, to_buffer_view(std::string_view(body)), accept, false);

		const auto buf = prequest->get_request_buffer();
		const std::string text(buf.data(), buf.size());

		// - Content-Length : once, the size of the body (the one given is ignored)
		EXPECT_NE(text.find("Content-Length: " + std::to_string(body.size()) + "\r\n"), std::string::npos);
		EXPECT_EQ(text.find("999"), std::string::npos);
		size_t count = 0;
		for (size_t pos = 0; (pos = text.find("ontent-", pos)) != std::string::npos; ++pos)
			if (text.compare(pos, 13, "ontent-Length") == 0 || text.compare(pos, 13, "ontent-length") == 0)
				++count;
		EXPECT_EQ(count, 1u);

		// - the body is the end (nothing is appended after it)
		ASSERT_GE(text.size(), body.size() + 4);
		EXPECT_EQ(text.substr(text.size() - body.size() - 4), "\r\n\r\n" + body);
	}

	TEST(CGDK10_http_request, no_body_get_and_large_headers)
	{
		auto prequest = make_object<net::http_request>();

		// - a header larger than the old fixed buffer (8192)
		const std::string large(20000, 'x');
		const std::string header = "X-Large: " + large;
		const std::vector<std::string_view> headers{ header };
		const std::vector<std::string_view> accept{ "*/*"sv };
		prequest->set_request(eHTTP_METHOD::GET_, "127.0.0.1"sv, 8080, "HTTP/1.1"sv, "/"sv, ""sv, headers, buffer_view(), accept, false);

		const auto buf = prequest->get_request_buffer();
		const std::string text(buf.data(), buf.size());

		// - GET without a body : no Content-Length, the request ends with the end of the header
		EXPECT_EQ(text.find("Content-Length"), std::string::npos);
		EXPECT_NE(text.find(large), std::string::npos);
		ASSERT_GE(text.size(), 4u);
		EXPECT_EQ(text.substr(text.size() - 4), "\r\n\r\n");
	}
}


//-----------------------------------------------------------------------------
// http client parser : the body of a response (RFC 9112 6.3)
//
//  - "Content-Length: 0", 204 : completed without the body (not the raw buffer)
//  - 1xx : an interim response (ignored)
//  - "Transfer-Encoding" overrides "Content-Length", chunked if the last coding is "chunked"
//  - an invalid "Content-Length" or chunk : '502 Bad Gateway'
//  - no "Content-Length" and not chunked : the body until the connection is closed (not completed yet)
//-----------------------------------------------------------------------------
namespace CGDK
{
	TEST(http_parser, client_no_body)
	{
		for (std::string_view response : { "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n"sv, "HTTP/1.1 204 No Content\r\n\r\n"sv, "HTTP/1.1 304 Not Modified\r\nContent-Length: 10\r\n\r\n"sv })
		{
			for (std::size_t size : { response.size(), std::size_t(1) })
			{
				auto psocket = make_own<test_http_client>();
				psocket->feed_split(response, size);

				ASSERT_EQ(psocket->responses.size(), 1u) << response;
				EXPECT_EQ(psocket->responses[0], std::string(response.substr(9, 3)) + "|") << response;
				EXPECT_EQ(psocket->pending_size(), 0u) << response;
			}
		}
	}

	TEST(http_parser, client_interim_1xx)
	{
		const std::string response = "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 103 Early Hints\r\nLink: </a.css>\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";

		for (std::size_t size : { response.size(), std::size_t(1), std::size_t(7) })
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed_split(response, size);

			ASSERT_EQ(psocket->responses.size(), 1u) << "piece size " << size;
			EXPECT_EQ(psocket->responses[0], "200|ok") << "piece size " << size;
		}
	}

	TEST(http_parser, client_transfer_encoding)
	{
		// - "Transfer-Encoding: chunked" overrides "Content-Length"
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed("HTTP/1.1 200 OK\r\nContent-Length: 100\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nok\r\n0\r\n\r\n");
			ASSERT_EQ(psocket->responses.size(), 1u);
			EXPECT_EQ(psocket->responses[0], "200|ok");
		}

		// - the last coding is "chunked" (spaces around)
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip, Chunked  \r\n\r\n3\r\nabc\r\n0\r\n\r\n");
			ASSERT_EQ(psocket->responses.size(), 1u);
			EXPECT_EQ(psocket->responses[0], "200|abc");
		}

		// - "Transfer-Encoding" not chunked : the body until close (not completed)
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\nContent-Length: 2\r\n\r\nokmore");
			EXPECT_TRUE(psocket->responses.empty());
			EXPECT_EQ(psocket->pending_size(), 0u);
		}
	}

	TEST(http_parser, client_content_length)
	{
		// - invalid : '502 Bad Gateway'
		for (std::string_view value : { "abc"sv, "-1"sv, "1 2"sv, "12x"sv, ""sv, "99999999999999999999999999"sv })
		{
			auto psocket = make_own<test_http_client>();
			EXPECT_NO_THROW(psocket->feed("HTTP/1.1 200 OK\r\nContent-Length: " + std::string(value) + "\r\n\r\nok")) << value;
			ASSERT_EQ(psocket->responses.size(), 1u) << value;
			EXPECT_EQ(psocket->responses[0].substr(0, 4), "502|") << value;
		}

		// - different values : '502 Bad Gateway'
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nContent-Length: 3\r\n\r\nokk");
			ASSERT_EQ(psocket->responses.size(), 1u);
			EXPECT_EQ(psocket->responses[0].substr(0, 4), "502|");
		}

		// - the same value repeated, spaces around : valid
		{
			auto psocket = make_own<test_http_client>();
			psocket->feed("HTTP/1.1 200 OK\r\nContent-Length:  2 \r\ncontent-length: 2\r\n\r\nok");
			ASSERT_EQ(psocket->responses.size(), 1u);
			EXPECT_EQ(psocket->responses[0], "200|ok");
		}
	}

	TEST(http_parser, client_chunk_without_crlf)
	{
		// - the data of a chunk must be followed by CRLF : '502 Bad Gateway'
		auto psocket = make_own<test_http_client>();
		EXPECT_NO_THROW(psocket->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nokXX0\r\n\r\n"));
		ASSERT_FALSE(psocket->responses.empty());
		EXPECT_EQ(psocket->responses.back().substr(0, 4), "502|");

		// - a huge chunk size : '502 Bad Gateway' (no overflow)
		auto psocket_huge = make_own<test_http_client>();
		psocket_huge->set_http_limits(0, 0);
		EXPECT_NO_THROW(psocket_huge->feed("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\nffffffffffffffff\r\nok\r\n"));
		ASSERT_FALSE(psocket_huge->responses.empty());
		EXPECT_EQ(psocket_huge->responses.back().substr(0, 4), "502|");
	}

	TEST(http_parser, client_body_until_close)
	{
		// - no "Content-Length", not chunked : kept until the connection is closed (completed by process_on_disconnect)
		auto psocket = make_own<test_http_client>();
		psocket->feed_split("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nhello"sv, 3);
		psocket->feed(" world");

		EXPECT_TRUE(psocket->responses.empty());
		EXPECT_EQ(psocket->pending_size(), 0u);

		// - limited by the max body size
		auto psocket_limit = make_own<test_http_client>();
		psocket_limit->set_http_limits(64 * 1024, 8);
		EXPECT_NO_THROW(psocket_limit->feed("HTTP/1.1 200 OK\r\n\r\n0123456789"));
		ASSERT_EQ(psocket_limit->responses.size(), 1u);
		EXPECT_EQ(psocket_limit->responses[0].substr(0, 4), "413|");
	}
}


//-----------------------------------------------------------------------------
// net::http_request::set_request (url)
//
//  - GET : the query is in the path, the fragment isn't sent, no Content-Length
//  - POST : the query is the body (form)
//  - "[ipv6]:port", the default path "/", invalid urls, "Connection: close"
//  - websocket : a random Sec-WebSocket-Key (24 characters)
//-----------------------------------------------------------------------------
namespace CGDK
{
	namespace
	{
		std::string request_text(const object_ptr<net::http_request>& _prequest)
		{
			const auto buf = _prequest->get_request_buffer();
			return std::string(buf.data(), buf.size());
		}

		std::string first_line(const std::string& _text)
		{
			return _text.substr(0, _text.find("\r\n"));
		}
	}

	TEST(CGDK10_http_request, url_get_query_and_fragment)
	{
		auto prequest = make_object<net::http_request>();
		prequest->set_request("http://127.0.0.1:8080/a/b?x=1&y=2#frag"sv);

		const auto text = request_text(prequest);
		EXPECT_EQ(first_line(text), "GET /a/b?x=1&y=2 HTTP/1.1");
		EXPECT_NE(text.find("Host: 127.0.0.1:8080\r\n"), std::string::npos);
		EXPECT_NE(text.find("Connection: close\r\n"), std::string::npos);
		EXPECT_EQ(text.find("Content-Length"), std::string::npos);
		EXPECT_EQ(text.find("frag"), std::string::npos);
		EXPECT_EQ(text.substr(text.size() - 4), "\r\n\r\n");
		EXPECT_EQ(prequest->m_http_method, eHTTP_METHOD::GET_);
	}

	TEST(CGDK10_http_request, url_post_query_is_body)
	{
		auto prequest = make_object<net::http_request>();
		prequest->set_request("http://127.0.0.1:8080/form?a=1&b=2"sv, eHTTP_METHOD::POST_);

		const auto text = request_text(prequest);
		EXPECT_EQ(first_line(text), "POST /form HTTP/1.1");
		EXPECT_NE(text.find("Content-Type: application/x-www-form-urlencoded\r\n"), std::string::npos);
		EXPECT_NE(text.find("Content-Length: 7\r\n"), std::string::npos);
		EXPECT_EQ(text.substr(text.size() - 11), "\r\n\r\na=1&b=2");
	}

	TEST(CGDK10_http_request, url_host_and_path)
	{
		// - the default path
		{
			auto prequest = make_object<net::http_request>();
			prequest->set_request("http://127.0.0.1:8080"sv);
			EXPECT_EQ(first_line(request_text(prequest)), "GET / HTTP/1.1");
		}

		// - ipv6 : "Host: [::1]:8080"
		{
			auto prequest = make_object<net::http_request>();
			prequest->set_request("http://[::1]:8080/v6?q=1"sv);

			const auto text = request_text(prequest);
			EXPECT_EQ(first_line(text), "GET /v6?q=1 HTTP/1.1");
			EXPECT_NE(text.find("Host: [::1]:8080\r\n"), std::string::npos);
			EXPECT_EQ(prequest->m_host_name, "::1");
		}

		// - the default port of https
		{
			auto prequest = make_object<net::http_request>();
			prequest->set_request("https://127.0.0.1/secure"sv);
			EXPECT_NE(request_text(prequest).find("Host: 127.0.0.1:443\r\n"), std::string::npos);
			EXPECT_TRUE(prequest->m_is_crypt);
		}
	}

	TEST(CGDK10_http_request, url_invalid)
	{
		for (std::string_view url : { "http://127.0.0.1:99999/"sv, "http://127.0.0.1:80x/"sv, "http://[::1/"sv, "unknown://127.0.0.1/"sv, "http:///path"sv })
		{
			auto prequest = make_object<net::http_request>();
			EXPECT_ANY_THROW(prequest->set_request(url)) << url;
		}
	}

	TEST(CGDK10_http_request, websocket_key_is_random)
	{
		auto prequest_1 = make_object<net::http_request>();
		auto prequest_2 = make_object<net::http_request>();
		prequest_1->set_request("ws://127.0.0.1:8080/ws"sv);
		prequest_2->set_request("ws://127.0.0.1:8080/ws"sv);

		const std::string key_1(prequest_1->m_keys.c_str(), prequest_1->m_keys.size());
		const std::string key_2(prequest_2->m_keys.c_str(), prequest_2->m_keys.size());
		EXPECT_EQ(key_1.size(), 24u);
		EXPECT_EQ(key_2.size(), 24u);
		EXPECT_NE(key_1, key_2);

		const auto text = request_text(prequest_1);
		EXPECT_NE(text.find("Sec-WebSocket-Key: " + key_1 + "\r\n"), std::string::npos);
		EXPECT_NE(text.find("Connection: Upgrade\r\n"), std::string::npos);
		EXPECT_EQ(text.find("Connection: close"), std::string::npos);
	}

	TEST(CGDK10_http_request, content_length_by_method)
	{
		const std::vector<std::string_view> headers;
		const std::vector<std::string_view> accept;

		// - HEAD / DELETE without a body : no Content-Length
		for (auto method : { eHTTP_METHOD::HEAD_, eHTTP_METHOD::DELETE_ })
		{
			auto prequest = make_object<net::http_request>();
			prequest->set_request(method, "127.0.0.1"sv, 8080, "HTTP/1.1"sv, "/x"sv, ""sv, headers, buffer_view(), accept, false);
			EXPECT_EQ(request_text(prequest).find("Content-Length"), std::string::npos);
			EXPECT_EQ(prequest->m_http_method, method);
		}

		// - POST without a body : "Content-Length: 0"
		auto prequest = make_object<net::http_request>();
		prequest->set_request(eHTTP_METHOD::POST_, "127.0.0.1"sv, 8080, "HTTP/1.1"sv, "/x"sv, ""sv, headers, buffer_view(), accept, false);
		EXPECT_NE(request_text(prequest).find("Content-Length: 0\r\n"), std::string::npos);
	}
}
