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
// net::kafka::producer (cf. net::socket::tcp_client)
//
//    1) kafka producer.
//    2) override on_delivery() to get the delivery reports.
//
//    ex) auto pproducer = make_object<net::kafka::producer>();
//        pproducer->start({ "localhost:9092", "test_topic" });
//        pproducer->produce(buf_message);
//        ...
//        pproducer->stop();
//
//-----------------------------------------------------------------------------
class net::kafka::producer :
	public						net::io::kafka::Nproducer
{
public:
			producer() noexcept {}
	virtual	~producer() noexcept {}
};


}
