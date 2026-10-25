#include "pch.h"

//-----------------------------------------------------------------------------
// linux epoll / io_uring readiness : an EPOLLOUT posted while the socket processes an event (socket::Ntcp::process_execute_epoll)
//
//  - post_epoll_event_EPOLLOUT() while the event of the socket is processed only adds EPOLLOUT to the mask : the end of the processing
//    gives the mask to the kernel (MOD) when it differs from the one registered. a writable socket under EPOLLET gives EPOLLOUT only
//    through that MOD (no new edge). while an EPOLLOUT event was processed the registered mask still had EPOLLOUT : EPOLLOUT asked
//    for again in it (returned by the sendable with messages left, or posted by another thread meanwhile) made the same mask, no MOD,
//    and the event never came : the sendable stalled
//  - here the server posts EPOLLOUT in on_message() of every message (the socket is writable), asks for it again in the first EPOLLOUT
//    event and counts the events : the client sends the next message after the two events of the last one came
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <atomic>
#include <thread>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	std::atomic<int> g_eor_count_posted{ 0 };
	std::atomic<int> g_eor_count_epollout{ 0 };
	std::atomic<bool> g_eor_connected{ false };

	class eor_server_socket : public net::socket::tcp<>
	{
	protected:
		virtual void				on_connect() override { g_eor_connected = true; }
		virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { g_eor_connected = false; }
		virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override
		{
			// - in the receive event of the socket (processing) : EPOLLOUT on a writable socket
			++g_eor_count_posted;
			this->post_epoll_event_EPOLLOUT();
			return eRESULT::DONE;
		}
		// - the EPOLLOUT event (nothing is sent by this socket : the sendable is not called). the first of a message asks for EPOLLOUT again
		//   (like a sendable with messages left : it is returned while the socket is still writable)
		virtual uint32_t			process_recover_sendable(intptr_t /*_event*/, std::size_t /*_param*/) override
		{
			return ((++g_eor_count_epollout % 2) == 1) ? EPOLLOUT : 0;
		}
	};

	void eor_check(executor::io_backend _backend)
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("epollout rearm", 4);
		executor::set_default_io_backend(backend_before);
		ASSERT_TRUE(pexecutor.exist());

		if (pexecutor->backend() == executor::io_backend::io_uring_completion || (_backend == executor::io_backend::io_uring && pexecutor->backend() != _backend))
		{
			pexecutor->stop();
			GTEST_SKIP() << "the backend is not available";
		}

		g_eor_count_posted = 0;
		g_eor_count_epollout = 0;
		g_eor_connected = false;

		auto pacceptor = make_own<net::acceptor<eor_server_socket>>();
		pacceptor->io_context(pexecutor.get());

		net::acceptor<eor_server_socket>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(36900 + api::get_process_id() % 500) };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);

		const int handle = ::socket(AF_INET, SOCK_STREAM, 0);
		ASSERT_GE(handle, 0);

		::sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(pacceptor->local_endpoint().port());
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		ASSERT_EQ(::connect(handle, reinterpret_cast<::sockaddr*>(&address), sizeof(address)), 0);

		ASSERT_TRUE([&]() { for (int i = 0; i < 500 && g_eor_connected.load() == false; ++i) std::this_thread::sleep_for(10ms); return g_eor_connected.load(); }());

		// - a message, then its two EPOLLOUT events (the posted one, the one asked for in it), 20 times
		char message[16] = {};
		const uint32_t size = sizeof(message);
		memcpy(message, &size, 4);

		int count_lost = 0;
		for (int i = 0; i < 20 && count_lost == 0; ++i)
		{
			ASSERT_EQ(::send(handle, message, sizeof(message), MSG_NOSIGNAL), static_cast<ssize_t>(sizeof(message))) << "message " << i;

			const auto time_end = std::chrono::steady_clock::now() + 2s;
			while (g_eor_count_epollout.load() < (i + 1) * 2 && std::chrono::steady_clock::now() < time_end)
				std::this_thread::sleep_for(1ms);

			if (g_eor_count_epollout.load() < (i + 1) * 2)
				++count_lost;
		}

		EXPECT_EQ(count_lost, 0) << "EPOLLOUT events " << g_eor_count_epollout.load() << " for " << g_eor_count_posted.load() << " posted";

		::close(handle);
		pacceptor->stop();
		pexecutor->stop();
	}
}

TEST(epollout_rearm, epoll)
{
	eor_check(executor::io_backend::epoll);
}

TEST(epollout_rearm, io_uring_readiness)
{
	eor_check(executor::io_backend::io_uring);
}

}

#endif
