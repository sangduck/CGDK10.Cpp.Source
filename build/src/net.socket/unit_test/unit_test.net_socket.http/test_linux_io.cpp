#include "pch.h"
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <filesystem>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <cstdio>

//-----------------------------------------------------------------------------
// linux socket performance features (functional checks)
//
//  1. TCP echo : many small messages in a stream (the receive buffer grows and keeps its size : hysteresis)
//  2. MSG_ZEROCOPY : large messages. the buffers held for the notifications are released (EPOLLERR : error queue), ENOBUFS (no memory to pin) => sent as a copy,
//     the copy check (a send the kernel copied : loopback => no zerocopy on the socket)
//  3. UDP : recvmmsg / sendmmsg (a burst of datagrams, echo)
//  4. acceptor : cpu affinity (an accepted socket runs on the io_context of its CPU), TCP_DEFER_ACCEPT
//  5. io_uring : an executor with the io_uring backend (TCP, UDS, large messages, UDP). completion : the accepted sockets spread over the workers
//  6. busy poll of epoll_wait() : EPIOCSPARAMS by the parameters / set_busy_poll() / the default, io_uring ignores it
//  7. send cork : the responses deferred in a long batch of received messages are sent when they are old (not all at its end)
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <sys/ioctl.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	template <class TCONDITION>
	bool lio_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
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

	// - the port of the tests (32400 + pid % 500 : not the ports of the other tests)
	uint16_t lio_port()
	{
		return static_cast<uint16_t>(32400 + api::get_process_id() % 500);
	}

	// - [uint32 size][uint32 index][pattern...]
	std::string lio_message(uint32_t _index, std::size_t _size)
	{
		std::string message(_size, '\0');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		memcpy(message.data() + 4, &_index, 4);
		for (std::size_t i = 8; i < _size; ++i)
			message[i] = static_cast<char>('a' + (i + _index) % 26);
		return message;
	}

	uint32_t lio_index(const char* _data)
	{
		uint32_t index = 0;
		memcpy(&index, _data + 4, 4);
		return index;
	}

	shared_buffer lio_buffer(const std::string& _message)
	{
		auto buf = alloc_shared_buffer(_message.size());
		buf.append(_message.size(), _message.data());
		return buf;
	}

	// - the io_uring rings of this process (/proc/self/fd : anon_inode:[io_uring])
	int lio_count_io_uring_fd()
	{
		int count = 0;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd", error))
		{
			std::error_code error_link;
			const auto target = std::filesystem::read_symlink(entry.path(), error_link);
			if (!error_link && target.string().find("io_uring") != std::string::npos)
				++count;
		}
		return count;
	}

	// - the io_uring fds once their number stayed the same for 200ms (at most 3s) : a completion system executor creates the ring of
	//   each worker when that worker starts (after executor() returned : counted before, they were taken for the executor's under test)
	int lio_count_io_uring_fd_settled()
	{
		int count = lio_count_io_uring_fd();
		const auto tick_end = std::chrono::steady_clock::now() + 3s;
		auto tick_same = std::chrono::steady_clock::now();

		while (std::chrono::steady_clock::now() < tick_end)
		{
			std::this_thread::sleep_for(20ms);

			const int count_now = lio_count_io_uring_fd();

			if (count_now != count)
			{
				count = count_now;
				tick_same = std::chrono::steady_clock::now();
			}
			else if (std::chrono::steady_clock::now() - tick_same >= 200ms)
			{
				break;
			}
		}

		return count;
	}

	// - the UDS switch of connect_requestable is restored at the end of a test
	struct lio_uds_guard
	{
		bool previous = net::io::connect_requestable::Ntcp::is_enable_uds_loopback();
		~lio_uds_guard() { net::io::connect_requestable::Ntcp::enable_uds_loopback(previous); }
	};
}

//-----------------------------------------------------------------------------
// sockets
//-----------------------------------------------------------------------------
class lio_echo_server_socket : public net::socket::tcp<>
{
protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

class lio_client_socket : public net::socket::tcp<>, public net::io::connect_requestable::tcp::Ndefault
{
public:
			std::atomic<bool>	is_connected{ false };
			std::atomic<int>	count_fail_connect{ 0 };
			std::atomic<int>	count_disconnect{ 0 };
			std::atomic<std::size_t> bytes_received{ 0 };
			std::atomic<std::size_t> count_received{ 0 };
			bool				keep_received = true;
			std::mutex			lockable;
			std::vector<std::string> received;

protected:
	virtual void				on_connect() override { this->is_connected = true; }
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override { ++this->count_fail_connect; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; ++this->count_disconnect; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		this->bytes_received += _msg.buf_message.size();
		++this->count_received;

		if (this->keep_received)
		{
			std::lock_guard lock(this->lockable);
			this->received.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
		}

		return eRESULT::DONE;
	}
};

class lio_udp_socket : public net::socket::udp
{
public:
			bool				echo = false;
			std::atomic<std::size_t> count_received{ 0 };
			std::atomic<std::size_t> bytes_received{ 0 };
			std::mutex			lockable;
			std::vector<uint32_t> indexes;

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		++this->count_received;
		this->bytes_received += _msg.buf_message.size();

		if (this->echo)
		{
			this->send_to(_msg.buf_message, _msg.address);
		}
		else if (_msg.buf_message.size() >= 8)
		{
			std::lock_guard lock(this->lockable);
			this->indexes.push_back(lio_index(reinterpret_cast<const char*>(_msg.buf_message.data())));
		}

		return eRESULT::DONE;
	}
};

net::acceptor<lio_echo_server_socket>::START_PARAMETER lio_acceptor_parameter(uint16_t _port)
{
	net::acceptor<lio_echo_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _port };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	return parameter;
}

sSOCKET_CONNECT_REQUEST_PARAMETER lio_connect_parameter(uint16_t _port)
{
	sSOCKET_CONNECT_REQUEST_PARAMETER parameter;
	parameter.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _port };
	parameter.enable_progress_log = false;
	return parameter;
}

template <class TSOCKET>
bool lio_send(TSOCKET* _psocket, uint32_t _index, std::size_t _size)
{
	return _psocket->send(lio_buffer(lio_message(_index, _size)));
}

// - an acceptor and clients on an io_context (nullptr : the system executor) : every client sends _count_message messages of _size and gets them back
void lio_check_tcp_echo(executor::Iio_context* _pio_context, bool _uds, bool _cpu_affinity, int _defer_accept, int _count_client, int _count_message, std::size_t _size)
{
	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(_uds);

	// 1) server
	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	if (_pio_context != nullptr)
		pacceptor->io_context(_pio_context);

	auto parameter = lio_acceptor_parameter(lio_port());
	parameter.enable_uds = _uds;
	parameter.enable_cpu_affinity = _cpu_affinity;
	parameter.tcp_defer_accept = _defer_accept;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) clients
	std::vector<own_ptr<lio_client_socket>> clients;
	for (int i = 0; i < _count_client; ++i)
	{
		auto psocket = make_own<lio_client_socket>();
		psocket->keep_received = (_size <= 1024);
		if (_pio_context != nullptr)
			psocket->io_context(_pio_context);
		psocket->start(lio_connect_parameter(port));
		clients.push_back(std::move(psocket));
	}
	ASSERT_TRUE(lio_wait_until([&]() { return std::all_of(clients.begin(), clients.end(), [](auto& _p) { return _p->is_connected.load(); }); }, 10000ms)) << "connect";
	ASSERT_TRUE(lio_wait_until([&]() { return pacceptor->count() == static_cast<std::size_t>(_count_client); })) << "accepted " << pacceptor->count();

	// 3) send all at once (pipelined)
	for (int m = 0; m < _count_message; ++m)
		for (int i = 0; i < _count_client; ++i)
			ASSERT_TRUE(lio_send(clients[i].get(), static_cast<uint32_t>(i * 100000 + m), _size)) << "send " << i << "/" << m;

	// 4) echo
	const auto bytes_expected = static_cast<std::size_t>(_count_message) * _size;
	ASSERT_TRUE(lio_wait_until([&]() { return std::all_of(clients.begin(), clients.end(), [&](auto& _p) { return _p->bytes_received.load() == bytes_expected; }); }, 60000ms)) << "received " << clients[0]->bytes_received.load() << " / " << bytes_expected;

	if (_size <= 1024)
	{
		for (int i = 0; i < _count_client; ++i)
		{
			std::lock_guard lock(clients[i]->lockable);
			ASSERT_EQ(clients[i]->received.size(), static_cast<std::size_t>(_count_message));
			for (int m = 0; m < _count_message; ++m)
				ASSERT_EQ(clients[i]->received[m], lio_message(static_cast<uint32_t>(i * 100000 + m), _size)) << "client " << i << " message " << m;
		}
	}

	// 5) close
	for (auto& psocket : clients)
		psocket->stop();
	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
}

// - a UDP echo server and a client on an io_context (nullptr : the system executor) : _count datagrams of _size at once
void lio_check_udp_echo(executor::Iio_context* _pio_context, int _count, std::size_t _size)
{
	auto pserver = make_own<lio_udp_socket>();
	pserver->echo = true;
	if (_pio_context != nullptr)
		pserver->io_context(_pio_context);
	ASSERT_EQ(pserver->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);
	const auto port = pserver->local_endpoint().port();
	ASSERT_NE(port, 0);

	auto pclient = make_own<lio_udp_socket>();
	if (_pio_context != nullptr)
		pclient->io_context(_pio_context);
	ASSERT_EQ(pclient->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);

	const net::sockaddr address_server{ net::ip::address_v4::loopback(), port };

	// - windows of datagrams (a window at once : recvmmsg takes them in batches). not one burst : UDP keeps nothing, the kernel
	//   drops what the socket buffer (rmem_default 212KB : ~26 datagrams of 8000B, ~200 of 64B) does not hold while the consumer is slow
	const int window = (_size >= 4096) ? 8 : 64;

	for (int sent = 0; sent < _count; )
	{
		const int count_window = std::min(window, _count - sent);

		for (int i = 0; i < count_window; ++i, ++sent)
			ASSERT_TRUE(pclient->send_to(lio_buffer(lio_message(static_cast<uint32_t>(sent), _size)), address_server)) << "send " << sent;

		ASSERT_TRUE(lio_wait_until([&]() { return pclient->count_received.load() >= static_cast<std::size_t>(sent); }, 20000ms)) << "echoed " << pclient->count_received.load() << " / " << sent << " (server received " << pserver->count_received.load() << ")";
	}

	ASSERT_EQ(pclient->count_received.load(), static_cast<std::size_t>(_count));
	EXPECT_EQ(pclient->bytes_received.load(), static_cast<std::size_t>(_count) * _size);

	// - every index once (the order is not kept by UDP)
	{
		std::lock_guard lock(pclient->lockable);
		std::vector<uint32_t> indexes = pclient->indexes;
		std::sort(indexes.begin(), indexes.end());
		ASSERT_EQ(indexes.size(), static_cast<std::size_t>(_count));
		for (int i = 0; i < _count; ++i)
			ASSERT_EQ(indexes[i], static_cast<uint32_t>(i)) << "index " << i;
	}

	pclient->stop();
	pserver->stop();
}


//-----------------------------------------------------------------------------
// 1. TCP stream of small messages (the receive buffer)
//-----------------------------------------------------------------------------
TEST(linux_io, tcp_stream_small_messages)
{
	lio_check_tcp_echo(nullptr, false, false, 0, 4, 2000, 64);
}

TEST(linux_io, tcp_stream_mixed_sizes)
{
	lio_check_tcp_echo(nullptr, false, false, 0, 2, 200, 700);
	lio_check_tcp_echo(nullptr, false, false, 0, 2, 50, 9000);
}


//-----------------------------------------------------------------------------
// 2. MSG_ZEROCOPY (large messages on the io_uring backend : the owners are held until the kernel notifies)
//-----------------------------------------------------------------------------
TEST(linux_io, tcp_zerocopy_large_messages_io_uring)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	lio_uds_guard guard;

	// - the copy check off : every large send is a zerocopy send over loopback too (the kernel copies : on, the first copy stops it)
	struct copy_check_off
	{
		bool previous = net::io::sendable::Itcp_gather::is_zerocopy_copy_check_enabled();
		copy_check_off() { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(false); }
		~copy_check_off() { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(this->previous); }
	} guard_copy_check;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	ASSERT_TRUE(net::io::sendable::Itcp_gather::is_zerocopy_enabled());
	ASSERT_GE(net::io::sendable::Itcp_gather::zerocopy_threshold(), 1024u);

	// 1) an io_uring executor : zerocopy is used on it only
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("io_uring zerocopy", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());

	// check) the ring could not be created (the locked memory of the user is short : the other processes pin too) => the executor is epoll (no zerocopy)
	if (pexecutor->backend() == executor::io_backend::epoll)
		GTEST_SKIP() << "the io_uring executor fell back to epoll";

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<lio_client_socket>();
	psocket->keep_received = false;
	psocket->io_context(pexecutor.get());
	psocket->start(lio_connect_parameter(port));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

	// 2) 32 x 256KB (above the threshold : MSG_ZEROCOPY on the client and the echo of the server)
	constexpr int count = 32;
	constexpr std::size_t size = 256 * 1024;
	for (int i = 0; i < count; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(i), size)) << "send " << i;

	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == count * size; }, 30000ms)) << "received " << psocket->bytes_received.load();

	// 3) the sends were zerocopy (or refused with ENOBUFS and copied : the locked memory of the user is short, the other processes pin too)
	//    and the buffers held for the notifications are released (EPOLLERR : the socket is still open)
	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
	ASSERT_NE(pgather, nullptr);
	EXPECT_GT(pgather->zerocopy_sent_count() + pgather->zerocopy_fallback_count(), 0u);
	EXPECT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; })) << "pending " << pgather->zerocopy_pending_count();
	EXPECT_TRUE(psocket->is_connected.load());
	EXPECT_EQ(psocket->count_disconnect.load(), 0);

	// 4) more after the notifications (the socket keeps working)
	for (int i = 0; i < 4; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(100 + i), size));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == (count + 4) * size; }, 30000ms));
	EXPECT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; }));

	psocket->stop();
	pacceptor->stop();
	pexecutor->stop();
}

// - a stale EPOLLERR : the notifications it reported were read already (by the loop of an earlier event, or the poll of another thread
//   reported them too) => the error queue is empty and the socket has no error : not an error. before : the socket was closed
//   (io_uring readiness : linux_io.tcp_stream_mixed_sizes failed ~5% of the runs, the echo server closed and the client send got EPIPE)
TEST(linux_io, tcp_zerocopy_stale_epollerr_io_uring)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	lio_uds_guard guard;

	struct copy_check_off
	{
		bool previous = net::io::sendable::Itcp_gather::is_zerocopy_copy_check_enabled();
		copy_check_off() { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(false); }
		~copy_check_off() { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(this->previous); }
	} guard_copy_check;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("io_uring stale epollerr", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());

	if (pexecutor->backend() == executor::io_backend::epoll)
		GTEST_SKIP() << "the io_uring executor fell back to epoll";

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<lio_client_socket>();
	psocket->keep_received = false;
	psocket->io_context(pexecutor.get());
	psocket->start(lio_connect_parameter(port));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

	// 1) zerocopy sends and their notifications (the error queue is read and empty after)
	constexpr std::size_t size = 64 * 1024;
	for (int i = 0; i < 4; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(i), size)) << "send " << i;
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == 4 * size; }, 30000ms)) << "received " << psocket->bytes_received.load();

	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
	ASSERT_NE(pgather, nullptr);
	if (pgather->zerocopy_sent_count() == 0)
		GTEST_SKIP() << "no zerocopy send (ENOBUFS : the locked memory of the user is short)";
	ASSERT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; })) << "pending " << pgather->zerocopy_pending_count();

	// 2) an EPOLLERR now finds nothing : handled (not an error of the socket)
	EXPECT_TRUE(pgather->process_complete_error_queue());

	// 3) the socket keeps working
	for (int i = 0; i < 4; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(100 + i), size)) << "send " << (100 + i);
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == 8 * size; }, 30000ms)) << "received " << psocket->bytes_received.load();
	EXPECT_TRUE(psocket->is_connected.load());
	EXPECT_EQ(psocket->count_disconnect.load(), 0);

	psocket->stop();
	pacceptor->stop();
	pexecutor->stop();
}

// - no memory to pin (RLIMIT_MEMLOCK 0 : the pages of a zerocopy send count against it, like the limit of the user shared with its
//   other processes) : the zerocopy sends are refused with ENOBUFS and go again as copies (closed before : "received 0" when the
//   receiver of the reset had not read a whole message yet)
TEST(linux_io, tcp_zerocopy_falls_back_to_copy_on_enobufs)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	// check) CAP_IPC_LOCK (root) is not limited by RLIMIT_MEMLOCK
	if (::geteuid() == 0)
		GTEST_SKIP() << "root is not limited by RLIMIT_MEMLOCK";

	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("io_uring zerocopy enobufs", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());

	// check) the ring could not be created (the locked memory of the user is short : the other processes pin too) => the executor is epoll (no zerocopy)
	if (pexecutor->backend() == executor::io_backend::epoll)
		GTEST_SKIP() << "the io_uring executor fell back to epoll";

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<lio_client_socket>();
	psocket->keep_received = false;
	psocket->io_context(pexecutor.get());
	psocket->start(lio_connect_parameter(port));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

	// 1) RLIMIT_MEMLOCK 0 (the soft limit of this process, restored at the end)
	struct memlock_guard
	{
		::rlimit before{};
		bool is_set = false;
		memlock_guard() { if (::getrlimit(RLIMIT_MEMLOCK, &before) == 0) { ::rlimit low = before; low.rlim_cur = 0; is_set = (::setrlimit(RLIMIT_MEMLOCK, &low) == 0); } }
		~memlock_guard() { if (is_set) ::setrlimit(RLIMIT_MEMLOCK, &before); }
	} memlock;
	ASSERT_TRUE(memlock.is_set);

	// 2) 16 x 256KB : every message comes back, the socket stays connected
	constexpr int count = 16;
	constexpr std::size_t size = 256 * 1024;
	for (int i = 0; i < count; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(i), size)) << "send " << i << " error " << psocket->get_last_socket_error();

	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == count * size; }, 30000ms)) << "received " << psocket->bytes_received.load() << " connected " << psocket->is_connected.load() << " error " << psocket->get_last_socket_error();
	EXPECT_TRUE(psocket->is_connected.load());
	EXPECT_EQ(psocket->count_disconnect.load(), 0);

	// 3) the refused sends were sent as copies (the client : its first large send at least)
	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
	ASSERT_NE(pgather, nullptr);
	EXPECT_GT(pgather->zerocopy_fallback_count(), 0u) << "backend " << static_cast<int>(psocket->io_context()->backend()) << " sent " << pgather->zerocopy_sent_count();
	EXPECT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; })) << "pending " << pgather->zerocopy_pending_count();

	psocket->stop();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(linux_io, tcp_zerocopy_not_on_epoll)
{
	// - an executor with the epoll backend (the system executor follows the option : CGDK_IO_URING=1 makes it io_uring) :
	//   a large send is a copy as before (nothing is held, nothing counted)
	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	// - the system executor (its ring with CGDK_IO_URING=1) is created before the count (by the first socket otherwise : the test alone counted it)
	static_cast<void>(system_executor::executor());
	const int count_fd_before = lio_count_io_uring_fd_settled();
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::epoll);
	auto pexecutor = executor::make_io_context_threads("epoll executor", 2);
	executor::set_default_io_backend(backend_before);

	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(lio_count_io_uring_fd(), count_fd_before) << "the executor uses io_uring";

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<lio_client_socket>();
	psocket->keep_received = false;
	psocket->io_context(pexecutor.get());
	psocket->start(lio_connect_parameter(port));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

	constexpr std::size_t size = 128 * 1024;
	for (int i = 0; i < 8; ++i)
		ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(i), size));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == 8 * size; }, 30000ms));

	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
	ASSERT_NE(pgather, nullptr);
	EXPECT_EQ(pgather->zerocopy_sent_count(), 0u);
	EXPECT_EQ(pgather->zerocopy_pending_count(), 0u);

	psocket->stop();
	pacceptor->stop();
	pexecutor->stop();
}

// - the copy check : a peer on this host (loopback) gets the zerocopy sends copied by the kernel (SO_EE_CODE_ZEROCOPY_COPIED, the
//   completion notification IORING_NOTIF_USAGE_ZC_COPIED : 6.2+). ZEROCOPY_COPIED_TO_STOP in a row stop zerocopy on the socket (on by default),
//   off : zerocopy goes on and the copies are counted only
namespace
{
	struct lio_zerocopy_copy_check_guard
	{
		bool previous = net::io::sendable::Itcp_gather::is_zerocopy_copy_check_enabled();
		explicit lio_zerocopy_copy_check_guard(bool _enable) { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(_enable); }
		~lio_zerocopy_copy_check_guard() { net::io::sendable::Itcp_gather::enable_zerocopy_copy_check(this->previous); }
	};

	bool lio_is_kernel_at_least(int _major, int _minor)
	{
		utsname name{};
		int major = 0, minor = 0;
		if (::uname(&name) != 0 || std::sscanf(name.release, "%d.%d", &major, &minor) != 2)
			return false;
		return major > _major || (major == _major && minor >= _minor);
	}
}

void lio_check_zerocopy_stops_when_copied(executor::io_backend _backend)
{
	if (_backend == executor::io_backend::io_uring && executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";
	if (_backend == executor::io_backend::io_uring_completion && executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";
	if (_backend == executor::io_backend::io_uring_completion && lio_is_kernel_at_least(6, 2) == false)
		GTEST_SKIP() << "IORING_SEND_ZC_REPORT_USAGE needs 6.2";

	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(_backend);
	auto pexecutor = executor::make_io_context_threads("zerocopy copy check", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());

	// check) the ring could not be created (the locked memory of the user is short) => the executor is epoll (no zerocopy)
	if (pexecutor->backend() == executor::io_backend::epoll)
		GTEST_SKIP() << "the io_uring executor fell back to epoll";

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	constexpr std::size_t size = 256 * 1024;

	for (const bool is_check : { true, false })
	{
		lio_zerocopy_copy_check_guard guard_check(is_check);

		auto psocket = make_own<lio_client_socket>();
		psocket->keep_received = false;
		psocket->io_context(pexecutor.get());
		psocket->start(lio_connect_parameter(port));
		ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

		auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
		ASSERT_NE(pgather, nullptr);

		// 1) large messages one by one (each echoed before the next) : the kernel reports them copied (ZEROCOPY_COPIED_TO_STOP in a row stop zerocopy)
		uint32_t index = 0;
		std::size_t bytes_expected = 0;
		for (std::size_t i = 0; i < net::io::sendable::Itcp_gather::ZEROCOPY_COPIED_TO_STOP + 4; ++i)
		{
			ASSERT_TRUE(lio_send(psocket.get(), index++, size)) << "send " << i;
			bytes_expected += size;
			ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == bytes_expected; }, 30000ms)) << "received " << psocket->bytes_received.load();
		}

		ASSERT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_copied_count() >= net::io::sendable::Itcp_gather::ZEROCOPY_COPIED_TO_STOP; })) << "check " << is_check << " : no copy reported (sent " << pgather->zerocopy_sent_count() << " fallback " << pgather->zerocopy_fallback_count() << ")";
		if (is_check)
			EXPECT_TRUE(lio_wait_until([&]() { return pgather->is_zerocopy_copied(); })) << "copied " << pgather->zerocopy_copied_count();
		else
			EXPECT_FALSE(pgather->is_zerocopy_copied());

		// 2) more : no zerocopy any more with the check on (a send before the report may still have been one), zerocopy goes on with it off
		const auto sent_before = pgather->zerocopy_sent_count();
		for (int i = 0; i < 8; ++i)
		{
			ASSERT_TRUE(lio_send(psocket.get(), index++, size)) << "send " << i;
			bytes_expected += size;
			ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == bytes_expected; }, 30000ms)) << "received " << psocket->bytes_received.load();
		}

		if (is_check)
			EXPECT_EQ(pgather->zerocopy_sent_count(), sent_before) << "zerocopy went on after the copy was reported";
		else
			EXPECT_GT(pgather->zerocopy_sent_count() + pgather->zerocopy_fallback_count(), sent_before) << "zerocopy stopped with the check off";

		EXPECT_TRUE(lio_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; })) << "pending " << pgather->zerocopy_pending_count();
		EXPECT_TRUE(psocket->is_connected.load());
		EXPECT_EQ(psocket->count_disconnect.load(), 0);

		psocket->stop();
		EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	}

	pacceptor->stop();
	pexecutor->stop();
}

TEST(linux_io, tcp_zerocopy_stops_when_copied_io_uring)
{
	lio_check_zerocopy_stops_when_copied(executor::io_backend::io_uring);
}

TEST(linux_io, tcp_zerocopy_stops_when_copied_io_uring_completion)
{
	lio_check_zerocopy_stops_when_copied(executor::io_backend::io_uring_completion);
}


//-----------------------------------------------------------------------------
// 3. UDP : recvmmsg / sendmmsg
//-----------------------------------------------------------------------------
TEST(linux_io, udp_echo_burst)
{
	lio_check_udp_echo(nullptr, 2000, 64);
}

TEST(linux_io, udp_echo_large_datagrams)
{
	lio_check_udp_echo(nullptr, 200, 8000);
}

TEST(linux_io, udp_send_deferred_in_group)
{
	// - the sends in a send_defer scope are queued and sent together (sendmmsg) at the end of the scope
	auto pserver = make_own<lio_udp_socket>();
	ASSERT_EQ(pserver->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);
	const auto port = pserver->local_endpoint().port();

	auto pclient = make_own<lio_udp_socket>();
	ASSERT_EQ(pclient->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);

	const net::sockaddr address_server{ net::ip::address_v4::loopback(), port };
	constexpr int count = 500;

	{
		send_defer::scope scope_defer;

		for (int i = 0; i < count; ++i)
			ASSERT_TRUE(pclient->send_to(lio_buffer(lio_message(static_cast<uint32_t>(i), 100)), address_server));
	}

	ASSERT_TRUE(lio_wait_until([&]() { return pserver->count_received.load() == count; }, 20000ms)) << "received " << pserver->count_received.load();

	{
		std::lock_guard lock(pserver->lockable);
		std::vector<uint32_t> indexes = pserver->indexes;
		std::sort(indexes.begin(), indexes.end());
		ASSERT_EQ(indexes.size(), static_cast<std::size_t>(count));
		for (int i = 0; i < count; ++i)
			ASSERT_EQ(indexes[i], static_cast<uint32_t>(i));
	}

	pclient->stop();
	pserver->stop();
}

TEST(linux_io, udp_receive_on_many_threads)
{
	//-----------------------------------------------------------------
	// one UDP socket received by the 16 threads of an epoll executor at once (EPOLLET : a datagram arriving while a thread drains the
	// socket wakes another). the batch of recvmmsg (Ndatagram::m_count_receive_batch) was a plain int : g++ stored its doubling in two
	// steps (the doubled count, then the limit 32) and a thread reading between them filled 64 entries of the arrays of 32 on its stack
	// ("stack smashing detected" : handover remaining 23). 8 threads flood it with sendmmsg (full batches : the batch doubles all the time)
	//-----------------------------------------------------------------
	class udp_counting_socket : public net::socket::udp
	{
	public:
				std::atomic<std::size_t> count_received{ 0 };
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { ++this->count_received; return eRESULT::DONE; }
	};

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::epoll);
	auto pexecutor = executor::make_io_context_threads("udp receive on many threads", 16);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());

	auto pserver = make_own<udp_counting_socket>();
	pserver->io_context(pexecutor.get());
	ASSERT_EQ(pserver->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);
	const auto port = pserver->local_endpoint().port();
	ASSERT_NE(port, 0);

	std::atomic<bool> is_running{ true };
	std::atomic<std::size_t> count_sent{ 0 };
	std::vector<std::thread> senders;

	for (int t = 0; t < 8; ++t)
	{
		senders.emplace_back([&]()
		{
			const int handle = ::socket(AF_INET, SOCK_DGRAM, 0);
			if (handle < 0)
				return;

			::sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

			char data[64]{};
			::iovec array_iovec[32];
			::mmsghdr array_message[32]{};
			for (int i = 0; i < 32; ++i)
			{
				array_iovec[i] = { data, sizeof(data) };
				array_message[i].msg_hdr.msg_name = &address;
				array_message[i].msg_hdr.msg_namelen = sizeof(address);
				array_message[i].msg_hdr.msg_iov = &array_iovec[i];
				array_message[i].msg_hdr.msg_iovlen = 1;
			}

			while (is_running.load(std::memory_order_relaxed))
			{
				const int count = ::sendmmsg(handle, array_message, 32, 0);
				if (count > 0)
					count_sent += static_cast<std::size_t>(count);
			}

			::close(handle);
		});
	}

	std::this_thread::sleep_for(3s);
	is_running = false;
	for (auto& iter : senders)
		iter.join();

	// - the process is alive (a broken stack aborts it) and the socket received
	EXPECT_TRUE(lio_wait_until([&]() { return pserver->count_received.load() != 0; })) << "sent " << count_sent.load();

	pserver->stop();
	pexecutor->stop();
}


//-----------------------------------------------------------------------------
// 4. acceptor options : cpu affinity, TCP_DEFER_ACCEPT
//-----------------------------------------------------------------------------
TEST(linux_io, acceptor_cpu_affinity)
{
	lio_check_tcp_echo(nullptr, false, true, 0, 16, 50, 100);
	lio_check_tcp_echo(nullptr, true, true, 0, 8, 50, 100);		// - UDS sockets too (SO_INCOMING_CPU of a UDS : -1 => the acceptor's io_context)

	// - the io_contexts of the CPUs are stopped (the destroy handler of the system_executor does it at the end too)
	net::io::connective::acceptor::Ntcp::stop_cpu_affinity_io_contexts();
}

TEST(linux_io, acceptor_tcp_defer_accept)
{
	lio_check_tcp_echo(nullptr, false, false, 2, 4, 20, 100);
}


//-----------------------------------------------------------------------------
// 5. io_uring backend
//-----------------------------------------------------------------------------
TEST(linux_io, io_uring_executor)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	// - the system executor (its ring with CGDK_IO_URING=1) is created before the count (by the first socket otherwise : the test alone counted it)
	static_cast<void>(system_executor::executor());
	const int count_fd_before = lio_count_io_uring_fd_settled();

	// 1) an executor with the io_uring backend (the option is read when the io_context is created)
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("io_uring executor", 4);
	executor::set_default_io_backend(backend_before);

	ASSERT_TRUE(pexecutor.exist());
	ASSERT_GT(lio_count_io_uring_fd(), count_fd_before) << "the executor does not use io_uring";

	// 2) TCP, UDS, large messages (EPOLLOUT), UDP on it
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 8, 100, 64);
	lio_check_tcp_echo(pexecutor.get(), true, false, 0, 8, 100, 64);
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 2, 16, 65536);
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 1, 8, 256 * 1024);
	lio_check_udp_echo(pexecutor.get(), 1000, 64);

	// 3) close at connect / stop while connecting (the fd of a closed socket is reused at once)
	{
		lio_uds_guard guard;
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

		auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
		pacceptor->io_context(pexecutor.get());
		ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
		const auto port = pacceptor->local_endpoint().port();

		for (int i = 0; i < 200; ++i)
		{
			auto psocket = make_own<lio_client_socket>();
			psocket->io_context(pexecutor.get());
			psocket->start(lio_connect_parameter(port));
			if (i % 2 == 0)
			{
				ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load() || psocket->count_fail_connect.load() != 0; }, 3000ms)) << "loop " << i;
			}
			psocket->stop();
		}

		EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
		pacceptor->stop();
	}

	// 4) stop the executor (the ring is destroyed with the threads waiting on it)
	pexecutor->stop();
	EXPECT_EQ(lio_count_io_uring_fd(), count_fd_before);
}

// - a request of io_uring belongs to the thread that submitted it : a poll that fires after that thread exited completes with
//   -ECANCELED. polled again (dropped before : the registration got no more events)
//   . an executor created by a thread that exits : its post eventfd was polled by that thread => no post was executed
TEST(linux_io, io_uring_poll_of_an_exited_thread)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	// 1) the executor (its ring and the poll of its post eventfd) created by a thread that exits
	object_ptr<executor::Iio_context_threads> pexecutor;
	std::thread([&]()
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(executor::io_backend::io_uring);
		pexecutor = executor::make_io_context_threads("io_uring executor of an exited thread", 2);
		executor::set_default_io_backend(backend_before);
	}).join();

	ASSERT_TRUE(pexecutor.exist());

	// 2) posts : executed
	std::atomic<int> count_executed{ 0 };

	for (int i = 0; i < 100; ++i)
		ASSERT_TRUE(pexecutor->post(make_executable([&count_executed] { ++count_executed; })));

	EXPECT_TRUE(lio_wait_until([&]() { return count_executed.load() == 100; })) << "executed " << count_executed.load();

	// 3) sockets on it
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 4, 50, 64);

	pexecutor->stop();
}

// - only the workers of an executor enter its ring : a request belongs to the thread that submitted it, and its completion runs as
//   the task work of that thread (TIF_NOTIFY_SIGNAL : a blocking call of it with a timeout returns EINTR). every thread that created a
//   ring or submitted to it gets a task work when the ring is released too
//   . before (io_uring readiness) : the thread that created the executor polled its post eventfd, the thread that started an acceptor
//     polled the listen socket, a thread that stopped it submitted the SQEs of the workers (and a nop). the posts and the accepts
//     interrupted it, and the release of the ring (and of the ring of is_available()) : emergency.* got EINTR in the blocking client of
//     a test when the executor of the test before was destroyed
//   . before (io_uring completion) : the thread that created the executor created its wake ring, a thread that staged on a ring or
//     stopped the executor woke the owner with an IORING_OP_MSG_RING submitted to it (the release of the wake ring)
namespace
{
void lio_check_requests_of_the_workers_only(executor::io_backend _backend)
{
	// - a blocking recv with a timeout on this thread (nothing comes : EAGAIN at the timeout. EINTR : interrupted)
	int fds[2] = { -1, -1 };
	ASSERT_EQ(::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds), 0);
	timeval timeout{ 0, 20 * 1000 };
	ASSERT_EQ(::setsockopt(fds[0], SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)), 0);

	int count_interrupted = 0;
	std::string text_interrupted;
	const auto time_start = std::chrono::steady_clock::now();
	const auto recv_blocking = [&]()
	{
		char c;
		if (::recv(fds[0], &c, 1, 0) < 0 && errno == EINTR)
		{
			++count_interrupted;
			text_interrupted += " " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - time_start).count()) + "ms";
		}
	};

	// 1) the executor and an acceptor on it, by this thread
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(_backend);
	auto pexecutor = executor::make_io_context_threads("io_uring executor (requests of the workers)", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), _backend);

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) another thread posts and connects while this one blocks
	std::atomic<int> count_executed{ 0 };
	std::atomic<int> count_connected{ 0 };
	std::atomic<bool> is_done{ false };
	std::thread thread_events([&]()
	{
		for (int i = 0; i < 20; ++i)
		{
			for (int j = 0; j < 100; ++j)
				pexecutor->post(make_executable([&count_executed] { ++count_executed; }));

			const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			if (fd >= 0 && ::connect(fd, reinterpret_cast<const ::sockaddr*>(&address), sizeof(address)) == 0)
				++count_connected;
			std::this_thread::sleep_for(5ms);
			if (fd >= 0)
				::close(fd);
		}
		is_done = true;
	});

	while (is_done == false)
		recv_blocking();
	thread_events.join();

	EXPECT_TRUE(lio_wait_until([&]() { return count_executed.load() == 2000; })) << "executed " << count_executed.load();
	EXPECT_EQ(count_connected.load(), 20);
	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();

	EXPECT_EQ(count_interrupted, 0) << "the blocking calls of this thread were interrupted by the completions of io_uring requests :" << text_interrupted;

	// 3) stopped by this thread : the ring is released (the kernel sends a task work to the threads that created it or submitted to it), then
	//    this thread blocks again
	count_interrupted = 0;
	text_interrupted.clear();

	pacceptor->stop();
	pexecutor->stop();
	pexecutor.reset();

	for (int i = 0; i < 15; ++i)
		recv_blocking();

	EXPECT_EQ(count_interrupted, 0) << "the blocking calls of this thread were interrupted when the ring was released :" << text_interrupted;

	::close(fds[0]);
	::close(fds[1]);
}
}

TEST(linux_io, io_uring_requests_of_the_workers_only)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	lio_check_requests_of_the_workers_only(executor::io_backend::io_uring);
}

TEST(linux_io, io_uring_completion_requests_of_the_workers_only)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	lio_check_requests_of_the_workers_only(executor::io_backend::io_uring_completion);
}

// - the io_uring completion backend : the TCP sockets, the acceptor and the connectors do their I/O with the ops (RECV/SEND/ACCEPT/CONNECT)
//   . no readiness registration is left for them (attached_count() : the UDP sockets register, released at their close)
//   . a large message goes as a SEND_ZC op (zerocopy_sent_count)
TEST(linux_io, io_uring_completion_sockets)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	// 1) an executor with the completion backend
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring_completion);
	auto pexecutor = executor::make_io_context_threads("io_uring completion executor", 4);
	executor::set_default_io_backend(backend_before);

	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);
	EXPECT_EQ(pexecutor->attached_count(), 0u);

	// 2) TCP, UDS, large messages (SEND ops), many small ones
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 8, 100, 64);
	lio_check_tcp_echo(pexecutor.get(), true, false, 0, 8, 100, 64);
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 2, 16, 65536);
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 1, 8, 256 * 1024);
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 32, 200, 16);
	EXPECT_EQ(pexecutor->attached_count(), 0u) << "a readiness registration is left for a completion socket";

	// 3) UDP stays on readiness (registered while open)
	lio_check_udp_echo(pexecutor.get(), 1000, 64);
	EXPECT_EQ(pexecutor->attached_count(), 0u);

	// 4) zerocopy : the large sends of a client go as SEND_ZC ops (the owner is held till the notification)
	{
		lio_uds_guard guard;
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

		auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
		pacceptor->io_context(pexecutor.get());
		ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
		const auto port = pacceptor->local_endpoint().port();

		auto psocket = make_own<lio_client_socket>();
		psocket->keep_received = false;
		psocket->io_context(pexecutor.get());
		psocket->start(lio_connect_parameter(port));
		ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));

		constexpr int count = 16;
		constexpr std::size_t size = 256 * 1024;
		for (int i = 0; i < count; ++i)
			ASSERT_TRUE(lio_send(psocket.get(), static_cast<uint32_t>(i), size)) << "send " << i;
		ASSERT_TRUE(lio_wait_until([&]() { return psocket->bytes_received.load() == count * size; }, 30000ms)) << "received " << psocket->bytes_received.load();

		auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(psocket.get());
		ASSERT_NE(pgather, nullptr);
		EXPECT_GT(pgather->zerocopy_sent_count(), 0u);
		EXPECT_TRUE(psocket->is_connected.load());

		psocket->stop();
		EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
		pacceptor->stop();
	}

	// 5) close at connect / stop while connecting (the CONNECT op is cancelled, the fd number is reused at once)
	{
		lio_uds_guard guard;
		net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

		auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
		pacceptor->io_context(pexecutor.get());
		ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
		const auto port = pacceptor->local_endpoint().port();

		for (int i = 0; i < 200; ++i)
		{
			auto psocket = make_own<lio_client_socket>();
			psocket->io_context(pexecutor.get());
			psocket->start(lio_connect_parameter(port));
			if (i % 2 == 0)
			{
				ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load() || psocket->count_fail_connect.load() != 0; }, 3000ms)) << "loop " << i;
			}
			psocket->stop();
		}

		EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
		pacceptor->stop();
	}

	EXPECT_EQ(pexecutor->attached_count(), 0u);

	// 6) stop
	pexecutor->stop();
}

//-----------------------------------------------------------------------------
// the receive mode of a completion socket (IOCP like) : small messages => the multishot into the provided buffers, large ones =>
// a RECV into the receive buffer that grows (1KB, 8KB, 64KB, the message size), small ones in a row => the multishot again.
// every message intact and in order through the switches
//-----------------------------------------------------------------------------
TEST(linux_io, io_uring_completion_receive_mode)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring_completion);
	auto pexecutor = executor::make_io_context_threads("io_uring completion receive mode", 4);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);

	if (pexecutor->provided_buffer_size() == 0)
	{
		pexecutor->stop();
		GTEST_SKIP() << "no provided buffers";
	}

	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(false);

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto psocket = make_own<lio_client_socket>();
	psocket->io_context(pexecutor.get());
	psocket->start(lio_connect_parameter(port));
	ASSERT_TRUE(lio_wait_until([&]() { return psocket->is_connected.load(); }));
	EXPECT_TRUE(psocket->is_receive_multishot_mode());

	std::vector<std::string> sent;
	auto round_trip = [&](std::size_t _size) -> bool
	{
		const auto index = static_cast<uint32_t>(sent.size());
		sent.push_back(lio_message(index, _size));
		if (lio_send(psocket.get(), index, _size) == false)
			return false;
		return lio_wait_until([&]() { return psocket->count_received.load() == sent.size(); }, 10000ms);
	};

	// 1) small messages one by one : the multishot
	for (int i = 0; i < 20; ++i)
		ASSERT_TRUE(round_trip(64)) << "small " << i;
	EXPECT_TRUE(psocket->is_receive_multishot_mode());

	// 2) large messages : the RECV mode (an arrival filled a provided buffer, or a message larger than one is waiting)
	for (int i = 0; i < 8; ++i)
		ASSERT_TRUE(round_trip((i % 2 == 0) ? 65536 : 256 * 1024)) << "large " << i;
	EXPECT_FALSE(psocket->is_receive_multishot_mode());

	// 3) small messages one by one again : the multishot after the small RECVs in a row
	for (int i = 0; i < 40; ++i)
		ASSERT_TRUE(round_trip(64)) << "small again " << i;
	EXPECT_TRUE(psocket->is_receive_multishot_mode());

	// 4) large and small mixed in a pipeline (the switches while data is in flight)
	{
		const auto count_before = sent.size();
		for (int i = 0; i < 64; ++i)
		{
			const std::size_t size = (i % 8 == 7) ? 100 * 1024 : (i % 3 == 0) ? 3000 : 64;
			const auto index = static_cast<uint32_t>(sent.size());
			sent.push_back(lio_message(index, size));
			ASSERT_TRUE(lio_send(psocket.get(), index, size)) << "mixed " << i;
		}
		ASSERT_TRUE(lio_wait_until([&]() { return psocket->count_received.load() == sent.size(); }, 20000ms)) << "received " << psocket->count_received.load() << " / " << sent.size() << " (before " << count_before << ")";
	}

	// - every message intact and in order
	{
		std::lock_guard lock(psocket->lockable);
		ASSERT_EQ(psocket->received.size(), sent.size());
		for (std::size_t i = 0; i < sent.size(); ++i)
			ASSERT_TRUE(psocket->received[i] == sent[i]) << "message " << i << " (size " << sent[i].size() << ")";
	}

	psocket->stop();
	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

//-----------------------------------------------------------------------------
// the sockets accepted on a completion io_context spread over its workers : an accepted socket is bound to a ring of round robin at
// the CQE of the accept. bound by its first submit (the RECV posted by the accept handler) every socket of an acceptor went to the
// ring of the listen socket : one worker ran them all while the others idled (2026-10-24 : 64 sockets on 1 of 4 threads). a socket
// runs on the owner of its ring : one thread per socket
// the connecting sockets (the TCP clients) too : bound round robin by submit_connect. bound by the CONNECT op instead, a socket went to
// the ring of the worker running its connect post (the posts are taken up to 16 at a time by the ring woken : a few threads)
//-----------------------------------------------------------------------------
class lio_thread_server_socket : public net::socket::tcp<>
{
public:
	inline static std::mutex	s_lockable;
	inline static std::map<const void*, std::set<std::thread::id>> s_threads;	// - the threads that ran the handlers of a socket

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		{
			std::lock_guard lock(s_lockable);
			s_threads[this].insert(std::this_thread::get_id());
		}

		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

// - the accepted sockets on the io_context of the acceptor (what the linux acceptor does by itself since r11489 : before, a socket without
//   an io_context got the system executor)
class lio_thread_acceptor : public net::acceptor<lio_thread_server_socket>
{
protected:
	[[nodiscard]] virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable* /*_pconnectable*/) override { return this->io_context(); }
};

// - a client recording the threads that ran its handlers
class lio_thread_client_socket : public lio_client_socket
{
public:
	inline static std::mutex	s_lockable;
	inline static std::map<const void*, std::set<std::thread::id>> s_threads;

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		{
			std::lock_guard lock(s_lockable);
			s_threads[this].insert(std::this_thread::get_id());
		}

		return lio_client_socket::on_message(_msg);
	}
};

// - the sockets per thread of a map of the threads per socket : (sockets, the most sockets on a thread, the sockets on several threads)
struct lio_spread
{
	std::size_t					count_socket = 0;
	int							max_sockets = 0;
	int							count_socket_on_threads = 0;
	std::map<std::thread::id, int> sockets_of_thread;
};

lio_spread lio_spread_of(std::mutex& _lockable, const std::map<const void*, std::set<std::thread::id>>& _threads)
{
	lio_spread result;

	std::lock_guard lock(_lockable);
	result.count_socket = _threads.size();
	for (const auto& [psocket, threads] : _threads)
	{
		if (threads.size() != 1)
			++result.count_socket_on_threads;
		for (const auto& id : threads)
			++result.sockets_of_thread[id];
	}

	for (const auto& [id, count] : result.sockets_of_thread)
		result.max_sockets = std::max(result.max_sockets, count);

	return result;
}

void lio_check_accepted_sockets_spread(executor::Iio_context* _pio_context, int _count_thread, bool _uds)
{
	constexpr int count_client = 64;
	constexpr int count_message = 4;
	constexpr std::size_t size = 64;

	lio_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(_uds);

	{
		std::lock_guard lock(lio_thread_server_socket::s_lockable);
		lio_thread_server_socket::s_threads.clear();
	}
	{
		std::lock_guard lock(lio_thread_client_socket::s_lockable);
		lio_thread_client_socket::s_threads.clear();
	}

	// 1) the acceptor and the clients on the io_context
	auto pacceptor = make_own<lio_thread_acceptor>();
	pacceptor->io_context(_pio_context);
	auto parameter = lio_acceptor_parameter(lio_port());
	parameter.enable_uds = _uds;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	std::vector<own_ptr<lio_thread_client_socket>> clients;
	for (int i = 0; i < count_client; ++i)
	{
		auto psocket = make_own<lio_thread_client_socket>();
		psocket->keep_received = false;
		psocket->io_context(_pio_context);
		psocket->start(lio_connect_parameter(port));
		clients.push_back(std::move(psocket));
	}
	ASSERT_TRUE(lio_wait_until([&]() { return std::all_of(clients.begin(), clients.end(), [](auto& _p) { return _p->is_connected.load(); }); }, 10000ms)) << "connect";
	ASSERT_TRUE(lio_wait_until([&]() { return pacceptor->count() == static_cast<std::size_t>(count_client); })) << "accepted " << pacceptor->count();

	// 2) messages : the handlers of every server socket run
	for (int m = 0; m < count_message; ++m)
		for (int i = 0; i < count_client; ++i)
			ASSERT_TRUE(lio_send(clients[i].get(), static_cast<uint32_t>(i * 100 + m), size)) << "send " << i << "/" << m;
	ASSERT_TRUE(lio_wait_until([&]() { return std::all_of(clients.begin(), clients.end(), [&](auto& _p) { return _p->bytes_received.load() == count_message * size; }); }, 10000ms)) << "received " << clients[0]->bytes_received.load();

	// 3) the threads of the server sockets and of the clients : every worker runs a share (round robin : count_client / _count_thread each),
	//    a socket one thread
	const auto server = lio_spread_of(lio_thread_server_socket::s_lockable, lio_thread_server_socket::s_threads);
	const auto client = lio_spread_of(lio_thread_client_socket::s_lockable, lio_thread_client_socket::s_threads);

	EXPECT_EQ(server.count_socket, static_cast<std::size_t>(count_client));
	EXPECT_EQ(server.sockets_of_thread.size(), static_cast<std::size_t>(_count_thread)) << "the server sockets ran on " << server.sockets_of_thread.size() << " of " << _count_thread << " workers (max " << server.max_sockets << " sockets on one, uds " << _uds << ")";
	EXPECT_LE(server.max_sockets, count_client / _count_thread + 2) << "uds " << _uds;
	EXPECT_EQ(server.count_socket_on_threads, 0) << "a server socket ran on several threads (uds " << _uds << ")";

	// - the clients : round robin over the workers by the CONNECT op (TCP). a UDS connects at once (no op : its completion is posted) :
	//   it goes to the ring of the worker running that post (the posts spread over the idle workers : not asserted)
	EXPECT_EQ(client.count_socket, static_cast<std::size_t>(count_client));
	EXPECT_EQ(client.count_socket_on_threads, 0) << "a client ran on several threads (uds " << _uds << ")";
	if (_uds == false)
	{
		EXPECT_EQ(client.sockets_of_thread.size(), static_cast<std::size_t>(_count_thread)) << "the clients ran on " << client.sockets_of_thread.size() << " of " << _count_thread << " workers (max " << client.max_sockets << " sockets on one)";
		EXPECT_LE(client.max_sockets, count_client / _count_thread + 2);
	}

	// 4) close
	for (auto& psocket : clients)
		psocket->stop();
	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
}

TEST(linux_io, io_uring_completion_accepted_sockets_spread)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	// 1) an executor with the completion backend (the rings of its workers created : the io_uring fds settled)
	constexpr int count_thread = 4;

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring_completion);
	auto pexecutor = executor::make_io_context_threads("io_uring completion accepted sockets", count_thread);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);
	lio_count_io_uring_fd_settled();

	// 2) TCP, UDS (the UDS listen socket of the acceptor : an accept op of its own)
	lio_check_accepted_sockets_spread(pexecutor.get(), count_thread, false);
	lio_check_accepted_sockets_spread(pexecutor.get(), count_thread, true);

	pexecutor->stop();
}

//-----------------------------------------------------------------------------
// io_uring completion : the multishot accept that ends while the acceptor is disabled is posted again
//
//  - before : the final CQE (no F_MORE) of the accept op was not posted again while disabled, and enable() posts nothing : the
//    connections stayed in the backlog for good. the op is ended here by a cancel of the ops of the listen fd (-ECANCELED : what
//    the exit of the submitting thread, an error of accept or a full CQ give)
//  - a connection while disabled is closed by the acceptor (the op runs), one after enable() is accepted
//-----------------------------------------------------------------------------
namespace
{
	// - a blocking client of the OS : connected (to the backlog of the listen socket at least), -1 : failed
	int lio_raw_connect(uint16_t _port)
	{
		const int handle = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if (handle < 0)
			return -1;

		timeval timeout{ 0, 100 * 1000 };
		::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(_port);
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		if (::connect(handle, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
		{
			::close(handle);
			return -1;
		}

		return handle;
	}

	// - the peer closed the connection (EOF or RST). a timeout of the recv : not closed (yet)
	bool lio_raw_is_closed(int _handle)
	{
		char byte;
		const auto result = ::recv(_handle, &byte, 1, 0);
		return result == 0 || (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR);
	}
}

TEST(linux_io, io_uring_completion_accept_ended_while_disabled)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring_completion);
	auto pexecutor = executor::make_io_context_threads("io_uring completion accept disabled", 2);
	executor::set_default_io_backend(backend_before);
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);

	auto pacceptor = make_own<net::acceptor<lio_echo_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();
	const auto handle_listen = pacceptor->native_handle();
	ASSERT_NE(handle_listen, INVALID_SOCKET);

	// 1) disabled, then the accept op ends (its ops cancelled : the final CQE -ECANCELED)
	pacceptor->disable();
	ASSERT_TRUE(pacceptor->is_disable());
	ASSERT_TRUE(pexecutor->submit_cancel_fd(static_cast<CGHANDLE>(handle_listen))) << "no op of the listen socket";

	// 2) a connection while disabled : accepted by the op posted again and closed (not left in the backlog)
	const int handle_disabled = lio_raw_connect(port);
	ASSERT_GE(handle_disabled, 0) << "connect while disabled";
	const bool is_closed = lio_wait_until([&]() { return lio_raw_is_closed(handle_disabled); }, 3000ms);
	::close(handle_disabled);
	EXPECT_TRUE(is_closed) << "the connections stay in the backlog : the accept op was not posted again while disabled";
	EXPECT_EQ(pacceptor->count(), 0u);

	// 3) enabled : a connection is accepted
	pacceptor->enable();
	const int handle_after = lio_raw_connect(port);
	ASSERT_GE(handle_after, 0);
	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 1; })) << "accepted " << pacceptor->count();
	EXPECT_FALSE(lio_raw_is_closed(handle_after)) << "the connection after enable() was closed";
	::close(handle_after);

	EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(linux_io, io_uring_default_backend_option)
{
	// - the option is kept until changed (the built-in default is io_uring_completion, epoll where it is not available)
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	EXPECT_EQ(executor::default_io_backend(), executor::io_backend::io_uring);
	executor::set_default_io_backend(executor::io_backend::epoll);
	EXPECT_EQ(executor::default_io_backend(), executor::io_backend::epoll);
	executor::set_default_io_backend(backend_before);
}


//-----------------------------------------------------------------------------
// 6. busy poll of epoll_wait() (EPIOCSPARAMS, kernel 6.9+)
//
//  - set per epoll instance : START_PARAMETER / context "busy_poll" of an io_context_threads, Iio_context::set_busy_poll(),
//    or executor::set_default_busy_poll() for every epoll io_context created afterwards. read back by EPIOCGPARAMS.
//  - loopback sockets have no NAPI ID : the kernel does not spin for them (the echo only checks nothing breaks with it set)
//-----------------------------------------------------------------------------
namespace
{
	// - EPIOCGPARAMS by hand : what the kernel holds for the epoll of an io_context
	struct lio_epoll_params { uint32_t busy_poll_usecs; uint16_t busy_poll_budget; uint8_t prefer_busy_poll; uint8_t pad; };

	bool lio_get_epoll_params(executor::Iio_context* _pio_context, lio_epoll_params& _params)
	{
		_params = {};
		return ::ioctl(static_cast<int>(_pio_context->handle()), _IOR(0x8A, 0x02, lio_epoll_params), &_params) == 0;
	}

	// - an epoll executor (whatever the default backend is) : make_io_context_threads(name, threads, cpu, busy_poll_usecs)
	object_ptr<executor::Iio_context_threads> lio_make_epoll_executor(std::string_view _name, int _thread_count, uint32_t _busy_poll_usecs)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(executor::io_backend::epoll);
		auto pexecutor = executor::make_io_context_threads(_name, _thread_count, -1, _busy_poll_usecs);
		executor::set_default_io_backend(backend_before);
		return pexecutor;
	}
}

TEST(linux_io, epoll_busy_poll_parameter)
{
	if (executor::is_busy_poll_available() == false)
		GTEST_SKIP() << "EPIOCSPARAMS is not available on this kernel (6.9+)";

	// 1) make_io_context_threads(..., busy_poll_usecs) => the epoll of the executor (EPIOCGPARAMS by hand too)
	auto pexecutor = lio_make_epoll_executor("busy poll executor", 2, 50);

	uint32_t usecs = 0; uint16_t budget = 0; bool prefer = false;
	ASSERT_TRUE(pexecutor->get_busy_poll(usecs, budget, prefer));
	EXPECT_EQ(usecs, 50u);
	EXPECT_EQ(budget, 0u);
	EXPECT_FALSE(prefer);

	lio_epoll_params params;
	ASSERT_TRUE(lio_get_epoll_params(pexecutor.get(), params));
	EXPECT_EQ(params.busy_poll_usecs, 50u);

	// 2) set_busy_poll() after the start : budget and prefer too, off again
	ASSERT_TRUE(pexecutor->set_busy_poll(20, 16, true));
	ASSERT_TRUE(lio_get_epoll_params(pexecutor.get(), params));
	EXPECT_EQ(params.busy_poll_usecs, 20u);
	EXPECT_EQ(params.busy_poll_budget, 16u);
	EXPECT_EQ(params.prefer_busy_poll, 1u);
	ASSERT_TRUE(pexecutor->get_busy_poll(usecs, budget, prefer));
	EXPECT_EQ(budget, 16u);
	EXPECT_TRUE(prefer);

	ASSERT_TRUE(pexecutor->set_busy_poll(0));
	ASSERT_TRUE(pexecutor->get_busy_poll(usecs, budget, prefer));
	EXPECT_EQ(usecs, 0u);

	// 3) sockets work with it set (loopback : no NAPI ID, the kernel does not spin)
	ASSERT_TRUE(pexecutor->set_busy_poll(50));
	lio_check_tcp_echo(pexecutor.get(), false, false, 0, 4, 50, 64);
	pexecutor->stop();

	// 4) context : "busy_poll" (microseconds), "busy_poll_budget", "prefer_busy_poll"
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(executor::io_backend::epoll);
		context c;
		c["thread_count"] = 1;
		c["busy_poll"] = 30;
		c["busy_poll_budget"] = 8;
		c["prefer_busy_poll"] = "true";
		auto pexecutor_context = executor::make_io_context_threads("busy poll executor (context)", c);
		executor::set_default_io_backend(backend_before);

		ASSERT_TRUE(lio_get_epoll_params(pexecutor_context.get(), params));
		EXPECT_EQ(params.busy_poll_usecs, 30u);
		EXPECT_EQ(params.busy_poll_budget, 8u);
		EXPECT_EQ(params.prefer_busy_poll, 1u);
		pexecutor_context->stop();
	}

	// 5) an executor without the parameter : off
	auto pexecutor_default = lio_make_epoll_executor("no busy poll executor", 1, 0);
	ASSERT_TRUE(pexecutor_default->get_busy_poll(usecs, budget, prefer));
	EXPECT_EQ(usecs, 0u);
	pexecutor_default->stop();
}

TEST(linux_io, epoll_busy_poll_default)
{
	if (executor::is_busy_poll_available() == false)
		GTEST_SKIP() << "EPIOCSPARAMS is not available on this kernel (6.9+)";

	uint32_t usecs_before = 0; uint16_t budget_before = 0; bool prefer_before = false;
	executor::get_default_busy_poll(usecs_before, budget_before, prefer_before);

	// 1) the default : every epoll io_context created afterwards
	executor::set_default_busy_poll(40, 4, false);

	uint32_t usecs = 0; uint16_t budget = 0; bool prefer = true;
	executor::get_default_busy_poll(usecs, budget, prefer);
	EXPECT_EQ(usecs, 40u);
	EXPECT_EQ(budget, 4u);
	EXPECT_FALSE(prefer);

	auto pexecutor = lio_make_epoll_executor("default busy poll executor", 1, 0);
	lio_epoll_params params;
	ASSERT_TRUE(lio_get_epoll_params(pexecutor.get(), params));
	EXPECT_EQ(params.busy_poll_usecs, 40u);
	EXPECT_EQ(params.busy_poll_budget, 4u);

	// 2) an own value of the executor replaces the default
	auto pexecutor_own = lio_make_epoll_executor("own busy poll executor", 1, 10);
	ASSERT_TRUE(lio_get_epoll_params(pexecutor_own.get(), params));
	EXPECT_EQ(params.busy_poll_usecs, 10u);

	// 3) the per CPU io_contexts of an acceptor (cpu_affinity) get the default too (they are created with the default backend : epoll only)
	//    (created on demand : the ones created in an earlier test keep their value => stopped first, a new one is asked)
	net::io::connective::acceptor::Ntcp::stop_cpu_affinity_io_contexts();
	if (auto pio_context_cpu = net::io::connective::acceptor::Ntcp::cpu_affinity_io_context(0); pio_context_cpu.exist() && pio_context_cpu->backend() == executor::io_backend::epoll)
	{
		EXPECT_TRUE(lio_get_epoll_params(pio_context_cpu.get(), params));
		EXPECT_EQ(params.busy_poll_usecs, 40u);
	}
	net::io::connective::acceptor::Ntcp::stop_cpu_affinity_io_contexts();

	// 4) off again : the next one has none
	executor::set_default_busy_poll(0);
	auto pexecutor_off = lio_make_epoll_executor("busy poll off executor", 1, 0);
	ASSERT_TRUE(lio_get_epoll_params(pexecutor_off.get(), params));
	EXPECT_EQ(params.busy_poll_usecs, 0u);

	pexecutor->stop();
	pexecutor_own->stop();
	pexecutor_off->stop();
	executor::set_default_busy_poll(usecs_before, budget_before, prefer_before);
}

TEST(linux_io, io_uring_has_no_busy_poll)
{
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	// - the io_uring backend : set_busy_poll() is not supported (false), the parameter is warned and ignored
	const auto backend_before = executor::default_io_backend();
	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("io_uring executor (busy poll ignored)", 1, -1, 50);
	executor::set_default_io_backend(backend_before);

	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring);
	EXPECT_FALSE(pexecutor->set_busy_poll(50));
	uint32_t usecs = 1; uint16_t budget = 0; bool prefer = false;
	EXPECT_FALSE(pexecutor->get_busy_poll(usecs, budget, prefer));
	EXPECT_EQ(usecs, 0u);
	pexecutor->stop();
}


//-----------------------------------------------------------------------------
// 7. send cork : the sends of the thread processing the received messages are deferred till the end of the batch, or till the oldest
//    one waited SEND_CORK_DURATION (200us. checked at the sends : at every send when the handler is slow, every 32 when it is fast)
//    - 24 messages of 32B in one buffer (in the first receive buffer of 1KB : one batch), 100us of work after every echo : a batch of
//      ~2.4ms. the responses are sent in several sends while it is processed, not all at its end
//    - checked on the server : the bytes it sent (handed to the kernel : statistics_get_sent_bytes()) after each echo. not by the
//      arrival times of the client : the client socket may run on the thread of the server socket (io_uring completion : a ring runs
//      its completions on its owner thread only, the client bound to the ring of the server waited for the end of the batch while
//      the server had sent during it. the arrival times came at once : 13~105us)
//-----------------------------------------------------------------------------
namespace
{
	// - the bytes the server socket sent after each echo of the batch
	std::mutex				g_lio_cork_lockable;
	std::vector<uint64_t>	g_lio_cork_sent;

	template <class TSOCKET>
	class lio_slow_echo_server_socket : public TSOCKET
	{
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			this->send(_msg.buf_message);

			{
				std::lock_guard lock(g_lio_cork_lockable);
				g_lio_cork_sent.push_back(this->statistics_get_sent_bytes());
			}

			// - work after the send (the response is deferred meanwhile)
			const auto time_end = std::chrono::steady_clock::now() + 100us;
			while (std::chrono::steady_clock::now() < time_end) {}

			return eRESULT::DONE;
		}
	};

	class lio_timed_client_socket : public net::socket::tcp<>, public net::io::connect_requestable::tcp::Ndefault
	{
	public:
				std::atomic<bool>	is_connected{ false };
				std::mutex			lockable;
				std::vector<uint32_t> indexes;
				std::vector<std::chrono::steady_clock::time_point> times;

	protected:
		virtual void				on_connect() override { this->is_connected = true; }
		virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; }
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			std::lock_guard lock(this->lockable);
			this->indexes.push_back(lio_index(reinterpret_cast<const char*>(_msg.buf_message.data())));
			this->times.push_back(std::chrono::steady_clock::now());
			return eRESULT::DONE;
		}
	};

	// - _pio_context : the executor of the server and the client (nullptr : the system executor)
	template <class TSOCKET>
	void lio_check_send_cork_old_sends(executor::Iio_context* _pio_context = nullptr)
	{
		constexpr std::size_t count_message = 24;

		{
			std::lock_guard lock(g_lio_cork_lockable);
			g_lio_cork_sent.clear();
		}

		// 1) server
		auto pacceptor = make_own<net::acceptor<lio_slow_echo_server_socket<TSOCKET>>>();
		if (_pio_context != nullptr)
			pacceptor->io_context(_pio_context);
		ASSERT_EQ(pacceptor->start(lio_acceptor_parameter(lio_port())), eRESULT::SUCCESS);

		// 2) client
		auto pclient = make_own<lio_timed_client_socket>();
		if (_pio_context != nullptr)
			pclient->io_context(_pio_context);
		pclient->start(lio_connect_parameter(pacceptor->local_endpoint().port()));
		ASSERT_TRUE(lio_wait_until([&]() { return pclient->is_connected.load(); }, 10000ms)) << "connect";

		// 3) the messages in one buffer
		std::string messages;
		for (std::size_t i = 0; i < count_message; ++i)
			messages += lio_message(static_cast<uint32_t>(i), 32);
		ASSERT_TRUE(pclient->send(lio_buffer(messages)));

		ASSERT_TRUE(lio_wait_until([&]() { std::lock_guard lock(pclient->lockable); return pclient->indexes.size() == count_message; }, 10000ms)) << "echo";

		// 4) in order
		{
			std::lock_guard lock(pclient->lockable);
			for (std::size_t i = 0; i < count_message; ++i)
				ASSERT_EQ(pclient->indexes[i], static_cast<uint32_t>(i)) << "message " << i;
		}

		// 5) sent while the batch was processed : several sends before its last echo, a part of the responses before its middle
		//    (the old responses sent at every ~3 echoes : 200us / 100us. none before the end : the cork waited for the end of the batch)
		{
			std::lock_guard lock(g_lio_cork_lockable);
			ASSERT_EQ(g_lio_cork_sent.size(), count_message);

			std::size_t count_sends = 0;
			std::string text_sent;
			for (std::size_t i = 0; i < count_message; ++i)
			{
				if (i + 1 < count_message && g_lio_cork_sent[i] != ((i == 0) ? 0 : g_lio_cork_sent[i - 1]))
					++count_sends;
				text_sent += " " + std::to_string(g_lio_cork_sent[i]);
			}

			EXPECT_GE(count_sends, 4u) << "the responses were not sent while the batch was processed (the bytes sent after each echo :" << text_sent << ")";
			EXPECT_GE(g_lio_cork_sent[count_message / 2 - 1], 3u * 32u) << "the bytes sent after each echo :" << text_sent;
		}

		// 6) close
		pclient->stop();
		EXPECT_TRUE(lio_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
		pacceptor->stop();
	}
}

TEST(linux_io, send_cork_sends_old_responses_gather)
{
	lio_check_send_cork_old_sends<net::socket::tcp<>>();
}

TEST(linux_io, send_cork_sends_old_responses_buffered)
{
	lio_check_send_cork_old_sends<net::socket::tcp_buffered<>>();
}

// - the server and the client on one thread (an executor of 1 thread : the client runs after the batch of the server, its arrival times
//   come at once) : the server sent while it processed the batch all the same
TEST(linux_io, send_cork_sends_old_responses_one_thread)
{
	auto pexecutor = executor::make_io_context_threads("send cork executor (1 thread)", 1);
	ASSERT_TRUE(pexecutor.exist());

	lio_check_send_cork_old_sends<net::socket::tcp<>>(pexecutor.get());
	lio_check_send_cork_old_sends<net::socket::tcp_buffered<>>(pexecutor.get());

	pexecutor->stop();
}

}

#endif
