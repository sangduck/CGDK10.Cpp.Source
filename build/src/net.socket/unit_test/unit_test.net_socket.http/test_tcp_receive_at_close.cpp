#include "pch.h"

//-----------------------------------------------------------------------------
// linux TCP receive : the data that came with the FIN (epoll / io_uring readiness / io_uring completion)
//
//  - an event receives at most MAX_RECEIVE_PER_EVENT full windows (8 : ~390KB with 64KB windows) and gives the others their turn.
//    an event with EPOLLRDHUP closes the socket after the receive, so the data left in the socket buffer was never received :
//    the messages a peer sent just before it shut down (a request and a close, a last batch of a log) were lost and the socket
//    was closed with unread data (RST).
//  - here the server stops in a handler while the client sends the rest and shuts down : the next event carries the rest of the data
//    (several MB : a large receive buffer) and the FIN.
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <atomic>
#include <thread>
#include <netinet/tcp.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	constexpr std::size_t	RAC_SIZE_MESSAGE = 1000;
	constexpr std::size_t	RAC_COUNT_MESSAGE = 12000;				// - 12MB
	constexpr std::size_t	RAC_COUNT_BEFORE_STOP = 4000;			// - the server stops in the handler of this message (4MB read)

	// - the messages the server received (the socket goes back to the pool when it is closed)
	std::atomic<std::size_t>	g_rac_bytes_received{ 0 };
	std::atomic<std::size_t>	g_rac_count_received{ 0 };

	class rac_server_socket : public net::socket::tcp<>
	{
	protected:
		// - a large receive buffer (net.core.rmem_max caps it) : the rest of the data waits in it while the server stops
		virtual void				on_connect() override
		{
			const int size = 4 * 1024 * 1024;
			::setsockopt(static_cast<int>(this->native_handle()), SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
		}
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			g_rac_bytes_received += _msg.buf_message.size();

			// - stop for a while : the client sends the rest and shuts down meanwhile
			if (++g_rac_count_received == RAC_COUNT_BEFORE_STOP)
				std::this_thread::sleep_for(1s);

			return eRESULT::DONE;
		}
	};

	void rac_check_receive_at_close(executor::io_backend _backend)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("tcp receive at close", 4);
		executor::set_default_io_backend(backend_before);
		ASSERT_TRUE(pexecutor.exist());

		g_rac_bytes_received = 0;
		g_rac_count_received = 0;

		// 1) server
		auto pacceptor = make_own<net::acceptor<rac_server_socket>>();
		pacceptor->io_context(pexecutor.get());

		net::acceptor<rac_server_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(34400 + api::get_process_id() % 500) };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
		const auto port = pacceptor->local_endpoint().port();

		// 2) client : all the messages, then shutdown (FIN), then waits for the server to close
		std::string data(RAC_SIZE_MESSAGE * RAC_COUNT_MESSAGE, '\0');
		for (std::size_t i = 0; i < RAC_COUNT_MESSAGE; ++i)
		{
			auto* p = data.data() + i * RAC_SIZE_MESSAGE;
			const uint32_t size = static_cast<uint32_t>(RAC_SIZE_MESSAGE);
			const uint32_t index = static_cast<uint32_t>(i);
			memcpy(p, &size, 4);
			memcpy(p + 4, &index, 4);
		}

		const int handle = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(handle, 0);

		::sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(port);
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(::connect(handle, reinterpret_cast<::sockaddr*>(&address), sizeof(address)), 0);

		for (std::size_t sent = 0; sent < data.size(); )
		{
			const auto result = ::send(handle, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
			ASSERT_GT(result, 0) << "send errno " << errno;
			sent += static_cast<std::size_t>(result);
		}

		ASSERT_EQ(::shutdown(handle, SHUT_WR), 0);

		// 3) the server closes after it received everything (EOF : 0, closed with unread data : RST)
		timeval timeout{ 10, 0 };
		::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		char buffer[256];
		const auto result_read = ::recv(handle, buffer, sizeof(buffer), 0);
		const int error_read = (result_read < 0) ? errno : 0;
		::close(handle);

		ASSERT_TRUE([&]()
		{
			const auto time_end = std::chrono::steady_clock::now() + 10s;
			while (std::chrono::steady_clock::now() < time_end)
			{
				if (pacceptor->count() == 0)
					return true;
				std::this_thread::sleep_for(10ms);
			}
			return false;
		}()) << "connected yet";

		EXPECT_EQ(g_rac_count_received.load(), RAC_COUNT_MESSAGE) << "bytes " << g_rac_bytes_received.load() << " / " << RAC_SIZE_MESSAGE * RAC_COUNT_MESSAGE;
		EXPECT_EQ(result_read, 0) << "the server closed with unread data (errno " << error_read << ")";

		pacceptor->stop();
		pexecutor->stop();
	}
}

TEST(tcp_receive_at_close, epoll)
{
	rac_check_receive_at_close(executor::io_backend::epoll);
}

TEST(tcp_receive_at_close, io_uring)
{
	rac_check_receive_at_close(executor::io_backend::io_uring);
}

TEST(tcp_receive_at_close, io_uring_completion)
{
	rac_check_receive_at_close(executor::io_backend::io_uring_completion);
}

}

#endif
