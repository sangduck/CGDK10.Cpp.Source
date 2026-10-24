#include "pch.h"
#include <atomic>
#include <mutex>
#include <thread>

//-----------------------------------------------------------------------------
// UDS (Unix domain socket, linux) for the same host connections
//
//  - an acceptor with START_PARAMETER::enable_uds listens on the UDS of its port ("\0cgdk.tcp.<port>") too.
//    the sockets accepted from it are the same class (on_connect, on_message ... are the same)
//  - an acceptor with START_PARAMETER::uds_only listens on the UDS only. (the TCP port is bound, not listened)
//  - connect_requestable to a loopback address of the port connects through the UDS (TCP if no UDS listener)
//  - net::sockaddr::is_unix() tells a UDS socket. (the tests check the domain of the socket handle too : SO_DOMAIN)
//
//  1. listen         : enable_uds, uds_only, the UDS name, stop/restart, ignored cases, name conflicts, context parameters
//  2. connect        : loopback v4/v6, TCP fallback, the switch, bind endpoints, nothing listening, reconnect, the connector
//  3. communication  : echo, large data, many sockets, active/passive close, the addresses of a UDS socket
//  4. epoll handles  : close at connect (in on_connect / another thread, Ntcp/Ntcp_reconnect, server side),
//                      stop while connecting, many TCP connects without an explicit bind
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <arpa/inet.h>
#include <ifaddrs.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - [uint32 size][uint32 index][pattern...]
	std::string make_uds_message(uint32_t _index, std::size_t _size)
	{
		std::string message(_size, '\0');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		memcpy(message.data() + 4, &_index, 4);
		for (std::size_t i = 8; i < _size; ++i)
			message[i] = static_cast<char>('a' + (i + _index) % 26);
		return message;
	}

	// - the index of a message
	uint32_t get_uds_message_index(const char* _data)
	{
		uint32_t index = 0;
		memcpy(&index, _data + 4, 4);
		return index;
	}

	// - the server closes the socket when it receives this index
	constexpr uint32_t UDS_MESSAGE_CLOSE = 0xffffffffu;

	// - the port of the test (31400 + pid % 500 : not the ports of the other tests)
	uint16_t uds_test_port()
	{
		return static_cast<uint16_t>(31400 + api::get_process_id() % 500);
	}

	// - the domain of a socket handle (AF_UNIX / AF_INET / AF_INET6)
	int socket_domain(CGSOCKET _handle)
	{
		int domain = -1;
		socklen_t length = sizeof(domain);
		if (::getsockopt(_handle, SOL_SOCKET, SO_DOMAIN, &domain, &length) != 0)
			return -1;
		return domain;
	}

	// - the accepted sockets (server side)
	std::atomic<int>	g_count_accepted_unix{ 0 };
	std::atomic<int>	g_count_accepted_tcp{ 0 };
	std::atomic<int>	g_count_accepted_remote_loopback{ 0 };
	std::atomic<int>	g_count_accepted_remote_unix{ 0 };
	std::atomic<int>	g_count_accepted_local_unix{ 0 };
	std::atomic<int>	g_count_server_disconnected{ 0 };
	std::atomic<bool>	g_server_close_in_on_connect{ false };
	std::string			g_server_remote_string;
	std::mutex			g_lockable_server;

	void reset_accepted()
	{
		g_count_accepted_unix = 0;
		g_count_accepted_tcp = 0;
		g_count_accepted_remote_loopback = 0;
		g_count_accepted_remote_unix = 0;
		g_count_accepted_local_unix = 0;
		g_count_server_disconnected = 0;
		g_server_close_in_on_connect = false;
		std::lock_guard lock(g_lockable_server);
		g_server_remote_string.clear();
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

	// - the abstract UDS address of a port ("\0cgdk.tcp.<port>")
	void make_test_uds_address(uint16_t _port, ::sockaddr_un& _address, socklen_t& _length)
	{
		api::socket::make_uds_address(_port, _address, _length);
	}

	// - a plain UDS connect (blocking) to "\0cgdk.tcp.<port>" : 0 or errno
	int raw_uds_connect(uint16_t _port)
	{
		::sockaddr_un addr;
		socklen_t length = 0;
		make_test_uds_address(_port, addr, length);
		const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
		const int result = (::connect(fd, reinterpret_cast<const ::sockaddr*>(&addr), length) == 0) ? 0 : errno;
		::close(fd);
		return result;
	}

	// - a plain TCP connect (blocking) to 127.0.0.1:<port> : 0 or errno
	int raw_tcp_connect(uint16_t _port)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		::sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		const int result = (::connect(fd, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)) == 0) ? 0 : errno;
		::close(fd);
		return result;
	}

	// - a plain TCP bind (SO_REUSEADDR) to 0.0.0.0:<port> : 0 or errno
	int raw_tcp_bind(uint16_t _port)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		const int reuse = 1;
		::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
		::sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		const int result = (::bind(fd, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)) == 0) ? 0 : errno;
		::close(fd);
		return result;
	}

	// - a non loopback IPv4 address of this host (empty if none)
	std::string find_non_loopback_v4()
	{
		std::string result;
		::ifaddrs* plist = nullptr;
		if (::getifaddrs(&plist) != 0)
			return result;
		for (auto* p = plist; p != nullptr; p = p->ifa_next)
		{
			if (p->ifa_addr == nullptr || p->ifa_addr->sa_family != AF_INET)
				continue;
			const auto* paddr = reinterpret_cast<const ::sockaddr_in*>(p->ifa_addr);
			if ((ntohl(paddr->sin_addr.s_addr) >> 24) == 127)
				continue;
			char buffer[INET_ADDRSTRLEN]{};
			::inet_ntop(AF_INET, &paddr->sin_addr, buffer, sizeof(buffer));
			result = buffer;
			break;
		}
		::freeifaddrs(plist);
		return result;
	}

	// - a UDS listener not by an acceptor (takes the name "\0cgdk.tcp.<port>")
	class raw_uds_listener
	{
	public:
		explicit raw_uds_listener(uint16_t _port)
		{
			::sockaddr_un addr;
			socklen_t length = 0;
			make_test_uds_address(_port, addr, length);
			this->m_fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
			if (::bind(this->m_fd, reinterpret_cast<const ::sockaddr*>(&addr), length) != 0 || ::listen(this->m_fd, 16) != 0)
			{
				::close(this->m_fd);
				this->m_fd = -1;
			}
		}
		~raw_uds_listener() { if (this->m_fd >= 0) ::close(this->m_fd); }
		[[nodiscard]] bool is_listening() const noexcept { return this->m_fd >= 0; }
	private:
		int m_fd = -1;
	};
}

//-----------------------------------------------------------------------------
// sockets
//-----------------------------------------------------------------------------
class uds_echo_server_socket : public net::socket::tcp<>
{
protected:
	virtual void				on_connect() override
	{
		if (socket_domain(this->native_handle()) == AF_UNIX)
			++g_count_accepted_unix;
		else
			++g_count_accepted_tcp;

		const auto remote = this->remote_endpoint();
		if (remote.is_unix())
			++g_count_accepted_remote_unix;
		if (remote.is_loopback())
			++g_count_accepted_remote_loopback;
		if (this->local_endpoint().is_unix())
			++g_count_accepted_local_unix;

		{
			std::lock_guard lock(g_lockable_server);
			g_server_remote_string = to_string(remote);
		}

		if (g_server_close_in_on_connect)
			this->closesocket();
	}
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override
	{
		++g_count_server_disconnected;
	}
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		if (_msg.buf_message.size() >= 8 && get_uds_message_index(reinterpret_cast<const char*>(_msg.buf_message.data())) == UDS_MESSAGE_CLOSE)
		{
			this->closesocket();
			return eRESULT::DONE;
		}
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

template <class TCONNECT>
class uds_test_client_socket_t : public net::socket::tcp<>, public TCONNECT
{
public:
			std::atomic<bool>	is_connected{ false };
			std::atomic<int>	count_connect{ 0 };
			std::atomic<int>	count_fail_connect{ 0 };
			std::atomic<int>	count_disconnect{ 0 };
			std::atomic<int>	domain{ -1 };
			net::sockaddr		local_address;
			net::sockaddr		remote_address;
			std::mutex			lockable;
			std::vector<std::string> received;
			std::atomic<std::size_t> bytes_received{ 0 };

protected:
	virtual void				on_connect() override
	{
		{
			std::lock_guard lock(this->lockable);
			this->local_address = this->local_endpoint();
			this->remote_address = this->remote_endpoint();
		}
		this->domain = socket_domain(this->native_handle());
		++this->count_connect;
		this->is_connected = true;
	}
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { ++this->count_fail_connect; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; ++this->count_disconnect; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		std::lock_guard lock(this->lockable);
		this->received.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		this->bytes_received += _msg.buf_message.size();
		return eRESULT::DONE;
	}
};

using uds_test_client_socket = uds_test_client_socket_t<net::io::connect_requestable::tcp::Ndefault>;
using uds_test_reconnect_socket = uds_test_client_socket_t<net::io::connect_requestable::Ntcp_reconnect>;

// - sends messages of several sizes and checks the echo (order and contents)
template <class TSOCKET>
void check_echo(TSOCKET* _psocket, uint32_t _index_base)
{
	const std::vector<std::size_t> sizes = { 8, 100, 1000, 16384, 70000, 9, 32768 };

	for (std::size_t i = 0; i < sizes.size(); ++i)
	{
		auto message = make_uds_message(_index_base + static_cast<uint32_t>(i), sizes[i]);
		auto buf = alloc_shared_buffer(message.size());
		buf.append(message.size(), message.data());
		ASSERT_TRUE(_psocket->send(buf)) << "send " << i;
	}

	ASSERT_TRUE(wait_until([&]() { std::lock_guard lock(_psocket->lockable); return _psocket->received.size() >= sizes.size(); })) << "echo";

	std::lock_guard lock(_psocket->lockable);
	ASSERT_EQ(_psocket->received.size(), sizes.size());
	for (std::size_t i = 0; i < sizes.size(); ++i)
		EXPECT_EQ(_psocket->received[i], make_uds_message(_index_base + static_cast<uint32_t>(i), sizes[i])) << "message " << i << " (size " << sizes[i] << ")";
	_psocket->received.clear();
}

// - sends a message (the index UDS_MESSAGE_CLOSE : the server closes the socket)
template <class TSOCKET>
bool send_uds_message(TSOCKET* _psocket, uint32_t _index, std::size_t _size)
{
	auto message = make_uds_message(_index, _size);
	auto buf = alloc_shared_buffer(message.size());
	buf.append(message.size(), message.data());
	return _psocket->send(buf);
}

net::acceptor<uds_echo_server_socket>::START_PARAMETER make_acceptor_parameter(net::ip::tcp::endpoint _local_endpoint, bool _enable_uds)
{
	net::acceptor<uds_echo_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = _local_endpoint;
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	parameter.enable_uds = _enable_uds;
	return parameter;
}

sSOCKET_CONNECT_REQUEST_PARAMETER make_connect_parameter(net::ip::tcp::endpoint _remote_endpoint)
{
	sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
	parameter.remote_endpoint = _remote_endpoint;
	parameter.enable_progress_log = false;
	return parameter;
}

// - starts an acceptor : the result (a failure may be thrown)
template <class TACCEPTOR, class TPARAMETER>
bool try_start_acceptor(TACCEPTOR& _pacceptor, const TPARAMETER& _parameter)
{
	try
	{
		return _pacceptor->start(_parameter) == eRESULT::SUCCESS;
	}
	catch (...)
	{
		return false;
	}
}

// - the default of the client switch is false (TCP). this suite tests the UDS path, so it turns the switch on at start-up.
//   (process wide : the other tests of this executable run with the switch on, as they did when the default was true)
static const bool s_uds_loopback_enabled_for_tests = (net::io::connect_requestable::Ntcp::enable_uds_loopback(true), true);

// - the switch of connect_requestable is restored at the end of a test
struct uds_loopback_switch_guard
{
	bool previous = net::io::connect_requestable::Ntcp::is_enable_uds_loopback();
	~uds_loopback_switch_guard() { net::io::connect_requestable::Ntcp::enable_uds_loopback(previous); }
};


//-----------------------------------------------------------------------------
// 1. listen
//-----------------------------------------------------------------------------
TEST(uds, listen_name_and_release)
{
	// 1) enable_uds : TCP and the UDS of the port
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	EXPECT_EQ(raw_uds_connect(port), 0);
	EXPECT_EQ(raw_tcp_connect(port), 0);

	// 2) stop : both are released
	pacceptor->stop();
	EXPECT_EQ(raw_uds_connect(port), ECONNREFUSED);
	EXPECT_EQ(raw_tcp_connect(port), ECONNREFUSED);

	// 3) start again at the same port : listened again
	auto parameter = make_acceptor_parameter({ net::ip::address_v4::loopback(), port }, true);
	parameter.reallocate_port_at_duplicate_port_error = false;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	EXPECT_EQ(raw_uds_connect(port), 0);
	pacceptor->stop();

	// 4) enable_uds : false => no UDS
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), port }, false)), eRESULT::SUCCESS);
	EXPECT_EQ(raw_uds_connect(pacceptor->local_endpoint().port()), ECONNREFUSED);
	pacceptor->stop();
}

TEST(uds, listen_stop_start_repeated)
{
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	auto parameter = make_acceptor_parameter({ net::ip::address_v4::any(), uds_test_port() }, true);

	for (int i = 0; i < 20; ++i)
	{
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS) << "loop " << i;
		const auto port = pacceptor->local_endpoint().port();
		parameter.local_endpoint.port(port);

		auto psocket = make_own<uds_test_client_socket>();
		psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); })) << "loop " << i;
		EXPECT_EQ(psocket->domain.load(), AF_UNIX) << "loop " << i;

		// - stop the acceptor with a connected socket : the socket is closed
		pacceptor->stop();
		EXPECT_TRUE(wait_until([&]() { return psocket->count_disconnect.load() == 1; })) << "loop " << i;
		EXPECT_EQ(raw_uds_connect(port), ECONNREFUSED) << "loop " << i;
		psocket->stop();
	}
}

TEST(uds, listen_ignored_for_non_loopback_address)
{
	const auto address = find_non_loopback_v4();
	if (address.empty())
		GTEST_SKIP() << "no non-loopback IPv4 address";

	// 1) enable_uds on a specific non-loopback address : a UDS would reach the acceptor from the same host only by its port.
	//    the TCP acceptor is not reachable through 127.0.0.1 : the UDS is not listened
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::make_address_v4(address), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();
	EXPECT_EQ(raw_uds_connect(port), ECONNREFUSED);

	// 2) a client to that address : TCP (not a loopback address)
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::make_address_v4(address), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	EXPECT_EQ(psocket->domain.load(), AF_INET);
	check_echo(psocket.get(), 50);

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, listen_name_conflict)
{
	const auto port = static_cast<uint16_t>(uds_test_port() + 2);

	// - someone else has the UDS name of the port
	raw_uds_listener listener(port);
	ASSERT_TRUE(listener.is_listening());

	// 1) enable_uds : TCP only (started)
	{
		auto parameter = make_acceptor_parameter({ net::ip::address_v4::loopback(), port }, true);
		parameter.reallocate_port_at_duplicate_port_error = false;
		auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
		ASSERT_TRUE(try_start_acceptor(pacceptor, parameter));
		EXPECT_EQ(raw_tcp_connect(port), 0);
		pacceptor->stop();
	}

	// 2) uds_only : fails (nothing to accept with)
	{
		auto parameter = make_acceptor_parameter({ net::ip::address_v4::loopback(), port }, false);
		parameter.uds_only = true;
		parameter.reallocate_port_at_duplicate_port_error = false;
		auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
		EXPECT_FALSE(try_start_acceptor(pacceptor, parameter));
		EXPECT_EQ(raw_tcp_bind(port), 0) << "the TCP port must be released";
		pacceptor->stop();
	}
}

TEST(uds, listen_context_parameters)
{
	reset_accepted();
	const auto port = uds_test_port();

	// 1) "enable_uds" : "true"
	{
		context context_setting;
		context_setting["local_endpoint"]["address"] = "127.0.0.1";
		context_setting["local_endpoint"]["port"] = port;
		context_setting["progress_log"] = "false";
		context_setting["enable_uds"] = "true";

		auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
		ASSERT_EQ(pacceptor->start(context_setting), eRESULT::SUCCESS);
		const auto port_listen = pacceptor->local_endpoint().port();
		EXPECT_EQ(raw_uds_connect(port_listen), 0);
		EXPECT_EQ(raw_tcp_connect(port_listen), 0);
		pacceptor->stop();
	}

	// 2) "enable_uds" : "false"
	{
		context context_setting;
		context_setting["local_endpoint"]["address"] = "127.0.0.1";
		context_setting["local_endpoint"]["port"] = port;
		context_setting["progress_log"] = "false";
		context_setting["enable_uds"] = "false";

		auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
		ASSERT_EQ(pacceptor->start(context_setting), eRESULT::SUCCESS);
		EXPECT_EQ(raw_uds_connect(pacceptor->local_endpoint().port()), ECONNREFUSED);
		pacceptor->stop();
	}

	// 3) "uds_only" : "1"
	{
		context context_setting;
		context_setting["local_endpoint"]["address"] = "127.0.0.1";
		context_setting["local_endpoint"]["port"] = port;
		context_setting["progress_log"] = "false";
		context_setting["uds_only"] = "1";

		auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
		ASSERT_EQ(pacceptor->start(context_setting), eRESULT::SUCCESS);
		const auto port_listen = pacceptor->local_endpoint().port();
		EXPECT_EQ(raw_uds_connect(port_listen), 0);
		EXPECT_EQ(raw_tcp_connect(port_listen), ECONNREFUSED);
		pacceptor->stop();
	}
}

TEST(uds, uds_only)
{
	reset_accepted();

	// 1) server (uds_only)
	auto parameter = make_acceptor_parameter({ net::ip::address_v4::any(), uds_test_port() }, false);
	parameter.uds_only = true;
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();
	ASSERT_NE(port, 0);

	// 2) TCP connects are refused and the port is reserved (bound, not listened)
	EXPECT_EQ(raw_tcp_connect(port), ECONNREFUSED);
	EXPECT_EQ(raw_tcp_bind(port), EADDRINUSE);

	// 3) client : 127.0.0.1 => UDS
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); })) << "connect (port " << port << ")";
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_unix.load() == 1; })) << "tcp " << g_count_accepted_tcp.load() << ", unix " << g_count_accepted_unix.load();
	EXPECT_EQ(g_count_accepted_tcp.load(), 0);
	check_echo(psocket.get(), 800);

	// 4) a client with the switch off : TCP is refused => on_fail_connect
	{
		uds_loopback_switch_guard guard;
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);
		auto psocket_tcp = make_own<uds_test_client_socket>();
		psocket_tcp->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		EXPECT_TRUE(wait_until([&]() { return psocket_tcp->count_fail_connect.load() == 1; }));
		EXPECT_EQ(psocket_tcp->count_connect.load(), 0);
		psocket_tcp->stop();
	}

	// 5) stop : the port and the UDS name are released
	psocket->stop();
	pacceptor->stop();
	EXPECT_EQ(raw_tcp_bind(port), 0);
	EXPECT_EQ(raw_uds_connect(port), ECONNREFUSED);

	// 6) start again at the same port (uds_only)
	parameter.local_endpoint.port(port);
	parameter.reallocate_port_at_duplicate_port_error = false;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	EXPECT_EQ(raw_tcp_connect(port), ECONNREFUSED);

	auto psocket_again = make_own<uds_test_client_socket>();
	psocket_again->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket_again->is_connected.load(); })) << "connect again";
	EXPECT_EQ(psocket_again->domain.load(), AF_UNIX);
	check_echo(psocket_again.get(), 900);

	psocket_again->stop();
	pacceptor->stop();
}

TEST(uds, uds_only_port_reallocated)
{
	// 1) a plain TCP acceptor holds the port
	auto pacceptor_tcp = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor_tcp->start(make_acceptor_parameter({ net::ip::address_v4::any(), uds_test_port() }, false)), eRESULT::SUCCESS);
	const auto port_tcp = pacceptor_tcp->local_endpoint().port();

	// 2) uds_only at the same port : the port is reallocated (the TCP bind fails first)
	auto parameter = make_acceptor_parameter({ net::ip::address_v4::any(), port_tcp }, false);
	parameter.uds_only = true;
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();
	EXPECT_NE(port, port_tcp);

	// 3) the client to the reallocated port => UDS
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); })) << "connect (port " << port << ")";
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	check_echo(psocket.get(), 1000);

	// 4) the client to the TCP acceptor's port : no UDS there => TCP
	auto psocket_tcp = make_own<uds_test_client_socket>();
	psocket_tcp->start(make_connect_parameter({ net::ip::address_v4::loopback(), port_tcp }));
	ASSERT_TRUE(wait_until([&]() { return psocket_tcp->is_connected.load(); }));
	EXPECT_EQ(psocket_tcp->domain.load(), AF_INET);

	psocket->stop();
	psocket_tcp->stop();
	pacceptor->stop();
	pacceptor_tcp->stop();
}


//-----------------------------------------------------------------------------
// 2. connect
//-----------------------------------------------------------------------------
TEST(uds, loopback_v4_through_uds)
{
	reset_accepted();

	// 1) server (enable_uds)
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) client : 127.0.0.1 => UDS
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); })) << "connect (port " << port << ")";

	// 3) both sides are UDS sockets, on_connect is called, the remote is the loopback address
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_unix.load() == 1; })) << "tcp " << g_count_accepted_tcp.load() << ", unix " << g_count_accepted_unix.load();
	EXPECT_EQ(g_count_accepted_tcp.load(), 0);
	EXPECT_EQ(g_count_accepted_remote_loopback.load(), 1);
	EXPECT_EQ(g_count_accepted_remote_unix.load(), 1);
	EXPECT_TRUE(psocket->remote_endpoint().is_loopback());
	EXPECT_TRUE(psocket->remote_endpoint().is_unix());

	// 4) echo
	check_echo(psocket.get(), 0);

	// 5) close
	psocket->stop();
	pacceptor->stop();
}

TEST(uds, loopback_v6_through_uds)
{
	reset_accepted();

	// 1) server (v6 any, enable_uds)
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v6::any(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) client : ::1 => UDS
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v6::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_unix.load() == 1; })) << "tcp " << g_count_accepted_tcp.load() << ", unix " << g_count_accepted_unix.load();

	// 3) echo
	check_echo(psocket.get(), 100);

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, tcp_without_enable_uds)
{
	reset_accepted();

	// 1) server (enable_uds : false)
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, false)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) client : no UDS listener => TCP
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	EXPECT_EQ(psocket->domain.load(), AF_INET);
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_tcp.load() == 1; })) << "tcp " << g_count_accepted_tcp.load() << ", unix " << g_count_accepted_unix.load();
	EXPECT_EQ(g_count_accepted_unix.load(), 0);
	EXPECT_FALSE(psocket->remote_endpoint().is_unix());

	check_echo(psocket.get(), 200);

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, tcp_when_disabled_or_bind_endpoint)
{
	reset_accepted();
	uds_loopback_switch_guard guard;

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) disabled by the switch => TCP
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);
	EXPECT_FALSE(net::io::connect_requestable::Ntcp::is_enable_uds_loopback());
	auto psocket_disabled = make_own<uds_test_client_socket>();
	psocket_disabled->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket_disabled->is_connected.load(); }));
	net::io::connect_requestable::Ntcp::enable_uds_loopback(true);
	EXPECT_EQ(psocket_disabled->domain.load(), AF_INET);

	// 2) a bind endpoint is given => TCP
	auto parameter = make_connect_parameter({ net::ip::address_v4::loopback(), port });
	parameter.bind_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), 0 };
	auto psocket_bind = make_own<uds_test_client_socket>();
	psocket_bind->start(parameter);
	ASSERT_TRUE(wait_until([&]() { return psocket_bind->is_connected.load(); }));
	EXPECT_EQ(psocket_bind->domain.load(), AF_INET);

	// 3) the default => UDS (together with the TCP ones)
	auto psocket_uds = make_own<uds_test_client_socket>();
	psocket_uds->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket_uds->is_connected.load(); }));
	EXPECT_EQ(psocket_uds->domain.load(), AF_UNIX);

	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_tcp.load() == 2 && g_count_accepted_unix.load() == 1; })) << "tcp " << g_count_accepted_tcp.load() << ", unix " << g_count_accepted_unix.load();

	check_echo(psocket_disabled.get(), 300);
	check_echo(psocket_bind.get(), 400);
	check_echo(psocket_uds.get(), 500);

	psocket_disabled->stop();
	psocket_bind->stop();
	psocket_uds->stop();
	pacceptor->stop();
}

TEST(uds, bind_endpoint_any_port_0_uses_uds)
{
	reset_accepted();

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// - any:0 is the same as no bind endpoint => UDS
	auto parameter = make_connect_parameter({ net::ip::address_v4::loopback(), port });
	parameter.bind_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::any(), 0 };
	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(parameter);
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	check_echo(psocket.get(), 550);

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, connect_nothing_listening)
{
	uds_loopback_switch_guard guard;

	// - a port nobody listens on (TCP nor UDS) : on_fail_connect at once, no on_connect (UDS refused => TCP refused)
	const auto port = static_cast<uint16_t>(uds_test_port() + 3);
	ASSERT_EQ(raw_tcp_connect(port), ECONNREFUSED);
	ASSERT_EQ(raw_uds_connect(port), ECONNREFUSED);

	for (bool enable : { true, false })
	{
		net::io::connect_requestable::Ntcp::enable_uds_loopback(enable);

		for (int i = 0; i < 50; ++i)
		{
			auto psocket = make_own<uds_test_client_socket>();
			psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
			ASSERT_TRUE(wait_until([&]() { return psocket->count_fail_connect.load() == 1; }, 3000ms)) << "uds " << enable << ", loop " << i;
			EXPECT_EQ(psocket->count_connect.load(), 0);
			psocket->stop();
		}
	}
}

TEST(uds, reconnect)
{
	// - the port is fixed before the acceptor starts (no reallocation) : one nobody listens on. (the ports of the tests come from
	//   the pid : the tests of another process (31400 + its pid % 500) may hold this one. the client connected to that listener
	//   before the acceptor started and the acceptor failed to bind)
	auto port = static_cast<uint16_t>(uds_test_port() + 1);
	for (int i = 0; i < 100 && (raw_tcp_connect(port) != ECONNREFUSED || raw_uds_connect(port) != ECONNREFUSED || raw_tcp_bind(port) != 0); ++i)
		port = static_cast<uint16_t>(port + 509);

	// 1) client first (reconnecting : no listener yet)
	auto psocket = make_own<uds_test_reconnect_socket>();
	auto parameter = make_connect_parameter({ net::ip::address_v4::loopback(), port });
	parameter.reconnection_enable = 2; // - 2 : enable, 1 : disable
	parameter.reconnection_interval = chrono::tick::milliseconds(200);
	psocket->start(parameter);

	std::this_thread::sleep_for(500ms);
	EXPECT_FALSE(psocket->is_connected.load());

	// 2) server
	auto acceptor_parameter = make_acceptor_parameter({ net::ip::address_v4::loopback(), port }, true);
	acceptor_parameter.reallocate_port_at_duplicate_port_error = false;
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(acceptor_parameter), eRESULT::SUCCESS);

	// 3) connected by reconnecting (through the UDS)
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }, 10000ms)) << "reconnect (port " << port << ")";
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	check_echo(psocket.get(), 600);

	// 4) the server stops and starts again : reconnected again
	pacceptor->stop();
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() == false; }));
	ASSERT_EQ(pacceptor->start(acceptor_parameter), eRESULT::SUCCESS);
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }, 10000ms)) << "reconnect again";
	EXPECT_GE(psocket->count_connect.load(), 2);
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	check_echo(psocket.get(), 700);

	// 5) the server restarts as uds_only : reconnected through the UDS
	pacceptor->stop();
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() == false; }));
	acceptor_parameter.uds_only = true;
	ASSERT_EQ(pacceptor->start(acceptor_parameter), eRESULT::SUCCESS);
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }, 10000ms)) << "reconnect to uds_only";
	EXPECT_EQ(psocket->domain.load(), AF_UNIX);
	check_echo(psocket.get(), 750);

	// 6) close
	psocket->disable_reconnect();
	psocket->stop();
	pacceptor->stop();
}

// - the connector (connective::connector) : a loopback connect goes through the UDS too (the same switch), TCP when disabled.
//   its sockets are pooled : the UDS and the TCP sockets alternate on the same objects
TEST(uds, connector)
{
	uds_loopback_switch_guard guard;

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), static_cast<uint16_t>(uds_test_port() + 5) }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto pconnector = make_own<net::connector<uds_test_client_socket>>();
	{
		context c;
		c["progress_log"] = "false";
		pconnector->start(c);
	}

	for (int i = 0; i < 20; ++i)
	{
		const bool enable = (i % 2) == 0;
		net::io::connect_requestable::Ntcp::enable_uds_loopback(enable);

		auto pconnectable = pconnector->request_connect({ net::ip::address_v4::loopback(), port });
		auto psocket = dynamic_cast<uds_test_client_socket*>(pconnectable.get());
		ASSERT_NE(psocket, nullptr);
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() || psocket->count_fail_connect.load() != 0; })) << "loop " << i;
		ASSERT_EQ(psocket->count_fail_connect.load(), 0) << "loop " << i;
		EXPECT_EQ(psocket->domain.load(), enable ? AF_UNIX : AF_INET) << "loop " << i;
		check_echo(psocket, 900 + static_cast<uint32_t>(i) * 10);

		psocket->closesocket();
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load() == false; })) << "loop " << i;
	}

	pconnector->close_connectable_all();
	pconnector->destroy();
	pacceptor->stop();
}


//-----------------------------------------------------------------------------
// 3. communication
//-----------------------------------------------------------------------------
TEST(uds, addresses_of_uds_socket)
{
	reset_accepted();

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_unix.load() == 1; }));

	// 1) client side : the addresses are AF_UNIX (is_unix, is_loopback, to_string "unix")
	{
		std::lock_guard lock(psocket->lockable);
		EXPECT_TRUE(psocket->local_address.is_unix());
		EXPECT_TRUE(psocket->remote_address.is_unix());
		EXPECT_TRUE(psocket->remote_address.is_loopback());
		EXPECT_FALSE(psocket->remote_address.is_v4());
		EXPECT_FALSE(psocket->remote_address.is_v6());
		EXPECT_EQ(to_string(psocket->remote_address), "unix");
	}

	// 2) server side
	EXPECT_EQ(g_count_accepted_remote_unix.load(), 1);
	EXPECT_EQ(g_count_accepted_local_unix.load(), 1);
	EXPECT_EQ(g_count_accepted_remote_loopback.load(), 1);
	{
		std::lock_guard lock(g_lockable_server);
		EXPECT_EQ(g_server_remote_string, "unix");
	}

	// 3) net::sockaddr : copy and compare keep AF_UNIX
	net::sockaddr address_copy;
	address_copy = psocket->remote_endpoint();
	EXPECT_TRUE(address_copy.is_unix());
	EXPECT_TRUE(address_copy == psocket->remote_endpoint());
	EXPECT_FALSE(address_copy == net::sockaddr(net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), port }));

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, large_data)
{
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<uds_test_client_socket>();
	psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
	ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
	ASSERT_EQ(psocket->domain.load(), AF_UNIX);

	// - 256 x 32KB (8MB) at once : sends that would block (EAGAIN) are queued and recovered by EPOLLOUT on both sides
	constexpr uint32_t count = 256;
	constexpr std::size_t size = 32768;
	for (uint32_t i = 0; i < count; ++i)
		ASSERT_TRUE(send_uds_message(psocket.get(), 10000 + i, size)) << "send " << i;

	ASSERT_TRUE(wait_until([&]() { return psocket->bytes_received.load() >= count * size; }, 20000ms)) << "received " << psocket->bytes_received.load();

	std::lock_guard lock(psocket->lockable);
	ASSERT_EQ(psocket->received.size(), count);
	for (uint32_t i = 0; i < count; ++i)
		ASSERT_EQ(psocket->received[i], make_uds_message(10000 + i, size)) << "message " << i;
	psocket->received.clear();

	psocket->stop();
	pacceptor->stop();
}

TEST(uds, many_sockets)
{
	reset_accepted();

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::any(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) 300 sockets at once
	constexpr int count = 300;
	std::vector<own_ptr<uds_test_client_socket>> sockets;
	for (int i = 0; i < count; ++i)
	{
		auto psocket = make_own<uds_test_client_socket>();
		psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		sockets.push_back(std::move(psocket));
	}
	ASSERT_TRUE(wait_until([&]() { return std::all_of(sockets.begin(), sockets.end(), [](auto& _p) { return _p->is_connected.load(); }); }, 10000ms));
	ASSERT_TRUE(wait_until([&]() { return g_count_accepted_unix.load() == count; })) << "unix " << g_count_accepted_unix.load();
	EXPECT_EQ(g_count_accepted_tcp.load(), 0);
	// - the acceptor lists a socket after its on_connect() (add_connectable after process_complete_connect) : waited
	EXPECT_TRUE(wait_until([&]() { return pacceptor->count() == static_cast<std::size_t>(count); })) << "listed " << pacceptor->count();

	// 2) each sends its index : each receives its own
	for (int i = 0; i < count; ++i)
		ASSERT_TRUE(send_uds_message(sockets[i].get(), static_cast<uint32_t>(i), 64));
	for (int i = 0; i < count; ++i)
	{
		ASSERT_TRUE(wait_until([&]() { std::lock_guard lock(sockets[i]->lockable); return sockets[i]->received.size() == 1; })) << "socket " << i;
		std::lock_guard lock(sockets[i]->lockable);
		EXPECT_EQ(sockets[i]->received[0], make_uds_message(static_cast<uint32_t>(i), 64)) << "socket " << i;
	}

	// 3) close all : the server gets on_disconnect for all
	for (auto& psocket : sockets)
		psocket->stop();
	EXPECT_TRUE(wait_until([&]() { return g_count_server_disconnected.load() == count; })) << "disconnected " << g_count_server_disconnected.load();
	EXPECT_TRUE(wait_until([&]() { return pacceptor->count() == 0; }));

	pacceptor->stop();
}

TEST(uds, close_by_client_and_by_server)
{
	reset_accepted();

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) the client closes (active close) : the server gets on_disconnect
	{
		auto psocket = make_own<uds_test_client_socket>();
		psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
		ASSERT_EQ(psocket->domain.load(), AF_UNIX);
		check_echo(psocket.get(), 1100);

		psocket->closesocket();
		EXPECT_TRUE(wait_until([&]() { return psocket->count_disconnect.load() == 1; }));
		EXPECT_TRUE(wait_until([&]() { return g_count_server_disconnected.load() == 1; }));
		psocket->stop();
	}

	// 2) the server closes (passive close for the client) : the client gets on_disconnect
	{
		auto psocket = make_own<uds_test_client_socket>();
		psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
		ASSERT_EQ(psocket->domain.load(), AF_UNIX);

		// - the data sent before the close request is echoed first
		check_echo(psocket.get(), 1200);
		ASSERT_TRUE(send_uds_message(psocket.get(), UDS_MESSAGE_CLOSE, 8));
		EXPECT_TRUE(wait_until([&]() { return psocket->count_disconnect.load() == 1; }));
		EXPECT_TRUE(wait_until([&]() { return g_count_server_disconnected.load() == 2; }));
		EXPECT_FALSE(psocket->is_connected.load());
		psocket->stop();
	}

	// 3) the acceptor stops : the clients get on_disconnect
	{
		auto psocket = make_own<uds_test_client_socket>();
		psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
		ASSERT_TRUE(wait_until([&]() { return psocket->is_connected.load(); }));
		pacceptor->stop();
		EXPECT_TRUE(wait_until([&]() { return psocket->count_disconnect.load() == 1; }));
		psocket->stop();
	}
}


//-----------------------------------------------------------------------------
// 4. epoll handles
//
//   the fd closed by a socket is reused by another socket at once.
//   - a connect completion must not touch the fd after the socket was closed meanwhile
//     (set_event() after on_connect replaced the executable of another socket given the same fd number : its connect never completed)
//   - a socket closed while connecting must not keep the closed fd as its epoll handle
//     (the destructor detached and closed it : another socket's fd)
//   - no explicit bind for any:0 (bind(0) was slow and failed with many ports in TIME_WAIT)
//-----------------------------------------------------------------------------
// - closed in on_connect, or by another thread as soon as on_connect is called
template <class TCONNECT>
class uds_close_at_connect_socket_t : public net::socket::tcp<>, public TCONNECT
{
public:
			bool				close_in_on_connect = false;
			std::atomic<int>	state{ 0 };		// - 0 : connecting, 1 : connected, 2 : failed, 3 : disconnected
protected:
	virtual void				on_connect() override { this->state = 1; if (this->close_in_on_connect) this->closesocket(); }
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { this->state = 2; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->state = 3; }
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
};

using uds_close_at_connect_socket = uds_close_at_connect_socket_t<net::io::connect_requestable::tcp::Ndefault>;
using uds_close_at_connect_reconnect_socket = uds_close_at_connect_socket_t<net::io::connect_requestable::Ntcp_reconnect>;

// - many sockets connect and close at once (in on_connect or by another thread right after it)
template <class TSOCKET>
void check_close_at_connect(uint16_t _port, bool _uds, bool _close_in_on_connect, int _count_thread = 16, int _count_loop = 300)
{
	uds_loopback_switch_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(_uds);

	std::atomic<int> count_stuck{ 0 };
	std::atomic<int> count_done{ 0 };
	std::vector<std::thread> threads;

	for (int t = 0; t < _count_thread; ++t)
	{
		threads.emplace_back([&]()
		{
			for (int i = 0; i < _count_loop; ++i)
			{
				auto psocket = make_own<TSOCKET>();
				psocket->close_in_on_connect = _close_in_on_connect;
				auto parameter = make_connect_parameter({ net::ip::address_v4::loopback(), _port });
				parameter.reconnection_enable = 1; // - Ntcp_reconnect : no reconnect (1 : disable)
				psocket->start(parameter);

				// - closes as soon as connected (spin : not to miss the window)
				const auto time_end = std::chrono::steady_clock::now() + 3s;
				while (psocket->state.load() == 0 && std::chrono::steady_clock::now() < time_end)
					std::this_thread::yield();

				if (psocket->state.load() == 1 && _close_in_on_connect == false)
					psocket->closesocket();

				if (!wait_until([&]() { return psocket->state.load() >= 2; }, 3000ms))
				{
					++count_stuck;
					psocket->stop();
					continue;
				}
				if (psocket->state.load() == 3)
					++count_done;
			}
		});
	}
	for (auto& t : threads)
		t.join();

	const char* name = _uds ? (_close_in_on_connect ? "uds, in on_connect" : "uds, other thread") : (_close_in_on_connect ? "tcp, in on_connect" : "tcp, other thread");
	EXPECT_EQ(count_stuck.load(), 0) << name;
	EXPECT_EQ(count_done.load(), _count_thread * _count_loop) << name;
}

TEST(uds, close_at_connect)
{
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	check_close_at_connect<uds_close_at_connect_socket>(port, true, false);
	check_close_at_connect<uds_close_at_connect_socket>(port, true, true);
	check_close_at_connect<uds_close_at_connect_socket>(port, false, false);
	check_close_at_connect<uds_close_at_connect_socket>(port, false, true);

	pacceptor->stop();
}

TEST(uds, close_at_connect_reconnect_socket)
{
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	check_close_at_connect<uds_close_at_connect_reconnect_socket>(port, true, false, 8, 200);
	check_close_at_connect<uds_close_at_connect_reconnect_socket>(port, true, true, 8, 200);
	check_close_at_connect<uds_close_at_connect_reconnect_socket>(port, false, false, 8, 200);

	pacceptor->stop();
}

TEST(uds, server_closes_in_on_connect)
{
	reset_accepted();
	uds_loopback_switch_guard guard;

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();
	g_server_close_in_on_connect = true;

	// - the server closes every socket in on_connect : the clients get on_disconnect, the acceptor keeps no socket
	constexpr int count_thread = 8;
	constexpr int count_loop = 100;

	for (bool uds : { true, false })
	{
		net::io::connect_requestable::Ntcp::enable_uds_loopback(uds);
		std::atomic<int> count_stuck{ 0 };
		std::vector<std::thread> threads;

		for (int t = 0; t < count_thread; ++t)
		{
			threads.emplace_back([&]()
			{
				for (int i = 0; i < count_loop; ++i)
				{
					auto psocket = make_own<uds_test_client_socket>();
					psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
					if (!wait_until([&]() { return psocket->count_disconnect.load() == 1 || psocket->count_fail_connect.load() == 1; }, 3000ms))
						++count_stuck;
					psocket->stop();
				}
			});
		}
		for (auto& t : threads)
			t.join();

		EXPECT_EQ(count_stuck.load(), 0) << (uds ? "uds" : "tcp");
	}

	EXPECT_TRUE(wait_until([&]() { return g_count_server_disconnected.load() == g_count_accepted_unix.load() + g_count_accepted_tcp.load(); })) << "accepted " << (g_count_accepted_unix.load() + g_count_accepted_tcp.load()) << ", disconnected " << g_count_server_disconnected.load();
	EXPECT_GT(g_count_accepted_unix.load(), 0);
	EXPECT_GT(g_count_accepted_tcp.load(), 0);
	EXPECT_TRUE(wait_until([&]() { return pacceptor->count() == 0; }));

	g_server_close_in_on_connect = false;
	pacceptor->stop();
}

TEST(uds, stop_while_connecting)
{
	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, true)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) sockets stopped while connecting (192.0.2.1 : TEST-NET-1, the connect is pending or fails) and destroyed at once
	std::atomic<bool> stop{ false };
	std::thread thread_closer([&]()
	{
		while (stop == false)
		{
			auto psocket = make_own<uds_test_client_socket>();
			psocket->start(make_connect_parameter({ net::ip::make_address_v4("192.0.2.1"), 9 }));
			psocket->stop();
		}
	});

	// 2) meanwhile, the other sockets connect and echo (their fd numbers are the ones just closed)
	std::atomic<int> count_failed{ 0 };
	std::vector<std::thread> threads;
	for (int t = 0; t < 8; ++t)
	{
		threads.emplace_back([&, t]()
		{
			for (int i = 0; i < 50; ++i)
			{
				auto psocket = make_own<uds_test_client_socket>();
				psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
				if (!wait_until([&]() { return psocket->is_connected.load(); }, 3000ms) || !send_uds_message(psocket.get(), static_cast<uint32_t>(t * 1000 + i), 128) || !wait_until([&]() { return psocket->bytes_received.load() == 128; }, 3000ms))
					++count_failed;
				psocket->stop();
			}
		});
	}
	for (auto& t : threads)
		t.join();
	stop = true;
	thread_closer.join();

	EXPECT_EQ(count_failed.load(), 0);
	pacceptor->stop();
}

TEST(uds, many_tcp_connects_without_bind)
{
	uds_loopback_switch_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	auto pacceptor = make_own<net::acceptor<uds_echo_server_socket>>();
	ASSERT_EQ(pacceptor->start(make_acceptor_parameter({ net::ip::address_v4::loopback(), uds_test_port() }, false)), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// - 3000 TCP connects closed by the client (TIME_WAIT on the client side) : connect() takes the local port itself
	constexpr int count_thread = 4;
	constexpr int count_loop = 750;
	std::atomic<int> count_failed{ 0 };
	std::atomic<int> count_no_port{ 0 };
	std::vector<std::thread> threads;

	for (int t = 0; t < count_thread; ++t)
	{
		threads.emplace_back([&]()
		{
			for (int i = 0; i < count_loop; ++i)
			{
				auto psocket = make_own<uds_test_client_socket>();
				psocket->start(make_connect_parameter({ net::ip::address_v4::loopback(), port }));
				if (!wait_until([&]() { return psocket->is_connected.load(); }, 3000ms))
				{
					++count_failed;
				}
				else
				{
					std::lock_guard lock(psocket->lockable);
					if (psocket->domain.load() != AF_INET || psocket->local_address.port() == 0)
						++count_no_port;
				}
				psocket->stop();
			}
		});
	}
	for (auto& t : threads)
		t.join();

	EXPECT_EQ(count_failed.load(), 0);
	EXPECT_EQ(count_no_port.load(), 0);
	pacceptor->stop();
}

}

#endif
