#include "StdAfx.h"


void ServerLobby::CSocket::OnConnect(CGNet::IO::IConnective* /*_pConnective*/)
{
	g_pgroupServerLobbyImmigrationSocket->EnterMember(this);
}

void ServerLobby::CSocket::OnFailConnect(CGNet::IO::IConnective* /*_pConnective*/, uint32_t /*_Reason*/)
{
	m_psocketLoginServer.reset();
}

void ServerLobby::CSocket::OnDisconnect(uint32_t /*_Reason*/)
{
	// 1) Group에서 떼내기
	LeaveGroup();

	// 2) Socket LoginServer를 Reset하기.
	m_psocketLoginServer.reset();
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

CCGBuffer ServerLobby::CSocket::Message_REQUEST_ACCOUNT_CHECKIN()
{
	// 1) 버퍼를 할당받는다.
	CCGBuffer	bufTemp	 = MEM_POOL_ALLOC(32);

	// 2) 메시지를 작성한다.   `
	bufTemp.append<uint32_t>();
	bufTemp.append<uint32_t>(CGMESSAGE::REQUEST_ACCOUNT_CHECKIN);
	bufTemp.append<ACCOUNT_ID>(m_idAccount);

	// 3) 메시지의 제일 앞에 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>()	 = bufTemp.len;

	// Return) 
	return	bufTemp;



	//// check) 
	//CGASSERT_ERROR(m_aid.qwKey!=0);

	//// 1) Buffer를 할당받는다.
	//CCGBuffer	tempBuffer	 = MEM_POOL_ALLOC(sizeof(SMESSAGE_REQUEST_ACCOUNT_CHECKIN));

	//// 2) Message Head
	//auto&	rMsg	 = tempBuffer.append<SMESSAGE_REQUEST_ACCOUNT_CHECKIN>();

	//static CGEDC::CCRC crc(0x9B);		// CRC-8-WCDMA
	//BYTE* b = (BYTE*)tempBuffer.buf;
	//BYTE c = (BYTE)crc.GetCRC(b, 3);
	//b[3] = c;

	//// 3) Message 작성
	//rMsg.idAccount		 = m_aid;
	//rMsg.iLocation		 = _iLocation;

	//// 4) 전송한다.
	//return	tempBuffer;

	return	CCGBuffer();
}
