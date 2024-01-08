#include "StdAfx.h"


ServerLogin::CGroupImmigrationSocket::CGroupImmigrationSocket()
{
	EnableEnter();
}


ServerLogin::CGroupImmigrationSocket::~CGroupImmigrationSocket()
{
}

eOBJECT_RESULT ServerLogin::CGroupImmigrationSocket::OnMemberEntering(ServerLogin::CSocket* /*_pMember*/, CGMSG& /*_msg*/)
{
	return	eOBJECT_RESULT::SUCCESS;
}

void ServerLogin::CGroupImmigrationSocket::OnMemberEntered(ServerLogin::CSocket* _pMember, CGMSG& /*_msg*/)
{
	// 1) Register Mediator한다.
	_pMember->RegisterMessageable(this);

	// 2) Client의 정보를 전송한다.
	_pMember->Send(Message_REQUEST_ACCOUNT_LOGIN(L"TestID", L"TestPassword"));
}

uintptr_t ServerLogin::CGroupImmigrationSocket::OnMemberLeaving(ServerLogin::CSocket* /*_pMember*/) 
{
	return	0;
}

void ServerLogin::CGroupImmigrationSocket::OnMemberLeaved(ServerLogin::CSocket* _pMember, uintptr_t /*_Result*/)	
{
	// 1) Unregister Mediator한다.
	_pMember->UnregisterMessageable(this);
}

void ServerLogin::CGroupImmigrationSocket::OnExecute(DWORD /*p_dwParam*/)
{
}

BEGIN_CGMESSAGE_MAP(ServerLogin::CGroupImmigrationSocket)
	CGMESSAGE_TYPE_SUB(CGMESSAGE::SYSTEM::MESSAGE, ((CGMSG_BUFFER&)_msg).Buffer.front<uint32_t>(4))
		ON_CGMESSAGE(CGMESSAGE::RESPONSE_ACCOUNT_LOGIN,		OnMessage_RESPONSE_ACCOUNT_LOGIN, CGNETMSG);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

int ServerLogin::CGroupImmigrationSocket::OnMessage(CGMSG& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// Return) Message 처리를 하지 못했다!!!
	return 0;
}


int ServerLogin::CGroupImmigrationSocket::OnMessage_RESPONSE_ACCOUNT_LOGIN(CGNETMSG& _msg)
{
	//const uint32_t	SIZE_OF_MESSAGE	 = 8 + sizeof(eOBJECT_RESULT) + sizeof(CHECKIN_ID);

	//// 1) Message의 크기를 검사한다.
	//CGASSERT_ERROR(_msg.Buffer.len == SIZE_OF_MESSAGE);

	//// 2) Message의 크기를 검사한다.
	//THROW_IF(_msg.Buffer.len != SIZE_OF_MESSAGE, throwable::failure(eRESULT::FAIL));

	// 1) Socket
	auto		pSocket		 = _msg.GetSource<ServerLogin::CSocket>();
	CCGBuffer	bufReceive	 = _msg.Buffer+8;

	// 2) Server Info를 얻는다.
	auto		result		 = bufReceive.extract<int>();

	// Case) 성공일 경우
	if(result==0)
	{
		// 3) Message에서 값을 읽어낸다.
		auto&	idAccount			 = bufReceive.extract<ACCOUNT_ID>();
		auto&	addrServerLobby		 = bufReceive.extract<SOCKADDR_IN>();

		// 4) Account 정보를 저장한다.
		auto	pConnectable		 = CSampleSocketClientTCPEchoDlg::m_pconnectorSerbverLobby->AllocConnectable();
		auto	psocketServerLobby	 = dynamic_pointer_cast<ServerLobby::CSocket>(pConnectable);

		// 5) ACCOUNT_ID는 저장해 놓는다.
		psocketServerLobby->m_idAccount			 = idAccount;
		psocketServerLobby->m_psocketLoginServer = pSocket;

		// 6) Lobby Server에 접속을 시도한다.
		CSampleSocketClientTCPEchoDlg::m_pconnectorSerbverLobby->Connecting(psocketServerLobby, (LPCSOCKADDR)&addrServerLobby, sizeof(SOCKADDR_IN));
	}
	// Case) 실패일 경우
	else
	{
		// 접속을 끊는다.
		pSocket->CloseSocket();
	}

	// Return) 
	return	1;
}

CCGBuffer ServerLogin::CGroupImmigrationSocket::Message_REQUEST_ACCOUNT_LOGIN(const wchar_t* _strName, const wchar_t* _strPassword)
{
	// 1) Buffer를 할당받는다.
	CCGBuffer	tempBuffer	 = MEM_POOL_ALLOC(4096);

	//// 2) Message Head
	tempBuffer.append<uint32_t>();
	tempBuffer.append<uint32_t>(CGMESSAGE::REQUEST_ACCOUNT_LOGIN);
	tempBuffer.append<int>(0);
	tempBuffer.append(_strName);
	tempBuffer.append(_strPassword);

	// 3) Message의 길이를 설정한다.
	tempBuffer.front<uint32_t>()	 = tempBuffer.len;

	// Return)
	return	tempBuffer;
}

CCGBuffer ServerLogin::CGroupImmigrationSocket::Message_REQUEST_ACCOUNT_LOGIN(const _CHAR* _DeviceID)
{
	// 1) Buffer를 할당받는다.
	CCGBuffer	tempBuffer = MEM_POOL_ALLOC(4096);

	// 2) Message Head
	tempBuffer.append<uint32_t>();
	tempBuffer.append<uint32_t>(CGMESSAGE::REQUEST_ACCOUNT_LOGIN);
	tempBuffer.append<int>(1);
	tempBuffer.append(_DeviceID);

	// 3) Message의 길이를 설정한다.
	tempBuffer.front<uint32_t>() = tempBuffer.len;

	// Return)
	return	tempBuffer;
}
