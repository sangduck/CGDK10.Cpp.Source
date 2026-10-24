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
// net::socket::tcp_crypt
//
//  1) 암호송수신 기능을 가진 클라이언트용 tcp socket 이다.
//
//-----------------------------------------------------------------------------
template <class RECEIVE_HEADER_T = uint32_t, class SEND_HEADER_T = RECEIVE_HEADER_T>
class net::socket::tcp_crypt :
	public						net::io::socket::tcp::Ndefault,
	public						net::io::connect_notify::tcp::Ndefault,
	public						net::io::Ntcp_crypt,
	public						net::io::send_request::tcp::Ndefault,
	public						net::io::sender::tcp::Ndefault<SEND_HEADER_T>,
	public						net::io::packetable::Nstream<RECEIVE_HEADER_T>
{
	virtual result_code			on_message(sMESSAGE_NETWORK& /*_msg*/) override { return eRESULT::BYPASS; }
};


}