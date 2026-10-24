#include "pch.h"
#include "framework.h"
#include "CGDK10.game_mmo_field.server.mfc.h"
#include "CGDK10.game_mmo_field.server.mfc.dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

CCGDK10_game_mmo_field_server_mfc_dlg::CCGDK10_game_mmo_field_server_mfc_dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CGDK10GAME_MMO_FIELDSERVERMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_AREA_GROUP, m_list_area_group);
}

BEGIN_MESSAGE_MAP(CCGDK10_game_mmo_field_server_mfc_dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedCancel)
	ON_COMMAND(ID_SYSTEM_START, &CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceStart)
	ON_COMMAND(ID_SYSTEM_STOP, &CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceStop)
	ON_COMMAND(ID_SYSTEM_EXIT, &CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemExit)
	ON_COMMAND(ID_SYSTEM_CANCEL, &CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceCancel)
	ON_BN_CLICKED(IDC_BUTTON_POOL_STATUS, &CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedButtonPoolStatus)
	ON_BN_CLICKED(IDC_BUTTON_THREAD_INFO, &CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedButtonThreadInfo)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY_UPDATE, &CCGDK10_game_mmo_field_server_mfc_dlg::on_notify_message_update)
END_MESSAGE_MAP()

BOOL CCGDK10_game_mmo_field_server_mfc_dlg::OnInitDialog()
{
	//-----------------------------------------------------------------
	// 1. 기본적으로 들어 있는 부분.
	//-----------------------------------------------------------------
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);


	//-----------------------------------------------------------------
	// 2. log 설정
	//-----------------------------------------------------------------
	// 1) create logger view
	m_child_logger_view.CreateChild(nullptr, this);

	// 2) dialog의 크기를 얻는다.
	RECT rect_dlg;
	GetClientRect(&rect_dlg);
	RECT rect_dlg_logger;
	m_child_logger_view.GetClientRect(&rect_dlg_logger);

	// 3) set logger view
	m_child_logger_view.Resize(rect_dlg.right - rect_dlg_logger.right - 24, 30);
	m_child_logger_view.Move(4, -10, ORIGIN_LEFT | ORIGIN_UPON, GetDlgItem(IDC_STATIC_FOOTER));
	m_child_logger_view.Initialize();

	// 4) mfc log control을 연결한다.
	service::get_logger()->register_log_targetable(&m_child_logger_view);



	//-----------------------------------------------------------------
	// 4. 초기화.
	//-----------------------------------------------------------------
	// 1) Service를 생성한다.
	g_pservice = make_own<service>();

	// 2) unregister한다.
	g_pservice->register_messageable(this);

	// 3) Service를 initialize한다.
	request_service_initialize();


	//-----------------------------------------------------------------
	// 4. Control 설정.
	//-----------------------------------------------------------------
	// 1) session server socket
	m_list_area_group.InsertColumn(0, TEXT("id"),		LVCFMT_LEFT,   40, 0);
	m_list_area_group.InsertColumn(1, TEXT("name"),		LVCFMT_LEFT,  150, 1);
	m_list_area_group.InsertColumn(2, TEXT("type"),		LVCFMT_LEFT,  100, 2);
	m_list_area_group.InsertColumn(3, TEXT("status"),	LVCFMT_LEFT,  100, 3);
	m_list_area_group.InsertColumn(4, TEXT("max"),		LVCFMT_RIGHT,  70, 4);
	m_list_area_group.InsertColumn(5, TEXT("users"),	LVCFMT_RIGHT,  70, 5);
	m_list_area_group.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// declare) 
	context temp_context;

	// 2) load json file for setting
	temp_context.from_json(api::file::read_file(CONFIGURE_FILE));

	// 3) channeel server socket
	m_child_acceptor_user.CreateChild(g_pservice->pacceptor_user, this);
	m_child_acceptor_user.SetDefaultPort(temp_context["acceptor_user"sv]["local_endpoint"sv]["port"sv]);
	m_child_acceptor_user.Move(5, 0, ORIGIN_REAR | ORIGIN_CENTER_Y, GetDlgItem(IDC_STATIC_TAG_USER_ACCEPTOR));

	// 4) Traffic Info
	m_child_traffic_info.CreateChild(&net::io::statistics::Nsocket::statistics_get_traffic_info_total(), this);
	m_child_traffic_info.Move(0, 60, ORIGIN_CENTER_X | ORIGIN_BELOW, GetDlgItem(IDC_STATIC_FRAME_AREA_GROUP));
	m_child_traffic_info.EnableWindow(FALSE);
	m_child_traffic_info.SetUpdateInterval(2);

	// 6) Pool/Thread Status Dialog
	m_dlg_factory_status.CreateModaless(factory::manager::instance(), this);
	m_dlg_thread_status.CreateModaless(api::manager_thread::instance(), executor::manager::instance(), system_executor::instance(), this);

	// 7) Control을 모두 Disable한다.
	enable_server_controls(FALSE);


	//-----------------------------------------------------------------
	// 6. Start
	//-----------------------------------------------------------------
	// 1) Start
	request_service_start();


	// return) 
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CCGDK10_game_mmo_field_server_mfc_dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		// check) 
		RETURN_IF(g_pservice.empty(), );

		// - 화면에 보이는 club을 Update~
		list_update_event_area_group();
	}

	__super::OnTimer(nIDEvent);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedButtonPoolStatus()
{
	m_dlg_factory_status.ShowWindow(SW_SHOW);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedButtonThreadInfo()
{
	m_dlg_thread_status.ShowWindow(SW_SHOW);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedOk()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnBnClickedCancel()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceStart()
{
	request_service_start();
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceCancel()
{
	request_service_cancel();
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemServiceStop()
{
	request_service_stop();
}

void CCGDK10_game_mmo_field_server_mfc_dlg::OnSystemExit()
{
	request_service_destroy();
}

LRESULT CCGDK10_game_mmo_field_server_mfc_dlg::on_notify_message_update(WPARAM _wParam, LPARAM _lParam)
{
	switch (_wParam)
	{
	case	eMESSAGE::SERVER::ADMIN::UPDATE_SERVICE_STATUS:	// CGNETMESSAGE_ADMIN_UPDATE_SERVICE_STATUS
			switch ((eOBJECT_STATE)_lParam)
			{
			case	eOBJECT_STATE::STOPPED:
					on_service_stopped();
					break;

			case	eOBJECT_STATE::START_PENDING:
					on_service_start_pending();
					break;

			case	eOBJECT_STATE::RUNNING:
					on_service_running();
					break;

			case	eOBJECT_STATE::STOP_PENDING:
					on_service_stop_pending();
					break;

			case	eOBJECT_STATE::INITIALIZING_PENDING:
					on_service_initializing_pending();
					break;

			case	eOBJECT_STATE::DESTROYING_PENDING:
					on_service_destroying_pending();
					break;

			case	eOBJECT_STATE::UPDATE:
					on_service_update();
					break;
			}; 
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE:
			{
				request_service_initialize();
			}
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE:
			{
				request_service_destroy();
			}
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE:
			{
				request_service_start();
			}
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE:
			{
				request_service_stop();
			}
			break;

	case	CGNOTIFY_SERVER_UPDATE:
			{
				g_pservice->release();
			}
			break;
	};

	// return) 
	return S_OK;
}

result_code CCGDK10_game_mmo_field_server_mfc_dlg::process_message(sMESSAGE& _msg)
{
	if (_msg.message == eMESSAGE::WINDOWS::NOTIFY_UPDATE)
	{
		sMESSAGE_NOTIFY& msg_notify = (sMESSAGE_NOTIFY&)_msg;

		switch (msg_notify.command)
		{
		case	CGNOTIFY_SERVER_UPDATE:
				{
					g_pservice->add_ref();
				}
				PostMessage(eMESSAGE::WINDOWS::NOTIFY_UPDATE, msg_notify.command, (LPARAM)reinterpret_cast<Igroup_user*>(msg_notify.wParam));
				break;

		case	CGNOTIFY_AREA_GROUP_ADD:
		case	CGNOTIFY_AREA_GROUP_REMOVE:
		case	CGNOTIFY_AREA_GROUP_UPDATE:
				{
					SCOPED_LOCK(m_cs_vector_event_area_group)
					{
						m_vector_event_area_group_queuing.emplace_back(EVENT_AREA_GROUP_NOTIFY{ msg_notify.command, reinterpret_cast<Igroup_user*>(msg_notify.wParam) });
					}
				}
				break;

		default:
				PostMessage(eMESSAGE::WINDOWS::NOTIFY_UPDATE, msg_notify.command, (LPARAM)reinterpret_cast<Igroup_user*>(msg_notify.wParam));
				break;
		};

		// return) 
		return eRESULT::DONE;
	}

	// return) 
	return eRESULT::BYPASS;
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_initializing_pending()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_destroying_pending()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_running()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	enable_server_controls(TRUE);

	auto& setting = g_pservice->get_service_instance()->info.setting;
	SetDlgItemTextA(m_hWnd, IDC_STATIC_SERVER_NAME, setting.name.c_str());
	auto str_server_id = std::format("{}", setting.iid);
	SetDlgItemTextA(m_hWnd, IDC_STATIC_SERVER_ID, str_server_id.c_str());
	auto str_server_version = std::format("{}", setting.revision.revision);
	SetDlgItemTextA(m_hWnd, IDC_STATIC_SERVER_VERSION, str_server_version.c_str());

	SetTimer(0, 1000, nullptr);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_start_pending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_stop_pending()
{
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_DISABLED);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_stopped()
{
	KillTimer(0);

	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	enable_server_controls(FALSE);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::on_service_update()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::request_service_initialize()
{
	// declare)
	context context_initialize;

	// 1) load json file for setting
	context_initialize.from_json(api::file::read_file(CONFIGURE_FILE));

	// 2) initialize service
	g_pservice->initialize(context_initialize);
}

void CCGDK10_game_mmo_field_server_mfc_dlg::request_service_start()
{
	// check) 
	RETURN_IF(g_pservice.empty());

	// 1) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 2) Open의 실행을 요청한다.
	system_executor::post(make_executable([]()
	{
		// declare)
		context context_start;

		// - load json file for setting
		context_start.from_json(api::file::read_file(CONFIGURE_FILE));

		// - initialize service
		g_pservice->start(context_start);
	}));
}

void CCGDK10_game_mmo_field_server_mfc_dlg::request_service_cancel()
{
}

void CCGDK10_game_mmo_field_server_mfc_dlg::request_service_stop()
{
	// check) 
	RETURN_IF(g_pservice.empty());

	// 1) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_ENABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 2) Close의 실행을 요청한다.
	system_executor::post(make_executable([]()
	{
		g_pservice->stop();
	}));
}

void CCGDK10_game_mmo_field_server_mfc_dlg::request_service_destroy()
{
	// 1) 현재 ThreadID를 구한다.(WM_QUIT message는 현재 Thread가 받아야 처리된다.)
	auto thread_id = GetCurrentThreadId();

	// 2) Menu Control
	GetMenu()->EnableMenuItem(ID_SYSTEM_START, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_CANCEL, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_STOP, MF_DISABLED);
	GetMenu()->EnableMenuItem(ID_SYSTEM_EXIT, MF_ENABLED);

	// 3) 
	SCOPED_LOCK(m_cs_vector_event_area_group)
	{
		m_vector_event_area_group_queuing.clear();
		m_vector_event_area_group_processing.clear();
	}

	// 4) List를 제거한다.
	SCOPED_LOCK(m_lockable_list_area_group)
	{
		for (int i = 0; i < m_list_area_group.GetItemCount(); ++i)
		{
			// - Item을 얻는다.
			auto* psocket_server_session = (Igroup_user*)m_list_area_group.GetItemData(i);

			// - release한다.
			psocket_server_session->release();
		}

		// - 모든 Item을 지운다.
		m_list_area_group.DeleteAllItems();
	}

	// 5) destroy의 실행을 요청한다.
	system_executor::post(make_executable([=]()
	{
		// - Server를 닫는다.
		g_pservice->destroy();

		// - logger를 등록해제한다.
		service::get_logger()->unregister_log_targetable(&m_child_logger_view);

		// - unregister한다.
		g_pservice->unregister_messageable(this);

		// - QUIT Message를 현재의 Thread에 날린다.(이것으로 애플리케이션은 종료된다.)
		PostThreadMessage(thread_id, WM_QUIT, (WPARAM)IDOK, 0);
	}));
}

void CCGDK10_game_mmo_field_server_mfc_dlg::enable_server_controls(BOOL _enable)
{
	// 1) ...
	GetDlgItem(IDC_STATIC_TAG_SERVER_NAME)->EnableWindow(_enable);
	GetDlgItem(IDC_STATIC_SERVER_NAME)->EnableWindow(_enable);
	GetDlgItem(IDC_STATIC_TAG_SERVER_ID)->EnableWindow(_enable);
	GetDlgItem(IDC_STATIC_SERVER_ID)->EnableWindow(_enable);
	GetDlgItem(IDC_STATIC_TAG_SERVER_VERSION)->EnableWindow(_enable);
	GetDlgItem(IDC_STATIC_SERVER_VERSION)->EnableWindow(_enable);

	// 2) Users & clubs
	GetDlgItem(IDC_STATIC_FRAME_AREA_GROUP)->EnableWindow(_enable);
	m_list_area_group.EnableWindow(_enable);

	// 3) server sockets
	m_child_acceptor_user.EnableWindow(_enable);

	// 4) traffic infos
	m_child_traffic_info.EnableWindow(_enable);

	// 5) update name
	if(_enable)
	{
		SetDlgItemTextA(m_hWnd, IDC_STATIC_SERVER_NAME, g_pservice->get_service_instance()->info.setting.name.c_str());
	}
}

int CCGDK10_game_mmo_field_server_mfc_dlg::list_add_area_group(Igroup_user* _parea_group)
{
	// declare) 
	LV_ITEM	list_item;

	SCOPED_LOCK(m_lockable_list_area_group)
	{
		// declare) 
		WCHAR text[256];

		// 1) 추가한다.
		list_item.iItem = m_list_area_group.InsertItem(m_list_area_group.GetItemCount(), TEXT(""));

		// 2) club
		list_item.mask = LVIF_TEXT;
		list_item.state = 0;
		list_item.cchTextMax = 256;

		// 0. "id"
		auto pgroup_area = dynamic_cast<area::group*>(_parea_group);

		if(pgroup_area != nullptr)
		{
			SCOPED_LOCK(pgroup_area->m_cs_area_info)
			{
				wsprintf(text, TEXT("%d"), pgroup_area->m_area_info.id);
			}
		}
		else
		{
			wsprintf(text, TEXT(" -"));
		}
		list_item.iSubItem = 0;
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);

		// 1. "name"
		list_item.iSubItem = 1;
		wsprintf(text, TEXT("%hs"), _parea_group->name().data());
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);

		// 2. "type"
		list_item.iSubItem = 2;
		if (pgroup_area != nullptr)
			wsprintf(text, TEXT("%hs"), "area group");
		else
			wsprintf(text, TEXT("%hs"), "general");
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);

		// 3. "status"
		list_item.iSubItem = 3;
		wsprintf(text, _parea_group->is_member_enterable() ? TEXT("open") : TEXT("close"));
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);
	

		// 4. "max"
		list_item.iSubItem = 4;
		if (_parea_group->member_capacity() != SIZE_T_MAX)
			wsprintf(text, TEXT("%I64u"), _parea_group->member_capacity());
		else
			wsprintf(text, TEXT("INFINITE"));
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);

		// 4. "users"
		list_item.iSubItem = 5;
		wsprintf(text, TEXT("%I64u"), _parea_group->member_count());
		list_item.pszText = text;
		m_list_area_group.SetItem(&list_item);

		// 4) Item Data로 포인터를 추가한다.
		m_list_area_group.SetItemData(list_item.iItem, (DWORD_PTR)_parea_group);

		// 5) Index를 ID로 맵핑하여 저장한다.
		_parea_group->m_idx_list = m_list_area_group.MapIndexToID(list_item.iItem);

		// 6) add_ref한다.
		_parea_group->add_ref();

		// 7) Update한다.
		list_update_area_group(list_item.iItem);
	}

	// return) 
	return list_item.iItem;
}

void CCGDK10_game_mmo_field_server_mfc_dlg::list_remove_area_group(Igroup_user* _parea_group)
{
	// declare) 
	int iCountUser;

	SCOPED_LOCK(m_lockable_list_area_group)
	{
		// 1) Index를 얻는다.
		int	index = m_list_area_group.MapIDToIndex(_parea_group->m_idx_list);

		// check)
		RETURN_IF(index < 0)

		// 2) Item을 얻는다.
		auto* psocket_lobby_server = (Igroup_user*)m_list_area_group.GetItemData(index);

		// check)
		CGASSERT_ERROR(_parea_group == psocket_lobby_server);

		// 3) 지운다.
		m_list_area_group.DeleteItem(index);

		// 4) reset m_idex_list
		psocket_lobby_server->m_idx_list = static_cast<uint32_t>(-1);

		// 5) release한다.
		psocket_lobby_server->release();

		// 6) ..
		iCountUser = m_list_area_group.GetItemCount();
	}
}

void CCGDK10_game_mmo_field_server_mfc_dlg::list_update_area_group(Igroup_user* _parea_group)
{
	SCOPED_LOCK(m_lockable_list_area_group)
	{
		// 1) Index를 얻는다.
		int	index = m_list_area_group.MapIDToIndex(_parea_group->m_idx_list);

		// check)
		RETURN_IF(index < 0)

		// 2) Group Room을 Update한다.
		list_update_area_group(index);
	}
}

void CCGDK10_game_mmo_field_server_mfc_dlg::list_update_area_group(int _index)
{
	// declare) 
	LV_ITEM	list_item;

	// 1) club
	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = 256;
	list_item.iItem = _index;

	SCOPED_LOCK(m_lockable_list_area_group)
	{
		// check) 
		RETURN_IF(_index < 0 || _index >= m_list_area_group.GetItemCount(), );

		// declare) 
		TCHAR text[256]{ 0 };

		// 1) socket_server_lobby을 얻는다.
		auto parea_group = (Igroup_user*)m_list_area_group.GetItemData(_index);
		 
		// 2) set value
		{
			// 3. "status"
			list_item.iSubItem = 3;
			wsprintf(text, parea_group->is_member_enterable() ? TEXT("open") : TEXT("close"));
			list_item.pszText = text;
			m_list_area_group.SetItem(&list_item);

			// 4. "max"
			list_item.iSubItem = 4;
			if(parea_group->member_capacity() != SIZE_T_MAX)
				wsprintf(text, TEXT("%I64u"), parea_group->member_capacity());
			else
				wsprintf(text, TEXT("INFINITE"));
			list_item.pszText = text;
			m_list_area_group.SetItem(&list_item);

			// 4. "users"
			list_item.iSubItem = 5;
			wsprintf(text, TEXT("%I64u"), parea_group->member_count());
			list_item.pszText = text;
			m_list_area_group.SetItem(&list_item);
		}
	}
}

void CCGDK10_game_mmo_field_server_mfc_dlg::list_update_event_area_group()
{
	SCOPED_LOCK(m_cs_vector_event_area_group)
	{
		m_vector_event_area_group_queuing.swap(m_vector_event_area_group_processing);
	}

	m_list_area_group.SetRedraw(false);

	SCOPED_LOCK(m_lockable_list_area_group)
	{
		// 1) event를 있으면 처리한다.
		if(m_vector_event_area_group_processing.empty() == false)
		{

			for (auto& iter : m_vector_event_area_group_processing)
			{
				switch (iter.message)
				{
				case	CGNOTIFY_AREA_GROUP_ADD:
						list_add_area_group(iter.psource.get());
						break;

				case	CGNOTIFY_AREA_GROUP_REMOVE:
						list_remove_area_group(iter.psource.get());
						break;

				case	CGNOTIFY_AREA_GROUP_UPDATE:
						list_update_area_group(iter.psource.get());
						break;

				default:
					CGASSERT_ERROR(false);
					break;
				}
			}
		}
		m_vector_event_area_group_processing.clear();

		// 2) 화면에 보이는 것만 Update한다.
		int	start = m_list_area_group.GetTopIndex();
		int	end = m_list_area_group.GetTopIndex() + m_list_area_group.GetCountPerPage() + 1;

		SATURATE_MAX(end, m_list_area_group.GetItemCount());

		for (int i = start; i < end; ++i)
		{
			list_update_area_group(i);
		}
	}

	m_list_area_group.SetRedraw(true);
}
