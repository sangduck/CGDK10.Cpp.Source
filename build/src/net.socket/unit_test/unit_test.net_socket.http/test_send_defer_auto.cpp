#include "pch.h"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// send_defer auto : the sends of an executable run by the executor (linux, windows)
//
//  - the executor opens a send_defer::auto_scope around each executable it runs : out of a group lock and a send cork, the
//    consecutive sends of the executable to one socket are deferred, and sent when it sends to another socket or returns.
//  - checked here : every message arrives (but after the abortive close of 4), in order per sender, without any later send (nothing
//    stays deferred), with the auto on and off, for tcp<> (Ntcp_async_gather) and tcp_buffered<> (Ntcp_async_gather_buffered).
//
//  1. interleave : one executable sends to the sockets in a mixed order (a switch at almost every send)
//  2. burst      : executables send K messages to each socket (a tick)
//  3. single     : many executables send one message each
//  4. close      : an executable sends to a socket then closesocket() (abortive) : the messages are given to the socket before the
//                  close (the deferred ones too), what arrives is in order, then the EOF
//  5. disconnect : the same with disconnect() (graceful) : the messages arrive before the EOF
//  6. exception  : an executable sends then throws : the messages arrive
//  7. cross      : executables and a thread out of the executor send to the same sockets at once : the order per sender is kept
//
//  - send_defer_auto.close_while_sending : threads send to a socket while another thread closes it (windows : the sending thread
//    calls WSASend out of the queue lock, the close swaps the handle under the lock of the send call). what arrives is in order
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
	using sda_fd_t = SOCKET;
	using sda_socklen_t = int;
	constexpr sda_fd_t SDA_FD_INVALID = INVALID_SOCKET;
	inline void sda_close_fd(sda_fd_t _fd) { ::closesocket(_fd); }
#else
	using sda_fd_t = int;
	using sda_socklen_t = socklen_t;
	constexpr sda_fd_t SDA_FD_INVALID = -1;
	inline void sda_close_fd(sda_fd_t _fd) { ::close(_fd); }
#endif

	// - waits for data (or the EOF) on _fd at most _timeout
	bool sda_wait_readable(sda_fd_t _fd, std::chrono::milliseconds _timeout)
	{
		fd_set set;
		FD_ZERO(&set);
		FD_SET(_fd, &set);
		timeval timeout{ static_cast<long>(_timeout.count() / 1000), static_cast<long>((_timeout.count() % 1000) * 1000) };
		return ::select(static_cast<int>(_fd + 1), &set, nullptr, nullptr, &timeout) > 0;
	}

	constexpr int			SDA_SOCKETS = 8;
	constexpr std::size_t	SDA_MESSAGE_SIZE = 32;

	template <class TCONDITION>
	bool sda_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
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

	// - the port of the tests (33900 + pid % 500 : not the ports of the other tests)
	uint16_t sda_port()
	{
		return static_cast<uint16_t>(33900 + api::get_process_id() % 500);
	}

	// - [uint32 size][uint32 sender][uint64 sequence][fill]
	shared_buffer sda_message(uint32_t _sender, uint64_t _sequence)
	{
		char message[SDA_MESSAGE_SIZE];
		const uint32_t size = static_cast<uint32_t>(SDA_MESSAGE_SIZE);
		memcpy(message, &size, 4);
		memcpy(message + 4, &_sender, 4);
		memcpy(message + 8, &_sequence, 8);
		memset(message + 16, 'x', SDA_MESSAGE_SIZE - 16);

		auto buf = alloc_shared_buffer(SDA_MESSAGE_SIZE);
		buf.append(SDA_MESSAGE_SIZE, message);
		return buf;
	}

	//-----------------------------------------------------------------------------
	// the server sockets (the test reaches them in the order of accept)
	//-----------------------------------------------------------------------------
	std::mutex				g_sda_lockable;
	std::vector<object_ptr<net::io::Isocket>> g_sda_servers;

	template <class TBASE>
	class sda_server_socket : public TBASE
	{
	protected:
		virtual void on_connect() override
		{
			std::lock_guard lock(g_sda_lockable);
			g_sda_servers.push_back(object_ptr<net::io::Isocket>(this));
		}
		virtual void on_disconnect(uint64_t /*_disconnect_reason*/) override {}
		virtual result_code on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::DONE; }
	};

	template <class TSOCKET>
	class sda_acceptor : public net::acceptor<TSOCKET>
	{
	protected:
		virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable* /*_pconnectable*/) override
		{
			object_ptr<executor::Iio_context> pio_context = this->io_context();
			return pio_context;
		}
	};

	bool sda_send(const object_ptr<net::io::Isocket>& _psocket, uint32_t _sender, uint64_t _sequence)
	{
		auto* psender = dynamic_cast<net::io::Isender*>(_psocket.get());
		return psender != nullptr && psender->send(sda_message(_sender, _sequence));
	}

	bool sda_disconnect(const object_ptr<net::io::Isocket>& _psocket)
	{
		auto* pconnectable = dynamic_cast<net::io::Iconnectable*>(_psocket.get());
		return pconnectable != nullptr && pconnectable->disconnect();
	}

	// - the port of the peer (the local port of its client)
	uint16_t sda_peer_port(const object_ptr<net::io::Isocket>& _psocket)
	{
		sockaddr_in address{};
		sda_socklen_t length = sizeof(address);
		if (::getpeername(static_cast<sda_fd_t>(_psocket->native_handle()), reinterpret_cast<sockaddr*>(&address), &length) != 0)
			return 0;
		return ntohs(address.sin_port);
	}

	// - the messages given to the socket (counted when sent)
	int64_t sda_sent_message(const object_ptr<net::io::Isocket>& _psocket)
	{
		auto* pstatistics = dynamic_cast<net::io::statistics::Nsocket*>(_psocket.get());
		return (pstatistics != nullptr) ? static_cast<int64_t>(pstatistics->statistics_get_sent_message()) : -1;
	}

	// - how a case closes the socket 0 (its client is the one that gets the EOF)
	//   abortive : closesocket() (SO_LINGER 0 : RST) drops what the kernel has not sent yet => a part of the messages arrives
	//   graceful : disconnect() (the FIN after the data) => every message arrives before the EOF
	enum class sda_close { none, abortive, graceful };

	//-----------------------------------------------------------------------------
	// a blocking client of the OS : reads until the count, the EOF or 300 ms without data
	//-----------------------------------------------------------------------------
	class sda_client
	{
	public:
		~sda_client() { this->close(); }

		bool connect(uint16_t _port)
		{
		#if defined(_WINSOCK2API_)
			this->m_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		#else
			this->m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
		#endif
			if (this->m_fd == SDA_FD_INVALID)
				return false;

			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(_port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

			return ::connect(this->m_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
		}

		uint16_t local_port() const
		{
			sockaddr_in address{};
			sda_socklen_t length = sizeof(address);
			if (::getsockname(this->m_fd, reinterpret_cast<sockaddr*>(&address), &length) != 0)
				return 0;
			return ntohs(address.sin_port);
		}

		// - the EOF is next (without a wait)
		bool is_eof_next() const
		{
			char byte;
			return this->m_eof || (sda_wait_readable(this->m_fd, 0ms) && ::recv(this->m_fd, &byte, 1, MSG_PEEK) == 0);
		}

		// - a message left deferred never arrives : the idle time ends the read and the count is short
		void read(std::size_t _count, std::chrono::milliseconds _limit = 3000ms)
		{
			const auto time_end = std::chrono::steady_clock::now() + _limit;
			auto time_last = std::chrono::steady_clock::now();
			char buffer[65536];

			while (this->m_data.size() < _count * SDA_MESSAGE_SIZE && this->m_eof == false
				&& std::chrono::steady_clock::now() < time_end && std::chrono::steady_clock::now() - time_last < 300ms)
			{
				// - 100 ms at most a wait (the conditions above are checked again)
				if (sda_wait_readable(this->m_fd, 100ms) == false)
					continue;

				const auto result = ::recv(this->m_fd, buffer, static_cast<int>(sizeof(buffer)), 0);
				if (result > 0)
				{
					this->m_data.insert(this->m_data.end(), buffer, buffer + result);
					time_last = std::chrono::steady_clock::now();
				}
				else
				{
					// - the EOF, or the reset of the abortive close (windows : WSAECONNRESET)
					this->m_eof = true;
				}
			}
		}

		std::size_t count() const { return this->m_data.size() / SDA_MESSAGE_SIZE; }

		// - the sequences of each sender are 0, 1, 2 ... in order
		bool check_order(std::string& _error) const
		{
			if ((this->m_data.size() % SDA_MESSAGE_SIZE) != 0)
			{
				_error = "a partial message";
				return false;
			}

			std::map<uint32_t, uint64_t> sequence_next;
			for (std::size_t i = 0; i < this->count(); ++i)
			{
				uint32_t sender = 0;
				uint64_t sequence = 0;
				memcpy(&sender, this->m_data.data() + i * SDA_MESSAGE_SIZE + 4, 4);
				memcpy(&sequence, this->m_data.data() + i * SDA_MESSAGE_SIZE + 8, 8);

				auto& expected = sequence_next[sender];
				if (sequence != expected)
				{
					_error = "sender " + std::to_string(sender) + " sequence " + std::to_string(sequence) + " expected " + std::to_string(expected);
					return false;
				}
				++expected;
			}
			return true;
		}

		void close()
		{
			if (this->m_fd != SDA_FD_INVALID)
				sda_close_fd(this->m_fd);
			this->m_fd = SDA_FD_INVALID;
		}

	public:
		sda_fd_t			m_fd = SDA_FD_INVALID;
		std::vector<char>	m_data;
		bool				m_eof = false;
	};

	// - stops the acceptor and the executor on any exit of the test (an ASSERT too)
	template <class TACCEPTOR>
	struct sda_guard
	{
		own_ptr<TACCEPTOR>&							pacceptor;
		object_ptr<executor::Iio_context_threads>&	pexecutor;
		bool										auto_before = send_defer::is_auto_enabled();

		~sda_guard()
		{
			{
				std::vector<object_ptr<net::io::Isocket>> servers;
				{
					std::lock_guard lock(g_sda_lockable);
					servers.swap(g_sda_servers);
				}
				for (auto& iter : servers)
					iter->closesocket();
			}
			if (pacceptor.exist())
			{
				sda_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms);
				pacceptor->stop();
			}
			if (pexecutor.exist())
				pexecutor->stop();
			send_defer::enable_auto(auto_before);
		}
	};
}

//-----------------------------------------------------------------------------
// the cases : _post sends from the executor (and a thread) to the server sockets, _expected = the messages per server socket
//   (the order of accept is not the order of connect : the client of each server socket is found by the port)
//-----------------------------------------------------------------------------
template <class TBASE>
void sda_check(bool _auto)
{
	using server_socket = sda_server_socket<TBASE>;
	using acceptor_type = sda_acceptor<server_socket>;

	auto pexecutor = executor::make_io_context_threads("send defer auto test", 4);
	own_ptr<acceptor_type> pacceptor;
	sda_guard<acceptor_type> guard{ pacceptor, pexecutor };

	ASSERT_TRUE(pexecutor.exist());
	send_defer::enable_auto(_auto);

	{
		std::lock_guard lock(g_sda_lockable);
		g_sda_servers.clear();
	}

	pacceptor = make_own<acceptor_type>();
	pacceptor->io_context(pexecutor.get());

	typename acceptor_type::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), sda_port() };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	auto run_case = [&](const char* _name, const std::function<void(const std::vector<object_ptr<net::io::Isocket>>&)>& _post, const std::vector<std::size_t>& _expected, sda_close _close, const std::function<void(const std::vector<object_ptr<net::io::Isocket>>&)>& _after_read = nullptr)
	{
		std::vector<sda_client> clients(SDA_SOCKETS);
		for (auto& iter : clients)
			ASSERT_TRUE(iter.connect(port)) << _name;

		ASSERT_TRUE(sda_wait_until([&]() { std::lock_guard lock(g_sda_lockable); return g_sda_servers.size() == static_cast<std::size_t>(SDA_SOCKETS); })) << _name << " accept";

		std::vector<object_ptr<net::io::Isocket>> servers;
		{
			std::lock_guard lock(g_sda_lockable);
			servers.swap(g_sda_servers);
		}

		// - the server socket of each client
		std::vector<std::size_t> index_server(clients.size(), servers.size());
		for (std::size_t i = 0; i < clients.size(); ++i)
		{
			const auto port_client = clients[i].local_port();
			for (std::size_t j = 0; j < servers.size(); ++j)
				if (sda_peer_port(servers[j]) == port_client)
					index_server[i] = j;

			ASSERT_LT(index_server[i], servers.size()) << _name << " the server socket of the client " << i;
		}

		_post(servers);

		for (std::size_t i = 0; i < clients.size(); ++i)
		{
			auto& iter = clients[i];
			const auto index = index_server[i];
			iter.read(_expected[index]);

			std::string error;
			EXPECT_TRUE(iter.check_order(error)) << _name << " : " << error;

			// - abortive : the closed one gets a part of its messages (an in-order prefix : checked above)
			if (index == 0 && _close == sda_close::abortive)
			{
				EXPECT_LE(iter.count(), _expected[index]) << _name << " (auto " << _auto << ")";
			}
			else
			{
				EXPECT_EQ(iter.count(), _expected[index]) << _name << " socket " << index << " (auto " << _auto << ")";
			}
		}

		// - the socket 0 closed by the executable : the EOF after its messages (the others : no EOF)
		if (_close != sda_close::none)
		{
			for (std::size_t i = 0; i < clients.size(); ++i)
			{
				if (index_server[i] == 0)
				{
					clients[i].read(clients[i].count() + 1, 500ms);
					EXPECT_TRUE(clients[i].m_eof) << _name << " EOF";
				}
			}
			for (std::size_t i = 0; i < clients.size(); ++i)
			{
				if (index_server[i] != 0)
				{
					EXPECT_FALSE(clients[i].is_eof_next()) << _name << " EOF of the socket " << index_server[i];
				}
			}
		}

		// - the checks on the server sockets after the reads (held by 'servers' till here)
		if (_after_read)
			_after_read(servers);

		for (auto& iter : clients)
			iter.close();
		for (auto& iter : servers)
			iter->closesocket();

		EXPECT_TRUE(sda_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << _name << " connected yet " << pacceptor->count();
	};

	// 1) interleave : the socket i of the step k of a round is (3k + round) % 8 (each once a round), a second send at an even k
	run_case("interleave", [&](const auto& _servers)
	{
		auto servers = _servers;
		pexecutor->post(make_executable([servers]()
		{
			std::vector<uint64_t> sequence(SDA_SOCKETS, 0);
			for (int round = 0; round < 100; ++round)
			{
				for (int k = 0; k < SDA_SOCKETS; ++k)
				{
					const int i = (k * 3 + round) % SDA_SOCKETS;
					sda_send(servers[i], 1, sequence[i]++);
					if ((k % 2) == 0)
						sda_send(servers[i], 1, sequence[i]++);
				}
			}
		}));
	}, std::vector<std::size_t>(SDA_SOCKETS, 150), sda_close::none);

	// 2) burst : 20 executables, 64 messages to each socket
	run_case("burst", [&](const auto& _servers)
	{
		for (uint32_t t = 0; t < 20; ++t)
		{
			auto servers = _servers;
			pexecutor->post(make_executable([servers, t]()
			{
				for (int i = 0; i < SDA_SOCKETS; ++i)
					for (uint64_t k = 0; k < 64; ++k)
						sda_send(servers[i], 100 + t, k);
			}));
		}
	}, std::vector<std::size_t>(SDA_SOCKETS, 20 * 64), sda_close::none);

	// 3) single : 504 executables, one message each
	run_case("single", [&](const auto& _servers)
	{
		for (uint32_t t = 0; t < 504; ++t)
		{
			auto pserver = _servers[t % SDA_SOCKETS];
			pexecutor->post(make_executable([pserver, t]() { sda_send(pserver, 1000 + t, 0); }));
		}
	}, std::vector<std::size_t>(SDA_SOCKETS, 504 / SDA_SOCKETS), sda_close::none);

	// 4) close : 10 messages to the others, 50 to the socket 0 then closesocket()
	//    - closesocket() is the abortive close (SO_LINGER 0 : RST) : the kernel drops what it has not sent yet. the clients are read one
	//      by one, so the client of the socket 0 may not read meanwhile : its ACKs are delayed and the cwnd holds the rest (epoll, auto
	//      off : 26 of 50 arrived = 16 quick ACKs + cwnd 10, 24 unsent in the kernel at the close, 2026-10-24) => a part arrives.
	//    - checked : the 50 are given to the socket before the close (the deferred ones are sent by the close) : the sent statistics
	//      right after closesocket() in the executable (linux). windows counts a message sent at the completion of its WSASend (it comes
	//      after the close) : the statistics of the socket once the completions came (after the reads)
	{
		std::vector<std::size_t> expected(SDA_SOCKETS, 10);
		expected[0] = 50;

		auto pcount_sent = std::make_shared<std::atomic<int64_t>>(-1);

		run_case("close", [&](const auto& _servers)
		{
			auto servers = _servers;
			pexecutor->post(make_executable([servers, pcount_sent]()
			{
				for (int i = 1; i < SDA_SOCKETS; ++i)
					for (uint64_t k = 0; k < 10; ++k)
						sda_send(servers[i], 7, k);
				for (uint64_t k = 0; k < 50; ++k)
					sda_send(servers[0], 7, k);
				servers[0]->closesocket();
			#if defined(_SYS_EPOLL_H)
				pcount_sent->store(sda_sent_message(servers[0]));
			#endif
			}));
		}, expected, sda_close::abortive, [&](const auto& _servers)
		{
		#if defined(_WINSOCK2API_)
			sda_wait_until([&]() { return sda_sent_message(_servers[0]) >= 50; }, 2000ms);
			pcount_sent->store(sda_sent_message(_servers[0]));
		#else
			(void)_servers;
		#endif
		});

		sda_wait_until([&]() { return pcount_sent->load() >= 0; });

	#if defined(_WINSOCK2API_)
		// - windows, the auto off : the first message goes by a WSASend, the next ones wait for its completion in the queue. the abortive
		//   close drops them (not given to the socket : the RST would drop them anyway). the auto on : the 50 go by one WSASend at the close
		if (_auto == false)
		{
			EXPECT_GE(pcount_sent->load(), 1) << "close (auto " << _auto << ")";
			EXPECT_LE(pcount_sent->load(), 50) << "close (auto " << _auto << ")";
		}
		else
	#endif
		{
			EXPECT_EQ(pcount_sent->load(), 50) << "close (auto " << _auto << ")";
		}
	}

	// 5) disconnect : the same with disconnect() (graceful : the FIN after the data)
	{
		std::vector<std::size_t> expected(SDA_SOCKETS, 10);
		expected[0] = 50;

		run_case("disconnect", [&](const auto& _servers)
		{
			auto servers = _servers;
			pexecutor->post(make_executable([servers]()
			{
				for (int i = 1; i < SDA_SOCKETS; ++i)
					for (uint64_t k = 0; k < 10; ++k)
						sda_send(servers[i], 8, k);
				for (uint64_t k = 0; k < 50; ++k)
					sda_send(servers[0], 8, k);
				sda_disconnect(servers[0]);
			}));
		}, expected, sda_close::graceful);
	}

	// 6) exception : 30 messages to each socket then a throw
	run_case("exception", [&](const auto& _servers)
	{
		auto servers = _servers;
		pexecutor->post(make_executable([servers]()
		{
			for (int i = 0; i < SDA_SOCKETS; ++i)
				for (uint64_t k = 0; k < 30; ++k)
					sda_send(servers[i], 9, k);
			throw std::runtime_error("send_defer auto test");
		}));
	}, std::vector<std::size_t>(SDA_SOCKETS, 30), sda_close::none);

	// 7) cross : 10 executables (40 rounds) and a thread out of the executor (400 rounds) at once
	run_case("cross", [&](const auto& _servers)
	{
		for (uint32_t t = 0; t < 10; ++t)
		{
			auto servers = _servers;
			pexecutor->post(make_executable([servers, t]()
			{
				for (uint64_t k = 0; k < 40; ++k)
					for (int i = 0; i < SDA_SOCKETS; ++i)
						sda_send(servers[i], 200 + t, k);
			}));
		}

		auto servers = _servers;
		std::thread([servers]()
		{
			for (uint64_t k = 0; k < 400; ++k)
				for (int i = 0; i < SDA_SOCKETS; ++i)
					sda_send(servers[i], 500, k);
		}).join();
	}, std::vector<std::size_t>(SDA_SOCKETS, 10 * 40 + 400), sda_close::none);
}

//-----------------------------------------------------------------------------
// close while sending : 4 threads send to a socket (no executor : the sending thread calls the send system call itself) while the test
//   thread closes it. what arrives is in order per sender. a socket accepted right after the close gets nothing (a send of the closed
//   socket never goes to a handle made again)
//-----------------------------------------------------------------------------
template <class TBASE>
void sda_close_while_sending()
{
	using server_socket = sda_server_socket<TBASE>;
	using acceptor_type = sda_acceptor<server_socket>;

	auto pexecutor = executor::make_io_context_threads("send defer close test", 4);
	own_ptr<acceptor_type> pacceptor;
	sda_guard<acceptor_type> guard{ pacceptor, pexecutor };

	ASSERT_TRUE(pexecutor.exist());

	{
		std::lock_guard lock(g_sda_lockable);
		g_sda_servers.clear();
	}

	pacceptor = make_own<acceptor_type>();
	pacceptor->io_context(pexecutor.get());

	typename acceptor_type::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), sda_port() };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	constexpr int ROUNDS = 60;
	constexpr int SENDERS = 4;

	for (int round = 0; round < ROUNDS; ++round)
	{
		sda_client client;
		ASSERT_TRUE(client.connect(port)) << "round " << round;
		ASSERT_TRUE(sda_wait_until([&]() { std::lock_guard lock(g_sda_lockable); return g_sda_servers.size() == 1; })) << "round " << round << " accept";

		object_ptr<net::io::Isocket> pserver;
		{
			std::lock_guard lock(g_sda_lockable);
			pserver = g_sda_servers.front();
			g_sda_servers.clear();
		}

		// 1) the senders (till a send fails : the close)
		std::atomic<int> count_started{ 0 };
		std::vector<std::thread> senders;
		for (uint32_t t = 0; t < SENDERS; ++t)
		{
			senders.emplace_back([pserver, t, &count_started]()
			{
				++count_started;
				for (uint64_t k = 0; k < 20000; ++k)
				{
					if (sda_send(pserver, t, k) == false)
						break;
				}
			});
		}

		// 2) the close (after 0 ~ 1.5 ms of sending)
		while (count_started.load() != SENDERS)
			std::this_thread::yield();
		std::this_thread::sleep_for(std::chrono::microseconds((round % 4) * 500));
		pserver->closesocket();

		// 3) a socket accepted right after the close
		sda_client client_after;
		ASSERT_TRUE(client_after.connect(port)) << "round " << round;

		for (auto& iter : senders)
			iter.join();

		// 4) what arrived : in order per sender (a partial message at the end : cut by the reset)
		client.read(SENDERS * 20000, 3000ms);
		client.m_data.resize(client.m_data.size() - (client.m_data.size() % SDA_MESSAGE_SIZE));
		std::string error;
		EXPECT_TRUE(client.check_order(error)) << "round " << round << " : " << error;

		client_after.read(1, 100ms);
		EXPECT_EQ(client_after.m_data.size(), 0u) << "round " << round << " : the socket accepted after the close got data";

		ASSERT_TRUE(sda_wait_until([&]() { std::lock_guard lock(g_sda_lockable); return g_sda_servers.size() == 1; })) << "round " << round << " accept after";
		{
			std::lock_guard lock(g_sda_lockable);
			for (auto& iter : g_sda_servers)
				iter->closesocket();
			g_sda_servers.clear();
		}
		client.close();
		client_after.close();
		pserver.reset();

		EXPECT_TRUE(sda_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "round " << round << " connected yet " << pacceptor->count();
	}
}

TEST(send_defer_auto, close_while_sending_gather)
{
	sda_close_while_sending<net::socket::tcp<>>();
}

TEST(send_defer_auto, close_while_sending_buffered)
{
	sda_close_while_sending<net::socket::tcp_buffered<>>();
}

#if defined(_WINSOCK2API_)
// - the raw sendables (windows) : they call WSASend out of m_lockable_sending too, the close swaps the handle under m_lockable_send_call
template <class TSENDABLE>
class sda_raw_socket :
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connectable::tcp::Ndefault,
	public						net::io::connect_notify::tcp::Ndefault,
	public						TSENDABLE,
	public				 		net::io::send_request::tcp::Ndefault,
	public						net::io::sender::tcp::Ndefault<uint32_t>,
	public						net::io::receivable::tcp::Ndefault,
	public						net::io::packetable::tcp::Ndefault<uint32_t>,
	public						net::io::messageable::tcp::Ndefault
{
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::BYPASS; }
};

TEST(send_defer_auto, close_while_sending_gather_raw)
{
	sda_close_while_sending<sda_raw_socket<net::io::sendable::Ntcp_async_gather_raw>>();
}

TEST(send_defer_auto, close_while_sending_buffered_raw)
{
	sda_close_while_sending<sda_raw_socket<net::io::sendable::Ntcp_async_gather_buffered_raw>>();
}
#endif

TEST(send_defer_auto, gather)
{
	sda_check<net::socket::tcp<>>(true);
}

TEST(send_defer_auto, buffered)
{
	sda_check<net::socket::tcp_buffered<>>(true);
}

TEST(send_defer_auto, gather_auto_off)
{
	sda_check<net::socket::tcp<>>(false);
}

TEST(send_defer_auto, buffered_auto_off)
{
	sda_check<net::socket::tcp_buffered<>>(false);
}

}

#endif
