// Real Redis regression checks. Uses ECHO and a never-populated BLPOP key only.
// Usage: check_batch [port=6390]
#include "cgdk/sdk10/server.query.redis.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <thread>
#include <vector>
using namespace CGDK;

static int port = 6390;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); ++failures; } } while (false)
using results_t = query::redis::querier::batch_results;

static auto start(int sessions)
{
	auto querier = make_object<query::redis::querier_t>();
	query::redis::querier::START_PARAMETER param;
	param.remote_endpoint = net::ip::tcp::endpoint{net::ip::make_address_v4("127.0.0.1"), static_cast<uint16_t>(port)};
	param.count_session = sessions;
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

int main(int argc, char** argv)
{
	if (argc > 1) port = std::atoi(argv[1]);
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
			auto synchronous = querier->execute_query_sync(mixed);
			CHECK(synchronous->result().type == eRESP_DATA_TYPE::ARRAY_);
			CHECK(synchronous->result()[0].str_value == "first");
			CHECK(synchronous->result()[1].type == eRESP_DATA_TYPE::ERROR_);
			CHECK(synchronous->result()[3].str_value == "last");
		}
		std::vector<std::string> owned_commands = {"echo strings", "ping"};
		CHECK(querier->execute_query_sync(owned_commands)->result()[0].str_value == "strings");
		CHECK(querier->execute_query({"echo initializer", "ping"})->result()[0].str_value == "initializer");
		std::promise<results_t> owned_promise;
		auto owned_future = owned_promise.get_future();
		querier->post_execute_query(owned_commands, [&](auto&& response) { owned_promise.set_value(std::move(response)); });
		CHECK(owned_future.get()[0].str_value == "strings");
		check_coroutine(querier).get();
		const std::string_view single[] = {"echo one"};
		CHECK(execute(querier.get(), single)[0].str_value == "one");
		CHECK(querier->execute_query_sync("echo normal")->result().str_value == "normal");
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
							auto received = querier->execute_query_sync(commands);
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
			auto query = querier->execute_query_sync("client id");
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
		control->execute_query_sync("client kill id " + id);
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
			return querier->execute_query_sync({"echo first", "blpop cgdk:batch:test:never-populated 0", "echo last"});
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
