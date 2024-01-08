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
// client::group
//
//-----------------------------------------------------------------------------
class client::app::group :
	public						CGDK::group::list_sender<socket>,
	public						net::io::messageable::Nbase,
	public						Nmessage_transmitter,
	public						Nobject_state,
	public						Ninitializable,
	public						Nstartable,
	public						Nnameable,
	public						schedulable::Nexecutable
{
public:
			 group(std::string_view _name);
	virtual ~group();

private:
	// 1) initialize/destroy
	virtual	void				on_initializing		(context& _context) override;
	virtual	void				on_initialize		(context& _context) override;
	virtual	void				on_destroying		() override;
	virtual	void				on_destroy			() override;

	// 2) start/Stop
	virtual	void				on_starting			(context& _context) override;
	virtual	void				on_start			(context& _context) override;
	virtual	void				on_stopping			() override;
	virtual	void				on_stop				() override;

	// 3) Member Enter/Leave
	virtual	result_code			on_member_entering	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_entered	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving	(socket* _pmember, sMESSAGE& _msg) override;
	virtual	void				on_member_leaved	(socket* _pmember, sMESSAGE& _msg, _In_ uintptr_t _result) override;

	// 4) Schedulable
	virtual	void				on_execute			() override;


protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// 1) Message 처리 함수
	virtual result_code			on_message		(sMESSAGE_NETWORK& _msg) override;

	// 2) Message별 처리 함수
			result_code			on_message_NOTIFY_SERVER_INIT_COMPLETE					(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_LIST_REQUEST							(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_SETTING_UPDATE						(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_INFO_UPDATE							(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_STATE_UPDATE							(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_CONTROL_SERVER_INIT_REQUEST 	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_SERVER_DESTROY_REQUEST	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_SERVER_START_REQUEST	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_SERVER_STOP_REQUEST		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_SERVER_KILL_REQUEST		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_MARKET_INITIALIZE_REQUEST(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_MARKET_START_REQUEST	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_MARKET_STOP_REQUEST		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_MARKET_DESTROY_REQUEST	(sMESSAGE_NETWORK& _msg);
		
			result_code			on_message_SERVER_ADMIN_CONTROL_CONFIRM_RESPONSE		(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_CONTROL_EVENT_REGISTER_REQUEST	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_EVENT_UNREGISTER_REQUEST(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONTROL_EVENT_UPDATE_REQUEST	(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_CONTROL_SETTING_REFRESH_REQUEST	(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_ADMIN_REQUEST_LOG						(sMESSAGE_NETWORK& _msg);

	// 3)
			shared_buffer		message_SERVER_ADMIN_REQUEST_LOG						(uint64_t _Index) const;
};
