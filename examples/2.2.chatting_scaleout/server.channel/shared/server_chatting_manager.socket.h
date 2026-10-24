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
//-----------------------------------------------------------------------------
//
// server_login::socket
//
//-----------------------------------------------------------------------------
class server_channel_manager::socket :
	public						net::socket::tcp<>,
	public						net::io::connect_requestable::Ntcp_reconnect,
	public						Nmessage_transmitter,
	public						schedulable::Nexecutable
{
public:
			void				request_account_checkin(const ACCOUNT_ID& _account_id, user::socket* _pSource);
			void				request_account_checkout(const ACCOUNT_ID& _account_id);
			void				cancel_account_checkin(uint32_t _transaction_id);

private:
	virtual void				on_connect() override;
	virtual	void				on_disconnect(uint64_t _reason) override;
	virtual	void				on_execute() override;

	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_ACCEPT_CERTIFICATION(sMESSAGE_NETWORK& _msg);
			result_code			on_message_RESPONSE_ACCOUNT_CHECKIN(sMESSAGE_NETWORK& _msg);
	static	shared_buffer		message_REQUEST_ACCOUNT_CHECKIN(const ACCOUNT_ID& _account_id, uint32_t _transaction_id);
	static	shared_buffer		message_REQUEST_ACCOUNT_CHECKOUT(const ACCOUNT_ID& _account_id);

public:
			UINT				m_idx_list{0xffffffff};
			//depository<user::socket> m_request;
};
