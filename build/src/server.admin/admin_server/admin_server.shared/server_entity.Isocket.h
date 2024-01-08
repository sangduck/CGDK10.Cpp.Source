//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 9.0 / Release 2017.01.03                        *
//*                                                                           *
//*                            Server Admin Server                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server_entity::host::socket
//
//-----------------------------------------------------------------------------
namespace server_entity
{

class Isocket :
	public						net::socket::tcp_buffered<>,
	public						Isocket_server,
	public						Nmessage_transmitter
{
public:
			Isocket() noexcept;
	virtual ~Isocket() noexcept;

public:
	lockable <std::vector<object_ptr<server_entity::instance>>> m_vector_server_instance;
};

}