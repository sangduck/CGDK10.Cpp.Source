//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                    benchmark - query redis (common)                       *
//*                                                                           *
//*  same workload for every client : options, phases, latency histogram,     *
//*  client cpu (getrusage) and one 'RESULT' line per run                     *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once
#include <atomic>
#include <cerrno>
#include <limits>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include <sys/resource.h>
#include <unistd.h>

namespace bench
{
//-----------------------------------------------------------------------------
// options (--name=value)
//   mode     : sync      callers blocking request -> response (threads, or coroutines/callback chains for async clients)
//              pipeline  each caller sends 'pipeline' commands then waits for all of them
//   op       : get | set
//   callers  : threads (sync/pipeline) or concurrent chains (async)
//   pipeline : commands per batch (pipeline mode)
//   size     : value bytes
//   keys     : key space (key:0 ... key:<keys-1>)
//   variant  : client specific (cgdk: shared | dedicated | async)
//   sessions : client specific connection count (0: default of the client)
//-----------------------------------------------------------------------------
struct options
{
	std::string	host = "127.0.0.1";
	int			port = 6390;
	std::string	mode = "sync";
	std::string	op = "get";
	std::string	variant;
	int			callers = 1;
	int			pipeline = 1;
	int			size = 16;
	int			keys = 10000;
	int			sessions = 0;
	double		seconds = 3.0;
	double		warmup = 0.5;
	bool		populate = false;	// only fills the keys (SET) and exits
	std::size_t request_send_window = 8;
	std::string resume_executor = "default";
	bool encode_on_session = true;
    bool post_response = true;
    std::string response_executor = "io";
    double callback_work_us = 0;
	bool post_latency = false;
	bool		request_latency = false; // CGDK pipeline: also record each command from submission to callback
	bool		enable_send_defer = true; // CGDK querier option; independent of the explicit --defer scope
	bool		defer = false;		// cgdk : send_defer::scope around a batch (pipeline)
};

inline options parse(int argc, char** argv)
{
	options o;
	for (int i = 1; i < argc; ++i)
	{
		std::string_view a(argv[i]);
		auto eq = a.find('=');
		if (a.substr(0, 2) != "--")
			continue;
		auto k = a.substr(2, (eq == std::string_view::npos) ? std::string_view::npos : eq - 2);
		std::string v = (eq == std::string_view::npos) ? std::string("1") : std::string(a.substr(eq + 1));

		if (k == "host") o.host = v;
		else if (k == "port") o.port = std::atoi(v.c_str());
		else if (k == "mode") o.mode = v;
		else if (k == "op") o.op = v;
		else if (k == "variant") o.variant = v;
		else if (k == "callers") o.callers = std::atoi(v.c_str());
		else if (k == "pipeline") o.pipeline = std::atoi(v.c_str());
		else if (k == "size") o.size = std::atoi(v.c_str());
		else if (k == "keys") o.keys = std::atoi(v.c_str());
		else if (k == "sessions") o.sessions = std::atoi(v.c_str());
		else if (k == "seconds") o.seconds = std::atof(v.c_str());
		else if (k == "warmup") o.warmup = std::atof(v.c_str());
		else if (k == "populate") o.populate = true;
		else if (k == "defer") o.defer = true;
		else if (k == "post-response") {
            bool enabled;
            if (v == "on" || v == "true" || v == "1") enabled = true;
            else if (v == "off" || v == "false" || v == "0") enabled = false;
            else { std::fprintf(stderr, "invalid boolean option\n"); std::exit(2); }
            o.post_response = enabled;
        }
        else if (k == "response-executor") {
            if (v != "io" && v != "system" && v != "private") { std::fprintf(stderr, "invalid response executor\n"); std::exit(2); }
            o.response_executor = v;
        }
        else if (k == "callback-work-us") { o.callback_work_us = std::atof(v.c_str()); if (o.callback_work_us < 0) std::exit(2); }
        else if (k == "post-latency") o.post_latency = true;
		else if (k == "encode-on-session") {
            if (v == "on" || v == "true" || v == "1") o.encode_on_session = true;
            else if (v == "off" || v == "false" || v == "0") o.encode_on_session = false;
            else { std::fprintf(stderr, "invalid --encode-on-session\n"); std::exit(2); }
        }
		else if (k == "request-latency") o.request_latency = true;
		else if (k == "request-window")
		{
			char* end = nullptr; errno = 0;
			const auto count = std::strtoull(v.c_str(), &end, 10);
			if (v.empty() || v.front() == '-' || *end || errno == ERANGE || count > (std::numeric_limits<std::size_t>::max)())
			{ std::fprintf(stderr, "invalid --request-window\n"); std::exit(2); }
			o.request_send_window = static_cast<std::size_t>(count);
		}
		else if (k == "resume-executor")
		{
			if (v != "default" && v != "system" && v != "private")
			{ std::fprintf(stderr, "invalid --resume-executor\n"); std::exit(2); }
			o.resume_executor = v;
		}
		else if (k == "send-defer")
		{
			if (v == "1" || v == "true" || v == "on") o.enable_send_defer = true;
			else if (v == "0" || v == "false" || v == "off") o.enable_send_defer = false;
			else { std::fprintf(stderr, "invalid --send-defer: %s\n", v.c_str()); std::exit(2); }
		}
		else { std::fprintf(stderr, "unknown option: %s\n", argv[i]); std::exit(2); }
	}
	// - large values : keep the key space under ~256MB
	const long long max_keys = (256LL << 20) / (o.size + 64);
	if (o.keys > max_keys) o.keys = static_cast<int>(max_keys < 1 ? 1 : max_keys);
	if (o.mode != "pipeline") o.pipeline = 1;
	return o;
}

inline std::string key_of(int i) { return "key:" + std::to_string(i); }
inline std::string value_of(int size) { return std::string(static_cast<std::size_t>(size), 'x'); }	// no blank (CGDK splits the command at blanks)

//-----------------------------------------------------------------------------
// latency histogram : log2 buckets with 64 sub buckets (~1.6% resolution)
//-----------------------------------------------------------------------------
class histogram
{
public:
	static constexpr int SUB = 64;
	histogram() : m_count(64 * SUB, 0) {}

	void record(uint64_t _ns) noexcept { ++m_count[index(_ns)]; ++m_total; }
	void merge(const histogram& _other) noexcept { for (std::size_t i = 0; i < m_count.size(); ++i) m_count[i] += _other.m_count[i]; m_total += _other.m_total; }
	uint64_t total() const noexcept { return m_total; }
	double percentile_us(double _p) const noexcept
	{
		if (m_total == 0) return 0.0;
		uint64_t target = static_cast<uint64_t>(_p * static_cast<double>(m_total));
		if (target >= m_total) target = m_total - 1;
		uint64_t acc = 0;
		for (std::size_t i = 0; i < m_count.size(); ++i)
		{
			acc += m_count[i];
			if (acc > target) return static_cast<double>(value(static_cast<int>(i))) / 1000.0;
		}
		return 0.0;
	}

private:
	static int index(uint64_t _v) noexcept
	{
		if (_v < SUB) return static_cast<int>(_v);
		int msb = 63 - __builtin_clzll(_v);
		int shift = msb - 6;
		return (msb - 5) * SUB + static_cast<int>((_v >> shift) & (SUB - 1));
	}
	static uint64_t value(int _i) noexcept
	{
		if (_i < SUB) return static_cast<uint64_t>(_i);
		int msb = _i / SUB + 5;
		int shift = msb - 6;
		return ((static_cast<uint64_t>(SUB) | static_cast<uint64_t>(_i % SUB)) << shift) + (1ULL << shift) / 2;
	}
	std::vector<uint64_t> m_count;
	uint64_t m_total = 0;
};

//-----------------------------------------------------------------------------
// run phases : warm up -> measure -> stop
//   a caller counts an operation only when it completes in the measure phase
//-----------------------------------------------------------------------------
enum : int { PHASE_WARMUP = 0, PHASE_MEASURE = 1, PHASE_STOP = 2 };
inline std::atomic<int> g_phase{ PHASE_WARMUP };
inline std::atomic<uint64_t> g_errors{ 0 };

using clock = std::chrono::steady_clock;
inline uint64_t now_ns() noexcept { return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(clock::now().time_since_epoch()).count()); }

// - per caller (one writer) : merged after the run
struct caller_stat
{
	histogram	latency;
	uint64_t	ops = 0;
	uint64_t total_ns = 0;

	// - '_ops' commands completed together (a batch) after '_ns'
	void complete(uint64_t _ns, uint64_t _ops = 1) noexcept
	{
		if (g_phase.load(std::memory_order_relaxed) != PHASE_MEASURE) return;
		latency.record(_ns);
		total_ns += _ns;
		ops += _ops;
	}
};

inline double cpu_seconds() noexcept
{
	rusage ru{};
	getrusage(RUSAGE_SELF, &ru);
	return static_cast<double>(ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + static_cast<double>(ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1e6;
}

// - cpu seconds of the redis server ('REDIS_PID' : same host. -1 : not set)
inline double server_cpu_seconds() noexcept
{
	const char* pid = std::getenv("REDIS_PID");
	if (pid == nullptr) return -1.0;
	char path[64];
	std::snprintf(path, sizeof(path), "/proc/%s/stat", pid);
	FILE* f = std::fopen(path, "r");
	if (f == nullptr) return -1.0;
	char buf[1024] = {};
	const auto n = std::fread(buf, 1, sizeof(buf) - 1, f);
	std::fclose(f);
	buf[n] = 0;
	const char* p = std::strrchr(buf, ')');	// fields after the command name : state(3) ... utime(14) stime(15)
	if (p == nullptr) return -1.0;
	unsigned long long ut = 0, st = 0;
	if (std::sscanf(p + 2, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu", &ut, &st) != 2) return -1.0;
	return static_cast<double>(ut + st) / static_cast<double>(sysconf(_SC_CLK_TCK));
}

// - runs the phases on the calling thread and prints the result.
//   '_stats' : one per caller, read after PHASE_STOP and '_join' returned
template <class FJOIN>
void run_and_report(const char* _client, const options& _o, std::vector<caller_stat>& _stats, FJOIN&& _join)
{
	std::this_thread::sleep_for(std::chrono::duration<double>(_o.warmup));
	rusage switches0{};
	getrusage(RUSAGE_SELF, &switches0);
	const double cpu0 = cpu_seconds();
	const double srv0 = server_cpu_seconds();
	const auto t0 = clock::now();
	g_phase.store(PHASE_MEASURE);
	std::this_thread::sleep_for(std::chrono::duration<double>(_o.seconds));
	g_phase.store(PHASE_STOP);
	const auto t1 = clock::now();
	const double cpu1 = cpu_seconds();
	rusage switches1{};
	getrusage(RUSAGE_SELF, &switches1);
	const double srv1 = server_cpu_seconds();
	_join();

	histogram h;
	uint64_t ops = 0, total_ns = 0;
	for (auto& s : _stats) { h.merge(s.latency); ops += s.ops; total_ns += s.total_ns; }

	const double sec = std::chrono::duration<double>(t1 - t0).count();
	const double cpu_us_per_op = (ops != 0) ? (cpu1 - cpu0) * 1e6 / static_cast<double>(ops) : 0.0;

	std::printf("RESULT client=%s variant=%s mode=%s op=%s size=%d callers=%d pipeline=%d ops=%llu ops_per_s=%.0f mean_us=%.1f p50_us=%.1f p95_us=%.1f p99_us=%.1f p999_us=%.1f cpu_us_per_op=%.3f cpu_cores=%.2f server_cores=%.2f errors=%llu voluntary_cs_per_kop=%.2f involuntary_cs_per_kop=%.2f\n",
		_client, _o.variant.empty() ? "-" : _o.variant.c_str(), _o.mode.c_str(), _o.op.c_str(), _o.size, _o.callers, _o.pipeline,
		static_cast<unsigned long long>(ops), static_cast<double>(ops) / sec,
		h.total() ? double(total_ns) / h.total() / 1000.0 : 0.0,
		h.percentile_us(0.50), h.percentile_us(0.95), h.percentile_us(0.99), h.percentile_us(0.999),
		cpu_us_per_op, (cpu1 - cpu0) / sec, (srv0 < 0.0) ? -1.0 : (srv1 - srv0) / sec, static_cast<unsigned long long>(g_errors.load()),
		ops ? (switches1.ru_nvcsw - switches0.ru_nvcsw) * 1000.0 / ops : 0.0,
		ops ? (switches1.ru_nivcsw - switches0.ru_nivcsw) * 1000.0 / ops : 0.0);
	std::fflush(stdout);
}

}
