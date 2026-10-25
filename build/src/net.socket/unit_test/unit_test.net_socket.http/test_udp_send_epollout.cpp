#include "pch.h"

//-----------------------------------------------------------------------------
// linux UDP send : the wait for EPOLLOUT (sendable::Nudp, epoll / io_uring readiness)
//
//  - a thread that meets EAGAIN queues the datagram and registers EPOLLOUT. the thread of the EPOLLOUT event takes the queue over and
//    removes EPOLLOUT. the flag (m_flag_epollout) changed under the lock but set_event() ran outside it : the 'off' of a thread that had
//    sent everything could land after the 'on' of the next thread that met EAGAIN => EPOLLOUT was not registered while the queue waited
//    and the socket sent nothing any more (every send after it was only queued).
//  - loopback UDP never returns EAGAIN (the kernel releases the send buffer in the sendto call) : this file replaces sendto / sendmmsg
//    of the test executable (the CGDK libraries are static) and returns EAGAIN for one socket now and then. the socket stays writable,
//    so a registered EPOLLOUT fires at once : every EAGAIN goes through the wait and the take over on another thread.
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <dlfcn.h>
#include <atomic>
#include <thread>

namespace
{
	std::atomic<int>			g_usend_fd{ -1 };				// - the socket that meets EAGAIN (-1 : none)
	std::atomic<std::size_t>	g_usend_sent{ 0 };				// - the datagrams of that socket the kernel took
	std::atomic<std::size_t>	g_usend_eagain{ 0 };			// - the EAGAINs returned

	// - EAGAIN once in 4 calls of a thread (a calling thread has its own sequence)
	bool usend_is_eagain() noexcept
	{
		thread_local uint32_t seed = 0x9e3779b9u ^ static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		return (seed & 3) == 0;
	}
}

extern "C" ssize_t sendto(int _fd, const void* _buf, size_t _n, int _flags, const struct sockaddr* _addr, socklen_t _addr_len)
{
	static const auto preal = reinterpret_cast<ssize_t(*)(int, const void*, size_t, int, const struct sockaddr*, socklen_t)>(::dlsym(RTLD_NEXT, "sendto"));

	if (_fd != g_usend_fd.load(std::memory_order_relaxed))
		return preal(_fd, _buf, _n, _flags, _addr, _addr_len);

	if (usend_is_eagain())
	{
		++g_usend_eagain;
		errno = EAGAIN;
		return -1;
	}

	const auto result = preal(_fd, _buf, _n, _flags, _addr, _addr_len);

	if (result >= 0)
		++g_usend_sent;

	return result;
}

extern "C" int sendmmsg(int _fd, struct mmsghdr* _vmessages, unsigned int _vlen, int _flags)
{
	static const auto preal = reinterpret_cast<int(*)(int, struct mmsghdr*, unsigned int, int)>(::dlsym(RTLD_NEXT, "sendmmsg"));

	if (_fd != g_usend_fd.load(std::memory_order_relaxed))
		return preal(_fd, _vmessages, _vlen, _flags);

	if (usend_is_eagain())
	{
		++g_usend_eagain;
		errno = EAGAIN;
		return -1;
	}

	const auto result = preal(_fd, _vmessages, _vlen, _flags);

	if (result > 0)
		g_usend_sent += static_cast<std::size_t>(result);

	return result;
}

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	class usend_socket : public net::socket::udp
	{
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
	};

	// - std::threads send datagrams on one UDP socket of an executor of _backend at once. every datagram must reach the kernel
	void usend_check_epollout_wait(executor::io_backend _backend)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("udp send epollout", 8);
		executor::set_default_io_backend(backend_before);
		ASSERT_TRUE(pexecutor.exist());

		// - the destination : a socket nobody reads (the kernel drops what its buffer does not hold)
		const int handle_sink = ::socket(AF_INET, SOCK_DGRAM, 0);
		ASSERT_GE(handle_sink, 0);

		::sockaddr_in address_sink{};
		address_sink.sin_family = AF_INET;
		address_sink.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(::bind(handle_sink, reinterpret_cast<::sockaddr*>(&address_sink), sizeof(address_sink)), 0);
		socklen_t length = sizeof(address_sink);
		ASSERT_EQ(::getsockname(handle_sink, reinterpret_cast<::sockaddr*>(&address_sink), &length), 0);

		// - the sending socket
		auto psocket = make_own<usend_socket>();
		psocket->io_context(pexecutor.get());
		ASSERT_EQ(psocket->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);

		const net::sockaddr address{ net::ip::address_v4::loopback(), ntohs(address_sink.sin_port) };
		const auto buffer = alloc_shared_buffer(32) ^ 32;

		constexpr int count_thread = 4;
		constexpr int count_per_thread = 30000;
		constexpr std::size_t count_total = static_cast<std::size_t>(count_thread) * count_per_thread;

		g_usend_sent = 0;
		g_usend_eagain = 0;
		g_usend_fd = static_cast<int>(psocket->native_handle());

		// - send
		std::vector<std::thread> threads;
		std::atomic<int> count_failed{ 0 };

		for (int t = 0; t < count_thread; ++t)
		{
			threads.emplace_back([&]()
			{
				for (int i = 0; i < count_per_thread; ++i)
				{
					if (psocket->send_to(buffer, address) == false)
						++count_failed;
				}
			});
		}

		for (auto& iter : threads)
			iter.join();

		// - every datagram reaches the kernel (a lost EPOLLOUT : the queue waits for ever and the count stops)
		const bool is_all_sent = [&]()
		{
			const auto time_end = std::chrono::steady_clock::now() + 10s;
			while (std::chrono::steady_clock::now() < time_end)
			{
				if (g_usend_sent.load() == count_total)
					return true;
				std::this_thread::sleep_for(10ms);
			}
			return g_usend_sent.load() == count_total;
		}();

		g_usend_fd = -1;

		EXPECT_EQ(count_failed.load(), 0);
		EXPECT_TRUE(is_all_sent) << "sent " << g_usend_sent.load() << " / " << count_total << " (EAGAIN " << g_usend_eagain.load() << ")";
		EXPECT_NE(g_usend_eagain.load(), 0u);

		psocket->stop();
		pexecutor->stop();
		::close(handle_sink);
	}
}

TEST(udp_send_epollout, many_threads_epoll)
{
	usend_check_epollout_wait(executor::io_backend::epoll);
}

TEST(udp_send_epollout, many_threads_io_uring)
{
	usend_check_epollout_wait(executor::io_backend::io_uring);
}

TEST(udp_send_epollout, many_threads_io_uring_completion)
{
	usend_check_epollout_wait(executor::io_backend::io_uring_completion);
}

}

#endif
