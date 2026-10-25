//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                 benchmark - query redis (redis-plus-plus)                 *
//*                                                                           *
//*  sync     : a thread per caller on one sw::redis::Redis (pool = callers)  *
//*  pipeline : sw::redis::Pipeline per thread (own connection)               *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#include "bench_common.h"
#include <sw/redis++/redis++.h>

using namespace bench;

int main(int argc, char** argv)
{
	auto o = parse(argc, argv);

	sw::redis::ConnectionOptions co;
	co.host = o.host;
	co.port = o.port;
	sw::redis::ConnectionPoolOptions po;
	po.size = static_cast<std::size_t>(o.sessions > 0 ? o.sessions : o.callers);
	sw::redis::Redis redis(co, po);

	const std::string value = value_of(o.size);
	std::vector<std::string> keys;
	keys.reserve(o.keys);
	for (int i = 0; i < o.keys; ++i) keys.push_back(key_of(i));
	const bool is_get = (o.op == "get");

	std::vector<caller_stat> stats(o.callers);
	std::vector<std::thread> threads;
	for (int ci = 0; ci < o.callers; ++ci)
	{
		threads.emplace_back([&, ci]()
		{
			uint64_t n = static_cast<uint64_t>(ci) * 7919;
			auto& stat = stats[ci];
			try
			{
				if (o.mode == "pipeline")
				{
					auto pipe = redis.pipeline(false);
					while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
					{
						const uint64_t t = now_ns();
						for (int i = 0; i < o.pipeline; ++i)
						{
							const auto& k = keys[(n++) % keys.size()];
							if (is_get) pipe.get(k); else pipe.set(k, value);
						}
						auto replies = pipe.exec();
						for (std::size_t i = 0; i < replies.size(); ++i)
						{
							bool ok = is_get ? (replies.get<sw::redis::OptionalString>(i).value_or("").size() == value.size()) : replies.get<bool>(i);
							if (!ok) g_errors.fetch_add(1, std::memory_order_relaxed);
						}
						stat.complete(now_ns() - t, o.pipeline);
					}
				}
				else
				{
					while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
					{
						const auto& k = keys[(n++) % keys.size()];
						const uint64_t t = now_ns();
						bool ok;
						if (is_get) { auto v = redis.get(k); ok = v && v->size() == value.size(); }
						else ok = redis.set(k, value);
						if (!ok) g_errors.fetch_add(1, std::memory_order_relaxed);
						stat.complete(now_ns() - t);
					}
				}
			}
			catch (const std::exception& e)
			{
				std::fprintf(stderr, "redis++ error: %s\n", e.what());
				std::exit(1);
			}
		});
	}

	run_and_report("redis++", o, stats, [&]() { for (auto& t : threads) t.join(); });
	return 0;
}
