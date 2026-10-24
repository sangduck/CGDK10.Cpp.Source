#include "pch.h"
#include <atomic>
#include <thread>

//-----------------------------------------------------------------------------
// connect_requestable : a failed connect
//
//  - a connect fails either at once (connect()/ConnectEx returns an error : ENETUNREACH, EADDRNOTAVAIL (no ephemeral port) ...)
//    or later (the completion : ECONNREFUSED of a port nobody listens on). both must end the same way :
//    on_fail_connect once, the socket CLOSED (a stop() does not wait), and Ntcp_reconnect keeps reconnecting.
//  - (linux) an immediate failure left the socket SYN without on_fail_connect : a stop() waited till the timeout of wait_complete_closed()
//    and Ntcp_reconnect did not schedule the reconnect.
//
//  1. immediate failure : the broadcast address (fails at once on linux. windows may fail at once or later : the same result either way)
//  2. failure later     : a loopback port nobody listens on
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - the port of the test (28400 + pid % 500 : not the ports of the other tests, below the linux ephemeral range 32768~ : a connecting socket never takes it as its local port)
	uint16_t connect_test_port()
	{
		return static_cast<uint16_t>(28400 + api::get_process_id() % 500);
	}

	template <class TCONDITION>
	bool wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
	{
		const auto time_end = std::chrono::steady_clock::now() + _duration;
		while (std::chrono::steady_clock::now() < time_end)
		{
			if (_condition())
				return true;
			std::this_thread::sleep_for(10ms);
		}
		return _condition();
	}

	sSOCKET_CONNECT_REQUEST_PARAMETER make_fail_connect_parameter(net::ip::tcp::endpoint _remote_endpoint, bool _reconnect)
	{
		sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
		parameter.remote_endpoint = _remote_endpoint;
		parameter.reconnection_enable = _reconnect ? 2 : 1;	// - 2 : enable, 1 : disable
		parameter.reconnection_interval = chrono::tick::milliseconds(100);
		parameter.enable_progress_log = false;
		return parameter;
	}

	// - the addresses that fail : at once / later
	net::ip::tcp::endpoint immediate_fail_endpoint()
	{
		return { net::ip::address_v4::broadcast(), connect_test_port() };
	}

	net::ip::tcp::endpoint refused_endpoint()
	{
		return { net::ip::address_v4::loopback(), connect_test_port() };
	}

	// - a refused connect completes in ~2s on windows (the SYN is retried after a RST) and at once on linux
	std::chrono::milliseconds refused_wait()
	{
		return 8000ms;
	}
}

template <class TCONNECT>
class fail_connect_socket_t : public net::socket::tcp<>, public TCONNECT
{
public:
			std::atomic<int>	count_connect{ 0 };
			std::atomic<int>	count_fail_connect{ 0 };
			std::atomic<int>	count_disconnect{ 0 };

protected:
	virtual void				on_connect() override { ++this->count_connect; }
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { ++this->count_fail_connect; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { ++this->count_disconnect; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

using fail_connect_socket = fail_connect_socket_t<net::io::connect_requestable::tcp::Ndefault>;
using fail_connect_reconnect_socket = fail_connect_socket_t<net::io::connect_requestable::Ntcp_reconnect>;

// - the server socket
class fail_connect_server_socket : public net::socket::tcp<>
{
protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

// - stop() returns at once (a socket left SYN waited till the timeout of wait_complete_closed())
template <class TSOCKET>
static std::chrono::milliseconds stop_duration(TSOCKET* _psocket)
{
	const auto time_begin = std::chrono::steady_clock::now();
	_psocket->stop();
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_begin);
}

// - a failed connect without reconnection : on_fail_connect once, CLOSED, no on_connect / on_disconnect
static void check_fail_connect(const net::ip::tcp::endpoint& _endpoint, int _count_loop, std::chrono::milliseconds _duration_wait)
{
	for (int i = 0; i < _count_loop; ++i)
	{
		auto psocket = make_own<fail_connect_socket>();
		psocket->start(make_fail_connect_parameter(_endpoint, false));

		ASSERT_TRUE(wait_until([&]() { return psocket->count_fail_connect.load() >= 1; }, _duration_wait)) << "on_fail_connect, loop " << i;
		ASSERT_TRUE(wait_until([&]() { return psocket->socket_state() == eSOCKET_STATE::CLOSED; }, 3000ms)) << "CLOSED, loop " << i;
		EXPECT_LT(stop_duration(psocket.get()), 3000ms) << "stop, loop " << i;
		EXPECT_EQ(psocket->count_fail_connect.load(), 1) << "loop " << i;
		EXPECT_EQ(psocket->count_connect.load(), 0) << "loop " << i;
		EXPECT_EQ(psocket->count_disconnect.load(), 0) << "loop " << i;
	}
}

// - a failed connect with reconnection : reconnects every interval (on_fail_connect each time) till stopped
static void check_reconnect_after_fail(const net::ip::tcp::endpoint& _endpoint, std::chrono::milliseconds _duration_wait)
{
	auto psocket = make_own<fail_connect_reconnect_socket>();
	psocket->start(make_fail_connect_parameter(_endpoint, true));

	ASSERT_TRUE(wait_until([&]() { return psocket->count_fail_connect.load() >= 3; }, _duration_wait)) << "reconnect (fail count " << psocket->count_fail_connect.load() << ")";
	EXPECT_EQ(psocket->count_connect.load(), 0);

	psocket->disable_reconnect();
	EXPECT_LT(stop_duration(psocket.get()), 3000ms);
	EXPECT_EQ(psocket->socket_state(), eSOCKET_STATE::CLOSED);
}


//-----------------------------------------------------------------------------
// 1. immediate failure
//-----------------------------------------------------------------------------
TEST(connect_requestable, immediate_failure_completes)
{
	check_fail_connect(immediate_fail_endpoint(), 20, 3000ms);
}

TEST(connect_requestable, reconnect_after_immediate_failure)
{
	check_reconnect_after_fail(immediate_fail_endpoint(), 5000ms);
}


//-----------------------------------------------------------------------------
// 2. failure later (refused)
//-----------------------------------------------------------------------------
TEST(connect_requestable, refused_completes)
{
	check_fail_connect(refused_endpoint(), 3, refused_wait());
}

TEST(connect_requestable, reconnect_after_refused)
{
	check_reconnect_after_fail(refused_endpoint(), refused_wait() * 3);
}

// - reconnected when the server starts after the failures
TEST(connect_requestable, reconnect_connects_when_the_server_starts)
{
	auto psocket = make_own<fail_connect_reconnect_socket>();
	psocket->start(make_fail_connect_parameter(refused_endpoint(), true));
	ASSERT_TRUE(wait_until([&]() { return psocket->count_fail_connect.load() >= 2; }, refused_wait() * 2));

	net::acceptor<fail_connect_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = refused_endpoint();
	parameter.reallocate_port_at_duplicate_port_error = false;
	parameter.enable_progress_log = false;
	auto pacceptor = make_own<net::acceptor<fail_connect_server_socket>>();

	// - retried (the port may still be held for a moment by a closing socket of an earlier test)
	const bool is_started = wait_until([&]()
	{
		try { return pacceptor->start(parameter) == eRESULT::SUCCESS; }
		catch (...) { return false; }
	}, 3000ms);
	ASSERT_TRUE(is_started);

	EXPECT_TRUE(wait_until([&]() { return psocket->count_connect.load() >= 1; }, refused_wait()));

	psocket->disable_reconnect();
	EXPECT_LT(stop_duration(psocket.get()), 3000ms);
	pacceptor->stop();
}

// - TCP Fast Open (linux : TCP_FASTOPEN_CONNECT) : connect() succeeds at once and on_connect() comes before the handshake,
//   so a refused connect ends with on_disconnect, not on_fail_connect. Ntcp_reconnect reconnects by that disconnect too.
//   (windows : ConnectEx with TCP_FASTOPEN => on_fail_connect as usual)
TEST(connect_requestable, reconnect_with_tcp_fastopen_refused)
{
	auto psocket = make_own<fail_connect_reconnect_socket>();
	psocket->socket_option().tcp_fastopen = true;
	psocket->start(make_fail_connect_parameter(refused_endpoint(), true));

	// - refused again and again : reconnected each time (on_fail_connect or on_disconnect)
	ASSERT_TRUE(wait_until([&]() { return psocket->count_fail_connect.load() + psocket->count_disconnect.load() >= 3; }, refused_wait() * 3))
		<< "fail " << psocket->count_fail_connect.load() << ", disconnect " << psocket->count_disconnect.load() << ", connect " << psocket->count_connect.load();

	// - the server starts : connected and stays
	net::acceptor<fail_connect_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = refused_endpoint();
	parameter.reallocate_port_at_duplicate_port_error = false;
	parameter.enable_progress_log = false;
	auto pacceptor = make_own<net::acceptor<fail_connect_server_socket>>();
	const bool is_started = wait_until([&]()
	{
		try { return pacceptor->start(parameter) == eRESULT::SUCCESS; }
		catch (...) { return false; }
	}, 3000ms);
	ASSERT_TRUE(is_started);

	// - the disconnect count stops growing (the last connect is a real one)
	int count_disconnect = -1;
	EXPECT_TRUE(wait_until([&]()
	{
		const int count_now = psocket->count_disconnect.load();
		const bool is_stable = (count_now == count_disconnect) && psocket->count_connect.load() > count_now;
		count_disconnect = count_now;
		std::this_thread::sleep_for(500ms);
		return is_stable;
	}, refused_wait() * 2));

	psocket->disable_reconnect();
	EXPECT_LT(stop_duration(psocket.get()), 3000ms);
	pacceptor->stop();
}


//-----------------------------------------------------------------------------
// 3. connector : many connects at once
//
//  - (linux) the connector attached the socket to epoll before connect() : a TCP socket not connected yet is reported as
//    EPOLLOUT | EPOLLHUP at once, so a worker taking the event before connect() completed it as a failure (a spurious on_fail_connect
//    and the ONESHOT event spent). connect() first, then attach (like connect_requestable)
//-----------------------------------------------------------------------------
namespace
{
	// - the counts of the connector test (its sockets are pooled : a count in a socket object outlives a test repeated)
	std::atomic<int> g_count_connector_connect{ 0 };
	std::atomic<int> g_count_connector_fail_connect{ 0 };
	std::atomic<int> g_count_connector_accepted{ 0 };
}

class connector_count_server_socket : public net::socket::tcp<>
{
protected:
	virtual void				on_connect() override { ++g_count_connector_accepted; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

class connector_count_socket : public net::socket::tcp<>, public net::io::connect_requestable::tcp::Ndefault
{
protected:
	virtual void				on_connect() override { ++g_count_connector_connect; }
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { ++g_count_connector_fail_connect; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

TEST(connect_requestable, connector_many_connects)
{
	g_count_connector_connect = 0;
	g_count_connector_fail_connect = 0;
	g_count_connector_accepted = 0;

	net::acceptor<connector_count_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(connect_test_port() + 1) };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	auto pacceptor = make_own<net::acceptor<connector_count_server_socket>>();
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const net::ip::tcp::endpoint endpoint{ net::ip::address_v4::loopback(), pacceptor->local_endpoint().port() };

	auto pconnector = make_own<net::connector<connector_count_socket>>();
	{
		context c;
		c["progress_log"] = "false";
		pconnector->start(c);
	}

	// - 8 threads x 250 connects at once
	const int COUNT_THREAD = 8;
	const int COUNT_CONNECT = 250;
	{
		std::vector<std::thread> threads;
		for (int t = 0; t < COUNT_THREAD; ++t)
			threads.emplace_back([&]() { for (int i = 0; i < COUNT_CONNECT; ++i) pconnector->request_connect(endpoint); });
		for (auto& thread : threads)
			thread.join();
	}

	const int total = COUNT_THREAD * COUNT_CONNECT;
	EXPECT_TRUE(wait_until([&]() { return g_count_connector_connect.load() + g_count_connector_fail_connect.load() >= total; }, 20000ms)) << "connected " << g_count_connector_connect.load() << ", failed " << g_count_connector_fail_connect.load() << ", accepted " << g_count_connector_accepted.load();
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(g_count_connector_fail_connect.load(), 0);
	EXPECT_EQ(g_count_connector_connect.load(), total);

	pconnector->close_connectable_all();
	pconnector->destroy();
	pacceptor->stop();
}

}
