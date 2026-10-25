#include "pch.h"

//-----------------------------------------------------------------------------
// linux epoll / io_uring readiness : the registration mode of a TCP socket (Isocket.h, socket::Ntcp::process_execute_epoll)
//
//  - small receives : EPOLLET (no epoll_ctl(MOD) per event). large receives : EPOLLONESHOT (no wake up of another thread for every
//    segment of a large message). back to EPOLLET after EPOLL_COUNT_SMALL_TO_EDGE events of small receives in a row
//  - here one connection goes large -> small (ping-pong, then a burst) -> large -> small : every message comes back in every phase and
//    the mode of the server socket follows (the switch back to EPOLLET was never run by the benches)
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <atomic>
#include <thread>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	std::atomic<net::io::Isocket*> g_erm_server{ nullptr };

	class erm_server_socket : public net::socket::tcp<>
	{
	protected:
		virtual void				on_connect() override { g_erm_server = this; }
		virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { g_erm_server = nullptr; }
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			this->send(_msg.buf_message);
			return eRESULT::DONE;
		}
	};

	bool erm_send_all(int _handle, const std::string& _data)
	{
		for (std::size_t sent = 0; sent < _data.size(); )
		{
			const auto result = ::send(_handle, _data.data() + sent, _data.size() - sent, MSG_NOSIGNAL);
			if (result <= 0)
				return false;
			sent += static_cast<std::size_t>(result);
		}
		return true;
	}

	bool erm_receive_all(int _handle, std::size_t _bytes)
	{
		std::string buffer(64 * 1024, '\0');
		for (std::size_t received = 0; received < _bytes; )
		{
			const auto result = ::recv(_handle, buffer.data(), std::min(buffer.size(), _bytes - received), 0);
			if (result <= 0)
				return false;
			received += static_cast<std::size_t>(result);
		}
		return true;
	}

	std::string erm_messages(std::size_t _size, int _count)
	{
		std::string message(_size, 'x');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);

		std::string data;
		for (int i = 0; i < _count; ++i)
			data += message;
		return data;
	}

	// - the mode of the server socket once its events are done (read under its event lock)
	bool erm_is_oneshot()
	{
		auto* psocket = g_erm_server.load();
		if (psocket == nullptr)
			return false;

		std::this_thread::sleep_for(50ms);

		std::lock_guard lock(psocket->m_epoll_event_cs);
		return psocket->m_epoll_event_oneshot;
	}

	void erm_check_receive_mode(executor::io_backend _backend)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("epoll receive mode", 4);
		executor::set_default_io_backend(backend_before);
		ASSERT_TRUE(pexecutor.exist());

		if (pexecutor->backend() == executor::io_backend::io_uring_completion || (_backend == executor::io_backend::io_uring && pexecutor->backend() != _backend))
		{
			pexecutor->stop();
			GTEST_SKIP() << "the backend is not available";
		}

		g_erm_server = nullptr;

		auto pacceptor = make_own<net::acceptor<erm_server_socket>>();
		pacceptor->io_context(pexecutor.get());

		net::acceptor<erm_server_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(36400 + api::get_process_id() % 500) };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);

		const int handle = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(handle, 0);

		timeval timeout{ 10, 0 };
		::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		::sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(pacceptor->local_endpoint().port());
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(::connect(handle, reinterpret_cast<::sockaddr*>(&address), sizeof(address)), 0);

		ASSERT_TRUE([&]() { for (int i = 0; i < 500 && g_erm_server.load() == nullptr; ++i) std::this_thread::sleep_for(10ms); return g_erm_server.load() != nullptr; }());

		// - a phase : the messages sent at once (a thread) and received back
		const auto round_trip = [&](std::size_t _size, int _count) -> bool
		{
			const auto data = erm_messages(_size, _count);
			std::thread sender([&]() { erm_send_all(handle, data); });
			const bool is_received = erm_receive_all(handle, data.size());
			sender.join();
			return is_received;
		};

		// 1) small first : EPOLLET
		ASSERT_TRUE(round_trip(64, 100));
		EXPECT_FALSE(erm_is_oneshot()) << "small receives";

		// 2) large : EPOLLONESHOT
		ASSERT_TRUE(round_trip(64 * 1024, 64));
		EXPECT_TRUE(erm_is_oneshot()) << "large receives";

		// 3) small one by one (an event each) : back to EPOLLET after EPOLL_COUNT_SMALL_TO_EDGE events
		for (int i = 0; i < 16; ++i)
			ASSERT_TRUE(round_trip(64, 1)) << "small " << i;
		EXPECT_FALSE(erm_is_oneshot()) << "back to EPOLLET";

		// 4) a burst of small and large again, then small : everything comes back
		ASSERT_TRUE(round_trip(100, 2000));
		ASSERT_TRUE(round_trip(64 * 1024, 32));
		EXPECT_TRUE(erm_is_oneshot());
		for (int i = 0; i < 16; ++i)
			ASSERT_TRUE(round_trip(64, 1)) << "small again " << i;
		EXPECT_FALSE(erm_is_oneshot());
		ASSERT_TRUE(round_trip(1000, 500));

		::close(handle);
		pacceptor->stop();
		pexecutor->stop();
	}
}

TEST(epoll_receive_mode, large_to_small_epoll)
{
	erm_check_receive_mode(executor::io_backend::epoll);
}

TEST(epoll_receive_mode, large_to_small_io_uring)
{
	erm_check_receive_mode(executor::io_backend::io_uring);
}

}

#endif
