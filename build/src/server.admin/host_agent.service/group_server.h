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
// group_server
//
// ----------------------------------------------------------------------------
class group_server : public ::Igroup
{
private:
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroy() override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;

	virtual	result_code			on_member_entering(socket_peer* _pmember, sMESSAGE& _param);
	virtual	void				on_member_entered(socket_peer* _pmember, sMESSAGE& _param);
	virtual	uintptr_t			on_member_leaving(socket_peer* _pmember, uintptr_t _param);
	virtual	void				on_member_leaved(socket_peer* _pmember, uintptr_t _result);

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_passby_to_instance(sMESSAGE_NETWORK& _msg);
};
