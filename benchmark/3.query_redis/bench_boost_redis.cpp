//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.11                        *
//*                                                                           *
//*                  benchmark - query redis (Boost.Redis)                    *
//*                                                                           *
//*  a connection = an io_context + a thread (basic_connection, not erased)   *
//*  sync     : 'callers' coroutines spread over 'sessions' connections       *
//*             (default 1 : Boost.Redis multiplexes one connection)          *
//*  pipeline : a connection per caller, one request with N commands          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#include "bench_common.h"
#include <memory>
#include <boost/asio.hpp>
#include <boost/redis.hpp>
#include <boost/redis/src.hpp>

using namespace bench;
namespace asio = boost::asio;
namespace redis = boost::redis;
using connection_t = redis::basic_connection<asio::io_context::executor_type>;

struct worker
{
	asio::io_context				ioc{ 1 };
	std::shared_ptr<connection_t>	conn;
	std::atomic<int>				remaining{ 0 };
	std::thread						thread;
};

static asio::awaitable<void> caller(worker& _w, const options& _o, const std::vector<std::string>& _keys, const std::string& _value, caller_stat& _stat, int _index)
{
	const bool is_get = (_o.op == "get");
	uint64_t n = static_cast<uint64_t>(_index) * 7919;
	redis::request req;
	redis::generic_response resp;

	while (g_phase.load(std::memory_order_relaxed) != PHASE_STOP)
	{
		const uint64_t t = now_ns();
		req.clear();
		for (int i = 0; i < _o.pipeline; ++i)
		{
			const auto& k = _keys[(n++) % _keys.size()];
			if (is_get) req.push("GET", k); else req.push("SET", k, _value);
		}
		resp.value().clear();

		auto [ec, size] = co_await _w.conn->async_exec(req, resp, asio::as_tuple(asio::use_awaitable));
		(void)size;

		if (ec || !resp.has_value() || resp.value().size() != static_cast<std::size_t>(_o.pipeline))
			g_errors.fetch_add(1, std::memory_order_relaxed);
		else if (is_get)
			for (auto& node : resp.value()) if (node.value.size() != _value.size()) g_errors.fetch_add(1, std::memory_order_relaxed);

		_stat.complete(now_ns() - t, _o.pipeline);
	}

	// - the last caller of this connection stops it
	if (_w.remaining.fetch_sub(1) == 1)
		_w.conn->cancel();
}

int main(int argc, char** argv)
{
	auto o = parse(argc, argv);

	const std::string value = value_of(o.size);
	std::vector<std::string> keys;
	keys.reserve(o.keys);
	for (int i = 0; i < o.keys; ++i) keys.push_back(key_of(i));

	// - connections : pipeline -> one per caller, sync -> 'sessions' (default 1)
	const int count_conn = (o.mode == "pipeline") ? o.callers : std::max(1, std::min(o.sessions > 0 ? o.sessions : 1, o.callers));

	redis::config cfg;
	cfg.addr.host = o.host;
	cfg.addr.port = std::to_string(o.port);
	cfg.health_check_interval = std::chrono::seconds(0);
	cfg.reconnect_wait_interval = std::chrono::seconds(0);

	std::vector<std::unique_ptr<worker>> workers;
	for (int i = 0; i < count_conn; ++i)
	{
		auto w = std::make_unique<worker>();
		w->conn = std::make_shared<connection_t>(w->ioc.get_executor(), redis::logger{ redis::logger::level::err });
		workers.push_back(std::move(w));
	}

	std::vector<caller_stat> stats(o.callers);
	for (int ci = 0; ci < o.callers; ++ci)
	{
		auto& w = *workers[ci % count_conn];
		w.remaining.fetch_add(1);
		asio::co_spawn(w.ioc, caller(w, o, keys, value, stats[ci], ci), asio::detached);
	}
	for (auto& w : workers)
	{
		w->conn->async_run(cfg, [](boost::system::error_code) {});
		w->thread = std::thread([pw = w.get()]() { pw->ioc.run(); });
	}

	run_and_report("boost.redis", o, stats, [&]() { for (auto& w : workers) w->thread.join(); });
	return 0;
}
