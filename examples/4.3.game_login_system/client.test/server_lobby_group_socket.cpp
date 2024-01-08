#include "StdAfx.h"


ServerLobby::CGroupSocket::CGroupSocket()
{
}

ServerLobby::CGroupSocket::~CGroupSocket()
{
}

eOBJECT_RESULT ServerLobby::CGroupSocket::OnMemberEntering(ServerLobby::CSocket* /*_pMember*/, CGMSG& /*_msg*/)
{
	return	eOBJECT_RESULT::SUCCESS;
}

void ServerLobby::CGroupSocket::OnMemberEntered(ServerLobby::CSocket* _pMember, CGMSG& /*_msg*/) 
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);
}

uintptr_t ServerLobby::CGroupSocket::OnMemberLeaving(ServerLobby::CSocket* /*_pMember*/) 
{
	return	0;
}

void ServerLobby::CGroupSocket::OnMemberLeaved(ServerLobby::CSocket* _pMember, uintptr_t /*_Result*/)
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

void ServerLobby::CGroupSocket::OnExecute(DWORD /*p_dwParam*/)
{
}

BEGIN_CGMESSAGE_MAP(ServerLobby::CGroupSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLobby::CGroupSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message 처리를 하지 못했다!!!
	return 0;
}
