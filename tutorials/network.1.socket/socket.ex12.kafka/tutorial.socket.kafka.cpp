//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         tutorials socket - kafka                          *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
//-----------------------------------------------------------------------------
//
// tutorial.socket.kafka
//
//  kafka producer와 consumer의 사용법
//
//  1. net::kafka::producer
//     - start(context) 혹은 start(sKAFKA_PRODUCER_PARAMETER)로 시작한다.
//     - produce()로 메시지를 보낸다. produce()는 바로 리턴하고 전송 결과는
//       on_delivery()로 전달된다.
//     - shared_buffer는 복사하지 않고 전송이 끝날 때까지 참조를 유지한다.
//       buffer_view는 복사해서 보낸다.
//
//  2. net::kafka::consumer
//     - on_message()를 재정의해 받은 메시지를 처리한다.
//     - start(context) 혹은 start(sKAFKA_CONSUMER_PARAMETER)로 시작한다.
//     - on_message()는 consumer의 poll thread에서 받은 순서대로 호출된다.
//
//  3. stop()으로 정지한다.
//     - 동작 중에는 poll thread가 객체의 참조를 가지고 있으므로 반드시 stop()을
//       호출해야 한다.
//
//  4. context 설정 (socket의 context 설정과 같은 방식이다)
//     - 객체의 이름이 있으면 context[이름] 아래의 설정을 사용한다.
//
//       context_setting["producer"]["bootstrap_servers"]       = "192.168.0.100:9092";
//       context_setting["producer"]["topic"]                   = "tutorial_topic";
//       context_setting["producer"]["properties"]["linger.ms"] = 5;	// librdkafka 설정
//
//  실행) tutorial.socket.kafka [bootstrap servers]
//     - bootstrap servers를 지정하지 않으면 librdkafka의 mock cluster(테스트용
//       내장 broker)를 사용한다.
//
//-----------------------------------------------------------------------------
#include "cgdk/sdk10/net.socket.kafka.h"

// - mock cluster (for the tutorial without a kafka broker)
#include <librdkafka/rdkafka.h>
#include <librdkafka/rdkafka_mock.h>

#include <iostream>

using namespace CGDK;


//-----------------------------------------------------------------------------
// 1. producer
//-----------------------------------------------------------------------------
class tutorial_producer : public net::kafka::producer
{
public:
	std::atomic<int>	count_delivered{ 0 };

protected:
	// (v) 메시지의 전송 결과가 전달되는 함수 (poll thread에서 호출된다)
	virtual void on_delivery(sKAFKA_DELIVERY& _delivery) override
	{
		// check) 전송 실패
		if (_delivery.result != eRESULT::SUCCESS)
		{
			std::cout << " @ fail to deliver ("sv << _delivery.reason << ")"sv << std::endl;
			return;
		}

		// - 전송 성공
		++this->count_delivered;
	}
};


//-----------------------------------------------------------------------------
// 2. consumer
//-----------------------------------------------------------------------------
class tutorial_consumer : public net::kafka::consumer
{
public:
	std::atomic<int>	count_received{ 0 };

protected:
	// (v) 메시지를 받았을 때 호출되는 함수 (poll thread에서 받은 순서대로 호출된다)
	virtual result_code on_message(sMESSAGE_KAFKA& _msg) override
	{
		// - _msg.buf_message : 메시지 (복사되지 않은 버퍼로 보관해 두었다가 나중에 사용해도 된다)
		// - _msg.buf_key     : key
		// - _msg.topic, _msg.partition, _msg.offset, _msg.timestamp
		std::cout << " @ received ["sv << _msg.topic << ":"sv << _msg.partition << ":"sv << _msg.offset << "] "sv
				  << "key='"sv << std::string_view(_msg.buf_key.data<char>(), _msg.buf_key.size()) << "' "sv
				  << "value='"sv << std::string_view(_msg.buf_message.data<char>(), _msg.buf_message.size()) << "'"sv << std::endl;

		++this->count_received;

		// return)
		return eRESULT::DONE;
	}
};


//-----------------------------------------------------------------------------
// helper) 문자열을 shared_buffer로
//-----------------------------------------------------------------------------
shared_buffer make_buffer(std::string_view _string)
{
	auto buf = alloc_shared_buffer(_string.size());
	memcpy(buf.data<char>(), _string.data(), _string.size());
	buf ^= _string.size();
	return buf;
}


int main(int argc, char* argv[])
{
	// trace)
	std::cout << "start [tut.socket.ex12.kafka]..."sv << std::endl;

	try
	{
		// ------------------------------------------------------------------
		// 1. producer - context로 시작하기
		// ------------------------------------------------------------------
		// 1) producer 객체를 생성한다.
		auto pproducer = make_object<tutorial_producer>();

		// 2) 이름을 설정한다. (context["producer"] 아래의 설정을 사용한다.)
		pproducer->name("producer");

		// 3) 설정
		context context_setting;

		if (argc >= 2)
		{
			// - kafka broker
			context_setting["producer"]["bootstrap_servers"] = argv[1];
		}
		else
		{
			// - mock cluster (librdkafka 내장 테스트용 broker)
			context_setting["producer"]["properties"]["test.mock.num.brokers"] = 1;
		}
		context_setting["producer"]["topic"]					= "tutorial_topic";	// produce()의 기본 topic
		context_setting["producer"]["properties"]["linger.ms"]	= 5;				// librdkafka 설정은 "properties"에

		// 4) 시작한다.
		pproducer->start(context_setting);

		// - mock cluster면 주소를 얻고 topic을 만든다.
		std::string bootstrap_servers = (argc >= 2) ? argv[1] : "";

		if (bootstrap_servers.empty())
		{
			auto* pmock_cluster = rd_kafka_handle_mock_cluster(pproducer->native_handle()->c_ptr());
			bootstrap_servers = rd_kafka_mock_cluster_bootstraps(pmock_cluster);
			rd_kafka_mock_topic_create(pmock_cluster, "tutorial_topic", 1, 1);

			std::cout << " - mock cluster: "sv << bootstrap_servers << std::endl;
		}


		// ------------------------------------------------------------------
		// 2. consumer - START_PARAMETER로 시작하기
		// ------------------------------------------------------------------
		// 1) consumer 객체를 생성한다.
		auto pconsumer = make_object<tutorial_consumer>();

		// 2) 설정 (bootstrap servers, consumer group, 구독할 topic)
		sKAFKA_CONSUMER_PARAMETER parameter{ bootstrap_servers, "tutorial_group", { "tutorial_topic" } };
		parameter.auto_offset_reset = "earliest";	// 처음부터 받는다.

		// 3) 시작한다.
		pconsumer->start(parameter);


		// ------------------------------------------------------------------
		// 3. produce
		// ------------------------------------------------------------------
		// 1) buffer_view - 복사해서 보낸다.
		std::string str_message = "hello kafka (buffer_view)";
		pproducer->produce(buffer_view(str_message.data(), str_message.size()));

		// 2) shared_buffer - 복사하지 않고 전송이 끝날 때까지 버퍼의 참조를 유지한다.
		pproducer->produce(make_buffer("hello kafka (shared_buffer)"));

		// 3) key - 같은 key의 메시지는 같은 partition으로 가서 순서가 유지된다.
		std::string str_key = "user_1";
		pproducer->produce(make_buffer("hello kafka (with key)"), buffer_view(str_key.data(), str_key.size()));

		// 4) produce_to - topic을 지정해서 보낸다.
		pproducer->produce_to("tutorial_topic", make_buffer("hello kafka (produce_to)"));

		// 5) operator <<
		*pproducer << make_buffer("hello kafka (operator <<)");


		// ------------------------------------------------------------------
		// 4. 받을 때까지 기다린다. (최대 30초)
		// ------------------------------------------------------------------
		for (int i = 0; i < 3000 && pconsumer->count_received < 5; ++i)
			std::this_thread::sleep_for(10ms);

		std::cout << " - delivered "sv << pproducer->count_delivered << ", received "sv << pconsumer->count_received << std::endl;


		// ------------------------------------------------------------------
		// 5. stop
		// ------------------------------------------------------------------
		pconsumer->stop();
		pproducer->stop();
	}
	catch (std::exception& _e)
	{
		std::cout << " @ exception ("sv << _e.what() << ")"sv << std::endl;
		return 1;
	}

	// trace)
	std::cout << "end [tut.socket.ex12.kafka]..."sv << std::endl;

	// return)
	return 0;
}
