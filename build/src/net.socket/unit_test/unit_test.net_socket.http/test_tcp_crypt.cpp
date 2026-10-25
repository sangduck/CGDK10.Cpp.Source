#include "pch.h"
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// tcp_crypt (net::io::Ntcp_crypt : pn_code stream cipher after the REQUEST / ACCEPT_CERTIFICATION handshake)
//
//  1. echo_concurrent_senders : threads send through one client socket at once. the cryptor is a stream cipher : the order of the
//    encryption must be the order of the bytes on the wire (it was encrypted out of the send lock : the server decrypted garbage).
//    every echo arrives in order per sender, the overlapped send count goes back to 0 (the completion never decreased it), and
//    on_receive() is given the decrypted bytes received (it was given the bytes past them)
//  2. encrypted_data_in_the_receive_of_accept : a raw server sends ACCEPT_CERTIFICATION and encrypted messages in one send. the client
//    installs its cryptor in the middle of the received bytes : the bytes after ACCEPT are decrypted then (they were taken as plain)
//  3. reconnect_after_close_while_sending : a raw server that does not read : the client queues sends till they wait, then the
//    server resets the connection. the client connects again : its REQUEST_CERTIFICATION is sent (m_flag_sending stayed true after
//    the reset : every send was queued and nothing sent again), and the overlapped send count is back to 0 after the close
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H) || defined(_WINSOCK2API_)

#if defined(_SYS_EPOLL_H)
	#include <arpa/inet.h>
	#include <netinet/tcp.h>
	#include <sys/select.h>
#endif

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
#if defined(_WINSOCK2API_)
	using tcr_fd_t = SOCKET;
	using tcr_socklen_t = int;
	constexpr tcr_fd_t TCR_FD_INVALID = INVALID_SOCKET;
	inline void tcr_close_fd(tcr_fd_t _fd) { ::closesocket(_fd); }
#else
	using tcr_fd_t = int;
	using tcr_socklen_t = socklen_t;
	constexpr tcr_fd_t TCR_FD_INVALID = -1;
	inline void tcr_close_fd(tcr_fd_t _fd) { ::close(_fd); }
#endif

	template <class TCONDITION>
	bool tcr_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 10000ms)
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

	// - the port of the tests (34400 + pid % 500 : not the ports of the other tests. the acceptor moves to the next free one)
	uint16_t tcr_port()
	{
		return static_cast<uint16_t>(34400 + api::get_process_id() % 500);
	}

	// - [uint32 size][uint32 sender][uint32 sequence][pattern...]
	std::string tcr_make_message(uint32_t _sender, uint32_t _sequence, std::size_t _size)
	{
		std::string message(_size, '\0');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		memcpy(message.data() + 4, &_sender, 4);
		memcpy(message.data() + 8, &_sequence, 4);
		for (std::size_t i = 12; i < _size; ++i)
			message[i] = static_cast<char>('a' + (i + _sender * 7 + _sequence) % 26);
		return message;
	}

	shared_buffer tcr_buffer(const std::string& _message)
	{
		auto buf = alloc_shared_buffer(_message.size());
		buf.append(_message.size(), _message.data());
		return buf;
	}

	//-----------------------------------------------------------------------------
	// raw socket (the server side of 2. and 3.)
	//-----------------------------------------------------------------------------
	bool tcr_wait_readable(tcr_fd_t _fd, std::chrono::milliseconds _timeout)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(_fd, &set);
		timeval timeout{ static_cast<long>(_timeout.count() / 1000), static_cast<long>((_timeout.count() % 1000) * 1000) };
		return ::select(static_cast<int>(_fd + 1), &set, nullptr, nullptr, &timeout) > 0;
	}

	// - listen on the loopback (port chosen by the OS) : _port
	tcr_fd_t tcr_listen(uint16_t& _port)
	{
	#if defined(_WINSOCK2API_)
		// - winsock (a test run alone has no CGDK socket made yet)
		static const bool is_started = [] { WSADATA data; return ::WSAStartup(MAKEWORD(2, 2), &data) == 0; }();
		(void)is_started;
	#endif

		tcr_fd_t fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (fd == TCR_FD_INVALID)
			return TCR_FD_INVALID;

		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = 0;

		tcr_socklen_t length = sizeof(address);
		if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 || ::listen(fd, 8) != 0 || ::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &length) != 0)
		{
			tcr_close_fd(fd);
			return TCR_FD_INVALID;
		}

		_port = ntohs(address.sin_port);
		return fd;
	}

	tcr_fd_t tcr_accept(tcr_fd_t _fd_listen, std::chrono::milliseconds _timeout = 10000ms)
	{
		if (tcr_wait_readable(_fd_listen, _timeout) == false)
			return TCR_FD_INVALID;
		return ::accept(_fd_listen, nullptr, nullptr);
	}

	// - receives exactly _size bytes (false : timeout / closed)
	bool tcr_recv_exact(tcr_fd_t _fd, char* _buffer, std::size_t _size, std::chrono::milliseconds _timeout = 10000ms)
	{
		const auto time_end = std::chrono::steady_clock::now() + _timeout;
		std::size_t received = 0;
		while (received < _size)
		{
			const auto now = std::chrono::steady_clock::now();
			if (now >= time_end || tcr_wait_readable(_fd, std::chrono::duration_cast<std::chrono::milliseconds>(time_end - now)) == false)
				return false;
			const auto result = ::recv(_fd, _buffer + received, static_cast<int>(_size - received), 0);
			if (result <= 0)
				return false;
			received += static_cast<std::size_t>(result);
		}
		return true;
	}

	bool tcr_send_all(tcr_fd_t _fd, const char* _data, std::size_t _size)
	{
		std::size_t sent = 0;
		while (sent < _size)
		{
			const auto result = ::send(_fd, _data + sent, static_cast<int>(_size - sent), 0);
			if (result <= 0)
				return false;
			sent += static_cast<std::size_t>(result);
		}
		return true;
	}

	// - the reset (SO_LINGER 0 : RST) of an accepted raw socket
	void tcr_close_abortive(tcr_fd_t _fd)
	{
		linger value{ 1, 0 };
		::setsockopt(_fd, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&value), sizeof(value));
		tcr_close_fd(_fd);
	}

	// - REQUEST_CERTIFICATION of the client : [uint32 size][uint32 message][crc]
	constexpr std::size_t TCR_SIZE_REQUEST = 8 + SIZE_OF_CRC;

	// - ACCEPT_CERTIFICATION : like net::io::Ntcp_crypt::message_NETWORK_SECURITY_ACCEPT_CERTIFICATION()
	shared_buffer tcr_make_accept(eCRYPTION_TYPE _type, const crypt::pn_code::key_t* _pkeys_client_encrypt, const crypt::pn_code::key_t* _pkeys_client_decrypt)
	{
		auto buf_message = alloc_shared_buffer(32768);
		buf_message.append<uint32_t>();
		buf_message.append<uint32_t>(eMESSAGE::NETWORK::SECURITY::ACCEPT_CERTIFICATION);
		buf_message.append<eCRYPTION_TYPE>(_type);
		if (_type == eCRYPTION_TYPE::PNCODE)
		{
			buf_message.append<crypt::pn_code::key_t>(*_pkeys_client_encrypt);
			buf_message.append<crypt::pn_code::key_t>(*_pkeys_client_decrypt);
		}
		buf_message.front<uint32_t>() = static_cast<uint32_t>(buf_message.size() + SIZE_OF_CRC);
		buf_message.append<CRC_T>(edc::default_crc.get_crc(buf_message));
		return buf_message;
	}

	//-----------------------------------------------------------------------------
	// sockets
	//-----------------------------------------------------------------------------
	class tcr_echo_server_socket : public net::socket::tcp_crypt<>
	{
	protected:
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			this->send(_msg.buf_message);
			return eRESULT::DONE;
		}
	};

	class tcr_client_socket : public net::socket::tcp_crypt<>, public net::io::connect_requestable::tcp::Ndefault
	{
	public:
				std::atomic<bool>	is_connected{ false };
				std::atomic<int>	count_connect{ 0 };
				std::atomic<int>	count_disconnect{ 0 };
				std::mutex			lockable;
				std::vector<std::string> received;		// - the messages
				std::string			stream_messages;	// - the messages appended
				std::string			stream_on_receive;	// - the bytes given to on_receive() appended

	protected:
		virtual void				on_connect() override { ++this->count_connect; this->is_connected = true; }
		virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override {}
		virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; ++this->count_disconnect; }
		virtual void				on_receive(const buffer_view& _buffer, const net::sockaddr& /*_address*/) override
		{
			std::lock_guard lock(this->lockable);
			this->stream_on_receive.append(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
		}
		virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
		{
			std::lock_guard lock(this->lockable);
			this->received.emplace_back(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
			this->stream_messages.append(reinterpret_cast<const char*>(_msg.buf_message.data()), _msg.buf_message.size());
			return eRESULT::DONE;
		}
	};
}

TEST(tcp_crypt, echo_concurrent_senders)
{
	// 1) server (pn_code)
	auto pacceptor = make_own<net::acceptor<tcr_echo_server_socket>>();
	sSOCKET_ACCEPT_PARAMETER parameter{ net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), tcr_port() } };
	parameter.cryption_type = eCRYPTION_TYPE::PNCODE;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) client (certified : on_connect)
	auto psocket = make_own<tcr_client_socket>();
	psocket->start({ { net::ip::address_v4::loopback(), port } });
	ASSERT_TRUE(tcr_wait_until([&] { return psocket->is_connected.load(); })) << "crypt connection (port " << port << ")";
	ASSERT_TRUE(psocket->get_cryptor().exist());

	// 3) senders at once
	constexpr uint32_t SENDERS = 8;
	constexpr uint32_t MESSAGES = 400;
	std::atomic<int> count_fail{ 0 };
	{
		std::vector<std::thread> threads;
		for (uint32_t sender = 0; sender < SENDERS; ++sender)
		{
			threads.emplace_back([&, sender]
			{
				for (uint32_t sequence = 0; sequence < MESSAGES; ++sequence)
				{
					if (psocket->send(tcr_buffer(tcr_make_message(sender, sequence, 12 + (sequence * 37 + sender * 11) % 700))) == false)
						++count_fail;
				}
			});
		}
		for (auto& iter : threads)
			iter.join();
	}
	EXPECT_EQ(count_fail.load(), 0);

	// 4) every echo arrives (in order per sender, the contents)
	const bool is_received = tcr_wait_until([&] { std::lock_guard lock(psocket->lockable); return psocket->received.size() >= SENDERS * MESSAGES; });
	EXPECT_TRUE(is_received) << "received " << psocket->received.size() << " / " << (SENDERS * MESSAGES) << " (connected : " << psocket->is_connected.load() << ")";
	{
		std::lock_guard lock(psocket->lockable);

		std::vector<uint32_t> next(SENDERS, 0);
		std::size_t count_wrong = 0;
		for (const auto& iter : psocket->received)
		{
			if (iter.size() < 12) { ++count_wrong; continue; }
			uint32_t sender, sequence;
			memcpy(&sender, iter.data() + 4, 4);
			memcpy(&sequence, iter.data() + 8, 4);
			if (sender >= SENDERS || sequence != next[sender] || iter != tcr_make_message(sender, sequence, iter.size())) { ++count_wrong; continue; }
			++next[sender];
		}
		EXPECT_EQ(count_wrong, 0u);

		// - on_receive() : the decrypted bytes (ACCEPT_CERTIFICATION, then the echoes)
		ASSERT_GE(psocket->stream_on_receive.size(), psocket->stream_messages.size());
		EXPECT_TRUE(psocket->stream_on_receive.compare(psocket->stream_on_receive.size() - psocket->stream_messages.size(), psocket->stream_messages.size(), psocket->stream_messages) == 0) << "on_receive() bytes are not the received bytes";
	}

	// 5) the overlapped sends all completed (statistics_get_now_async_io() reaches 0 : the acceptor stop does not wait 30s)
	EXPECT_TRUE(tcr_wait_until([&] { return psocket->statistics_get_async_sending() == 0; }, 3000ms)) << "async sending " << psocket->statistics_get_async_sending();

	psocket->closesocket();
	pacceptor->stop();
}

TEST(tcp_crypt, encrypted_data_in_the_receive_of_accept)
{
	// 1) raw server
	uint16_t port = 0;
	const auto fd_listen = tcr_listen(port);
	ASSERT_NE(fd_listen, TCR_FD_INVALID);

	// 2) client
	auto psocket = make_own<tcr_client_socket>();
	psocket->start({ { net::ip::address_v4::loopback(), port } });

	const auto fd = tcr_accept(fd_listen);
	ASSERT_NE(fd, TCR_FD_INVALID);

	// 3) REQUEST_CERTIFICATION
	char request[TCR_SIZE_REQUEST];
	ASSERT_TRUE(tcr_recv_exact(fd, request, sizeof(request)));

	// 4) ACCEPT_CERTIFICATION (pn_code) and encrypted messages in one send
	auto keys_server_encrypt = crypt::pn_code::generate_key();
	auto keys_server_decrypt = crypt::pn_code::generate_key();
	auto pcryptor_server = crypt::pn_code::make_cryptor(keys_server_encrypt, keys_server_decrypt);

	std::vector<std::string> messages;
	std::string data;
	{
		auto buf_accept = tcr_make_accept(eCRYPTION_TYPE::PNCODE, &keys_server_decrypt, &keys_server_encrypt);
		data.append(reinterpret_cast<const char*>(buf_accept.data()), buf_accept.size());

		std::string plain;
		for (uint32_t i = 0; i < 5; ++i)
		{
			messages.push_back(tcr_make_message(0, i, 20 + i * 100));
			plain += messages.back();
		}
		auto buf_encrypted = pcryptor_server->encrypt(buffer_view(plain.data(), plain.size()));
		data.append(reinterpret_cast<const char*>(buf_encrypted.data()), buf_encrypted.size());
	}
	ASSERT_TRUE(tcr_send_all(fd, data.data(), data.size()));

	// 5) the client is certified and receives the messages
	ASSERT_TRUE(tcr_wait_until([&] { return psocket->is_connected.load(); })) << "certified";
	const bool is_received = tcr_wait_until([&] { std::lock_guard lock(psocket->lockable); return psocket->received.size() >= messages.size(); }, 3000ms);
	EXPECT_TRUE(is_received) << "received " << psocket->received.size() << " (connected : " << psocket->is_connected.load() << ")";
	{
		std::lock_guard lock(psocket->lockable);
		EXPECT_EQ(psocket->received, messages);
	}

	// 6) the client encrypts with the keys given (the raw server decrypts)
	const auto message_client = tcr_make_message(1, 7, 300);
	ASSERT_TRUE(psocket->send(tcr_buffer(message_client)));
	std::string received(message_client.size(), '\0');
	ASSERT_TRUE(tcr_recv_exact(fd, received.data(), received.size()));
	auto buf_decrypted = pcryptor_server->decrypt(buffer_view(received.data(), received.size()));
	EXPECT_EQ(std::string(reinterpret_cast<const char*>(buf_decrypted.data()), buf_decrypted.size()), message_client);

	psocket->closesocket();
	tcr_close_fd(fd);
	tcr_close_fd(fd_listen);
}

TEST(tcp_crypt, reconnect_after_close_while_sending)
{
	// 1) raw server (does not read after the handshake)
	uint16_t port = 0;
	const auto fd_listen = tcr_listen(port);
	ASSERT_NE(fd_listen, TCR_FD_INVALID);

	auto psocket = make_own<tcr_client_socket>();

	for (int round = 0; round < 2; ++round)
	{
		// 2) connect (the second : the same socket again)
		if (round != 0)
		{
			psocket->stop();

			bool is_started = false;
			for (int i = 0; i < 200 && is_started == false; ++i)
			{
				is_started = (psocket->start({ { net::ip::address_v4::loopback(), port } }) == eRESULT::SUCCESS);
				if (is_started == false)
					std::this_thread::sleep_for(10ms);
			}
			ASSERT_TRUE(is_started) << "start again";
		}
		else
		{
			psocket->start({ { net::ip::address_v4::loopback(), port } });
		}

		const auto fd = tcr_accept(fd_listen);
		ASSERT_NE(fd, TCR_FD_INVALID) << "round " << round;

		// 3) REQUEST_CERTIFICATION (round 1 : it was queued and never sent)
		char request[TCR_SIZE_REQUEST];
		ASSERT_TRUE(tcr_recv_exact(fd, request, sizeof(request), 5000ms)) << "REQUEST_CERTIFICATION of round " << round;

		// 4) ACCEPT_CERTIFICATION (no cryption)
		auto buf_accept = tcr_make_accept(eCRYPTION_TYPE::NONE, nullptr, nullptr);
		ASSERT_TRUE(tcr_send_all(fd, reinterpret_cast<const char*>(buf_accept.data()), buf_accept.size()));
		ASSERT_TRUE(tcr_wait_until([&] { return psocket->is_connected.load(); })) << "certified, round " << round;

		if (round == 1)
		{
			tcr_close_abortive(fd);
			break;
		}

		const int count_disconnect = psocket->count_disconnect.load();

		// 5) send till the sends wait (the server does not read) : a send in progress at the close
		int count_sent = 0;
		const auto message = tcr_make_message(0, 0, 64 * 1024);
		for (int i = 0; i < 8192; ++i)
		{
			if (psocket->send(tcr_buffer(message)) == false)
				break;
			++count_sent;
		}
		std::this_thread::sleep_for(100ms);

		// 6) reset by the server
		tcr_close_abortive(fd);
		ASSERT_TRUE(tcr_wait_until([&] { return psocket->count_disconnect.load() > count_disconnect; })) << "disconnected (sent " << count_sent << ", connected " << psocket->is_connected.load() << ", disconnects " << psocket->count_disconnect.load() << ")";

		// 7) the overlapped sends completed (the count goes back to 0)
		EXPECT_TRUE(tcr_wait_until([&] { return psocket->statistics_get_async_sending() == 0; }, 3000ms)) << "async sending " << psocket->statistics_get_async_sending();
	}

	psocket->closesocket();
	tcr_close_fd(fd_listen);
}

}

#endif
