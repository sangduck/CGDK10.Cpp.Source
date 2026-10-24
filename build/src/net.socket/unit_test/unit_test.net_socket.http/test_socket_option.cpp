#include "pch.h"
#include <atomic>
#include <mutex>
#include <thread>

//-----------------------------------------------------------------------------
// tcp socket option (sSOCKET_OPTION_TCP : TCP_NODELAY, TCP_QUICKACK, SO_BUSY_POLL, TCP_NOTSENT_LOWAT, TCP Fast Open)
//
//  - every socket holds a socket_option() (Isocket). it is applied to the handle when the connect/accept completes
//    (apply_socket_option()). nothing but TCP_NODELAY (as before) is applied unless an option is set.
//  - the acceptor sets it to the accepted sockets ("socket_option" / START_PARAMETER::socket_option + flag_socket_option),
//    and turns TCP Fast Open of the listen socket on ("tcp_fastopen" / START_PARAMETER::tcp_fastopen).
//  - a client (connect_requestable) reads "socket_option" of its start context / START_PARAMETER, or socket_option() is set by code before start().
//  - the connector sets it to the sockets it allocates ("socket_option" of the start context) and reads the one of a connect context.
//
//  1. defaults    : nothing is on by default (the struct, the START_PARAMETERs, a socket)
//  2. parameters  : get_parameter_socket_option (every key, the string forms, "default", invalid values, aliases),
//                   "tcp_fastopen" / "socket_option" of the acceptor, the start context of a client
//  3. applied     : the options are on the handles of the accepted/connected sockets (getsockopt at on_connect), the listen socket (TCP_FASTOPEN),
//                   not applied without an option, set by code (on_request_accept, before start), the START_PARAMETER path, the context path, the connector
//  4. TCP Fast Open : a client with tcp_fastopen connects and the echo works (linux : the deferred connect of TCP_FASTOPEN_CONNECT)
//  5. UDS (linux) : the options are skipped for a UDS socket
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - the port of the test (32400 + pid % 500 : not the ports of the other tests)
	uint16_t option_test_port()
	{
		return static_cast<uint16_t>(32400 + api::get_process_id() % 500);
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

	// - an int socket option of a handle (-1 : failed). windows writes one byte for a bool option (TCP_NODELAY) : the value starts from 0
	int get_option_int(CGSOCKET _handle, int _level, int _name)
	{
		int value = 0;
		int length = sizeof(value);
		if (api::socket::get_sock_opt(_handle, _name, &value, &length, _level) == false)
			return -1;
		return value;
	}

	// - a message : [uint32 size (header included)][body]
	shared_buffer make_option_message(std::string_view _body)
	{
		auto buf = alloc_shared_buffer(sizeof(uint32_t) + _body.size());
		const uint32_t size = static_cast<uint32_t>(sizeof(uint32_t) + _body.size());
		buf.append(sizeof(size), reinterpret_cast<const char*>(&size));
		buf.append(_body.size(), _body.data());
		return buf;
	}

	// - what the server sockets saw at on_connect (the options are applied before on_connect)
	struct sSERVER_OBSERVED
	{
		std::atomic<int>	count_connected{ 0 };
		std::atomic<int>	count_message{ 0 };
		std::atomic<int>	nodelay{ -2 };			// - TCP_NODELAY of the accepted socket
		std::atomic<int>	notsent_lowat{ -2 };	// - TCP_NOTSENT_LOWAT (linux)
		std::atomic<int>	quickack{ -2 };			// - TCP_QUICKACK (linux)
		std::atomic<bool>	apply_result{ false };	// - apply_socket_option() called again : every option is accepted by the OS
		std::atomic<bool>	is_unix{ false };
		std::atomic<bool>	option_quickack{ false };
		std::atomic<bool>	option_nodelay{ true };
	};
	sSERVER_OBSERVED g_observed;

	void reset_observed()
	{
		g_observed.count_connected = 0;
		g_observed.count_message = 0;
		g_observed.nodelay = -2;
		g_observed.notsent_lowat = -2;
		g_observed.quickack = -2;
		g_observed.apply_result = false;
		g_observed.is_unix = false;
		g_observed.option_quickack = false;
		g_observed.option_nodelay = true;
	}
}

//-----------------------------------------------------------------------------
// sockets
//-----------------------------------------------------------------------------
// - the server socket : records the options of its handle at on_connect and echoes
class option_server_socket : public net::socket::tcp<>
{
protected:
	virtual void				on_connect() override
	{
		const auto handle = this->native_handle();

		g_observed.nodelay = get_option_int(handle, IPPROTO_TCP, TCP_NODELAY);
	#if defined(_SYS_EPOLL_H)
		g_observed.notsent_lowat = get_option_int(handle, IPPROTO_TCP, TCP_NOTSENT_LOWAT);
		g_observed.quickack = get_option_int(handle, IPPROTO_TCP, TCP_QUICKACK);
	#endif
		g_observed.is_unix = this->remote_endpoint().is_unix();
		g_observed.option_quickack = this->socket_option().tcp_quickack;
		g_observed.option_nodelay = this->socket_option().tcp_nodelay;
		g_observed.apply_result = this->apply_socket_option();
		++g_observed.count_connected;
	}
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		++g_observed.count_message;
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

// - the client socket
template <class TCONNECT>
class option_client_socket_t : public net::socket::tcp<>, public TCONNECT
{
public:
			std::atomic<bool>	is_connected{ false };
			std::atomic<bool>	is_failed{ false };
			std::atomic<int>	count_connect{ 0 };
			std::atomic<int>	count_disconnect{ 0 };
			std::atomic<int>	nodelay{ -2 };			// - TCP_NODELAY of the handle at on_connect
			std::atomic<bool>	apply_result{ false };
			std::mutex			lockable;
			std::vector<std::string> received;

protected:
	virtual void				on_connect() override
	{
		this->nodelay = get_option_int(this->native_handle(), IPPROTO_TCP, TCP_NODELAY);
		this->apply_result = this->apply_socket_option();
		++this->count_connect;
		this->is_connected = true;
	}
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { this->is_failed = true; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; ++this->count_disconnect; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		std::lock_guard lock(this->lockable);
		this->received.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()) + sizeof(uint32_t), _msg.buf_message.size() - sizeof(uint32_t));
		return eRESULT::DONE;
	}
};

using option_client_socket = option_client_socket_t<net::io::connect_requestable::tcp::Ndefault>;

// - the acceptor : the parameter parser is exposed. the option of an accepted socket may be set by code :
//   on_request_accept (at the alloc : valid when the acceptor has no socket_option) or on_accept (right after the acceptor's socket_option is set to the socket)
class option_test_acceptor : public net::acceptor<option_server_socket>
{
public:
			std::atomic<bool>	set_nodelay_off_in_on_request_accept{ false };
			std::atomic<bool>	set_nodelay_off_in_on_accept{ false };

			void				parse(context& _context, START_PARAMETER& _parameter) { this->process_get_acceptor_parameters(_context, _parameter); }
	[[nodiscard]] CGSOCKET		listen_handle() const noexcept { return this->native_handle(); }

protected:
	virtual void				on_request_accept(net::io::Iconnectable* _pconnectable) override
	{
		if (this->set_nodelay_off_in_on_request_accept)
			_pconnectable->socket_option().tcp_nodelay = false;
	}
	virtual void				on_accept(net::io::Iconnectable* _pconnectable) override
	{
		if (this->set_nodelay_off_in_on_accept)
			_pconnectable->socket_option().tcp_nodelay = false;
	}
};

static option_test_acceptor::START_PARAMETER make_acceptor_parameter(uint16_t _port)
{
	option_test_acceptor::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _port };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	return parameter;
}

static sSOCKET_CONNECT_REQUEST_PARAMETER make_connect_parameter(uint16_t _port)
{
	sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
	parameter.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _port };
	parameter.reconnection_enable = 1;		// - disable
	parameter.enable_progress_log = false;
	return parameter;
}

// - connects and waits for the connect of both sides
template <class TSOCKET>
static bool connect_and_wait(TSOCKET* _psocket, const sSOCKET_CONNECT_REQUEST_PARAMETER& _parameter, int _count_server_connected)
{
	_psocket->start(_parameter);

	if (wait_until([&]() { return _psocket->is_connected.load() || _psocket->is_failed.load(); }) == false || _psocket->is_failed)
		return false;

	return wait_until([&]() { return g_observed.count_connected.load() >= _count_server_connected; });
}

// - sends messages of several sizes and checks the echo
template <class TSOCKET>
static void check_echo(TSOCKET* _psocket)
{
	const std::vector<std::size_t> sizes = { 4, 100, 1000, 16384, 70000 };

	for (std::size_t i = 0; i < sizes.size(); ++i)
	{
		std::string body(sizes[i], static_cast<char>('a' + i));
		ASSERT_TRUE(_psocket->send(make_option_message(body))) << "send " << i;
	}

	ASSERT_TRUE(wait_until([&]() { std::lock_guard lock(_psocket->lockable); return _psocket->received.size() >= sizes.size(); })) << "echo";

	std::lock_guard lock(_psocket->lockable);
	ASSERT_EQ(_psocket->received.size(), sizes.size());
	for (std::size_t i = 0; i < sizes.size(); ++i)
		EXPECT_EQ(_psocket->received[i], std::string(sizes[i], static_cast<char>('a' + i))) << "message " << i;
	_psocket->received.clear();
}


//-----------------------------------------------------------------------------
// 1. defaults
//-----------------------------------------------------------------------------
TEST(socket_option, defaults_are_off)
{
	// 1) the struct : nothing but TCP_NODELAY (as before)
	sSOCKET_OPTION_TCP option;
	EXPECT_EQ(option.tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
	EXPECT_FALSE(option.tcp_quickack);
	EXPECT_FALSE(option.tcp_fastopen);
	EXPECT_EQ(option.busy_poll, 0u);
	EXPECT_EQ(option.tcp_notsent_lowat, 0u);

	// 2) the START_PARAMETERs : not given
	sSOCKET_ACCEPT_PARAMETER accept_parameter;
	EXPECT_EQ(accept_parameter.tcp_fastopen, 0);
	EXPECT_EQ(accept_parameter.flag_socket_option, 0);

	sSOCKET_CONNECT_REQUEST_PARAMETER connect_parameter;
	EXPECT_EQ(connect_parameter.flag_socket_option, 0);

	// 3) a socket
	auto psocket = make_own<option_client_socket>();
	EXPECT_EQ(psocket->socket_option().tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
	EXPECT_FALSE(psocket->socket_option().tcp_quickack);
	EXPECT_FALSE(psocket->socket_option().tcp_fastopen);
	EXPECT_EQ(psocket->socket_option().busy_poll, 0u);
	EXPECT_EQ(psocket->socket_option().tcp_notsent_lowat, 0u);

	// 4) an acceptor
	auto pacceptor = make_own<option_test_acceptor>();
	EXPECT_EQ(pacceptor->tcp_fastopen(), 0);
	EXPECT_FALSE(pacceptor->socket_option().tcp_quickack);
}


//-----------------------------------------------------------------------------
// 2. parameters
//-----------------------------------------------------------------------------
TEST(socket_option, parse_socket_option)
{
	// 1) no "socket_option" : false, untouched
	{
		context c;
		c["remote_endpoint"]["port"] = 1;

		sSOCKET_OPTION_TCP option;
		option.tcp_quickack = true;
		EXPECT_FALSE(get_parameter_socket_option(c, option));
		EXPECT_TRUE(option.tcp_quickack);
	}

	// 2) every key
	{
		context c;
		c["socket_option"]["tcp_nodelay"] = false;
		c["socket_option"]["tcp_quickack"] = true;
		c["socket_option"]["tcp_fastopen"] = true;
		c["socket_option"]["busy_poll"] = 50;
		c["socket_option"]["tcp_notsent_lowat"] = 65536;

		sSOCKET_OPTION_TCP option;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_FALSE(option.tcp_nodelay);
		EXPECT_TRUE(option.tcp_quickack);
		EXPECT_TRUE(option.tcp_fastopen);
		EXPECT_EQ(option.busy_poll, 50u);
		EXPECT_EQ(option.tcp_notsent_lowat, 65536u);
	}

	// 3) the string forms : "enable"/"disable"/"1"/"0", "64k"/"1m"
	{
		context c;
		c["socket_option"]["tcp_nodelay"] = "0";
		c["socket_option"]["tcp_quickack"] = "enable";
		c["socket_option"]["tcp_fastopen"] = "disable";
		c["socket_option"]["busy_poll"] = "1m";
		c["socket_option"]["tcp_notsent_lowat"] = "64k";

		sSOCKET_OPTION_TCP option;
		option.tcp_fastopen = true;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_FALSE(option.tcp_nodelay);
		EXPECT_TRUE(option.tcp_quickack);
		EXPECT_FALSE(option.tcp_fastopen);
		EXPECT_EQ(option.busy_poll, 1024u * 1024u);
		EXPECT_EQ(option.tcp_notsent_lowat, 65536u);
	}

	// 4) "default" : the default of the key
	{
		context c;
		c["socket_option"]["tcp_nodelay"] = "default";
		c["socket_option"]["tcp_quickack"] = "default";
		c["socket_option"]["busy_poll"] = "default";
		c["socket_option"]["tcp_notsent_lowat"] = "default";

		sSOCKET_OPTION_TCP option;
		option.tcp_nodelay = !DEFAULT_ENABLE_TCP_NODELAY;
		option.tcp_quickack = true;
		option.busy_poll = 7;
		option.tcp_notsent_lowat = 7;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_EQ(option.tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
		EXPECT_FALSE(option.tcp_quickack);
		EXPECT_EQ(option.busy_poll, 0u);
		EXPECT_EQ(option.tcp_notsent_lowat, 0u);
	}

	// 5) the keys not given keep their value
	{
		context c;
		c["socket_option"]["tcp_quickack"] = true;

		sSOCKET_OPTION_TCP option;
		option.tcp_nodelay = false;
		option.busy_poll = 20;
		option.tcp_notsent_lowat = 30;
		option.tcp_fastopen = true;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_TRUE(option.tcp_quickack);
		EXPECT_FALSE(option.tcp_nodelay);
		EXPECT_EQ(option.busy_poll, 20u);
		EXPECT_EQ(option.tcp_notsent_lowat, 30u);
		EXPECT_TRUE(option.tcp_fastopen);
	}

	// 6) invalid values are ignored
	{
		context c;
		c["socket_option"]["tcp_quickack"] = "maybe";
		c["socket_option"]["tcp_notsent_lowat"] = "-5";
		c["socket_option"]["busy_poll"] = "12x";

		sSOCKET_OPTION_TCP option;
		option.tcp_quickack = true;
		option.tcp_notsent_lowat = 1;
		option.busy_poll = 2;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_TRUE(option.tcp_quickack);
		EXPECT_EQ(option.tcp_notsent_lowat, 1u);
		EXPECT_EQ(option.busy_poll, 2u);
	}

	// 7) aliases
	{
		context c;
		c["socket option"]["nodelay"] = false;
		c["socket option"]["quickack"] = true;
		c["socket option"]["fastopen"] = true;
		c["socket option"]["so_busy_poll"] = 5;
		c["socket option"]["notsent_lowat"] = "2kb";

		sSOCKET_OPTION_TCP option;
		EXPECT_TRUE(get_parameter_socket_option(c, option));
		EXPECT_FALSE(option.tcp_nodelay);
		EXPECT_TRUE(option.tcp_quickack);
		EXPECT_TRUE(option.tcp_fastopen);
		EXPECT_EQ(option.busy_poll, 5u);
		EXPECT_EQ(option.tcp_notsent_lowat, 2048u);
	}
}

TEST(socket_option, parse_acceptor_parameters)
{
	auto pacceptor = make_own<option_test_acceptor>();

	// 1) "tcp_fastopen": true, "socket_option"
	{
		context c;
		c["local_endpoint"]["port"] = 20000;
		c["tcp_fastopen"] = true;
		c["socket_option"]["tcp_nodelay"] = false;
		c["socket_option"]["tcp_quickack"] = true;
		c["socket_option"]["tcp_notsent_lowat"] = "64k";

		option_test_acceptor::START_PARAMETER parameter;
		parameter.enable_progress_log = false;
		pacceptor->parse(c, parameter);
		EXPECT_EQ(parameter.tcp_fastopen, DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH);
		EXPECT_EQ(parameter.flag_socket_option, 1);
		EXPECT_FALSE(parameter.socket_option.tcp_nodelay);
		EXPECT_TRUE(parameter.socket_option.tcp_quickack);
		EXPECT_EQ(parameter.socket_option.tcp_notsent_lowat, 65536u);
		EXPECT_FALSE(parameter.socket_option.tcp_fastopen);
	}

	// 2) "tcp_fastopen": a queue length / "false"
	{
		context c;
		c["local_endpoint"]["port"] = 20000;
		c["tcp_fastopen"] = 64;

		option_test_acceptor::START_PARAMETER parameter;
		parameter.enable_progress_log = false;
		pacceptor->parse(c, parameter);
		EXPECT_EQ(parameter.tcp_fastopen, 64);

		c["tcp_fastopen"] = "false";
		parameter.tcp_fastopen = 1;
		pacceptor->parse(c, parameter);
		EXPECT_EQ(parameter.tcp_fastopen, 0);
	}

	// 3) nothing : off
	{
		context c;
		c["local_endpoint"]["port"] = 20000;

		option_test_acceptor::START_PARAMETER parameter;
		parameter.enable_progress_log = false;
		pacceptor->parse(c, parameter);
		EXPECT_EQ(parameter.tcp_fastopen, 0);
		EXPECT_EQ(parameter.flag_socket_option, 0);
		EXPECT_EQ(parameter.socket_option.tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
		EXPECT_FALSE(parameter.socket_option.tcp_quickack);
		EXPECT_EQ(parameter.socket_option.tcp_notsent_lowat, 0u);
	}
}

TEST(socket_option, parse_client_start_context)
{
	// - "socket_option" of the start context is read before the connect (a port without a listener : the connect fails, the option is set)
	auto psocket = make_own<option_client_socket>();

	context c;
	c["remote_endpoint"]["address"] = "127.0.0.1";
	c["remote_endpoint"]["port"] = static_cast<int>(option_test_port() + 9);
	c["reconnection"] = "false";
	c["progress_log"] = "false";
	c["socket_option"]["tcp_nodelay"] = false;
	c["socket_option"]["tcp_quickack"] = true;
	c["socket_option"]["tcp_fastopen"] = true;

	try { psocket->start(c); } catch (...) {}

	EXPECT_FALSE(psocket->socket_option().tcp_nodelay);
	EXPECT_TRUE(psocket->socket_option().tcp_quickack);
	EXPECT_TRUE(psocket->socket_option().tcp_fastopen);

	wait_until([&]() { return psocket->is_failed.load() || psocket->is_connected.load(); }, 3000ms);
	psocket->stop();
}


//-----------------------------------------------------------------------------
// 3. applied
//-----------------------------------------------------------------------------
TEST(socket_option, applied_to_accepted_and_connected_sockets)
{
	reset_observed();

	// 1) the acceptor : tcp_fastopen, socket_option (tcp_nodelay off, tcp_quickack, tcp_notsent_lowat) by START_PARAMETER
	auto pacceptor = make_own<option_test_acceptor>();
	auto parameter = make_acceptor_parameter(option_test_port());
	parameter.tcp_fastopen = DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH;
	parameter.socket_option.tcp_nodelay = false;
	parameter.socket_option.tcp_quickack = true;
	parameter.socket_option.tcp_notsent_lowat = 65536;
	parameter.flag_socket_option = 1;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	EXPECT_EQ(pacceptor->tcp_fastopen(), DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH);
	EXPECT_TRUE(pacceptor->socket_option().tcp_quickack);

	// - the listen socket : TCP_FASTOPEN
#if defined(_SYS_EPOLL_H)
	EXPECT_EQ(get_option_int(pacceptor->listen_handle(), IPPROTO_TCP, TCP_FASTOPEN), DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH);
#elif defined(TCP_FASTOPEN)
	if (const auto value = get_option_int(pacceptor->listen_handle(), IPPROTO_TCP, TCP_FASTOPEN); value >= 0)
		EXPECT_EQ(value, 1);
#endif

	// 2) the client : socket_option by code before the start (tcp_nodelay off, tcp_quickack)
	auto psocket = make_own<option_client_socket>();
	psocket->socket_option().tcp_nodelay = false;
	psocket->socket_option().tcp_quickack = true;
	ASSERT_TRUE(connect_and_wait(psocket.get(), make_connect_parameter(port), 1));

	// 3) the handles (read at on_connect)
	EXPECT_EQ(g_observed.nodelay.load(), 0);
	EXPECT_TRUE(g_observed.apply_result.load());
	EXPECT_TRUE(g_observed.option_quickack.load());
	EXPECT_EQ(psocket->nodelay.load(), 0);
	EXPECT_TRUE(psocket->apply_result.load());
#if defined(_SYS_EPOLL_H)
	EXPECT_EQ(g_observed.notsent_lowat.load(), 65536);
	EXPECT_EQ(g_observed.quickack.load(), 1);
#endif

	// 4) the echo works with the options
	check_echo(psocket.get());
	EXPECT_TRUE(wait_until([&]() { return g_observed.count_message.load() >= 5; }));

	psocket->stop();
	pacceptor->stop();
}

TEST(socket_option, not_applied_without_option)
{
	reset_observed();

	// 1) the acceptor and the client without an option : TCP_NODELAY on (as before), nothing else
	auto pacceptor = make_own<option_test_acceptor>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter(option_test_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	EXPECT_EQ(pacceptor->tcp_fastopen(), 0);
#if defined(_SYS_EPOLL_H)
	EXPECT_EQ(get_option_int(pacceptor->listen_handle(), IPPROTO_TCP, TCP_FASTOPEN), 0);
#endif

	auto psocket = make_own<option_client_socket>();
	ASSERT_TRUE(connect_and_wait(psocket.get(), make_connect_parameter(port), 1));

	EXPECT_EQ(g_observed.nodelay.load(), DEFAULT_ENABLE_TCP_NODELAY ? 1 : 0);
	EXPECT_TRUE(g_observed.option_nodelay.load());
	EXPECT_FALSE(g_observed.option_quickack.load());
	EXPECT_TRUE(g_observed.apply_result.load());
	EXPECT_EQ(psocket->nodelay.load(), DEFAULT_ENABLE_TCP_NODELAY ? 1 : 0);
	EXPECT_TRUE(psocket->apply_result.load());
#if defined(_SYS_EPOLL_H)
	EXPECT_EQ(g_observed.notsent_lowat.load(), 0);
#endif

	check_echo(psocket.get());

	psocket->stop();
	pacceptor->stop();
}

TEST(socket_option, set_by_code)
{
	reset_observed();

	// 1) the acceptor without an option but on_request_accept sets the option of the accepted socket
	auto pacceptor = make_own<option_test_acceptor>();
	pacceptor->set_nodelay_off_in_on_request_accept = true;
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter(option_test_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) the client : socket_option() by code before the start
	auto psocket = make_own<option_client_socket>();
	psocket->socket_option().tcp_nodelay = false;
	ASSERT_TRUE(connect_and_wait(psocket.get(), make_connect_parameter(port), 1));

	EXPECT_EQ(g_observed.nodelay.load(), 0);
	EXPECT_FALSE(g_observed.option_nodelay.load());
	EXPECT_EQ(psocket->nodelay.load(), 0);

	// 3) the acceptor : socket_option() by code after the start (set to the sockets accepted after : right before on_accept, even to the accept sockets posted in advance)
	sSOCKET_OPTION_TCP option;
	option.tcp_nodelay = true;
	option.tcp_quickack = true;
	pacceptor->set_nodelay_off_in_on_request_accept = false;
	pacceptor->socket_option(option);

	reset_observed();
	auto psocket2 = make_own<option_client_socket>();
	ASSERT_TRUE(connect_and_wait(psocket2.get(), make_connect_parameter(port), 1));
	EXPECT_EQ(g_observed.nodelay.load(), 1);
	EXPECT_TRUE(g_observed.option_quickack.load());
	EXPECT_TRUE(g_observed.apply_result.load());

	// 4) on_accept changes the option of a socket after the acceptor's one is set to it
	pacceptor->set_nodelay_off_in_on_accept = true;

	reset_observed();
	auto psocket3 = make_own<option_client_socket>();
	ASSERT_TRUE(connect_and_wait(psocket3.get(), make_connect_parameter(port), 1));
	EXPECT_EQ(g_observed.nodelay.load(), 0);
	EXPECT_FALSE(g_observed.option_nodelay.load());
	EXPECT_TRUE(g_observed.option_quickack.load());

	psocket->stop();
	psocket2->stop();
	psocket3->stop();
	pacceptor->stop();
}

TEST(socket_option, start_parameter_of_client)
{
	reset_observed();

	auto pacceptor = make_own<option_test_acceptor>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter(option_test_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) START_PARAMETER::socket_option with flag_socket_option = 1 : applied
	{
		auto psocket = make_own<option_client_socket>();
		auto parameter = make_connect_parameter(port);
		parameter.socket_option.tcp_nodelay = false;
		parameter.socket_option.tcp_quickack = true;
		parameter.flag_socket_option = 1;
		ASSERT_TRUE(connect_and_wait(psocket.get(), parameter, 1));

		EXPECT_FALSE(psocket->socket_option().tcp_nodelay);
		EXPECT_TRUE(psocket->socket_option().tcp_quickack);
		EXPECT_EQ(psocket->nodelay.load(), 0);
		EXPECT_TRUE(psocket->apply_result.load());
		psocket->stop();
	}

	// 2) flag_socket_option = 0 : the socket keeps its own (the struct's socket_option is ignored)
	{
		reset_observed();
		auto psocket = make_own<option_client_socket>();
		auto parameter = make_connect_parameter(port);
		parameter.socket_option.tcp_nodelay = false;
		ASSERT_TRUE(connect_and_wait(psocket.get(), parameter, 1));

		EXPECT_EQ(psocket->socket_option().tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
		EXPECT_EQ(psocket->nodelay.load(), DEFAULT_ENABLE_TCP_NODELAY ? 1 : 0);
		psocket->stop();
	}

	pacceptor->stop();
}

TEST(socket_option, context_of_acceptor_and_client)
{
	reset_observed();

	// 1) the acceptor by context : "tcp_fastopen", "socket_option"
	auto pacceptor = make_own<option_test_acceptor>();
	{
		context c;
		c["local_endpoint"]["address"] = "127.0.0.1";
		c["local_endpoint"]["port"] = static_cast<int>(option_test_port());
		c["progress_log"] = "false";
		c["tcp_fastopen"] = true;
		c["socket_option"]["tcp_nodelay"] = false;
		c["socket_option"]["tcp_quickack"] = true;
		ASSERT_EQ(pacceptor->start(c), eRESULT::SUCCESS);
	}
	const auto port = pacceptor->local_endpoint().port();
	EXPECT_EQ(pacceptor->tcp_fastopen(), DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH);
	EXPECT_FALSE(pacceptor->socket_option().tcp_nodelay);

	// 2) the client by context : "socket_option"
	auto psocket = make_own<option_client_socket>();
	{
		context c;
		c["remote_endpoint"]["address"] = "127.0.0.1";
		c["remote_endpoint"]["port"] = static_cast<int>(port);
		c["reconnection"] = "false";
		c["progress_log"] = "false";
		c["socket_option"]["tcp_nodelay"] = false;
		psocket->start(c);
	}
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() || psocket->is_failed.load(); }));
	ASSERT_FALSE(psocket->is_failed.load());
	ASSERT_TRUE(wait_until([&]() { return g_observed.count_connected.load() >= 1; }));

	EXPECT_EQ(g_observed.nodelay.load(), 0);
	EXPECT_TRUE(g_observed.option_quickack.load());
	EXPECT_EQ(psocket->nodelay.load(), 0);

	check_echo(psocket.get());

	psocket->stop();
	pacceptor->stop();
}


TEST(socket_option, connector)
{
	reset_observed();

	auto pacceptor = make_own<option_test_acceptor>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter(option_test_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) the connector : "socket_option" of the start context => the sockets it allocates
	auto pconnector = make_own<net::connector<option_client_socket>>();
	{
		context c;
		c["progress_log"] = "false";
		c["socket_option"]["tcp_nodelay"] = false;
		c["socket_option"]["tcp_quickack"] = true;
		pconnector->start(c);
	}
	EXPECT_FALSE(pconnector->socket_option().tcp_nodelay);
	EXPECT_TRUE(pconnector->socket_option().tcp_quickack);

	// 2) a connect by an endpoint : the option of the connector
	auto pconnectable = pconnector->request_connect({ net::ip::address_v4::loopback(), port });
	auto psocket = dynamic_cast<option_client_socket*>(pconnectable.get());
	ASSERT_NE(psocket, nullptr);
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() || psocket->is_failed.load(); }));
	ASSERT_FALSE(psocket->is_failed.load());
	EXPECT_FALSE(psocket->socket_option().tcp_nodelay);
	EXPECT_TRUE(psocket->socket_option().tcp_quickack);
	EXPECT_EQ(psocket->nodelay.load(), 0);
	EXPECT_TRUE(psocket->apply_result.load());

	// 3) a connect by a context with its own "socket_option" : the keys given change the socket's one (the others are kept)
	{
		context c;
		c["remote_endpoint"]["address"] = "127.0.0.1";
		c["remote_endpoint"]["port"] = static_cast<int>(port);
		c["socket_option"]["tcp_nodelay"] = true;
		auto pconnectable2 = pconnector->request_connect(c);
		auto psocket2 = dynamic_cast<option_client_socket*>(pconnectable2.get());
		ASSERT_NE(psocket2, nullptr);
		ASSERT_TRUE(wait_until([&]() { return psocket2->is_connected.load() || psocket2->is_failed.load(); }));
		ASSERT_FALSE(psocket2->is_failed.load());
		EXPECT_TRUE(psocket2->socket_option().tcp_nodelay);
		EXPECT_TRUE(psocket2->socket_option().tcp_quickack);
		EXPECT_EQ(psocket2->nodelay.load(), 1);
	}

	// 4) a connector without an option : the sockets keep their own (nothing but TCP_NODELAY)
	{
		auto pconnector2 = make_own<net::connector<option_client_socket>>();
		pconnector2->start();
		auto pconnectable3 = pconnector2->request_connect({ net::ip::address_v4::loopback(), port });
		auto psocket3 = dynamic_cast<option_client_socket*>(pconnectable3.get());
		ASSERT_NE(psocket3, nullptr);
		ASSERT_TRUE(wait_until([&]() { return psocket3->is_connected.load() || psocket3->is_failed.load(); }));
		ASSERT_FALSE(psocket3->is_failed.load());
		EXPECT_EQ(psocket3->socket_option().tcp_nodelay, DEFAULT_ENABLE_TCP_NODELAY);
		EXPECT_FALSE(psocket3->socket_option().tcp_quickack);
		EXPECT_EQ(psocket3->nodelay.load(), DEFAULT_ENABLE_TCP_NODELAY ? 1 : 0);
		pconnector2->close_connectable_all();
		pconnector2->destroy();
	}

	EXPECT_TRUE(wait_until([&]() { return g_observed.count_connected.load() >= 3; }));

	pconnector->close_connectable_all();
	pconnector->destroy();
	pacceptor->stop();
}


//-----------------------------------------------------------------------------
// 4. TCP Fast Open of a client
//-----------------------------------------------------------------------------
TEST(socket_option, tcp_fastopen_connect)
{
	reset_observed();

	// 1) the acceptor with TCP Fast Open
	auto pacceptor = make_own<option_test_acceptor>();
	auto parameter = make_acceptor_parameter(option_test_port());
	parameter.tcp_fastopen = DEFAULT_TCP_FASTOPEN_QUEUE_LENGTH;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) clients with tcp_fastopen, one after another (the first connect requests the TFO cookie, the next ones may use it)
	//    - linux : TCP_FASTOPEN_CONNECT. the connect completes at once and the first send() sends the SYN
	//      (without a cookie the send() returns EINPROGRESS and the data waits for EPOLLOUT : the echo must work the same)
	for (int i = 0; i < 3; ++i)
	{
		reset_observed();

		auto psocket = make_own<option_client_socket>();
		psocket->socket_option().tcp_fastopen = true;
		psocket->start(make_connect_parameter(port));

		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() || psocket->is_failed.load(); })) << "connect " << i;
		ASSERT_FALSE(psocket->is_failed.load()) << "connect " << i;

		check_echo(psocket.get());
		EXPECT_TRUE(wait_until([&]() { return g_observed.count_connected.load() >= 1; })) << "server connect " << i;
		EXPECT_TRUE(wait_until([&]() { return g_observed.count_message.load() >= 5; })) << "server messages " << i;

		psocket->stop();
		EXPECT_TRUE(wait_until([&]() { return psocket->count_disconnect.load() >= 1 || psocket->is_connected.load() == false; })) << "disconnect " << i;
	}

	pacceptor->stop();
}


//-----------------------------------------------------------------------------
// 5. UDS (linux) : the options are skipped for a UDS socket
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)
TEST(socket_option, uds_socket_skips_the_options)
{
	reset_observed();

	// 1) the acceptor with enable_uds and socket_option
	auto pacceptor = make_own<option_test_acceptor>();
	auto parameter = make_acceptor_parameter(option_test_port());
	parameter.enable_uds = true;
	parameter.socket_option.tcp_nodelay = false;
	parameter.socket_option.tcp_quickack = true;
	parameter.socket_option.tcp_notsent_lowat = 65536;
	parameter.flag_socket_option = 1;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) a loopback connect : a UDS. the options are not applied (no TCP option on a UDS) and nothing fails
	auto psocket = make_own<option_client_socket>();
	psocket->socket_option().tcp_quickack = true;
	ASSERT_TRUE(connect_and_wait(psocket.get(), make_connect_parameter(port), 1));

	EXPECT_TRUE(g_observed.is_unix.load());
	EXPECT_TRUE(g_observed.apply_result.load());
	EXPECT_TRUE(psocket->apply_result.load());
	EXPECT_EQ(g_observed.nodelay.load(), -1);

	check_echo(psocket.get());

	psocket->stop();
	pacceptor->stop();
}
#endif

//-----------------------------------------------------------------------------
// the send queue limits of a socket from the pool : back to the ones of the socket object at each accept
//  (before : a connection that set maximum_depth_of_send_buffer() / _bytes() left them to the next connection the pooled object served.
//   the same kind as the socket option above : the acceptor and the connector call reset_send_queue_limit() at the allocation)
//-----------------------------------------------------------------------------
namespace
{
	struct sLIMIT_SEEN
	{
		const void*			psocket;
		std::size_t			depth;
		std::size_t			bytes;
	};
	std::mutex				g_limit_lockable;
	std::vector<sLIMIT_SEEN> g_limit_seen;
	std::atomic<int>		g_limit_count_connected{ 0 };

	constexpr std::size_t	LIMIT_DEPTH_OF_THE_OBJECT = 64;

	class limit_server_socket : public net::socket::tcp<>
	{
	public:
		limit_server_socket() { this->maximum_depth_of_send_buffer(LIMIT_DEPTH_OF_THE_OBJECT); }	// - the limit of the socket object

	protected:
		virtual void				on_connect() override
		{
			{
				std::lock_guard lock(g_limit_lockable);
				g_limit_seen.push_back(sLIMIT_SEEN{ this, this->maximum_depth_of_send_buffer(), this->maximum_depth_of_send_buffer_bytes() });
			}

			// - this connection sets its own limits
			this->maximum_depth_of_send_buffer(7);
			this->maximum_depth_of_send_buffer_bytes(300 * 1024);

			++g_limit_count_connected;
		}
	};
}

TEST(socket_option, send_queue_limit_of_a_pooled_socket)
{
	{
		std::lock_guard lock(g_limit_lockable);
		g_limit_seen.clear();
	}
	g_limit_count_connected = 0;

	auto pacceptor = make_own<net::acceptor<limit_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter(option_test_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// - a connection served by a socket object that served one before
	const auto count_reused = [&]()
	{
		std::lock_guard lock(g_limit_lockable);
		std::size_t count = 0;
		for (std::size_t i = 0; i < g_limit_seen.size(); ++i)
		{
			for (std::size_t j = 0; j < i; ++j)
			{
				if (g_limit_seen[j].psocket == g_limit_seen[i].psocket)
				{
					++count;
					break;
				}
			}
		}
		return count;
	};

	// 1) connections one after another till 3 of them were served by a reused socket object (the server socket goes back to the pool.
	//    the pool hands a returned object out again after a while : the accept sockets prepared in advance, the cache per thread)
	constexpr int max_connect = 300;
	int count_connect = 0;
	for (; count_connect < max_connect && count_reused() < 3; ++count_connect)
	{
		auto psocket = make_own<option_client_socket>();
		psocket->start(make_connect_parameter(port));
		ASSERT_TRUE(wait_until([&]() { return g_limit_count_connected.load() == count_connect + 1; })) << "connect " << count_connect;
		psocket->stop();
		ASSERT_TRUE(wait_until([&]() { return pacceptor->count() == 0; })) << "close " << count_connect;
	}

	EXPECT_GE(count_reused(), 3u) << "the socket objects were not reused in " << count_connect << " connections : the test checks little";

	// 2) every connection started with the limits of the socket object (the bytes : the one the first connection of an object got,
	//    the default of the accepted sockets on windows)
	{
		std::lock_guard lock(g_limit_lockable);
		ASSERT_EQ(g_limit_seen.size(), static_cast<std::size_t>(count_connect));

		for (std::size_t i = 0; i < g_limit_seen.size(); ++i)
		{
			EXPECT_EQ(g_limit_seen[i].depth, LIMIT_DEPTH_OF_THE_OBJECT) << "connection " << i;
			EXPECT_EQ(g_limit_seen[i].bytes, g_limit_seen[0].bytes) << "connection " << i;
		}
	}

	pacceptor->stop();
}

}
