//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                  benchmark - query redis (CGDK10 query::redis)            *
//*                                                                           *
//*  variant (--variant=)                                                     *
//*   shared    : querier->execute_query_sync() / post_execute_query()        *
//*               (shared queries use a session selected per caller thread)   *
//*   dedicated : alloc_query() per caller thread (a session of its own)      *
//*   async     : sync mode only. 'callers' chains of post_execute_query()    *
//*               (the next one is posted in the response callback)           *
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
static std::vector<std::string> g_commands;	// "get key:N" / "set key:N xxx..."

static bool check_result(const resp_data& _r)
{
	if (o.op == "get")
		return _r.type == eRESP_DATA_TYPE::BULK_STRING_ && _r.str_value.size() == static_cast<std::size_t>(o.size);
	return _r.type == eRESP_DATA_TYPE::SIMPLE_STRING_;
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
			auto pquery = _pquerier->execute_query_sync(cmd);
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
			pquery->execute_query_sync(cmd);
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
	object_ptr<query::redis::query> pdedicated;
	if (_is_dedicated)
		pdedicated = _pquerier->alloc_query();

	auto on_response = [&waiter](query::redis::query* _pquery)
		{
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
				if (_is_dedicated)
				{
					// - a query object per command on the dedicated session
					auto pquery = make_object<query::redis::query>();
					pquery->set_session(pdedicated->get_session());
					pquery->post_execute_query(cmd, [pquery, on_response]() { on_response(pquery.get()); });
				}
				else
				{
					_pquerier->post_execute_query(cmd, std::function<void(query::redis::query*)>(on_response));
				}
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
				auto query = querier->execute_query_sync(commands);
				for (const auto& result : query->result().vector_child)
					if (!check_result(result)) g_errors.fetch_add(1, std::memory_order_relaxed);
			}
			else
			{
				querier->post_execute_query(commands, [&waiter](auto&& results) {
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

int main(int argc, char** argv)
{
	o = parse(argc, argv);
	if (o.variant.empty()) o.variant = "shared";
	const bool is_dedicated = (o.variant == "dedicated");
	const bool is_async = (o.variant == "async");
	if (o.defer) o.variant += "+defer";

	// 1) commands
	const std::string value = value_of(o.size);
	g_commands.reserve(o.keys);
	for (int i = 0; i < o.keys; ++i)
		g_commands.push_back((o.op == "get") ? ("get " + key_of(i)) : ("set " + key_of(i) + " " + value));

	// 2) querier
	auto pquerier = make_object<query::redis::querier_t>();
	query::redis::querier::START_PARAMETER param;
	param.remote_endpoint = net::ip::tcp::endpoint{ net::ip::make_address_v4(o.host.c_str()), static_cast<uint16_t>(o.port) };
	param.count_session = (o.sessions > 0) ? o.sessions : (is_dedicated ? o.callers : 4);
	param.wait_seconds = 5;
	pquerier->start(param);

	// 3) callers
	std::vector<caller_stat> stats(o.callers);
	std::vector<std::thread> threads;
	std::vector<async_chain> chains;
	std::atomic<int> remaining{ 0 };

	if (is_async)
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
		});

	// - destroy() : before r11608, stop() and then the last release (without destroy()) ended in 'pure virtual method called'
	pquerier->stop();
	pquerier->destroy();
	pquerier.reset();
	std::_Exit(0);	// no static tear down race with the io threads
}
