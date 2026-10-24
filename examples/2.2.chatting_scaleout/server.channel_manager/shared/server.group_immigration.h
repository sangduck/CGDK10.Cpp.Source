//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Login Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class server::group_immigration : public Igroup_list<socket>, public schedulable::Nexecutable
{
private:
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_member_entered(socket* _pmember, sMESSAGE& _param) override;
	virtual	void				on_member_leaved(socket* _pmember, uintptr_t _result) override;
	virtual	void				on_execute() override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_REQUEST_CERTIFICATION(sMESSAGE_NETWORK& _msg);

	static	shared_buffer		message_ACCEPT_CERTICIATION();
};
