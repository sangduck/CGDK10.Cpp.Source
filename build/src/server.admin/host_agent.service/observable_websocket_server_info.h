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
// observable_websocket_server_info
//
//
// ----------------------------------------------------------------------------
class observable_websocket_server_info : 
	public						Iobservable_websocket
{
protected:
	explicit observable_websocket_server_info(const std::string_view _name);
	virtual ~observable_websocket_server_info();

public:
			void				notify_to_web_controls_INSTANCE_ENTER(server_admin::Iinstance* _pservice_entity);
			void				notify_to_web_controls_INSTANCE_LEAVE(server_admin::Iinstance* _pservice_entity);
			void				notify_to_web_controls_INSTANCE_STATUS_UPDATE(server_admin::Iinstance* _pservice_entity);
			void				notify_to_web_controls_INSTANCE_STATUS_UPDATE();
private:
	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);
};