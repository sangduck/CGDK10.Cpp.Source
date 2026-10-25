//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                    benchmark - query redis (hiredis)                      *
//*                                                                           *
//*  sync     : a thread per caller, a connection per thread (redisCommandArgv)
//*  pipeline : redisAppendCommandArgv x N -> redisGetReply x N               *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#include "bench_common.h"
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/poll.h>
#include <memory>

using namespace bench;

static bool check_reply(redisReply* _r, const options& _o)
{
	if (_r == nullptr) return false;
	bool ok = (_o.op == "get") ? (_r->type == REDIS_REPLY_STRING && _r->len == static_cast<std::size_t>(_o.size)) : (_r->type == REDIS_REPLY_STATUS);
	freeReplyObject(_r);
	return ok;
}

static void caller(const options& _o, caller_stat& _stat, int _index)
{
	redisContext* c = redisConnect(_o.host.c_str(), _o.port);
	if (c == nullptr || c->err) { std::fprintf(stderr, "connect failed\n"); std::exit(1); }

	const std::string value = value_of(_o.size);
	std::vector<std::string> keys;
	keys.reserve(_o.keys);
	for (int i = 0; i < _o.keys; ++i) keys.push_back(key_of(i));

	const bool is_get = (_o.op == "get");
	const char* argv[3] = { is_get ? "GET" : "SET", nullptr, value.data() };
	size_t argl[3] = { 3, 0, value.size() };
	const int argc = is_get ? 2 : 3;

	uint64_t n = static_cast<uint64_t>(_index) * 7919;
	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const uint64_t t = now_ns();
		if (_o.mode == "pipeline")
		{
			for (int i = 0; i < _o.pipeline; ++i)
			{
				const auto& k = keys[(n++) % keys.size()];
				argv[1] = k.data(); argl[1] = k.size();
				redisAppendCommandArgv(c, argc, argv, argl);
			}
			for (int i = 0; i < _o.pipeline; ++i)
			{
				void* r = nullptr;
				if (redisGetReply(c, &r) != REDIS_OK || !check_reply(static_cast<redisReply*>(r), _o)) g_errors.fetch_add(1, std::memory_order_relaxed);
			}
			_stat.complete(now_ns() - t, _o.pipeline);
		}
		else
		{
			const auto& k = keys[(n++) % keys.size()];
			argv[1] = k.data(); argl[1] = k.size();
			if (!check_reply(static_cast<redisReply*>(redisCommandArgv(c, argc, argv, argl)), _o)) g_errors.fetch_add(1, std::memory_order_relaxed);
			_stat.complete(now_ns() - t);
		}
	}
	redisFree(c);
}

static void populate(const options& _o)
{
	redisContext* c = redisConnect(_o.host.c_str(), _o.port);
	if (c == nullptr || c->err) { std::fprintf(stderr, "connect failed\n"); std::exit(1); }
	const std::string value = value_of(_o.size);
	for (int i = 0; i < _o.keys; ++i)
	{
		auto k = key_of(i);
		const char* argv[3] = { "SET", k.data(), value.data() };
		size_t argl[3] = { 3, k.size(), value.size() };
		redisAppendCommandArgv(c, 3, argv, argl);
		if ((i % 256) == 255 || i == _o.keys - 1)
			for (int j = (i / 256) * 256; j <= i; ++j) { void* r = nullptr; redisGetReply(c, &r); if (r) freeReplyObject(r); }
	}
	redisFree(c);
	std::printf("populated %d keys x %d bytes\n", _o.keys, _o.size);
}

// Callback chains: one outstanding command per chain, shared over N connections.
// The bundled poll adapter needs no third-party event-loop library.
struct async_worker;
struct async_chain
{
	async_worker* worker = nullptr;
	caller_stat* stat = nullptr;
	uint64_t n = 0;
	uint64_t started = 0;
	void post();
};
struct async_worker
{
	const options* opts = nullptr;
	const std::vector<std::string>* keys = nullptr;
	const std::string* value = nullptr;
	redisAsyncContext* context = nullptr;
	std::vector<async_chain> chains;
	std::size_t active = 0;
	bool disconnected = false;
	std::thread thread;
};
static void async_reply(redisAsyncContext*, void* reply, void* data)
{
	auto& chain = *static_cast<async_chain*>(data);
	const auto& o = *chain.worker->opts;
	auto* r = static_cast<redisReply*>(reply);
	const bool ok = r && (o.op == "get" ?
		(r->type == REDIS_REPLY_STRING && r->len == static_cast<std::size_t>(o.size)) : r->type == REDIS_REPLY_STATUS);
	if (!ok) g_errors.fetch_add(1, std::memory_order_relaxed);
	chain.stat->complete(now_ns() - chain.started);
	chain.post();
}
void async_chain::post()
{
	if (g_phase.load(std::memory_order_relaxed) == PHASE_STOP)
	{
		if (--worker->active == 0) redisAsyncDisconnect(worker->context);
		return;
	}
	const auto& key = (*worker->keys)[(n++) % worker->keys->size()];
	const bool get = worker->opts->op == "get";
	const char* argv[] = { get ? "GET" : "SET", key.data(), worker->value->data() };
	const size_t lengths[] = { 3, key.size(), worker->value->size() };
	started = now_ns();
	if (redisAsyncCommandArgv(worker->context, async_reply, this, get ? 2 : 3, argv, lengths) != REDIS_OK)
	{
		std::fprintf(stderr, "hiredis async submit failed\n");
		std::exit(1);
	}
}
static void run_async(const options& o)
{
	if (o.mode != "sync" || o.callers < 1) { std::fprintf(stderr, "async requires mode=sync and callers>0\n"); std::exit(2); }
	const auto value = value_of(o.size);
	std::vector<std::string> keys;
	for (int i = 0; i < o.keys; ++i) keys.push_back(key_of(i));
	const int count = std::max(1, std::min(o.sessions > 0 ? o.sessions : 1, o.callers));
	std::vector<caller_stat> stats(o.callers);
	std::vector<std::unique_ptr<async_worker>> workers;
	for (int i = 0; i < count; ++i)
	{
		auto w = std::make_unique<async_worker>();
		w->opts = &o; w->keys = &keys; w->value = &value;
		workers.push_back(std::move(w));
	}
	for (int i = 0; i < o.callers; ++i)
	{
		auto& w = *workers[i % count];
		w.chains.push_back(async_chain{ &w, &stats[i], static_cast<uint64_t>(i) * 7919, 0 });
	}
	for (auto& w : workers)
		w->thread = std::thread([pw = w.get()] {
			pw->context = redisAsyncConnect(pw->opts->host.c_str(), pw->opts->port);
			if (!pw->context || pw->context->err || redisPollAttach(pw->context) != REDIS_OK)
			{ std::fprintf(stderr, "hiredis async connect failed\n"); std::exit(1); }
			pw->context->data = pw;
			redisAsyncSetDisconnectCallback(pw->context, [](const redisAsyncContext* c, int status) {
				auto* w = static_cast<async_worker*>(c->data);
				w->disconnected = true;
				if (status != REDIS_OK) g_errors.fetch_add(1, std::memory_order_relaxed);
			});
			redisAsyncSetConnectCallback(pw->context, [](const redisAsyncContext*, int status) {
				if (status != REDIS_OK) { std::fprintf(stderr, "hiredis async connection failed\n"); std::exit(1); }
			});
			pw->active = pw->chains.size();
			for (auto& chain : pw->chains) chain.post();
			while (!pw->disconnected)
				if (redisPollTick(pw->context, 0.1) < 0) { std::fprintf(stderr, "hiredis poll failed\n"); std::exit(1); }
		});
	run_and_report("hiredis.async", o, stats, [&] { for (auto& w : workers) w->thread.join(); });
}

int main(int argc, char** argv)
{
	auto o = parse(argc, argv);
	if (o.populate) { populate(o); return 0; }
	if (o.variant == "async") { run_async(o); return 0; }

	std::vector<caller_stat> stats(o.callers);
	std::vector<std::thread> threads;
	for (int i = 0; i < o.callers; ++i)
		threads.emplace_back([&, i]() { caller(o, stats[i], i); });

	run_and_report("hiredis", o, stats, [&]() { for (auto& t : threads) t.join(); });
	return 0;
}
