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
// ----------------------------------------------------------------------------
//
// service_ex
//
// 
// ----------------------------------------------------------------------------
class server_system::Iservice_component : virtual public Ireferenceable
{
public:
	virtual void				on_service_complete_initialize(context& /*_context*/) {}
	virtual void				on_service_complete_start(context& /*_context*/) {}
	virtual void				on_service_complete_stop() {}
	virtual void				on_service_complete_destroy() {}
};


}