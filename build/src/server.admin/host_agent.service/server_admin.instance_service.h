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
// instance::instance
//
// ----------------------------------------------------------------------------
class server_system::instance_service : virtual public Iinstance
{
public:
			instance_service();
	virtual ~instance_service() noexcept;

	virtual	iid_t				get_iid() const { return this->m_instance.info.setting.iid; }
	virtual std::string			get_name() const { return this->m_instance.info.setting.name; }
	virtual int					get_instance_type() const override { return eINSTANCE_TYPE::SERVICE; }

	virtual	void				set_instance_info(server_system::sSERVICE&&);
	virtual	void				reset_instance_status_info();
	virtual	void				update_instance_info(server_system::sSERVICE_STATUS_UPDATE&&);
	virtual	eOBJECT_STATE		set_instance_state(eOBJECT_STATE _state) noexcept override;
	virtual	void				update_application_status() {}
	virtual	void				update_revision_info();

	virtual	bool				request_service_initialize() { return true; }
	virtual	bool				request_service_destroy() { return true; }
	virtual	bool				request_service_start(const std::string&) { return true; }
	virtual	bool				request_service_stop() { return true; }
	virtual	bool				request_service_continue() { return true; }
	virtual	bool				request_service_pause() { return true; }
	virtual	bool				request_service_kill() { return true; }
	virtual	bool				request_service_update() { return true; }
	virtual	bool				request_service_check_out() { return true; }
	virtual	bool				request_service_switch() { return true; }

			OBJ<server_system::service::instance> m_instance;
			std::vector<PROCESS::sSTATUS> m_vector_process_status;		// - process status history (sSERVICE::sSTATUS has only the current one)
			eOBJECT_STATE		m_state_applied{ eOBJECT_STATE::NONE };

			object_ptr<instance_host> m_pinstance;
			bool				m_is_preserve_start{ false };
			std::string			m_str_configure;

			DEFINE_BUFFER_SERIALIZE(m_instance)
private:
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_server_system_INSTANCE_UPDATE_STATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_server_system_INSTANCE_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_LIST_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_FACTORY_FACTORY_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_LIST_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_EXECUTOR_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg);
			result_code			on_message_DUMP(sMESSAGE_NETWORK& _msg);
			result_code			on_message_http_websocket_REQUEST(sMESSAGE_HTTP_WEBSOCKET& _msg);

	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;

			own_ptr<observable_websocket_peer_overview_info>	pobservable_websocket_peer_overview_info;
			own_ptr<observable_websocket_peer_factory_info>		pobservable_websocket_peer_factory_info;
			own_ptr<observable_websocket_peer_executor_info>	pobservable_websocket_peer_executor_info;
			own_ptr<observable_websocket_peer_network_info>		pobservable_websocket_peer_network_info;
			own_ptr<observable_websocket_peer_querier_info>		pobservable_websocket_peer_querier_info;
			own_ptr<observable_websocket_peer_crash_info>		pobservable_websocket_peer_crash_info;

			object_ptr<Iobservable_websocket> _get_pobservable_webpage(char _select) const noexcept;
};
