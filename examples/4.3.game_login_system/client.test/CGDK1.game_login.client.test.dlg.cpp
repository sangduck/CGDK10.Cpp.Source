// SampleSocketClientTCPEchoDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "SampleSocketClientTCPEcho.h"
#include "SampleSocketClientTCPEchoDlg.h"
#include <mmsystem.h>
#include <locale.h>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

CGPTR<CCGLogFile> g_pLogFile;
CGPTR<CCGLogHub> g_pTraceEvent;


#pragma warning(disable:4407)
// CSampleSocketClientTCPEchoDlg 대화 상자


CGPTR<CGNet::CConnector<ServerLogin::CSocket>> CSampleSocketClientTCPEchoDlg::m_pconnectorServerLogin;
CGPTR<CGNet::CConnector<ServerLobby::CSocket>> CSampleSocketClientTCPEchoDlg::m_pconnectorSerbverLobby;
ATOMIC<int>							g_countRequestEnterUser;

CGPTR<ServerLogin::CGroupImmigrationSocket>	g_pgroupServerLoginImmigrationSocket;
CGPTR<ServerLobby::CGroupImmigrationSocket>	g_pgroupServerLobbyImmigrationSocket;
CGPTR<ServerLobby::CGroupSocket>			g_pgroupServerLobbySocket;
bool								g_IsGuildTest;			// 길드 테스트용.
extern	uint32_t					g_iSeed	 = 1;


CSampleSocketClientTCPEchoDlg::CSampleSocketClientTCPEchoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSampleSocketClientTCPEchoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	// Packet
	m_countErrorCRC			= 0;
	m_tickLoding			= 0;
	m_tickConStart			= timeGetTime();
	m_IsConClick			= false;

	SetMaxConcurrentExecutingCount(INT_MAX);
}

CSampleSocketClientTCPEchoDlg::~CSampleSocketClientTCPEchoDlg()
{
}


void CSampleSocketClientTCPEchoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSampleSocketClientTCPEchoDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CSampleSocketClientTCPEchoDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSampleSocketClientTCPEchoDlg::OnBnClickedCancel)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT1, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect1)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT10, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect10)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT100, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect100)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT_ALL, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonDisconnectAll)
	ON_BN_CLICKED(IDC_CHECK_CONNECT_TEST_STARTSTOP, &CSampleSocketClientTCPEchoDlg::OnBnClickedCheckConnectTestStartstop)
	ON_BN_CLICKED(IDC_BUTTON_POOL, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonPool)
	ON_BN_CLICKED(IDC_BUTTON_THREAD, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonThread)
	ON_BN_CLICKED(IDC_BUTTON_GAMESERVER_DISCONNECT, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonGameserverDisconnect)
	ON_BN_CLICKED(IDC_BUTTON__GAMESERVER_DISCONNECT_ALL, &CSampleSocketClientTCPEchoDlg::OnBnClickedButtonGameserverDisconnectAll)
END_MESSAGE_MAP()


// CSampleSocketClientTCPEchoDlg 메시지 처리기

BOOL CSampleSocketClientTCPEchoDlg::OnInitDialog()
{
	//-----------------------------------------------------------------
	// 1. MFC Default
	//-----------------------------------------------------------------
	CDialog::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// Local을 설정한다.
	setlocale(LC_ALL, "korean");
	

	//-----------------------------------------------------------------
	// 2. CGNet Configuration
	//-----------------------------------------------------------------
	// 1) 최대 Exception 발생수 제한
	CGException::Manager::GetInstance()->SetDumpCountLimit(1024);

	// 2) Send시 최대 Gathering 수.
	CGNet::IO::Sender::NStreamGather::SetDefaultMaxDepthOfSendBuffer(64);

	// 3) 기본 Send Buffer와 Receive Buffer의 크기 
	//    - 송수신 성능을 높이기 위해서는 이 수치를 놓게 하면된다.
	CGNet::IO::Socket::NTCP::SetDefaultSendBufferSize(4096);		// 기본값은 65536Byte이다.
	CGNet::IO::Socket::NTCP::SetDefaultReceiveBufferSize(4096);	// 기본값은 65536Byte이다.

	// 1) 최대 Send Gather Buffer의 갯수
	//CGNet::IO::Receivable::NStreamQueue::SetDefaultMaxReceivedBytesQueued(1024*1024*32);
	//CGNet::IO::Receivable::SetDefaultMessageBufferMaxSize(65536*4);


	//-----------------------------------------------------------------
	// 3. Connector
	//-----------------------------------------------------------------
	// Declare) 
	CGMSG_CONTEXT	msgContext;

	// 1) Connector [Login]를 생성한다.
	m_pconnectorServerLogin = make_object<CGNet::CConnector<ServerLogin::CSocket>>();
	m_pconnectorServerLogin->Start(msgContext);

	// 2) Connector [Game]
	m_pconnectorSerbverLobby = make_object<CGNet::CConnector<ServerLobby::CSocket>>();
	m_pconnectorSerbverLobby->Start(msgContext);

	// 3) Groups
	g_pgroupServerLoginImmigrationSocket	 = make_object<ServerLogin::CGroupImmigrationSocket>();
	g_pgroupServerLobbyImmigrationSocket	 = make_object<ServerLobby::CGroupImmigrationSocket>();
	g_pgroupServerLobbySocket				 = make_object<ServerLobby::CGroupSocket>();
	g_IsGuildTest					 = false;


	//-----------------------------------------------------------------
	// 4. Control 설정.
	//-----------------------------------------------------------------
	// 1) Connective
	m_childConnectiveLogin.CreateChild(m_pconnectorServerLogin->Statistics_GetConnectionInfo(), this);
	m_childConnectiveLogin.MoveToCenter(GetDlgItem(IDC_STATIC_FRAME_CONNECTION), 0, 35);
	m_childConnectiveLogin.ShowWindow(SW_SHOW);
	m_childConnectiveLogin.SetUpdateInterval(1);
	
	// 2) Connective
	m_childConnectiveGame.CreateChild(m_pconnectorSerbverLobby->Statistics_GetConnectionInfo(), this);
	m_childConnectiveGame.MoveToCenter(GetDlgItem(IDC_STATIC_FRAME_CONNECTION_GAME), 0, -35);
	m_childConnectiveGame.ShowWindow(SW_SHOW);
	m_childConnectiveGame.SetUpdateInterval(1);

	// 3) Traffice Info
	m_childTrafficInfo.CreateChild(&CGNet::IO::Statistics::NSocket::GetTotal(), this);
	m_childTrafficInfo.MoveToCenter(GetDlgItem(IDC_STATIC_FRAME_TRAFFIC));
	m_childTrafficInfo.ShowWindow(SW_SHOW);
	m_childTrafficInfo.SetUpdateInterval(2);

	// 4) Pool Status Dialog
	m_dlgObjectPool.CreateModaless(CGPool::Manager::GetInstance(), this);

	// 5) Thread StatusDialog
	m_dlgThreadStatus.CreateModaless(CGAPI::ManagerThread::GetInstance(), this);
	
	// 6) Destination IP Address
	SetDlgItemText(IDC_EDIT_ADDRESS, TEXT("127.0.0.1"));

	// 7) Destination Port
	SetDlgItemInt(IDC_EDIT_PORT, 21000);

	// 10) Connect Text Info
	SetDlgItemInt(IDC_EDIT_CONNECTIONS_PER_INTERVAL, 1000);
	SetDlgItemInt(IDC_EDIT_TEST_INTERVAL, 2000);
	SetDlgItemInt(IDC_EDIT_TEST_CONNECT_MIN, 5000);
	SetDlgItemInt(IDC_EDIT_TEST_CONNECT_MAX, 30000);

	// 11) Connect Times
	SetDlgItemInt(IDC_EDIT_CONNECT_TIMES, 2000);


	//-----------------------------------------------------------------
	// 5. Executor를 설정한다.
	//-----------------------------------------------------------------
	// 1) 
	CGExecutor::Default::InitInstance();

	// 2) Executor를 저장해 놓는다.
	m_pExecutor	 = CGExecutor::Default::GetInstance()->Executor();

	// 3) 
	srand(timeGetTime());

	// 4) Tracer 생성
	g_pLogFile		= make_object<log_file>();
	g_pTraceEvent	= make_object<logger_hub>();
	g_pTraceEvent->RegisterMessageable(g_pLogFile);


	//-----------------------------------------------------------------
	// 6. Timer설정.
	//-----------------------------------------------------------------
	// 1) Statistics를 위한 Timer
	SetTimer(1, 1000, NULL);

	//-----------------------------------------------------------------
	// 7. Log 설정
	//-----------------------------------------------------------------
	// 1) Log View
	m_childLogView.CreateChild(nullptr, this);
	m_childLogView.Resize(0, -80);
	m_childLogView.Move(5, 30, ORIGIN_LEFT | ORIGIN_BELOW, GetDlgItem(IDC_STATIC_FRAME_TRAFFIC));

	// 2) 기본 Log에 Filter를 생성하여 설정한다.
	//{
	//	CGPTR<CCGLogFilterDefault>	pLogFilter	 = NEW<CCGLogFilterDefault>();
	//	pLogFilter->EnableLog(LOG_TYPE_INFO);		pLogFilter->EnableLevelRange(LOG_TYPE_INFO,		true);	pLogFilter->SetLevelRange(LOG_TYPE_INFO,		LOG_LEVEL_NORMAL,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_PROGRESS);	pLogFilter->EnableLevelRange(LOG_TYPE_PROGRESS,	true);	pLogFilter->SetLevelRange(LOG_TYPE_PROGRESS,	LOG_LEVEL_NORMAL,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_DEBUG);	pLogFilter->EnableLevelRange(LOG_TYPE_DEBUG,		false);	pLogFilter->SetLevelRange(LOG_TYPE_DEBUG,		LOG_LEVEL_NORMAL,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_EXCEPTION);pLogFilter->EnableLevelRange(LOG_TYPE_EXCEPTION,	false);	pLogFilter->SetLevelRange(LOG_TYPE_EXCEPTION, LOG_LEVEL_LOWEST,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_ERROR);	pLogFilter->EnableLevelRange(LOG_TYPE_ERROR,		true);	pLogFilter->SetLevelRange(LOG_TYPE_ERROR,		LOG_LEVEL_NORMAL,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_USER);		pLogFilter->EnableLevelRange(LOG_TYPE_USER,		false);	pLogFilter->SetLevelRange(LOG_TYPE_USER,		LOG_LEVEL_LOWEST,		LOG_LEVEL_HIGHEST);
	//	pLogFilter->EnableLog(LOG_TYPE_SYSTEM);	pLogFilter->EnableLevelRange(LOG_TYPE_SYSTEM,		false);	pLogFilter->SetLevelRange(LOG_TYPE_SYSTEM,	LOG_LEVEL_LOWEST,		LOG_LEVEL_HIGHEST);
	//	m_childLogView.SetLogFilter(0, pLogFilter);
	//}

	// 3) 추가 Log창을 생성한다.
	{
		CGPTR<CCGLogFilterDefault>	pLogInfoFilter	 = make_object<CCGLogFilterDefault>();
		pLogInfoFilter->EnableLog(eLOG_TYPE::INFO_);
		m_childLogView.AddLogList(TEXT("Info"), pLogInfoFilter);

		//CGPTR<CCGLogFilterDefault>	pLogFilter	 = make_object<CCGLogFilterDefault>();
		//pLogFilter->EnableLog(eLOG_TYPE::ERROR_);
		//pLogFilter->EnableLog(eLOG_TYPE::EXCEPTION_);
		//m_childLogView.AddLogList(TEXT("Errors"), pLogFilter);
	}

	// 4) 연결한다.
	g_pTraceEvent->RegisterMessageable(&m_childLogView);

	// Return)
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CSampleSocketClientTCPEchoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

		CWnd::SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CSampleSocketClientTCPEchoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSampleSocketClientTCPEchoDlg::OnBnClickedOk()
{
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedCancel()
{
	// 1) Socket을 닫는다.
	KillTimer(1);
	KillTimer(2);
	KillTimer(3);

	// 2) 모든 소켓의 접속을 끊고 처리를 종료한다.
	m_pconnectorServerLogin->Stop();

	// 3) OnOK~
	OnOK();
}

void CSampleSocketClientTCPEchoDlg::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case	1:
			{
				SetDlgItemInt(IDC_STATIC_ERROR_COUNT, m_countErrorCRC);

				SetDlgItemInt(IDC_STATIC_COUNT_LOGIN_IMMIGRATION, g_pgroupServerLoginImmigrationSocket->GetMemberCount());
				SetDlgItemInt(IDC_STATIC_COUNT_GAME_IMMIGRATION, g_pgroupServerLobbyImmigrationSocket->GetMemberCount());
				SetDlgItemInt(IDC_STATIC_COUNT_GAME_STAGE, g_pgroupServerLobbySocket->GetMemberCount());

				if( m_IsConClick == true )
				{
					m_tickLoding = (DWORD)(timeGetTime() - m_tickConStart);

					if( m_pconnectorServerLogin->Statistics_GetKeep() == 0 )
					{
						//CGLOG(eLOG_TYPE::INFO_, eLOG_LEVEL::NORMAL, TEXT(" (Info) AllLoginEnd : %d millisecond "), m_tickLoding );
						m_IsConClick = false;
					}
				}
			}
			break;
	case	2:
			{
				m_iTestConnectPerInterval	 = GetDlgItemInt(IDC_EDIT_CONNECTIONS_PER_INTERVAL);
				m_iTestMinConnect			 = GetDlgItemInt(IDC_EDIT_TEST_CONNECT_MIN);
				m_iTestMaxConnect			 = GetDlgItemInt(IDC_EDIT_TEST_CONNECT_MAX);

				ConnectTest();

			}
			break;
	}


	CDialog::OnTimer(nIDEvent);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedCheckConnectTestStartstop()
{
	//-----------------------------------------------------------------
	// 1. Server Address를 읽는다.
	//-----------------------------------------------------------------
	ReadServerAddress();

	//-----------------------------------------------------------------
	// 2. Packet Send를 시작한다.
	//-----------------------------------------------------------------
	CButton*	pButton	 = ((CButton*)GetDlgItem(IDC_CHECK_CONNECT_TEST_STARTSTOP));

	if(pButton->GetCheck() & BST_CHECKED)
	{
		// - 마지막 Time을 Update함. 
		m_tickLast_Connect	 = timeGetTime();

		// -  Timer를 돌린다.
		DWORD	tempInterval = GetDlgItemInt(IDC_EDIT_TEST_INTERVAL);

		SATURATE_MIN(tempInterval, 1);

		SetTimer(2, tempInterval, NULL);
	}
	else
	{
		// - Timer를 죽임.
		KillTimer(2);
	}
}

void CSampleSocketClientTCPEchoDlg::ReadServerAddress()
{
	//-----------------------------------------------------------------
	// 1. Socket처리를 위한 Timer
	//-----------------------------------------------------------------
	// Declare) 
	TCHAR	strAddress[256];

	// 1) Address String을 얻는다.
	GetDlgItemText(IDC_EDIT_ADDRESS, strAddress, 256);

	// 2) Address로 변환한다.
	m_sockaddrServer			 = CGNetAPI::StringToAddress(strAddress);
	m_sockaddrServer.sin_port	 = htons((WORD)GetDlgItemInt(IDC_EDIT_PORT));	// - Port를 설정한다.
}

void CSampleSocketClientTCPEchoDlg::ConnectN(int p_n)
{
	//-----------------------------------------------------------------
	// 1. Server Address를 읽는다.
	//-----------------------------------------------------------------
	ReadServerAddress();

	//-----------------------------------------------------------------
	// 2. 접속한다.
	//-----------------------------------------------------------------
	for(int i=0; i<p_n; i++)
	{
		m_pconnectorServerLogin->RequestConnect((LPSOCKADDR)&m_sockaddrServer);
	}


	//-----------------------------------------------------------------
	// 3. 후처리.
	//-----------------------------------------------------------------
	//// View) Update
	//SetDlgItemInt(IDC_STATIC_NUMBER_OF_TRY,		(DWORD)m_pconnectorServerLogin->Statistics_GetTry());
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect1()
{
	ConnectN(1);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect10()
{
	ConnectN(10);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect100()
{
	ConnectN(100);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonConnect()
{
	// 1) 초기화.
	m_IsConClick = true;
	m_tickLoding = 0;
	m_tickConStart = timeGetTime();

	int	nTimer	 = GetDlgItemInt(IDC_EDIT_CONNECT_TIMES);

	ConnectN(nTimer);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonDisconnect()
{
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonDisconnectAll()
{
	m_pconnectorServerLogin->CloseAllConnectable();
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonGameserverDisconnect()
{
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonGameserverDisconnectAll()
{
	m_pconnectorSerbverLobby->DisconnectAllConnectable();
}

//void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonReset()
//{
//	m_dwCounterTotalConnected	 = 0;
//	m_fTimeTotalConnected		 = 0.0f;
//	SetDlgItemText(IDC_STATIC_AVERAGE_CONNECTION_DELAY, TEXT("0"));
//}

void CSampleSocketClientTCPEchoDlg::OnConnect(ServerLogin::CSocket* /*_pSocket*/)
{
}

void CSampleSocketClientTCPEchoDlg::OnDisconnect(ServerLogin::CSocket* /*_pSocket*/)
{
}

void CSampleSocketClientTCPEchoDlg::OnFailConnect(ServerLogin::CSocket* /*_pSocket*/)
{
}

intptr_t CSampleSocketClientTCPEchoDlg::ProcessExecute(_In_ intptr_t /*_Return*/, _In_ size_t /*_Param*/)
{
	return	0;
}

void CSampleSocketClientTCPEchoDlg::ConnectTest()
{
	//-----------------------------------------------------------------
	// 1. Disconnect Test
	//-----------------------------------------------------------------
	int		disconnected = 0;

	scoped_lock(*m_pconnectorServerLogin)
	{
		long	countSocket	 = m_pconnectorServerLogin->GetCount();

		if(countSocket > m_iTestMinConnect)
		{
			int	Range		 = countSocket;
			int	disconnect	 = (Range) ? rand()%Range : 0;

			auto	iter	 = m_pconnectorServerLogin->begin();
			auto	iterEnd	 = m_pconnectorServerLogin->end();

			for( ; iter!=iterEnd; ++iter)
			{
				// - Disconnect한다.
				if((*iter)->CloseSocket()==TRUE)
				{
					BREAK_IF((++disconnected)==disconnect);
				}
			}

			countSocket	-= disconnected;
		}
	}


	scoped_lock(*m_pconnectorSerbverLobby)
	{
		long	countSocket	 = m_pconnectorSerbverLobby->GetCount();

		if(countSocket > m_iTestMinConnect)
		{
			int	Range		 = countSocket-m_iTestMinConnect;
			Range			 = MIN(Range, m_iTestConnectPerInterval);
			int	disconnect	 = (Range) ? rand()%Range : 0;

			auto	iter	 = m_pconnectorSerbverLobby->begin();
			auto	iterEnd	 = m_pconnectorSerbverLobby->end();

			for( ; iter!=iterEnd; ++iter)
			{
				// - Disconnect한다.
				if((*iter)->CloseSocket()==TRUE)
				{
					BREAK_IF((++disconnected)==disconnect);
				}
			}

			countSocket	-= disconnected;
		}
	}


	//-----------------------------------------------------------------
	// 2. Connect Test
	//-----------------------------------------------------------------
	{
		// Declare) 
		int	nConnect;

		// 1) 현재 Socket 수를 계산한다.
		long	countSocket	 = m_pconnectorSerbverLobby->GetAllocatedCount()-disconnected;

		// 2) 최대접속수와 현재접속수의 차를 구한다
		nConnect	 = (countSocket < m_iTestMaxConnect) ? (m_iTestMaxConnect-countSocket) : 0;

		// 3) 최대접속수와 현재접속수의 차와 회당최대접속수 중 작은 값을 선택한다.
		nConnect	 = MIN(nConnect, m_iTestConnectPerInterval);

		// 4) 접속한다.
		ConnectN(nConnect);
	}
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonPool()
{
	m_dlgObjectPool.ShowWindow(SW_SHOW);
}

void CSampleSocketClientTCPEchoDlg::OnBnClickedButtonThread()
{
	m_dlgThreadStatus.ShowWindow(SW_SHOW);
}

