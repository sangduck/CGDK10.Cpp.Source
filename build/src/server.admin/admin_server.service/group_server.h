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
// 1. group_server는...
//     접속된 Server의 Socket을 관리하는 group이다.
//    내부적으로 Admin Client Socket을 관리하는 Admin Client group을 가지고 있다.
//
// ----------------------------------------------------------------------------
class group_server : 
// inherited classes)
	public						group::list_sender<socket_server>,
	public						Nnameable,
	public                      Nobject_startable,
	virtual public				Nmessage_transmitter_single,
	public						net::io::messageable::Nbase
{
// constructor/destructor)
protected:
	explicit group_server(const std::string_view _name);
	virtual ~group_server();

// public) 
public:
			void				update_server_status_info();
			object_ptr<server_entity> find_server	(const server_admin::sID& _id) const noexcept;
			object_ptr<server_entity> find_server	(uint64_t _idCode) const noexcept;

// framework) 
private:
	virtual	void				on_initialize		(context& _context) override;
	virtual	void				on_destroy			() override;
	virtual	void				on_start			(context& _context) override;
	virtual	void				on_stop				() override;

	virtual	result_code			on_member_entering	(socket_server* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_entered	(socket_server* _pmember, sMESSAGE& _msg);
	virtual	uintptr_t			on_member_leaving	(socket_server* _pmember, sMESSAGE& _msg);
	virtual	void				on_member_leaved	(socket_server* _pmember, sMESSAGE& _msg, uintptr_t _result);

// message) 
private:
	DECLARE_CGMESSAGE_MAP

	virtual result_code			on_message											(sMESSAGE_NETWORK& _msg);

			result_code			on_message_SEND_HOST_INFO							(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_SERVICE_STATE_UPDATE		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_LIST_UPDATE				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_REGISTER					(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_UNREGISTER				(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_LIST_UPDATE						(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_REGISTER						(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_UNREGISTER						(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SCHEDULABLE_LIST							(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_REGISTER			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_UNREGISTER			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_CONNECTIVE_INFO_UPDATE		(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE			(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SYSTEM_LOG								(sMESSAGE_NETWORK& _msg);

			result_code			on_message_DUMP					(sMESSAGE_NETWORK& _msg);

public:
			shared_buffer		message_SERVER_ADMIN_SERVER_LIST();
			shared_buffer		message_SERVER_ADMIN_SERVER_STATUS_UPDATE();
			shared_buffer		message_websocket_SERVER_ADMIN_SERVER_STATUS_UPDATE();

// implementation) 
private:
		void					set_server_register_info(socket_server* _pmember, server_entity* _pserver_setting);
		void					reset_server_register_info(socket_server* _pmember) noexcept;

			lockable<>			m_csLock;
	static	std::atomic<int>	m_valueIdentity;
};