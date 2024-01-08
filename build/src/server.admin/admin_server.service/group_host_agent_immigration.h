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
// group_server_immigration
//
// 1. group_server_immigration는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class group_host_agent_immigration :
// Inherited Classes).
	public						group::list_sender<Isocket_host_agent>,
	public						Nnameable,
	public                      Nobject_startable,
	virtual public				Nmessage_transmitter_single,
	public						net::io::messageable::Nbase
{
// constructor/destructor).
public:
	explicit group_host_agent_immigration(std::string_view _name);
	virtual	~group_host_agent_immigration();

// framework) 
private:
	// 1) initialize/Destry
	virtual	void				on_initializing		(context& _context) override;
	virtual	void				on_initialize		(context& _context) override;
	virtual	void				on_destroying		() override;
	virtual	void				on_destroy			() override;
	virtual	void				on_starting			(context& _context) override;
	virtual	void				on_start			(context& _context) override;
	virtual	void				on_stopping			() override;
	virtual	void				on_stop				() override;

	// 3) Member Enter관련.
	virtual	result_code			on_member_entering	(Isocket_host_agent* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_entered	(Isocket_host_agent* _pmember, sMESSAGE& _msg);
	virtual	uintptr_t			on_member_leaving	(Isocket_host_agent* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_leaved	(Isocket_host_agent* _pmember, sMESSAGE& _msg, uintptr_t _result);

private:
			eRESULT				process_resuest_login(std::string_view _str_id, std::string_view _str_password);

// message) 
private:
	DECLARE_CGMESSAGE_MAP

	virtual result_code			on_message									(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SEND_HOST_INFO		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SEND_CONTROL_INFO	(sMESSAGE_NETWORK& _msg);

			shared_buffer		message_SERVER_ADMIN_LOGIN_ACCEPT			(eRESULT _result);
};

