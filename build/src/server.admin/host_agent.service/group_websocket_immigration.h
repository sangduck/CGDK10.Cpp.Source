//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
// ----------------------------------------------------------------------------
//
// group_peer_immigration
//
// 1. group_peer_immigration는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class group_websocket_immigration :
	public						group::list_sender<socket_websocket>,
	public						object::Nnameable,
	public                      object::Ninitializable_startable,
	virtual public				Nmessage_transmitter_single,
	public						net::io::messageable::Nbase
{
private:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;

	virtual	result_code			on_member_entering(socket_websocket* _pmember, sMESSAGE& _param);
	virtual	void				on_member_entered(socket_websocket* _pmember, sMESSAGE& _param);
	virtual	uintptr_t			on_member_leaving(socket_websocket* _pmember, uintptr_t _param);
	virtual	void				on_member_leaved(socket_websocket* _pmember, uintptr_t _result);

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
};

