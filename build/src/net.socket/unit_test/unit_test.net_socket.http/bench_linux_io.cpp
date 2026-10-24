#include "pch.h"
#include <algorithm>
#include <atomic>
#include <deque>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
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
//  - a "[bench-cpu]" line per case : the cpu of the measurement (the process, its threads, the machine with the softirq), the
//    sockets per thread (the thread that ran the last handler of a socket : its ring on io_uring completion), the handlers of a socket
//    in a row on a thread ("run" : about the messages of a receive) and the tcp counters per message (/proc/net : the whole network
//    namespace)
//  - CGDK_BENCH_CASES : runs the cases whose name has one of the comma separated parts only (CGDK_BENCH_CASES="tcp echo 64B,uds")
//  - CGDK_BENCH_ZEROCOPY_THRESHOLD : Itcp_gather::zerocopy_threshold() of the run (a large value : the 64KB messages are copied)
//  - the same file builds on the tree before these changes (the baseline of a comparison) : the io_uring part is left out there
//-----------------------------------------------------------------------------
#if defined(_SYS_EPOLL_H)
#include <dirent.h>
#include <sys/resource.h>
#include <unistd.h>

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

	bool lbn_case_enabled(const char* _case)
	{
		const char* penv = std::getenv("CGDK_BENCH_CASES");
		if (penv == nullptr || *penv == 0)
			return true;

		const std::string_view list(penv);
		const std::string_view name(_case);
		std::size_t begin = 0;
		while (begin <= list.size())
		{
			const auto end = std::min(list.find(',', begin), list.size());
			const auto token = list.substr(begin, end - begin);
			if (token.empty() == false && name.find(token) != std::string_view::npos)
				return true;
			begin = end + 1;
		}
		return false;
	}

	int lbn_tid()
	{
		thread_local const int t_tid = static_cast<int>(::gettid());
		return t_tid;
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

	// - runs : the handlers of one socket in a row on the thread (the messages of a receive : count / runs, a little more when the next
	//   receive of the socket comes next)
	struct alignas(64) lbn_thread_count
	{
		int						generation = 0;
		int						side = 0;
		std::atomic<std::size_t> count{ 0 };
		std::atomic<std::size_t> runs{ 0 };
	};

	std::mutex					g_lbn_lockable_threads;
	std::deque<lbn_thread_count> g_lbn_threads;
	std::atomic<int>			g_lbn_generation{ 0 };

	void lbn_count_thread(int _side, const void* _psocket)
	{
		thread_local int t_generation[2] = { -1, -1 };
		thread_local lbn_thread_count* t_pcount[2] = { nullptr, nullptr };
		thread_local const void* t_psocket_last[2] = { nullptr, nullptr };

		const int generation = g_lbn_generation.load(std::memory_order_relaxed);
		if (t_generation[_side] != generation)
		{
			std::lock_guard lock(g_lbn_lockable_threads);
			auto& entry = g_lbn_threads.emplace_back();
			entry.generation = generation;
			entry.side = _side;
			t_pcount[_side] = &entry;
			t_generation[_side] = generation;
			t_psocket_last[_side] = nullptr;
		}

		t_pcount[_side]->count.fetch_add(1, std::memory_order_relaxed);

		if (t_psocket_last[_side] != _psocket)
		{
			t_psocket_last[_side] = _psocket;
			t_pcount[_side]->runs.fetch_add(1, std::memory_order_relaxed);
		}
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

	// - the handlers per run of a side in this case
	double lbn_run_length(int _side)
	{
		std::lock_guard lock(g_lbn_lockable_threads);
		const int generation = g_lbn_generation.load();
		std::size_t total = 0, runs = 0;
		for (const auto& iter : g_lbn_threads)
		{
			if (iter.generation != generation || iter.side != _side)
				continue;
			total += iter.count.load();
			runs += iter.runs.load();
		}
		return (runs != 0) ? static_cast<double>(total) / static_cast<double>(runs) : 0.0;
	}

	// - the cpu at a time of a measurement : the process, its threads (tid -> utime + stime in clock ticks), the "cpu" line of /proc/stat
	//   (user nice system idle iowait irq softirq steal : clock ticks of the machine) and the counters "Tcp." / "TcpExt." of /proc/net
	struct lbn_cpu_snapshot
	{
		double					process_seconds = 0;
		long					context_switches = 0;
		std::map<int, long long> thread_ticks;
		std::vector<long long>	machine_ticks;
		std::map<std::string, long long> tcp;
	};

	void lbn_read_proc_counters(const char* _path, std::map<std::string, long long>& _counters)
	{
		// - pairs of lines "Tcp: <names>" / "Tcp: <values>"
		std::ifstream file(_path);
		std::string names, values;
		while (std::getline(file, names) && std::getline(file, values))
		{
			std::istringstream stream_names(names), stream_values(values);
			std::string prefix, prefix_values, name;
			stream_names >> prefix;
			stream_values >> prefix_values;
			if (prefix != prefix_values || (prefix != "Tcp:" && prefix != "TcpExt:"))
				continue;
			prefix.pop_back();
			long long value = 0;
			while (stream_names >> name && stream_values >> value)
				_counters[prefix + "." + name] = value;
		}
	}

	lbn_cpu_snapshot lbn_take_cpu_snapshot()
	{
		lbn_cpu_snapshot snapshot;

		rusage usage{};
		::getrusage(RUSAGE_SELF, &usage);
		snapshot.process_seconds = static_cast<double>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) + static_cast<double>(usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) * 1e-6;
		snapshot.context_switches = usage.ru_nvcsw + usage.ru_nivcsw;

		// - /proc/self/task/<tid>/stat : utime and stime are the fields 14 and 15 (the 12th and 13th after the "(comm)")
		if (auto pdir = ::opendir("/proc/self/task"); pdir != nullptr)
		{
			while (auto pentry = ::readdir(pdir))
			{
				const int tid = std::atoi(pentry->d_name);
				if (tid <= 0)
					continue;
				std::ifstream file(std::string("/proc/self/task/") + pentry->d_name + "/stat");
				std::string line;
				std::getline(file, line);
				const auto pos = line.rfind(')');
				if (pos == std::string::npos)
					continue;
				std::istringstream stream(line.substr(pos + 1));
				std::string field;
				long long utime = 0, stime = 0;
				for (int i = 0; i < 11 && stream >> field; ++i) {}
				stream >> utime >> stime;
				snapshot.thread_ticks[tid] = utime + stime;
			}
			::closedir(pdir);
		}

		{
			std::ifstream file("/proc/stat");
			std::string cpu;
			file >> cpu;
			long long value = 0;
			for (int i = 0; i < 8 && file >> value; ++i)
				snapshot.machine_ticks.push_back(value);
		}

		lbn_read_proc_counters("/proc/net/snmp", snapshot.tcp);
		lbn_read_proc_counters("/proc/net/netstat", snapshot.tcp);

		return snapshot;
	}

	// - "<threads> thr <the fewest>-<the most>" : the sockets per thread that ran their last handler
	std::string lbn_socket_summary(const std::vector<int>& _tids)
	{
		std::map<int, int> sockets;
		for (const int tid : _tids)
			if (tid != 0)
				++sockets[tid];
		int fewest = 0, most = 0;
		for (const auto& [tid, count] : sockets)
		{
			fewest = (fewest == 0) ? count : std::min(fewest, count);
			most = std::max(most, count);
		}
		char text[64];
		snprintf(text, sizeof(text), "%zu thr %d-%d", sockets.size(), fewest, most);
		return text;
	}

	void lbn_report_cpu(const lbn_cpu_snapshot& _begin, const lbn_cpu_snapshot& _end, std::size_t _count, double _seconds, const std::vector<int>& _tids_server, const std::vector<int>& _tids_client)
	{
		const double ticks_per_second = static_cast<double>(::sysconf(_SC_CLK_TCK));
		const double messages = static_cast<double>(std::max<std::size_t>(_count, 1));
		const double cpu_process = _end.process_seconds - _begin.process_seconds;

		// - the threads that used 10% of a core or more, the busiest one
		int count_busy = 0;
		double top = 0;
		for (const auto& [tid, ticks] : _end.thread_ticks)
		{
			const auto iter = _begin.thread_ticks.find(tid);
			const double share = static_cast<double>(ticks - ((iter != _begin.thread_ticks.end()) ? iter->second : 0)) / ticks_per_second / _seconds;
			if (share >= 0.1)
				++count_busy;
			top = std::max(top, share);
		}

		// - the machine : busy = all but idle and iowait
		auto machine = [&](std::size_t _index) { return (_index < _begin.machine_ticks.size() && _index < _end.machine_ticks.size()) ? static_cast<double>(_end.machine_ticks[_index] - _begin.machine_ticks[_index]) / ticks_per_second / _seconds : 0.0; };
		const double machine_busy = machine(0) + machine(1) + machine(2) + machine(5) + machine(6) + machine(7);

		auto tcp = [&](const char* _name) { const auto iter_end = _end.tcp.find(_name); const auto iter_begin = _begin.tcp.find(_name); return (iter_end != _end.tcp.end() && iter_begin != _begin.tcp.end()) ? static_cast<double>(iter_end->second - iter_begin->second) / messages : 0.0; };

		printf("[bench-cpu] process %.2f cores %.2f us/msg (ctx %.3f/msg), threads %d over 10%% top %.0f%%, machine %.2f cores (sys %.2f softirq %.2f) | sockets server %s, client %s | run server %.1f client %.1f msg | per msg tcp in %.3f out %.3f delayed ack %.4f\n",
			cpu_process / _seconds, cpu_process * 1e6 / messages, static_cast<double>(_end.context_switches - _begin.context_switches) / messages, count_busy, top * 100.0,
			machine_busy, machine(2), machine(6),
			lbn_socket_summary(_tids_server).c_str(), lbn_socket_summary(_tids_client).c_str(),
			lbn_run_length(LBN_SIDE_SERVER), lbn_run_length(LBN_SIDE_CLIENT),
			tcp("Tcp.InSegs"), tcp("Tcp.OutSegs"), tcp("TcpExt.DelayedACKs"));

		// - the other tcp counters that moved (per message : TcpExt.TCPOrigDataSent = the data segments, TcpExt.TCPPureAcks ...)
		std::string text_tcp;
		for (const auto& [name, value] : _end.tcp)
		{
			const double per_message = tcp(name.c_str());
			if (per_message >= 0.001 || per_message <= -0.001)
			{
				char text[96];
				snprintf(text, sizeof(text), " %s %.3f", name.c_str(), per_message);
				text_tcp += text;
			}
		}
		if (text_tcp.empty() == false)
			printf("[bench-tcp]%s\n", text_tcp.c_str());
		fflush(stdout);
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
class lbn_echo_server_socket;

namespace
{
	// - the server sockets connected in this case (the sockets per thread of the report : alive till the clients stop)
	std::mutex					g_lbn_lockable_servers;
	std::vector<lbn_echo_server_socket*> g_lbn_servers;
}

class lbn_echo_server_socket : public net::socket::tcp<>
{
public:
			std::atomic<int>	tid_last{ 0 };

protected:
	virtual void				on_connect() override
	{
		this->tid_last = 0;
		std::lock_guard lock(g_lbn_lockable_servers);
		g_lbn_servers.push_back(this);
	}
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(LBN_SIDE_SERVER, this);
		this->tid_last.store(lbn_tid(), std::memory_order_relaxed);
		this->send(_msg.buf_message);
		return eRESULT::DONE;
	}
};

namespace
{
	std::vector<int> lbn_server_tids()
	{
		std::lock_guard lock(g_lbn_lockable_servers);
		std::vector<int> tids;
		for (auto* psocket : g_lbn_servers)
			tids.push_back(psocket->tid_last.load());
		return tids;
	}

	template <class TSOCKETS>
	std::vector<int> lbn_client_tids(const TSOCKETS& _sockets)
	{
		std::vector<int> tids;
		for (auto& psocket : _sockets)
			tids.push_back(psocket->tid_last.load());
		return tids;
	}
}

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
			std::atomic<int>	tid_last{ 0 };

protected:
	virtual void				on_connect() override { this->is_connected = true; }
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override { this->is_connected = false; }
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(LBN_SIDE_CLIENT, this);
		this->tid_last.store(lbn_tid(), std::memory_order_relaxed);
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
			std::atomic<int>	tid_last{ 0 };

protected:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override
	{
		lbn_count_thread(this->echo ? LBN_SIDE_SERVER : LBN_SIDE_CLIENT, this);
		this->tid_last.store(lbn_tid(), std::memory_order_relaxed);
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
	if (lbn_case_enabled(_case) == false)
		return;

	lbn_uds_guard guard;
	net::io::connect_requestable::Ntcp::enable_uds_loopback(_uds);
	lbn_begin_case();
	{
		std::lock_guard lock(g_lbn_lockable_servers);
		g_lbn_servers.clear();
	}

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
	const auto cpu_begin = lbn_take_cpu_snapshot();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));

	std::size_t count = 0, bytes = 0;
	for (auto& psocket : clients)
	{
		count += psocket->count_received.load();
		bytes += psocket->bytes_received.load();
	}
	const auto cpu_end = lbn_take_cpu_snapshot();
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - time_begin).count();

	// 4) stop (the messages in flight drain)
	g_lbn_running = false;
	std::this_thread::sleep_for(300ms);

	lbn_report(_case, _backend, count, bytes, seconds);
	lbn_report_cpu(cpu_begin, cpu_end, count, seconds, lbn_server_tids(), lbn_client_tids(clients));

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
	if (lbn_case_enabled(_case) == false)
		return;

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
	const auto cpu_begin = lbn_take_cpu_snapshot();
	std::this_thread::sleep_for(std::chrono::seconds(_seconds));

	std::size_t count = 0, bytes = 0;
	for (auto& psocket : clients)
	{
		count += psocket->count_received.load();
		bytes += psocket->bytes_received.load();
	}
	const auto cpu_end = lbn_take_cpu_snapshot();
	const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - time_begin).count();

	g_lbn_running = false;
	std::this_thread::sleep_for(300ms);

	lbn_report(_case, _backend, count, bytes, seconds);
	lbn_report_cpu(cpu_begin, cpu_end, count, seconds, { pserver->tid_last.load() }, lbn_client_tids(clients));

	for (auto& psocket : clients)
		psocket->stop();
	pserver->stop();
}

//-----------------------------------------------------------------------------
// the cases on a backend
//-----------------------------------------------------------------------------
void lbn_run_all(const char* _backend, executor::Iio_context* _pio_context, int _seconds)
{
	// - CGDK_BENCH_ZEROCOPY_THRESHOLD : the size of a single buffer sent with zerocopy (io_uring : MSG_ZEROCOPY / SEND_ZC. 16KB by default)
	if (const char* penv = std::getenv("CGDK_BENCH_ZEROCOPY_THRESHOLD"); penv != nullptr && *penv != 0)
		net::io::sendable::Itcp_gather::zerocopy_threshold(static_cast<std::size_t>(std::strtoull(penv, nullptr, 10)));

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
// - the completion backend (a ring per worker thread). the sockets submit ops on it (ACCEPT, CONNECT, RECV multishot)
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
