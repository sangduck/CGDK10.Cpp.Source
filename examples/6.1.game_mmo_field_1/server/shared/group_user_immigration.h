//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
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

class group_user_immigration :
	public Igroup_user,
	public	schedulable::Nexecutable
{
private:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	result_code			on_member_entering(socket_user* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving(socket_user* _pmember, uintptr_t _param) override;
	virtual	void				on_execute() override;
	virtual	result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_USER_LOGIN_REQUEST(sMESSAGE_NETWORK& _msg);

			void				process_disconnect_timeout_sockets();
};
