//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                      tutorials query - kafka.simple                       *
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
//// * standard header file
//#include <string>
//#include <chrono>
//#include <thread>
//#ifdef _WIN32
//	#include <conio.h>
//#endif

#define CPPKAFKA_STATIC 1
#include <cppkafka/cppkafka.h>
#include <iostream>
//#include <process.h> // to use exit()

using namespace std;
using namespace cppkafka;

class test_producer
{
public:
    cppkafka::Producer producer;
};

// Producer 객체를 생성합니다.
cppkafka::Producer* producer{ nullptr};

int serial = 0;
bool is_run = true;
std::shared_ptr<std::thread> thread_produce;

void execute_producer()
{
    while (is_run)
    {
        auto message = std::string("Hello, Kafka! [") + std::to_string(serial) + "]";

        // 메시지를 생성하고 전송합니다.
        producer->produce(cppkafka::MessageBuilder("test_topic3").partition(0).payload(message));

        // 모든 메시지가 전송될 때까지 기다립니다.
        producer->flush();
        //producer->poll();

        std::this_thread::sleep_for(100ms);
    }
}

void kafka_producer()
{
    // Kafka 설정을 생성합니다.
    cppkafka::Configuration config = {
        { "metadata.broker.list", "192.168.0.66:9092" } // 브로커 주소를 설정합니다.
        //,{ "acks", "0" }
    };

    // Producer 객체를 생성합니다.
    producer = new cppkafka::Producer(config);

    thread_produce = std::make_shared<std::thread>(execute_producer);
}

void message_callback(Consumer& consumer, const Message& msg) {
    if (msg.get_error()) {
        if (!msg.is_eof()) {
            cerr << "[Error] " << msg.get_error().to_string() << endl;
        }

        auto temp_header = msg.get_header_list();
    }
    else {
        // 메시지 처리
        cout << "Message received: " << msg.get_payload() << endl;
        // 메시지 오프셋 커밋
        consumer.commit(msg);
    }
}

class test_consumer
{
public:
    test_consumer(const cppkafka::Configuration& _config) :consumer_(_config) {}
    cppkafka::Consumer consumer_;
    std::string topic_;

    void Start()
    {

    }
};

void kafka_consumer()
{
    // Kafka 설정을 생성합니다.
    cppkafka::Configuration config = {
         { "metadata.broker.list", "192.168.0.66:9092" } // 브로커 주소를 설정합니다.
        ,{ "group.id", "testx4" }                           // 컨슈머 그룹 ID를 설정합니다.
        ,{ "auto.offset.reset", "latest" }                  // 오프셋 리셋 정책을 설정합니다.
        ,{ "enable.auto.commit", true }                     //
    };

    // Consumer 객체를 생성합니다.
    test_consumer consumer(config);

    consumer.Start();

    // 구독할 토픽을 설정합니다.
    consumer.consumer_.subscribe({ "test_topic3" });

    // 메시지를 소비하는 루프를 시작합니다.
    while (true) {
        // 메시지를 가져옵니다.
        cppkafka::Message msg = consumer.consumer_.poll(10ms);
        if (msg) {
            if (!msg.get_error()) {
                // 메시지가 정상적으로 수신되었습니다.
                std::cout << "Message received: " << msg.get_payload() << endl;
            }
            else if (!msg.is_eof()) 
            {
                // 오류가 발생했습니다.
                std::cerr << "Error: " << msg.get_error().to_string() << endl;
            }
        }

        ++serial;
    }

    // unsubscribe
    consumer.consumer_.unsubscribe();
}

int main()
{
    kafka_producer();
    kafka_consumer();


    is_run = false;
    thread_produce->join();
    thread_produce.reset();

    return 0;
}

namespace cppkafka {
    void KafkaHandleBase::HandleDeleter::operator()(rd_kafka_t* handle) {
    #if RD_KAFKA_VERSION >= RD_KAFKA_DESTROY_FLAGS_SUPPORT_VERSION
        rd_kafka_destroy_flags(handle, handle_base_ptr_->get_destroy_flags());
    #else
        rd_kafka_destroy(handle);
    #endif
    }
}