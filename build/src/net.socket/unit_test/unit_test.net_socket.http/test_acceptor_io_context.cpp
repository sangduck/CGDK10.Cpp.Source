#include "pch.h"
#include <atomic>
#include <mutex>
#include <thread>

//-----------------------------------------------------------------------------
// the io_context of an accepted socket
//
//  - process_get_connectable_io_context() first (an executor per socket, cpu affinity), else the io_context of the acceptor,
//    else the system executor (like windows). linux gave the system executor to the sockets of an acceptor given an executor.
//  - the default backend of the process (CGDK_IO_URING=io_uring / completion : the executor made here has it too)
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <arpa/inet.h>
#include <netinet/tcp.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - the port of the tests (28900 + pid % 300 : below the ephemeral range, not the ports of the other tests)
	uint16_t aic_port()
	{
		return static_cast<uint16_t>(28900 + api::get_process_id() % 300);
	}

	std::mutex						g_aic_lockable;
	std::vector<executor::Iio_context*> g_aic_io_contexts;		// - the io_context of every socket at on_connect / on_message
	std::atomic<int>				g_aic_messages{ 0 };

	class aic_server_socket : public net::socket::tcp<>
	{
	protected:
		virtual void on_connect() override
		{
			std::lock_guard lock(g_aic_lockable);
			g_aic_io_contexts.push_back(this->io_context().get());
		}

		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			{
				std::lock_guard lock(g_aic_lockable);
				g_aic_io_contexts.push_back(this->io_context().get());
			}
			++g_aic_messages;
			this->send(_msg.buf_message);
			return eRESULT::DONE;
		}
	};

	// - a blocking client of the OS : connects, an echo of 8 bytes
	bool aic_echo(uint16_t _port)
	{
		const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(_port);
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		bool result = (::connect(fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0);
		if (result)
		{
			const timeval timeout{ 5, 0 };
			::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			char message[8]{};
			const uint32_t size = 8;
			memcpy(message, &size, 4);
			result = (::send(fd, message, 8, MSG_NOSIGNAL) == 8);
			std::size_t received = 0;
			while (result && received < 8)
			{
				const auto r = ::recv(fd, message + received, 8 - received, 0);
				if (r < 0 && errno == EINTR)
					continue;
				result = (r > 0);
				if (result)
					received += static_cast<std::size_t>(r);
			}
		}
		::close(fd);
		return result;
	}

	// - the io_contexts the server sockets ran on, then cleared
	std::vector<executor::Iio_context*> aic_take()
	{
		std::lock_guard lock(g_aic_lockable);
		auto result = std::move(g_aic_io_contexts);
		g_aic_io_contexts.clear();
		return result;
	}

	auto aic_parameter(uint16_t _port)
	{
		net::acceptor<aic_server_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _port };
		parameter.enable_progress_log = false;
		return parameter;
	}
}

TEST(acceptor_io_context, the_executor_of_the_acceptor)
{
	aic_take();

	// 1) an executor of its own (the backend of the process)
	auto pexecutor = executor::make_io_context_threads("acceptor io_context executor", 2);
	ASSERT_TRUE(pexecutor.exist());
	auto* pio_context_acceptor = static_cast<executor::Iio_context*>(pexecutor.get());

	auto pacceptor = make_own<net::acceptor<aic_server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(aic_parameter(aic_port())), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) the accepted sockets run on it (on_connect and on_message), not on the system executor
	constexpr int count = 8;
	for (int i = 0; i < count; ++i)
		ASSERT_TRUE(aic_echo(port)) << "echo " << i;

	const auto io_contexts = aic_take();
	EXPECT_EQ(io_contexts.size(), static_cast<std::size_t>(count * 2));
	for (auto* pio_context : io_contexts)
	{
		EXPECT_EQ(pio_context, pio_context_acceptor);
		EXPECT_NE(pio_context, system_executor::executor_ptr_peek());
	}

	EXPECT_TRUE([&]() { for (int i = 0; i < 500 && pacceptor->count() != 0; ++i) std::this_thread::sleep_for(10ms); return pacceptor->count() == 0; }()) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(acceptor_io_context, the_system_executor_by_default)
{
	aic_take();

	// - an acceptor without an io_context : the system executor (its own and the sockets')
	auto pacceptor = make_own<net::acceptor<aic_server_socket>>();
	ASSERT_EQ(pacceptor->start(aic_parameter(static_cast<uint16_t>(aic_port() + 1))), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	for (int i = 0; i < 4; ++i)
		ASSERT_TRUE(aic_echo(port)) << "echo " << i;

	const auto io_contexts = aic_take();
	EXPECT_EQ(io_contexts.size(), 8u);
	for (auto* pio_context : io_contexts)
		EXPECT_EQ(pio_context, system_executor::executor_ptr_peek());

	EXPECT_TRUE([&]() { for (int i = 0; i < 500 && pacceptor->count() != 0; ++i) std::this_thread::sleep_for(10ms); return pacceptor->count() == 0; }()) << "connected yet " << pacceptor->count();
	pacceptor->stop();
}

}

#endif
