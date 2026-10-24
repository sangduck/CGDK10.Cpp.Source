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
// observable_websocket_host_info
//
//
// ----------------------------------------------------------------------------
class observable_websocket_host_info : 
	public						Iobservable_websocket,
	virtual public				Ilogger
{
public:
			observable_websocket_host_info();
	virtual ~observable_websocket_host_info();

			void				notify_to_web_controls_INSTANCE_ENTER(server_system::Iinstance* _pInstance);
			void				notify_to_web_controls_INSTANCE_LEAVE(server_system::Iinstance* _pInstance);
			void				notify_to_web_controls_INSTANCE_STATUS_UPDATE(server_system::Iinstance* _pInstance, uint64_t _flag=0xffffffff);
			void				notify_to_web_controls_INSTANCE_STATUS_UPDATE();

			void				add_time_event(time_event::Ientity* _pentity);
			void				remove_time_event(time_event::Ientity* _pentity);
			void				execute_time_event(time_event::Ientity* _pentity);
			void				update_time_event(time_event::Ientity* _pentity);
			void				add_log_records(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record);
private:
	virtual	void				on_start(context& _context) override;
	virtual	void				on_stop() override;

	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_time_event(sMESSAGE_HTTP_WEBSOCKET& _msg);
			result_code			on_message_server_list(sMESSAGE_HTTP_WEBSOCKET& _msg);

	virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) override;
			void				process_send_logs(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record);
			void				process_send_webmessage_log_count_update();
			void				process_web_service_instance_list(net::io::Isender* _psender);

			bool				m_is_count_log_count_updating;
			std::array<size_t, MAX_LOG_TYPE> m_array_log_count_last;

};