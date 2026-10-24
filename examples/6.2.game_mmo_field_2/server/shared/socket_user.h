//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once

class socket_user :
	public net::socket::tcp<>,
	public Nmessage_transmitter,
	public Igroupable<socket_user>
{
public:
			void				set_account(object_ptr<account>&& _paccount);
			object_ptr<account>	reset_account() noexcept;
			object_ptr<account>	get_account() const noexcept;
			object_ptr<area::entity_player> get_area_entity() const noexcept;

private:
	virtual void				on_connect();
	virtual void				on_disconnect(uint64_t _disconnect_reason);
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg);

public:
			lock_object			m_cs_account;
			object_ptr<account>	m_paccount;
};