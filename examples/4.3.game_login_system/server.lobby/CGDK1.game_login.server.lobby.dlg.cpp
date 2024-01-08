
// LobbyServerDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "LobbyServer.h"
#include "LobbyServerDlg.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

extern CGPTR<CCGLogHub>	g_pLog;
CGPTR<CService>			g_pService;


// CLobbyServerDlg 대화 상자
CLobbyServerDlg::CLobbyServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOBBYSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLobbyServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_USER,	m_listUser);
}

BEGIN_MESSAGE_MAP(CLobbyServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CLobbyServerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CLobbyServerDlg::OnBnClickedCancel)
	ON_COMMAND(ID_ABOUT_APPLICATION_INFO, &CLobbyServerDlg::OnAboutApplicationInfo)
	ON_COMMAND(ID_SYSTEM_START, &CLobbyServerDlg::OnSystemServiceStart)
	ON_COMMAND(ID_SYSTEM_STOP, &CLobbyServerDlg::OnSystemServiceStop)
	ON_COMMAND(ID_SYSTEM_EXIT, &CLobbyServerDlg::OnSystemExit)
	ON_COMMAND(ID_SETTING_CONFIQURES, &CLobbyServerDlg::OnSettingConfigures)
	ON_COMMAND(ID_SETTING_ENGINETUNNING, &CLobbyServerDlg::OnSettingEnginetunning)
	ON_COMMAND(ID_ABOUT_SYSTEM_INFO, &CLobbyServerDlg::OnAboutSystemInfo)
	ON_COMMAND(ID_SYSTEM_CANCEL, &CLobbyServerDlg::OnSystemServiceCancel)
	ON_BN_CLICKED(IDC_BUTTON_POOL_STATUS, &CLobbyServerDlg::OnBnClickedButtonPoolStatus)
	ON_BN_CLICKED(IDC_BUTTON_THREAD_INFO, &CLobbyServerDlg::OnBnClickedButtonThreadInfo)
	ON_MESSAGE(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, &CLobbyServerDlg::OnNotifyMessage_Update)
END_MESSAGE_MAP()


// CLobbyServerDlg 메시지 처리기

BOOL CLobbyServerDlg::OnInitDialog()
{
	//-----------------------------------------------------------------
	// 1. MFC Default
	//-----------------------------------------------------------------
	CDialogEx::OnInitDialog();


	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.


	//-----------------------------------------------------------------
	// CGCII) CGCII Socket Parameters Tunning
	//-----------------------------------------------------------------
	//// 1) 최대 Exception 발생수 제한
	//CGException::Manager::GetInstance()->SetMiniDumpCountLimit(1024);
	//CGException::Manager::GetInstance()->SetMiniDumpType(CGException::MINIDUMP_TYPE_DETAIL);

	//// 2) Send시 최대 Gathering 수.
	//CGNet::IO::Sender::NStreamGather::SetDefaultMaxDepthOfSendBuffer(64);

	//// 3) 기본 Send Buffer와 Receive Buffer의 크기 
	////    - 송수신 성능을 높이기 위해서는 이 수치를 놓게 하면된다.
	//CGNet::IO::Socket::NTCP::SetDefaultSendBufferSize(4096);		// 기본값은 65536Byte이다.
	//CGNet::IO::Socket::NTCP::SetDefaultReceiveBufferSize(4096);	// 기본값은 65536Byte이다.


	//-----------------------------------------------------------------
	// 3. Log 설정
	//-----------------------------------------------------------------
	// 1) Log View
	m_childLogView.CreateChild(nullptr, this);
	m_childLogView.Resize(80, 26);
	m_childLogView.Move(8, 60, ORIGIN_LEFT | ORIGIN_BELOW, GetDlgItem(IDC_STATIC_FRAME_USER));

	// 2) Tracer 생성
	g_pLog			 = make_object<logger_hub>();

	// 3) 연결한다.
	g_pLog->RegisterMessageable(&m_childLogView);

	// 4) 기본 Log로 설정한다.
	SetDefaultTracer(g_pLog);


	//-----------------------------------------------------------------
	// 4. 초기화.
	//-----------------------------------------------------------------
	// 1) Service를 생성한다.
	g_pService = make_object<service>();

	// 2) Unregister한다.
	g_pService->RegisterMessageable(this);

	// 3) Initialize
	RequestServiceInitialize();


	//-----------------------------------------------------------------
	// 5. Control 설정.
	//-----------------------------------------------------------------
	// Declare) 
	CContext	tempContext;

	// 1) Load Json file for setting
	tempContext.read_from_JSON_file("LobbyServer_Setting.json");

	// 2) Server Login
	m_childConnectorServerLogin.CreateChild(g_pService->m_psocketServerLogin, 20000, this);
	//m_childConnectorServerLogin.SetDefaultPort(tempContext["acceptorServerLogin"]["Port"]);
	m_childConnectorServerLogin.Move(5, 0, ORIGIN_REAR | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_SOCKET_LOGINSERVER));


	// 3) User
	m_childAcceptorUser.CreateChild(g_pService->m_pacceptorUser, this);
	//m_childAcceptorUser.SetDefaultPort(tempContext["acceptorServerGame"]["Port"]);
	m_childAcceptorUser.Move(5, 0, ORIGIN_REAR | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_ACCEPTOR_USER));

	// 4) User
	m_childConnectiveUser.CreateChild(g_pService->m_pacceptorUser->Statistics_GetConnectionInfo(), this);
	m_childConnectiveUser.Move(0, 60, ORIGIN_CENTER_X | ORIGIN_TOP, GetDlgItem(IDC_STATIC_FRAME_USER));

	m_listUser.InsertColumn(0, TEXT("Name"),				LVCFMT_LEFT,	200,	 0);
	m_listUser.InsertColumn(1, TEXT("User"),				LVCFMT_LEFT,	 60,	 1);
	m_listUser.InsertColumn(2, TEXT("State"),				LVCFMT_LEFT,	 80,	 2);
	m_listUser.InsertColumn(3, TEXT("IP"),					LVCFMT_LEFT,	140,	 3);
	m_listUser.InsertColumn(4, TEXT("Acceptor Address"),	LVCFMT_LEFT,	140,	 4);
	m_listUser.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 5) Pool Status Dialog
	m_dlgPoolStatus.CreateModaless(CGPool::Manager::GetInstance(), this);

	// 6) Thread StatusDialog
	m_dlgThreadStatus.CreateModaless(CGAPI::ManagerThread::GetInstance(), this);

	// 7) Control을 모두 Disable한다.
	EnableServerControls(FALSE);


	//-----------------------------------------------------------------
	// 6. Start
	//-----------------------------------------------------------------
	// 1) Start
	RequestServiceStart();


	// Return) 
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CLobbyServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

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
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CLobbyServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLobbyServerDlg::OnBnClickedOk()
{
	//CDialogEx::OnOK();
}


void CLobbyServerDlg::OnBnClickedCancel()
{
	//CDialogEx::OnCancel();
}

void CLobbyServerDlg::OnSystemServiceStart()
{
	RequestServiceStart();
}

void CLobbyServerDlg::OnSystemServiceCancel()
{
	RequestServiceCancel();
}

void CLobbyServerDlg::OnSystemServiceStop()
{
	RequestServiceStop();
}
									  
void CLobbyServerDlg::OnSystemExit()
{
	RequestServiceDestroy();
}
										   
void CLobbyServerDlg::OnSettingConfigures()
{
}
										   
void CLobbyServerDlg::OnSettingEnginetunning()
{
}
										   
void CLobbyServerDlg::OnAboutSystemInfo()
{
}

void CLobbyServerDlg::OnAboutApplicationInfo()
{
}

int CLobbyServerDlg::OnMessage(CGMSG& _msg)
{
	if (_msg.message == CGMESSAGE::WINDOWS::NOTIFY_UPDATE)
	{
		CGMSG_NOTIFY&	rMSG = (CGMSG_NOTIFY&)_msg;

		switch (rMSG.dwCommand)
		{
		case	CGNOTIFY_SERVER_UPDATE:
				{
					g_pService->AddRef();
				}
				break;

		case	CGNOTIFY_LOGINSERVER_CONNECT:
		case	CGNOTIFY_LOGINSERVER_DISCONNECT:
		case	CGNOTIFY_LOGINSERVER_UPDATE:
				{
					((ServerLogin::CSocket*)rMSG.wParam)->AddRef();
				}
				break;

		case	CGNOTIFY_USER_CONNECT:
		case	CGNOTIFY_USER_DISCONNECT:
		case	CGNOTIFY_USER_UPDATE:
				{
					((User::CSocket*)rMSG.wParam)->AddRef();
				}
				break;
		};

		PostMessage(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, rMSG.dwCommand, rMSG.wParam);

		// Return) 
		return	TRUE;
	}

	// Return) 
	return	0;
}

LRESULT CLobbyServerDlg::OnNotifyMessage_Update(WPARAM _wParam, LPARAM _lParam)
{
	switch (_wParam)
	{
	case	CGMESSAGE::ADMIN::UPDATE_SERVICE_STATUS:	// CGNETMESSAGE_ADMIN_UPDATE_SERVICE_STATUS
			switch ((eOBJECT_STATE)_lParam)
			{
			case	eOBJECT_STATE::STOPPED:
					OnServiceStopped();
					break;

			case	eOBJECT_STATE::START_PENDING:
					OnServiceStartPending();
					break;

			case	eOBJECT_STATE::RUNNING:
					OnServiceRunning();
					break;

			case	eOBJECT_STATE::STOP_PENDING:
					OnServiceStopPending();
					break;

			case	eOBJECT_STATE::INITIALIZING_PENDING:
					OnServiceInitializingPending();
					break;

			case	eOBJECT_STATE::DESTROYING_PENDING:
					OnServiceDestroyingPending();
					break;

			case	eOBJECT_STATE::UPDATE:
					OnServiceUpdate();
					break;
			};
			break;

	case	CGMESSAGE::ADMIN::REQUEST_STOP_SERVICE:	// CGNETMESSAGE_ADMIN_REQUEST_STOP_SERVICE
			{
				RequestServiceStop();
			}
			break;


	case	CGNOTIFY_SERVER_UPDATE:
			{
				//UpdateServerInfo();
				g_pService->Release();
			}
			break;


	case	CGNOTIFY_LOGINSERVER_CONNECT:
			{
				auto	pSocket	 = (ServerLogin::CSocket*)_lParam;

				OnLoginServerConnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_LOGINSERVER_DISCONNECT:
			{
				auto	pSocket	 = (ServerLogin::CSocket*)_lParam;

				OnLoginServerDisconnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_LOGINSERVER_UPDATE:
			{
				auto	pSocket	 = (ServerLogin::CSocket*)_lParam;

				OnLoginServerUpdate(pSocket);
				pSocket->Release();
			}
			break;


	case	CGNOTIFY_USER_CONNECT:
			{
				auto	pSocket = (User::CSocket*)_lParam;

				OnUserConnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_USER_DISCONNECT:
			{
				auto	pSocket = (User::CSocket*)_lParam;

				OnUserDisconnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_USER_UPDATE:
			{
				auto	pSocket = (User::CSocket*)_lParam;

				OnUserUpdate(pSocket);
				pSocket->Release();
			}
			break;
	};

	// Return) 
	return	S_OK;
}

void CLobbyServerDlg::OnServiceInitializingPending()
{
}

void CLobbyServerDlg::OnServiceDestroyingPending()
{
}

void CLobbyServerDlg::OnServiceRunning()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	EnableServerControls(TRUE);

	SetTimer(0, 2000, nullptr);
}

void CLobbyServerDlg::OnServiceStartPending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CLobbyServerDlg::OnServiceStopPending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CLobbyServerDlg::OnServiceStopped()
{
	KillTimer(0);

	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	EnableServerControls(FALSE);
}

void CLobbyServerDlg::OnServiceUpdate()
{
}

void CLobbyServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		// check) 
		RETURN_IF(g_pService.empty());

		//SetDlgItemInt(IDC_STATIC_SERVER_VERSION, g_pService->GetServerVersion());
		//SetDlgItemText(IDC_STATIC_SVN_VERSION, SVN_UPDATE_REVISION);

		//time::local::time_point	timeUpdated	 = g_pService->m_timeServerList_Updated;
		//SetDlgItemText(IDC_STATIC_SERVERLIST_LAST_UPDATED, timeUpdated.to_string<TCHAR>().c_str());
	}

	__super::OnTimer(nIDEvent);
}

void CLobbyServerDlg::OnBnClickedButtonPoolStatus()
{
	m_dlgPoolStatus.ShowWindow(SW_SHOW);
}

void CLobbyServerDlg::OnBnClickedButtonThreadInfo()
{
	m_dlgThreadStatus.ShowWindow(SW_SHOW);
}

void CLobbyServerDlg::RequestServiceInitialize()
{
	// Declar)
	CGMSG_CONTEXT	msgContext;

	// 1) Service를 초기화한다.
	g_pService->Initialize(msgContext);
}

void CLobbyServerDlg::RequestServiceStart()
{
	// check) 
	RETURN_IF(g_pService.empty());

	// 1) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 2) Open의 실행을 요청한다.
	POST_EXECUTE([]()
	{
		// Declar)
		CGMSG_CONTEXT	msgContext;

		g_pService->Start(msgContext);
	});
}

void CLobbyServerDlg::RequestServiceCancel()
{
}

void CLobbyServerDlg::RequestServiceStop()
{
	// check) 
	RETURN_IF(g_pService.empty());

	// 1) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 2) Close의 실행을 요청한다.
	POST_EXECUTE([]()
	{
		g_pService->Stop();
	});
}

void CLobbyServerDlg::RequestServiceDestroy()
{
	// 1) 현재 ThreadID를 구한다.(WM_QUIT message는 현재 Thread가 받아야 처리된다.)
	auto	iThreadID = GetCurrentThreadId();

	// 2) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 3) Destroy의 실행을 요청한다.
	POST_EXECUTE([=]
	{
		// - Server를 닫는다.
		g_pService->Destroy();

		// - Trace를 등록해제한다.
		g_pLog->UnregisterMessageable(&m_childLogView);

		// - Unregister한다.
		g_pService->UnregisterMessageable(this);

		// - QUIT Message를 현재의 Thread에 날린다.(이것으로 어플리케이션은 종료된다.)
		PostThreadMessage(iThreadID, WM_QUIT, (WPARAM)-1, 0);
	});
}

void CLobbyServerDlg::EnableServerControls(BOOL _bEnable)
{
	// 1) ...
	//m_childConnectorServerLog.EnableWindow(_bEnable);
	//GetDlgItem(IDC_STATIC_CONNECTOR_ADMIN)->EnableWindow(_bEnable);

	// 2) Querier
	GetDlgItem(IDC_STATIC_SERVER_VERSION_TAG)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_SERVER_VERSION)->EnableWindow(_bEnable);

	GetDlgItem(IDC_STATIC_SVN_VERSION_TAG)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_SVN_VERSION)->EnableWindow(_bEnable);

	// 3) Server Login
	m_childConnectorServerLogin.EnableWindow(_bEnable);
	m_listUser.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_SOCKET_LOGINSERVER)->EnableWindow(_bEnable);

	// 4) Server Game
	m_childAcceptorUser.EnableWindow(_bEnable);
	m_listUser.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_FRAME_USER)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_ACCEPTOR_USER)->EnableWindow(_bEnable);
}

void CLobbyServerDlg::OnLoginServerConnect(ServerLogin::CSocket* _pSocket)
{
}

void CLobbyServerDlg::OnLoginServerDisconnect(ServerLogin::CSocket* _pSocket)
{
}

void CLobbyServerDlg::OnLoginServerUpdate(ServerLogin::CSocket* _pSocket)
{
}

void CLobbyServerDlg::OnUserConnect(User::CSocket* _pSocket)
{
	List_UserAdd(_pSocket);
}

void CLobbyServerDlg::OnUserDisconnect(User::CSocket* _pSocket)
{
	List_UserRemove(_pSocket);
}

void CLobbyServerDlg::OnUserUpdate(User::CSocket* _pSocket)
{
	List_UserUpdate(_pSocket);
}

int CLobbyServerDlg::List_UserAdd(User::CSocket* _pSocket)
{
	// Declare) 
	WCHAR	text[256];
	LV_ITEM	list_item;
	int		iCountUser;

	scoped_lock(m_listUser)
	{
		// 1) 추가한다.
		list_item.iItem = m_listUser.InsertItem(m_listUser.GetItemCount(), TEXT(""));

		list_item.mask			 = LVIF_TEXT;
		list_item.state			 = 0;
		list_item.cchTextMax	 = 256;
		list_item.iSubItem		 = 3;
		list_item.pszText		 = _pSocket->GetPeerAddress<TCHAR>(text, 256);
		m_listUser.SetItem(&list_item);

		// 2) Item Data로 포인터를 추가한다.
		m_listUser.SetItemData(list_item.iItem, (DWORD_PTR)_pSocket);

		// 3) Index를 ID로 맵핑하여 저장한다.
		_pSocket->m_idxList = m_listUser.MapIndexToID(list_item.iItem);

		// 4) AddRef한다.
		_pSocket->AddRef();

		// 5) Update한다.
		List_UserUpdate(list_item.iItem);

		// 6) ..
		iCountUser = m_listUser.GetItemCount();
	}

	// Return) 
	return	list_item.iItem;
}

void CLobbyServerDlg::List_UserRemove(User::CSocket* _pSocket)
{
	// Declare) 
	int		iCountUser;

	scoped_lock(m_listUser)
	{
		// 1) Index를 얻는다.
		int	iIndex	 = m_listUser.MapIDToIndex(_pSocket->m_idxList);

		// check) 
		ERROR_RETURN_IF(iIndex<0 || iIndex>m_listUser.GetItemCount(), , CGLOG(g_pLog, eLOG_TYPE::EXCEPTION_, eLOG_LEVEL::NORMAL, _CGTEXT(" Exception) ListCtrl Map ID is invalid [%d->%d] (") TEXT(__FUNCTION__) TEXT(")"), _pSocket->m_idxList, iIndex));

		// 2) Item을 얻는다.
		User::CSocket* pSocket	 = (User::CSocket*)m_listUser.GetItemData(iIndex);

		// check)
		CGASSERT_ERROR(_pSocket==pSocket);

		// 3) 지운다.
		m_listUser.DeleteItem(iIndex);

		// 4) Release한다.
		pSocket->Release();

		// 5) ..
		iCountUser	 = m_listUser.GetItemCount();
	}
}

void CLobbyServerDlg::List_UserUpdate(User::CSocket* _pSocket)
{
	scoped_lock(m_listUser)
	{
		// 1) Index를 얻는다.
		int	iIndex = m_listUser.MapIDToIndex(_pSocket->m_idxList);

		// 2) Game Server를 Update한다.
		List_UserUpdate(iIndex);
	}
}

TCHAR* GetString_ServerState(int _ServerState)
{
	TCHAR*	strState	 = nullptr;

	switch(_ServerState)
	{
	// -1 - 초기화 상태
	case	-1:
			strState	 = TEXT(" - ");
			break;

	// 0 - 서버원활
	case	0:
			strState	 = TEXT("STABLE");
			break;

	// 1 - 서버 중지
	case	1:
			strState	 = TEXT("STOP");
			break;

	// 2 - 유저 full
	case	2:
			strState	 = TEXT("FULL");
			break;

	// Default) 
	default:
			strState	 = TEXT("N/A");
			break;
	}

	// Return)
	return	strState;
}

void CLobbyServerDlg::List_UserUpdate(int _Index)
{
	// check) 
	RETURN_IF(_Index < 0 || _Index >= m_listUser.GetItemCount());

	// 1) Socket을 얻는다.
	User::CSocket*		pSocket = (User::CSocket*)m_listUser.GetItemData(_Index);

	// 2) ...
	WCHAR		text[256];
	LV_ITEM		list_item;
	list_item.mask			 = LVIF_TEXT;
	list_item.state			 = 0;
	list_item.cchTextMax	 = 256;
	list_item.iItem			 = _Index;

	//// 3) TEXT("Name")
	//list_item.iSubItem		 = 0;
	//list_item.pszText		 = pSocket->m_infoUser.ServerName;
	//m_listUser.SetItem(&list_item);

	//// 4) TEXT("Users")
	//list_item.iSubItem		 = 1;
	//list_item.pszText		 = text;
	//wsprintf(text, TEXT("%d"), pSocket->m_infoUser.TotalUser);
	//m_listUser.SetItem(&list_item);

	//// 5) TEXT("State")
	//list_item.iSubItem		 = 2;
	//list_item.pszText		 = GetString_ServerState(pSocket->m_infoUser.State);
	//m_listUser.SetItem(&list_item);

	//// 6) TEXT("IP")
	//list_item.iSubItem		 = 4;
	//list_item.pszText		 = pSocket->GetPeerAddress<TCHAR>(text, 256);
	//m_listUser.SetItem(&list_item);

	//// 7) TEXT("Acceptor Address")
	//list_item.iSubItem		 = 5;
	//list_item.pszText		 = text;
	//wsprintf(text, TEXT("%s:%d"), pSocket->m_infoUser.ServerIP, pSocket->m_infoUser.Port);
	m_listUser.SetItem(&list_item);
}

void CLobbyServerDlg::List_UserUpdateAll()
{
	scoped_lock(m_listUser)
	{
		// 1) Game Server의 갯수를 얻는다.
		int	countUser = m_listUser.GetItemCount();

		/// 2) 모든 Game Server를 Update한다.
		for (int i = 0; i < countUser; ++i)
		{
			List_UserUpdate(i);
		}
	}
}

//void CLobbyServerDlg::UpdateServerInfo()
//{
//	//SetDlgItemInt(IDC_STATIC_SERVER_VERSION, g_pService->GetServerVersion());
//	//SetDlgItemText(IDC_STATIC_SVN_VERSION, _TO_CGSTRING(SVN_UPDATE_REVISION));
//}