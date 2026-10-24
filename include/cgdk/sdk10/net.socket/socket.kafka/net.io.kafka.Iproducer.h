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
// net::io::kafka::Iproducer (cf. net::io::Isender)
//
// 1. net::io::kafka::Iproducer
//    1) the interface to produce messages. (like send() of net::io::Isender)
//    2) produce(...)     produces to the default topic. ('topic' of the start parameter)
//       produce_to(...)  produces to the topic.
//    3) shared_buffer is not copied. (the buffer is held until its delivery is
//       reported) buffer_view is copied.
//
//    ex) pproducer->produce(buf_message);
//        pproducer->produce(buf_message, buffer_view{ key.data(), key.size() });
//        pproducer->produce_to("topic_a", buf_message);
//        *pproducer << buf_message;
//
//-----------------------------------------------------------------------------
class net::io::kafka::Iproducer : virtual public net::io::kafka::Iproducible
{
public:
	// 1) default topic
			bool				produce(const shared_buffer& _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(std::string_view(), PARTITION_ANY, _buf_key, _buf_message, object_ptr<Ireferenceable>(_buf_message.get_source().get())); }
			bool				produce(buffer_view _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(std::string_view(), PARTITION_ANY, _buf_key, _buf_message, object_ptr<Ireferenceable>()); }

	// 2) topic
			bool				produce_to(std::string_view _topic, const shared_buffer& _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(_topic, PARTITION_ANY, _buf_key, _buf_message, object_ptr<Ireferenceable>(_buf_message.get_source().get())); }
			bool				produce_to(std::string_view _topic, buffer_view _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(_topic, PARTITION_ANY, _buf_key, _buf_message, object_ptr<Ireferenceable>()); }

	// 3) topic + partition
			bool				produce_to(std::string_view _topic, int32_t _partition, const shared_buffer& _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(_topic, _partition, _buf_key, _buf_message, object_ptr<Ireferenceable>(_buf_message.get_source().get())); }
			bool				produce_to(std::string_view _topic, int32_t _partition, buffer_view _buf_message, buffer_view _buf_key = buffer_view()) { return this->process_produce(_topic, _partition, _buf_key, _buf_message, object_ptr<Ireferenceable>()); }

	// 4) operator
	template <class T>
			Iproducer&			operator << (const T& _rhs) { this->produce(_rhs); return *this; }
};


}
