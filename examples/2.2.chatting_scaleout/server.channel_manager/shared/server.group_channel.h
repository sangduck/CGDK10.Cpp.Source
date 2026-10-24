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

class server::group_channel : public Igroup_list<socket>, public schedulable::Nexecutable
{
public:
			object_ptr<socket>	get_suitable_channel_server();

private:
	virtual	void				on_start (context& _context) override;
	virtual	void				on_stopping () override;
	virtual	void				on_member_entered(socket* _pmember, CGDK::sMESSAGE& param) override;
	virtual	void				on_member_leaved(socket* _pmember, uintptr_t _result) override;
	virtual	void				on_execute () override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SEND_LOBBYSERVER_STATUS(sMESSAGE_NETWORK& _msg);
			result_code			on_message_REQUEST_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& _msg);
			result_code			on_message_REQUEST_ACCOUNT_CHECKOUT(sMESSAGE_NETWORK& _msg);

	static	shared_buffer		message_RESPONSE_ACCOUNT_CHECKIN(int _result, uint32_t _transaction_id, const ACCOUNT_ID& _account_id);
};
