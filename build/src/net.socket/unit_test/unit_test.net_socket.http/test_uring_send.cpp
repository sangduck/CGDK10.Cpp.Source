#include "pch.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// io_uring completion : the send path (Ntcp_async_gather : tcp<>, Ntcp_async_gather_buffered : tcp_buffered<>)
//
//  - the sends are direct (send/sendmsg) like epoll. the socket buffer full posts a SENDMSG op (a large buffer of the user : a SEND_ZC
//    op) instead of waiting for EPOLLOUT : its completion goes on with the queue.
//  - the server sockets run on the completion executor of the test (the acceptor gives its io_context to the accepted sockets). the
//    clients are OS blocking sockets that read when the test says so (a small receive buffer : the server side fills up).
//
//  1. a full socket buffer : the sends of a thread out of the executor and of a worker queue behind the op, every message arrives in
//     order once the client reads. a large buffer goes as a SEND_ZC op.
//  2. send then close in the handler (send cork, send_defer::scope, a posted executable) : the sends arrive before the EOF.
//  3. close while an op is in flight (the server closes / the client resets) : the socket is closed and reused cleanly.
//  4. the send queue limit : the batch of the op in flight is not counted, the sends over the limit fail and close the socket.
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#include <arpa/inet.h>
#include <netinet/tcp.h>

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	template <class TCONDITION>
	bool lus_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
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

	// - the port of the tests (26400 + pid % 500 : below the ephemeral range, not the ports of the other tests)
	uint16_t lus_port()
	{
		return static_cast<uint16_t>(26400 + api::get_process_id() % 500);
	}

	// - [uint32 size][uint32 index][char kind][pattern...]
	std::string lus_message(uint32_t _index, std::size_t _size, char _kind = 'P')
	{
		std::string message(_size, '\0');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		memcpy(message.data() + 4, &_index, 4);
		message[8] = _kind;
		for (std::size_t i = 9; i < _size; ++i)
			message[i] = static_cast<char>('a' + (i + _index) % 26);
		return message;
	}

	shared_buffer lus_buffer(const std::string& _message)
	{
		auto buf = alloc_shared_buffer(_message.size());
		buf.append(_message.size(), _message.data());
		return buf;
	}

	// - the size of the message _index of a burst : small ones (the gather buffers of tcp_buffered), a large one in 50 (>= the zerocopy threshold)
	std::size_t lus_burst_size(uint32_t _index)
	{
		return ((_index % 50) == 49) ? 20000 : 40 + (_index * 37) % 1200;
	}

	object_ptr<executor::Iio_context_threads> lus_make_executor()
	{
		const auto backend_before = executor::default_io_backend();
		executor::set_default_io_backend(executor::io_backend::io_uring_completion);
		auto pexecutor = executor::make_io_context_threads("uring send test", 4);
		executor::set_default_io_backend(backend_before);
		return pexecutor;
	}

	//-----------------------------------------------------------------------------
	// the server socket : the test thread reaches the socket accepted last (g_lus_server)
	//  'E' echo, 'C' two replies then closesocket(), 'D' the same in a send_defer::scope, 'P' the same in a posted executable
	//-----------------------------------------------------------------------------
	std::mutex				g_lus_lockable;
	object_ptr<net::io::Isocket> g_lus_server;
	std::atomic<int>		g_lus_count_disconnect{ 0 };
	std::atomic<bool>		g_lus_small_send_buffer{ false };

	template <class TBASE>
	class lus_server_socket : public TBASE
	{
	protected:
		virtual void on_connect() override
		{
			if (g_lus_small_send_buffer)
			{
				int size = 4096;
				::setsockopt(this->native_handle(), SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
			}

			std::lock_guard lock(g_lus_lockable);
			g_lus_server = object_ptr<net::io::Isocket>(this);
		}
		virtual void on_disconnect(uint64_t /*_disconnect_reason*/) override
		{
			++g_lus_count_disconnect;
		}
		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			const auto* pdata = reinterpret_cast<const char*>(_msg.buf_message.data());
			const char kind = (_msg.buf_message.size() > 8) ? pdata[8] : 'E';
			uint32_t index = 0;
			memcpy(&index, pdata + 4, 4);

			switch (kind)
			{
			case 'C':
				this->send(lus_buffer(lus_message(index, 100, 'R')));
				this->send(lus_buffer(lus_message(index + 1, 3000, 'R')));
				this->closesocket();
				break;

			case 'D':
				{
					send_defer::scope scope;

					this->send(lus_buffer(lus_message(index, 100, 'R')));
					this->send(lus_buffer(lus_message(index + 1, 3000, 'R')));
					this->closesocket();
				}
				break;

			case 'P':
				{
					auto psocket = object_ptr<lus_server_socket>(this);
					auto* pio_context = this->io_context().get();

					object_ptr<Iexecutable> pexecutable = make_executable([psocket, index]()
					{
						psocket->send(lus_buffer(lus_message(index, 100, 'R')));
						psocket->send(lus_buffer(lus_message(index + 1, 3000, 'R')));
						psocket->closesocket();
					});

					pio_context->post(std::move(pexecutable));
				}
				break;

			default:
				this->send(_msg.buf_message);
				break;
			}

			return eRESULT::DONE;
		}
	};

	// - the accepted sockets run on the io_context of the acceptor (the default on linux : the system executor)
	template <class TSOCKET>
	class lus_acceptor : public net::acceptor<TSOCKET>
	{
	protected:
		virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable* /*_pconnectable*/) override
		{
			object_ptr<executor::Iio_context> pio_context = this->io_context();
			return pio_context;
		}
	};

	template <class TSOCKET>
	typename lus_acceptor<TSOCKET>::START_PARAMETER lus_acceptor_parameter()
	{
		typename lus_acceptor<TSOCKET>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), lus_port() };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		return parameter;
	}

	// - a send to the server socket (the test reaches it as an Isocket)
	bool lus_send(const object_ptr<net::io::Isocket>& _psocket, shared_buffer&& _buffer)
	{
		auto* psender = dynamic_cast<net::io::Isender*>(_psocket.get());
		return psender != nullptr && psender->send(std::move(_buffer));
	}

	object_ptr<net::io::Isocket> lus_take_server()
	{
		std::lock_guard lock(g_lus_lockable);
		auto psocket = std::move(g_lus_server);
		g_lus_server.reset();
		return psocket;
	}

	//-----------------------------------------------------------------------------
	// a blocking client of the OS (reads when the test reads)
	//-----------------------------------------------------------------------------
	class lus_client
	{
	public:
		~lus_client() { this->close(); }

		bool connect(uint16_t _port, int _receive_buffer = 0)
		{
			this->m_fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
			if (this->m_fd < 0)
				return false;

			if (_receive_buffer != 0)
				::setsockopt(this->m_fd, SOL_SOCKET, SO_RCVBUF, &_receive_buffer, sizeof(_receive_buffer));

			int one = 1;
			::setsockopt(this->m_fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

			timeval timeout{ 5, 0 };
			::setsockopt(this->m_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(_port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

			return ::connect(this->m_fd, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0;
		}

		bool send(const std::string& _message)
		{
			std::size_t sent = 0;
			while (sent < _message.size())
			{
				const auto result = ::send(this->m_fd, _message.data() + sent, _message.size() - sent, MSG_NOSIGNAL);
				if (result <= 0)
					return false;
				sent += static_cast<std::size_t>(result);
			}
			return true;
		}

		// - false : timeout / EOF / error (m_last : 0 EOF, -1 error)
		bool read_exact(void* _pbuffer, std::size_t _size)
		{
			std::size_t received = 0;
			while (received < _size)
			{
				const auto result = ::recv(this->m_fd, static_cast<char*>(_pbuffer) + received, _size - received, 0);
				if (result <= 0)
				{
					this->m_last = static_cast<int>(result);
					this->m_errno = errno;
					return false;
				}
				received += static_cast<std::size_t>(result);
			}
			return true;
		}

		bool read_message(std::string& _message)
		{
			uint32_t size = 0;
			if (this->read_exact(&size, 4) == false || size < 4 || size > 1024 * 1024)
				return false;

			_message.resize(size);
			memcpy(_message.data(), &size, 4);
			return this->read_exact(_message.data() + 4, size - 4);
		}

		// - true : the peer closed (recv 0) with nothing more to read
		bool read_eof()
		{
			char byte;
			const auto result = ::recv(this->m_fd, &byte, 1, 0);
			this->m_errno = errno;
			return result == 0;
		}

		void reset()
		{
			if (this->m_fd < 0)
				return;
			linger option{ 1, 0 };
			::setsockopt(this->m_fd, SOL_SOCKET, SO_LINGER, &option, sizeof(option));
			this->close();
		}

		void close()
		{
			if (this->m_fd >= 0)
				::close(this->m_fd);
			this->m_fd = -1;
		}

		int					m_fd = -1;
		int					m_last = 0;
		int					m_errno = 0;
	};

	struct lus_small_send_buffer_guard
	{
		lus_small_send_buffer_guard() { g_lus_small_send_buffer = true; }
		~lus_small_send_buffer_guard() { g_lus_small_send_buffer = false; }
	};
}

//-----------------------------------------------------------------------------
// 1. a full socket buffer
//-----------------------------------------------------------------------------
template <class TBASE>
void lus_check_socket_buffer_full()
{
	using server_socket = lus_server_socket<TBASE>;

	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	auto pexecutor = lus_make_executor();
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);

	lus_small_send_buffer_guard guard;
	lus_take_server();

	auto pacceptor = make_own<lus_acceptor<server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lus_acceptor_parameter<server_socket>()), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 1) a client that does not read yet
	lus_client client;
	ASSERT_TRUE(client.connect(port, 4096));
	ASSERT_TRUE(lus_wait_until([&]() { std::lock_guard lock(g_lus_lockable); return g_lus_server.exist(); })) << "accept";

	auto pserver = lus_take_server();
	ASSERT_TRUE(pserver->is_io_completion()) << "the server socket is not a completion socket";

	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pserver.get());
	ASSERT_NE(pgather, nullptr);

	// 2) a burst from this thread (not a worker) and one from a worker (a posted executable) behind it
	constexpr uint32_t COUNT_THREAD = 2000;
	constexpr uint32_t COUNT_WORKER = 500;

	for (uint32_t i = 0; i < COUNT_THREAD; ++i)
		ASSERT_TRUE(lus_send(pserver, lus_buffer(lus_message(i, lus_burst_size(i))))) << "send " << i;

	std::atomic<int> result_worker{ -1 };
	object_ptr<Iexecutable> pexecutable_worker = make_executable([pserver, &result_worker]()
	{
		int failed = 0;
		for (uint32_t i = COUNT_THREAD; i < COUNT_THREAD + COUNT_WORKER; ++i)
			if (lus_send(pserver, lus_buffer(lus_message(i, lus_burst_size(i)))) == false)
				++failed;
		result_worker = failed;
	});
	pexecutor->post(std::move(pexecutable_worker));
	ASSERT_TRUE(lus_wait_until([&]() { return result_worker.load() >= 0; })) << "the sends of the worker";
	EXPECT_EQ(result_worker.load(), 0) << "sends of the worker failed";

	// - the client reads nothing yet : most of it waits (the op in flight and the queue behind it)
	EXPECT_GT(pgather->sending_queued_bytes_now(), 64u * 1024u) << "the socket buffer did not fill (the op path is not tested)";

	// 3) every message in order
	for (uint32_t i = 0; i < COUNT_THREAD + COUNT_WORKER; ++i)
	{
		std::string message;
		ASSERT_TRUE(client.read_message(message)) << "message " << i << " (recv " << client.m_last << " errno " << client.m_errno << ")";
		ASSERT_EQ(message, lus_message(i, lus_burst_size(i))) << "message " << i;
	}

	EXPECT_TRUE(lus_wait_until([&]() { return pgather->sending_queued_bytes_now() == 0; })) << "queued " << pgather->sending_queued_bytes_now();

	// 4) large buffers one by one (the socket is not sending : a SEND_ZC op each)
	const auto zerocopy_before = pgather->zerocopy_sent_count();

	for (uint32_t i = 0; i < 4; ++i)
	{
		ASSERT_TRUE(lus_send(pserver, lus_buffer(lus_message(10000 + i, 64 * 1024)))) << "large " << i;

		std::string message;
		ASSERT_TRUE(client.read_message(message)) << "large " << i;
		ASSERT_EQ(message, lus_message(10000 + i, 64 * 1024)) << "large " << i;
	}

	EXPECT_GE(pgather->zerocopy_sent_count(), zerocopy_before + 4) << "the large sends did not go as SEND_ZC ops";
	EXPECT_TRUE(lus_wait_until([&]() { return pgather->zerocopy_pending_count() == 0; }));

	// 5) close
	const int count_disconnect = g_lus_count_disconnect.load();
	client.close();
	EXPECT_TRUE(lus_wait_until([&]() { return g_lus_count_disconnect.load() > count_disconnect; })) << "disconnect";
	pserver.reset();

	EXPECT_TRUE(lus_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(uring_send, socket_buffer_full_gather)
{
	lus_check_socket_buffer_full<net::socket::tcp<>>();
}

TEST(uring_send, socket_buffer_full_buffered)
{
	lus_check_socket_buffer_full<net::socket::tcp_buffered<>>();
}

//-----------------------------------------------------------------------------
// 2. send then close in the handler : the replies arrive before the EOF
//-----------------------------------------------------------------------------
template <class TBASE>
void lus_check_send_then_close()
{
	using server_socket = lus_server_socket<TBASE>;

	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	auto pexecutor = lus_make_executor();
	ASSERT_TRUE(pexecutor.exist());

	lus_take_server();

	auto pacceptor = make_own<lus_acceptor<server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lus_acceptor_parameter<server_socket>()), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	const char kinds[] = { 'C', 'D', 'P' };

	for (int loop = 0; loop < 60; ++loop)
	{
		const char kind = kinds[loop % 3];
		const auto index = static_cast<uint32_t>(loop * 10);

		lus_client client;
		ASSERT_TRUE(client.connect(port)) << "loop " << loop;

		// - a few echoes first (the socket is running), then the request that closes
		for (uint32_t e = 0; e < 3; ++e)
		{
			ASSERT_TRUE(client.send(lus_message(index + 100 + e, 64, 'E')));

			std::string message;
			ASSERT_TRUE(client.read_message(message)) << "loop " << loop << " echo " << e;
		}

		ASSERT_TRUE(client.send(lus_message(index, 16, kind)));

		std::string reply_1, reply_2;
		ASSERT_TRUE(client.read_message(reply_1)) << "loop " << loop << " kind " << kind << " reply 1 (recv " << client.m_last << " errno " << client.m_errno << ")";
		ASSERT_EQ(reply_1, lus_message(index, 100, 'R')) << "loop " << loop << " kind " << kind;
		ASSERT_TRUE(client.read_message(reply_2)) << "loop " << loop << " kind " << kind << " reply 2 (recv " << client.m_last << " errno " << client.m_errno << ")";
		ASSERT_EQ(reply_2, lus_message(index + 1, 3000, 'R')) << "loop " << loop << " kind " << kind;
		EXPECT_TRUE(client.read_eof()) << "loop " << loop << " kind " << kind << " errno " << client.m_errno;

		lus_take_server();
	}

	EXPECT_TRUE(lus_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(uring_send, send_then_close_gather)
{
	lus_check_send_then_close<net::socket::tcp<>>();
}

TEST(uring_send, send_then_close_buffered)
{
	lus_check_send_then_close<net::socket::tcp_buffered<>>();
}

//-----------------------------------------------------------------------------
// 3. close while an op is in flight (the server closes, the client resets) : closed, reused cleanly
//-----------------------------------------------------------------------------
template <class TBASE>
void lus_check_close_while_sending()
{
	using server_socket = lus_server_socket<TBASE>;

	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	auto pexecutor = lus_make_executor();
	ASSERT_TRUE(pexecutor.exist());

	lus_small_send_buffer_guard guard;
	lus_take_server();

	auto pacceptor = make_own<lus_acceptor<server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lus_acceptor_parameter<server_socket>()), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	for (int loop = 0; loop < 20; ++loop)
	{
		lus_client client;
		ASSERT_TRUE(client.connect(port, 4096)) << "loop " << loop;
		ASSERT_TRUE(lus_wait_until([&]() { std::lock_guard lock(g_lus_lockable); return g_lus_server.exist(); })) << "loop " << loop;

		auto pserver = lus_take_server();
		auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pserver.get());
		ASSERT_NE(pgather, nullptr);
		ASSERT_TRUE(pserver->is_io_completion()) << "loop " << loop << " : not a completion socket";
		ASSERT_EQ(pgather->sending_queued_bytes_now(), 0u) << "loop " << loop << " : a reused socket kept its queue";

		for (uint32_t i = 0; i < 400; ++i)
			ASSERT_TRUE(lus_send(pserver, lus_buffer(lus_message(i, lus_burst_size(i))))) << "loop " << loop << " send " << i;

		ASSERT_TRUE(lus_wait_until([&]() { return pgather->sending_queued_bytes_now() != 0; })) << "loop " << loop;

		const int count_disconnect = g_lus_count_disconnect.load();

		if ((loop % 2) == 0)
			pserver->closesocket(DISCONNECT_REASON_ABORTIVE);
		else
			client.reset();

		ASSERT_TRUE(lus_wait_until([&]() { return g_lus_count_disconnect.load() > count_disconnect; })) << "loop " << loop << " : not disconnected";
		EXPECT_TRUE(lus_wait_until([&]() { return pgather->sending_queued_bytes_now() == 0; })) << "loop " << loop << " queued " << pgather->sending_queued_bytes_now();
	}

	// - a reused socket echoes
	{
		lus_client client;
		ASSERT_TRUE(client.connect(port));

		for (uint32_t e = 0; e < 200; ++e)
			ASSERT_TRUE(client.send(lus_message(e, 32 + e * 13, 'E')));

		for (uint32_t e = 0; e < 200; ++e)
		{
			std::string message;
			ASSERT_TRUE(client.read_message(message)) << "echo " << e;
			ASSERT_EQ(message, lus_message(e, 32 + e * 13, 'E')) << "echo " << e;
		}
	}

	lus_take_server();
	EXPECT_TRUE(lus_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(uring_send, close_while_sending_gather)
{
	lus_check_close_while_sending<net::socket::tcp<>>();
}

TEST(uring_send, close_while_sending_buffered)
{
	lus_check_close_while_sending<net::socket::tcp_buffered<>>();
}

//-----------------------------------------------------------------------------
// 4. the send queue limit : the batch of the op in flight is not counted. a send over the limit fails and closes the socket
//-----------------------------------------------------------------------------
template <class TBASE>
void lus_check_send_queue_limit()
{
	using server_socket = lus_server_socket<TBASE>;

	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "io_uring completion is not available on this kernel";

	auto pexecutor = lus_make_executor();
	ASSERT_TRUE(pexecutor.exist());

	lus_small_send_buffer_guard guard;
	lus_take_server();

	auto pacceptor = make_own<lus_acceptor<server_socket>>();
	pacceptor->io_context(pexecutor.get());
	ASSERT_EQ(pacceptor->start(lus_acceptor_parameter<server_socket>()), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	lus_client client;
	ASSERT_TRUE(client.connect(port, 4096));
	ASSERT_TRUE(lus_wait_until([&]() { std::lock_guard lock(g_lus_lockable); return g_lus_server.exist(); }));

	auto pserver = lus_take_server();
	ASSERT_TRUE(pserver->is_io_completion()) << "not a completion socket";
	auto* pgather = dynamic_cast<net::io::sendable::Itcp_gather*>(pserver.get());
	ASSERT_NE(pgather, nullptr);

	// - 64 buffers queued at most (2KB messages : not copied into the gather buffers of tcp_buffered)
	constexpr std::size_t LIMIT = 64;
	pgather->maximum_depth_of_send_buffer(LIMIT);

	const int count_disconnect = g_lus_count_disconnect.load();
	std::size_t count_accepted = 0;

	for (uint32_t i = 0; i < 10000; ++i)
	{
		if (lus_send(pserver, lus_buffer(lus_message(i, 2048))) == false)
			break;

		++count_accepted;
	}

	// - more than the limit : the socket buffer and the batch of the op in flight (the rest put back) are not counted
	EXPECT_GT(count_accepted, LIMIT) << "accepted " << count_accepted;
	EXPECT_LT(count_accepted, 10000u) << "the limit did not apply";

	// - the overflow closes the socket
	EXPECT_TRUE(lus_wait_until([&]() { return g_lus_count_disconnect.load() > count_disconnect; })) << "not closed by the overflow";

	// - the limit of the socket back to the default (a pooled socket keeps it : the next test would get this socket again)
	pgather->maximum_depth_of_send_buffer(net::io::sendable::Itcp_gather::default_maximum_depth_of_send_buffer());

	pserver.reset();
	client.close();
	EXPECT_TRUE(lus_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms)) << "connected yet " << pacceptor->count();
	pacceptor->stop();
	pexecutor->stop();
}

TEST(uring_send, send_queue_limit_gather)
{
	lus_check_send_queue_limit<net::socket::tcp<>>();
}

TEST(uring_send, send_queue_limit_buffered)
{
	lus_check_send_queue_limit<net::socket::tcp_buffered<>>();
}

}

#endif
