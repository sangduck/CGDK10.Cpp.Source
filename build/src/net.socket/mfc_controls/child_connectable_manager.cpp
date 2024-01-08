#include "pch.h"


namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connectable_manager, CChildDialog)

CChild_connectable_manager::CChild_connectable_manager(CWnd* pParent)
	: CChildDialog(IDD_DIALOG_CONNECTABLE_MANAGER, pParent)
{
}

CChild_connectable_manager::~CChild_connectable_manager()
{
}

void CChild_connectable_manager::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONNECTABLE, m_list_connectable);
}


BEGIN_MESSAGE_MAP(CChild_connectable_manager, CChildDialog)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CChild_connectable_manager::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT_ALL, &CChild_connectable_manager::OnBnClickedButtonDisconnectAll)
	ON_BN_CLICKED(IDC_BUTTON_GHOST_CLEAR, &CChild_connectable_manager::OnBnClickedButtonClearGhost)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CONNECTABLE, &CChild_connectable_manager::OnLvnItemchangedListConnectable)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CONNECTABLE, &CChild_connectable_manager::OnNMDblclkListConnectable)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connectable_manager::OnCGMessageNotify)
	ON_WM_ENABLE()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_ACTIVATE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


BOOL CChild_connectable_manager::CreateChild(net::io::Iconnectable_manager* _pconnectable_manager, CWnd* _pwnd)
{
	// declare) 
	BOOL	result;

	// 1) Connectable Manager을 설정한다.
	set_connectable_manager(_pconnectable_manager);

	// 2) 윈도우를 생성하로 Return한다.
	result	 = Create(IDD_DIALOG_CONNECTABLE_MANAGER, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 
	return	TRUE;
}

// CChild_connectable_manager message handlers

BOOL CChild_connectable_manager::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Memory Block Pool Status
	m_list_connectable.InsertColumn(0, TEXT("Socket"),		LVCFMT_LEFT,	100, 0);
	m_list_connectable.InsertColumn(1, TEXT("peer"),		LVCFMT_LEFT,	100, 1);
	m_list_connectable.InsertColumn(2, TEXT("State"),		LVCFMT_RIGHT,	 80, 2);
	m_list_connectable.InsertColumn(3, TEXT("Connect"),		LVCFMT_RIGHT,	 80, 3);
	m_list_connectable.InsertColumn(4, TEXT("Snd Msg"),		LVCFMT_RIGHT,	 80, 4);
	m_list_connectable.InsertColumn(5, TEXT("Snd Byte"),	LVCFMT_RIGHT,	 70, 5);
	m_list_connectable.InsertColumn(6, TEXT("Snd I/O"),		LVCFMT_RIGHT,	 50, 6);
	m_list_connectable.InsertColumn(7, TEXT("Snd Time"),	LVCFMT_RIGHT,	 60, 7);
	m_list_connectable.InsertColumn(8, TEXT("Rcv Msg"),		LVCFMT_RIGHT,	 80, 8);
	m_list_connectable.InsertColumn(9, TEXT("Rcv Byte"),	LVCFMT_RIGHT,	 70, 9);
	m_list_connectable.InsertColumn(10, TEXT("Rcv I/O"),	LVCFMT_RIGHT,	 50, 10);
	m_list_connectable.InsertColumn(11, TEXT("Rcv Time"),	LVCFMT_RIGHT,	 60, 11);
	m_list_connectable.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChild_connectable_manager::OnEnable(BOOL is_enable)
{
	__super::OnEnable(is_enable);


	GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_DISCONNECT_ALL)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_GHOST_CLEAR)->EnableWindow(is_enable);

	m_list_connectable.EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T4)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T5)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T6)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T7)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T8)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T9)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T10)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T11)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_TOTAL_SOCKET)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SEND_IO)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SEND_BYTES)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SEND_MESSAGE)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVE_IO)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVE_BYTES)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVE_MESSAGE)->EnableWindow(is_enable);
}
void CChild_connectable_manager::OnDestroy()
{
	// 1) Connectable List를 clear한다.
	clear_connectable_list();

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CChild_connectable_manager::OnSizing(UINT _fwSide, LPRECT _pRect)
{
	__super::OnSizing(_fwSide, _pRect);

	// 1) Width의 한계를 설정한다.

	// 2) Height의 한계를 설정한다.
}

void CChild_connectable_manager::OnSize(UINT _nType, int _cx, int _cy)
{
	__super::OnSize(_nType, _cx, _cy);

	if(m_list_connectable.m_hWnd!=NULL)
	{
		CRect	tempRectOriginal;
		GetClientRect(tempRectOriginal);

		CRect	tempRect;
		m_list_connectable.GetWindowRect(tempRect);

		ScreenToClient(tempRect);

		tempRect.left	 = 0;
		tempRect.right	 = tempRectOriginal.right;
		tempRect.bottom	 = tempRectOriginal.bottom;

		m_list_connectable.MoveWindow(tempRect);
	}
}

void CChild_connectable_manager::OnBnClickedButtonDisconnect()
{
	// check) m_pConnectableManager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(m_pConnectableManager.empty())

	POSITION pos = m_list_connectable.GetFirstSelectedItemPosition();
	while(pos)
	{
		int	nItem = m_list_connectable.GetNextSelectedItem(pos);

		// 1) Connectable을 얻는다.
		net::io::Iconnectable* p_connectable_disconnect = (net::io::Iconnectable*)m_list_connectable.GetItemData(nItem);

		// check) pconnectable이 nullptr일 수는 없다.
		CGASSERT_ERROR(p_connectable_disconnect!=nullptr);

		// 2) pConnectabl이 제대로 된 것인지 확인한다.
		object_ptr<net::io::Iconnectable> pconnectable = VarifyConnectable(p_connectable_disconnect);

		// check) 제대로 된 것이 아니라면 그냥 끝낸다.
		RETURN_IF(pconnectable.empty());
		
		// 3) Connectable의 접속을 종료한다.
		pconnectable->disconnect();
	}

	// 4) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_connectable_manager::OnBnClickedButtonDisconnectAll()
{
	//-----------------------------------------------------------------
	// 1. 
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Iconnectable_manager> pconnectable_manager;

	scoped_lock(m_lockable_connectable_manager)
	{
		pconnectable_manager = m_pConnectableManager;
	}


	//-----------------------------------------------------------------
	// 2. 
	//-----------------------------------------------------------------
	// check) pconnectable_manager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pconnectable_manager.empty())

	// 1) 모든 접속된 Socket을 끊는다.
	pconnectable_manager->close_connectable_all();

	// 2) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_connectable_manager::OnBnClickedButtonClearGhost()
{
	//-----------------------------------------------------------------
	// 1. 
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Iconnectable_manager>	pconnectable_manager;

	scoped_lock(m_lockable_connectable_manager)
	{
		pconnectable_manager	 = m_pConnectableManager;
	}


	//-----------------------------------------------------------------
	// 2. 
	//-----------------------------------------------------------------
	// check) pconnectable_manager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pconnectable_manager.empty())

	// 1) 모든 접속된 Socket을 끊는다.
	pconnectable_manager->close_connectable_all(TRUE);

	// 2) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}


void CChild_connectable_manager::clear_connectable_list()
{
	// 1) 값을 clear
	SetDlgItemText(IDC_STATIC_TOTAL_SOCKET,		TEXT(""));
	SetDlgItemText(IDC_STATIC_SEND_IO,			TEXT(""));
	SetDlgItemText(IDC_STATIC_SEND_BYTES,		TEXT(""));
	SetDlgItemText(IDC_STATIC_SEND_MESSAGE,		TEXT(""));
	SetDlgItemText(IDC_STATIC_RECEIVE_IO,		TEXT(""));
	SetDlgItemText(IDC_STATIC_RECEIVE_BYTES,	TEXT(""));
	SetDlgItemText(IDC_STATIC_RECEIVE_MESSAGE,	TEXT(""));

	// 2) 모든 Item을 지운다.
	m_list_connectable.DeleteAllItems();

	// 3) Acceptor를 Reset한다.
	scoped_lock(m_lockable_connectable_manager)
	{
		m_pConnectableManager.reset();
	}
}

object_ptr<net::io::Iconnectable> CChild_connectable_manager::VarifyConnectable(net::io::Iconnectable* _pconnectable)
{
	// declare) 
	object_ptr<net::io::Iconnectable_manager>	pmanager;

	// 1) Connective Manager를 설정한다.
	scoped_lock(m_lockable_connectable_manager)
	{
		pmanager	 = m_pConnectableManager;
	}

	// check) 
	RETURN_IF(pmanager.empty(),	object_ptr<net::io::Iconnectable>());

	// declare) 
	net::io::Iconnectable* pconnectable;

	// 2) Connectable의 Iterator를 얻는다.
	auto	pIter	= pmanager->alloc_iterator();

	while((pconnectable=pIter->get_next()) != nullptr)
	{
		// check) 
		if(pconnectable==_pconnectable)
		{
			return	pconnectable;
		}
	}

	// return) 
	return	object_ptr<net::io::Iconnectable>();
}

void CChild_connectable_manager::EnableControls(bool p_bStatus)
{
	// 1) enable 일반 control들...
	GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_BUTTON_DISCONNECT_ALL)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_BUTTON_GHOST_CLEAR)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_TOTAL_SOCKET)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_SEND_MESSAGE)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_SEND_BYTES)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_SEND_IO)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_RECEIVE_MESSAGE)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_RECEIVE_BYTES)->EnableWindow(p_bStatus);
	GetDlgItem(IDC_STATIC_RECEIVE_IO)->EnableWindow(p_bStatus);
	m_list_connectable.EnableWindow(p_bStatus);

	// 2) Static들 설정
	for(int i=IDC_STATIC_T1; i<=IDC_STATIC_T7; ++i)
	{
		GetDlgItem(i)->EnableWindow(p_bStatus);
	}
}

void CChild_connectable_manager::set_connectable_manager(net::io::Iconnectable_manager* _pconnectable_manager)
{
	// 1) Connectable Manager를 설정한다.
	scoped_lock(m_lockable_connectable_manager)
	{
		m_pConnectableManager = _pconnectable_manager;
	}

	// 2) ConnectableList를 Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}


void CChild_connectable_manager::UpdateConnectableList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Connectable Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Iconnectable_manager>	pconnectable_manager;

	scoped_lock(m_lockable_connectable_manager)
	{
		pconnectable_manager	 = m_pConnectableManager;
	}


	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// 1) 만약 ConnectableManager가 nullptr이면 clear만하고 끝낸다.
	if(pconnectable_manager.empty())
	{
		clear_connectable_list();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 Update한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_list_connectable.SetRedraw(FALSE);

	// declare) 
	int64_t		qwSend_IO			 = 0;
	int64_t		qwSend_Bytes		 = 0;
	int64_t		qwSend_Message		 = 0;
	int64_t		qwReceive_IO		 = 0;
	int64_t		qwReceive_Bytes		 = 0;
	int64_t		qwReceive_Message	 = 0;
	
	// declare) 
	LV_ITEM		list_item;

	list_item.mask			 = LVIF_TEXT;
	list_item.state			 = 0;
	list_item.cchTextMax	 = 1024;
	list_item.iItem			 = 0;

	auto tick_now = chrono::tick::now();

	// Chekck)
	net::io::Iconnectable* pconnectable;

	// 2) Iterator를 얻는다.(이건 다 쓰고 반드시 지워줘야 한다.)
	auto pIter= pconnectable_manager->alloc_iterator();

	while((pconnectable=pIter->get_next()) != nullptr)
	{
		// 3) 접속종료된 Connectable을 제거한다. 
		while(list_item.iItem <m_list_connectable.GetItemCount())
		{
			// - Connectable을 얻는다.
			net::io::Iconnectable* pConnectableCompare	 = (net::io::Iconnectable*)m_list_connectable.GetItemData(list_item.iItem);

			// check) 같으면 끝낸다~
			BREAK_IF(pConnectableCompare == pconnectable);

			// - 지운다~
			m_list_connectable.DeleteItem(list_item.iItem);
		}

		// 4) 새로 추가된 것이면 추가한다.
		if(list_item.iItem>=m_list_connectable.GetItemCount())
		{
			// declare) 
			TCHAR text[MAX_STRING_LENGTH]={};

			net::sockaddr addr_socket;

			// 0. "Socket Address"
			list_item.iSubItem		 = 0;
			addr_socket				 = pconnectable->local_endpoint();
			if(addr_socket.is_loopback()==false)
			{
				to_string<TCHAR>(text, MAX_STRING_LENGTH, addr_socket);
				list_item.pszText	 = text;
			}
			else
			{
				wsprintf(text, TEXT("localhost:%u"), addr_socket.port());
				list_item.pszText	 = text;
			}
			m_list_connectable.InsertItem(&list_item);

			// 1."peer Address"
			list_item.iSubItem		 = 1;
			addr_socket				 = pconnectable->remote_endpoint();
			
			if(addr_socket.is_loopback()==false)
			{
				to_string<TCHAR>(text, MAX_STRING_LENGTH, addr_socket);
				list_item.pszText		 = text;
			}
			else
			{
				wsprintf(text, TEXT("localhost:%u"), addr_socket.port());
				list_item.pszText		 = text;
			}
			m_list_connectable.SetItem(&list_item);

			list_item.iSubItem = 3;
			wsprintf(text, TEXT("%s"), to_string<TCHAR>(chrono::time::local::time_point(pconnectable->statistics_get_connect_time())).c_str());
			list_item.pszText = text;
			m_list_connectable.SetItem(&list_item);

			m_list_connectable.SetItemData(list_item.iItem, (DWORD_PTR)pconnectable);
		}

		//-----------------------------------------------------------------
		// 4) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
		//-----------------------------------------------------------------
		// declare) 
		TCHAR text[MAX_STRING_LENGTH]={};

		// 2. "State"
		list_item.iSubItem		 = 2;
		switch(pconnectable->socket_state())
		{
		case	eSOCKET_STATE::CLOSED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("CLOSED"));
				break;

		case	eSOCKET_STATE::BINDED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("BINDED"));
				break;

		case	eSOCKET_STATE::SYN:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("SYN"));
				break;

		case	eSOCKET_STATE::SEND_DISCONNECTED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("SND_DIS"));
				break;

		case	eSOCKET_STATE::FIN_WAIT_1:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("FIN_WAIT_1"));
				break;

		case	eSOCKET_STATE::FIN_WAIT_2:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("FIN_WAIT_2"));
				break;

		case	eSOCKET_STATE::TIME_WAIT:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("TIME_WAIT"));
				break;

		case	eSOCKET_STATE::CLOSE_WAIT:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("CLOSE_WAIT"));
				break;

		case	eSOCKET_STATE::LAST_ACK:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("LAST_ACK"));
				break;

		case	eSOCKET_STATE::ESTABLISHED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("ESTABLISHED"));
				break;

		case	eSOCKET_STATE::LISTEN:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("LISTEN"));
				break;

		case	eSOCKET_STATE::CERTIFIED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("CERTIFIED"));
				break;

		case	eSOCKET_STATE::LOGINED:
				list_item.pszText		 = const_cast<TCHAR*>(TEXT("LOGINED"));
				break;
		};
		m_list_connectable.SetItem(&list_item);

		// 4. "Snd Msg"
		list_item.iSubItem		 = 4;
		__to_string(text, pconnectable->statistics_get_sended_message());
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);

		// 5. "Snd Byte"
		list_item.iSubItem		 = 5;
		list_item.pszText		 = (TCHAR*)to_string_scaled_byte(text, 1024, pconnectable->statistics_get_sended_bytes()).data();
		m_list_connectable.SetItem(&list_item);

		// 6. "Snd I/O"
		list_item.iSubItem		 = 6;
		__to_string(text, pconnectable->statistics_get_async_send());
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);

		// 7. "Snd Time"
		list_item.iSubItem		 = 7;
		__to_string(text, (uint32_t)(tick_now>=pconnectable->statistics_get_tick_last_send() ? (tick_now-pconnectable->statistics_get_tick_last_send()).count() : 0));
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);

		// 8. "Rcv Msg"
		list_item.iSubItem		 = 8;
		__to_string(text, pconnectable->statistics_get_received_message());
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);

		// 9. "Rcv Byte"
		list_item.iSubItem		 = 9;
		list_item.pszText		 = (TCHAR*)to_string_scaled_byte(text, 1024, pconnectable->statistics_get_received_bytes()).data();
		m_list_connectable.SetItem(&list_item);

		// 10. "Rcv I/O"
		list_item.iSubItem		 = 10;
		__to_string(text, pconnectable->statistics_get_async_receive());
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);

		// 11. "Rcv Time"
		list_item.iSubItem		 = 11;
		__to_string(text, static_cast<uint32_t>(tick_now>=pconnectable->statistics_get_tick_last_receive() ? (tick_now-pconnectable->statistics_get_tick_last_receive()).count() : 0));
		list_item.pszText		 = text;
		m_list_connectable.SetItem(&list_item);


		//-----------------------------------------------------------------
		// 5) 통계자료를 계산한다.
		//-----------------------------------------------------------------
		qwSend_IO			+= pconnectable->statistics_get_async_send();
		qwSend_Bytes		+= pconnectable->statistics_get_sended_bytes();
		qwSend_Message		+= pconnectable->statistics_get_sended_message();

		qwReceive_IO		+= pconnectable->statistics_get_async_receive();
		qwReceive_Bytes		+= pconnectable->statistics_get_received_bytes();
		qwReceive_Message	+= pconnectable->statistics_get_received_message();

		++list_item.iItem;
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(int	count=m_list_connectable.GetItemCount()-list_item.iItem; count>0; count--)
	{
		// - Item을 지운다.
		m_list_connectable.DeleteItem(list_item.iItem);
	}


	//-----------------------------------------------------------------+
	// 4. Static 값들을 Update한다.
	//-----------------------------------------------------------------
	// declare) 
	TCHAR string[256] = { NULL, };

	// 1) 통계값을 표시한다.
	SetDlgItemInt(IDC_STATIC_TOTAL_SOCKET,		static_cast<uint32_t>(pconnectable_manager->count()));
	SetDlgItemText(IDC_STATIC_SEND_IO,			to_string_scaled_byte(string, 256, qwSend_IO).data());
	SetDlgItemText(IDC_STATIC_SEND_BYTES,		to_string_scaled_byte(string, 256, qwSend_Bytes).data());
	SetDlgItemText(IDC_STATIC_SEND_MESSAGE,		to_string_scaled_byte(string, 256, qwSend_Message).data());
	SetDlgItemText(IDC_STATIC_RECEIVE_IO,		to_string_scaled_byte(string, 256, qwReceive_IO).data());
	SetDlgItemText(IDC_STATIC_RECEIVE_BYTES,	to_string_scaled_byte(string, 256, qwReceive_Bytes).data());
	SetDlgItemText(IDC_STATIC_RECEIVE_MESSAGE,	to_string_scaled_byte(string, 256, qwReceive_Message).data());

	// 2) Redraw On
	m_list_connectable.SetRedraw(TRUE);
}


void CChild_connectable_manager::OnLvnItemchangedListConnectable(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	//LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	*pResult = 0;
}

void CChild_connectable_manager::OnNMDblclkListConnectable(NMHDR* pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// 1) ISocket을 얻는다.
	net::io::Iconnectable* pConnectableFind = reinterpret_cast<net::io::Iconnectable*>(m_list_connectable.GetItemData(pNMItemActivate->iItem));

	// 2) pConnectabl이 제대로 된 것인지 확인한다.
	object_ptr<net::io::Iconnectable> pconnectable = VarifyConnectable(pConnectableFind);

	// check) 제대로 된 것이 아니라면 그냥 끝낸다.
	RETURN_IF(pconnectable.empty());

	// 3) Dialog를 생성한다.
	CDialog_connectable_status dlg(pconnectable);

	// 4) DoModal해서 결과를 Return한다.
	dlg.DoModal();

	// - ...
	*pResult = 0;
}


void CChild_connectable_manager::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	// 1) 
	CChildDialog::OnActivate(nState, pWndOther, bMinimized);
	
	// 2) List를 Updae한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_connectable_manager::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Update한다.
		UpdateConnectableList();
	}

	CChildDialog::OnTimer(nIDEvent);
}

LRESULT CChild_connectable_manager::OnCGMessageNotify(WPARAM _wparam, LPARAM _lparam)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				// 1) 새로운 Acceptor를 얻는다.
				auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);

				// 2) Dynamic case
				auto _pconnectable_manager = dynamic_cast<net::io::Iconnectable_manager*>(pconnective);

				// 3) ConnectableManager를 설정한다.
				set_connectable_manager(_pconnectable_manager);
			}
			break;

	case	CGACCEPTOR_READY:
			{
				// 1) 새로운 Acceptor를 얻는다.
				auto pconnective = (net::io::Iconnective*)_lparam;

				// 2) Dynamic case
				auto _pconnectable_manager = dynamic_cast<net::io::Iconnectable_manager*>(pconnective);

				// declare) 
				bool bIsSame = false;

				// 3) ConnectableManager를 설정한다.
				scoped_lock(m_lockable_connectable_manager)
				{
					bIsSame = _pconnectable_manager==m_pConnectableManager;
				}

				// 4) 같은가???
				if(bIsSame)
				{
					EnableWindow(TRUE);
				}
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
				// 1) 새로운 Acceptor를 얻는다.
				auto pconnective = (net::io::Iconnective*)_lparam;

				// 2) Dynamic case
				auto _pconnectable_manager = dynamic_cast<net::io::Iconnectable_manager*>(pconnective);

				// declare) 
				bool bIsSame = false;

				// 3) ConnectableManager를 설정한다.
				scoped_lock(m_lockable_connectable_manager)
				{
					bIsSame	 = _pconnectable_manager==m_pConnectableManager;
				}

				// 4) 같은가???
				if(bIsSame)
				{
					EnableWindow(FALSE);
				}
			}
			break;

	case	CGACCEPTOR_START:
			{
			}
			break;

	case	CGACCEPTOR_STOP:
			{
			}
			break;

	case	CGUPDATE:
			{
				UpdateConnectableList();
			}
			break;
	};

	return	0;

}


}