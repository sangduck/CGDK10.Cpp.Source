#include "pch.h"
#include <atomic>
#include <memory>
#include <thread>

//-----------------------------------------------------------------------------
// http client : response timeout (net::io::Nhttp_client::set_response_timeout)
//
//  - the server accepts the request but does not respond
//  - the client closes the connection after the response timeout and completes the request with '504 Gateway Timeout'
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - the port of the test (30400 + pid % 500 : not the port of ssl_loopback)
	uint16_t http_timeout_test_port()
	{
		return static_cast<uint16_t>(30400 + api::get_process_id() % 500);
	}

	struct sRESULT
	{
		std::atomic<bool>	is_completed{ false };
		std::atomic<int>	status_code{ -1 };
	};
}

// - receives the request and does not respond
class silent_http_server_socket : public net::socket::http_server
{
protected:
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/) override { return eRESULT::DONE; }
};

TEST(http_client, response_timeout)
{
	// 1) server (no response)
	auto pacceptor = make_own<net::acceptor<silent_http_server_socket>>();
	const auto port = http_timeout_test_port();
	net::io::connective::acceptor::Ntcp_ssl::START_PARAMETER parameter;		// - no ssl (pssl_context nullptr)
	parameter.local_endpoint = net::ip::tcp::endpoint(net::ip::address_v4::loopback(), port);
	pacceptor->start(parameter);

	// 2) request
	auto presult = std::make_shared<sRESULT>();
	auto prequest = make_object<net::http_request>();
	prequest->set_request("http://127.0.0.1:" + std::to_string(port) + "/slow");
	prequest->set_function([presult](sMESSAGE_HTTP_RESPONSE& _response)
	{
		if (_response.is_completed)
		{
			presult->status_code = static_cast<int>(_response.status_code);
			presult->is_completed = true;
		}
	});

	auto psocket = make_object<net::socket::http_client>();
	psocket->set_response_timeout(2s);
	const auto time_start = std::chrono::steady_clock::now();
	ASSERT_TRUE(psocket->connect(prequest));

	// 3) wait (2s + the period of the check)
	for (int i = 0; i < 300 && presult->is_completed == false; ++i)
		std::this_thread::sleep_for(50ms);
	const auto elapsed = std::chrono::steady_clock::now() - time_start;

	EXPECT_TRUE(presult->is_completed);
	EXPECT_EQ(presult->status_code, static_cast<int>(eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_));
	EXPECT_GE(elapsed, 1500ms);
	EXPECT_LT(elapsed, 12s);

	// 4) close
	pacceptor->stop();
}


TEST(http_client, connect_timeout)
{
	// - 192.0.2.1 (TEST-NET-1, RFC 5737) : not routed. SYN is not answered (OS default : windows about 21s, linux about 2 minutes)
	auto presult = std::make_shared<sRESULT>();
	auto prequest = make_object<net::http_request>();
	prequest->set_request("http://192.0.2.1:80/");
	prequest->set_function([presult](sMESSAGE_HTTP_RESPONSE& _response)
	{
		if (_response.is_completed)
		{
			presult->status_code = static_cast<int>(_response.status_code);
			presult->is_completed = true;
		}
	});

	auto psocket = make_object<net::socket::http_client>();
	psocket->set_connect_timeout(2s);
	const auto time_start = std::chrono::steady_clock::now();
	if (psocket->connect(prequest) == false)
		GTEST_SKIP() << "connect request failed (no route)";

	for (int i = 0; i < 300 && presult->is_completed == false; ++i)
		std::this_thread::sleep_for(50ms);
	const auto elapsed = std::chrono::steady_clock::now() - time_start;

	// check) failed at once (no route / unreachable) : the timeout is not tested
	if (presult->is_completed && presult->status_code != static_cast<int>(eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_) && elapsed < 1500ms)
		GTEST_SKIP() << "the connection failed at once (status " << presult->status_code << ")";

	EXPECT_TRUE(presult->is_completed);
	EXPECT_EQ(presult->status_code, static_cast<int>(eHTTP_STATUS_CODE::GATEWAY_TIMEOUT_));
	EXPECT_GE(elapsed, 1500ms);
	EXPECT_LT(elapsed, 10s);
}



//-----------------------------------------------------------------------------
// http client : responses (e2e)
//
//  - the server sends the raw response of the test (and closes the connection if is_close)
//-----------------------------------------------------------------------------
namespace
{
	// - the port of the test (30900 + pid % 500)
	uint16_t http_response_test_port()
	{
		return static_cast<uint16_t>(30900 + api::get_process_id() % 500);
	}

	struct sRAW_RESPONSE
	{
		std::mutex			lockable;
		std::string			response;
		bool				is_close = false;
		std::string			request_head;		// - the head of the last request
	};

	sRAW_RESPONSE& raw_response()
	{
		static sRAW_RESPONSE value;
		return value;
	}

	struct sRESPONSE_RESULT
	{
		std::atomic<bool>	is_completed{ false };
		std::atomic<int>	status_code{ -1 };
		std::atomic<int>	count_completed{ 0 };
		std::mutex			lockable;
		std::string			body;
	};

	// - a request to the test server (the result is set by the response function)
	object_ptr<net::http_request> make_test_request(const std::shared_ptr<sRESPONSE_RESULT>& _presult)
	{
		auto prequest = make_object<net::http_request>();
		prequest->set_function([_presult](sMESSAGE_HTTP_RESPONSE& _response)
		{
			if (_response.is_completed == false)
				return;

			std::string body;
			for (auto& iter : _response.contents)
				body.append(reinterpret_cast<const char*>(iter.data()), iter.size());
			{
				std::lock_guard lock(_presult->lockable);
				_presult->body = std::move(body);
			}
			_presult->status_code = static_cast<int>(_response.status_code);
			++_presult->count_completed;
			_presult->is_completed = true;
		});
		return prequest;
	}

	bool wait_completed(const std::shared_ptr<sRESPONSE_RESULT>& _presult, std::chrono::milliseconds _timeout)
	{
		const auto time_end = std::chrono::steady_clock::now() + _timeout;
		while (_presult->is_completed == false && std::chrono::steady_clock::now() < time_end)
			std::this_thread::sleep_for(10ms);
		return _presult->is_completed;
	}
}

// - sends the raw response of the test
class raw_http_server_socket : public net::socket::http_server
{
protected:
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& _msg) override
	{
		std::string response;
		bool is_close = false;
		{
			auto& raw = raw_response();
			std::lock_guard lock(raw.lockable);
			raw.request_head.assign(reinterpret_cast<const char*>(_msg.buf_head.data()), _msg.buf_head.size());
			response = raw.response;
			is_close = raw.is_close;
		}

		auto buf = alloc_shared_buffer(response.size() + 1);
		buf.append(response.size(), response.data());
		this->process_send(std::move(buf), 1, 0);

		if (is_close)
			this->disconnect();

		return eRESULT::DONE;
	}
};

class http_client_response : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		s_pacceptor = make_own<net::acceptor<raw_http_server_socket>>();
		net::io::connective::acceptor::Ntcp_ssl::START_PARAMETER parameter;		// - no ssl
		parameter.local_endpoint = net::ip::tcp::endpoint(net::ip::address_v4::loopback(), http_response_test_port());
		s_pacceptor->start(parameter);
	}
	static void TearDownTestSuite()
	{
		s_pacceptor->stop();
		s_pacceptor.reset();
	}

	static void set_response(std::string _response, bool _is_close)
	{
		auto& raw = raw_response();
		std::lock_guard lock(raw.lockable);
		raw.response = std::move(_response);
		raw.is_close = _is_close;
		raw.request_head.clear();
	}
	static std::string request_head()
	{
		auto& raw = raw_response();
		std::lock_guard lock(raw.lockable);
		return raw.request_head;
	}
	static std::string url(std::string_view _path)
	{
		return "http://127.0.0.1:" + std::to_string(http_response_test_port()) + std::string(_path);
	}

	static inline own_ptr<net::acceptor<raw_http_server_socket>> s_pacceptor;
};

// 1) no Content-Length : the body until the connection is closed
TEST_F(http_client_response, body_until_close)
{
	set_response("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nhello until close", true);

	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	prequest->set_request(url("/until_close"));
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);

	ASSERT_TRUE(wait_completed(presult, 10s));
	EXPECT_EQ(presult->status_code, 200);
	EXPECT_EQ(presult->body, "hello until close");
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(presult->count_completed, 1);
}

// 2) HEAD : no body even with Content-Length (completed without waiting for the body)
TEST_F(http_client_response, head_has_no_body)
{
	set_response("HTTP/1.1 200 OK\r\nContent-Length: 1000\r\n\r\n", false);

	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	const std::vector<std::string_view> headers;
	const std::vector<std::string_view> accepts;
	prequest->set_request(eHTTP_METHOD::HEAD_, "127.0.0.1"sv, http_response_test_port(), "HTTP/1.1"sv, "/head"sv, std::string_view(), headers, buffer_view(), accepts, false);
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);

	ASSERT_TRUE(wait_completed(presult, 5s));
	EXPECT_EQ(presult->status_code, 200);
	EXPECT_TRUE(presult->body.empty());
	EXPECT_EQ(request_head().substr(0, 11), "HEAD /head ");
}

// 3) 204 / "Content-Length: 0" / 1xx before the response
TEST_F(http_client_response, no_content_and_interim)
{
	const std::vector<std::tuple<std::string, int, std::string>> cases{
		{ "HTTP/1.1 204 No Content\r\n\r\n", 204, "" },
		{ "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n", 200, "" },
		{ "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok", 200, "ok" },
		{ "HTTP/1.1 103 Early Hints\r\nLink: </a.css>\r\n\r\nHTTP/1.1 201 Created\r\nContent-Length: 3\r\n\r\nnew", 201, "new" } };

	for (auto& [response, status, body] : cases)
	{
		set_response(response, false);

		auto presult = std::make_shared<sRESPONSE_RESULT>();
		auto prequest = make_test_request(presult);
		prequest->set_request(url("/status"));
		ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);

		ASSERT_TRUE(wait_completed(presult, 5s)) << response;
		EXPECT_EQ(presult->status_code, status) << response;
		std::lock_guard lock(presult->lockable);
		EXPECT_EQ(presult->body, body) << response;
	}
}

// 4) wait() : the completion before wait() is not missed
TEST_F(http_client_response, wait_after_completion)
{
	set_response("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ndone", false);

	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	prequest->set_request(url("/wait"));
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);
	ASSERT_TRUE(wait_completed(presult, 5s));

	// - completed already : returns at once
	EXPECT_TRUE(prequest->is_completed());
	EXPECT_TRUE(prequest->wait_for(1s));
	prequest->wait();

	// - requested again : completed by the response
	presult->is_completed = false;
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);
	EXPECT_TRUE(prequest->wait_for(5s));
	EXPECT_EQ(presult->status_code, 200);
}

// 5) the url of GET : the query is in the path, the fragment isn't sent
TEST_F(http_client_response, get_query_in_path)
{
	set_response("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok", false);

	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	prequest->set_request(url("/api/items?x=1&y=two#fragment"));
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);
	ASSERT_TRUE(wait_completed(presult, 5s));

	const auto head = request_head();
	EXPECT_EQ(head.substr(0, head.find("\r\n")), "GET /api/items?x=1&y=two HTTP/1.1");
	EXPECT_EQ(head.find("Content-Length"), std::string::npos);
	EXPECT_NE(head.find("Connection: close\r\n"), std::string::npos);
}

// 6) the connection closed before the whole response : '502 Bad Gateway' (once)
TEST_F(http_client_response, closed_before_response)
{
	set_response("HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\nshort", true);

	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	prequest->set_request(url("/short"));
	ASSERT_EQ(prequest->request(), eRESULT::SUCCESS);

	ASSERT_TRUE(wait_completed(presult, 10s));
	EXPECT_EQ(presult->status_code, static_cast<int>(eHTTP_STATUS_CODE::BAD_GATEWAY_));
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(presult->count_completed, 1);
}

// 7) fail to connect : '503 Service Unavailable' (not 404)
TEST(http_client, connect_refused)
{
	// - a port nobody listens (the test port + 1)
	auto presult = std::make_shared<sRESPONSE_RESULT>();
	auto prequest = make_test_request(presult);
	prequest->set_request("http://127.0.0.1:" + std::to_string(http_response_test_port() + 1) + "/");
	if (prequest->request() != eRESULT::SUCCESS)
	{
		EXPECT_TRUE(prequest->is_completed());		// - failed at once : completed (wait() returns)
		return;
	}

	ASSERT_TRUE(wait_completed(presult, 30s));
	EXPECT_EQ(presult->status_code, static_cast<int>(eHTTP_STATUS_CODE::SERVICE_UNAVAILABLE_));
}


//-----------------------------------------------------------------------------
// socket sustain : the common time limits (net::io::add_on::Nsocket_sustain::set_sustain_timeout)
//
//  - certify : not CERTIFIED (the first request is not finished) after the connect -> closed
//  - idle    : nothing received -> closed
//  - ping    : nothing received -> process_keepalive_ping(), nothing received for 2x -> closed
//  - a close by a time limit : DISCONNECT_REASON_SUSTAIN_TIMEOUT (the counts of sTIMEOUT_COUNT)
//-----------------------------------------------------------------------------
namespace
{
	struct sSUSTAIN_TEST
	{
		net::io::add_on::Nsocket_sustain::sTIMEOUT			timeout;
		net::io::add_on::Nsocket_sustain::sTIMEOUT_COUNT	count;
		std::atomic<int>		count_close{ 0 };
		std::atomic<uint64_t>	reason_close{ 0 };
		std::atomic<int>		count_ping{ 0 };

		void reset(const net::io::add_on::Nsocket_sustain::sTIMEOUT& _timeout)
		{
			this->timeout = _timeout;
			this->count.connecting = 0; this->count.certify = 0; this->count.idle = 0;
			this->count_close = 0; this->reason_close = 0; this->count_ping = 0;
		}
	};
	sSUSTAIN_TEST g_sustain_test;

	// - connects and sends nothing (raw tcp)
	struct raw_tcp_client
	{
		CGSOCKET handle = INVALID_SOCKET;

		bool connect(uint16_t _port)
		{
			this->handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (this->handle == INVALID_SOCKET)
				return false;
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(_port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			return ::connect(this->handle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
		}
		~raw_tcp_client()
		{
			if (this->handle != INVALID_SOCKET)
			{
			#if defined(_WIN32)
				::closesocket(this->handle);
			#else
				::close(this->handle);
			#endif
			}
		}
	};

	template <class TPREDICATE>
	bool wait_until(TPREDICATE&& _predicate, std::chrono::milliseconds _timeout)
	{
		const auto time_end = std::chrono::steady_clock::now() + _timeout;
		while (_predicate() == false)
		{
			if (std::chrono::steady_clock::now() > time_end)
				return false;
			std::this_thread::sleep_for(20ms);
		}
		return true;
	}

	uint64_t user_reason(uint64_t _reason) { return _reason & DISCONNECT_REASON_MASK_USER; }
}

// - the server socket with the common time limits (g_sustain_test)
//   (set in the constructor : the http server calls on_connect() after the first request.
//    a type for each test : the sockets are pooled (reused with the setting of the previous test))
template <int INDEX>
class sustain_http_server_socket : public net::socket::http_server, public net::io::add_on::Nsocket_sustain
{
public:
								sustain_http_server_socket() { this->set_sustain_timeout(g_sustain_test.timeout, &g_sustain_test.count); }
protected:
	virtual void				on_closesocket(uint64_t _disconnect_reason) override { g_sustain_test.reason_close = _disconnect_reason; ++g_sustain_test.count_close; }
	virtual bool				process_keepalive_ping() override { ++g_sustain_test.count_ping; return true; }
	virtual result_code			on_message(sMESSAGE_HTTP_REQUEST& /*_msg*/) override { return eRESULT::DONE; }
};

template <int INDEX>
static own_ptr<net::acceptor<sustain_http_server_socket<INDEX>>> start_sustain_server(uint16_t _port)
{
	auto pacceptor = make_own<net::acceptor<sustain_http_server_socket<INDEX>>>();
	net::io::connective::acceptor::Ntcp_ssl::START_PARAMETER parameter;		// - no ssl
	parameter.local_endpoint = net::ip::tcp::endpoint(net::ip::address_v4::loopback(), _port);
	pacceptor->start(parameter);
	return pacceptor;
}

TEST(socket_sustain, certify_timeout)
{
	net::io::add_on::Nsocket_sustain::sTIMEOUT timeout;
	timeout.certify = 1s;
	g_sustain_test.reset(timeout);

	const auto port = static_cast<uint16_t>(http_timeout_test_port() + 1);
	auto pacceptor = start_sustain_server<1>(port);

	// - connects and does not send the request
	raw_tcp_client client;
	ASSERT_TRUE(client.connect(port));

	ASSERT_TRUE(wait_until([] { return g_sustain_test.count_close.load() != 0; }, 10s));
	EXPECT_EQ(user_reason(g_sustain_test.reason_close), DISCONNECT_REASON_SUSTAIN_TIMEOUT);
	EXPECT_EQ(g_sustain_test.count.certify.load(), 1u);
	EXPECT_EQ(g_sustain_test.count.idle.load(), 0u);

	pacceptor->stop();
}

TEST(socket_sustain, idle_timeout)
{
	net::io::add_on::Nsocket_sustain::sTIMEOUT timeout;
	timeout.idle = 1s;
	g_sustain_test.reset(timeout);

	const auto port = static_cast<uint16_t>(http_timeout_test_port() + 2);
	auto pacceptor = start_sustain_server<2>(port);

	raw_tcp_client client;
	ASSERT_TRUE(client.connect(port));

	ASSERT_TRUE(wait_until([] { return g_sustain_test.count_close.load() != 0; }, 10s));
	EXPECT_EQ(user_reason(g_sustain_test.reason_close), DISCONNECT_REASON_SUSTAIN_TIMEOUT);
	EXPECT_EQ(g_sustain_test.count.idle.load(), 1u);
	EXPECT_EQ(g_sustain_test.count.certify.load(), 0u);

	pacceptor->stop();
}

TEST(socket_sustain, keepalive_ping)
{
	net::io::add_on::Nsocket_sustain::sTIMEOUT timeout;
	timeout.ping_interval = 1s;
	g_sustain_test.reset(timeout);

	const auto port = static_cast<uint16_t>(http_timeout_test_port() + 3);
	auto pacceptor = start_sustain_server<3>(port);

	raw_tcp_client client;
	ASSERT_TRUE(client.connect(port));

	// - PING first (nothing received for 1s), closed after 2s without receiving (no PONG)
	ASSERT_TRUE(wait_until([] { return g_sustain_test.count_close.load() != 0; }, 12s));
	EXPECT_GE(g_sustain_test.count_ping.load(), 1);
	EXPECT_EQ(user_reason(g_sustain_test.reason_close), DISCONNECT_REASON_SUSTAIN_TIMEOUT);
	EXPECT_EQ(g_sustain_test.count.idle.load(), 1u);

	pacceptor->stop();
}

TEST(socket_sustain, no_timeout)
{
	// - all 0 : not closed
	g_sustain_test.reset(net::io::add_on::Nsocket_sustain::sTIMEOUT{});

	const auto port = static_cast<uint16_t>(http_timeout_test_port() + 4);
	auto pacceptor = start_sustain_server<4>(port);

	raw_tcp_client client;
	ASSERT_TRUE(client.connect(port));

	EXPECT_FALSE(wait_until([] { return g_sustain_test.count_close.load() != 0; }, 3s));

	pacceptor->stop();
}

//-----------------------------------------------------------------------------
// disconnect reason : the flags are ORed, the user code (a number) is the first one
//-----------------------------------------------------------------------------
class reason_probe_socket : public net::socket::http_server
{
public:
	using net::io::Isocket::sDISCONNECT_REASON;
};

TEST(socket_sustain, disconnect_reason_code)
{
	reason_probe_socket::sDISCONNECT_REASON reason;

	reason |= DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_OVERFLOW_MAX_BYTES;
	reason |= DISCONNECT_REASON_ABORTIVE | DISCONNECT_REASON_SUSTAIN_TIMEOUT;

	const uint64_t value = reason;
	EXPECT_EQ(value & DISCONNECT_REASON_MASK_USER, DISCONNECT_REASON_OVERFLOW_MAX_BYTES);	// - not 0x17 (0x02 | 0x15)
	EXPECT_NE(value & DISCONNECT_REASON_ACTIVE, 0u);
	EXPECT_NE(value & DISCONNECT_REASON_ABORTIVE, 0u);

	// - the first code when there was none
	reason_probe_socket::sDISCONNECT_REASON reason_2;
	reason_2 |= DISCONNECT_REASON_FAIL;
	reason_2 |= DISCONNECT_REASON_SUSTAIN_TIMEOUT;
	EXPECT_EQ(static_cast<uint64_t>(reason_2) & DISCONNECT_REASON_MASK_USER, DISCONNECT_REASON_SUSTAIN_TIMEOUT);
	EXPECT_NE(static_cast<uint64_t>(reason_2) & DISCONNECT_REASON_FAIL, 0u);
}

}
