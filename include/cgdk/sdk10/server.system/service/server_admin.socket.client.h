//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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

namespace CGDK
{
// ----------------------------------------------------------------------------
//
// server_system::socket_to_admin (server -> admin)
//
// 
// ----------------------------------------------------------------------------
class server_system::socket::client : 
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						net::socket::tcp<>,
	public						Nmessage_transmitter,
	public						schedulable::Iexecutable,
	virtual public				Ilogger
{
public:
			client();
	virtual ~client();

protected:
	virtual void				on_connect() override;
	virtual void				on_disconnect(uint64_t _disconnect_reason) override;
	virtual result_code			on_message (sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_SERVER_ADMIN_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg);

	virtual result_code			process_message (sMESSAGE& _msg) override;
			result_code			process_message_SERVER_ADMIN_UPDATE_SERVICE_STATUS(sMESSAGE_NOTIFY& _msg);
			result_code			process_message_SERVER_ADMIN_EXCEPTION_DUMP (sMESSAGE_DUMP& _msg);
			result_code			process_message_FACTORY_REGISTER (sMESSAGE& _msg);
			result_code			process_message_FACTORY_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_FACTORY_UPDATE (sMESSAGE& _msg);
			result_code			process_message_THREAD_REGISTER (sMESSAGE& _msg);
			result_code			process_message_THREAD_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_EXECUTOR_REGISTER (sMESSAGE& _msg);
			result_code			process_message_EXECUTOR_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_EXECUTOR_UPDATE (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_CONNECTIVE_REGISTER (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_CONNECTIVE_UPDATE (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_BINDER_REGISTER (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_BINDER_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_NETWORK_SOCKET_BINDER_UPDATE (sMESSAGE& _msg);
			result_code			process_message_SERVER_TIME_EVENT_EVENT_REGISTER (sMESSAGE& _msg);
			result_code			process_message_SERVER_TIME_EVENT_EVENT_UNREGISTER (sMESSAGE& _msg);
			result_code			process_message_SERVER_TIME_EVENT_EVENT_UPDATE (sMESSAGE& _msg);
			result_code			process_message_SERVER_TIME_EVENT_EVENT_EXECUTE (sMESSAGE& _msg);
	virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) override;

			void				process_update_instance_info_status(service::instance_local* _pinstance, server_system::sSERVICE_STATUS_UPDATE&& _service_status_update);

			void				process_service_instance_prepare();
			void				process_certify_failed(eRESULT _result);
			void				process_certify_succeeded();
			void				process_start_service_instance_start(server_system::Xservice* _pservice);
			void				process_service_instance_close();
			auto				process_get_service_instance() const noexcept ->object_ptr<service::instance_local>;

	virtual	intptr_t			process_execute(intptr_t _return, size_t _param) override;

	mutable	lock_object			m_lockable_pservice;
			sSERVICE			m_remote_service_info;
			std::atomic<bool>	m_is_instance_certified{ false };	// - INSTANCE_CERTIFY_RESPONSE(SUCCESS) received on this connection
			bool				m_is_failure_counted{ false };		// - certify failure is counted once per connection
			int					m_count_certify_fail{ 0 };
			chrono::tick::duration m_reconnect_interval_base{ chrono::tick::duration::zero() };
			std::string			m_certify_key;				// - CERTIFY_REQUEST에 함께 보내는 key ('socket_to_admin'.'certify_key' or CGDK_ADMIN_CERTIFY_KEY)
			object_ptr<service::instance_local> m_pservice_instance;
			friend class		server_system::Xservice;

			std::map<server_system::iid_t, object_ptr<server_system::host::instance>> m_map_host_instance;
			std::map<server_system::iid_t, object_ptr<server_system::service::instance>> m_map_service_instance;
};


}
