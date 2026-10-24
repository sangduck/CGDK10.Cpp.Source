//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                Lobby Server                               *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr)                         *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once

class user::socket :
	public						net::socket::tcp<>,
	public						Igroupable<socket>,
	public						Nmessage_transmitter
{
private:
	virtual void				on_connect() override;
	virtual	void				on_disconnect(uint64_t _reason) override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

public:
			uint32_t			m_transaction_id;
			ACCOUNT_ID			m_account_id;
			int					m_idx_list;
};