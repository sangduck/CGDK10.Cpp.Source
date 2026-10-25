#include "pch.h"

#if defined(__linux__)
#include <atomic>
#include <thread>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <sys/resource.h>

// post path bench of io_context_threads (linux. disabled : --gtest_also_run_disabled_tests --gtest_filter=bench_post.*)
//  env : BP_THREADS (8) BP_POSTS (2000000) BP_HOPS (2000000) BP_SAMPLES (2000), CGDK_RUNNEXT=0 : runnext off (r11560)
//  A) external post throughput : P producer threads post a no-op executable
//  B) self post chain : K chains, each executable re-posts itself from the worker L times
//  C) idle wake latency : post to an idle executor -> executed
using namespace CGDK;

namespace
{
	using clk = std::chrono::steady_clock;

	int env_int(const char* _name, int _default) { auto p = getenv(_name); return p ? atoi(p) : _default; }

	struct ctx_switch { long vol; long invol; };
	ctx_switch get_ctx() { rusage ru{}; getrusage(RUSAGE_SELF, &ru); return { ru.ru_nvcsw, ru.ru_nivcsw }; }

	class bench_noop : public Iexecutable
	{
	public:
		std::atomic<int64_t>* pcount{ nullptr };
		virtual intptr_t process_execute(intptr_t, size_t) override { pcount->fetch_add(1, std::memory_order_relaxed); return 0; }
	};

	class bench_chain : public Iexecutable
	{
	public:
		Iexecutor*				pexecutor{ nullptr };
		int64_t					remaining{ 0 };
		std::atomic<int>*		pdone{ nullptr };
		virtual intptr_t process_execute(intptr_t, size_t) override
		{
			if (--remaining > 0)
				pexecutor->post(object_ptr<Iexecutable>(this));
			else
				pdone->fetch_add(1, std::memory_order_release);
			return 0;
		}
	};

	class bench_stamp : public Iexecutable
	{
	public:
		std::atomic<int64_t>	tick{ 0 };
		virtual intptr_t process_execute(intptr_t, size_t) override { tick.store(clk::now().time_since_epoch().count(), std::memory_order_release); return 0; }
	};

	const char* backend_name(executor::io_backend _b) { return _b == executor::io_backend::epoll ? "epoll" : _b == executor::io_backend::io_uring ? "io_uring" : "uring_cq"; }

	void run_backend(executor::io_backend _backend)
	{
		const int N = env_int("BP_THREADS", 8);
		executor::set_default_io_backend(_backend);
		auto pexecutor = executor::make_io_context_threads("bench post", N);
		executor::set_default_io_backend(executor::io_backend::epoll);
		const char* bname = backend_name(pexecutor->backend());
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		const bool only_c = env_int("BP_ONLY_C", 0) != 0;		// - BP_ONLY_C=1 : C only

		// A) external post throughput
		if (!only_c) for (int P : { 1, 4 })
		{
			const int64_t per = env_int("BP_POSTS", 2000000) / P;
			std::atomic<int64_t> count{ 0 };
			auto pnoop = make_object<bench_noop>();
			pnoop->pcount = &count;
			auto c0 = get_ctx();
			auto t0 = clk::now();
			std::vector<std::thread> th;
			for (int p = 0; p < P; ++p)
				th.emplace_back([&] { for (int64_t i = 0; i < per; ++i) pexecutor->post(object_ptr<Iexecutable>(pnoop.get())); });
			for (auto& t : th) t.join();
			while (count.load() < per * P) std::this_thread::yield();
			auto sec = std::chrono::duration<double>(clk::now() - t0).count();
			auto c1 = get_ctx();
			printf("[bench_post] %-8s A external post  P=%d N=%d : %8.2f M/s  (%.0f ns/post, ctxsw vol %.3f/post)\n", bname, P, N, per * P / sec / 1e6, sec * 1e9 / (per * P), double(c1.vol - c0.vol) / (per * P));
		}

		// B) self post chain
		if (!only_c) for (int K : { 1, N, 4 * N })
		{
			const int64_t L = env_int("BP_HOPS", 2000000) / K;
			std::atomic<int> done{ 0 };
			std::vector<object_ptr<bench_chain>> chains;
			for (int k = 0; k < K; ++k) { auto p = make_object<bench_chain>(); p->pexecutor = pexecutor.get(); p->remaining = L; p->pdone = &done; chains.push_back(std::move(p)); }
			auto c0 = get_ctx();
			auto t0 = clk::now();
			for (auto& p : chains) pexecutor->post(object_ptr<Iexecutable>(p.get()));
			while (done.load(std::memory_order_acquire) < K) std::this_thread::yield();
			auto sec = std::chrono::duration<double>(clk::now() - t0).count();
			auto c1 = get_ctx();
			printf("[bench_post] %-8s B self post chain K=%-3d N=%d : %8.2f M hops/s (%.0f ns/hop per chain, ctxsw vol %.3f/hop)\n", bname, K, N, K * L / sec / 1e6, sec * 1e9 / L, double(c1.vol - c0.vol) / (K * L));
		}

		// B0) the same chain run inline (no post) : the cost without the post path
		if (!only_c)
		{
			const int64_t L = env_int("BP_HOPS", 2000000);
			std::atomic<int> done{ 0 };
			auto p = make_object<bench_chain>(); p->pexecutor = nullptr; p->remaining = 1; p->pdone = &done;
			auto t0 = clk::now();
			for (int64_t i = 0; i < L; ++i) { p->remaining = 1; static_cast<Iexecutable*>(p.get())->process_execute(0, 0); }
			auto sec = std::chrono::duration<double>(clk::now() - t0).count();
			printf("[bench_post] %-8s B0 inline call (no post)          : %.1f ns/hop\n", bname, sec * 1e9 / L);
		}

		// C) idle wake latency
		{
			const int S = env_int("BP_SAMPLES", 2000);
			std::vector<double> lat;
			lat.reserve(S);
			auto pstamp = make_object<bench_stamp>();
			for (int s = 0; s < S; ++s)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(300));
				pstamp->tick.store(0);
				auto t0 = clk::now();
				pexecutor->post(object_ptr<Iexecutable>(pstamp.get()));
				int64_t t1;
				while ((t1 = pstamp->tick.load(std::memory_order_acquire)) == 0) {}
				lat.push_back(double(t1 - t0.time_since_epoch().count()) / 1000.0);
			}
			std::sort(lat.begin(), lat.end());
			printf("[bench_post] %-8s C idle wake latency N=%d : p50 %.1f us, p90 %.1f us, p99 %.1f us\n", bname, N, lat[S / 2], lat[S * 9 / 10], lat[S * 99 / 100]);
		}

		pexecutor->stop();
	}
}

TEST(bench_post, DISABLED_epoll)
{
	run_backend(executor::io_backend::epoll);
}

TEST(bench_post, DISABLED_io_uring)
{
	run_backend(executor::io_backend::io_uring);
}

TEST(bench_post, DISABLED_io_uring_completion)
{
	run_backend(executor::io_backend::io_uring_completion);
}

#endif

// executor::strand bench (windows and linux. disabled : --gtest_also_run_disabled_tests --gtest_filter=bench_strand.*)
//  env : BS_THREADS (4 : workers of the executor of the strands) BS_POSTS (2000000) BS_HOPS (2000000) BS_ROUNDS (3)
//  A) external posts : P producer threads post a no-op executable to S strands (round robin)
//  B) chains in one strand : K executables each re-post themselves to the strand L times (the strand is busy : queue, batch, swap)
//  C) queue_list_threads : P producer threads post a no-op executable
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdio>
#include <cstdlib>

namespace
{
	using bs_clk = std::chrono::steady_clock;

	int bs_env_int(const char* _name, int _default) { auto p = std::getenv(_name); return p ? std::atoi(p) : _default; }

	class bench_strand_noop : public CGDK::Iexecutable
	{
	public:
		std::atomic<int64_t>* pcount{ nullptr };
		virtual intptr_t process_execute(intptr_t, size_t) override { pcount->fetch_add(1, std::memory_order_relaxed); return 0; }
	};

	class bench_strand_chain : public CGDK::Iexecutable
	{
	public:
		CGDK::executor::strand*	pstrand{ nullptr };
		int64_t					remaining{ 0 };
		std::atomic<int>*		pdone{ nullptr };
		virtual intptr_t process_execute(intptr_t, size_t) override
		{
			if (--remaining > 0)
				pstrand->post(CGDK::object_ptr<CGDK::Iexecutable>(this));
			else
				pdone->fetch_add(1, std::memory_order_release);
			return 0;
		}
	};
}

TEST(bench_strand, DISABLED_post)
{
	using namespace CGDK;

	const int N = bs_env_int("BS_THREADS", 4);
	const int ROUNDS = bs_env_int("BS_ROUNDS", 3);
	auto pexecutor = executor::make_io_context_threads("bench strand", N);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	for (int round = 0; round < ROUNDS; ++round)
	{
		// A) external posts
		for (int S : { 1, 16 })
		for (int P : { 1, 4 })
		{
			std::vector<object_ptr<executor::strand>> strands;
			for (int s = 0; s < S; ++s) { auto p = make_object<executor::strand>(); p->executor(object_ptr<Iexecutor>(pexecutor.get())); p->start(); strands.push_back(std::move(p)); }

			const int64_t per = bs_env_int("BS_POSTS", 2000000) / P;
			std::atomic<int64_t> count{ 0 };
			std::vector<object_ptr<bench_strand_noop>> noops;
			for (int p = 0; p < P; ++p) { auto pn = make_object<bench_strand_noop>(); pn->pcount = &count; noops.push_back(std::move(pn)); }

			auto t0 = bs_clk::now();
			std::vector<std::thread> th;
			for (int p = 0; p < P; ++p)
				th.emplace_back([&, p] { auto* pn = noops[p].get(); for (int64_t i = 0; i < per; ++i) strands[(i + p) % S]->post(object_ptr<Iexecutable>(pn)); });
			for (auto& t : th) t.join();
			while (count.load() < per * P) std::this_thread::yield();
			auto sec = std::chrono::duration<double>(bs_clk::now() - t0).count();
			printf("[bench_strand] round %d A external post S=%-2d P=%d N=%d : %8.2f M/s (%.0f ns/post)\n", round, S, P, N, per * P / sec / 1e6, sec * 1e9 / (per * P));

			for (auto& iter : strands) iter->destroy();
		}

		// B) chains in one strand
		for (int K : { 1, 64 })
		{
			auto pstrand = make_object<executor::strand>();
			pstrand->executor(object_ptr<Iexecutor>(pexecutor.get()));
			pstrand->start();

			const int64_t L = bs_env_int("BS_HOPS", 2000000) / K;
			std::atomic<int> done{ 0 };
			std::vector<object_ptr<bench_strand_chain>> chains;
			for (int k = 0; k < K; ++k) { auto p = make_object<bench_strand_chain>(); p->pstrand = pstrand.get(); p->remaining = L; p->pdone = &done; chains.push_back(std::move(p)); }

			auto t0 = bs_clk::now();
			for (auto& p : chains) pstrand->post(object_ptr<Iexecutable>(p.get()));
			while (done.load(std::memory_order_acquire) < K) std::this_thread::yield();
			auto sec = std::chrono::duration<double>(bs_clk::now() - t0).count();
			printf("[bench_strand] round %d B chains in a strand K=%-2d N=%d : %8.2f M hops/s (%.0f ns/hop)\n", round, K, N, K * L / sec / 1e6, sec * 1e9 / (K * L));

			pstrand->destroy();
		}

		// C) queue_list_threads : P producer threads post a no-op executable
		for (int P : { 1, 4 })
		{
			auto pqueue = make_own<executor::queue_list_threads>();
			pqueue->start({ N });

			const int64_t per = bs_env_int("BS_POSTS", 2000000) / P;
			std::atomic<int64_t> count{ 0 };
			std::vector<object_ptr<bench_strand_noop>> noops;
			for (int p = 0; p < P; ++p) { auto pn = make_object<bench_strand_noop>(); pn->pcount = &count; noops.push_back(std::move(pn)); }

			auto t0 = bs_clk::now();
			std::vector<std::thread> th;
			for (int p = 0; p < P; ++p)
				th.emplace_back([&, p] { auto* pn = noops[p].get(); for (int64_t i = 0; i < per; ++i) pqueue->post(object_ptr<Iexecutable>(pn)); });
			for (auto& t : th) t.join();
			while (count.load() < per * P) std::this_thread::yield();
			auto sec = std::chrono::duration<double>(bs_clk::now() - t0).count();
			printf("[bench_strand] round %d C queue_list_threads post P=%d N=%d : %8.2f M/s (%.0f ns/post)\n", round, P, N, per * P / sec / 1e6, sec * 1e9 / (per * P));

			pqueue->stop();
		}
	}

	pexecutor->stop();
}
