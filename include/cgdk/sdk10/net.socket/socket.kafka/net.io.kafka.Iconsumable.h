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
// net::io::kafka::Iconsumable (cf. net::io::Ireceivable_stream)
//
// 1. net::io::kafka::Iconsumable
//    1) defines the low level process of a consumed message.
//    2) process_consume() is called with each consumed message on the poll
//       thread of the consumer. (the ownership of the message is passed)
//
//-----------------------------------------------------------------------------
class net::io::kafka::Iconsumable : virtual public Ireferenceable
{
public:
	//! @brief called with a consumed message. (the ownership of '_pmessage' is passed)
	virtual	void				process_consume(std::unique_ptr<RdKafka::Message>&& _pmessage) PURE;
};


}
