#include "pch.h"
#include <algorithm>
#include <atomic>
#include <mutex>

//-----------------------------------------------------------------------------
// net::kafka::producer / net::kafka::consumer on the mock cluster of librdkafka
//
//  - "test.mock.num.brokers" starts an in-process mock cluster: no broker is needed.
//  - the bootstrap servers of the mock cluster are taken from the producer's
//    native handle (rd_kafka_handle_mock_cluster) and given to the consumer.
//  - the callbacks (on_delivery(), on_message()) are called on the poll threads,
//    so the tests read the results with atomics and a lock.
//-----------------------------------------------------------------------------
namespace CGDK
{
	// wait until _predicate() is true (timeout : false)
	template <class TPREDICATE>
	static bool wait_until_true(TPREDICATE&& _predicate, chrono::tick::duration _timeout = 5s)
	{
		auto tick_end = chrono::tick::now() + _timeout;
		while (!_predicate())
		{
			if (chrono::tick::now() > tick_end)
				return false;
			api::sleep_for(10ms);
		}
		return true;
	}

	namespace
	{
		constexpr std::string_view TEST_TOPIC = "unit_test_topic";

		// - counts the delivery reports
		class test_producer : public net::kafka::producer
		{
		public:
			std::atomic<int>	count_success{ 0 };
			std::atomic<int>	count_fail{ 0 };

		protected:
			virtual void on_delivery(sKAFKA_DELIVERY& _delivery) override
			{
				if (_delivery.result == eRESULT::SUCCESS)
					++this->count_success;
				else
					++this->count_fail;
			}
		};

		// - keeps the received messages (key, value)
		class test_consumer : public net::kafka::consumer
		{
		public:
			std::atomic<int>	count_received{ 0 };
			std::mutex			lock;
			std::vector<std::pair<std::string, std::string>> messages;
			std::string			topic_last;

		protected:
			virtual result_code on_message(sMESSAGE_KAFKA& _msg) override
			{
				SCOPED_LOCK(this->lock)
				{
					this->messages.emplace_back(std::string(_msg.buf_key.data<char>(), _msg.buf_key.size()), std::string(_msg.buf_message.data<char>(), _msg.buf_message.size()));
					this->topic_last = std::string(_msg.topic);
				}

				++this->count_received;

				return eRESULT::DONE;
			}
		};

		// - a producer on a mock cluster and the bootstrap servers of the cluster
		struct sMOCK
		{
			object_ptr<test_producer> pproducer;
			std::string			bootstrap_servers;
		};

		static sMOCK start_mock_producer(std::string_view _topic = TEST_TOPIC)
		{
			sMOCK mock;

			// 1) the producer starts the mock cluster ("test.mock.num.brokers")
			mock.pproducer = make_object<test_producer>();

			sKAFKA_PRODUCER_PARAMETER parameter;
			parameter.topic = _topic;
			parameter.enable_progress_log = false;
			parameter.properties.emplace_back("test.mock.num.brokers", "1");
			parameter.properties.emplace_back("linger.ms", "5");

			mock.pproducer->start(parameter);

			// 2) the address of the mock cluster and the topic
			auto* pcluster = rd_kafka_handle_mock_cluster(mock.pproducer->native_handle()->c_ptr());

			mock.bootstrap_servers = rd_kafka_mock_cluster_bootstraps(pcluster);
			rd_kafka_mock_topic_create(pcluster, std::string(_topic).c_str(), 1, 1);

			return mock;
		}

		static shared_buffer make_buffer(std::string_view _string)
		{
			auto buf = alloc_shared_buffer(_string.size());
			memcpy(buf.data<char>(), _string.data(), _string.size());
			buf ^= _string.size();
			return buf;
		}
	}

	// 1) the parameters are read from a context (producer)
	TEST(CGDK10_kafka, producer_parameter_from_context)
	{
		context context_setting;
		context_setting["bootstrap_servers"] = "127.0.0.1:9092";
		context_setting["client_id"] = "unit_test";
		context_setting["topic"] = "topic_a";
		context_setting["flush_timeout"] = 1234;
		context_setting["properties"]["linger.ms"] = 5;
		context_setting["properties"]["acks"] = "all";

		auto parameter = get_parameter_kafka_producer(context_setting, false);

		EXPECT_EQ(parameter.bootstrap_servers, "127.0.0.1:9092");
		EXPECT_EQ(parameter.client_id, "unit_test");
		EXPECT_EQ(parameter.topic, "topic_a");
		EXPECT_EQ(std::chrono::duration_cast<std::chrono::milliseconds>(parameter.flush_timeout).count(), 1234);

		auto find_property = [&](std::string_view _name) { auto iter = std::find_if(parameter.properties.begin(), parameter.properties.end(), [&](auto& _p) { return _p.first == _name; }); return (iter != parameter.properties.end()) ? iter->second : std::string(); };

		EXPECT_EQ(find_property("linger.ms"), "5");
		EXPECT_EQ(find_property("acks"), "all");
	}

	// 2) the parameters are read from a context (consumer: "topics" as "a,b")
	TEST(CGDK10_kafka, consumer_parameter_from_context)
	{
		context context_setting;
		context_setting["bootstrap.servers"] = "127.0.0.1:9092";
		context_setting["group_id"] = "group_a";
		context_setting["topics"] = "topic_a,topic_b";
		context_setting["auto_offset_reset"] = "earliest";

		auto parameter = get_parameter_kafka_consumer(context_setting, false);

		EXPECT_EQ(parameter.bootstrap_servers, "127.0.0.1:9092");
		EXPECT_EQ(parameter.group_id, "group_a");
		EXPECT_EQ(parameter.auto_offset_reset, "earliest");
		ASSERT_EQ(parameter.topics.size(), 2u);
		EXPECT_EQ(parameter.topics[0], "topic_a");
		EXPECT_EQ(parameter.topics[1], "topic_b");
	}

	// 3) before start(): produce() fails and subscribe() is not ready. nothing crashes
	TEST(CGDK10_kafka, not_started)
	{
		auto pproducer = make_object<test_producer>();
		EXPECT_FALSE(pproducer->produce(make_buffer("not started")));
		EXPECT_EQ(pproducer->pending_count(), 0u);

		auto pconsumer = make_object<test_consumer>();
		EXPECT_TRUE(pconsumer->subscribe({ std::string(TEST_TOPIC) }) == eRESULT::NOT_READY);
	}

	// 4) start/stop on the mock cluster: the parameters are set and the handle is released on stop()
	TEST(CGDK10_kafka, producer_start_stop_mock)
	{
		auto mock = start_mock_producer();

		EXPECT_FALSE(mock.bootstrap_servers.empty());
		EXPECT_EQ(mock.pproducer->default_topic(), TEST_TOPIC);
		EXPECT_TRUE(mock.pproducer->native_handle() != nullptr);

		mock.pproducer->stop();

		EXPECT_TRUE(mock.pproducer->native_handle() == nullptr);
		EXPECT_FALSE(mock.pproducer->produce(make_buffer("after stop")));
	}

	// 5) produce: every message is delivered and reported by on_delivery(). nothing is pending after flush()
	TEST(CGDK10_kafka, produce_delivery_report)
	{
		auto mock = start_mock_producer();

		for (int i = 0; i < 10; ++i)
			EXPECT_TRUE(mock.pproducer->produce(make_buffer("message " + std::to_string(i))));

		EXPECT_TRUE(mock.pproducer->flush(30s) == eRESULT::SUCCESS);
		EXPECT_TRUE(wait_until_true([&] { return mock.pproducer->count_success == 10; }, 30s));
		EXPECT_EQ(mock.pproducer->count_fail, 0);
		EXPECT_EQ(mock.pproducer->pending_count(), 0u);

		mock.pproducer->stop();
	}

	// 6) a consumer on the mock cluster receives the produced messages with their key, value and topic
	TEST(CGDK10_kafka, consumer_receives_messages)
	{
		auto mock = start_mock_producer();

		// 1) consumer
		auto pconsumer = make_object<test_consumer>();

		sKAFKA_CONSUMER_PARAMETER parameter{ mock.bootstrap_servers, "unit_test_group", { std::string(TEST_TOPIC) } };
		parameter.auto_offset_reset = "earliest";
		parameter.enable_progress_log = false;

		pconsumer->start(parameter);

		ASSERT_EQ(pconsumer->subscription().size(), 1u);
		EXPECT_EQ(pconsumer->group_id(), "unit_test_group");

		// 2) produce (default topic / with a key / produce_to / operator <<)
		std::string key = "key_1";
		EXPECT_TRUE(mock.pproducer->produce(make_buffer("value_1"), buffer_view(key.data(), key.size())));
		EXPECT_TRUE(mock.pproducer->produce_to(TEST_TOPIC, make_buffer("value_2")));
		*mock.pproducer << make_buffer("value_3");

		// 3) received (the group join of the mock cluster can take a few seconds)
		ASSERT_TRUE(wait_until_true([&] { return pconsumer->count_received >= 3; }, 60s));

		SCOPED_LOCK(pconsumer->lock)
		{
			EXPECT_EQ(pconsumer->messages.size(), 3u);
			EXPECT_EQ(pconsumer->topic_last, TEST_TOPIC);

			auto has = [&](std::string_view _key, std::string_view _value) { return std::any_of(pconsumer->messages.begin(), pconsumer->messages.end(), [&](auto& _m) { return _m.first == _key && _m.second == _value; }); };

			EXPECT_TRUE(has("key_1", "value_1"));
			EXPECT_TRUE(has("", "value_2"));
			EXPECT_TRUE(has("", "value_3"));
		}

		EXPECT_TRUE(wait_until_true([&] { return mock.pproducer->count_success == 3; }, 30s));

		// 4) stop
		pconsumer->stop();
		mock.pproducer->stop();
	}
}
