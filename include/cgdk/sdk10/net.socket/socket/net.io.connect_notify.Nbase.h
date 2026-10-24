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
// net::io::connect_notify::Nbase
//
//    1) connect 결과를 알려주는 hook함수.
// 	  2) connect 혹은 disconnect 시 처리해야 될 내역을 정의할 때 사용된다.
//
//-----------------------------------------------------------------------------
class net::io::connect_notify::Nbase : virtual public net::io::Iconnect_notify
{
protected:
	virtual void				on_request_connect(const net::sockaddr& /*_address*/) override {};
	virtual void				on_connect() override {};
	virtual void				on_fail_connect(uint64_t /*_disconnect_reason*/) override {};
	virtual void				on_disconnect(uint64_t /*_disconnect_reason*/) override {};
};


}