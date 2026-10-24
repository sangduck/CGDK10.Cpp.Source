//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket kafka classes                       *
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

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// message
//
//-----------------------------------------------------------------------------
namespace eMESSAGE::NETWORK::KAFKA
{
	constexpr static message_t	BASE					 = (CODE_TYPE::NETWORK | CODE_TYPE::TAIL_8);

	// 1) a consumed message (sMESSAGE_KAFKA)
	constexpr static message_t	MESSAGE					 = BASE + 1;
}

//-----------------------------------------------------------------------------
// sMESSAGE_KAFKA (a consumed message)
//
//  - 'buf_message' is the value of the message. (not copied. the message is kept
//    alive while 'buf_message' is referenced. it can be kept after stop())
//  - 'topic', 'buf_key' and 'pnative' are valid only in on_message().
//-----------------------------------------------------------------------------
struct sMESSAGE_KAFKA : public sMESSAGE_NETWORK
{
public:
	template<class TSOURCE>
	sMESSAGE_KAFKA(TSOURCE _psource, const shared_buffer& _buffer) noexcept : sMESSAGE_NETWORK(eMESSAGE::NETWORK::KAFKA::MESSAGE, _psource, _buffer) {}

public:
			std::string_view	topic;
			int32_t				partition{ -1 };
			int64_t				offset{ -1 };
			int64_t				timestamp{ -1 };		// milliseconds since epoch (-1: not available)
			buffer_view			buf_key;
			RdKafka::Message*	pnative{ nullptr };
};

//-----------------------------------------------------------------------------
// sKAFKA_DELIVERY (the delivery report of a produced message)
//
//  - all members are valid only in on_delivery().
//-----------------------------------------------------------------------------
struct sKAFKA_DELIVERY
{
public:
			result_code			result;					// eRESULT::SUCCESS or eRESULT::FAIL
			int					error_code{ 0 };		// RdKafka::ErrorCode
			std::string_view	reason;
			std::string_view	topic;
			int32_t				partition{ -1 };
			int64_t				offset{ -1 };
			buffer_view			buf_key;
			buffer_view			buf_message;
};


//-----------------------------------------------------------------------------
//
// start parameters
//
//-----------------------------------------------------------------------------
// 1) common
struct sKAFKA_CLIENT_PARAMETER
{
public:
			std::string			bootstrap_servers;						// "bootstrap.servers" (empty: "localhost:9092")
			std::string			client_id;								// "client.id"
			std::vector<std::pair<std::string, std::string>> properties;// other librdkafka properties ("linger.ms", "acks", "security.protocol", ...)
			chrono::tick::duration poll_interval{ std::chrono::milliseconds{100} };	// max blocking time of a poll
			bool				enable_progress_log{ true };
};

// 2) producer
struct sKAFKA_PRODUCER_PARAMETER : public sKAFKA_CLIENT_PARAMETER
{
public:
			std::string			topic;									// default topic of produce()
			chrono::tick::duration flush_timeout{ std::chrono::seconds{10} };	// max waiting time for the remained messages on stop()
			bool				block_on_queue_full{ false };			// true: produce() blocks while the local queue is full (false: fails)

public:
	sKAFKA_PRODUCER_PARAMETER() {}
	sKAFKA_PRODUCER_PARAMETER(std::string_view _bootstrap_servers, std::string_view _topic = std::string_view()) { bootstrap_servers = _bootstrap_servers; topic = _topic; }
};

// 3) consumer
struct sKAFKA_CONSUMER_PARAMETER : public sKAFKA_CLIENT_PARAMETER
{
public:
			std::string			group_id;								// "group.id" (required)
			std::vector<std::string> topics;							// topics to subscribe
			std::string			auto_offset_reset;						// "auto.offset.reset" ("earliest", "latest", ...)

public:
	sKAFKA_CONSUMER_PARAMETER() {}
	sKAFKA_CONSUMER_PARAMETER(std::string_view _bootstrap_servers, std::string_view _group_id, std::vector<std::string> _topics = {}) : topics(std::move(_topics)) { bootstrap_servers = _bootstrap_servers; group_id = _group_id; }
};

// 4) gets the parameters from a context
void get_parameter_kafka_client(const context& _context, sKAFKA_CLIENT_PARAMETER& _parameter, bool _enable_progress_log = true);
sKAFKA_PRODUCER_PARAMETER get_parameter_kafka_producer(const context& _context, bool _enable_progress_log = true);
sKAFKA_CONSUMER_PARAMETER get_parameter_kafka_consumer(const context& _context, bool _enable_progress_log = true);


}
