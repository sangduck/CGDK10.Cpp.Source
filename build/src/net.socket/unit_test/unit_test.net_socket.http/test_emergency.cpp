#include "pch.h"
#include <atomic>
#include <cerrno>
#include <csignal>
#include <mutex>
#include <optional>
#include <thread>
#if !defined(_WIN32)
	#include <pthread.h>
	#include <sched.h>
	#include <sys/resource.h>
	#include <sys/syscall.h>
	#include <unistd.h>
#endif

//-----------------------------------------------------------------------------
// net::emergency (the emergency acceptor and the signals)
//
//  1. thread priority : eTHREAD_PRIORITY text, api::set_current_thread_priority(), "thread_priority" of an executor
//  2. Nacceptor       : the acceptor runs on its own executor, every accepted socket on an executor of its own
//                       (not the system_executor, not the one of the acceptor, at the top priority).
//                       the executor of a socket is removed after its disconnect (supervise()), stop() removes them all
//  3. isolation       : the emergency acceptor answers while every thread of the system_executor is blocked,
//                       a session stuck in a handler blocks no other session, an exception closes only its socket
//  4. manager         : the emergency acceptor is created on the emergency signal (once), the shutdown signal calls
//                       fn_shutdown (once, a real signal too), the terminate signal closes the connections and calls
//                       fn_terminate (the process is not ended here : exit_process_on_terminate = false),
//                       uninstall() stops the acceptor and gives the signal handlers back
//  5. system_executor : register_destroy_handler() / unregister_destroy_handler()
//
//  * the clients are blocking OS sockets : they work while the system_executor is jammed
//-----------------------------------------------------------------------------
namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	// - true when [::]:_port and 0.0.0.0:_port can be bound now (not used, not reserved)
	bool is_port_bindable(uint16_t _port)
	{
		// - the socket api is ready (WSAStartup on windows)
		static_cast<void>(api::socket::instance());

		auto try_bind = [_port](int _family) -> bool
		{
			const auto handle = ::socket(_family, SOCK_STREAM, IPPROTO_TCP);
			if (handle == INVALID_SOCKET)
				return false;

			int result;
			if (_family == AF_INET6)
			{
				sockaddr_in6 address{};
				address.sin6_family = AF_INET6;
				address.sin6_port = htons(_port);
				address.sin6_addr = in6addr_any;
				result = ::bind(handle, reinterpret_cast<const ::sockaddr*>(&address), sizeof(address));
			}
			else
			{
				sockaddr_in address{};
				address.sin_family = AF_INET;
				address.sin_port = htons(_port);
				address.sin_addr.s_addr = htonl(INADDR_ANY);
				result = ::bind(handle, reinterpret_cast<const ::sockaddr*>(&address), sizeof(address));
			}
		#if defined(_WIN32)
			::closesocket(handle);
		#else
			::close(handle);
		#endif
			return result == 0;
		};

		return try_bind(AF_INET6) && try_bind(AF_INET);
	}

	// - the ports of the tests : the first bindable one of 20000 + n * 8 + _offset, n from pid % 1500 (step 37 of 1500)
	//   (below the ephemeral range of linux (32768~) : a client socket of another test may hold a port there, and the acceptor
	//    moved to the next port silently (reallocate_port_at_duplicate_port_error) while the test connected to the one it asked)
	//   (bound before use : windows reserves port ranges (netsh int ipv4 show excludedportrange, hyper-v / wsl : 27319~27919 on
	//    a machine) and bind() fails there. the fixed 27400 + (pid % 100) * 8 failed whenever the pid gave one of them)
	uint16_t emergency_test_port(int _offset)
	{
		constexpr int COUNT = 1500;
		const int first = static_cast<int>(api::get_process_id() % COUNT);
		for (int i = 0; i < COUNT; ++i)
		{
			const auto port = static_cast<uint16_t>(20000 + ((first + i * 37) % COUNT) * 8 + _offset);
			if (is_port_bindable(port))
				return port;
		}
		return static_cast<uint16_t>(20000 + first * 8 + _offset);
	}

	template <class TCONDITION>
	bool wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
	{
		const auto time_end = std::chrono::steady_clock::now() + _duration;
		while (std::chrono::steady_clock::now() < time_end)
		{
			if (_condition())
				return true;
			std::this_thread::sleep_for(10ms);
		}
		return _condition();
	}

	//-------------------------------------------------------------------------
	// thread priority of the calling thread
	//-------------------------------------------------------------------------
	bool is_current_thread_time_critical()
	{
	#if defined(_WIN32)
		return ::GetThreadPriority(::GetCurrentThread()) == THREAD_PRIORITY_TIME_CRITICAL;
	#else
		int policy = 0;
		sched_param param{};
		if (::pthread_getschedparam(::pthread_self(), &policy, &param) == 0 && policy == SCHED_RR)
			return true;
		errno = 0;
		const int nice = ::getpriority(PRIO_PROCESS, static_cast<id_t>(::syscall(SYS_gettid)));
		return errno == 0 && nice == -20;
	#endif
	}

	bool is_current_thread_normal()
	{
	#if defined(_WIN32)
		return ::GetThreadPriority(::GetCurrentThread()) == THREAD_PRIORITY_NORMAL;
	#else
		int policy = 0;
		sched_param param{};
		if (::pthread_getschedparam(::pthread_self(), &policy, &param) != 0 || policy != SCHED_OTHER)
			return false;
		errno = 0;
		const int nice = ::getpriority(PRIO_PROCESS, static_cast<id_t>(::syscall(SYS_gettid)));
		return errno == 0 && nice == 0;
	#endif
	}

	// - can this process raise a thread to TIME_CRITICAL? (windows : always. linux : CAP_SYS_NICE / RLIMIT_RTPRIO / RLIMIT_NICE)
	bool can_raise_thread_priority()
	{
		bool result = false;
		std::thread([&result]() { result = api::set_current_thread_priority(eTHREAD_PRIORITY::TIME_CRITICAL); }).join();
		return result;
	}

	//-------------------------------------------------------------------------
	// a blocking client of the OS (independent of every executor)
	//-------------------------------------------------------------------------
	class raw_client
	{
	public:
		~raw_client() { this->close(); }

		bool connect(uint16_t _port)
		{
			// - the socket api is ready (WSAStartup on windows)
			static_cast<void>(api::socket::instance());

			this->m_handle = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (this->m_handle == INVALID_SOCKET)
				return false;

			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(_port);
			address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

			if (::connect(this->m_handle, reinterpret_cast<const ::sockaddr*>(&address), sizeof(address)) != 0)
			{
				this->close();
				return false;
			}
			return true;
		}

		void close()
		{
			if (this->m_handle == INVALID_SOCKET)
				return;
		#if defined(_WIN32)
			::closesocket(this->m_handle);
		#else
			::close(this->m_handle);
		#endif
			this->m_handle = INVALID_SOCKET;
		}

		// - [uint32 size (header included)][text]
		bool send_text(std::string_view _text)
		{
			std::string message(sizeof(uint32_t) + _text.size(), '\0');
			const uint32_t size = static_cast<uint32_t>(message.size());
			memcpy(message.data(), &size, sizeof(size));
			memcpy(message.data() + sizeof(size), _text.data(), _text.size());
		#if defined(_WIN32)
			constexpr int flags = 0;
		#else
			constexpr int flags = MSG_NOSIGNAL;		// - no SIGPIPE on a socket closed by the server
		#endif
			return ::send(this->m_handle, message.data(), static_cast<int>(message.size()), flags) == static_cast<int>(message.size());
		}

		// - a text message (nullopt : timeout, closed or an error. is_closed() : closed by the server)
		std::optional<std::string> receive_text(std::chrono::milliseconds _timeout = 3000ms)
		{
			this->set_timeout(_timeout);

			char header[sizeof(uint32_t)];
			if (this->receive_exact(header, sizeof(header)) == false)
				return std::nullopt;

			uint32_t size = 0;
			memcpy(&size, header, sizeof(size));
			if (size < sizeof(uint32_t) || size > 65536)
				return std::nullopt;

			std::string text(size - sizeof(uint32_t), '\0');
			if (text.empty() == false && this->receive_exact(text.data(), text.size()) == false)
				return std::nullopt;

			return text;
		}

		std::optional<std::string> ask(std::string_view _text, std::chrono::milliseconds _timeout = 3000ms)
		{
			if (this->send_text(_text) == false)
				return std::nullopt;
			return this->receive_text(_timeout);
		}

		// - closed by the server (0 bytes or a reset) within _timeout
		bool wait_closed(std::chrono::milliseconds _timeout = 3000ms)
		{
			this->set_timeout(_timeout);
			char buf[64];
			const auto result = ::recv(this->m_handle, buf, sizeof(buf), 0);
			if (result == 0)
				return true;
			if (result > 0)
				return false;
		#if defined(_WIN32)
			return ::WSAGetLastError() != WSAETIMEDOUT;
		#else
			return errno != EAGAIN && errno != EWOULDBLOCK;
		#endif
		}

	private:
		void set_timeout(std::chrono::milliseconds _timeout)
		{
		#if defined(_WIN32)
			DWORD value = static_cast<DWORD>(_timeout.count());
			::setsockopt(this->m_handle, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&value), sizeof(value));
		#else
			timeval value{ static_cast<time_t>(_timeout.count() / 1000), static_cast<suseconds_t>((_timeout.count() % 1000) * 1000) };
			::setsockopt(this->m_handle, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(value));
		#endif
		}

		bool receive_exact(char* _buf, std::size_t _size)
		{
			std::size_t received = 0;
			while (received < _size)
			{
				const auto result = ::recv(this->m_handle, _buf + received, static_cast<int>(_size - received), 0);
				if (result <= 0)
					return false;
				received += static_cast<std::size_t>(result);
			}
			return true;
		}

		CGSOCKET m_handle = INVALID_SOCKET;
	};

	bool is_port_open(uint16_t _port)
	{
		raw_client client;
		return client.connect(_port);
	}

	//-------------------------------------------------------------------------
	// the socket of the emergency acceptor
	//   "where"  : "own" when this handler runs on the executor of this socket (not the system_executor), else "wrong"
	//   "prio"   : "top" when this thread is TIME_CRITICAL, else "not"
	//   "stuck"  : blocks this thread 1.5s, then "unstuck"
	//   "throw"  : an exception in the handler
	//   else     : echo
	//-------------------------------------------------------------------------
	struct sEMERGENCY_OBSERVED
	{
		std::mutex			lockable;
		std::vector<executor::Iio_context*> io_contexts;	// - the io_context of every connected socket
		std::atomic<int>	count_connected{ 0 };
		std::atomic<int>	count_disconnected{ 0 };
	};
	sEMERGENCY_OBSERVED g_emergency_observed;

	class socket_emergency_test : public net::socket::tcp<>
	{
		virtual void on_connect() override
		{
			{
				std::lock_guard lock(g_emergency_observed.lockable);
				g_emergency_observed.io_contexts.push_back(this->io_context().get());
			}
			++g_emergency_observed.count_connected;
		}

		virtual void on_disconnect(uint64_t) override
		{
			++g_emergency_observed.count_disconnected;
		}

		virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
		{
			const auto& buf = _msg.buf_message;
			const std::string_view text(reinterpret_cast<const char*>(buf.data()) + sizeof(uint32_t), buf.size() - sizeof(uint32_t));

			if (text == "where")
			{
				auto pio_context = this->io_context();
				auto pexecutor_now = get_current_executor();
				const bool is_own = pio_context.exist()
								 && pexecutor_now.get() == static_cast<Iexecutor*>(pio_context.get())
								 && pio_context.get() != system_executor::executor_ptr_peek();
				this->send_text(is_own ? "own" : "wrong");
			}
			else if (text == "prio")
			{
				this->send_text(is_current_thread_time_critical() ? "top" : "not");
			}
			else if (text == "stuck")
			{
				std::this_thread::sleep_for(1500ms);
				this->send_text("unstuck");
			}
			else if (text == "throw")
			{
				throw std::runtime_error("test : an exception in the handler of an emergency socket");
			}
			else
			{
				this->send(_msg.buf_message);
			}
			return eRESULT::DONE;
		}

		void send_text(std::string_view _text)
		{
			auto buf = alloc_shared_buffer(sizeof(uint32_t) + _text.size());
			buf.append<uint32_t>(static_cast<uint32_t>(sizeof(uint32_t) + _text.size()));
			buf.append_text(_text);
			this->send(buf);
		}
	};

	void reset_observed()
	{
		std::lock_guard lock(g_emergency_observed.lockable);
		g_emergency_observed.io_contexts.clear();
		g_emergency_observed.count_connected = 0;
		g_emergency_observed.count_disconnected = 0;
	}

	// - the emergency acceptor : started on the port (prepare_on_start 2, must_prepare 1 : the defaults of Nacceptor)
	own_ptr<net::emergency::acceptor<socket_emergency_test>> start_emergency_acceptor(uint16_t _port)
	{
		auto pacceptor = make_own<net::emergency::acceptor<socket_emergency_test>>();
		net::emergency::Nacceptor::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v6::any(), _port };
		parameter.enable_progress_log = false;
		parameter.reallocate_port_at_duplicate_port_error = false;		// - the port asked or none (the tests connect to it)
		pacceptor->start(parameter);
		return pacceptor;
	}

	// - blocks every thread of the system_executor till the guard is destroyed (at most 10s)
	//   (a thread of the system_executor may take several posted executables at once (linux : the event batch of
	//    executor::io_context) : a blocking one delays the others of its batch. so the jams are posted till every
	//    thread runs one. the jams not started yet return at once when released)
	class system_executor_jam
	{
	public:
		system_executor_jam()
		{
			// - the deadlock observer of the threads : no dump for this test
			auto pmanager_thread = api::manager_thread::instance();
			m_is_deadlock_dump = pmanager_thread->deadlock_dump();
			pmanager_thread->deadlock_dump(false);

			m_count = system_executor::thread_count();
			this->post_jams(m_count);
		}
		~system_executor_jam()
		{
			m_pstate->is_released = true;
			wait_until([this]() { return m_pstate->count_jammed == 0; }, 12000ms);
			api::manager_thread::instance()->deadlock_dump(m_is_deadlock_dump);
		}

		// - every thread of the system_executor runs a jam (posts more while some threads are free)
		bool wait_jammed()
		{
			const auto time_end = std::chrono::steady_clock::now() + 5s;
			while (std::chrono::steady_clock::now() < time_end)
			{
				const auto count_jammed = static_cast<std::size_t>(m_pstate->count_jammed.load());
				if (count_jammed >= m_count)
					return true;

				// - the free threads : one more jam each (at most 8 rounds of the thread count in all)
				std::this_thread::sleep_for(50ms);
				if (m_count_posted < m_count * 8 && static_cast<std::size_t>(m_pstate->count_jammed.load()) == count_jammed)
					this->post_jams(m_count - count_jammed);
			}
			return static_cast<std::size_t>(m_pstate->count_jammed.load()) >= m_count;
		}

	private:
		void post_jams(std::size_t _count)
		{
			for (std::size_t i = 0; i < _count; ++i)
			{
				auto pstate = m_pstate;
				system_executor::post(make_executable([pstate]()
				{
					// - released already (a jam left in a batch or in the queue) : nothing
					if (pstate->is_released)
						return;

					++pstate->count_jammed;
					const auto time_end = std::chrono::steady_clock::now() + 10s;
					while (pstate->is_released == false && std::chrono::steady_clock::now() < time_end)
						std::this_thread::sleep_for(5ms);
					--pstate->count_jammed;
				}));
			}
			m_count_posted += _count;
		}

		struct STATE
		{
			std::atomic<int>	count_jammed{ 0 };
			std::atomic<bool>	is_released{ false };
		};
		std::shared_ptr<STATE>	m_pstate = std::make_shared<STATE>();
		std::size_t				m_count = 0;
		std::size_t				m_count_posted = 0;
		bool					m_is_deadlock_dump = true;
	};
}


//-----------------------------------------------------------------------------
// 1. thread priority
//-----------------------------------------------------------------------------
TEST(emergency, thread_priority_text)
{
	for (auto priority : { eTHREAD_PRIORITY::LOWEST, eTHREAD_PRIORITY::LOW, eTHREAD_PRIORITY::NORMAL, eTHREAD_PRIORITY::HIGH, eTHREAD_PRIORITY::HIGHEST, eTHREAD_PRIORITY::TIME_CRITICAL })
	{
		EXPECT_EQ(to_thread_priority(to_string(priority)), priority) << to_string(priority);
	}

	EXPECT_EQ(to_thread_priority("Time-Critical"), eTHREAD_PRIORITY::TIME_CRITICAL);
	EXPECT_EQ(to_thread_priority("time critical"), eTHREAD_PRIORITY::TIME_CRITICAL);
	EXPECT_EQ(to_thread_priority("HIGHEST"), eTHREAD_PRIORITY::HIGHEST);
	EXPECT_EQ(to_thread_priority("default"), eTHREAD_PRIORITY::DEFAULT);
	EXPECT_EQ(to_thread_priority("realtime"), eTHREAD_PRIORITY::DEFAULT);
	EXPECT_EQ(to_thread_priority(""), eTHREAD_PRIORITY::DEFAULT);
	EXPECT_EQ(to_thread_priority("a_text_longer_than_the_buffer_of_32_bytes"), eTHREAD_PRIORITY::DEFAULT);
	EXPECT_EQ(to_string(eTHREAD_PRIORITY::DEFAULT), "default");
}

TEST(emergency, thread_priority_set_current_thread)
{
	const bool can_raise = can_raise_thread_priority();
#if defined(_WIN32)
	EXPECT_TRUE(can_raise);
#endif

	std::atomic<int> step{ 0 };
	std::thread([&]()
	{
		// - DEFAULT : nothing is changed
		EXPECT_TRUE(api::set_current_thread_priority(eTHREAD_PRIORITY::DEFAULT));
		EXPECT_TRUE(is_current_thread_normal());

		// - TIME_CRITICAL (linux : only with the permission)
		EXPECT_EQ(api::set_current_thread_priority(eTHREAD_PRIORITY::TIME_CRITICAL), can_raise);
		EXPECT_EQ(is_current_thread_time_critical(), can_raise);

		// - back to NORMAL
		EXPECT_TRUE(api::set_current_thread_priority(eTHREAD_PRIORITY::NORMAL));
		EXPECT_TRUE(is_current_thread_normal());
		step = 1;
	}).join();
	EXPECT_EQ(step.load(), 1);
}

TEST(emergency, executor_thread_priority)
{
	const bool can_raise = can_raise_thread_priority();

	// 1) an executor of 2 threads at TIME_CRITICAL (context "thread_priority")
	context context_top;
	context_top["thread_count"] = static_cast<int64_t>(2);
	context_top["thread_priority"] = std::string_view("time_critical");
	auto pexecutor_top = executor::make_io_context_threads("test emergency executor [top]", context_top);
	ASSERT_TRUE(pexecutor_top.exist());
	EXPECT_TRUE(wait_until([&]() { return pexecutor_top->existing_thread_count() == 2u; }));	// - counted by each thread when it starts

	// 2) an executor with the default priority
	auto pexecutor_default = executor::make_io_context_threads("test emergency executor [default]", 1);
	ASSERT_TRUE(pexecutor_default.exist());

	// 3) where do the executables run, at which priority
	std::atomic<int> count_done{ 0 };
	std::atomic<int> count_top{ 0 };
	std::atomic<int> count_own{ 0 };
	std::atomic<int> count_default_normal{ 0 };

	for (int i = 0; i < 8; ++i)
	{
		pexecutor_top->post(make_executable([&, pexecutor = pexecutor_top.get()]()
		{
			if (is_current_thread_time_critical())
				++count_top;
			if (get_current_executor().get() == static_cast<Iexecutor*>(pexecutor) && static_cast<executor::Iio_context*>(pexecutor) != system_executor::executor_ptr_peek())
				++count_own;
			++count_done;
		}));
	}
	pexecutor_default->post(make_executable([&]()
	{
		if (is_current_thread_normal())
			++count_default_normal;
		++count_done;
	}));

	ASSERT_TRUE(wait_until([&]() { return count_done == 9; }));
	EXPECT_EQ(count_own.load(), 8);
	EXPECT_EQ(count_top.load(), can_raise ? 8 : 0);
	EXPECT_EQ(count_default_normal.load(), 1);

	// 4) stop
	pexecutor_top->stop();
	pexecutor_default->stop();
	EXPECT_EQ(pexecutor_top->existing_thread_count(), 0u);
	EXPECT_EQ(pexecutor_default->existing_thread_count(), 0u);
}


//-----------------------------------------------------------------------------
// 2. Nacceptor : its own executor, an executor per socket
//-----------------------------------------------------------------------------
TEST(emergency, acceptor_own_executors)
{
	reset_observed();
	const auto port = emergency_test_port(0);
	const bool can_raise = can_raise_thread_priority();

	// 1) start
	auto pacceptor = start_emergency_acceptor(port);
	ASSERT_TRUE(pacceptor->is_listening());
	EXPECT_EQ(pacceptor->executor_thread_priority(), eTHREAD_PRIORITY::TIME_CRITICAL);

	// - the acceptor : its own executor (1 thread), not the system_executor
	auto pexecutor_own = pacceptor->own_executor();
	ASSERT_TRUE(pexecutor_own.exist());
	EXPECT_NE(static_cast<executor::Iio_context*>(pexecutor_own.get()), system_executor::executor_ptr_peek());
	EXPECT_TRUE(wait_until([&]() { return pexecutor_own->existing_thread_count() == 1u; }));	// - counted by the thread when it starts

	// - the prepared sockets have their executors already
	//   (windows : the accepts are prepared, prepare_on_start 2 / must_prepare 1. linux : nothing prepared,
	//    an accepted socket gets its executor)
#if defined(_WIN32)
	constexpr std::size_t count_prepared_on_start = 2;
	constexpr std::size_t count_must_prepare = 1;
#else
	constexpr std::size_t count_prepared_on_start = 0;
	constexpr std::size_t count_must_prepare = 0;
#endif
	EXPECT_EQ(pacceptor->socket_executor_count(), count_prepared_on_start);

	// 2) two sessions
	raw_client client_a;
	raw_client client_b;
	ASSERT_TRUE(client_a.connect(port));
	ASSERT_TRUE(client_b.connect(port));
	ASSERT_TRUE(wait_until([&]() { return g_emergency_observed.count_connected == 2 && pacceptor->count() == 2; }));

	// - every handler runs on the executor of its socket, at the top priority
	EXPECT_EQ(client_a.ask("where"), "own");
	EXPECT_EQ(client_b.ask("where"), "own");
	EXPECT_EQ(client_a.ask("prio"), can_raise ? "top" : "not");
	EXPECT_EQ(client_b.ask("hello"), "hello");

	// - the io_contexts of the sockets : all different, none of them the acceptor's or the system_executor's
	{
		std::lock_guard lock(g_emergency_observed.lockable);
		ASSERT_EQ(g_emergency_observed.io_contexts.size(), 2u);
		EXPECT_NE(g_emergency_observed.io_contexts[0], g_emergency_observed.io_contexts[1]);
		for (auto* pio_context : g_emergency_observed.io_contexts)
		{
			EXPECT_NE(pio_context, nullptr);
			EXPECT_NE(pio_context, static_cast<executor::Iio_context*>(pexecutor_own.get()));
			EXPECT_NE(pio_context, system_executor::executor_ptr_peek());
		}
	}

	// - 2 connected + the prepared ones (must_prepare)
	EXPECT_EQ(pacceptor->socket_executor_count(), 2u + count_must_prepare);

	// 3) A closes : its executor is removed by supervise()
	//    (the retired executor is counted till supervise() has stopped it : wait for the count without it)
	client_a.close();
	ASSERT_TRUE(wait_until([&]() { pacceptor->supervise(); return g_emergency_observed.count_disconnected == 1 && pacceptor->socket_executor_count() == 1u + count_must_prepare; }));
	EXPECT_EQ(client_b.ask("still"), "still");

	// 4) stop : every executor is stopped and removed, the port is closed
	pacceptor->stop();
	EXPECT_EQ(pacceptor->socket_executor_count(), 0u);
	EXPECT_TRUE(pacceptor->own_executor().empty());
	EXPECT_EQ(pexecutor_own->existing_thread_count(), 0u);
	EXPECT_TRUE(client_b.wait_closed());
	EXPECT_FALSE(is_port_open(port));

	pacceptor.destroy();
}


//-----------------------------------------------------------------------------
// 3. isolation
//-----------------------------------------------------------------------------
TEST(emergency, acceptor_works_while_system_executor_is_jammed)
{
	reset_observed();
	const auto port = emergency_test_port(1);

	auto pacceptor = start_emergency_acceptor(port);
	ASSERT_TRUE(pacceptor->is_listening());

	raw_client client_before;
	ASSERT_TRUE(client_before.connect(port));
	ASSERT_TRUE(wait_until([&]() { return g_emergency_observed.count_connected == 1; }));

	{
		// 1) every thread of the system_executor blocked
		system_executor_jam jam;
		ASSERT_TRUE(jam.wait_jammed());

		// 2) the session connected before answers at once
		const auto time_begin = std::chrono::steady_clock::now();
		EXPECT_EQ(client_before.ask("ping", 1000ms), "ping");
		EXPECT_LT(std::chrono::steady_clock::now() - time_begin, 1000ms);

		// 3) a new session is accepted and served
		raw_client client_new;
		ASSERT_TRUE(client_new.connect(port));
		EXPECT_EQ(client_new.ask("where", 1000ms), "own");
		EXPECT_EQ(client_new.ask("pong", 1000ms), "pong");
	}

	pacceptor->stop();
	pacceptor.destroy();
}

TEST(emergency, acceptor_stuck_session_and_exception)
{
	reset_observed();
	const auto port = emergency_test_port(2);

	auto pacceptor = start_emergency_acceptor(port);
	ASSERT_TRUE(pacceptor->is_listening());

	raw_client client_a;
	raw_client client_b;
	raw_client client_c;
	ASSERT_TRUE(client_a.connect(port));
	ASSERT_TRUE(client_b.connect(port));
	ASSERT_TRUE(client_c.connect(port));
	ASSERT_TRUE(wait_until([&]() { return g_emergency_observed.count_connected == 3; }));

	// 1) A is stuck in its handler (1.5s) : B answers at once
	ASSERT_TRUE(client_a.send_text("stuck"));
	std::this_thread::sleep_for(100ms);
	const auto time_begin = std::chrono::steady_clock::now();
	EXPECT_EQ(client_b.ask("b", 1000ms), "b");
	EXPECT_LT(std::chrono::steady_clock::now() - time_begin, 500ms);

	// 2) an exception in the handler of C : only C is closed
	ASSERT_TRUE(client_c.send_text("throw"));
	EXPECT_TRUE(client_c.wait_closed());
	EXPECT_EQ(client_b.ask("after exception"), "after exception");
	EXPECT_TRUE(pacceptor->is_listening());

	// 3) A comes back
	EXPECT_EQ(client_a.receive_text(3000ms), "unstuck");
	EXPECT_EQ(client_a.ask("a"), "a");

	// 4) a new session after all that
	raw_client client_d;
	ASSERT_TRUE(client_d.connect(port));
	EXPECT_EQ(client_d.ask("d"), "d");

	pacceptor->stop();
	pacceptor.destroy();
}


//-----------------------------------------------------------------------------
// 4. manager
//-----------------------------------------------------------------------------
TEST(emergency, manager_signals)
{
	reset_observed();
	const auto port = emergency_test_port(3);

	// - the signals of each role (the defaults of the platform)
	const net::emergency::manager::SETTING setting_default;
	ASSERT_FALSE(setting_default.signals_shutdown.empty());
	ASSERT_FALSE(setting_default.signals_emergency.empty());
	ASSERT_FALSE(setting_default.signals_terminate.empty());
	const int signal_shutdown = setting_default.signals_shutdown.front();

	// - the handler of the shutdown signal before install (given back by uninstall)
	auto handler_before = std::signal(signal_shutdown, SIG_DFL);
	std::signal(signal_shutdown, handler_before);

	// 1) install : nothing is created till the emergency signal
	std::atomic<int> count_shutdown{ 0 };
	std::atomic<int> count_terminate{ 0 };

	net::emergency::manager::SETTING setting;
	setting.fn_shutdown = [&]() { ++count_shutdown; };
	setting.fn_terminate = [&]() { ++count_terminate; };
	setting.enable_dump_on_terminate = false;
	setting.exit_process_on_terminate = false;		// - the test process keeps running
	setting.interval_supervise = std::chrono::duration_cast<chrono::tick::duration>(20ms);

	net::emergency::manager::install<socket_emergency_test>({ { net::ip::tcp::v6(), port } }, std::move(setting));
	ASSERT_TRUE(net::emergency::manager::is_installed());
	EXPECT_TRUE(net::emergency::manager::acceptor().empty());
	EXPECT_FALSE(is_port_open(port));

	// - installed twice : refused
	EXPECT_ANY_THROW(net::emergency::manager::install(net::emergency::manager::SETTING()));

	// 2) emergency : the acceptor is created and listens
	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::EMERGENCY);
	ASSERT_TRUE(wait_until([]() { auto p = net::emergency::manager::acceptor(); return p.exist() && p->is_listening(); }));
	auto pacceptor = net::emergency::manager::acceptor();
	EXPECT_EQ(pacceptor->executor_thread_priority(), eTHREAD_PRIORITY::TIME_CRITICAL);

	raw_client client;
	ASSERT_TRUE(client.connect(port));
	EXPECT_EQ(client.ask("where"), "own");
	EXPECT_EQ(client.ask("hello"), "hello");

	// - a second emergency signal : the same acceptor
	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::EMERGENCY);
	std::this_thread::sleep_for(200ms);
	EXPECT_EQ(net::emergency::manager::acceptor().get(), pacceptor.get());

	// 3) shutdown by the real signal (the handler of the manager) : fn_shutdown once
	std::raise(signal_shutdown);
	ASSERT_TRUE(wait_until([&]() { return count_shutdown == 1; }));
	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::SHUTDOWN);
	std::this_thread::sleep_for(200ms);
	EXPECT_EQ(count_shutdown.load(), 1);
	EXPECT_EQ(client.ask("after shutdown signal"), "after shutdown signal");

	// 4) terminate : every connection is closed, fn_terminate is called (the process is not ended : exit_process_on_terminate = false)
	net::emergency::manager::trigger(net::emergency::manager::eSIGNAL::TERMINATE);
	ASSERT_TRUE(wait_until([&]() { return count_terminate == 1; }));
	EXPECT_TRUE(client.wait_closed());

	// 5) uninstall : the acceptor is stopped, the signal handler given back
	pacceptor.reset();
	net::emergency::manager::uninstall();
	EXPECT_FALSE(net::emergency::manager::is_installed());
	EXPECT_TRUE(net::emergency::manager::acceptor().empty());
	EXPECT_FALSE(is_port_open(port));

	auto handler_after = std::signal(signal_shutdown, SIG_DFL);
	std::signal(signal_shutdown, handler_after);
	EXPECT_EQ(handler_after, handler_before);

	// 6) installed again after uninstall
	net::emergency::manager::SETTING setting_again;
	setting_again.signals_shutdown.clear();
	setting_again.signals_emergency.clear();
	setting_again.signals_terminate.clear();
	setting_again.exit_process_on_terminate = false;
	net::emergency::manager::install<socket_emergency_test>({ { net::ip::tcp::v6(), port } }, std::move(setting_again));
	EXPECT_TRUE(net::emergency::manager::is_installed());
	net::emergency::manager::uninstall();
}


//-----------------------------------------------------------------------------
// 5. system_executor destroy handlers (the call on destroy : examples/1.6.emergency_signal)
//-----------------------------------------------------------------------------
TEST(emergency, system_executor_destroy_handler_register)
{
	std::atomic<int> count_called{ 0 };

	EXPECT_FALSE(system_executor::register_destroy_handler("test.emergency.empty", system_executor::destroy_handler_t()));
	EXPECT_TRUE(system_executor::register_destroy_handler("test.emergency", [&]() { ++count_called; }));
	EXPECT_TRUE(system_executor::register_destroy_handler("test.emergency", [&]() { count_called += 10; }));	// - replaced
	EXPECT_TRUE(system_executor::unregister_destroy_handler("test.emergency"));
	EXPECT_FALSE(system_executor::unregister_destroy_handler("test.emergency"));
	EXPECT_FALSE(system_executor::unregister_destroy_handler("test.emergency.unknown"));
	EXPECT_EQ(count_called.load(), 0);
}


}
