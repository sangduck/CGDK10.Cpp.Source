#include "StdAfx.h"


void ServerLogin::CSocket::OnRequestConnect(LPCSOCKADDR_IN /*_SockAddr*/)
{
	// 1) 접속 시도 시간을 얻는다.
	m_performanceConnect.CheckTimeStart();
}

void ServerLogin::CSocket::OnConnect(CGNet::IO::IConnective* /*_pConnective*/)
{
	// 1) 접속 완료 시간을 얻는다.
	m_performanceConnect.CheckTimeEnd();

	// 2) 아이디를 얻는다.
	GenerateLoginID();

	// 3) Group에 붙인다.
	g_pgroupServerLoginImmigrationSocket->EnterMember(this);
	// 3) ...
}

void ServerLogin::CSocket::OnFailConnect(CGNet::IO::IConnective* /*_pConnective*/, uint32_t /*_Reason*/)
{
	// 1) 접속 완료 시간을 얻는다.
	m_performanceConnect.CheckTimeEnd();
}

void ServerLogin::CSocket::OnDisconnect(uint32_t /*_Reason*/)
{
	// 1) Group에서 떼내기
	LeaveGroup();
}

int ServerLogin::CSocket::OnMessage(CGMSG& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return)
	return 0;
}







static int userIndex;
void ServerLogin::CSocket::GenerateLoginID()
{
	//if (g_IsGuildTest == true )	// 길드 테스트용.
	//{
	//	SCOPED_LOCK(m_csGuildTestLock)
	//	{
	//		static int nUserNameIndex_1 = 4000;

	//		++nUserNameIndex_1;
	//		ZeroMemory( m_cid.strID, sizeof(m_cid.strID) );
	//		sprintf(m_cid.strID, "GU%04d", nUserNameIndex_1);

	//		WCHAR CreateIDTest[32];
	//		ZeroMemory( CreateIDTest, sizeof(CreateIDTest) );
	//		wsprintf(CreateIDTest, L"GU%04d", nUserNameIndex_1);

	//		//CGLOG(LOG_TYPE_INFO, LOG_LEVEL_NORMAL, CreateIDTest);
	//	}
	//}
	//else
	//{
	//	GetRandomNickname(m_cid.strID, sizeof(m_cid.strID));
	//}

	//
	//
	//	// 1) Random을 Seed값을 구한다.
	//	ZeroMemory(m_cid.strID, sizeof(m_cid.strID));
	//
	//	int	iMod = 'a'-'z';
	//	srand(GetTickCount());
	//
	////	DWORD	dwRandom			 = (rand()+(++g_iSeed))%50000;
	//	srand(timeGetTime()+(++g_iSeed));
	//
	//
	//	//sprintf(m_cid.strID, "%08d", ++userIndex);
	//
	//
	//	m_cid.strID[0]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[1]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[2]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[3]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[4]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[5]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[6]	 = (BYTE)(rand()%iMod+'a');
	//	m_cid.strID[7]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[8]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[9]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[10]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[11]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[12]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[13]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[14]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[15]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[16]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[17]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[18]	 = (BYTE)(rand()%iMod+'a');
	//	//m_cid.strID[3]	 = (BYTE)(rand()%iMod+'0');
	//	//m_cid.strID[4]	 = (BYTE)(rand()%iMod+'0');
	//	//m_cid.strID[5]	 = (BYTE)(rand()%iMod+'0');
	//	//m_cid.strID[6]	 = (BYTE)(rand()%iMod+'0');
	//	//m_cid.strID[0]	 = '9';
	//	//m_cid.strID[1]	 = '9';
	//	//m_cid.strID[2]	 = 'T';
	//	//*(DWORD*)m_cid.strID	 = 100;
}

void ServerLogin::CSocket::GetRandomNickname(char* p_str, int strLength)
{
	ZeroMemory(p_str, strLength);

	int	iMod = 'a'-'z';
	srand((unsigned int)GetTickCount64());

	//	DWORD	dwRandom			 = (rand()+(++g_iSeed))%50000;
	srand(timeGetTime()+(++g_iSeed));


	p_str[0]	 = (BYTE)(rand()%iMod+'a');
	p_str[1]	 = (BYTE)(rand()%iMod+'a');
	p_str[2]	 = (BYTE)(rand()%iMod+'a');
	p_str[3]	 = (BYTE)(rand()%iMod+'a');
	p_str[4]	 = (BYTE)(rand()%iMod+'a');
	p_str[5]	 = (BYTE)(rand()%iMod+'a');
	p_str[6]	 = (BYTE)(rand()%iMod+'a');
	p_str[7]	 = (BYTE)(rand()%iMod+'a');
}
