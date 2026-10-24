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
// observable_websocket_peer_network_info
//
//
// ----------------------------------------------------------------------------
class observable_websocket_peer_network_info :
	public						Iobservable_websocket
{
protected:
			observable_websocket_peer_network_info();
	virtual ~observable_websocket_peer_network_info();

public:
			void				request_update_webpage_connective(uint64_t _flag = 0xffffffff);
			void				request_update_webpage_binder(uint64_t _flag = 0xffffffff);
private:
	virtual	result_code			on_observer_registering(socket_websocket* _pobserver, observer_channel_t _ocid, observer_data_t& _observer_data) override;
	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;

public:
			server_system::instance_service* m_pservice_entity = nullptr;
};