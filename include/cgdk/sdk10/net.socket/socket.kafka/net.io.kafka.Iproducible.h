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
// net::io::kafka::Iproducible (cf. net::io::Isendable)
//
// 1. net::io::kafka::Iproducible
//    1) defines the low level process of producing a message.
//    2) process_produce() requests to produce a message.
//       '_powner' is held until the delivery of the message is reported and the
//       message is not copied. (empty '_powner': the message is copied)
//    3) process_complete_produce() is called when the delivery of the message
//       is reported.
//
//-----------------------------------------------------------------------------
class net::io::kafka::Iproducible : virtual public Ireferenceable
{
public:
	//! @brief any partition (decided by the partitioner of librdkafka)
	static constexpr int32_t	PARTITION_ANY = -1;

public:
	//! @brief requests to produce a message. @param _topic topic (empty: the default topic) @param _partition partition (PARTITION_ANY: any) @param _buf_key key @param _buf_message value @param _powner the object which holds '_buf_message' (empty: '_buf_message' is copied) @return true success @return false failure
	virtual	bool				process_produce(std::string_view _topic, int32_t _partition, buffer_view _buf_key, buffer_view _buf_message, object_ptr<Ireferenceable>&& _powner) PURE;

	//! @brief called when the delivery of a message is reported.
	virtual	void				process_complete_produce(sKAFKA_DELIVERY& _delivery) PURE;
};


}
