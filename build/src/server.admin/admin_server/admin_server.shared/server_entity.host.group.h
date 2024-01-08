//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ⓒ2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server::group
//
//-----------------------------------------------------------------------------
class server_entity::host::group :
	public						CGDK::group::list_sender<socket>,
	public						net::io::messageable::Nbase,
	public						Nmessage_transmitter,
	public						Nobject_state,
	public						Ninitializable,
	public						Nstartable,
	public						Nnameable
{
public:
			 group(std::string_view _name);
	virtual ~group();

private:
	// 1) initialize/destroy
	virtual	void				on_initializing				(context& _context) override;
	virtual	void				on_initialize				(context& _context) override;
	virtual	void				on_destroying				() override;
	virtual	void				on_destroy					() override;

	// 2) start/Stop
	virtual	void				on_starting					(context& _context) override;
	virtual	void				on_start					(context& _context) override;
	virtual	void				on_stopping					() override;
	virtual	void				on_stop						() override;

	// 3) Member Enter/Leave
	virtual	result_code			on_member_entering			(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_entered			(socket* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving			(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_leaved			(socket* _pmember, sMESSAGE& _msg, _In_ uintptr_t _result) override;

protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// 1) Message 처리 함수
	virtual result_code			on_message												(sMESSAGE_NETWORK& _msg) override;

			//int					on_message_SERVER_ADMIN_SEND_SERVER_SETTING				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_HOST_STATUS_UPDATE				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_UPDATE_SERVER_INFO				(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_REGISTER		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_UNREGISTER		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_ENTER			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_LEAVE			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_SETTING_UPDATE	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVER_INSTANCE_STATUS_UPDATE	(sMESSAGE_NETWORK& _msg);

			result_code			on_message_NETWORK_SOCKET_BINDER_REGISTER				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_UNREGISTER				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER			(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_SERVER_PID_UPDATE				(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_ERROR							(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_REQUEST_LOG						(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SYSTEM_LOG									(sMESSAGE_NETWORK& _msg);
};
