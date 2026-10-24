#include "pch.h"


void user::socket::on_connect()
{
	// 1) 먼저 Account ID를 Reset한다.
	this->m_paccount.reset();

	// 2) Enter group user Immigration
	g_pservice->pgroup_user_immigration->enter(this);
}

void user::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) Leave from group
	this->leave_group();

	// 2) Check Out을 요청한다.
	g_pservice->pmanager_account->request_account_checkout(this);
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
