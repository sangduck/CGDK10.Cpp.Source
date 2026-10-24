#include "pch.h"
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <thread>

//-----------------------------------------------------------------------------
// a handler that blocks (scoped_thread_block : a sync query ...) does not hold the other sockets
//
//  - epoll / io_uring (readiness) : the other threads take the events.
//  - io_uring completion : a ring runs on its owner thread. while the owner blocks, its ring is lent : the other workers run its
//    completions (api::io_context_uring_cq::thread_blocked). the results of the executable the owner blocked in wait for it (in order).
//
//  the test runs on the executor of the process (the backend of the run : CGDK_IO_URING=io_uring / completion). 200 connections :
//  the rings of the system executor are shared by several of them (the sockets on the ring of the blocked one are the case). completion :
//  the connections spread over the rings (the accepted sockets are bound round robin : not all on the ring of the listen socket).
//  a busy handler (CPU in user space) is not covered : completion runs the completions of its ring after it (COOP_TASKRUN).
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/syscall.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - the port of the tests (27900 + pid % 300 : below the ephemeral range, not the ports of the other tests)
	uint16_t bh_port()
	{
		return static_cast<uint16_t>(27900 + api::get_process_id() % 300);
	}

	long bh_tid()
	{
		return static_cast<long>(::syscall(SYS_gettid));
	}

	// - [uint32 size = 12][char kind][3 pad][uint32 index] : 'E' echo, 'S' blocks 300ms in a scoped_thread_block then echoes
	constexpr uint32_t BH_SIZE = 12;

	std::mutex				g_bh_lockable;
	std::map<uint16_t, long> g_bh_tid_of_port;		// - the peer port of a connection => the thread that ran its last handler

	class bh_server_socket : public net::socket::tcp<>
	{
	protected:
		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			{
				std::lock_guard lock(g_bh_lockable);
				g_bh_tid_of_port[this->remote_endpoint().port()] = bh_tid();
			}

			const char kind = (_msg.buf_message.size() >= BH_SIZE) ? reinterpret_cast<const char*>(_msg.buf_message.data())[4] : 'E';

			if (kind == 'S')
			{
				scoped_thread_block block;
				std::this_thread::sleep_for(300ms);
			}

			this->send(_msg.buf_message);
			return eRESULT::DONE;
		}
	};

	// - a blocking client of the OS
	class bh_client
	{
	public:
		~bh_client() { if (this->m_fd >= 0) ::close(this->m_fd); }

		bool connect(uint16_t _port)
		{
			this->m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(_port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			if (::connect(this->m_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0)
				return false;
			const int one = 1;
			::setsockopt(this->m_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
			const timeval timeout{ 5, 0 };
			::setsockopt(this->m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			return true;
		}

		uint16_t local_port() const
		{
			sockaddr_in address{};
			socklen_t length = sizeof(address);
			::getsockname(this->m_fd, reinterpret_cast<sockaddr*>(&address), &length);
			return ntohs(address.sin_port);
		}

		bool send(char _kind, uint32_t _index)
		{
			char message[BH_SIZE]{};
			const uint32_t size = BH_SIZE;
			memcpy(message, &size, 4);
			message[4] = _kind;
			memcpy(message + 8, &_index, 4);
			return ::send(this->m_fd, message, BH_SIZE, MSG_NOSIGNAL) == static_cast<ssize_t>(BH_SIZE);
		}

		// - the index of the next echo (-1 : timeout, closed. EINTR : again)
		int64_t receive()
		{
			char message[BH_SIZE];
			std::size_t received = 0;
			while (received < BH_SIZE)
			{
				const auto result = ::recv(this->m_fd, message + received, BH_SIZE - received, 0);
				if (result < 0 && errno == EINTR)
					continue;
				if (result <= 0)
					return -1;
				received += static_cast<std::size_t>(result);
			}
			uint32_t index = 0;
			memcpy(&index, message + 8, 4);
			return index;
		}

	private:
		int m_fd = -1;
	};

	long bh_tid_of(const bh_client& _client)
	{
		std::lock_guard lock(g_bh_lockable);
		const auto iter = g_bh_tid_of_port.find(_client.local_port());
		return (iter != g_bh_tid_of_port.end()) ? iter->second : 0;
	}
}

TEST(blocking_handler, does_not_hold_the_other_sockets)
{
	{
		std::lock_guard lock(g_bh_lockable);
		g_bh_tid_of_port.clear();
	}

	// 1) server (the executor of the process) and 200 connections
	auto pacceptor = make_own<net::acceptor<bh_server_socket>>();
	net::acceptor<bh_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), bh_port() };
	parameter.enable_progress_log = false;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	constexpr int count = 200;
	std::vector<std::unique_ptr<bh_client>> clients;
	for (int i = 0; i < count; ++i)
	{
		clients.push_back(std::make_unique<bh_client>());
		ASSERT_TRUE(clients.back()->connect(port)) << "connect " << i;
	}

	// - an echo each : the thread of each connection (completion : the owner of its ring)
	for (int i = 0; i < count; ++i)
	{
		ASSERT_TRUE(clients[i]->send('E', 0));
		ASSERT_EQ(clients[i]->receive(), 0) << "client " << i;
	}

	const long tid_blocked = bh_tid_of(*clients[0]);
	int count_same_thread = 0;
	int index_same = -1;
	std::set<long> threads{ tid_blocked };
	for (int i = 1; i < count; ++i)
	{
		const long tid = bh_tid_of(*clients[i]);
		threads.insert(tid);
		if (tid == tid_blocked)
		{
			++count_same_thread;
			if (index_same < 0)
				index_same = i;
		}
	}

	// - completion : the connections spread over the rings of the workers (an accepted socket is bound round robin at the CQE of the
	//   accept. bound by its first submit, every one was on the ring of the listen socket : 199/199 on one thread before 2026-10-24).
	//   not for epoll : it wakes the thread that waited last, the echoes one at a time ran on one thread there (199/199 : not the sockets)
	if (system_executor::executor()->backend() == executor::io_backend::io_uring_completion)
	{
		EXPECT_GE(threads.size(), 2u) << "the connections ran on one thread";
		EXPECT_LE(count_same_thread + 1, count / 2) << "the connections on the thread of the blocked one : " << count_same_thread << "/" << (count - 1) << " (" << threads.size() << " threads)";
	}

	// 2) client 0 blocks its handler 300ms, then sends 4 more (they come after the block : in order)
	ASSERT_TRUE(clients[0]->send('S', 1));
	std::this_thread::sleep_for(30ms);
	for (uint32_t k = 2; k <= 5; ++k)
		ASSERT_TRUE(clients[0]->send('E', k));

	// 3) meanwhile : every other connection answers at once (a socket on the same thread too : a pipeline of 8 in order)
	int64_t max_us = 0;
	int count_late = 0;
	for (int i = 1; i < count; ++i)
	{
		const bool is_pipeline = (i == index_same);
		const uint32_t count_send = is_pipeline ? 8 : 1;

		const auto time_begin = std::chrono::steady_clock::now();
		for (uint32_t k = 0; k < count_send; ++k)
			ASSERT_TRUE(clients[i]->send('E', 100 + k));
		for (uint32_t k = 0; k < count_send; ++k)
			EXPECT_EQ(clients[i]->receive(), static_cast<int64_t>(100 + k)) << "client " << i << " (the thread of the blocked one : " << (is_pipeline ? "yes" : "no") << ")";
		const auto us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - time_begin).count();

		max_us = std::max<int64_t>(max_us, us);
		if (us > 100000)
			++count_late;
	}

	EXPECT_EQ(count_late, 0) << "max " << max_us << "us, the connections on the thread of the blocked one : " << count_same_thread;

	// 4) the blocked one : its echoes in order after the block
	for (uint32_t k = 1; k <= 5; ++k)
		EXPECT_EQ(clients[0]->receive(), static_cast<int64_t>(k)) << "the blocked one, echo " << k;

	// - the case was there (completion : the rings are shared by the connections. epoll / readiness : any thread)
	std::cout << "[ info ] the other connections on the thread of the blocked one (at the first echo) : " << count_same_thread << "/" << (count - 1) << " (" << threads.size() << " threads), max " << max_us << "us" << std::endl;

	// 5) close
	clients.clear();
	EXPECT_TRUE([&]() { for (int i = 0; i < 500 && pacceptor->count() != 0; ++i) std::this_thread::sleep_for(10ms); return pacceptor->count() == 0; }()) << "connected yet " << pacceptor->count();
	pacceptor->stop();
}

}

#endif
