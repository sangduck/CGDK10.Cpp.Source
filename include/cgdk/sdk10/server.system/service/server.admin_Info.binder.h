//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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
namespace server_system
{
namespace admin_info
{
// ----------------------------------------------------------------------------
//
// server_system
//
// 1. 
//    1) 
//    2) 
//
//
// ----------------------------------------------------------------------------
class binder
{
// constructor/destructor)
public:
			binder();
	virtual ~binder() noexcept;

// implementation)
public:
			server_system::socket_info m_socket_info;
			server_system::socket_status m_socket_status;
};


}
}
}