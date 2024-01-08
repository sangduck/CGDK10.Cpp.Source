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
// observable_websocket_system_info
//
//
// ----------------------------------------------------------------------------
class observable_websocket_system_info : 
	public						observable::list_sender<socket_websocket>,
	public						object::Nnameable,
	public						object::Ninitializable_startable,
	public						Nmessage_transmitter,
	public						net::io::messageable::Nbase,
	public						schedulable::Nexecutable
{
protected:
	explicit observable_websocket_system_info(const std::string_view _name);
	virtual ~observable_websocket_system_info();

private:
	virtual	result_code			on_observer_registering(socket_websocket* _pobserver, observer_channel_t _ocid, observer_data_t& _observer_data) override;
	virtual	void				on_observer_registered(socket_websocket* _pobserver, observer_channel_t _ocid, const observer_data_t& _observer_data) override;
	virtual	uintptr_t			on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t _ocid) override;
	virtual	void				on_observer_unregistered(socket_websocket* _pobserver, observer_channel_t _ocid, uintptr_t _result) override;
	virtual	uintptr_t			on_observer_data_updating(socket_websocket* _pobserver, observer_channel_t _ocid, observer_data_t& _observer_data) override;
	virtual	void				on_observer_data_update(socket_websocket* _pobserver, observer_channel_t _ocid, uintptr_t _result) override;

	virtual	void				on_execute() override;
};