#include "StdAfx.h"


ServerLobby::CSocket::CSocket() : 
	m_idxList(-1)
{
}

ServerLobby::CSocket::~CSocket()
{
}

void ServerLobby::CSocket::OnConnect(CGNet::IO::IConnective* /*_pConnective*/)
{
	// Trace) 
	CGLOG(g_pLog, eLOG_TYPE::INFO_, eLOG_LEVEL::HIGHER, _CGTEXT("> Lobby Server Connected [{}]"), CGNetAPI::AddressToString<_CHAR>(GetPeerAddress()).c_str());

	// View)
	g_pService->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_CONNECT, (DWORD_PTR)this));

	// 1) Enter groupUserImmigration
	g_pService->m_pgroupServerLobbyImmigration->EnterMember(this);
}

void ServerLobby::CSocket::OnDisconnect(uint32_t /*_Reason*/)
{
	// View)
	g_pService->NotifyMessage(CGMSG_NOTIFY(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_DISCONNECT, (DWORD_PTR)this));

	// 1) Leave from group
	LeaveGroup();

	// Trace)
	CGLOG(g_pLog, eLOG_TYPE::INFO_, eLOG_LEVEL::HIGHER, _CGTEXT("< Lobby Server Disconnected [{}]"), CGNetAPI::AddressToString<_CHAR>(GetPeerAddress()).c_str());
}

int ServerLobby::CSocket::OnMessage(CGMSG& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)
	return 0;
}
