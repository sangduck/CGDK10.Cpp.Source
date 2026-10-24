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
// group_peer_client
//
// 1. group_peer_client는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class group_peer_client : public ::Igroup
#ifdef _USE_SVN_CONTROL
	,virtual public				server_system::Isvn_notify
#endif
{
private:
	virtual	void				on_initializing(context& _context) override;
	virtual	void				on_initialize(context& _context) override;
	virtual	void				on_destroying() override;
	virtual	void				on_destroy() override;
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	void				on_stop() override;

	virtual	result_code			on_member_entering(socket_peer* _pmember, sMESSAGE& _param) override;
	virtual	void				on_member_entered(socket_peer* _pmember, sMESSAGE& _param) override;
	virtual	uintptr_t			on_member_leaving(socket_peer* _pmember, uintptr_t _param) override;
	virtual	void				on_member_leaved(socket_peer* _pmember, uintptr_t _result) override;

#ifdef _USE_SVN_CONTROL
	virtual	void				on_svn_notify(server_system::Isvn_command* _baton, const svn_wc_notify_t* _notify, apr_pool_t* _pool);
	virtual	void				on_svn_progress(apr_off_t _progress, apr_off_t _total, server_system::Isvn_command* _baton, apr_pool_t* _pool);
	virtual	svn_error_t*		on_svn_log(const char** _log_msg, const char** _tmp_file, const apr_array_header_t* _commit_items, server_system::Isvn_command* _baton, apr_pool_t* _pool);
	virtual	svn_error_t*		on_svn_conflict_resolver(svn_wc_conflict_result_t** _result, const svn_wc_conflict_description2_t* _description, server_system::Isvn_command* _baton, apr_pool_t* _result_pool, apr_pool_t* _scratch_pool);
	virtual	svn_error_t*		on_svn_cancel(server_system::Isvn_command* _cancel_baton);
	virtual	void				on_svn_complete(server_system::Isvn_command* _cancel_baton, const SVN_RESULT& _result);
#endif

private:
	virtual result_code			process_message(sMESSAGE& _msg) override;

			result_code			on_message_SERVER_HOST_SETTING(sMESSAGE_NETWORK& _msg);

			result_code			on_message_server_system_INSTANCE_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_INSTANCE_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_INSTANCE_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_VERSION_UPDATE(sMESSAGE_NETWORK& _msg);

			result_code			on_message_server_system_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_START_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_PAUSE_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_CONTINUE_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_KILL_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_REQUEST_LOG(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg);
public:
			shared_buffer		message_SERVER_TIME_EVENT_EVENT_LIST() const;
			shared_buffer		message_SERVER_TIME_EVENT_EVENT_REGISTER(time_event::Ientity* _pentity) const;
			shared_buffer		message_SERVER_TIME_EVENT_EVENT_UNREGISTER(time_event::Ientity* _pentity) const;
			shared_buffer		message_SERVER_TIME_EVENT_EVENT_EXECUTE(time_event::Ientity* _pentity) const;
			shared_buffer		message_server_system_SUCCESS(uint64_t _authority) const;
			shared_buffer		message_server_system_REQUEST_LOG(uint64_t _Index) const;
};

