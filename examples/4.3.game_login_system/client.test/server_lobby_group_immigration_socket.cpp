#include "StdAfx.h"


ServerLobby::CGroupImmigrationSocket::CGroupImmigrationSocket()
{
	EnableEnter();
}


ServerLobby::CGroupImmigrationSocket::~CGroupImmigrationSocket()
{
}

eOBJECT_RESULT ServerLobby::CGroupImmigrationSocket::OnMemberEntering(ServerLobby::CSocket* /*_pMember*/, CGMSG& /*_msg*/)
{
	return	eOBJECT_RESULT::SUCCESS;
}

void ServerLobby::CGroupImmigrationSocket::OnMemberEntered(ServerLobby::CSocket* _pMember, CGMSG& /*_msg*/) 
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);

	// 2) Request Check을 한다.
	_pMember->Send(_pMember->Message_REQUEST_ACCOUNT_CHECKIN());
}

uintptr_t ServerLobby::CGroupImmigrationSocket::OnMemberLeaving(ServerLobby::CSocket* /*_pMember*/) 
{
	return	0;
}

void ServerLobby::CGroupImmigrationSocket::OnMemberLeaved(ServerLobby::CSocket* _pMember, uintptr_t /*_Result*/)	
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

void ServerLobby::CGroupImmigrationSocket::OnExecute(DWORD /*p_dwParam*/)
{
}

BEGIN_CGMESSAGE_MAP(ServerLobby::CGroupImmigrationSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::RESPONSE_ACCOUNT_CHECKIN,			OnMessage_RESPONSE_ACCOUNT_CHECKIN, CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLobby::CGroupImmigrationSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message 처리를 하지 못했다!!!
	return 0;
}

int ServerLobby::CGroupImmigrationSocket::OnMessage_RESPONSE_ACCOUNT_CHECKIN(CGNETMSG& _msg)
{
	// 1) Socket
	CGPTR<ServerLobby::CSocket>	psocket	 = _msg.GetSource<ServerLobby::CSocket>();

	// Declare) 
	CCGBuffer	bufReceive	 = _msg.Buffer+8;

	// 2) 
	auto	idTransaction	 = bufReceive.extract<uint32_t>();
	auto	idAccount		 = bufReceive.extract<ACCOUNT_ID>();
	auto	result			 = bufReceive.extract<uint32_t>();

	// 3-A) 성공!
	if(result==0)
	{
	}
	// 3-2) 실패!
	else
	{
	}

	// 4) 
	auto	pSocketLoginServer	 = psocket->m_psocketLoginServer;
	psocket->m_psocketLoginServer.reset();

	// 5) LoginServer와의 접속을 끊는다.
	if(pSocketLoginServer.exist())
	{
		pSocketLoginServer->CloseSocket();
	}

	// Return) 성공!!!
	return	1;
}
