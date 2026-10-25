// windows IOCP socket benchmarks (the cases of bench_linux_io.cpp + fan-in / fan-out sends from many threads)
//  usage : bench_windows_io <seconds> <threads> [case filter, comma separated]
//
//  - every case runs on an executor of its own (executor::make_io_context_threads, <threads> threads), the server sockets too
//  - echo : every client keeps 'window' messages in flight (a message received is sent again). msg/s = the messages the clients received
//  - fan-in / fan-out : 8 std::thread (out of the executor) send 64B to the server sockets (1 socket / 32 sockets in turn) as fast as
//    they can while the queue of the socket is under 64KB. msg/s = the messages the clients received ("sent" = the sends accepted)
//  - "us/msg" : the cpu time of the process / the messages
//  - build : build.bat <the root of a CGDK tree with its Release libs> <exe name>. A / B : ab.ps1 -exes <exe names> (alternate rounds, medians)
//  - 2026-10-24 r11530 (WSASend out of the queue lock) and r11531 (send_defer::auto_scope on windows) were measured with it
#include <string>
#include <vector>
#include <memory>
#include <span>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "cgdk/sdk10/net.socket.h"

using namespace CGDK;
using namespace std::chrono_literals;

namespace
{
	std::atomic<bool> g_running{ false };
	int g_threads = 8;
	std::string g_filter;

	bool case_enabled(const char* _case)
	{
		if (g_filter.empty())
			return true;
		std::string_view list(g_filter), name(_case);
		std::size_t begin = 0;
		while (begin <= list.size())
		{
			const auto end = (std::min)(list.find(',', begin), list.size());
			const auto token = list.substr(begin, end - begin);
			if (!token.empty() && name.find(token) != std::string_view::npos)
				return true;
			begin = end + 1;
		}
		return false;
	}

	template <class T>
	bool wait_until(T&& _cond, std::chrono::milliseconds _d = 10000ms)
	{
		const auto end = std::chrono::steady_clock::now() + _d;
		while (std::chrono::steady_clock::now() < end)
		{
			if (_cond()) return true;
			std::this_thread::sleep_for(10ms);
		}
		return _cond();
	}

	shared_buffer make_message(std::size_t _size)
	{
		auto buf = alloc_shared_buffer(_size);
		std::string message(_size, 'x');
		const uint32_t size = static_cast<uint32_t>(_size);
		memcpy(message.data(), &size, 4);
		buf.append(message.size(), message.data());
		return buf;
	}

	double process_cpu_seconds()
	{
		FILETIME c, e, k, u;
		GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u);
		auto to64 = [](FILETIME f) { return ((long long)f.dwHighDateTime << 32) | f.dwLowDateTime; };
		return (to64(k) + to64(u)) / 1e7;
	}

	void report(const char* _case, std::size_t _count, double _seconds, double _cpu)
	{
		printf("[bench] %-44s : %12.0f msg/s cpu %.2f core %.3f us/msg\n", _case, _count / _seconds, _cpu / _seconds, _cpu * 1e6 / (std::max<std::size_t>)(_count, 1));
		fflush(stdout);
	}
}

class echo_server_socket : public net::socket::tcp<>
{
public:
	bool echo = true;
protected:
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		if (echo)
			this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

std::mutex g_lock_servers;
std::vector<object_ptr<echo_server_socket>> g_servers;

class server_socket_registered : public echo_server_socket
{
protected:
	virtual void on_connect() override
	{
		std::lock_guard lock(g_lock_servers);
		g_servers.push_back(object_ptr<echo_server_socket>(this));
	}
};

class bench_acceptor : public net::acceptor<server_socket_registered>
{
protected:
	[[nodiscard]] virtual object_ptr<executor::Iio_context> process_get_connectable_io_context(net::io::Iconnectable*) override { return this->io_context(); }
};

class client_socket : public net::socket::tcp<>, public net::io::connect_requestable::tcp::Ndefault
{
public:
	bool echo = true;
	std::atomic<bool> is_connected{ false };
	std::atomic<std::size_t> count_received{ 0 };
protected:
	virtual void on_connect() override { this->is_connected = true; }
	virtual void on_disconnect(uint64_t) override { this->is_connected = false; }
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		count_received.fetch_add(1, std::memory_order_relaxed);
		if (echo && g_running.load(std::memory_order_relaxed))
			this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

class bench_udp_socket : public net::socket::udp
{
public:
	bool echo = false;
	std::atomic<std::size_t> count_received{ 0 };
protected:
	virtual result_code on_message(sMESSAGE_NETWORK& _msg) override
	{
		count_received.fetch_add(1, std::memory_order_relaxed);
		if (this->echo || g_running.load(std::memory_order_relaxed))
			this->send_to(_msg.buf_message, _msg.address);
		return eRESULT::DONE;
	}
};

struct tcp_setup
{
	own_ptr<bench_acceptor> acceptor;
	std::vector<own_ptr<client_socket>> clients;

	bool start(executor::Iio_context* _pio, int _count, bool _client_echo, bool _server_echo)
	{
		{
			std::lock_guard lock(g_lock_servers);
			g_servers.clear();
		}
		acceptor = make_own<bench_acceptor>();
		acceptor->io_context(_pio);
		net::acceptor<server_socket_registered>::START_PARAMETER parameter;
		parameter.local_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), static_cast<uint16_t>(34400 + GetCurrentProcessId() % 400) };
		parameter.reallocate_port_at_duplicate_port_error = true;
		parameter.enable_progress_log = false;
		if (acceptor->start(parameter) != eRESULT::SUCCESS) { printf("acceptor start failed\n"); return false; }
		const auto port = acceptor->local_endpoint().port();
		for (int i = 0; i < _count; ++i)
		{
			auto p = make_own<client_socket>();
			p->echo = _client_echo;
			p->io_context(_pio);
			sSOCKET_CONNECT_REQUEST_PARAMETER pc;
			pc.remote_endpoint = net::ip::tcp::endpoint{ net::ip::address_v4::loopback(), port };
			pc.enable_progress_log = false;
			p->start(pc);
			clients.push_back(std::move(p));
		}
		if (!wait_until([&] { return std::all_of(clients.begin(), clients.end(), [](auto& p) { return p->is_connected.load(); }); })) { printf("connect failed\n"); return false; }
		if (!wait_until([&] { std::lock_guard lock(g_lock_servers); return g_servers.size() == static_cast<std::size_t>(_count); })) { printf("accept failed\n"); return false; }
		std::lock_guard lock(g_lock_servers);
		for (auto& s : g_servers) s->echo = _server_echo;
		return true;
	}
	std::size_t received() const
	{
		std::size_t n = 0;
		for (auto& p : clients) n += p->count_received.load();
		return n;
	}
	void stop()
	{
		for (auto& p : clients) p->stop();
		wait_until([&] { return acceptor->count() == 0; });
		acceptor->stop();
		std::lock_guard lock(g_lock_servers);
		g_servers.clear();
	}
};

void tcp_echo(const char* _case, executor::Iio_context* _pio, int _count, std::size_t _size, int _window, int _seconds)
{
	if (!case_enabled(_case)) return;
	tcp_setup setup;
	if (!setup.start(_pio, _count, true, true)) return;
	g_running = true;
	const auto message = make_message(_size);
	for (auto& p : setup.clients)
		for (int w = 0; w < _window; ++w)
			p->send(message);
	std::this_thread::sleep_for(300ms);
	const auto n0 = setup.received(); const auto c0 = process_cpu_seconds(); const auto t0 = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));
	const auto n1 = setup.received(); const auto c1 = process_cpu_seconds(); const auto t1 = std::chrono::steady_clock::now();
	g_running = false;
	std::this_thread::sleep_for(300ms);
	report(_case, n1 - n0, std::chrono::duration<double>(t1 - t0).count(), c1 - c0);
	setup.stop();
}

// - many threads send to the same server sockets (_count sockets : the threads take the sockets in turn). the clients count
void tcp_fan(const char* _case, executor::Iio_context* _pio, int _count, int _senders, std::size_t _size, int _seconds)
{
	if (!case_enabled(_case)) return;
	tcp_setup setup;
	if (!setup.start(_pio, _count, false, false)) return;
	std::vector<object_ptr<echo_server_socket>> servers;
	{
		std::lock_guard lock(g_lock_servers);
		servers = g_servers;
	}
	g_running = true;
	const auto message = make_message(_size);
	std::atomic<std::size_t> sent{ 0 }, failed{ 0 };
	std::vector<std::thread> threads;
	for (int t = 0; t < _senders; ++t)
		threads.emplace_back([&, t]
		{
			std::size_t i = t, local_sent = 0, local_failed = 0;
			while (g_running.load(std::memory_order_relaxed))
			{
				auto& s = servers[i++ % servers.size()];
				// - keep the queue short : the receivers set the pace
				if (s->sending_queued_bytes_now() > 64 * 1024) { ++local_failed; std::this_thread::yield(); continue; }
				if (s->send(message)) ++local_sent; else ++local_failed;
			}
			sent += local_sent; failed += local_failed;
		});
	std::this_thread::sleep_for(300ms);
	const auto n0 = setup.received(); const auto c0 = process_cpu_seconds(); const auto t0 = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));
	const auto n1 = setup.received(); const auto c1 = process_cpu_seconds(); const auto t1 = std::chrono::steady_clock::now();
	g_running = false;
	for (auto& th : threads) th.join();
	std::this_thread::sleep_for(300ms);
	report(_case, n1 - n0, std::chrono::duration<double>(t1 - t0).count(), c1 - c0);
	printf("        sent %zu failed %zu received total %zu\n", sent.load(), failed.load(), setup.received());
	servers.clear();
	setup.stop();
}

void udp_echo(const char* _case, executor::Iio_context* _pio, int _count, std::size_t _size, int _window, int _seconds)
{
	if (!case_enabled(_case)) return;
	auto pserver = make_own<bench_udp_socket>();
	pserver->echo = true;
	pserver->io_context(_pio);
	if (pserver->start({ { net::ip::address_v4::loopback(), 0 } }) != eRESULT::SUCCESS) { printf("udp start failed\n"); return; }
	const net::sockaddr address_server{ net::ip::address_v4::loopback(), pserver->local_endpoint().port() };
	std::vector<own_ptr<bench_udp_socket>> clients;
	for (int i = 0; i < _count; ++i)
	{
		auto p = make_own<bench_udp_socket>();
		p->io_context(_pio);
		p->start({ { net::ip::address_v4::loopback(), 0 } });
		clients.push_back(std::move(p));
	}
	g_running = true;
	const auto message = make_message(_size);
	for (auto& p : clients)
		for (int w = 0; w < _window; ++w)
			p->send_to(message, address_server);
	std::this_thread::sleep_for(300ms);
	auto received = [&] { std::size_t n = 0; for (auto& p : clients) n += p->count_received.load(); return n; };
	const auto n0 = received(); const auto c0 = process_cpu_seconds(); const auto t0 = std::chrono::steady_clock::now();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));
	const auto n1 = received(); const auto c1 = process_cpu_seconds(); const auto t1 = std::chrono::steady_clock::now();
	g_running = false;
	std::this_thread::sleep_for(300ms);
	report(_case, n1 - n0, std::chrono::duration<double>(t1 - t0).count(), c1 - c0);
	for (auto& p : clients) p->stop();
	pserver->stop();
}

int main(int argc, char** argv)
{
	const int seconds = (argc > 1) ? atoi(argv[1]) : 3;
	g_threads = (argc > 2) ? atoi(argv[2]) : 8;
	if (argc > 3) g_filter = argv[3];

	auto pexecutor = executor::make_io_context_threads("bench iocp", g_threads);
	std::this_thread::sleep_for(300ms);

	tcp_echo("tcp echo 64B x32 clients window16", pexecutor.get(), 32, 64, 16, seconds);
	tcp_echo("tcp echo 64KB x4 clients window2", pexecutor.get(), 4, 64 * 1024, 2, seconds);
	tcp_echo("tcp echo 16B x256 clients window1", pexecutor.get(), 256, 16, 1, seconds);
	udp_echo("udp echo 64B x16 sockets window32", pexecutor.get(), 16, 64, 32, seconds);
	tcp_fan("tcp fan-in 64B 8 senders -> 1 socket", pexecutor.get(), 1, 8, 64, seconds);
	tcp_fan("tcp fan-out 64B 8 senders -> 32 sockets", pexecutor.get(), 32, 8, 64, seconds);

	pexecutor->stop();
	return 0;
}
