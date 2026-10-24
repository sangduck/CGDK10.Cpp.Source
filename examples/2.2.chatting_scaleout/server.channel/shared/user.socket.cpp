#include "pch.h"


void user::socket::on_connect()
{
	// 1) 처음 접속 시 일단 AccountID과 TransactionID를 Reset한다.
	this->m_transaction_id = 0;
	this->m_account_id.reset();

	// 2) Enter group user Immigration
	g_pservice->pgroup_user_immigration->enter(this);
}

void user::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) Leave from group
	this->leave_group();

	// 2) LogOut요청 혹은 Checkin요청 취소를 한다.
	if(this->m_account_id.valid())
	{
		g_pservice->psocket_server_channel_manager->request_account_checkout(this->m_account_id);
	}

	g_pservice->psocket_server_channel_manager->cancel_account_checkin(this->m_transaction_id);
}

result_code user::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)message is not processed
	return eRESULT::BYPASS;
}