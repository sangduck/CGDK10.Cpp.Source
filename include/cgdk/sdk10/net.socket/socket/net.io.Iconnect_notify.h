//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
// net::io::Iconnect_notify
//
// 1. net::io::Iconnect_notify은~
//    1) 데이터를 전송을 I/O함수를 사용하여 구현한 저수준 처리를 정의하는 함수.
//    2) process_send()는 데이터의 전송을 요청하는 처리 과정을 정의하는 함수.
//    3) process_complete_sendable()는 데이터의 전송이 완료되었을 때 호출되어 처리
//       과정을 정의하는 함수이다.
//
//-----------------------------------------------------------------------------
class net::io::Iconnect_notify : virtual public Ireferenceable
{
protected:
	virtual void				on_request_connect(const net::sockaddr& _address) PURE;
	virtual void				on_connect() PURE;
	virtual void				on_fail_connect(uint64_t _disconnect_reason) PURE;
	virtual void				on_disconnect(uint64_t _disconnect_reason) PURE;
};


}