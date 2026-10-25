#include "pch.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// windows IOCP sockets
//
// 1. UDP (net::socket::udp : receivable::Ndatagram, sendable::Nudp)
//
//  - port_unreachable_reported : with 'enable_report_connect_reset' a datagram sent to a closed port completes a receive with
//    STATUS_PORT_UNREACHABLE : the socket reports it (a message with an empty buffer). the status was read with api::get_last_error()
//    after GetQueuedCompletionStatusEx (not set by it) : the report never came
//  - zero_length_datagram : a datagram of 0 bytes is legal : it is a message (a buffer of 0 bytes), not a failure of the receive
//  - send_statistics : the sends (direct WSASendTo, the overlapped one when the socket buffer is full) count the messages and the bytes
//    and end with no send in flight
//  - send_while_close : threads send while the socket is closed and sockets are made right after (they may take the handle value of
//    the closed one) : nothing is sent from them (the handle is read under the lock the close swaps it under)
//
// 2. TCP
//  - async_sending_{gather,buffered,no_gather} : the overlapped sends in flight (statistics_get_async_sending()) go back to 0 when the
//    sends completed (the gather sendables never decreased it, Ntcp_async counted the completion as another send : the socket that had
//    sent never had statistics_get_now_async_io() == 0 : the zombie check of the acceptor stop / emergency acceptor)
//  - receive_sizes : messages of 8B ~ 200KB in a mixed order arrive whole and in order (the receive buffer is reused, moved, grown, shrunk)
//-----------------------------------------------------------------------------
#if defined(_WINSOCK2API_)

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	template <class TCONDITION>
	bool iu_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
	{
		const auto time_end = std::chrono::steady_clock::now() + _duration;
		while (std::chrono::steady_clock::now() < time_end)
		{
			if (_condition())
				return true;
			std::this_thread::sleep_for(5ms);
		}
		return _condition();
	}

	shared_buffer iu_message(std::size_t _size)
	{
		auto buf = alloc_shared_buffer(_size);
		std::vector<char> message(_size, 'x');
		if (_size >= 4)
		{
			const uint32_t size = static_cast<uint32_t>(_size);
			memcpy(message.data(), &size, 4);
		}
		buf.append(message.size(), message.data());
		return buf;
	}

	// - a raw UDP socket bound to 127.0.0.1:_port (0 : any port)
	SOCKET iu_raw_socket(uint16_t _port = 0)
	{
		SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = htons(_port);
		if (s != INVALID_SOCKET && ::bind(s, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			::closesocket(s);
			return INVALID_SOCKET;
		}
		return s;
	}

	uint16_t iu_port_of(SOCKET _s)
	{
		sockaddr_in addr{};
		int length = sizeof(addr);
		::getsockname(_s, reinterpret_cast<::sockaddr*>(&addr), &length);
		return ntohs(addr.sin_port);
	}

	class iu_udp_socket : public net::socket::udp
	{
	public:
		std::atomic<int>	count_message{ 0 };
		std::atomic<int>	count_empty_source{ 0 };	// - the report of a connect reset (no buffer)
		std::atomic<int>	count_zero_length{ 0 };		// - a datagram of 0 bytes (a buffer of 0 bytes)
		std::atomic<int>	count_fail_receive{ 0 };
		std::mutex			lockable_ports;
		std::vector<uint16_t> vector_port;				// - the source ports of the datagrams

	protected:
		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			if (_msg.buf_message.get_source().empty())
				++this->count_empty_source;
			else if (_msg.buf_message.size() == 0)
				++this->count_zero_length;
			else
			{
				++this->count_message;
				std::lock_guard lock(this->lockable_ports);
				this->vector_port.push_back(_msg.address.port());
			}
			return eRESULT::DONE;
		}
		virtual void on_fail_receive(int /*_error_code*/, const buffer_view& /*_buffer*/, const net::sockaddr& /*_address*/) override
		{
			++this->count_fail_receive;
		}
	};

	net::io::socket::Nudp::START_PARAMETER iu_parameter(bool _report_connect_reset = false)
	{
		net::io::socket::Nudp::START_PARAMETER parameter;
		parameter.bind_endpoint = net::ip::udp::endpoint{ net::ip::address_v4::loopback(), 0 };
		parameter.enable_report_connect_reset = _report_connect_reset;
		return parameter;
	}
}

TEST(iocp_udp, port_unreachable_reported)
{
	auto psocket = make_own<iu_udp_socket>();
	ASSERT_EQ(psocket->start(iu_parameter(true)), eRESULT::SUCCESS);

	// - a closed port (bound and closed)
	const auto raw = iu_raw_socket();
	ASSERT_NE(raw, INVALID_SOCKET);
	const auto port_closed = iu_port_of(raw);
	::closesocket(raw);

	// - send : the ICMP port unreachable completes a receive with STATUS_PORT_UNREACHABLE
	const net::sockaddr address_closed{ net::ip::address_v4::loopback(), port_closed };
	EXPECT_TRUE(psocket->send_to(iu_message(32), address_closed));

	EXPECT_TRUE(iu_wait_until([&] { return psocket->count_empty_source.load() >= 1; }, 3000ms)) << "the connect reset was not reported";

	// - the socket receives after it
	const auto raw_sender = iu_raw_socket();
	ASSERT_NE(raw_sender, INVALID_SOCKET);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(psocket->local_endpoint().port());
	char data[32]{};
	const uint32_t size = sizeof(data);
	memcpy(data, &size, 4);
	EXPECT_EQ(::sendto(raw_sender, data, sizeof(data), 0, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)), static_cast<int>(sizeof(data)));
	EXPECT_TRUE(iu_wait_until([&] { return psocket->count_message.load() == 1; }));
	::closesocket(raw_sender);

	psocket->stop();
}

TEST(iocp_udp, zero_length_datagram)
{
	auto psocket = make_own<iu_udp_socket>();
	ASSERT_EQ(psocket->start(iu_parameter()), eRESULT::SUCCESS);

	const auto raw_sender = iu_raw_socket();
	ASSERT_NE(raw_sender, INVALID_SOCKET);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(psocket->local_endpoint().port());

	// - 3 datagrams of 0 bytes, then one of 32 bytes
	char data[32]{};
	const uint32_t size = sizeof(data);
	memcpy(data, &size, 4);
	for (int i = 0; i < 3; ++i)
		EXPECT_EQ(::sendto(raw_sender, data, 0, 0, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)), 0);
	EXPECT_EQ(::sendto(raw_sender, data, sizeof(data), 0, reinterpret_cast<const ::sockaddr*>(&addr), sizeof(addr)), static_cast<int>(sizeof(data)));

	EXPECT_TRUE(iu_wait_until([&] { return psocket->count_zero_length.load() == 3 && psocket->count_message.load() == 1; }))
		<< "zero length " << psocket->count_zero_length.load() << " message " << psocket->count_message.load() << " fail " << psocket->count_fail_receive.load();
	EXPECT_EQ(psocket->count_fail_receive.load(), 0);
	EXPECT_EQ(psocket->count_empty_source.load(), 0);

	// - the receives are all posted again
	EXPECT_TRUE(iu_wait_until([&] { return psocket->statistics_get_async_receiving() == static_cast<int>(psocket->get_prepare_receive_count()); }));

	::closesocket(raw_sender);
	psocket->stop();
}

TEST(iocp_udp, send_statistics)
{
	auto pserver = make_own<iu_udp_socket>();
	auto pclient = make_own<iu_udp_socket>();
	ASSERT_EQ(pserver->start(iu_parameter()), eRESULT::SUCCESS);
	ASSERT_EQ(pclient->start(iu_parameter()), eRESULT::SUCCESS);

	const net::sockaddr address_server{ net::ip::address_v4::loopback(), pserver->local_endpoint().port() };
	constexpr int COUNT = 2000;
	constexpr std::size_t SIZE = 100;
	const auto message = iu_message(SIZE);

	// - 4 threads send at once (out of the executor)
	std::vector<std::thread> threads;
	std::atomic<int> count_sent{ 0 };
	for (int t = 0; t < 4; ++t)
		threads.emplace_back([&]
		{
			for (int i = 0; i < COUNT / 4; ++i)
			{
				if (pclient->send_to(message, address_server))
					++count_sent;
				if ((i % 64) == 63)
					std::this_thread::sleep_for(1ms);	// - the receiver keeps up (loopback UDP drops when the socket buffer is full)
			}
		});
	for (auto& th : threads) th.join();

	EXPECT_EQ(count_sent.load(), COUNT);
	EXPECT_TRUE(iu_wait_until([&] { return pclient->statistics_get_async_sending() == 0; }));
	EXPECT_TRUE(iu_wait_until([&] { return pserver->count_message.load() == COUNT; })) << "received " << pserver->count_message.load();
	EXPECT_EQ(pclient->statistics_get_sent_message(), static_cast<uint64_t>(COUNT));
	EXPECT_EQ(pclient->statistics_get_sent_bytes(), static_cast<uint64_t>(COUNT) * SIZE);
	EXPECT_EQ(pclient->statistics_get_async_send(), static_cast<uint64_t>(COUNT));

	pclient->stop();
	pserver->stop();
}

TEST(iocp_udp, send_while_close)
{
	auto psink = make_own<iu_udp_socket>();
	ASSERT_EQ(psink->start(iu_parameter()), eRESULT::SUCCESS);
	const net::sockaddr address_sink{ net::ip::address_v4::loopback(), psink->local_endpoint().port() };
	const auto message = iu_message(32);

	std::vector<uint16_t> vector_port_raw;

	for (int round = 0; round < 40; ++round)
	{
		auto psender = make_own<iu_udp_socket>();
		ASSERT_EQ(psender->start(iu_parameter()), eRESULT::SUCCESS);

		std::atomic<bool> is_running{ true };
		std::vector<std::thread> threads;
		for (int t = 0; t < 3; ++t)
			threads.emplace_back([&]
			{
				while (is_running.load(std::memory_order_relaxed))
				{
					psender->send_to(message, address_sink);
					std::this_thread::yield();
				}
			});

		std::this_thread::sleep_for(2ms);

		// - close, then make sockets at once (a handle value closed is given again)
		psender->closesocket();
		std::vector<SOCKET> vector_raw;
		for (int i = 0; i < 8; ++i)
		{
			auto raw = iu_raw_socket();
			if (raw != INVALID_SOCKET)
			{
				vector_port_raw.push_back(iu_port_of(raw));
				vector_raw.push_back(raw);
			}
		}

		std::this_thread::sleep_for(2ms);
		is_running = false;
		for (auto& th : threads) th.join();
		for (auto raw : vector_raw) ::closesocket(raw);
		psender->stop();
	}

	std::this_thread::sleep_for(100ms);

	// - nothing came from the sockets made after the close
	std::lock_guard lock(psink->lockable_ports);
	int count_stolen = 0;
	for (auto port : psink->vector_port)
		for (auto port_raw : vector_port_raw)
			if (port == port_raw)
				++count_stolen;
	EXPECT_EQ(count_stolen, 0);
	EXPECT_GT(psink->vector_port.size(), 0u);

	psink->stop();
}

//-----------------------------------------------------------------------------
// 2. TCP
//-----------------------------------------------------------------------------
namespace
{
	uint16_t it_port()
	{
		return static_cast<uint16_t>(36400 + api::get_process_id() % 300);
	}

	// - [uint32 size][uint32 sequence][bytes : (sequence + i) & 0xff]
	shared_buffer it_message(uint32_t _size, uint32_t _sequence)
	{
		auto buf = alloc_shared_buffer(_size);
		std::vector<char> message(_size);
		memcpy(message.data(), &_size, 4);
		memcpy(message.data() + 4, &_sequence, 4);
		for (uint32_t i = 8; i < _size; ++i)
			message[i] = static_cast<char>((_sequence + i) & 0xff);
		buf.append(message.size(), message.data());
		return buf;
	}

	std::mutex g_it_lockable;
	std::vector<object_ptr<net::io::Isocket>> g_it_servers;

	template <class TSOCKET>
	class it_server_socket : public TSOCKET
	{
	public:
		std::atomic<int>	count_message{ 0 };
		std::atomic<int>	count_error{ 0 };
		uint32_t			sequence_next = 0;
	protected:
		virtual void on_connect() override
		{
			std::lock_guard lock(g_it_lockable);
			g_it_servers.push_back(object_ptr<net::io::Isocket>(this));
		}
		virtual void on_disconnect(uint64_t /*_disconnect_reason*/) override {}
		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			// - check the size, the order and the bytes
			const auto size = _msg.buf_message.size();
			uint32_t size_header = 0, sequence = 0;
			memcpy(&size_header, _msg.buf_message.data(), 4);
			memcpy(&sequence, _msg.buf_message.data() + 4, 4);
			bool is_ok = (size == size_header) && (sequence == this->sequence_next);
			for (uint32_t i = 8; is_ok && i < size; i += 61)
				is_ok = _msg.buf_message.data()[i] == static_cast<char>((sequence + i) & 0xff);
			if (is_ok == false)
				++this->count_error;
			this->sequence_next = sequence + 1;
			++this->count_message;
			return eRESULT::DONE;
		}
	};

	template <class TSOCKET>
	class it_client_socket : public TSOCKET, public net::io::connect_requestable::tcp::Ndefault
	{
	public:
		std::atomic<bool>	is_connected{ false };
	protected:
		virtual void on_connect() override { this->is_connected = true; }
		virtual void on_disconnect(uint64_t) override { this->is_connected = false; }
		virtual result_code on_message(sMESSAGE_NETWORK&) override { return eRESULT::DONE; }
	};

	template <class TSOCKET>
	class it_acceptor : public net::acceptor<it_server_socket<TSOCKET>>
	{
	};

	// - a client connected to a server socket of _pacceptor. the server socket : _pserver
	template <class TSOCKET>
	bool it_connect(it_acceptor<TSOCKET>& _acceptor, own_ptr<it_client_socket<TSOCKET>>& _pclient, it_server_socket<TSOCKET>*& _pserver)
	{
		{
			std::lock_guard lock(g_it_lockable);
			g_it_servers.clear();
		}
		typename net::acceptor<it_server_socket<TSOCKET>>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), it_port() };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		if (_acceptor.start(parameter) != eRESULT::SUCCESS)
			return false;

		_pclient = make_own<it_client_socket<TSOCKET>>();
		sSOCKET_CONNECT_REQUEST_PARAMETER parameter_connect;
		parameter_connect.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), _acceptor.local_endpoint().port() };
		parameter_connect.enable_progress_log = false;
		_pclient->start(parameter_connect);

		if (iu_wait_until([&] { std::lock_guard lock(g_it_lockable); return _pclient->is_connected.load() && g_it_servers.size() == 1; }) == false)
			return false;

		std::lock_guard lock(g_it_lockable);
		_pserver = dynamic_cast<it_server_socket<TSOCKET>*>(g_it_servers.front().get());
		return _pserver != nullptr;
	}

	template <class TSOCKET>
	void it_run_async_sending(bool _is_one_send_per_message)
	{
		own_ptr<it_acceptor<TSOCKET>> pacceptor = make_own<it_acceptor<TSOCKET>>();
		own_ptr<it_client_socket<TSOCKET>> pclient;
		it_server_socket<TSOCKET>* pserver = nullptr;
		ASSERT_TRUE(it_connect<TSOCKET>(*pacceptor, pclient, pserver));

		constexpr int COUNT = 300;
		for (uint32_t i = 0; i < COUNT; ++i)
		{
			EXPECT_TRUE(pclient->send(it_message(64, i)));
			if ((i % 50) == 49)
				std::this_thread::sleep_for(1ms);
		}

		EXPECT_TRUE(iu_wait_until([&] { return pserver->count_message.load() == COUNT; })) << "received " << pserver->count_message.load();
		EXPECT_EQ(pserver->count_error.load(), 0);

		// - no send in flight after the completions
		EXPECT_TRUE(iu_wait_until([&] { return pclient->statistics_get_async_sending() == 0; })) << "async sending " << pclient->statistics_get_async_sending();
		EXPECT_EQ(pclient->statistics_get_sent_message(), static_cast<uint64_t>(COUNT));
		if (_is_one_send_per_message)
			EXPECT_EQ(pclient->statistics_get_async_send(), static_cast<uint64_t>(COUNT));

		// - the sockets closed : no I/O left
		pclient->closesocket();
		EXPECT_TRUE(iu_wait_until([&] { return pclient->statistics_get_now_async_io() == 0; })) << "async io " << pclient->statistics_get_now_async_io();

		pclient->stop();
		pacceptor->stop();
		std::lock_guard lock(g_it_lockable);
		g_it_servers.clear();
	}

	template <class TSOCKET>
	void it_run_receive_sizes()
	{
		own_ptr<it_acceptor<TSOCKET>> pacceptor = make_own<it_acceptor<TSOCKET>>();
		own_ptr<it_client_socket<TSOCKET>> pclient;
		it_server_socket<TSOCKET>* pserver = nullptr;
		ASSERT_TRUE(it_connect<TSOCKET>(*pacceptor, pclient, pserver));

		// - sizes : runs of small ones (the shrink), bursts of large ones (the growth), a large one between small ones (a partial message moved)
		const uint32_t sizes[] = { 8, 16, 100, 1000, 1500, 4000, 9000, 30000, 65536, 65537, 70000, 120000, 200000, 12, 64 };
		std::vector<uint32_t> vector_size;
		uint32_t seed = 12345;
		for (int i = 0; i < 3000; ++i)
		{
			seed = seed * 1103515245 + 12345;
			const auto index = (seed >> 16) % std::size(sizes);
			// - mostly small (a run of small receives shrinks the buffer)
			vector_size.push_back(((seed >> 8) % 4 == 0 || (i % 500) < 400) ? sizes[index % 6] : sizes[index]);
		}

		uint32_t sequence = 0;
		for (auto size : vector_size)
		{
			// - the send queue under its limit (an overflow closes the socket)
			iu_wait_until([&] { return pclient->sending_queued_bytes_now() < 256 * 1024; });
			EXPECT_TRUE(pclient->send(it_message(size, sequence++)));
		}

		EXPECT_TRUE(iu_wait_until([&] { return pserver->count_message.load() == static_cast<int>(vector_size.size()); }, 10000ms)) << "received " << pserver->count_message.load();
		EXPECT_EQ(pserver->count_error.load(), 0);

		pclient->stop();
		pacceptor->stop();
		std::lock_guard lock(g_it_lockable);
		g_it_servers.clear();
	}
}

TEST(iocp_tcp, async_sending_gather)		{ it_run_async_sending<net::socket::tcp<>>(false); }
TEST(iocp_tcp, async_sending_buffered)		{ it_run_async_sending<net::socket::tcp_buffered<>>(false); }
TEST(iocp_tcp, async_sending_no_gather)		{ it_run_async_sending<net::socket::tcp_no_gather<>>(true); }
TEST(iocp_tcp, receive_sizes)				{ it_run_receive_sizes<net::socket::tcp<>>(); }

}

#endif
