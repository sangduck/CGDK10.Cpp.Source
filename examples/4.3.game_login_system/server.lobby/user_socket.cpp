#include "StdAfx.h"


User::CSocket::CSocket()
{
}

User::CSocket::~CSocket()
{
}

void User::CSocket::OnConnect(CGNet::IO::IConnective* /*_pConnective*/)
{
	// 1) Enter group User Immigration
	g_pservice->m_pgroupUserImmigration->EnterMember(this);
}

void User::CSocket::OnDisconnect(uint32_t /*_Reason*/)
{
	// 1) Leave from group
	LeaveGroup();
}

int User::CSocket::OnMessage(CGMSG& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return) 
	return 0;
}

