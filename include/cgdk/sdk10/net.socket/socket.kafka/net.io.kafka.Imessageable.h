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
// net::io::kafka::Imessageable (cf. net::io::Imessageable)
//
// 1. net::io::kafka::Imessageable
//    1) process_message() passes a consumed message (sMESSAGE_KAFKA) to
//       on_message().
//    2) override on_message() to process the consumed messages.
//
//-----------------------------------------------------------------------------
class net::io::kafka::Imessageable :
	virtual public				CGDK::Imessageable
{
public:
	virtual ~Imessageable() noexcept {}

protected:
	virtual result_code			on_message(sMESSAGE_KAFKA& _msg) PURE;

protected:
	virtual	result_code			process_message(sMESSAGE& _msg) override
	{
		// check) only consumed messages
		RETURN_IF(_msg.message != eMESSAGE::NETWORK::KAFKA::MESSAGE, result_code(eRESULT::BYPASS));

		// 1) call 'on_message'
		return this->on_message(static_cast<sMESSAGE_KAFKA&>(_msg));
	}
};


}
