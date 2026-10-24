
// LoginServerDlg.cpp : 구현 파일
//

#include "stdafx.h"
#include "LoginServer.h"
#include "LoginServerDlg.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

extern CGPTR<CCGLogHub>	g_pLog;
CGPTR<CService>			g_pservice;


// CLoginServerDlg 대화 상자
CLoginServerDlg::CLoginServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOGINSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLoginServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOGINSERVER,	m_listServerLobby);
}

BEGIN_MESSAGE_MAP(CLoginServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CLoginServerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CLoginServerDlg::OnBnClickedCancel)
	ON_COMMAND(ID_ABOUT_APPLICATION_INFO, &CLoginServerDlg::OnAboutApplicationInfo)
	ON_COMMAND(ID_SYSTEM_START, &CLoginServerDlg::OnSystemServiceStart)
	ON_COMMAND(ID_SYSTEM_STOP, &CLoginServerDlg::OnSystemServiceStop)
	ON_COMMAND(ID_SYSTEM_EXIT, &CLoginServerDlg::OnSystemExit)
	ON_COMMAND(ID_SETTING_CONFIGURES, &CLoginServerDlg::OnSettingConfigures)
	ON_COMMAND(ID_SETTING_ENGINETUNNING, &CLoginServerDlg::OnSettingEnginetunning)
	ON_COMMAND(ID_ABOUT_SYSTEM_INFO, &CLoginServerDlg::OnAboutSystemInfo)
	ON_COMMAND(ID_SYSTEM_CANCEL, &CLoginServerDlg::OnSystemServiceCancel)
	ON_BN_CLICKED(IDC_BUTTON_POOL_STATUS, &CLoginServerDlg::OnBnClickedButtonPoolStatus)
	ON_BN_CLICKED(IDC_BUTTON_THREAD_INFO, &CLoginServerDlg::OnBnClickedButtonThreadInfo)
	ON_MESSAGE(CGMESSAGE::WINDOWS::NOTIFY_UPDATE, &CLoginServerDlg::OnNotifyMessage_Update)
END_MESSAGE_MAP()


// CLoginServerDlg 메시지 처리기

BOOL CLoginServerDlg::OnInitDialog()
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
	m_childLogView.Resize(180, 46);
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
	g_pservice = make_object<service>();

	// 2) Unregister한다.
	g_pservice->RegisterMessageable(this);

	// 3) Initialize
	RequestServiceInitialize();


	//-----------------------------------------------------------------
	// 5. Control 설정.
	//-----------------------------------------------------------------
	// Declare) 
	CContext	tempContext;

	// 1) Load Json file for setting
	tempContext.read_from_JSON_file("LoginServer_Setting.json");

	// 2) User
	m_childAcceptorUser.CreateChild(g_pservice->m_pacceptorUser, this);
	//m_childAcceptorUser.SetDefaultPort(tempContext["acceptorUser"]["Port"]);
	m_childAcceptorUser.Move(5, 0, ORIGIN_REAR | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_ACCEPTOR_USER));

	m_childConnectiveUser.CreateChild(g_pservice->m_pacceptorUser->Statistics_GetConnectionInfo(), this);
	m_childConnectiveUser.Move(0, 25, ORIGIN_CENTER_X | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_FRAME_USER));
	m_childConnectiveUser.SetUpdateInterval(1);

	// 3) Server Lobby
	m_childAcceptorServerLobby.CreateChild(g_pservice->m_pacceptorServerLobby, this);
	//m_childAcceptorServerLobby.SetDefaultPort(tempContext["acceptorServerLobby"]["Port"]);
	m_childAcceptorServerLobby.Move(5, 0, ORIGIN_REAR | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_ACCEPTOR_LOGINSERVER));

	// 4) List Server Lobby
	m_listServerLobby.InsertColumn(0, TEXT("Server_No"),			LVCFMT_LEFT,	 70, 0);
	m_listServerLobby.InsertColumn(1, TEXT("Server_SessionKey"),	LVCFMT_LEFT,	120, 1);
	m_listServerLobby.InsertColumn(2, TEXT("IP"),					LVCFMT_LEFT,	120, 2);
	m_listServerLobby.InsertColumn(3, TEXT("Connect"),				LVCFMT_LEFT,	120, 3);
	m_listServerLobby.InsertColumn(4, TEXT("Acceptor Info"),		LVCFMT_LEFT,	160, 4);
	m_listServerLobby.InsertColumn(5, TEXT("Users"),				LVCFMT_RIGHT,	 70, 5);
	m_listServerLobby.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

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

void CLoginServerDlg::OnPaint()
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
HCURSOR CLoginServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLoginServerDlg::OnBnClickedOk()
{
	//CDialogEx::OnOK();
}


void CLoginServerDlg::OnBnClickedCancel()
{
	//CDialogEx::OnCancel();
}

void CLoginServerDlg::OnSystemServiceStart()
{
	RequestServiceStart();
}

void CLoginServerDlg::OnSystemServiceCancel()
{
	RequestServiceCancel();
}

void CLoginServerDlg::OnSystemServiceStop()
{
	RequestServiceStop();
}
									  
void CLoginServerDlg::OnSystemExit()
{
	RequestServiceDestroy();
}
										   
void CLoginServerDlg::OnSettingConfigures()
{
}
										   
void CLoginServerDlg::OnSettingEnginetunning()
{
}
										   
void CLoginServerDlg::OnAboutSystemInfo()
{
}

void CLoginServerDlg::OnAboutApplicationInfo()
{
}

int CLoginServerDlg::OnMessage(CGMSG& _msg)
{
	if (_msg.message == CGMESSAGE::WINDOWS::NOTIFY_UPDATE)
	{
		CGMSG_NOTIFY&	rMSG = (CGMSG_NOTIFY&)_msg;

		switch (rMSG.dwCommand)
		{
		case	CGNOTIFY_SERVER_UPDATE:
				{
					g_pservice->AddRef();
				}
				break;

		case	CGNOTIFY_LOBBYSERVER_CONNECT:
		case	CGNOTIFY_LOBBYSERVER_DISCONNECT:
		case	CGNOTIFY_LOBBYSERVER_UPDATE:
				{
					((ServerLobby::CSocket*)rMSG.wParam)->AddRef();
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

LRESULT CLoginServerDlg::OnNotifyMessage_Update(WPARAM _wParam, LPARAM _lParam)
{
	switch (_wParam)
	{
	case	CGMESSAGE::ADMIN::UPDATE_SERVICE_STATUS:	// CGNETMESSAGE_ADMIN_UPDATE_SERVICE_STATUS:
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

	case	CGMESSAGE::ADMIN::REQUEST_STOP_SERVICE:	// CGNETMESSAGE_ADMIN_REQUEST_STOP_SERVICE:
			{
				RequestServiceStop();
			}
			break;


	case	CGNOTIFY_SERVER_UPDATE:
			{
				UpdateServerInfo();
				g_pservice->Release();
			}
			break;


	case	CGNOTIFY_LOBBYSERVER_CONNECT:
			{
				auto	pSocket	 = (ServerLobby::CSocket*)_lParam;

				OnLobbyServerConnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_LOBBYSERVER_DISCONNECT:
			{
				auto	pSocket	 = (ServerLobby::CSocket*)_lParam;

				OnLobbyServerDisconnect(pSocket);
				pSocket->Release();
			}
			break;

	case	CGNOTIFY_LOBBYSERVER_UPDATE:
			{
				auto	pSocket	 = (ServerLobby::CSocket*)_lParam;

				OnLobbyServerUpdate(pSocket);
				pSocket->Release();
			}
			break;
	};

	// Return) 
	return	S_OK;
}

void CLoginServerDlg::OnServiceInitializingPending()
{
}

void CLoginServerDlg::OnServiceDestroyingPending()
{
}

void CLoginServerDlg::OnServiceRunning()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	EnableServerControls(TRUE);

	SetTimer(0, 2000, nullptr);
}

void CLoginServerDlg::OnServiceStartPending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CLoginServerDlg::OnServiceStopPending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CLoginServerDlg::OnServiceStopped()
{
	KillTimer(0);

	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	EnableServerControls(FALSE);
}

void CLoginServerDlg::OnServiceUpdate()
{
}

void CLoginServerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		// check) 
		RETURN_IF(g_pservice.empty());

		//SetDlgItemInt(IDC_STATIC_SERVER_VERSION, g_pservice->GetServerVersion());
		//SetDlgItemText(IDC_STATIC_SVN_VERSION, SVN_UPDATE_REVISION);

		//time::local::time_point	timeUpdated	 = g_pservice->m_timeServerList_Updated;
		//SetDlgItemText(IDC_STATIC_SERVERLIST_LAST_UPDATED, timeUpdated.to_string<TCHAR>().c_str());

		// 1) 모든 Lobby Server 정보를 Update한다.
		List_LobbyServerUpdateAll();
	}

	__super::OnTimer(nIDEvent);
}

void CLoginServerDlg::OnBnClickedButtonPoolStatus()
{
	m_dlgPoolStatus.ShowWindow(SW_SHOW);
}

void CLoginServerDlg::OnBnClickedButtonThreadInfo()
{
	m_dlgThreadStatus.ShowWindow(SW_SHOW);
}

void CLoginServerDlg::RequestServiceInitialize()
{
	// Declar)
	CGMSG_CONTEXT	msgContext;

	// 1) Service를 초기화한다.
	g_pservice->Initialize(msgContext);
}

void CLoginServerDlg::RequestServiceStart()
{
	// check) 
	RETURN_IF(g_pservice.empty());

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

		g_pservice->Start(msgContext);
	});
}

void CLoginServerDlg::RequestServiceCancel()
{
}

void CLoginServerDlg::RequestServiceStop()
{
	// check) 
	RETURN_IF(g_pservice.empty());

	// 1) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 2) Close의 실행을 요청한다.
	POST_EXECUTE([]()
	{
		g_pservice->Stop();
	});
}

void CLoginServerDlg::RequestServiceDestroy()
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
		g_pservice->Destroy();

		// - Trace를 등록해제한다.
		g_pLog->UnregisterMessageable(&m_childLogView);

		// - Unregister한다.
		g_pservice->UnregisterMessageable(this);

		// - QUIT Message를 현재의 Thread에 날린다.(이것으로 애플리케이션은 종료된다.)
		PostThreadMessage(iThreadID, WM_QUIT, (WPARAM)-1, 0);
	});
}

void CLoginServerDlg::EnableServerControls(BOOL _bEnable)
{
	// 1) ...
	GetDlgItem(IDC_STATIC_SERVER_VERSION_TAG)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_SERVER_VERSION)->EnableWindow(_bEnable);

	GetDlgItem(IDC_STATIC_SVN_VERSION_TAG)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_SVN_VERSION)->EnableWindow(_bEnable);

	// 3) User
	m_childAcceptorServerLobby.EnableWindow(_bEnable);
	m_listServerLobby.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_FRAME_LOGINSERVER)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_ACCEPTOR_LOGINSERVER)->EnableWindow(_bEnable);

	// 4) Server Game
	m_childAcceptorUser.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_FRAME_USER)->EnableWindow(_bEnable);
}

void CLoginServerDlg::OnLobbyServerConnect(ServerLobby::CSocket* _pSocket)
{
	List_LobbyServerAdd(_pSocket);
}

void CLoginServerDlg::OnLobbyServerDisconnect(ServerLobby::CSocket* _pSocket)
{
	List_LobbyServerRemove(_pSocket);
}

void CLoginServerDlg::OnLobbyServerUpdate(ServerLobby::CSocket* _pSocket)
{
	List_LobbyServerUpdate(_pSocket);
}

int CLoginServerDlg::List_LobbyServerAdd(ServerLobby::CSocket* _pSocket)
{
	// Declare) 
	WCHAR	text[256];
	LV_ITEM	list_item;
	int		iCountUser;

	SCOPED_LOCK(m_listServerLobby)
	{
		// 1) 추가한다.
		list_item.iItem = m_listServerLobby.InsertItem(m_listServerLobby.GetItemCount(), TEXT(""));

		list_item.mask			 = LVIF_TEXT;
		list_item.state			 = 0;
		list_item.cchTextMax	 = 256;
		list_item.iSubItem		 = 2;
		list_item.pszText		 = _pSocket->GetPeerAddress<TCHAR>(text, 256);
		m_listServerLobby.SetItem(&list_item);

		// 2) Item Data로 포인터를 추가한다.
		m_listServerLobby.SetItemData(list_item.iItem, (DWORD_PTR)_pSocket);

		// 3) Index를 ID로 맵핑하여 저장한다.
		_pSocket->m_idxList = m_listServerLobby.MapIndexToID(list_item.iItem);

		// 4) AddRef한다.
		_pSocket->AddRef();

		// 5) Update한다.
		List_LobbyServerUpdate(list_item.iItem);

		// 6) ..
		iCountUser = m_listServerLobby.GetItemCount();
	}

	// Return) 
	return	list_item.iItem;
}

void CLoginServerDlg::List_LobbyServerRemove(ServerLobby::CSocket* _pSocket)
{
	// Declare) 
	int		iCountUser;

	SCOPED_LOCK(m_listServerLobby)
	{
		// 1) Index를 얻는다.
		int	iIndex = m_listServerLobby.MapIDToIndex(_pSocket->m_idxList);

		// check) 
		ERROR_RETURN_IF(iIndex<0 || iIndex>m_listServerLobby.GetItemCount(), , CGLOG(g_pLog, eLOG_TYPE::EXCEPTION_, eLOG_LEVEL::NORMAL, _CGTEXT(" Exception) ListCtrl Map ID is invalid [%d->%d] (") TEXT(__FUNCTION__) TEXT(")"), _pSocket->m_idxList, iIndex));

		// 2) Item을 얻는다.
		auto	pSocket = (ServerLobby::CSocket*)m_listServerLobby.GetItemData(iIndex);

		// check)
		CGASSERT_ERROR(_pSocket == pSocket);

		// 3) 지운다.
		m_listServerLobby.DeleteItem(iIndex);

		// 4) Release한다.
		pSocket->Release();

		// 5) ..
		iCountUser = m_listServerLobby.GetItemCount();
	}
}

void CLoginServerDlg::List_LobbyServerUpdate(ServerLobby::CSocket* _pSocket)
{
	SCOPED_LOCK(m_listServerLobby)
	{
		// 1) Index를 얻는다.
		int	iIndex = m_listServerLobby.MapIDToIndex(_pSocket->m_idxList);

		// 2) Login Server를 Update한다.
		List_LobbyServerUpdate(iIndex);
	}
}

void CLoginServerDlg::List_LobbyServerUpdate(int _Index)
{
	// check) 
	RETURN_IF(_Index < 0 || _Index >= m_listServerLobby.GetItemCount());

	// 1) 
	auto				pSocket		 = (ServerLobby::CSocket*)m_listServerLobby.GetItemData(_Index);
	const SERVER_INFO&	rServerInfo	 = pSocket->m_ServerInfo;
 
	// 2) ...
	WCHAR		text[256];
	LV_ITEM		list_item;

	list_item.mask		 = LVIF_TEXT;
	list_item.state		 = 0;
	list_item.cchTextMax = 256;
	list_item.iItem		 = _Index;
 
	// 3) TEXT("Server_No")
	list_item.iSubItem	 = 0;
	wsprintf(text, TEXT("%u"), rServerInfo.nServer_No);
	list_item.pszText	 = text;
	m_listServerLobby.SetItem(&list_item);

	// 3) TEXT("Server_SessionKey")
	list_item.iSubItem	 = 1;
	wsprintf(text, TEXT("%I64u"), rServerInfo.nServer_SessionKey);
	list_item.pszText	 = text;
	m_listServerLobby.SetItem(&list_item);

	// 4) TEXT("Count")
	list_item.iSubItem	 = 3;
	auto	strTime		 = pSocket->Statistics_GetConnectTime().to_string<TCHAR>();
	list_item.pszText	 = (TCHAR*)strTime.c_str();
	m_listServerLobby.SetItem(&list_item);

	// 5) Acceptor Info
	list_item.iSubItem	 = 4;
	if(rServerInfo.nServer_Address.sin_port!=0)
	{
		CGNetAPI::AddressToString<wchar_t>(text, 256, rServerInfo.nServer_Address);
		list_item.pszText	 = text;
	}
	else
	{
		list_item.pszText	 = TEXT("NOT READY");
	}
	m_listServerLobby.SetItem(&list_item);

	// 6) User
	list_item.iSubItem	 = 5;
	wsprintf(text, TEXT("%d"), rServerInfo.nServer_Player_Count);
	list_item.pszText	 = text;
	m_listServerLobby.SetItem(&list_item);
}

void CLoginServerDlg::List_LobbyServerUpdateAll()
{
	SCOPED_LOCK(m_listServerLobby)
	{
		// 1) Game Server의 개수를 얻는다.
		int	countGameServer = m_listServerLobby.GetItemCount();

		/// 2) 모든 Game Server를 Update한다.
		for (int i = 0; i < countGameServer; ++i)
		{
			List_LobbyServerUpdate(i);
		}
	}
}

void CLoginServerDlg::UpdateServerInfo()
{
	//SetDlgItemInt(IDC_STATIC_SERVER_VERSION, g_pservice->GetServerVersion());
	//SetDlgItemText(IDC_STATIC_SVN_VERSION, _TO_CGSTRING(SVN_UPDATE_REVISION));
}