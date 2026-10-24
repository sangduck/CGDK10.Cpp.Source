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
// server_system::instance_host
//
// ----------------------------------------------------------------------------
class server_system::instance_host :	virtual public Iinstance
{
public:
	virtual	iid_t				get_iid() const override { return this->m_instance.info.setting.iid; }
	virtual std::string			get_name() const override { return this->m_instance.info.setting.name; }
	virtual int					get_instance_type() const override { return eINSTANCE_TYPE::HOST; }

			void				register_service_instance(object_ptr<server_system::instance_service>&& _pinstance);
			void				unregister_service_instance() noexcept;

			OBJ<server_system::host::instance> m_instance;

			DEFINE_BUFFER_SERIALIZE(m_instance)
private:
	virtual	eOBJECT_STATE		set_instance_state(eOBJECT_STATE _state) noexcept override;
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
};
