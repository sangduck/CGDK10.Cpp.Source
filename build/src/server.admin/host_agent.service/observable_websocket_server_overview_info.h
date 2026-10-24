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
// observable_websocket_peer_overview_info
//
//
// ----------------------------------------------------------------------------
class observable_websocket_peer_overview_info : public Iobservable_websocket
{
protected:
			observable_websocket_peer_overview_info();
	virtual ~observable_websocket_peer_overview_info();

public:
			void				request_update_webpage_time_event();
			void				add_log_records(const circular_list<object_ptr<LOG_RECORD>>& _list_log_record);
private:
	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
			result_code			on_message_time_event(sMESSAGE_HTTP_WEBSOCKET& _msg);

			void				process_send_webmessage_log_count_update();

			bool				m_is_count_log_count_updating;
			std::array<size_t, MAX_LOG_TYPE> m_array_log_count_last;
public:
			server_system::instance_service* m_pservice_entity = nullptr;
};