#include "pch.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// the io_uring completion backend of the io_context (executor::io_backend::io_uring_completion, linux 6.0+)
//
//  1. post : from threads that own no ring (the rings are woken through their eventfd)
//  2. readiness : the attach / set_event / detach interface of the sockets (polls on the bound ring), a closed fd number reused
//  3. ops : recv / send (zero copy) / sendmsg (gather) / accept (multishot : the accepted sockets bound round robin) / connect / cancel /
//     cancel_fd / wait_cancelled_fd
//  4. the executor stopped while ops are in flight : every submit reference is released
//  5. the ops and the polls submitted by short lived threads
//  6. multishot recv + provided buffers : a completion per arrival, the buffers given back by any thread, the buffers run out
//     (-ENOBUFS) and the recv submitted again, cancel_fd, a submit from a short lived thread, many sockets at once
//-----------------------------------------------------------------------------
#if defined(__linux__)

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <cerrno>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	constexpr size_t CQ_F_MORE = 1u << 1;
	constexpr size_t CQ_F_NOTIF = 1u << 3;

	template <class TPREDICATE>
	bool cq_wait_until(TPREDICATE&& _predicate, std::chrono::milliseconds _timeout = 5000ms)
	{
		const auto time_end = std::chrono::steady_clock::now() + _timeout;
		while (std::chrono::steady_clock::now() < time_end)
		{
			if (_predicate())
				return true;
			std::this_thread::sleep_for(1ms);
		}
		return _predicate();
	}

	// - an executor with the completion backend (nullptr : not available)
	object_ptr<executor::Iio_context_threads> cq_make_executor(const char* _name, int _threads)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(executor::io_backend::io_uring_completion);
		auto pexecutor = executor::make_io_context_threads(_name, _threads);
		executor::set_default_io_backend(backend_before);

		if (pexecutor.exist() && pexecutor->backend() != executor::io_backend::io_uring_completion)
		{
			pexecutor->stop();
			return nullptr;
		}

		return pexecutor;
	}

	// - the completions delivered to an executable : (result, param) in order
	class cq_executable : virtual public Iexecutable
	{
	public:
		struct COMPLETION { intptr_t result; size_t param; };

		std::mutex				lock;
		std::vector<COMPLETION>	completions;
		std::atomic<int>		count{ 0 };

		virtual intptr_t process_execute(intptr_t _result, size_t _param) override
		{
			{
				std::lock_guard guard(this->lock);
				this->completions.push_back(COMPLETION{ _result, _param });
			}
			++this->count;
			return 0;
		}

		COMPLETION at(size_t _index)
		{
			std::lock_guard guard(this->lock);
			return this->completions.at(_index);
		}

		bool wait(int _count, std::chrono::milliseconds _timeout = 5000ms)
		{
			return cq_wait_until([&]() { return this->count.load() >= _count; }, _timeout);
		}

		std::string dump()
		{
			std::lock_guard guard(this->lock);
			std::string text = "completions " + std::to_string(this->completions.size()) + " :";
			for (const auto& iter : this->completions)
				text += " (" + std::to_string(iter.result) + ", " + std::to_string(iter.param) + ")";
			return text;
		}
	};

	bool cq_socketpair(int (&_fd)[2])
	{
		return ::socketpair(AF_UNIX, SOCK_STREAM, 0, _fd) == 0;
	}

	// - a listening TCP socket on loopback : the port
	int cq_listen(uint16_t& _port)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		if (fd < 0)
			return -1;

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = 0;

		if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || ::listen(fd, 128) != 0)
		{
			::close(fd);
			return -1;
		}

		socklen_t length = sizeof(address);
		::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length);
		_port = ntohs(address.sin_port);

		return fd;
	}
}


//-----------------------------------------------------------------------------
// 0. availability (skipped elsewhere)
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, availability)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq availability", 2);
	ASSERT_TRUE(pexecutor.exist());
	EXPECT_NE(pexecutor->handle(), INVALID_CGHANDLE_VALUE);
	EXPECT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);

	pexecutor->stop();
	EXPECT_EQ(pexecutor->handle(), INVALID_CGHANDLE_VALUE);
}


//-----------------------------------------------------------------------------
// 1. post : 10k executables from 8 threads that own no ring
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, post_from_non_worker_threads)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq post", 4);
	ASSERT_TRUE(pexecutor.exist());

	std::atomic<int> count_executed{ 0 };
	std::atomic<int> count_posted{ 0 };
	std::vector<std::thread> threads;

	for (int t = 0; t < 8; ++t)
	{
		threads.emplace_back([&]()
		{
			for (int i = 0; i < 1250; ++i)
			{
				if (pexecutor->post(make_executable([&count_executed] { ++count_executed; })))
					++count_posted;
			}
		});
	}

	for (auto& thread : threads)
		thread.join();

	EXPECT_EQ(count_posted.load(), 10000);
	EXPECT_TRUE(cq_wait_until([&]() { return count_executed.load() == 10000; }, 10000ms)) << "executed " << count_executed.load();

	// - the executables post executables (from the workers : the ring of the worker)
	std::atomic<int> count_nested{ 0 };

	for (int i = 0; i < 1000; ++i)
		pexecutor->post(make_executable([&]() { pexecutor->post(make_executable([&count_nested] { ++count_nested; })); }));

	EXPECT_TRUE(cq_wait_until([&]() { return count_nested.load() == 1000; }, 10000ms)) << "nested " << count_nested.load();

	pexecutor->stop();
}

// - a post wakes one ring (the queue is signaled till a ring takes it). no owner waiting in the kernel at that moment => the wake went
//   to a busy ring : a worker going idle takes the queue (before : the post waited till the busy owner returned. the other workers
//   slept : emergency.acceptor_works_while_system_executor_is_jammed failed)
//   . 2 workers : W1 blocked in an executable, W2 in a short one when the post comes (the wake goes to W1 or W2 : round robin)
TEST(io_context_uring_cq, post_taken_by_a_worker_going_idle)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq post to a busy ring", 2);
	ASSERT_TRUE(pexecutor.exist());

	int count_late = 0;

	for (int round = 0; round < 20; ++round)
	{
		std::atomic<bool> is_released{ false };
		std::atomic<int> count_blocked{ 0 };
		std::atomic<bool> is_short_started{ false };
		std::atomic<bool> is_posted_started{ false };
		std::atomic<int> count_done{ 0 };

		// 1) W1 : blocked till released
		pexecutor->post(make_executable([&]()
		{
			++count_blocked;
			while (is_released == false)
				std::this_thread::sleep_for(1ms);
			++count_done;
		}));
		ASSERT_TRUE(cq_wait_until([&]() { return count_blocked.load() == 1; }, 2000ms)) << "round " << round;

		// 2) W2 : a short one (50ms)
		pexecutor->post(make_executable([&]()
		{
			is_short_started = true;
			const auto time_end = std::chrono::steady_clock::now() + 50ms;
			while (std::chrono::steady_clock::now() < time_end) {}
			++count_done;
		}));
		ASSERT_TRUE(cq_wait_until([&]() { return is_short_started.load(); }, 2000ms)) << "round " << round;

		// 3) the post while both are busy : taken by W2 when its short one ends (not after W1 is released)
		pexecutor->post(make_executable([&]()
		{
			is_posted_started = true;
			++count_done;
		}));

		if (cq_wait_until([&]() { return is_posted_started.load(); }, 1000ms) == false)
			++count_late;

		is_released = true;
		ASSERT_TRUE(cq_wait_until([&]() { return count_done.load() == 3; }, 5000ms)) << "round " << round;
	}

	EXPECT_EQ(count_late, 0) << "a post waited for the busy worker its wake went to (" << count_late << " of 20 rounds)";

	pexecutor->stop();
}


//-----------------------------------------------------------------------------
// 2. readiness : attach / set_event / detach (a socketpair), a closed fd number reused
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, readiness_compatibility)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq readiness", 2);
	ASSERT_TRUE(pexecutor.exist());

	int fd[2];
	ASSERT_TRUE(cq_socketpair(fd));

	auto pexecutable = make_object<cq_executable>();

	// 1) attach (EPOLLIN | EPOLLONESHOT) : nothing yet
	ASSERT_TRUE(pexecutor->attach(fd[0], pexecutable.get(), EPOLLIN | EPOLLONESHOT));
	std::this_thread::sleep_for(50ms);
	EXPECT_EQ(pexecutable->count.load(), 0);

	// 2) write => executed once with EPOLLIN (one shot : a second write does not fire again)
	ASSERT_EQ(::write(fd[1], "a", 1), 1);
	ASSERT_TRUE(pexecutable->wait(1));
	EXPECT_NE(pexecutable->at(0).result & EPOLLIN, 0);

	ASSERT_EQ(::write(fd[1], "b", 1), 1);
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(pexecutable->count.load(), 1);

	// 3) re-arm (the data is still there : fires at once)
	ASSERT_TRUE(pexecutor->set_event(fd[0], EPOLLIN | EPOLLONESHOT));
	ASSERT_TRUE(pexecutable->wait(2));
	EXPECT_NE(pexecutable->at(1).result & EPOLLIN, 0);

	char buffer[8];
	EXPECT_EQ(::read(fd[0], buffer, sizeof(buffer)), 2);

	// 4) re-arm with the socket drained : the next write fires. a re-arm while armed (EPOLLOUT added) : one event
	ASSERT_TRUE(pexecutor->set_event(fd[0], EPOLLIN | EPOLLONESHOT));
	ASSERT_TRUE(pexecutor->set_event(fd[0], EPOLLIN | EPOLLOUT | EPOLLONESHOT));
	ASSERT_TRUE(pexecutable->wait(3));
	EXPECT_NE(pexecutable->at(2).result & EPOLLOUT, 0);

	// 5) detach, close : no event after (the poll is gone with the detach)
	ASSERT_TRUE(pexecutor->set_event(fd[0], EPOLLIN | EPOLLONESHOT));
	ASSERT_TRUE(pexecutor->detach(fd[0]));
	ASSERT_EQ(::write(fd[1], "c", 1), 1);
	std::this_thread::sleep_for(100ms);
	EXPECT_EQ(pexecutable->count.load(), 3);

	::close(fd[0]);
	::close(fd[1]);

	// 6) the fd number reused by a new socketpair : its own registration (the events of the old one do not reach it)
	int fd_new[2];
	ASSERT_TRUE(cq_socketpair(fd_new));

	auto pexecutable_new = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->attach(fd_new[0], pexecutable_new.get(), EPOLLIN | EPOLLONESHOT));
	ASSERT_EQ(::write(fd_new[1], "d", 1), 1);
	ASSERT_TRUE(pexecutable_new->wait(1));
	EXPECT_EQ(pexecutable->count.load(), 3);
	ASSERT_TRUE(pexecutor->detach(fd_new[0]));
	::close(fd_new[0]);
	::close(fd_new[1]);

	// 7) a registration without EPOLLONESHOT (armed again after each event by the worker)
	int fd_level[2];
	ASSERT_TRUE(cq_socketpair(fd_level));

	auto pexecutable_level = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->attach(fd_level[0], pexecutable_level.get(), EPOLLIN));
	ASSERT_EQ(::write(fd_level[1], "e", 1), 1);
	ASSERT_TRUE(pexecutable_level->wait(1));
	EXPECT_EQ(::read(fd_level[0], buffer, sizeof(buffer)), 1);
	ASSERT_EQ(::write(fd_level[1], "f", 1), 1);
	ASSERT_TRUE(pexecutable_level->wait(2));
	EXPECT_EQ(::read(fd_level[0], buffer, sizeof(buffer)), 1);
	ASSERT_TRUE(pexecutor->detach(fd_level[0]));
	::close(fd_level[0]);
	::close(fd_level[1]);

	pexecutor->stop();

	// - the references of the registrations are released
	EXPECT_EQ(pexecutable->reference_count(), 1);
	EXPECT_EQ(pexecutable_new->reference_count(), 1);
	EXPECT_EQ(pexecutable_level->reference_count(), 1);
}


//-----------------------------------------------------------------------------
// 3. ops
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, op_recv_send)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq recv send", 2);
	ASSERT_TRUE(pexecutor.exist());

	int fd[2];
	ASSERT_TRUE(cq_socketpair(fd));

	// 1) recv + send from this thread (no ring) : both completions with the byte counts
	char buffer[64] = {};
	auto precv = make_object<cq_executable>();
	auto psend = make_object<cq_executable>();

	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv.get()));
	ASSERT_TRUE(pexecutor->submit_send(fd[1], "hello", 5, 0, psend.get(), false));

	ASSERT_TRUE(psend->wait(1));
	ASSERT_TRUE(precv->wait(1));
	EXPECT_EQ(psend->at(0).result, 5);
	EXPECT_EQ(psend->at(0).param & CQ_F_MORE, 0u);
	EXPECT_EQ(precv->at(0).result, 5);
	EXPECT_EQ(precv->at(0).param & CQ_F_MORE, 0u);
	EXPECT_EQ(std::string(buffer, 5), "hello");

	// - the submit references are released after the delivery
	EXPECT_TRUE(cq_wait_until([&]() { return precv->reference_count() == 1 && psend->reference_count() == 1; }));

	// 2) a recv submitted from a worker thread (its own ring)
	auto precv_worker = make_object<cq_executable>();
	std::atomic<bool> is_submitted{ false };
	std::atomic<bool> is_result{ false };

	pexecutor->post(make_executable([&]()
	{
		is_result = pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv_worker.get());
		is_submitted = true;
	}));

	ASSERT_TRUE(cq_wait_until([&]() { return is_submitted.load(); }));
	ASSERT_TRUE(is_result.load());
	ASSERT_EQ(::write(fd[1], "world!", 6), 6);
	ASSERT_TRUE(precv_worker->wait(1));
	EXPECT_EQ(precv_worker->at(0).result, 6);

	// 3) the peer closed : recv completes with 0
	auto precv_eof = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv_eof.get()));
	std::this_thread::sleep_for(20ms);
	::close(fd[1]);
	ASSERT_TRUE(precv_eof->wait(1));
	EXPECT_EQ(precv_eof->at(0).result, 0);

	::close(fd[0]);

	pexecutor->stop();

	EXPECT_EQ(precv_worker->reference_count(), 1);
	EXPECT_EQ(precv_eof->reference_count(), 1);
}

TEST(io_context_uring_cq, op_send_zerocopy_and_sendmsg)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq zerocopy", 2);
	ASSERT_TRUE(pexecutor.exist());

	// - a TCP pair (SEND_ZC needs an inet socket : a UDS pair answers EOPNOTSUPP on some kernels)
	uint16_t port = 0;
	const int fd_listen = cq_listen(port);
	ASSERT_GE(fd_listen, 0);

	const int fd_client = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	ASSERT_GE(fd_client, 0);

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);
	ASSERT_EQ(::connect(fd_client, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);

	const int fd_server = ::accept4(fd_listen, nullptr, nullptr, SOCK_CLOEXEC);
	ASSERT_GE(fd_server, 0);

	// 1) a zero copy send : the first CQE (the bytes, F_MORE) then the notification (F_NOTIF). loopback : the kernel copies, still 2 CQEs
	//    (MSG_WAITALL : a stream socket sends the whole buffer (the kernel waits for the room : the initial sndbuf is 16KB).
	//     read by a thread of its own : the op belongs to this thread, its task work runs when this thread sleeps)
	std::string message(32 * 1024, 'z');
	auto psend_zc = make_object<cq_executable>();

	std::string received;
	received.resize(message.size());
	std::atomic<size_t> count_received{ 0 };
	std::thread reader([&]()
	{
		while (count_received < message.size())
		{
			const auto result = ::recv(fd_server, received.data() + count_received, received.size() - count_received, 0);
			if (result <= 0)
				break;
			count_received += static_cast<size_t>(result);
		}
	});

	ASSERT_TRUE(pexecutor->submit_send(fd_client, message.data(), message.size(), MSG_WAITALL, psend_zc.get(), true));

	EXPECT_TRUE(cq_wait_until([&]() { return count_received.load() == message.size(); }, 10000ms)) << "received " << count_received.load();
	if (count_received.load() != message.size())
		::shutdown(fd_client, SHUT_RDWR);
	reader.join();
	EXPECT_EQ(received, message);

	ASSERT_TRUE(psend_zc->wait(2)) << psend_zc->dump();
	EXPECT_EQ(psend_zc->at(0).result, static_cast<intptr_t>(message.size()));
	EXPECT_NE(psend_zc->at(0).param & CQ_F_MORE, 0u);
	EXPECT_NE(psend_zc->at(1).param & CQ_F_NOTIF, 0u);
	EXPECT_EQ(psend_zc->at(1).param & CQ_F_MORE, 0u);
	EXPECT_TRUE(cq_wait_until([&]() { return psend_zc->reference_count() == 1; }));

	// 2) sendmsg : a gather of 3 parts (the msghdr / iov live till the completion)
	const char part_1[] = "abc";
	const char part_2[] = "defgh";
	const char part_3[] = "ij";
	iovec iov[3] = { { const_cast<char*>(part_1), 3 }, { const_cast<char*>(part_2), 5 }, { const_cast<char*>(part_3), 2 } };
	msghdr msg{};
	msg.msg_iov = iov;
	msg.msg_iovlen = 3;

	auto psendmsg = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_sendmsg(fd_client, &msg, 0, psendmsg.get(), false));
	ASSERT_TRUE(psendmsg->wait(1));
	EXPECT_EQ(psendmsg->at(0).result, 10);

	char buffer[16] = {};
	size_t count_gather = 0;
	while (count_gather < 10)
	{
		const auto result = ::recv(fd_server, buffer + count_gather, sizeof(buffer) - count_gather, 0);
		ASSERT_GT(result, 0);
		count_gather += static_cast<size_t>(result);
	}
	EXPECT_EQ(std::string(buffer, 10), "abcdefghij");

	// 3) recvmsg
	char buffer_recv[16] = {};
	iovec iov_recv{ buffer_recv, sizeof(buffer_recv) };
	msghdr msg_recv{};
	msg_recv.msg_iov = &iov_recv;
	msg_recv.msg_iovlen = 1;

	auto precvmsg = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recvmsg(fd_server, &msg_recv, 0, precvmsg.get()));
	ASSERT_EQ(::send(fd_client, "xyz", 3, 0), 3);
	ASSERT_TRUE(precvmsg->wait(1));
	EXPECT_EQ(precvmsg->at(0).result, 3);
	EXPECT_EQ(std::string(buffer_recv, 3), "xyz");

	::close(fd_client);
	::close(fd_server);
	::close(fd_listen);

	pexecutor->stop();
}

TEST(io_context_uring_cq, op_accept_multishot_and_connect)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq accept", 2);
	ASSERT_TRUE(pexecutor.exist());

	uint16_t port = 0;
	const int fd_listen = cq_listen(port);
	ASSERT_GE(fd_listen, 0);

	// 1) a multishot accept : 50 connects => 50 CQEs with F_MORE (the accepted fds)
	auto paccept = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_accept(fd_listen, nullptr, nullptr, SOCK_CLOEXEC, true, paccept.get()));

	std::vector<int> clients;
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);

	for (int i = 0; i < 50; ++i)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		ASSERT_GE(fd, 0);
		ASSERT_EQ(::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
		clients.push_back(fd);
	}

	ASSERT_TRUE(paccept->wait(50)) << paccept->dump();
	for (int i = 0; i < 50; ++i)
	{
		const auto completion = paccept->at(static_cast<size_t>(i));
		EXPECT_GE(completion.result, 0);
		EXPECT_NE(completion.param & CQ_F_MORE, 0u);
		if (completion.result >= 0)
			::close(static_cast<int>(completion.result));
	}

	// 2) cancel_fd(listen) : the final CQE (-ECANCELED, no F_MORE). the submit reference goes with it
	EXPECT_TRUE(pexecutor->submit_cancel_fd(fd_listen));
	ASSERT_TRUE(paccept->wait(51));
	EXPECT_EQ(paccept->at(50).result, -ECANCELED);
	EXPECT_EQ(paccept->at(50).param & CQ_F_MORE, 0u);
	EXPECT_TRUE(cq_wait_until([&]() { return paccept->reference_count() == 1; }));

	for (const int fd : clients)
		::close(fd);

	// 3) a one shot accept with the address
	auto paccept_one = make_object<cq_executable>();
	sockaddr_in address_peer{};
	socklen_t length_peer = sizeof(address_peer);
	ASSERT_TRUE(pexecutor->submit_accept(fd_listen, reinterpret_cast<sockaddr*>(&address_peer), &length_peer, SOCK_CLOEXEC, false, paccept_one.get()));

	// 4) connect : completes with 0 (the op) and the accept above with the fd
	auto pconnect = make_object<cq_executable>();
	const int fd_connect = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	ASSERT_GE(fd_connect, 0);
	ASSERT_TRUE(pexecutor->submit_connect(fd_connect, reinterpret_cast<sockaddr*>(&address), sizeof(address), pconnect.get()));
	ASSERT_TRUE(pconnect->wait(1));
	EXPECT_EQ(pconnect->at(0).result, 0);
	ASSERT_TRUE(paccept_one->wait(1));
	EXPECT_GE(paccept_one->at(0).result, 0);
	EXPECT_EQ(paccept_one->at(0).param & CQ_F_MORE, 0u);
	EXPECT_EQ(address_peer.sin_family, AF_INET);
	if (paccept_one->at(0).result >= 0)
		::close(static_cast<int>(paccept_one->at(0).result));
	::close(fd_connect);

	// 5) connect to a closed port : -ECONNREFUSED
	::close(fd_listen);

	auto pconnect_refused = make_object<cq_executable>();
	const int fd_refused = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	ASSERT_GE(fd_refused, 0);
	ASSERT_TRUE(pexecutor->submit_connect(fd_refused, reinterpret_cast<sockaddr*>(&address), sizeof(address), pconnect_refused.get()));
	ASSERT_TRUE(pconnect_refused->wait(1));
	EXPECT_EQ(pconnect_refused->at(0).result, -ECONNREFUSED);
	::close(fd_refused);

	pexecutor->stop();
}

// - the sockets accepted by an accept op are bound round robin at its CQE. bound by their first submit instead : the accept handler runs
//   on the owner of the listen ring, a RECV it submits for an accepted socket went to that ring (every socket of an acceptor on one
//   thread while the other workers idled)
//   . 4 workers, 32 connections : the RECV of each accepted socket (submitted by the accept handler) completes on the owner of the
//     ring of the socket => 4 threads, 8 sockets each
namespace
{
	// - a RECV of an accepted socket : the threads that ran its completions
	class cq_receive_executable : virtual public Iexecutable
	{
	public:
		char					buffer[16];
		std::mutex				lock;
		std::set<std::thread::id> threads;
		std::atomic<int>		count{ 0 };

		virtual intptr_t process_execute(intptr_t /*_result*/, size_t /*_param*/) override
		{
			{
				std::lock_guard guard(this->lock);
				this->threads.insert(std::this_thread::get_id());
			}
			++this->count;
			return 0;
		}
	};

	// - a multishot accept whose handler submits a RECV for each accepted socket (on the thread running the accept)
	class cq_accept_receive_executable : virtual public Iexecutable
	{
	public:
		executor::Iio_context_threads* pio_context = nullptr;
		std::mutex				lock;
		std::vector<int>		fds;
		std::vector<object_ptr<cq_receive_executable>> receives;
		std::atomic<int>		count{ 0 };
		std::atomic<int>		count_submit_failed{ 0 };

		virtual intptr_t process_execute(intptr_t _result, size_t /*_param*/) override
		{
			if (_result >= 0)
			{
				auto preceive = make_object<cq_receive_executable>();

				if (this->pio_context->submit_recv(static_cast<CGHANDLE>(_result), preceive->buffer, sizeof(preceive->buffer), 0, preceive.get()) == false)
					++this->count_submit_failed;

				std::lock_guard guard(this->lock);
				this->fds.push_back(static_cast<int>(_result));
				this->receives.push_back(std::move(preceive));
			}

			++this->count;
			return 0;
		}
	};

	// - the io_uring fds of this process once their count stayed the same for 200ms (at most 3s) : the workers of a new executor create
	//   their rings when they start (after make_io_context_threads() returned)
	int cq_count_io_uring_fd_settled()
	{
		auto count_fd = []()
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
		};

		int count = count_fd();
		const auto tick_end = std::chrono::steady_clock::now() + 3s;
		auto tick_same = std::chrono::steady_clock::now();

		while (std::chrono::steady_clock::now() < tick_end)
		{
			std::this_thread::sleep_for(20ms);

			const int count_now = count_fd();

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
}

TEST(io_context_uring_cq, op_accept_binds_the_accepted_sockets_round_robin)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	constexpr int count_thread = 4;
	constexpr int count_client = 32;

	auto pexecutor = cq_make_executor("uring_cq accept round robin", count_thread);
	ASSERT_TRUE(pexecutor.exist());
	cq_count_io_uring_fd_settled();

	uint16_t port = 0;
	const int fd_listen = cq_listen(port);
	ASSERT_GE(fd_listen, 0);

	// 1) the multishot accept : its handler submits a RECV for each accepted socket
	auto paccept = make_object<cq_accept_receive_executable>();
	paccept->pio_context = pexecutor.get();
	ASSERT_TRUE(pexecutor->submit_accept(fd_listen, nullptr, nullptr, SOCK_CLOEXEC, true, paccept.get()));

	std::vector<int> clients;
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);

	for (int i = 0; i < count_client; ++i)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		ASSERT_GE(fd, 0);
		ASSERT_EQ(::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
		clients.push_back(fd);
	}

	ASSERT_TRUE(cq_wait_until([&]() { return paccept->count.load() >= count_client; })) << "accepted " << paccept->count.load();
	EXPECT_EQ(paccept->count_submit_failed.load(), 0);

	// 2) a byte to each : the RECVs complete on the owners of the rings of the accepted sockets
	for (const int fd : clients)
		ASSERT_EQ(::write(fd, "a", 1), 1);

	std::vector<object_ptr<cq_receive_executable>> receives;
	{
		std::lock_guard guard(paccept->lock);
		receives = paccept->receives;
	}
	ASSERT_EQ(receives.size(), static_cast<std::size_t>(count_client));
	ASSERT_TRUE(cq_wait_until([&]() { return std::all_of(receives.begin(), receives.end(), [](auto& _p) { return _p->count.load() == 1; }); }));

	std::map<std::thread::id, int> sockets_of_thread;
	for (auto& preceive : receives)
	{
		std::lock_guard guard(preceive->lock);
		for (const auto& id : preceive->threads)
			++sockets_of_thread[id];
	}

	int max_sockets = 0;
	for (const auto& [id, count] : sockets_of_thread)
		max_sockets = std::max(max_sockets, count);

	EXPECT_EQ(sockets_of_thread.size(), static_cast<std::size_t>(count_thread)) << "the accepted sockets ran on " << sockets_of_thread.size() << " of " << count_thread << " workers (max " << max_sockets << " sockets on one)";
	EXPECT_LE(max_sockets, count_client / count_thread + 1);

	// 3) close : the accepted sockets (their RECVs completed), the clients, the accept (cancel_fd : its final CQE)
	{
		std::lock_guard guard(paccept->lock);
		for (const int fd : paccept->fds)
			::close(fd);
	}
	for (const int fd : clients)
		::close(fd);

	EXPECT_TRUE(pexecutor->submit_cancel_fd(fd_listen));
	EXPECT_TRUE(cq_wait_until([&]() { return paccept->count.load() == count_client + 1; })) << "the final CQE of the accept";
	::close(fd_listen);

	pexecutor->stop();
}

TEST(io_context_uring_cq, op_cancel_and_wait_cancelled_fd)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq cancel", 2);
	ASSERT_TRUE(pexecutor.exist());

	int fd[2];
	ASSERT_TRUE(cq_socketpair(fd));

	char buffer[64];

	// 1) a pending recv cancelled by the executable : -ECANCELED delivered
	auto precv = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv.get()));
	std::this_thread::sleep_for(20ms);
	EXPECT_TRUE(pexecutor->submit_cancel(precv.get()));
	ASSERT_TRUE(precv->wait(1));
	EXPECT_EQ(precv->at(0).result, -ECANCELED);
	EXPECT_TRUE(cq_wait_until([&]() { return precv->reference_count() == 1; }));

	// - nothing to cancel any more
	EXPECT_FALSE(pexecutor->submit_cancel(precv.get()));

	// 2) two recvs on the fd cancelled by cancel_fd
	auto precv_1 = make_object<cq_executable>();
	auto precv_2 = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv_1.get()));
	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv_2.get()));
	std::this_thread::sleep_for(20ms);
	EXPECT_TRUE(pexecutor->submit_cancel_fd(fd[0]));
	ASSERT_TRUE(precv_1->wait(1));
	ASSERT_TRUE(precv_2->wait(1));
	EXPECT_EQ(precv_1->at(0).result, -ECANCELED);
	EXPECT_EQ(precv_2->at(0).result, -ECANCELED);

	// 3) wait_cancelled_fd before close : the recv is cancelled, delivered, nothing in flight
	auto precv_3 = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, precv_3.get()));
	std::this_thread::sleep_for(20ms);
	EXPECT_TRUE(pexecutor->wait_cancelled_fd(fd[0], 2s)) << precv_3->dump() << " errno " << errno;
	EXPECT_TRUE(precv_3->wait(1));
	EXPECT_EQ(precv_3->at(0).result, -ECANCELED);
	EXPECT_EQ(precv_3->reference_count(), 1);

	// - nothing in flight : true at once
	EXPECT_TRUE(pexecutor->wait_cancelled_fd(fd[0], 100ms));

	::close(fd[0]);
	::close(fd[1]);

	// 4) the fd number reused : a recv on the new socket is its own
	int fd_new[2];
	ASSERT_TRUE(cq_socketpair(fd_new));

	auto precv_new = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd_new[0], buffer, sizeof(buffer), 0, precv_new.get()));
	ASSERT_EQ(::write(fd_new[1], "ok", 2), 2);
	ASSERT_TRUE(precv_new->wait(1));
	EXPECT_EQ(precv_new->at(0).result, 2);
	::close(fd_new[0]);
	::close(fd_new[1]);

	pexecutor->stop();
}


//-----------------------------------------------------------------------------
// 4. the executor stopped while ops are in flight : delivered or cancelled, every submit reference released
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, stop_with_ops_in_flight)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq stop", 3);
	ASSERT_TRUE(pexecutor.exist());

	std::vector<int> fds;
	std::vector<object_ptr<cq_executable>> executables;
	char buffer[64];

	// 1) pending recvs (from this thread and from the workers), a multishot accept, a readiness poll, posts
	for (int i = 0; i < 16; ++i)
	{
		int fd[2];
		ASSERT_TRUE(cq_socketpair(fd));
		fds.push_back(fd[0]);
		fds.push_back(fd[1]);

		auto pexecutable = make_object<cq_executable>();
		executables.push_back(pexecutable);

		if (i % 2 == 0)
		{
			ASSERT_TRUE(pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, pexecutable.get()));
		}
		else
		{
			std::atomic<bool> is_done{ false };
			pexecutor->post(make_executable([&]() { pexecutor->submit_recv(fd[0], buffer, sizeof(buffer), 0, pexecutable.get()); is_done = true; }));
			ASSERT_TRUE(cq_wait_until([&]() { return is_done.load(); }));
		}
	}

	uint16_t port = 0;
	const int fd_listen = cq_listen(port);
	ASSERT_GE(fd_listen, 0);
	auto paccept = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_accept(fd_listen, nullptr, nullptr, SOCK_CLOEXEC, true, paccept.get()));

	int fd_poll[2];
	ASSERT_TRUE(cq_socketpair(fd_poll));
	auto ppoll = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->attach(fd_poll[0], ppoll.get(), EPOLLIN | EPOLLONESHOT));

	auto ppost = make_object<cq_executable>();
	for (int i = 0; i < 100; ++i)
		pexecutor->post(object_ptr<Iexecutable>(ppost.get()));

	std::this_thread::sleep_for(50ms);

	// 2) stop : the ops are cancelled (delivered with -ECANCELED or released by the destroy), the registrations released
	pexecutor->stop();

	for (auto& pexecutable : executables)
	{
		EXPECT_EQ(pexecutable->reference_count(), 1);
		if (pexecutable->count.load() != 0)
		{
			EXPECT_EQ(pexecutable->at(0).result, -ECANCELED);
		}
	}

	EXPECT_EQ(paccept->reference_count(), 1);
	EXPECT_EQ(ppoll->reference_count(), 1);
	EXPECT_EQ(ppost->reference_count(), 1);

	for (const int fd : fds)
		::close(fd);
	::close(fd_listen);
	::close(fd_poll[0]);
	::close(fd_poll[1]);

	// 3) a new executor after (the rings of the old one are gone)
	auto pexecutor_2 = cq_make_executor("uring_cq after stop", 2);
	ASSERT_TRUE(pexecutor_2.exist());

	std::atomic<int> count{ 0 };
	for (int i = 0; i < 100; ++i)
		pexecutor_2->post(make_executable([&count] { ++count; }));
	EXPECT_TRUE(cq_wait_until([&]() { return count.load() == 100; }));

	pexecutor_2->stop();
}


//-----------------------------------------------------------------------------
// 5. the ops and the polls submitted by short lived threads : a request belongs to the thread that submitted it (the kernel
//    cancels the requests of a thread that exits), so they are staged and submitted by the owner of the ring
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, ops_from_short_lived_threads)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq short lived", 2);
	ASSERT_TRUE(pexecutor.exist());

	uint16_t port = 0;
	const int fd_listen = cq_listen(port);
	ASSERT_GE(fd_listen, 0);

	auto paccept = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_accept(fd_listen, nullptr, nullptr, SOCK_CLOEXEC, true, paccept.get()));

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(port);

	// 1) 8 threads x 50 connects, each thread exits right after its submits : every connect completes with 0 (not -ECANCELED)
	const int COUNT_THREAD = 8;
	const int COUNT_CONNECT = 50;
	const int total = COUNT_THREAD * COUNT_CONNECT;

	std::vector<int> fds_connect(static_cast<size_t>(total), -1);
	std::vector<object_ptr<cq_executable>> connects(static_cast<size_t>(total));
	for (auto& pconnect : connects)
		pconnect = make_object<cq_executable>();

	std::atomic<int> count_submit_failed{ 0 };
	{
		std::vector<std::thread> threads;
		for (int t = 0; t < COUNT_THREAD; ++t)
		{
			threads.emplace_back([&, t]()
				{
					for (int i = 0; i < COUNT_CONNECT; ++i)
					{
						const auto index = static_cast<size_t>(t * COUNT_CONNECT + i);
						fds_connect[index] = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
						if (fds_connect[index] < 0 || pexecutor->submit_connect(fds_connect[index], reinterpret_cast<sockaddr*>(&address), sizeof(address), connects[index].get()) == false)
							++count_submit_failed;
					}
				});
		}
		for (auto& thread : threads)
			thread.join();
	}
	EXPECT_EQ(count_submit_failed.load(), 0);

	int count_cancelled = 0;
	int count_connected = 0;
	for (size_t i = 0; i < connects.size(); ++i)
	{
		ASSERT_TRUE(connects[i]->wait(1)) << "connect " << i << " not completed : " << connects[i]->dump();
		const auto result = connects[i]->at(0).result;
		if (result == -ECANCELED) ++count_cancelled;
		if (result == 0) ++count_connected;
	}
	EXPECT_EQ(count_cancelled, 0);
	EXPECT_EQ(count_connected, total);
	ASSERT_TRUE(paccept->wait(total)) << paccept->dump();
	for (int i = 0; i < total; ++i)
	{
		const auto completion = paccept->at(static_cast<size_t>(i));
		if (completion.result >= 0)
			::close(static_cast<int>(completion.result));
	}
	for (const int fd : fds_connect)
		if (fd >= 0) ::close(fd);

	// 2) recvs submitted by threads that exit at once : completed by the data
	std::vector<int> fds_pair;
	std::vector<object_ptr<cq_executable>> recvs;
	char buffers[16][8] = {};
	for (int i = 0; i < 16; ++i)
	{
		int fd[2];
		ASSERT_TRUE(cq_socketpair(fd));
		fds_pair.push_back(fd[0]);
		fds_pair.push_back(fd[1]);
		recvs.push_back(make_object<cq_executable>());
	}
	{
		std::vector<std::thread> threads;
		for (int t = 0; t < 4; ++t)
		{
			threads.emplace_back([&, t]()
				{
					for (int i = t * 4; i < t * 4 + 4; ++i)
						if (pexecutor->submit_recv(fds_pair[static_cast<size_t>(i) * 2], buffers[i], sizeof(buffers[i]), 0, recvs[static_cast<size_t>(i)].get()) == false)
							++count_submit_failed;
				});
		}
		for (auto& thread : threads)
			thread.join();
	}
	EXPECT_EQ(count_submit_failed.load(), 0);
	for (int i = 0; i < 16; ++i)
		ASSERT_EQ(::write(fds_pair[static_cast<size_t>(i) * 2 + 1], "ok", 2), 2);
	for (int i = 0; i < 16; ++i)
	{
		ASSERT_TRUE(recvs[static_cast<size_t>(i)]->wait(1)) << "recv " << i << " : " << recvs[static_cast<size_t>(i)]->dump();
		EXPECT_EQ(recvs[static_cast<size_t>(i)]->at(0).result, 2) << "recv " << i;
	}

	// 3) a readiness poll attached by a short lived thread : the event comes (the poll is staged for the owner too)
	int fd_poll[2];
	ASSERT_TRUE(cq_socketpair(fd_poll));
	auto ppoll = make_object<cq_executable>();
	{
		std::thread thread([&]() { EXPECT_TRUE(pexecutor->attach(fd_poll[0], ppoll.get(), EPOLLIN | EPOLLONESHOT)); });
		thread.join();
	}
	ASSERT_EQ(::write(fd_poll[1], "p", 1), 1);
	ASSERT_TRUE(ppoll->wait(1)) << ppoll->dump();
	EXPECT_NE(ppoll->at(0).result & EPOLLIN, 0);
	EXPECT_TRUE(pexecutor->detach(fd_poll[0]));

	// 4) an op cancelled right after its submit from this thread (not a worker : still staged, or in the kernel already) : -ECANCELED delivered
	int fd_cancel[2];
	ASSERT_TRUE(cq_socketpair(fd_cancel));
	char buffer_cancel[8];

	auto precv_cancel = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd_cancel[0], buffer_cancel, sizeof(buffer_cancel), 0, precv_cancel.get()));
	EXPECT_TRUE(pexecutor->submit_cancel(precv_cancel.get()));
	ASSERT_TRUE(precv_cancel->wait(1)) << precv_cancel->dump();
	EXPECT_EQ(precv_cancel->at(0).result, -ECANCELED);
	EXPECT_TRUE(cq_wait_until([&]() { return precv_cancel->reference_count() == 1; }));

	auto precv_wait = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd_cancel[0], buffer_cancel, sizeof(buffer_cancel), 0, precv_wait.get()));
	EXPECT_TRUE(pexecutor->wait_cancelled_fd(fd_cancel[0], 2s)) << precv_wait->dump() << " errno " << errno;
	ASSERT_TRUE(precv_wait->wait(1)) << precv_wait->dump();
	EXPECT_EQ(precv_wait->at(0).result, -ECANCELED);
	EXPECT_EQ(precv_wait->reference_count(), 1);

	// - the end
	EXPECT_TRUE(pexecutor->submit_cancel_fd(fd_listen));
	ASSERT_TRUE(paccept->wait(total + 1));
	EXPECT_TRUE(cq_wait_until([&]() { return paccept->reference_count() == 1; }));

	for (const int fd : fds_pair)
		::close(fd);
	::close(fd_poll[0]);
	::close(fd_poll[1]);
	::close(fd_cancel[0]);
	::close(fd_cancel[1]);
	::close(fd_listen);

	pexecutor->stop();
}


//-----------------------------------------------------------------------------
// 6. multishot recv + provided buffers (submit_recv_multishot) : one submit, a completion per arrival with the data in a provided
//    buffer of the ring (IORING_CQE_F_BUFFER), given back by the consumer (return_buffer : any thread). the buffers run out =>
//    the recv ends with -ENOBUFS (the data waits in the socket) and is submitted again after they are given back
//-----------------------------------------------------------------------------
namespace
{
	constexpr size_t CQ_F_BUFFER = 1u << 0;

	// - the completions of a multishot recv : the data copied out, the buffer given back at once (is_hold : kept till give_back()).
	//   is_resubmit : submitted again by the executable when the recv ended with data or -ENOBUFS (like the stream socket)
	class cq_multishot_executable : virtual public Iexecutable
	{
	public:
		executor::Iio_context*	pio_context = nullptr;
		int						fd = -1;
		std::atomic<bool>		is_hold{ false };
		std::atomic<bool>		is_resubmit{ false };

		std::mutex				lock;
		std::string				data;
		std::vector<cq_executable::COMPLETION> completions;
		std::vector<size_t>		held;
		std::atomic<int>		count{ 0 };
		std::atomic<int>		count_final{ 0 };
		std::atomic<int>		count_enobufs{ 0 };
		std::atomic<int>		count_executing{ 0 };
		std::atomic<int>		count_concurrent{ 0 };	// - executions that overlapped (the completions of one op run one at a time : 0)
		std::atomic<int>		count_resubmit_failed{ 0 };

		virtual intptr_t process_execute(intptr_t _result, size_t _param) override
		{
			if (++this->count_executing != 1)
				++this->count_concurrent;

			const bool is_final = (_param & CQ_F_MORE) == 0;
			{
				std::lock_guard guard(this->lock);
				this->completions.push_back(cq_executable::COMPLETION{ _result, _param });

				if ((_param & CQ_F_BUFFER) != 0)
				{
					const auto* pdata = static_cast<const char*>(this->pio_context->get_provided_buffer(_param));

					if (pdata != nullptr && _result > 0)
						this->data.append(pdata, static_cast<size_t>(_result));

					if (this->is_hold.load())
						this->held.push_back(_param);
					else
						this->pio_context->return_buffer(_param);
				}
			}

			if (is_final)
			{
				if (_result == -ENOBUFS)
					++this->count_enobufs;

				++this->count_final;

				if (this->is_resubmit.load() && (_result > 0 || _result == -ENOBUFS) && this->pio_context->submit_recv_multishot(this->fd, this) == false)
					++this->count_resubmit_failed;
			}

			--this->count_executing;
			++this->count;
			return 0;
		}

		std::vector<size_t> take_held()
		{
			std::lock_guard guard(this->lock);
			std::vector<size_t> params;
			params.swap(this->held);
			return params;
		}

		size_t received_size()
		{
			std::lock_guard guard(this->lock);
			return this->data.size();
		}

		std::string received()
		{
			std::lock_guard guard(this->lock);
			return this->data;
		}

		cq_executable::COMPLETION last()
		{
			std::lock_guard guard(this->lock);
			return this->completions.back();
		}

		std::string dump(size_t _last = 8)
		{
			std::lock_guard guard(this->lock);
			std::string text = "completions " + std::to_string(this->completions.size()) + " (last) :";
			const size_t start = (this->completions.size() > _last) ? this->completions.size() - _last : 0;
			for (size_t i = start; i < this->completions.size(); ++i)
				text += " (" + std::to_string(this->completions[i].result) + ", " + std::to_string(this->completions[i].param & 0xFFFFFFFFu) + ")";
			return text;
		}
	};

	// - writes all of _data (a blocking socket : the reader drains it meanwhile)
	bool cq_write_all(int _fd, const std::string& _data)
	{
		size_t offset = 0;
		while (offset < _data.size())
		{
			const auto result = ::write(_fd, _data.data() + offset, _data.size() - offset);
			if (result <= 0)
				return false;
			offset += static_cast<size_t>(result);
		}
		return true;
	}

	std::string cq_pattern(size_t _size, unsigned _seed)
	{
		std::string text(_size, '\0');
		uint32_t value = 0x9E3779B9u * (_seed + 1);
		for (auto& iter : text)
		{
			value = value * 1664525u + 1013904223u;
			iter = static_cast<char>(value >> 24);
		}
		return text;
	}
}

TEST(io_context_uring_cq, op_recv_multishot_provided_buffer)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	auto pexecutor = cq_make_executor("uring_cq multishot", 2);
	ASSERT_TRUE(pexecutor.exist());

	int fd[2];
	ASSERT_TRUE(cq_socketpair(fd));

	auto pms = make_object<cq_multishot_executable>();
	pms->pio_context = pexecutor.get();
	pms->fd = fd[0];

	// 1) one submit from this thread (not a worker : staged, submitted by the owner of the ring) : a completion per arrival,
	//    F_BUFFER + F_MORE, the submit reference kept while the recv goes on
	if (pexecutor->submit_recv_multishot(fd[0], pms.get()) == false)
	{
		::close(fd[0]);
		::close(fd[1]);
		pexecutor->stop();
		GTEST_SKIP() << "multishot recv with a provided buffer ring is not available (CGDK_IO_URING_MULTISHOT=0 or the kernel refused it)";
	}

	ASSERT_EQ(::write(fd[1], "hello", 5), 5);
	ASSERT_TRUE(cq_wait_until([&]() { return pms->received_size() == 5; })) << pms->dump();
	ASSERT_EQ(::write(fd[1], "world!", 6), 6);
	ASSERT_TRUE(cq_wait_until([&]() { return pms->received_size() == 11; })) << pms->dump();
	EXPECT_EQ(pms->received(), "helloworld!");
	EXPECT_EQ(pms->count_final.load(), 0) << pms->dump();
	{
		const auto completion = pms->last();
		EXPECT_NE(completion.param & CQ_F_MORE, 0u);
		EXPECT_NE(completion.param & CQ_F_BUFFER, 0u);
	}
	EXPECT_GE(pms->reference_count(), 2);

	// 2) a large write (many buffers) : the data in order, intact
	{
		const auto text = cq_pattern(512 * 1024, 1);
		std::thread writer([&]() { EXPECT_TRUE(cq_write_all(fd[1], text)); });
		const bool is_received = cq_wait_until([&]() { return pms->received_size() == 11 + text.size(); }, 10000ms);
		writer.join();
		ASSERT_TRUE(is_received) << "received " << pms->received_size() << " " << pms->dump();
		EXPECT_TRUE(pms->received().substr(11) == text) << "the data differs";
	}
	EXPECT_EQ(pms->count_final.load(), 0) << pms->dump();

	// 3) the buffers run out : kept (not given back) => the recv ends with -ENOBUFS (no F_MORE). a write per completion
	//    (each one a buffer). the kernel picks the buffer before the recv : the -ENOBUFS comes as soon as the last buffer is taken
	//    (the retry of the multishot) or with the next arrival (that byte waits in the socket)
	pms->is_hold = true;
	const auto size_before_hold = pms->received_size();
	int count_written = 0;

	for (int i = 0; i < 4096 && pms->count_final.load() == 0; ++i)
	{
		const int count_before = pms->count.load();
		ASSERT_EQ(::write(fd[1], "x", 1), 1);
		++count_written;
		ASSERT_TRUE(cq_wait_until([&]() { return pms->count.load() > count_before; }, 2000ms)) << "write " << i << " " << pms->dump();
	}

	ASSERT_EQ(pms->count_final.load(), 1) << pms->dump();
	{
		const auto completion = pms->last();
		EXPECT_EQ(completion.result, -ENOBUFS) << pms->dump();
		EXPECT_EQ(completion.param & CQ_F_MORE, 0u);
	}
	EXPECT_GE(pms->received_size() + 1, size_before_hold + static_cast<size_t>(count_written));
	EXPECT_LE(pms->received_size(), size_before_hold + static_cast<size_t>(count_written));

	// - given back by executables posted to the workers (the worker of the other ring among them : the buffer ring of another ring
	//   is written under its lock), and by this thread (no ring)
	auto held = pms->take_held();
	EXPECT_GT(held.size(), 0u);
	std::atomic<int> count_returned{ 0 };
	std::atomic<int> count_return_failed{ 0 };
	for (size_t i = 0; i < held.size(); ++i)
	{
		const size_t param = held[i];
		if (i % 4 == 0)
		{
			if (pexecutor->return_buffer(param)) ++count_returned; else ++count_return_failed;
		}
		else
		{
			pexecutor->post(make_executable([&, param]() { if (pexecutor->return_buffer(param)) ++count_returned; else ++count_return_failed; }));
		}
	}
	ASSERT_TRUE(cq_wait_until([&]() { return count_returned.load() + count_return_failed.load() == static_cast<int>(held.size()); })) << count_returned.load();
	EXPECT_EQ(count_return_failed.load(), 0);

	// - a buffer given back twice : ignored
	EXPECT_FALSE(pexecutor->return_buffer(held[0]));

	// - submitted again : the byte that waited (if any) comes, nothing is lost
	pms->is_hold = false;
	ASSERT_TRUE(pexecutor->submit_recv_multishot(fd[0], pms.get()));
	ASSERT_TRUE(cq_wait_until([&]() { return pms->received_size() == size_before_hold + static_cast<size_t>(count_written); })) << pms->dump();
	EXPECT_EQ(pms->received().substr(size_before_hold), std::string(static_cast<size_t>(count_written), 'x'));

	// 4) cancel_fd : the recv ends with -ECANCELED, the submit reference released
	EXPECT_TRUE(pexecutor->submit_cancel_fd(fd[0]));
	ASSERT_TRUE(cq_wait_until([&]() { return pms->count_final.load() == 2; })) << pms->dump();
	EXPECT_EQ(pms->last().result, -ECANCELED) << pms->dump();
	EXPECT_TRUE(cq_wait_until([&]() { return pms->reference_count() == 1; })) << pms->reference_count();

	// 5) a submit from a short lived thread (the op is staged for the owner of the ring : it does not end with the thread)
	{
		std::thread thread([&]() { EXPECT_TRUE(pexecutor->submit_recv_multishot(fd[0], pms.get())); });
		thread.join();
	}
	std::this_thread::sleep_for(50ms);
	const auto size_before_after = pms->received_size();
	ASSERT_EQ(::write(fd[1], "after", 5), 5);
	ASSERT_TRUE(cq_wait_until([&]() { return pms->received_size() == size_before_after + 5; })) << pms->dump();
	EXPECT_NE(pms->last().param & CQ_F_MORE, 0u) << pms->dump();
	EXPECT_EQ(pms->count_final.load(), 2) << pms->dump();

	// - the peer closes : the recv ends with 0
	::close(fd[1]);
	ASSERT_TRUE(cq_wait_until([&]() { return pms->count_final.load() == 3; })) << pms->dump();
	EXPECT_EQ(pms->last().result, 0) << pms->dump();
	EXPECT_TRUE(cq_wait_until([&]() { return pms->reference_count() == 1; }));
	EXPECT_EQ(pms->count_concurrent.load(), 0);
	::close(fd[0]);

	// 6) many sockets at once (both rings, the buffers shared by the sockets of a ring : they run out and the recvs are submitted
	//    again by the executables, like the stream socket) : every stream in order and intact
	{
		constexpr int COUNT_SOCKET = 16;
		constexpr size_t SIZE_STREAM = 1024 * 1024;

		std::vector<std::array<int, 2>> fds(COUNT_SOCKET);
		std::vector<object_ptr<cq_multishot_executable>> executables;
		std::vector<std::string> texts;

		for (int i = 0; i < COUNT_SOCKET; ++i)
		{
			int pair[2];
			ASSERT_TRUE(cq_socketpair(pair));
			fds[static_cast<size_t>(i)] = { pair[0], pair[1] };

			auto pexecutable = make_object<cq_multishot_executable>();
			pexecutable->pio_context = pexecutor.get();
			pexecutable->fd = pair[0];
			pexecutable->is_resubmit = true;
			executables.push_back(pexecutable);
			texts.push_back(cq_pattern(SIZE_STREAM, static_cast<unsigned>(i + 10)));
		}

		// - submitted from the workers (posted) and from this thread
		std::atomic<int> count_submit_failed{ 0 };
		for (int i = 0; i < COUNT_SOCKET; ++i)
		{
			auto* pexecutable = executables[static_cast<size_t>(i)].get();
			if (i % 2 == 0)
			{
				if (pexecutor->submit_recv_multishot(pexecutable->fd, pexecutable) == false) ++count_submit_failed;
			}
			else
			{
				pexecutor->post(make_executable([&, pexecutable]() { if (pexecutor->submit_recv_multishot(pexecutable->fd, pexecutable) == false) ++count_submit_failed; }));
			}
		}

		std::vector<std::thread> writers;
		for (int i = 0; i < COUNT_SOCKET; ++i)
			writers.emplace_back([&, i]() { EXPECT_TRUE(cq_write_all(fds[static_cast<size_t>(i)][1], texts[static_cast<size_t>(i)])); });

		for (int i = 0; i < COUNT_SOCKET; ++i)
		{
			auto* pexecutable = executables[static_cast<size_t>(i)].get();
			EXPECT_TRUE(cq_wait_until([&]() { return pexecutable->received_size() == SIZE_STREAM; }, 30000ms)) << "socket " << i << " received " << pexecutable->received_size() << " " << pexecutable->dump();
		}

		for (auto& writer : writers)
			writer.join();

		EXPECT_EQ(count_submit_failed.load(), 0);

		int count_enobufs = 0;
		for (int i = 0; i < COUNT_SOCKET; ++i)
		{
			auto* pexecutable = executables[static_cast<size_t>(i)].get();
			EXPECT_TRUE(pexecutable->received() == texts[static_cast<size_t>(i)]) << "socket " << i << " : the data differs";
			EXPECT_EQ(pexecutable->count_concurrent.load(), 0) << "socket " << i;
			EXPECT_EQ(pexecutable->count_resubmit_failed.load(), 0) << "socket " << i;
			count_enobufs += pexecutable->count_enobufs.load();
		}
		std::cout << "[          ] multishot : " << COUNT_SOCKET << " sockets x " << SIZE_STREAM << " bytes, -ENOBUFS " << count_enobufs << std::endl;

		// - the end : the peers close (the recvs end with 0), every reference released
		for (int i = 0; i < COUNT_SOCKET; ++i)
			::close(fds[static_cast<size_t>(i)][1]);

		for (int i = 0; i < COUNT_SOCKET; ++i)
		{
			auto* pexecutable = executables[static_cast<size_t>(i)].get();
			EXPECT_TRUE(cq_wait_until([&]() { return pexecutable->reference_count() == 1; })) << "socket " << i << " " << pexecutable->dump();
			::close(fds[static_cast<size_t>(i)][0]);
		}
	}

	pexecutor->stop();
	EXPECT_EQ(pms->reference_count(), 1);
}



//-----------------------------------------------------------------------------
// 7. the open file limit raised after the io_context was created (the net socket initialization raises it to the hard limit) :
//    the fds above the old limit work (the handle table of the backend was sized by the limit at create() : their ops were refused)
//-----------------------------------------------------------------------------
TEST(io_context_uring_cq, fd_above_the_limit_at_create)
{
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel (6.0+)";

	rlimit limit_before{};
	ASSERT_EQ(::getrlimit(RLIMIT_NOFILE, &limit_before), 0);

	constexpr rlim_t LIMIT_LOW = 256;
	constexpr int FD_HIGH = 700;

	if (limit_before.rlim_max != RLIM_INFINITY && limit_before.rlim_max < static_cast<rlim_t>(FD_HIGH + 16))
		GTEST_SKIP() << "the hard limit of open files is too low (" << limit_before.rlim_max << ")";

	// 1) an executor created with a low limit
	rlimit limit_low = limit_before;
	limit_low.rlim_cur = LIMIT_LOW;
	ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit_low), 0);

	auto pexecutor = cq_make_executor("uring_cq limit", 2);

	// 2) the limit raised after (like the net socket initialization)
	rlimit limit_high = limit_before;
	if (limit_high.rlim_cur < static_cast<rlim_t>(FD_HIGH + 16))
		limit_high.rlim_cur = static_cast<rlim_t>(FD_HIGH + 16);
	ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit_high), 0);

	ASSERT_TRUE(pexecutor.exist());

	// 3) a socketpair moved above the old limit : recv, cancel_fd and wait_cancelled_fd on it
	int fd_low[2];
	ASSERT_TRUE(cq_socketpair(fd_low));
	const int fd_high = ::fcntl(fd_low[0], F_DUPFD_CLOEXEC, FD_HIGH);
	ASSERT_GE(fd_high, FD_HIGH);
	::close(fd_low[0]);

	char buffer[16];
	auto precv = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd_high, buffer, sizeof(buffer), 0, precv.get())) << "fd " << fd_high;
	ASSERT_EQ(::write(fd_low[1], "high", 4), 4);
	ASSERT_TRUE(precv->wait(1)) << precv->dump();
	EXPECT_EQ(precv->at(0).result, 4);

	auto precv_cancel = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->submit_recv(fd_high, buffer, sizeof(buffer), 0, precv_cancel.get()));
	EXPECT_TRUE(pexecutor->wait_cancelled_fd(fd_high, 2s)) << precv_cancel->dump();
	ASSERT_TRUE(precv_cancel->wait(1)) << precv_cancel->dump();
	EXPECT_EQ(precv_cancel->at(0).result, -ECANCELED);

	// 4) a readiness poll on it
	auto ppoll = make_object<cq_executable>();
	ASSERT_TRUE(pexecutor->attach(fd_high, ppoll.get(), EPOLLIN | EPOLLONESHOT));
	ASSERT_EQ(::write(fd_low[1], "p", 1), 1);
	ASSERT_TRUE(ppoll->wait(1)) << ppoll->dump();
	EXPECT_TRUE(pexecutor->detach(fd_high));

	::close(fd_high);
	::close(fd_low[1]);

	pexecutor->stop();

	ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit_before), 0);
}

}

#endif
