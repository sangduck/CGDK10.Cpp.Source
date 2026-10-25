//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                  benchmark - query redis (CGDK10 query::redis)            *
//*                                                                           *
//*  variant (--variant=)                                                     *
//*   shared    : querier->execute_query() / post_execute_query()        *
//*               (shared queries use a session selected per caller thread)   *
//*   dedicated : alloc_query() per caller thread (a session of its own)      *
//*   async     : sync mode only. 'callers' chains of post_execute_query()    *
//*               (the next one is posted in the response callback)           *
//*   coroutine_blocking : co_execute_query() blocks the coroutine worker
//*   coroutine : sequential co_await execute_query(), Redis I/O by default        *
//*   coroutine_separate : resume on a separate, explicitly sized executor    *
//*   coroutine_io : same requests, resume on the Redis I/O executor          *
//*   coroutine_dedicated_io : reuse a dedicated query on that executor       *
//*  defer      : pipeline. send_defer::scope around the posts of a batch      *
//*  sessions   : count_session of the querier (= its io threads)             *
//*               default : callers (dedicated), 4 (others)                   *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#include "bench_common.h"
#include <optional>
#include "cgdk/sdk10/server.query.redis.h"

using namespace CGDK;
using namespace bench;

static options o;
static std::vector<caller_stat> g_request_stats;
static std::vector<caller_stat> g_post_stats;
static std::vector<std::string> g_commands;	// "get key:N" / "set key:N xxx..."
static void callback_work()
{
    if (o.callback_work_us <= 0) return;
    const auto until = now_ns() + static_cast<uint64_t>(o.callback_work_us * 1000);
    while (now_ns() < until) std::atomic_signal_fence(std::memory_order_seq_cst);
}


static bool check_result(const resp_data& _r)
{
	if (o.op == "get")
		return _r.type == eRESP_DATA_TYPE::BULK_STRING_ && _r.str_value.size() == static_cast<std::size_t>(o.size);
	return _r.type == eRESP_DATA_TYPE::SIMPLE_STRING_ && (o.op != "status" || _r.str_value.size() == static_cast<std::size_t>(o.size));
}

// - pipeline : waits until '_n' responses arrived
struct batch_wait
{
	std::atomic<int> done{ 0 };
	void arrive() noexcept { done.fetch_add(1, std::memory_order_acq_rel); done.notify_one(); }
	void wait(int _n) noexcept { for (int v = done.load(std::memory_order_acquire); v < _n; v = done.load(std::memory_order_acquire)) done.wait(v); done.store(0, std::memory_order_relaxed); }
};

static void caller_sync_shared(query::redis::querier* _pquerier, caller_stat& _stat, int _index)
{
	uint64_t n = static_cast<uint64_t>(_index) * 7919;
	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const auto& cmd = g_commands[(n++) % g_commands.size()];
		const uint64_t t = now_ns();
		try
		{
			auto pquery = _pquerier->execute_query(cmd);
			if (!check_result(pquery->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
		}
		catch (...) { g_errors.fetch_add(1, std::memory_order_relaxed); }
		_stat.complete(now_ns() - t);
	}
}

static void caller_sync_dedicated(query::redis::querier* _pquerier, caller_stat& _stat, int _index)
{
	uint64_t n = static_cast<uint64_t>(_index) * 7919;
	auto pquery = _pquerier->alloc_query();
	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const auto& cmd = g_commands[(n++) % g_commands.size()];
		const uint64_t t = now_ns();
		try
		{
			pquery->execute_query(cmd);
			if (!check_result(pquery->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
		}
		catch (...) { g_errors.fetch_add(1, std::memory_order_relaxed); }
		pquery->close_query();
		_stat.complete(now_ns() - t);
	}
}

static void caller_pipeline(query::redis::querier* _pquerier, caller_stat& _stat, int _index, bool _is_dedicated)
{
	uint64_t n = static_cast<uint64_t>(_index) * 7919;
	batch_wait waiter;
	struct request_samples
	{
		caller_stat* stat;
		batch_wait* waiter;
		std::vector<uint64_t> started;
	} samples{ &g_request_stats[_index], &waiter, std::vector<uint64_t>(o.request_latency ? o.pipeline : 0) };
	object_ptr<query::redis::query> pdedicated;
	if (_is_dedicated)
		pdedicated = _pquerier->alloc_query();

	auto on_response = [&waiter](query::redis::query* _pquery)
		{
            callback_work();
			if (!check_result(_pquery->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
			waiter.arrive();
		};

	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const uint64_t t = now_ns();
		int posted = 0;
		std::optional<send_defer::scope> defer;
		if (o.defer) defer.emplace();
		for (int i = 0; i < o.pipeline; ++i)
		{
			const auto& cmd = g_commands[(n++) % g_commands.size()];
			try
			{
				auto submit = [&](auto response) {
                    const auto started = o.post_latency ? now_ns() : 0;
					if (_is_dedicated)
					{
						// - a query object per command on the dedicated session
						auto pquery = make_object<query::redis::query>();
						pquery->set_session(pdedicated->get_session());
						pquery->post_execute_query(cmd, [pquery, response]() { response(pquery.get()); });
					}
					else
					{
						_pquerier->post_execute_query(cmd, std::function<void(query::redis::query*)>(response));
					}
                    if (o.post_latency) g_post_stats[_index].complete(now_ns() - started);
				};
				if (o.request_latency)
				{
					samples.started[i] = now_ns();
					submit([&samples, i](query::redis::query* query) {
                        callback_work();
						samples.stat->complete(now_ns() - samples.started[i]);
						if (!check_result(query->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
						samples.waiter->arrive();
					});
				}
				else submit(on_response);
				++posted;
			}
			catch (...) { g_errors.fetch_add(1, std::memory_order_relaxed); }
		}
		defer.reset();
		waiter.wait(posted);
		_stat.complete(now_ns() - t, static_cast<uint64_t>(o.pipeline));
	}
}

static void caller_batch(query::redis::querier* querier, caller_stat& stat, int index, bool synchronous)
{
	uint64_t n = static_cast<uint64_t>(index) * 7919;
	batch_wait waiter;
	std::vector<std::string_view> commands(o.pipeline);
	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const auto start = now_ns();
		for (auto& command : commands) command = g_commands[(n++) % g_commands.size()];
		try
		{
			if (synchronous)
			{
				auto query = querier->execute_query(commands);
				for (const auto& result : query->result().vector_child)
					if (!check_result(result)) g_errors.fetch_add(1, std::memory_order_relaxed);
			}
			else
			{
				querier->post_execute_query(commands, [&waiter](auto&& results) {
                    callback_work();
					for (const auto& result : results)
						if (!check_result(result)) g_errors.fetch_add(1, std::memory_order_relaxed);
					waiter.arrive();
				});
				waiter.wait(1);
			}
		}
		catch (...) { g_errors.fetch_add(1, std::memory_order_relaxed); }
		stat.complete(now_ns() - start, static_cast<uint64_t>(o.pipeline));
	}
}

// - async : a chain posts the next command from the response callback
struct async_chain
{
	query::redis::querier*	pquerier = nullptr;
	caller_stat*			pstat = nullptr;
	uint64_t				n = 0;
	uint64_t				t = 0;
	std::atomic<int>*		premaining = nullptr;

	void post()
	{
		if (g_phase.load(std::memory_order_relaxed) == PHASE_STOP)
		{
			if (premaining->fetch_sub(1) == 1) premaining->notify_all();
			return;
		}
		const auto& cmd = g_commands[(n++) % g_commands.size()];
		t = now_ns();
		try
		{
			pquerier->post_execute_query(cmd, [this](query::redis::query* _pquery)
				{
                    callback_work();
					if (!check_result(_pquery->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
					pstat->complete(now_ns() - t);
					post();
				});
		}
		catch (...)
		{
			g_errors.fetch_add(1, std::memory_order_relaxed);
			if (premaining->fetch_sub(1) == 1) premaining->notify_all();
		}
	}
};

// Each coroutine has exactly one command in flight. Concurrency is the number
// of chains, not a batch. Time includes encoding, I/O and continuation dispatch.
static coroutine::task<void> caller_coroutine(query::redis::querier* querier,
	caller_stat& stat, int index, bool dedicated)
{
	uint64_t n = static_cast<uint64_t>(index) * 7919;
	object_ptr<query::redis::query> query;
	if (dedicated) query = co_await querier->co_alloc_query();
	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const auto& command = g_commands[(n++) % g_commands.size()];
		const auto start = now_ns();
		try
		{
			if (o.variant == "coroutine_blocking")
			{
				auto response = co_await querier->co_execute_query<CGDK::query::redis::query>(command);
				if (!check_result(response->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
			}
			else if (dedicated)
			{
				co_await query->execute_query(command);
				if (!check_result(query->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
				query->close_query();
			}
			else
			{
				auto response = co_await querier->execute_query(command);
				if (!check_result(response->result())) g_errors.fetch_add(1, std::memory_order_relaxed);
			}
		}
		catch (...) { g_errors.fetch_add(1, std::memory_order_relaxed); }
		stat.complete(now_ns() - start);
	}
}

int main(int argc, char** argv)
{
	o = parse(argc, argv);
	if (o.variant.empty()) o.variant = "shared";
	const bool is_dedicated = (o.variant == "dedicated");
	const bool is_async = (o.variant == "async");
	const bool is_coroutine = (o.variant == "coroutine_blocking" || o.variant == "coroutine" || o.variant == "coroutine_separate" || o.variant == "coroutine_io" || o.variant == "coroutine_dedicated_io");
	const bool coroutine_same_io = (o.variant == "coroutine_io" || o.variant == "coroutine_dedicated_io");
	const bool coroutine_custom_executor = coroutine_same_io || o.variant == "coroutine_separate";
	const bool coroutine_dedicated = (o.variant == "coroutine_dedicated_io");
	if (o.defer) o.variant += "+defer";

	// 1) commands
	const std::string value = value_of(o.size);
	g_commands.reserve(o.keys);
	for (int i = 0; i < o.keys; ++i)
		g_commands.push_back(o.op == "status" ? ("eval return(redis.status_reply(redis.call('GET',KEYS[1]))) 1 " + key_of(i)) : ((o.op == "get") ? ("get " + key_of(i)) : ("set " + key_of(i) + " " + value)));

	// 2) querier
	auto pquerier = make_object<query::redis::querier_t>();
	query::redis::querier::START_PARAMETER param;
	param.remote_endpoint = net::ip::tcp::endpoint{ net::ip::make_address_v4(o.host.c_str()), static_cast<uint16_t>(o.port) };
	param.count_session = (o.sessions > 0) ? o.sessions : (is_dedicated ? o.callers : 4);
	param.wait_seconds = 5;
	param.enable_send_defer = o.enable_send_defer;
	param.request_send_window = o.request_send_window;
	param.encode_on_session = o.encode_on_session;
    param.post_response = o.post_response;
    object_ptr<executor::Iio_context_threads> response_io;
    if (o.response_executor == "system") param.presponse_executor = coroutine::default_executor();
    else if (o.response_executor == "private") { response_io = executor::make_io_context_threads("redis response benchmark", param.count_session); param.presponse_executor = response_io; }
	object_ptr<executor::Iio_context_threads> resume_io;
	object_ptr<executor::Iio_context_threads> coroutine_io;
	object_ptr<Iexecutor> coroutine_executor;
	if (coroutine_dedicated && param.count_session < o.callers)
	{
		std::fprintf(stderr, "coroutine_dedicated_io requires sessions >= callers\n");
		return 2;
	}
	if (coroutine_custom_executor)
	{
		coroutine_io = executor::make_io_context_threads("redis benchmark", param.count_session);
		if (coroutine_same_io) param.pio_context = coroutine_io;
		coroutine_executor = coroutine_io;
	}
	if (o.resume_executor == "system") param.pcoroutine_executor = coroutine::default_executor();
	else if (o.resume_executor == "private")
	{
		resume_io = executor::make_io_context_threads("redis resume benchmark", param.count_session);
		param.pcoroutine_executor = resume_io;
	}
	pquerier->start(param);
	{
		auto probe = pquerier->alloc_shared_query();
		std::printf("CONFIG io_backend=%d io_threads=%d coroutine_executor=%s send_defer=%d request_window=%zu resume_executor=%s encode_on_session=%d post_response=%d response_executor=%s callback_work_us=%.1f\n",
			static_cast<int>(probe->get_session()->io_context()->backend()), param.count_session,
			is_coroutine ? (coroutine_same_io ? "redis_io" : (coroutine_custom_executor ? "private" : o.resume_executor.c_str())) : "none",
			param.enable_send_defer ? 1 : 0, param.request_send_window, o.resume_executor.c_str(), param.encode_on_session ? 1 : 0, param.post_response ? 1 : 0, o.response_executor.c_str(), o.callback_work_us);
		if (is_coroutine && !coroutine_custom_executor)
		{
			auto system_io = dynamic_pointer_cast<executor::Iio_context_threads>(coroutine::default_executor());
			if (system_io.exist()) std::printf("CONFIG system_io_backend=%d system_threads=%zu\n",
				static_cast<int>(system_io->backend()), system_io->existing_thread_count());
		}
	}

	// 3) callers
	std::vector<caller_stat> stats(o.callers);
	g_request_stats.resize(o.callers);
	g_post_stats.resize(o.callers);
	std::vector<std::thread> threads;
	std::vector<async_chain> chains;
	std::atomic<int> remaining{ 0 };

	if (is_coroutine)
	{
		remaining = o.callers;
		for (int i = 0; i < o.callers; ++i)
			caller_coroutine(pquerier.get(), stats[i], i, coroutine_dedicated).start(coroutine_executor,
				[&remaining](std::exception_ptr error) noexcept {
					if (error) g_errors.fetch_add(1, std::memory_order_relaxed);
					if (remaining.fetch_sub(1) == 1) remaining.notify_all();
				});
	}
	else if (is_async)
	{
		chains.resize(o.callers);
		remaining = o.callers;
		for (int i = 0; i < o.callers; ++i)
		{
			chains[i].pquerier = pquerier.get();
			chains[i].pstat = &stats[i];
			chains[i].n = static_cast<uint64_t>(i) * 7919;
			chains[i].premaining = &remaining;
		}
		for (auto& c : chains) c.post();
	}
	else
	{
		for (int i = 0; i < o.callers; ++i)
		{
			threads.emplace_back([&, i]()
			{
				if (o.mode == "pipeline" && (o.variant == "batch" || o.variant == "batch_sync")) caller_batch(pquerier.get(), stats[i], i, o.variant == "batch_sync");
				else if (o.mode == "pipeline") caller_pipeline(pquerier.get(), stats[i], i, is_dedicated);
				else if (is_dedicated) caller_sync_dedicated(pquerier.get(), stats[i], i);
				else caller_sync_shared(pquerier.get(), stats[i], i);
			});
		}
	}

	run_and_report("cgdk", o, stats, [&]()
		{
			for (auto& t : threads) t.join();
			for (int v = remaining.load(); v != 0; v = remaining.load()) remaining.wait(v);
			if (is_coroutine) coroutine::wait_detached();
		});

	if (o.request_latency && o.mode == "pipeline")
	{
		histogram h;
		uint64_t total_ns = 0;
		for (auto& stat : g_request_stats) { h.merge(stat.latency); total_ns += stat.total_ns; }
		std::printf("REQUEST_LATENCY samples=%llu mean_us=%.1f p50_us=%.1f p95_us=%.1f p99_us=%.1f p999_us=%.1f\n",
			static_cast<unsigned long long>(h.total()), h.total() ? double(total_ns) / h.total() / 1000.0 : 0.0,
			h.percentile_us(.5), h.percentile_us(.95), h.percentile_us(.99), h.percentile_us(.999));
		std::fflush(stdout);
	}

    if (o.post_latency) {
        histogram h; uint64_t ns = 0;
        for (auto& stat : g_post_stats) { h.merge(stat.latency); ns += stat.total_ns; }
        std::printf("POST_LATENCY samples=%llu mean_us=%.3f p95_us=%.3f p99_us=%.3f\n", (unsigned long long)h.total(), h.total() ? double(ns) / h.total() / 1000.0 : 0.0, h.percentile_us(.95), h.percentile_us(.99));
        std::fflush(stdout);
    }

	// - destroy() : before r11608, stop() and then the last release (without destroy()) ended in 'pure virtual method called'
	pquerier->stop();
	pquerier->destroy();
	pquerier.reset();
	coroutine_executor.reset();
	if (coroutine_io.exist())
	{
		dynamic_pointer_cast<object::Istartable>(coroutine_io)->stop();
		coroutine_io.reset();
	}
	if (resume_io.exist()) dynamic_pointer_cast<object::Istartable>(resume_io)->stop();
    if (response_io.exist()) dynamic_pointer_cast<object::Istartable>(response_io)->stop();
	std::_Exit(0);	// no static tear down race with the io threads
}
