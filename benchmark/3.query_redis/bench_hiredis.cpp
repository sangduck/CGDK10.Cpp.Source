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

int main(int argc, char** argv)
{
	auto o = parse(argc, argv);
	if (o.populate) { populate(o); return 0; }

	std::vector<caller_stat> stats(o.callers);
	std::vector<std::thread> threads;
	for (int i = 0; i < o.callers; ++i)
		threads.emplace_back([&, i]() { caller(o, stats[i], i); });

	run_and_report("hiredis", o, stats, [&]() { for (auto& t : threads) t.join(); });
	return 0;
}
