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
// net::kafka::consumer (cf. net::socket::tcp_client)
//
//    1) kafka consumer.
//    2) override on_message() to process the consumed messages.
//
//    ex) class foo : public net::kafka::consumer
//        {
//        protected:
//            virtual result_code on_message(sMESSAGE_KAFKA& _msg) override
//            {
//                // _msg.buf_message, _msg.buf_key, _msg.topic, _msg.partition, _msg.offset
//                return eRESULT::DONE;
//            }
//        };
//
//        auto pconsumer = make_object<foo>();
//        pconsumer->start({ "localhost:9092", "test_group", { "test_topic" } });
//        ...
//        pconsumer->stop();
//
//-----------------------------------------------------------------------------
class net::kafka::consumer :
	public						net::io::kafka::Nconsumer
{
public:
			consumer() noexcept {}
	virtual	~consumer() noexcept {}

protected:
	virtual result_code			on_message(sMESSAGE_KAFKA& /*_msg*/) override { return eRESULT::BYPASS; }
};


}
