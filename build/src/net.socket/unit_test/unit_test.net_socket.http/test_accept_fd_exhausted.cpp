#include "pch.h"

//-----------------------------------------------------------------------------
// linux acceptor : accept with no fd left (EMFILE)
//
//  - the pending connection stayed in the backlog and the listen socket stayed readable : the level-triggered event (epoll / io_uring
//    readiness) or the multishot accept posted again (io_uring completion) came back at once and the workers spun (CPU 100%) until a fd
//    was freed.
//  - now the acceptor closes the pending connections with a spare fd : the clients see their connection closed and the CPU is idle.
//  - io_uring completion (the multishot ACCEPT op) is not checked here : its accepts got fds under the lowered limit in this test
//    (kernel 7.0), so it never met EMFILE. the CQE of an EMFILE goes through the same rejection (net.io.connective.acceptor_tcp.uring.cpp)
//  - here the free fd numbers are filled and RLIMIT_NOFILE is lowered right above them for a second (no new fd in the process), clients connect meanwhile.
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <atomic>
#include <filesystem>
#include <fcntl.h>
#include <thread>
#include <sys/resource.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	class afe_server_socket : public net::socket::tcp<>
	{
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
	};

	// - fills the free fd numbers up to the highest one open in this process (/dev/null) : with the soft limit right above the last,
	//   the process gets no new fd (until one is closed)
	std::vector<int> afe_fill_fds()
	{
		int fd_max = 0;
		std::error_code error;
		for (const auto& entry : std::filesystem::directory_iterator("/proc/self/fd", error))
			fd_max = std::max(fd_max, std::atoi(entry.path().filename().c_str()));

		std::vector<int> fds;
		for (;;)
		{
			const int fd = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
			if (fd < 0)
				break;
			fds.push_back(fd);
			if (fd >= fd_max)
				break;
		}
		return fds;
	}

	double afe_cpu_seconds()
	{
		rusage usage{};
		::getrusage(RUSAGE_SELF, &usage);
		return static_cast<double>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) + static_cast<double>(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1e6;
	}

	int afe_connect_nonblocking(int _handle, uint16_t _port)
	{
		::sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(_port);
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		return ::connect(_handle, reinterpret_cast<::sockaddr*>(&address), sizeof(address));
	}

	void afe_check_accept_fd_exhausted(executor::io_backend _backend)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("accept fd exhausted", 4);
		executor::set_default_io_backend(backend_before);
		ASSERT_TRUE(pexecutor.exist());

		// 1) server
		auto pacceptor = make_own<net::acceptor<afe_server_socket>>();
		pacceptor->io_context(pexecutor.get());

		net::acceptor<afe_server_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(35400 + api::get_process_id() % 500) };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
		const auto port = pacceptor->local_endpoint().port();

		// 2) a connection accepted as usual (the acceptor opens its spare fd at the first accept)
		const int handle_first = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(handle_first, 0);
		ASSERT_EQ(afe_connect_nonblocking(handle_first, port), 0);
		ASSERT_TRUE([&]() { for (int i = 0; i < 500 && pacceptor->count() != 1; ++i) std::this_thread::sleep_for(10ms); return pacceptor->count() == 1; }());

		// 3) the client sockets (made before the limit)
		constexpr int COUNT_CLIENT = 8;
		int handles[COUNT_CLIENT];
		for (auto& iter : handles)
		{
			iter = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
			ASSERT_GE(iter, 0);
		}

		// 4) no new fd in this process : the free numbers filled, the soft limit right above them
		rlimit limit_before{};
		ASSERT_EQ(::getrlimit(RLIMIT_NOFILE, &limit_before), 0);

		const auto fds_filled = afe_fill_fds();
		ASSERT_FALSE(fds_filled.empty());

		rlimit limit = limit_before;
		limit.rlim_cur = static_cast<rlim_t>(fds_filled.back() + 1);
		ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit), 0);

		// 5) the clients connect (to the backlog : the accept fails with EMFILE), the CPU of the process for a second
		for (auto iter : handles)
			afe_connect_nonblocking(iter, port);

		const double cpu_begin = afe_cpu_seconds();
		std::this_thread::sleep_for(1s);
		const double cpu_used = afe_cpu_seconds() - cpu_begin;

		// - the connections closed by the acceptor (EOF or RST)
		int count_closed = 0;
		for (auto iter : handles)
		{
			char buffer[16];
			const auto result = ::recv(iter, buffer, sizeof(buffer), 0);
			if (result == 0 || (result < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
				++count_closed;
		}

		const auto count_accepted = pacceptor->count();

		// 6) the limit back
		ASSERT_EQ(::setrlimit(RLIMIT_NOFILE, &limit_before), 0);

		for (auto iter : fds_filled)
			::close(iter);

		EXPECT_LT(cpu_used, 0.3) << "the workers spun on the listen socket";
		EXPECT_EQ(count_closed, COUNT_CLIENT);
		EXPECT_EQ(count_accepted, 1u);

		// - the acceptor accepts again
		const int handle_after = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(handle_after, 0);
		ASSERT_EQ(afe_connect_nonblocking(handle_after, port), 0);
		EXPECT_TRUE([&]() { for (int i = 0; i < 500 && pacceptor->count() != 2; ++i) std::this_thread::sleep_for(10ms); return pacceptor->count() == 2; }()) << "accepted " << pacceptor->count();

		for (auto iter : handles)
			::close(iter);
		::close(handle_first);
		::close(handle_after);

		pacceptor->stop();
		pexecutor->stop();
	}
}

TEST(accept_fd_exhausted, epoll)
{
	afe_check_accept_fd_exhausted(executor::io_backend::epoll);
}

TEST(accept_fd_exhausted, io_uring)
{
	afe_check_accept_fd_exhausted(executor::io_backend::io_uring);
}

}

#endif
