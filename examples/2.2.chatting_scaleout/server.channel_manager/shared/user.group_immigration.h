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

class user::group_immigration : public Igroup_list<socket>, public schedulable::Nexecutable
{
private:
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_member_entered(user::socket* _pmember, sMESSAGE& _param) override;
	virtual	void				on_member_leaved(user::socket* _pmember, uintptr_t _result) override;
	virtual	void				on_execute() override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_REQUEST_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& _msg);

public:
	static	shared_buffer		message_RESULT_ACCOUNT_CHECKIN_ACCEPT(const ACCOUNT_ID& _account_id, const SERVER_INFO& _rServerInfo);
	static	shared_buffer		message_RESULT_ACCOUNT_CHECKIN_REJECT(int _Error);
};
