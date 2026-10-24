#include "pch.h"
#include <algorithm>
#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <cstdlib>

//-----------------------------------------------------------------------------
// linux socket benchmarks (echo ping-pong on an executor of their own)
//
//  - skipped unless the env CGDK_BENCH is set (the seconds of a case : CGDK_BENCH=3)
//  - every client keeps 'window' messages in flight : a message received is sent again. the count of the messages
//    the clients receive in the time is the throughput.
//  - the backends : epoll, and io_uring when this tree has it (executor::set_default_io_backend) and the kernel supports it
//  - the server sockets run on the executor of the case like the clients (lbn_acceptor. before 2026-10-24 they ran on the system
//    executor : the backend of a case was the backend of its clients only)
//  - the threads that ran the handlers of the server and of the clients are counted per case ("server 8 thr top 13%" : the
//    handlers ran on 8 threads, the busiest one ran 13% of them. io_uring completion runs a socket on the owner of its ring)
//  - the same file builds on the tree before these changes (the baseline of a comparison) : the io_uring part is left out there
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)

#if __has_include("cgdk/sdk10/system/object/api.io_context.uring.h")
	#define LIO_BENCH_HAS_URING 1
#else
	#define LIO_BENCH_HAS_URING 0
#endif

#if __has_include("cgdk/sdk10/system/object/api.io_context.uring_cq.h")
	#define LIO_BENCH_HAS_URING_CQ 1
#else
	#define LIO_BENCH_HAS_URING_CQ 0
#endif

namespace CGDK
{
using namespace std::chrono_literals;

namespace
{
	std::atomic<bool>	g_lbn_running{ false };

	template <class TCONDITION>
	bool lbn_wait_until(TCONDITION&& _condition, std::chrono::milliseconds _duration = 5000ms)
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

	uint16_t lbn_port()
	{
		return static_cast<uint16_t>(33400 + api::get_process_id() % 500);
	}

	int lbn_seconds()
	{
		const char* penv = std::getenv("CGDK_BENCH");
		if (penv == nullptr || *penv == 0)
			return 0;
		const int seconds = std::atoi(penv);
		return (seconds > 0) ? seconds : 3;
	}

	int lbn_threads()
	{
		const char* penv = std::getenv("CGDK_BENCH_THREADS");
		const int threads = (penv != nullptr) ? std::atoi(penv) : 0;
		return (threads > 0) ? threads : 4;
	}

	shared_buffer lbn_message(std::size_t _size)
	{
		auto buf = alloc_shared_buffer(_size);
		std::string message(_size, 'x');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		buf.append(message.size(), message.data());
		return buf;
	}

	struct lbn_uds_guard
	{
		bool previous = net::io::connect_requestable::Ntcp::is_enable_uds_loopback();
		~lbn_uds_guard() { net::io::connect_requestable::Ntcp::enable_uds_loopback(previous); }
	};

	// - the handlers run per thread in a case (LBN_SIDE_SERVER / LBN_SIDE_CLIENT). an entry per thread and side (never removed : a late
	//   handler of the case before keeps its entry. a cache line each : the threads count without sharing)
	constexpr int				LBN_SIDE_SERVER = 0;
	constexpr int				LBN_SIDE_CLIENT = 1;

	struct alignas(64) lbn_thread_count
	{
		int						generation = 0;
		int						side = 0;
		std::atomic<std::size_t> count{ 0 };
	};

	std::mutex					g_lbn_lockable_threads;
	std::deque<lbn_thread_count> g_lbn_threads;
	std::atomic<int>			g_lbn_generation{ 0 };

	void lbn_count_thread(int _side)
	{
		thread_local int t_generation[2] = { -1, -1 };
		thread_local lbn_thread_count* t_pcount[2] = { nullptr, nullptr };

		const int generation = g_lbn_generation.load(std::memory_order_relaxed);
		if (t_generation[_side] != generation)
		{
			std::lock_guard lock(g_lbn_lockable_threads);
			auto& entry = g_lbn_threads.emplace_back();
			entry.generation = generation;
			entry.side = _side;
			t_pcount[_side] = &entry;
			t_generation[_side] = generation;
		}

		t_pcount[_side]->count.fetch_add(1, std::memory_order_relaxed);
	}

	// - a new case : the threads are counted again
	void lbn_begin_case()
	{
		std::lock_guard lock(g_lbn_lockable_threads);
		++g_lbn_generation;
	}

	// - "<threads> thr top <the share of the busiest thread>%" of a side in this case
	std::string lbn_thread_summary(int _side)
	{
		std::lock_guard lock(g_lbn_lockable_threads);
		const int generation = g_lbn_generation.load();
		std::size_t count_thread = 0, total = 0, top = 0;
		for (const auto& iter : g_lbn_threads)
		{
			if (iter.generation != generation || iter.side != _side)
				continue;
			const auto count = iter.count.load();
			++count_thread;
			total += count;
			top = std::max(top, count);
		}
		char text[64];
		snprintf(text, sizeof(text), "%zu thr top %.0f%%", count_thread, (total != 0) ? 100.0 * static_cast<double>(top) / static_cast<double>(total) : 0.0);
		return text;
	}

	void lbn_report(const char* _case, const char* _backend, std::size_t _count, std::size_t _bytes, double _seconds)
	{
		const double per_second = (_seconds > 0) ? static_cast<double>(_count) / _seconds : 0;
		const double mb_per_second = (_seconds > 0) ? static_cast<double>(_bytes) / _seconds / (1024.0 * 1024.0) : 0;
		printf("[bench] %-40s %-8s : %12.0f msg/s, %10.1f MB/s (%zu messages in %.2fs) server %s, client %s\n", _case, _backend, per_second, mb_per_second, _count, _seconds, lbn_thread_summary(LBN_SIDE_SERVER).c_str(), lbn_thread_summary(LBN_SIDE_CLIENT).c_str());
		fflush(stdout);
	}
}

//-----------------------------------------------------------------------------
// sockets
//-----------------------------------------------------------------------------
class lbn_echo_server_socket : public net::socket::tcp<>
{
protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(LBN_SIDE_SERVER);
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

// - the accepted sockets run on the io_context of the acceptor (the executor of the case). the linux acceptor gives an accepted socket
//   the system executor when it has no io_context of its own : the server sockets of the bench ran there (epoll unless the process
//   runs with CGDK_IO_URING, 2 x cores + 2 threads) whatever backend the case was
class lbn_acceptor : public net::acceptor<lbn_echo_server_socket>
{
protected:
	[[nodiscard]] virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable* /*_pconnectable*/) override { return this->io_context(); }
};

class lbn_client_socket : public net::socket::tcp<>, public net::io::connect_requestable::tcp::Ndefault
{
public:
			std::atomic<bool>	is_connected{ false };
			std::atomic<std::size_t> count_received{ 0 };
			std::atomic<std::size_t> bytes_received{ 0 };

protected:
	virtual void				on_connect() override { this->is_connected = true; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(LBN_SIDE_CLIENT);
		++this->count_received;
		this->bytes_received += _msg.buf_message.size();

		// - ping-pong : sent again while running
		if (g_lbn_running.load(std::memory_order_relaxed))
			this->send(_msg.buf_message);

		return eRESULT::DONE;
	}
};

class lbn_udp_socket : public net::socket::udp
{
public:
			bool				echo = false;
			std::atomic<std::size_t> count_received{ 0 };
			std::atomic<std::size_t> bytes_received{ 0 };

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(this->echo ? LBN_SIDE_SERVER : LBN_SIDE_CLIENT);
		++this->count_received;
		this->bytes_received += _msg.buf_message.size();

		if (this->echo || g_lbn_running.load(std::memory_order_relaxed))
			this->send_to(_msg.buf_message, _msg.address);

		return eRESULT::DONE;
	}
};

//-----------------------------------------------------------------------------
// TCP / UDS echo
//-----------------------------------------------------------------------------
void lbn_tcp_echo(const char* _case, const char* _backend, executor::Iio_context* _pio_context, bool _uds, int _count_client, std::size_t _size, int _window, int _seconds)
{
	lbn_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(_uds);
	lbn_begin_case();

	// 1) server
	auto pacceptor = make_own<lbn_acceptor>();
	pacceptor->io_context(_pio_context);

	net::acceptor<lbn_echo_server_socket>::START_PARAMETER parameter;
	parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), lbn_port() };
	parameter.reallocate_port_at_duplicate_port_error = true;
	parameter.enable_progress_log = false;
	parameter.enable_uds = _uds;
	ASSERT_EQ(pacceptor->start(parameter), eRESULT::SUCCESS);
	const auto port = pacceptor->local_endpoint().port();

	// 2) clients
	std::vector<own_ptr<lbn_client_socket>> clients;
	for (int i = 0; i < _count_client; ++i)
	{
		auto psocket = make_own<lbn_client_socket>();
		psocket->io_context(_pio_context);

		sSOCKET_CONNECT_REQUEST_PARAMETER parameter_connect;
		parameter_connect.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), port };
		parameter_connect.enable_progress_log = false;
		psocket->start(parameter_connect);
		clients.push_back(std::move(psocket));
	}
	ASSERT_TRUE(lbn_wait_until([&]() { return std::all_of(clients.begin(), clients.end(), [](auto& _p) { return _p->is_connected.load(); }); }, 10000ms)) << "connect";

	// 3) run : 'window' messages in flight per client
	g_lbn_running = true;

	const auto message = lbn_message(_size);
	for (auto& psocket : clients)
		for (int w = 0; w < _window; ++w)
			ASSERT_TRUE(psocket->send(message));

	const auto time_begin = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));

	std::size_t count = 0, bytes = 0;
	for (auto& psocket : clients)
	{
		count += psocket->count_received.load();
		bytes += psocket->bytes_received.load();
	}
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - time_begin).count();

	// 4) stop (the messages in flight drain)
	g_lbn_running = false;
	std::this_thread::sleep_for(300ms);

	lbn_report(_case, _backend, count, bytes, seconds);

	for (auto& psocket : clients)
		psocket->stop();
	lbn_wait_until([&]() { return pacceptor->count() == 0; }, 10000ms);
	pacceptor->stop();
}

//-----------------------------------------------------------------------------
// UDP echo
//-----------------------------------------------------------------------------
void lbn_udp_echo(const char* _case, const char* _backend, executor::Iio_context* _pio_context, int _count_client, std::size_t _size, int _window, int _seconds)
{
	lbn_begin_case();

	// 1) server
	auto pserver = make_own<lbn_udp_socket>();
	pserver->echo = true;
	pserver->io_context(_pio_context);
	ASSERT_EQ(pserver->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);
	const net::sockaddr address_server{ net::ip::address_v4::loopback(), pserver->local_endpoint().port() };

	// 2) clients
	std::vector<own_ptr<lbn_udp_socket>> clients;
	for (int i = 0; i < _count_client; ++i)
	{
		auto psocket = make_own<lbn_udp_socket>();
		psocket->io_context(_pio_context);
		ASSERT_EQ(psocket->start({ { net::ip::address_v4::loopback(), 0 } }), eRESULT::SUCCESS);
		clients.push_back(std::move(psocket));
	}

	// 3) run
	g_lbn_running = true;

	const auto message = lbn_message(_size);
	for (auto& psocket : clients)
		for (int w = 0; w < _window; ++w)
			ASSERT_TRUE(psocket->send_to(message, address_server));

	const auto time_begin = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));

	std::size_t count = 0, bytes = 0;
	for (auto& psocket : clients)
	{
		count += psocket->count_received.load();
		bytes += psocket->bytes_received.load();
	}
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - time_begin).count();

	g_lbn_running = false;
	std::this_thread::sleep_for(300ms);

	lbn_report(_case, _backend, count, bytes, seconds);

	for (auto& psocket : clients)
		psocket->stop();
	pserver->stop();
}

//-----------------------------------------------------------------------------
// the cases on a backend
//-----------------------------------------------------------------------------
void lbn_run_all(const char* _backend, executor::Iio_context* _pio_context, int _seconds)
{
	// - the workers of the executor started (completion : the rings bound. the sockets accepted before are spread over the rings bound then)
	std::this_thread::sleep_for(300ms);

	lbn_tcp_echo("tcp echo 64B x32 clients window16", _backend, _pio_context, false, 32, 64, 16, _seconds);
	lbn_tcp_echo("tcp echo 64KB x4 clients window2", _backend, _pio_context, false, 4, 64 * 1024, 2, _seconds);
	lbn_tcp_echo("uds echo 64B x32 clients window16", _backend, _pio_context, true, 32, 64, 16, _seconds);
	lbn_tcp_echo("tcp echo 16B x256 clients window1", _backend, _pio_context, false, 256, 16, 1, _seconds);
	lbn_udp_echo("udp echo 64B x16 sockets window32", _backend, _pio_context, 16, 64, 32, _seconds);
}

TEST(bench, epoll)
{
	const int seconds = lbn_seconds();
	if (seconds == 0)
		GTEST_SKIP() << "set CGDK_BENCH=<seconds> to run";

	auto pexecutor = executor::make_io_context_threads("bench epoll", lbn_threads());
	ASSERT_TRUE(pexecutor.exist());

	lbn_run_all("epoll", pexecutor.get(), seconds);

	pexecutor->stop();
}

#if LIO_BENCH_HAS_URING
TEST(bench, io_uring)
{
	const int seconds = lbn_seconds();
	if (seconds == 0)
		GTEST_SKIP() << "set CGDK_BENCH=<seconds> to run";
	if (executor::is_io_uring_available() == false)
		GTEST_SKIP() << "io_uring is not available on this kernel";

	executor::set_default_io_backend(executor::io_backend::io_uring);
	auto pexecutor = executor::make_io_context_threads("bench io_uring", lbn_threads());
	executor::set_default_io_backend(executor::io_backend::epoll);
	ASSERT_TRUE(pexecutor.exist());

	lbn_run_all("io_uring", pexecutor.get(), seconds);

	pexecutor->stop();
}
#endif

#if LIO_BENCH_HAS_URING_CQ
// - the completion backend (a ring per worker thread). the sockets still use the readiness interface (polls) on it
TEST(bench, io_uring_completion)
{
	const int seconds = lbn_seconds();
	if (seconds == 0)
		GTEST_SKIP() << "set CGDK_BENCH=<seconds> to run";
	if (executor::is_io_uring_completion_available() == false)
		GTEST_SKIP() << "the io_uring completion backend is not available on this kernel";

	executor::set_default_io_backend(executor::io_backend::io_uring_completion);
	auto pexecutor = executor::make_io_context_threads("bench io_uring_completion", lbn_threads());
	executor::set_default_io_backend(executor::io_backend::epoll);
	ASSERT_TRUE(pexecutor.exist());
	ASSERT_EQ(pexecutor->backend(), executor::io_backend::io_uring_completion);

	lbn_run_all("uring_cq", pexecutor.get(), seconds);

	pexecutor->stop();
}
#endif

}

#endif
