#include "pch.h"

extern	HINSTANCE	g_hInstance;

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connective_list, CChildDialog)

CChild_connective_list::CChild_connective_list(net::io::connective::manager* _pconnective_manager, CWnd* pParent)
	: CChildDialog(IDD_DIALOG_CONNECTIVE_LIST, pParent)
	, m_pmanager_connective(_pconnective_manager)
{
}

CChild_connective_list::~CChild_connective_list()
{
}

void CChild_connective_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONNECTIVE,	m_list_connective);
}


BEGIN_MESSAGE_MAP(CChild_connective_list, CChildDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CONNECTIVE, OnLvnItemchangedListConnective)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connective_list::OnCGMessageNotify)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CChild_connective_list::CreateChild(net::io::connective::manager* _pconnective_manager, CWnd* _pwnd)
{
	// declare) 
	BOOL	result;

	// 1) Connective Manager를 설정한다.
	SetConnectiveManager(_pconnective_manager);

	// 2) 윈도우를 생성한다.
	result	 = Create(IDD_DIALOG_CONNECTIVE_LIST, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return	TRUE;
}

void CChild_connective_list::OnLvnItemchangedListConnective(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here

	// 1) Check box를 선택한 것이다.
	if(pNMLV->uNewState & 0x2000)
	{
	}
	// 2) Check box를 선택해제한 것이다.
	else if(pNMLV->uNewState & 0x1000)
	{
	}
	// 3) 새로운 Item을 선택한 것이다.
	else if(pNMLV->uNewState & LVIS_SELECTED)
	{
		// - ConnectiveFind를 얻는다.
		net::io::Iconnective* pconnective_find = (net::io::Iconnective*)m_list_connective.GetItemData(pNMLV->iItem);

		// - Connective를 확인한다.
		object_ptr<net::io::Iconnective> pconnective_confirmed = VarifyConnective(pconnective_find);

		// - message를 날린다.
		if(pconnective_confirmed.exist())
		{
			SendMessageToTarget(static_cast<WPARAM>(CGACCEPTOR_SET_CONNECTIVE), reinterpret_cast<LPARAM>(pconnective_find));
		}
	}
	// 4) Item을 해제한 것이다.
	else if(pNMLV->uNewState!=pNMLV->uOldState)
	{
		SendMessageToTarget(static_cast<WPARAM>(CGACCEPTOR_SET_CONNECTIVE), reinterpret_cast<LPARAM>(nullptr));
	}

	*pResult = 0;
}

void CChild_connective_list::OnNMDblclkListConnective(NMHDR* pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// 1) ISocket을 얻는다.
	auto pconnective_find = reinterpret_cast<net::io::Iconnective*>(m_list_connective.GetItemData(pNMItemActivate->iItem));

	// 2) Connective를 확인한다.
	object_ptr<net::io::Iconnective> pconnective_confirmed = VarifyConnective(pconnective_find);

	// check) Connective를 확인 받지 못했다면 끝낸다.
	RETURN_IF(pconnective_confirmed.empty());

	// 3) message를 날린다.
	SendMessageToTarget((WPARAM)CGACCEPTOR_OPEN_CONNECTIVE, reinterpret_cast<LPARAM>(pconnective_find));

	// MFC기본)
	*pResult = 0;
}

void CChild_connective_list::OnEnable(BOOL _bEnable)
{
	CChildDialog::OnEnable(_bEnable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_CONNECTIVE_COUNT)->EnableWindow(_bEnable);

	m_list_connective.EnableWindow(_bEnable);
}

void CChild_connective_list::OnDestroy()
{
	// 1) Connective를 Reset한다.
	scoped_lock(m_lockable_manager_connective)
	{
		m_pmanager_connective.reset();
	}

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CChild_connective_list::SetConnectiveManager(net::io::connective::manager* _pconnective_manager)
{
	// 1) Connective Manager를 설정한다.
	scoped_lock(m_lockable_manager_connective)
	{
		m_pmanager_connective = _pconnective_manager;
	}

	// 2) ConnectiveList를 Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

object_ptr<net::io::Iconnective>	CChild_connective_list::VarifyConnective(net::io::Iconnective* _pconnective)
{
	// declare) 
	object_ptr<net::io::connective::manager> pmanager;

	// 1) Connective Manager를 설정한다.
	scoped_lock(m_lockable_manager_connective)
	{
		pmanager = m_pmanager_connective;
	}

	// check) 
	RETURN_IF(pmanager.empty(),	object_ptr<net::io::Iconnective>());

	// 2) 해당하는 Connective를 찾는다.
	scoped_lock(pmanager->get_lockable())
	{
		// - 해당하는 Connective를 얻는다.
		auto iter = std::find(pmanager->begin(), pmanager->end(), _pconnective);

		// check) 
		if(iter!=pmanager->end())
		{
			return	*iter;
		}
	}

	// return) 
	return	object_ptr<net::io::Iconnective>();
}

void CChild_connective_list::UpdateConnectiveList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)
	
	// 1) Connective Manager를 얻는다.
	object_ptr<net::io::connective::manager> pmanager_connective;

	scoped_lock(m_lockable_manager_connective)
	{
		// 1) Connective Manager를 설정한다.
		pmanager_connective = m_pmanager_connective;
	}

	// 2) Manager가 Empty면 모두 지운다.
	if(pmanager_connective.empty())
	{
		// 1) 모조리 지운다.
		m_list_connective.DeleteAllItems();

		// return) 
		return;
	}

	// 3) List를 다시 setting한다.
	m_list_connective.SetRedraw(FALSE);

	// declare) 
	int index = 0;
	
	scoped_lock(pmanager_connective->get_lockable())
	{
		// 2) managerConnective에서 begin과 end를 얻는다.
		auto iter = pmanager_connective->begin();
		auto iter_end = pmanager_connective->end();

		for(; iter!=iter_end; ++iter, ++index)
		{
			// 3) pconnective를 얻는다.
			auto* pconnective = *iter;

			// check) pconnective가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pconnective == nullptr);

			// 4) 접속종료된 Connectable을 제거한다. 
			while(index <m_list_connective.GetItemCount())
			{
				// - Connectable을 얻는다.
				auto* pconnective_compare = (net::io::Iconnective*)m_list_connective.GetItemData(index);

				// check) 같으면 끝낸다~
				BREAK_IF(pconnective_compare == pconnective);

				// - 지운다~
				m_list_connective.DeleteItem(index);
			}

			// 5) 새로 추가된 것이면 추가한다.
			if(index>=m_list_connective.GetItemCount())
			{
				// - Nameable을 얻는다.
				auto pnameable = dynamic_cast<object::Inameable*>(pconnective);

				// - 값을 추가하고 데이타를 설정한다.
				if(pnameable != nullptr)
				{
				#ifdef _UNICODE
						TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
						encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH);
						m_list_connective.InsertItem(index, text, -1);
						free(text);
				#else
						m_list_connective.InsertItem(index, pnameable->name().data(), -1);
				#endif
				}
				else
				{
					m_list_connective.InsertItem(index, TEXT("No Name"), -1);
				}

				m_list_connective.SetItemData(index, (DWORD_PTR)pconnective);
			}

			// 6) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
			UpdateConnective(index);
		}
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(int	count=m_list_connective.GetItemCount()-index; count>0; count--)
	{
		m_list_connective.DeleteItem(index);
	}

	// 8) Connective의 수를 표시한다.
	SetDlgItemInt(IDC_STATIC_CONNECTIVE_COUNT, static_cast<int>(pmanager_connective->size()));

	// 9) Redraw On
	m_list_connective.SetRedraw(TRUE);
}

void CChild_connective_list::UpdateConnective(int _index)
{
	// check)
	RETURN_IF(_index<0 || _index>=m_list_connective.GetItemCount());

	// 1) Connective를 얻는다.
	auto pconnective = (net::io::Iconnective*)m_list_connective.GetItemData(_index);

	// check) pconnective가 nullptr이면 그냥 되돌린다.
	RETURN_IF(pconnective == nullptr);

	// 2) Nameable을 얻는다.
	auto pnameable = dynamic_cast<object::Inameable*>(pconnective);

	// 3) List를 설정한다.
	TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	LV_ITEM	list_item;

	list_item.mask		 = LVIF_TEXT;
	list_item.state		 = 0;
	list_item.cchTextMax = MAX_STRING_LENGTH;
	list_item.iItem		 = _index;

	// 0	"Name"
	list_item.iSubItem	 = 0;

	if(pnameable!=nullptr)
	{
	#ifdef _UNICODE
		#if defined(_USE_CHAR_STRING)
			list_item.pszText = (LPWSTR)pnameable->name().data();
		#elif defined(_USE_WCHAR_STRING)
			encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH);
			list_item.pszText = text;
		#else
			#error
		#endif
	#else
		#if defined(_USE_CHAR_STRING)
			encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH);
			list_item.pszText = text;
		#elif defined(_USE_WCHAR_STRING)
			list_item.pszText = (LPSTR)pnameable->name().data();
		#else
			#error
		#endif
	#endif
	}
	else
	{
		list_item.pszText = const_cast<TCHAR*>(TEXT("No Name"));
	}
	m_list_connective.SetItem(&list_item);

	// 2) Acceptor를 얻는다.
	auto pacceptor = dynamic_cast<net::io::connective::Iacceptor*>(pconnective);

	// 2	"Address"
	if(pacceptor!=nullptr)
	{
		if(pacceptor->is_binded())
		{
			// 1	"Status"
			list_item.iSubItem		 = 1;
			list_item.pszText		 = const_cast<TCHAR*>(pacceptor->is_listening() ? TEXT("Listen") : TEXT("Binded"));
			m_list_connective.SetItem(&list_item);

			list_item.iSubItem		 = 2;
			auto addr_socket = pacceptor->remote_endpoint();
			if(addr_socket.exist())
			{
				if(addr_socket.is_bind_any())
				{
					wsprintf(text, TEXT("ADDR_ANY:%u"), addr_socket.port());
				}
				else if(addr_socket.is_loopback())
				{
					wsprintf(text, TEXT("localhost:%u"), addr_socket.port());
				}
				else
				{
					to_string<TCHAR>(text, 1024, addr_socket);
				}
			}
			else
			{
				wsprintf(text, TEXT(""));
			}

			list_item.pszText		 = text;
			m_list_connective.SetItem(&list_item);
		}
		else
		{
			// 1	"Status"
			list_item.iSubItem		 = 1;
			list_item.pszText		 = const_cast<TCHAR*>(TEXT("Unbinded"));
			m_list_connective.SetItem(&list_item);

			// 2	"Address"
			list_item.iSubItem		 = 2;
			list_item.pszText		 = const_cast<TCHAR*>(TEXT(""));
			m_list_connective.SetItem(&list_item);
		}
	}
	else
	{
	}


	// 3) Connectable Manager를 얻는다.
	auto pconnectable_manager = dynamic_cast<net::io::Iconnectable_manager*>(pconnective);

	// check) Connectable Manager이 nullptr이 아닐 경우...
	if(pconnectable_manager!=nullptr)
	{
		// 5	"Connected"
		list_item.iSubItem	 = 3;
		wsprintf(text,TEXT("%lld"), static_cast<uint64_t>(pconnectable_manager->count()));
		list_item.pszText	 = text;
		m_list_connective.SetItem(&list_item);
	}
	else
	{
		// 5	"Connected"
		list_item.iSubItem	 = 3;
		list_item.pszText	 = const_cast<TCHAR*>(TEXT(""));
		m_list_connective.SetItem(&list_item);
	}

	free(text);
}

LRESULT CChild_connective_list::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
			}
			break;

	case	CGACCEPTOR_READY:
	case	CGACCEPTOR_CLOSE:
	case	CGACCEPTOR_START:
	case	CGACCEPTOR_STOP:
	case	CGUPDATE:
			{
				UpdateConnectiveList();
			}
			break;
	};

	return	0;
}

void CChild_connective_list::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	// 1) 
	CChildDialog::OnActivate(nState, pWndOther, bMinimized);

	// 2) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_connective_list::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Update한다.
		UpdateConnectiveList();
	}

	CChildDialog::OnTimer(nIDEvent);
}

BOOL CChild_connective_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Acceptor Info
	m_list_connective.InsertColumn(0, TEXT("Name"),		LVCFMT_LEFT,	200, 0);
	m_list_connective.InsertColumn(1, TEXT("Status"),	LVCFMT_LEFT,	 70, 1);
	m_list_connective.InsertColumn(2, TEXT("Address"),	LVCFMT_LEFT,	120, 2);
	m_list_connective.InsertColumn(3, TEXT("Connected"),	LVCFMT_RIGHT,	 80, 3);
	m_list_connective.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}


}