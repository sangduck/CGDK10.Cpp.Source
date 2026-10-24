#include "pch.h"
#include "framework.h"
#include "CGDK10.udp_echo_client.mfc.h"
#include "CGDK10.udp_echo_client_dlg.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

Ctcp_echo_client_dlg::Ctcp_echo_client_dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCP_ECHOWINDOWSMFC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ctcp_echo_client_dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PACKET_SIZE, m_comboMessageSize);
	DDX_Control(pDX, IDC_SLIDER_TIMES, m_sliderSendTimes);
	DDX_Control(pDX, IDC_LIST_SOCKET, m_listctrl_sockets);
	DDX_Control(pDX, IDC_LIST_DEST_ADDRESS, m_listctrl_dest_address);
}

#pragma warning(disable:4407)
BEGIN_MESSAGE_MAP(Ctcp_echo_client_dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_SYSTEM_EXIT,						&Ctcp_echo_client_dlg::OnSystemExit)
	ON_COMMAND(ID_ABOUT_CGDKINFO,					&Ctcp_echo_client_dlg::OnAboutCGDKinfo)
	ON_BN_CLICKED(IDOK,								&Ctcp_echo_client_dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL,							&Ctcp_echo_client_dlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_BIND,					&Ctcp_echo_client_dlg::OnBnClickedButtonBind)
	ON_BN_CLICKED(IDC_BUTTON_CLOSESOCKET,			&Ctcp_echo_client_dlg::OnBnClickedButtonClosesocket)
	ON_BN_CLICKED(IDC_BUTTON_CLOSESOCKET_ALL,		&Ctcp_echo_client_dlg::OnBnClickedButtonClosesocketAll)

	ON_BN_CLICKED(IDC_BUTTON_DEST_ADDRESS_ADD,		&Ctcp_echo_client_dlg::OnBnClickedButtonDestAddressAdd)
	ON_BN_CLICKED(IDC_BUTTON_DEST_ADDRESS_DEL,		&Ctcp_echo_client_dlg::OnBnClickedButtonDestAddressDel)

	ON_CBN_SELCHANGE(IDC_COMBO_PACKET_SIZE,			&Ctcp_echo_client_dlg::OnCbnSelchangeComboMessageSize)
	ON_BN_CLICKED(IDC_BUTTON_SINGLE_SEND,			&Ctcp_echo_client_dlg::OnBnClickedButtonSingleSend) 
	ON_BN_CLICKED(IDC_CHECK_SEND_TEST_STARTSTOP,	&Ctcp_echo_client_dlg::OnBnClickedCheckSendTestStartstop)
	ON_BN_CLICKED(IDC_CHECK_RELAY_ECHO,				&Ctcp_echo_client_dlg::OnBnClickedCheckRelayEcho)
	ON_EN_CHANGE(IDC_EDIT_SEND_PER_ONETIME,			&Ctcp_echo_client_dlg::OnEnChangeEditSendPerOnetime)
	ON_BN_CLICKED(IDC_BUTTON_POOL,					&Ctcp_echo_client_dlg::OnBnClickedButtonPool)
	ON_BN_CLICKED(IDC_BUTTON_THREAD,				&Ctcp_echo_client_dlg::OnBnClickedButtonThread)

	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_1,			&Ctcp_echo_client_dlg::OnDeltaposSpinTps1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_10,		&Ctcp_echo_client_dlg::OnDeltaposSpinTps10)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_100,		&Ctcp_echo_client_dlg::OnDeltaposSpinTps100)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_1000,		&Ctcp_echo_client_dlg::OnDeltaposSpinTps1000)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_10000,		&Ctcp_echo_client_dlg::OnDeltaposSpinTps10000)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_TPS_100000,	&Ctcp_echo_client_dlg::OnDeltaposSpinTps100000)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_SOCKET,		&Ctcp_echo_client_dlg::OnLvnItemchangedListSocket)
END_MESSAGE_MAP()
#pragma warning(default:4407)

// Ctcp_echo_client_dlg 메시지 처리기
BOOL Ctcp_echo_client_dlg::OnInitDialog()
{
	//-----------------------------------------------------------------
	// 1. MFC Default
	//-----------------------------------------------------------------
	CDialogEx::OnInitDialog();

	this->SetIcon(m_hIcon, TRUE);
	this->SetIcon(m_hIcon, FALSE);


	//-----------------------------------------------------------------
	// 2. 시작
	//-----------------------------------------------------------------
	// 1) tester 객체 생성
	g_ptest_udp_echo_client = make_own<test_udp_echo_client>();

	// 2) tester start
	g_ptest_udp_echo_client->start();


	//-----------------------------------------------------------------
	// 2. control 설정.
	//-----------------------------------------------------------------
	// 1) Connective
	this->m_listctrl_sockets.InsertColumn(0, TEXT("bind"), LVCFMT_LEFT, 120, 0);
	this->m_listctrl_sockets.InsertColumn(1, TEXT("send"), LVCFMT_RIGHT, 100, 1);
	this->m_listctrl_sockets.InsertColumn(2, TEXT("msg/s"), LVCFMT_RIGHT, 90, 2);
	this->m_listctrl_sockets.InsertColumn(3, TEXT("recv"), LVCFMT_RIGHT, 100, 3);
	this->m_listctrl_sockets.InsertColumn(4, TEXT("msg/s"), LVCFMT_RIGHT, 90, 4);
	this->m_listctrl_sockets.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	this->m_listctrl_dest_address.InsertColumn(0, TEXT("address"), LVCFMT_LEFT, 140, 0);
	this->m_listctrl_dest_address.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);


	// 2) 
	this->m_child_traffic_info.CreateChild(&net::io::statistics::Nsocket::statistics_get_traffic_info_total(), this);
	this->m_child_traffic_info.MoveToCenter(GetDlgItem(IDC_STATIC_CONTROL_TRAFFIC));
	this->m_child_traffic_info.ShowWindow(SW_SHOW);
	this->m_child_traffic_info.SetUpdateInterval(1);

	// 3) Pool Status Dialog
	this->m_dlg_object_factory_status.CreateModaless(factory::manager::instance(), this);

	// 4) Thread StatusDialog
	this->m_dlg_thread_status.CreateModaless(api::manager_thread::instance(), executor::manager::instance(), system_executor::instance(), this);

	// 5) Bind Address/Port
	this->SetDlgItemText(IDC_EDIT_ADDRESS, TEXT("any"));
	this->SetDlgItemInt(IDC_COMBO_PORT, 30000);
	// 6) Dest Address/Port
	this->SetDlgItemText(IDC_EDIT_ADDRESS_DESTINATION, TEXT("localhost"));
	this->SetDlgItemInt(IDC_COMBO_PORT_DESTINATION, 30001);

	// 7) send Times
	this->m_sliderSendTimes.SetRange(0, 1280000);
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, 100);
	this->m_sliderSendTimes.SetPos(100);

	// 8) Message Select
	this->m_comboMessageSize.SetCurSel(static_cast<int>(g_ptest_udp_echo_client->m_traffic_test_selected));

	// 9) tooltip
	this->m_ctrl_tooltip.Create(this);
	this->m_ctrl_tooltip.SetDelayTime(TTDT_AUTOMATIC, 100);
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_1), _TEXT("1 times"));
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_10), _TEXT("10 times"));
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_100), _TEXT("100 times"));
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_1000), _TEXT("1`000 times"));
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_10000), _TEXT("10`000 times"));
	this->m_ctrl_tooltip.AddTool(GetDlgItem(IDC_SPIN_TPS_100000), _TEXT("100`000 times"));

	// 10) ...
	this->enable_dest_address(false);


	//-----------------------------------------------------------------
	// 3. Timer설정.
	//-----------------------------------------------------------------
	// 1) Statistics를 위한 Timer
	this->SetTimer(1, 1000, NULL);


	// return) 
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void Ctcp_echo_client_dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	this->CDialogEx::OnSysCommand(nID, lParam);
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void Ctcp_echo_client_dlg::OnPaint()
{
	if (this->IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		this->GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, this->m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

void Ctcp_echo_client_dlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1)
	{
		this->list_update_socket_udp();
	}

	__super::OnTimer(nIDEvent);
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR Ctcp_echo_client_dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(this->m_hIcon);
}

void Ctcp_echo_client_dlg::OnBnClickedOk()
{
}

void Ctcp_echo_client_dlg::OnSystemExit()
{
	this->OnBnClickedCancel();
}

void Ctcp_echo_client_dlg::OnAboutCGDKinfo()
{
}

void Ctcp_echo_client_dlg::OnBnClickedCancel()
{
	// 1) close dialogs
	this->m_child_connective_info.DestroyWindow();
	this->m_child_traffic_info.DestroyWindow();
	this->m_dlg_object_factory_status.DestroyWindow();
	this->m_dlg_thread_status.DestroyWindow();

	// 2) 모든 소켓의 접속을 끊고 처리를 종료한다.
	g_ptest_udp_echo_client.reset();

	// 3) OnOK~
	this->OnOK();
}

void Ctcp_echo_client_dlg::OnBnClickedCheckSendTestStartstop()
{
	CButton* pButton = ((CButton*)GetDlgItem(IDC_CHECK_SEND_TEST_STARTSTOP));

	if (pButton->GetCheck() & BST_CHECKED)
	{
		g_ptest_udp_echo_client->enable_traffic_test(true);
	}
	else
	{
		g_ptest_udp_echo_client->enable_traffic_test(false);
	}
}

void Ctcp_echo_client_dlg::OnBnClickedCheckRelayEcho()
{
	CButton* pButton = ((CButton*)GetDlgItem(IDC_CHECK_RELAY_ECHO));

	if (pButton->GetCheck() & BST_CHECKED)
	{
		g_ptest_udp_echo_client->enable_relay_echo_test(true);
	}
	else
	{
		g_ptest_udp_echo_client->enable_relay_echo_test(false);
	}
}

void Ctcp_echo_client_dlg::OnBnClickedButtonBind()
{
	// 1)  read address from UI
	auto sockaddr_bind = this->ReadBindAddress();

	// 2) read
	auto psocket_udp = make_object<socket_udp>();

	// 3) bind
	auto result = g_ptest_udp_echo_client->request_bind(psocket_udp, sockaddr_bind);

	// check)
	if (result == false)
	{
		AfxMessageBox(TEXT("fail to bind socket"), MB_OK);
		return;
	}

	// 4) add
	this->list_add_socket_udp(psocket_udp);
}

void Ctcp_echo_client_dlg::OnBnClickedButtonClosesocket()
{
	std::vector<object_ptr<socket_udp>> vector_sockets;

	this->m_listctrl_sockets.SetRedraw(false);
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		for (auto i = 0; i < this->m_listctrl_sockets.GetItemCount(); )
		{
			if (this->m_listctrl_sockets.GetItemState(i, LVIS_SELECTED) == 0)
			{
				++i;
				continue;
			}

			socket_udp* psocket_udp = (socket_udp*)this->m_listctrl_sockets.GetItemData(i);
			vector_sockets.push_back(psocket_udp);
			if (psocket_udp == this->m_socket_selected)
				m_socket_selected.reset();
			this->m_listctrl_sockets.DeleteItem(i);

		}
	}
	this->m_listctrl_sockets.SetRedraw(true);

	for (auto& iter : vector_sockets)
	{
		g_ptest_udp_echo_client->request_closesocket(iter);
		iter->release();
	}
}

void Ctcp_echo_client_dlg::OnBnClickedButtonClosesocketAll()
{
	std::vector<object_ptr<socket_udp>> vector_sockets;

	this->m_listctrl_sockets.SetRedraw(false);
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		auto item_count = this->m_listctrl_sockets.GetItemCount();
		for (auto i = 0; i < item_count; ++i)
		{
			socket_udp* psocket_udp = (socket_udp*)this->m_listctrl_sockets.GetItemData(i);
			vector_sockets.push_back(psocket_udp);
		}
		this->m_socket_selected.reset();

	}
	this->m_listctrl_sockets.SetRedraw(true);

	for (auto& iter : vector_sockets)
	{
		g_ptest_udp_echo_client->request_closesocket(iter);
		iter->release();
	}
}

void Ctcp_echo_client_dlg::OnBnClickedButtonDestAddressAdd()
{
	// 1) get dest
	auto addr_dest = this->ReadDestinationAddress();

	// declare) 
	object_ptr<socket_udp> psocket_udp;

	// 1) get socket udp
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		// - get selected socket
		psocket_udp = this->m_socket_selected;

		// check)
		RETURN_IF(psocket_udp.empty(), );

		// - add address
		psocket_udp->m_vector_socket_destination.push_back(addr_dest);
	}

	this->list_set_dest_address(psocket_udp);
}


void Ctcp_echo_client_dlg::OnBnClickedButtonDestAddressDel()
{
	// declare) 
	object_ptr<socket_udp> psocket_udp;

	// 1) get socket udp
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		// - get selected socket
		psocket_udp = this->m_socket_selected;

		// check)
		RETURN_IF(psocket_udp.empty(), );
	}

	// declare)
	int deleted = 0;

	// 1) get socket udp
	SCOPED_LOCK(this->m_cs_listctrl_dest_address)
	{
		for (auto i = 0; i < this->m_listctrl_dest_address.GetItemCount(); )
		{
			if (this->m_listctrl_dest_address.GetItemState(i, LVIS_SELECTED) == 0)
			{
				++i;
				continue;
			}

			psocket_udp->m_vector_socket_destination.erase(psocket_udp->m_vector_socket_destination.begin() + i);
			this->m_listctrl_dest_address.DeleteItem(i);
			++deleted;
		}
	}

	if(deleted != 0)
		this->list_set_dest_address(psocket_udp);
}


void Ctcp_echo_client_dlg::OnCbnSelchangeComboMessageSize()
{
	// 1) 새로 선택한 값을 읽어온다.
	std::size_t	selected = this->m_comboMessageSize.GetCurSel();

	// 2) 새로 선택한 값을 써넣는다.
	g_ptest_udp_echo_client->set_traffic_test_message_size(selected);

	// 3) Randge를 선택한다.
	int	range_max = 128 * 10000;
	for(std::size_t i = 0; i < selected; ++i)
	{
		range_max /=2;
	}

	this->m_sliderSendTimes.SetRange(0, range_max);
}

void Ctcp_echo_client_dlg::OnBnClickedButtonSingleSend()
{
	g_ptest_udp_echo_client->request_send_immediately(1);
}

void Ctcp_echo_client_dlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	{
		int	iTimes = this->m_sliderSendTimes.GetPos();
		this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);

		g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void Ctcp_echo_client_dlg::OnEnChangeEditSendPerOnetime()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(this->m_sliderSendTimes.m_hWnd == NULL);

	// 1) 횟수를 구한다.
	int	iTimes = GetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME);

	// 2) Slider의 Pos를 설정한다.
	this->m_sliderSendTimes.SetPos(iTimes);

	// 3) set times
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);
}

void Ctcp_echo_client_dlg::OnBnClickedButtonPool()
{
	this->m_dlg_object_factory_status.ShowWindow(SW_SHOW);
}

void Ctcp_echo_client_dlg::OnBnClickedButtonThread()
{
	this->m_dlg_thread_status.ShowWindow(SW_SHOW);
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps10(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta * 10;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps100(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta * 100;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps1000(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta * 1000;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps10000(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta * 10000;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

void Ctcp_echo_client_dlg::OnDeltaposSpinTps100000(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int	iTimes = this->m_sliderSendTimes.GetPos() - pNMUpDown->iDelta * 100000;

	if (iTimes < 0)
		iTimes = 0;

	this->m_sliderSendTimes.SetPos(iTimes);;
	this->SetDlgItemInt(IDC_EDIT_SEND_PER_ONETIME, iTimes);
	g_ptest_udp_echo_client->set_traffic_test_count_per_single(iTimes);

	*pResult = 0;
}

BOOL Ctcp_echo_client_dlg::PreTranslateMessage(MSG* pMsg)
{
	this->m_ctrl_tooltip.RelayEvent(pMsg);

	return this->CDialogEx::PreTranslateMessage(pMsg);
}

void Ctcp_echo_client_dlg::list_add_socket_udp(socket_udp* _psocket)
{
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		// 1) get item_count
		auto item_count = this->m_listctrl_sockets.GetItemCount();

		// 2) 추가한다.
		auto item_pos = this->m_listctrl_sockets.InsertItem(item_count, TEXT(""));

		// 3) Item Data로 포인터를 추가한다.
		this->m_listctrl_sockets.SetItemData(item_pos, (DWORD_PTR)_psocket);

		// 4) Index를 ID로 맵핑하여 저장한다.
		_psocket->m_idx_list = this->m_listctrl_sockets.MapIndexToID(item_pos);

		// 5) add_ref한다.
		_psocket->add_ref();

		// 6) Update한다.
		this->_list_update_socket_udp(item_pos);
	}
}

void Ctcp_echo_client_dlg::list_remove_socket_udp(socket_udp* _psocket)
{
	// declare) 
	int iCountUser;

	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		// 1) Index를 얻는다.
		int	index = this->m_listctrl_sockets.MapIDToIndex(_psocket->m_idx_list);

		// check)
		RETURN_IF(index < 0)

		// 2) Item을 얻는다.
		auto* psocket_udp = (socket_udp*)this->m_listctrl_sockets.GetItemData(index);

		// check)
		CGASSERT_ERROR(_psocket == psocket_udp);

		// 3) 지운다.
		this->m_listctrl_sockets.DeleteItem(index);

		// 4) reset m_idex_list
		psocket_udp->m_idx_list = static_cast<uint32_t>(-1);

		// 5) release한다.
		psocket_udp->release();

		// 6) ..
		iCountUser = this->m_listctrl_sockets.GetItemCount();
	}
}

void Ctcp_echo_client_dlg::list_update_socket_udp(socket_udp* _psocket)
{
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		// 1) Index를 얻는다.
		int	index = this->m_listctrl_sockets.MapIDToIndex(_psocket->m_idx_list);

		// check)
		RETURN_IF(index < 0)

		// 2) Group Room을 Update한다.
		this->_list_update_socket_udp(index);
	}
}

void Ctcp_echo_client_dlg::list_update_socket_udp(int _index)
{
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		this->_list_update_socket_udp(_index);
	}
}

void Ctcp_echo_client_dlg::_list_update_socket_udp(int _index)
{
	// check) 
	RETURN_IF(_index < 0 || _index >= this->m_listctrl_sockets.GetItemCount(), );

	// declare) 
	TCHAR text[256]{ 0 };

	// 1) socket_server_lobby을 얻는다.
	auto psocket_udp = (socket_udp*)this->m_listctrl_sockets.GetItemData(_index);

	// 2) 
	LV_ITEM	list_item;
	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = 256;
	list_item.iItem = _index;

	// 3) ...
	if (psocket_udp != nullptr)
	{
		auto [mps_send, mps_recv, gap] = psocket_udp->statistics_update(chrono::time::utc::now());

		// check)
		RETURN_IF(gap == false, );

		// 0. "bind"
		list_item.iSubItem = 0;
		wsprintf(text, TEXT("%hs:%u"), to_string(psocket_udp->local_endpoint()).c_str(), psocket_udp->local_endpoint().port());
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 1. "send"
		list_item.iSubItem = 1;
		wsprintf(text, TEXT("%I64u"), psocket_udp->m_statistics_send.load());
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 2. "msg/s"
		list_item.iSubItem = 2;
		swprintf_s(text, 256, TEXT("%.2f"), mps_send);
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 3. "recv"
		list_item.iSubItem = 3;
		wsprintf(text, TEXT("%I64u"), psocket_udp->m_statistics_recv.load());
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 4. "msg/s"
		list_item.iSubItem = 4;
		swprintf_s(text, 256, TEXT("%.2f"), mps_recv);
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);
	}
	else
	{
		// 0. "bind"
		list_item.iSubItem = 0;
		wsprintf(text, TEXT(""));
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 1. "send"
		list_item.iSubItem = 1;
		wsprintf(text, TEXT(""));
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 2. "msg/s"
		list_item.iSubItem = 2;
		wsprintf(text, TEXT(""));
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 3. "recv"
		list_item.iSubItem = 3;
		wsprintf(text, TEXT(""));
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);

		// 4. "msg.s"
		list_item.iSubItem = 4;
		wsprintf(text, TEXT(""));
		list_item.pszText = text;
		this->m_listctrl_sockets.SetItem(&list_item);
	}
}

void Ctcp_echo_client_dlg::list_update_socket_udp()
{
	this->m_listctrl_sockets.SetRedraw(false);
	SCOPED_LOCK(this->m_cs_listctrl_sockets)
	{
		auto item_count = this->m_listctrl_sockets.GetItemCount();
		for (auto i = 0; i < item_count; ++i)
		{
			this->_list_update_socket_udp(i);
		}
	}
	this->m_listctrl_sockets.SetRedraw(true);
}


void Ctcp_echo_client_dlg::list_set_dest_address(socket_udp* _psocket)
{
	// declare) 
	TCHAR text[256]{ 0 };

	// 2) 
	LV_ITEM	list_item;
	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = 256;
	list_item.iItem = 0;

	this->m_listctrl_dest_address.SetRedraw(false);
	SCOPED_LOCK(this->m_cs_listctrl_dest_address)
	{
		m_listctrl_dest_address.DeleteAllItems();

		for (auto& iter : _psocket->m_vector_socket_destination)
		{
			// 0. "bind"
			list_item.iSubItem = 0;
			wsprintf(text, TEXT("%hs:%u"), to_string(iter).c_str(), iter.port());
			list_item.pszText = text;
			this->m_listctrl_dest_address.InsertItem(&list_item);

			++list_item.iItem;
		}
	}
	this->m_listctrl_dest_address.SetRedraw(true);
}

void Ctcp_echo_client_dlg::enable_dest_address(BOOL _enable)
{
	auto show = (_enable) ? SW_SHOW : SW_HIDE;

	GetDlgItem(IDC_STATIC_FRAME_DESTINATION_ADDR)->ShowWindow(show);
	GetDlgItem(IDC_EDIT_ADDRESS_DESTINATION)->ShowWindow(show);
	GetDlgItem(IDC_COMBO_PORT_DESTINATION)->ShowWindow(show);
	GetDlgItem(IDC_BUTTON_DEST_ADDRESS_ADD)->ShowWindow(show);
	GetDlgItem(IDC_BUTTON_DEST_ADDRESS_DEL)->ShowWindow(show);
	GetDlgItem(IDC_LIST_DEST_ADDRESS)->ShowWindow(show);
}

net::sockaddr Ctcp_echo_client_dlg::ReadBindAddress()
{
	// declare) 
	TCHAR str_address[256];

	// 1) Address String을 얻는다.
	this->GetDlgItemText(IDC_EDIT_ADDRESS, str_address, 256);

	// 2) Port를 읽는다.
	auto port = this->GetDlgItemInt(IDC_COMBO_PORT); // - Port를 설정한다.

	// 3) Address로 변환한다.
	 auto sockaddr_bind = from_string(str_address, static_cast<uint16_t>(port), AF_INET);

	// return)
	return sockaddr_bind;
}

net::sockaddr Ctcp_echo_client_dlg::ReadDestinationAddress()
{
	// declare) 
	TCHAR str_address[256];

	// 1) Address String을 얻는다.
	this->GetDlgItemText(IDC_EDIT_ADDRESS_DESTINATION, str_address, 256);

	// 2) Port를 읽는다.
	auto port = this->GetDlgItemInt(IDC_COMBO_PORT_DESTINATION); // - Port를 설정한다.

	// declare) 
	net::sockaddr sockaddr_destination;

	// 3) Address로 변환한다.
	sockaddr_destination = from_string(str_address);
	sockaddr_destination.port(static_cast<uint16_t>(port));

	// return)
	return sockaddr_destination;
}

void Ctcp_echo_client_dlg::OnLvnItemchangedListSocket(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (pNMLV->uNewState & LVIS_SELECTED)
	{
		this->enable_dest_address(true);

		object_ptr<socket_udp> psocket;
		SCOPED_LOCK(this->m_cs_listctrl_sockets)
		{
			psocket = (socket_udp*)m_listctrl_sockets.GetItemData(pNMLV->iItem);
			this->m_socket_selected = psocket;
		}
		this->list_set_dest_address(psocket);
	}
	else if (pNMLV->uOldState & LVIS_SELECTED)
	{
		this->enable_dest_address(false);
		this->m_listctrl_dest_address.SetRedraw(false);
		SCOPED_LOCK(this->m_cs_listctrl_dest_address)
		{
			this->m_listctrl_dest_address.DeleteAllItems();
			this->m_socket_selected.reset();
		}
		this->m_listctrl_dest_address.SetRedraw(true);
	}
	*pResult = 0;
}
