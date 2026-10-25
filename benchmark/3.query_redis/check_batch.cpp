// Real Redis regression checks. Uses ECHO and a never-populated BLPOP key only.
// Usage: check_batch [port=6390]
#include "cgdk/sdk10/server.query.redis.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <set>
#include <mutex>
#include <thread>
#include <vector>
using namespace CGDK;

static int port = 6390;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (false)
using results_t = query::redis::querier::batch_results;

static auto start(int sessions, bool enable_send_defer = true)
{
	auto querier = make_object<query::redis::querier_t>();
	query::redis::querier::START_PARAMETER param;
	param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
	param.count_session = sessions;
	param.enable_send_defer = enable_send_defer;
	querier->start(param);
	return querier;
}

static results_t execute(query::redis::querier* querier, std::span<const std::string_view> commands)
{
	std::promise<results_t> promise;
	auto future = promise.get_future();
	querier->post_execute_query(commands, [&](results_t&& results) { promise.set_value(std::move(results)); });
	if (future.wait_for(std::chrono::seconds(15)) != std::future_status::ready)
	{
		std::fprintf(stderr, "batch timeout\n");
		std::abort(); // Do not return with a dangling callback capture.
	}
	return future.get();
}

static coroutine::task<void> check_coroutine(object_ptr<query::redis::querier_t> querier)
{
	std::vector<std::string> commands = {"echo coroutine", "ping"};
	auto deferred = querier->execute_query(commands);
	commands.clear(); // The deferred operation must own these strings.
	auto result = co_await std::move(deferred);
	CHECK(result->result().type == eRESP_DATA_TYPE::ARRAY_);
	CHECK(result->result()[0].str_value == "coroutine");
	CHECK(result->result()[1].str_value == "PONG");
	std::vector<std::string_view> sync_commands = {"echo sync-in-coroutine", "ping"};
	auto synchronous = querier->execute_query(sync_commands);
	CHECK(synchronous->result()[0].str_value == "sync-in-coroutine");
}

struct executor_probe
{
    Iexecutor* value = nullptr;
    bool await_ready() const noexcept { return false; }
    template<class P> bool await_suspend(std::coroutine_handle<P> handle) noexcept { value = handle.promise().executor().get(); return false; }
    Iexecutor* await_resume() const noexcept { return value; }
};
static coroutine::task<void> check_resume_routes(object_ptr<query::redis::querier_t> querier,
    Iexecutor* expected, object_ptr<Iexecutor> override_executor)
{
    auto scalar = querier->execute_query("echo scalar-resume");
    auto result = override_executor.exist() ? co_await scalar.resume_on(override_executor) : co_await scalar;
    CHECK(result->result().str_value == "scalar-resume"); CHECK((co_await executor_probe{}) == expected);
    auto batch = querier->execute_query({"echo batch-resume", "ping"});
    auto results = override_executor.exist() ? co_await batch.resume_on(override_executor) : co_await batch;
    CHECK(results->result()[0].str_value == "batch-resume"); CHECK((co_await executor_probe{}) == expected);
    result.reset(); results.reset();
    auto query = co_await querier->co_alloc_query();
    auto operation = query->execute_query("echo query-resume");
    if (override_executor.exist()) co_await operation.resume_on(override_executor); else co_await operation;
    CHECK(query->result().str_value == "query-resume"); CHECK((co_await executor_probe{}) == expected);
}
static void check_real_resume_routes()
{
    auto other = executor::make_io_context_threads("redis resume checks", 1);
    for (int policy = 0; policy < 4; ++policy) {
        auto querier = make_object<query::redis::querier_t>();
        query::redis::querier::START_PARAMETER param;
        param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
        param.count_session = 1; param.request_send_window = policy == 0 ? 6 : 10;
        if (policy == 2) param.pcoroutine_executor = other;
        querier->start(param);
        auto session = querier->alloc_shared_query()->get_session();
        CHECK(session->request_send_window() == param.request_send_window);
        Iexecutor* expected = policy ? static_cast<Iexecutor*>(other.get()) : static_cast<Iexecutor*>(session->io_context().get());
        std::promise<void> done; auto future = done.get_future();
        check_resume_routes(querier, expected, policy == 3 ? object_ptr<Iexecutor>(other) : object_ptr<Iexecutor>()).start(
            policy == 1 ? object_ptr<Iexecutor>(other) : object_ptr<Iexecutor>(),
            [&](std::exception_ptr error) { if (error) done.set_exception(error); else done.set_value(); });
        if (future.wait_for(std::chrono::seconds(10)) != std::future_status::ready) std::abort();
        future.get(); coroutine::wait_detached(); session.reset(); querier->stop(); querier->destroy();
    }
    dynamic_pointer_cast<object::Istartable>(other)->stop();
    std::printf("coroutine_resume default/custom/querier/operation checks complete\n");
}

static void check_send_defer_option(bool enabled, bool from_context)
{
	object_ptr<query::redis::querier_t> querier;
	if (from_context)
	{
		querier = make_object<query::redis::querier_t>();
		context config;
		config["remote_endpoint"]["address"] = "127.0.0.1";
		config["remote_endpoint"]["port"] = port;
		config["session"] = 1;
		config["send_defer"] = enabled;
		querier->start(config);
	}
	else querier = start(1, enabled);
	CHECK(querier->alloc_shared_query()->get_session()->is_send_defer_enabled() == enabled);
	std::promise<bool> single, batch;
	auto single_ready = single.get_future();
	auto batch_ready = batch.get_future();
	{
		send_defer::scope group;
		querier->post_execute_query(std::string_view("echo option"), [&](query::redis::query* query) {
			single.set_value(query->result().str_value == "option");
		});
		querier->post_execute_query({"echo batch-option", "ping"}, [&](results_t&& result) {
			batch.set_value(result.size() == 2 && result[0].str_value == "batch-option" && result[1].str_value == "PONG");
		});
		// A plain future wait does not flush CGDK's deferred sends.
		if (enabled)
			CHECK(single_ready.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout);
		else
		{
			CHECK(single_ready.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
			CHECK(batch_ready.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		}
	}
	if (single_ready.wait_for(std::chrono::seconds(5)) != std::future_status::ready ||
		batch_ready.wait_for(std::chrono::seconds(5)) != std::future_status::ready) std::abort();
	CHECK(single_ready.get()); CHECK(batch_ready.get());
	querier->stop(); querier->destroy();
}

static void check_send_defer_isolation()
{
	auto enabled = start(1, true), disabled = start(1, false);
	std::promise<void> on_done, off_done;
	auto on = on_done.get_future(), off = off_done.get_future();
	{
		send_defer::scope group;
		enabled->post_execute_query(std::string_view("ping"), [&](auto*) { on_done.set_value(); });
		disabled->post_execute_query(std::string_view("ping"), [&](auto*) { off_done.set_value(); });
		CHECK(off.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		CHECK(on.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout);
	}
	if (on.wait_for(std::chrono::seconds(5)) != std::future_status::ready ||
		off.wait_for(std::chrono::seconds(5)) != std::future_status::ready) std::abort();
	enabled->stop(); enabled->destroy(); disabled->stop(); disabled->destroy();
}

static void check_callback_connection_affinity(int sessions)
{
	auto querier = start(sessions);
	struct chain_state { int index = 0; query::redis::session* home = nullptr; std::function<void()> post; };
	constexpr int count = 32, per_chain = 1000;
	std::vector<chain_state> chains(count);
	std::atomic<int> remaining{count}, errors{0};
	std::promise<void> complete; auto done = complete.get_future();
	std::set<query::redis::session*> homes; std::mutex homes_lock;
	for (int i = 0; i < count; ++i)
		chains[i].post = [&, i] {
			const auto expected = "affinity-" + std::to_string(i) + "-" + std::to_string(chains[i].index);
			const auto command = "ECHO " + expected;
			querier->post_execute_query(std::string_view(command), [&, i, expected](query::redis::query* reply) {
				auto& chain = chains[i];
				if (reply->result().str_value != expected) ++errors;
				const auto session = reply->get_session();
				if (chain.index == 1)
				{
					chain.home = session.get();
					std::lock_guard<std::mutex> lock(homes_lock); homes.insert(chain.home);
				}
				else if (chain.index > 1 && chain.home != session.get()) ++errors;
				if (++chain.index == per_chain)
				{
					if (remaining.fetch_sub(1) == 1) complete.set_value();
				}
				else chain.post();
			});
		};
	for (auto& chain : chains) chain.post();
	if (done.wait_for(std::chrono::seconds(20)) != std::future_status::ready) std::abort();
	CHECK(errors.load() == 0); CHECK(homes.size() == static_cast<std::size_t>(sessions));
	querier->stop(); querier->destroy();
	std::printf("callback_affinity sessions=%d commands=%d homes=%zu errors=%d\n", sessions, count * per_chain, homes.size(), errors.load());
}

static void check_stop_from_affine_callback()
{
	auto querier = start(4);
	std::promise<bool> stopped; auto done = stopped.get_future();
	querier->post_execute_query(std::string_view("PING"), [&](auto*) {
		querier->post_execute_query(std::string_view("PING"), [&](auto*) {
			querier->stop();
			bool rejected = false;
			try { querier->post_execute_query(std::string_view("PING"), [](auto*) {}); }
			catch (const throwable::failure&) { rejected = true; }
			stopped.set_value(rejected);
		});
	});
	if (done.wait_for(std::chrono::seconds(10)) != std::future_status::ready) std::abort();
	CHECK(done.get()); querier->destroy();
}

static void check_concurrent_mixed_encoding()
{
    auto querier = start(1);
    std::atomic<int> left{2400}, errors{0}; std::promise<void> done; auto future = done.get_future();
    auto finish = [&] { if (left.fetch_sub(1) == 1) done.set_value(); };
    std::vector<std::thread> producers;
    for (int producer = 0; producer < 8; ++producer) producers.emplace_back([&, producer] {
        for (int i = 0; i < 300; ++i) {
            const auto expected = "mixed-" + std::to_string(producer) + "-" + std::to_string(i);
            if (i % 2) {
                std::vector<std::string> commands{"echo " + expected, "echo " + expected + "-tail", "ping"};
                querier->post_execute_query(commands, [&, expected](results_t&& results) {
                    if (results.size() != 3 || results[0].str_value != expected || results[1].str_value != expected + "-tail" || results[2].str_value != "PONG") ++errors;
                    finish();
                });
                commands.clear();
            } else querier->post_execute_query("echo " + expected, [&, expected](query::redis::query* reply) {
                if (reply->result().str_value != expected) ++errors; finish();
            });
        }
    });
    for (auto& producer : producers) producer.join();
    if (future.wait_for(std::chrono::seconds(15)) != std::future_status::ready) std::abort();
    CHECK(errors == 0); CHECK(querier->statistics_get_query_processing() == 0);
    querier->stop(); querier->destroy();
    std::printf("mixed_encoding producers=8 commands=4800 errors=%d\n", errors.load());
}

static void check_fixed_route_restart()
{
    auto querier = make_object<query::redis::querier_t>();
    query::redis::querier::START_PARAMETER param;
    param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
    param.count_session = 2;
    for (int generation = 0; generation < 3; ++generation) {
        querier->start(param);
        auto old = querier->alloc_shared_query();
        CHECK(querier->execute_query("PING")->result().str_value == "PONG");
        querier->stop();
        bool rejected = false;
        try { querier->post_execute_query(std::string_view("PING"), [](auto*) {}); } catch (const throwable::failure&) { rejected = true; }
        CHECK(rejected); rejected = false;
        try { (void)querier->alloc_shared_query(); } catch (const throwable::failure&) { rejected = true; }
        CHECK(rejected); old.reset();
        CHECK(querier->statistics_get_query_processing() == 0);
    }
    querier->destroy(); std::printf("fixed_route stop/restart checks complete\n");
}

static void check_posted_response_sync_and_stop()
{
    for (bool separate : {false, true}) {
        object_ptr<executor::Iio_context_threads> response_io;
        auto querier = make_object<query::redis::querier_t>();
        query::redis::querier::START_PARAMETER param;
        param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
        param.count_session = 1; param.post_response = true;
        if (separate) { response_io = executor::make_io_context_threads("callback test", 1); param.presponse_executor = response_io; }
        querier->start(param);
        std::promise<void> complete; auto ready = complete.get_future();
        querier->post_execute_query(std::string_view("PING"), [&](query::redis::query* first) {
            try {
                CHECK(first->result().str_value == "PONG");
                CHECK(querier->execute_query("PING")->result().str_value == "PONG");
                querier->stop(); complete.set_value();
            } catch (...) { complete.set_exception(std::current_exception()); }
        });
        if (ready.wait_for(std::chrono::seconds(10)) != std::future_status::ready) std::abort();
        ready.get();
        if (response_io.exist()) dynamic_pointer_cast<object::Istartable>(response_io)->stop();
        querier->destroy();
    }
    std::printf("posted_response synchronous callback and stop checks complete\n");
}

static void check_redis_has_no_native_cork()
{
    {
        auto querier = make_object<query::redis::querier_t>();
        query::redis::querier::START_PARAMETER param;
        param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
        param.count_session = 1; param.post_response = false;
        querier->start(param);
        std::promise<bool> received; auto ready = received.get_future();
        querier->post_execute_query(std::string_view("PING"), [&](query::redis::query* query) {
            CHECK(!query->get_session()->send_cork_enabled());
            received.set_value(send_defer::cork_socket() == static_cast<const net::io::Isocket*>(query->get_session().get()));
        });
        if (ready.wait_for(std::chrono::seconds(5)) != std::future_status::ready) std::abort();
#if defined(_SYS_EPOLL_H)
        CHECK(!ready.get());
#else
        (void)ready.get();
#endif
        querier->stop(); querier->destroy();
    }
    std::printf("Redis native cork disabled check complete\n");
}

int main(int argc, char** argv)
{
	if (argc > 1) port = std::atoi(argv[1]);
	check_redis_has_no_native_cork();
	check_posted_response_sync_and_stop();
	check_fixed_route_restart();
	check_concurrent_mixed_encoding();
	check_real_resume_routes();
	check_stop_from_affine_callback();
	check_callback_connection_affinity(1);
	check_callback_connection_affinity(4);
	check_send_defer_isolation();
	for (bool enabled : {true, false})
		for (bool from_context : {false, true}) check_send_defer_option(enabled, from_context);
	for (int sessions : {1, 4})
	{
		auto querier = start(sessions);
		const std::string_view mixed[] = {"echo first", "cgdk_nonexistent_command", "ping", "echo last"};
		auto results = execute(querier.get(), mixed);
		CHECK(results.size() == 4);
		CHECK(results[0].str_value == "first");
		CHECK(results[1].type == eRESP_DATA_TYPE::ERROR_);
		CHECK(results[2].str_value == "PONG");
		CHECK(results[3].str_value == "last");
		{
			send_defer::scope defer; // A synchronous batch must flush/suspend deferral.
			auto synchronous = querier->execute_query(mixed);
			CHECK(synchronous->result().type == eRESP_DATA_TYPE::ARRAY_);
			CHECK(synchronous->result()[0].str_value == "first");
			CHECK(synchronous->result()[1].type == eRESP_DATA_TYPE::ERROR_);
			CHECK(synchronous->result()[3].str_value == "last");
		}
		std::vector<std::string> owned_commands = {"echo strings", "ping"};
		CHECK(querier->execute_query(owned_commands)->result()[0].str_value == "strings");
		CHECK(querier->execute_query({"echo initializer", "ping"})->result()[0].str_value == "initializer");
		std::promise<results_t> owned_promise;
		auto owned_future = owned_promise.get_future();
		querier->post_execute_query(owned_commands, [&](auto&& response) { owned_promise.set_value(std::move(response)); });
		CHECK(owned_future.get()[0].str_value == "strings");
		check_coroutine(querier).get();
		const std::string_view single[] = {"echo one"};
		CHECK(execute(querier.get(), single)[0].str_value == "one");
		CHECK(querier->execute_query("echo normal")->result().str_value == "normal");
		// Encoding must copy inputs before returning and preserve large values.
		std::string large = "echo " + std::string(128 * 1024, 'x');
		const std::string_view large_commands[] = {large};
		CHECK(execute(querier.get(), large_commands)[0].str_value.size() == 128 * 1024);
		std::atomic<int> errors{0};
		std::vector<std::thread> callers;
		for (int caller = 0; caller < 4; ++caller)
			callers.emplace_back([&, caller] {
				try
				{
					for (int batch = 0; batch < 30; ++batch)
					{
						std::vector<std::string> storage;
						std::vector<std::string_view> commands;
						for (int item = 0; item < 100; ++item)
							storage.push_back("echo " + std::to_string(caller) + ":" + std::to_string(batch * 100 + item));
						for (const auto& command : storage) commands.push_back(command);
						if (batch % 2)
						{
							auto received = querier->execute_query(commands);
							for (std::size_t i = 0; i < received->result().size(); ++i)
								if (received->result()[static_cast<int>(i)].str_value != std::string_view(storage[i]).substr(5)) ++errors;
						}
						else
						{
							auto received = execute(querier.get(), commands);
							for (std::size_t i = 0; i < received.size(); ++i)
								if (received[i].str_value != std::string_view(storage[i]).substr(5)) ++errors;
						}
					}
				}
				catch (...) { ++errors; }
			});
		for (auto& caller : callers) caller.join();
		CHECK(errors == 0);
		querier->stop();
		CHECK(querier->statistics_get_query_processing() == 0);
		querier.reset();
		std::printf("sessions=%d concurrent_commands=12000 errors=%d\n", sessions, errors.load());
	}
	// One dedicated query occupies the only session: the batch must wait, then run.
	{
		auto querier = start(1);
		auto dedicated = querier->alloc_query();
		std::promise<results_t> promise;
		auto future = promise.get_future();
		std::atomic<int> calls{0};
		{
			std::string ephemeral = "echo queued";
			const std::string_view commands[] = {ephemeral};
			querier->post_execute_query(commands, [&](results_t&& results) { ++calls; promise.set_value(std::move(results)); });
		}
		CHECK(calls == 0);
		dedicated.reset();
		CHECK(future.wait_for(std::chrono::seconds(10)) == std::future_status::ready);
		CHECK(future.get()[0].str_value == "queued");
		CHECK(calls == 1);
		querier->stop();
	}
	// Cancellation while waiting for a session must invoke the callback once.
	{
		auto querier = start(1);
		auto dedicated = querier->alloc_query();
		std::atomic<int> calls{0};
		const std::string_view commands[] = {"echo queued"};
		querier->post_execute_query(commands, [&](results_t&& results) {
			CHECK(results.size() == 1 && results[0].type == eRESP_DATA_TYPE::NONE_);
			++calls;
		});
		querier->stop();
		CHECK(calls == 1);
		dedicated.reset();
	}
	// One reply arrives, then BLPOP blocks: stop retains the first reply and cancels the rest.
	{
		auto querier = start(1);
		std::promise<results_t> promise;
		auto future = promise.get_future();
		std::atomic<int> calls{0};
		const std::string_view commands[] = {"echo first", "blpop cgdk:batch:test:never-populated 0", "echo last"};
		querier->post_execute_query(commands, [&](results_t&& results) { ++calls; promise.set_value(std::move(results)); });
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (querier->statistics_get_query_success() != 1 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		CHECK(querier->statistics_get_query_success() == 1);
		querier->stop();
		CHECK(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		auto results = future.get();
		CHECK(results[0].str_value == "first");
		CHECK(results[1].type == eRESP_DATA_TYPE::NONE_);
		CHECK(results[2].type == eRESP_DATA_TYPE::NONE_);
		CHECK(calls == 1);
		CHECK(querier->statistics_get_query_processing() == 0);
	}
	// A transport disconnect cancels the unfinished batch rather than replaying it.
	{
		auto querier = start(1);
		auto control = start(1);
		std::string id;
		{
			auto query = querier->execute_query("client id");
			id = query->result().str_value;
		}
		std::promise<results_t> promise;
		auto future = promise.get_future();
		std::atomic<int> calls{0};
		querier->post_execute_query({"echo first", "blpop cgdk:batch:test:never-populated 0", "echo last"}, [&](results_t&& results) {
			++calls;
			promise.set_value(std::move(results));
		});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (querier->statistics_get_query_success() != 2 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		CHECK(querier->statistics_get_query_success() == 2);
		control->execute_query("client kill id " + id);
		CHECK(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		auto results = future.get();
		CHECK(results[0].str_value == "first");
		CHECK(results[1].type == eRESP_DATA_TYPE::NONE_);
		CHECK(results[2].type == eRESP_DATA_TYPE::NONE_);
		CHECK(calls == 1);
		CHECK(querier->statistics_get_query_processing() == 0);
		querier->stop();
		control->stop();
	}
	// Completion may post another batch on the same connection without deadlocking.
	{
		auto querier = start(1);
		std::promise<results_t> promise;
		auto future = promise.get_future();
		querier->post_execute_query({"echo outer"}, [&](results_t&&) {
			querier->post_execute_query({"echo inner"}, [&](results_t&& results) { promise.set_value(std::move(results)); });
		});
		CHECK(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		CHECK(future.get()[0].str_value == "inner");
		querier->stop();
	}
	// A synchronous batch is woken by stop, preserving partial results.
	{
		auto querier = start(1);
		auto future = std::async(std::launch::async, [&] {
			return querier->execute_query({"echo first", "blpop cgdk:batch:test:never-populated 0", "echo last"});
		});
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
		while (querier->statistics_get_query_success() != 1 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		CHECK(querier->statistics_get_query_success() == 1);
		querier->stop();
		CHECK(future.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
		auto result = future.get();
		CHECK(result->result().type == eRESP_DATA_TYPE::ARRAY_);
		CHECK(result->result()[0].str_value == "first");
		CHECK(result->result()[1].type == eRESP_DATA_TYPE::NONE_);
		CHECK(result->result()[2].type == eRESP_DATA_TYPE::NONE_);
		CHECK(querier->statistics_get_query_processing() == 0);
	}
	std::printf("%s failures=%d\n", failures ? "FAILED" : "OK", failures);
	return failures ? 1 : 0;
}
