#include "pch.h"
#include "tchar.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_querier_list, CChildDialog)

CChild_querier_list::CChild_querier_list(query::manager_querier* _pquerier_manager /*=NULL*/, CWnd* pParent /*=NULL*/)
	: CChildDialog(IDD_DIALOG_DBQUERIER_LIST, pParent),
	m_pquerier_manager(_pquerier_manager)
{
}

CChild_querier_list::~CChild_querier_list()
{
}

void CChild_querier_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_QUERIER, m_list_querier);
}


BEGIN_MESSAGE_MAP(CChild_querier_list, CChildDialog)
	ON_WM_ENABLE()
	ON_WM_SIZE()
	ON_WM_SIZING()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_querier_list::OnCGMessageNotify)
	ON_BN_CLICKED(IDC_BUTTON_OPEN, &CChild_querier_list::OnBnClickedOpen)
	ON_BN_CLICKED(IDC_BUTTON_OPEN_WITH_PARAM, &CChild_querier_list::OnBnClickedOpenWithParam)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CChild_querier_list::OnBnClickedClose)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()


// CChild_querier_list message handlers
BOOL CChild_querier_list::CreateChild(query::manager_querier* _pquerier_manager, CWnd* p_pWnd)
{
	// declare) 
	BOOL	bResult;

	// 1) Event Manager를 설정한다.
	SetDBQuerierManager(_pquerier_manager);

	// 2) 윈도우를 생성한다.
	bResult	 = Create(IDD_DIALOG_DBQUERIER_LIST, p_pWnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(bResult==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return	TRUE;
}

void CChild_querier_list::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	CWnd*	pWindow	 = nullptr;

	m_list_querier.EnableWindow(is_enable);

	pWindow = GetDlgItem(IDC_STATIC_T2);				if (pWindow != nullptr) pWindow->EnableWindow(is_enable);

	pWindow = GetDlgItem(IDC_BUTTON_CLOSE);				if (pWindow != nullptr) pWindow->EnableWindow(is_enable);

	pWindow = GetDlgItem(IDC_STATIC_T1);				if (pWindow != nullptr) pWindow->EnableWindow(is_enable);
	pWindow = GetDlgItem(IDC_STATIC_QUERIER_COUNT);		if (pWindow != nullptr) pWindow->EnableWindow(is_enable);

	pWindow = GetDlgItem(IDC_BUTTON_OPEN_WITH_PARAM);	if (pWindow != nullptr) pWindow->EnableWindow(is_enable);
	pWindow = GetDlgItem(IDC_BUTTON_OPEN);				if (pWindow != nullptr) pWindow->EnableWindow(is_enable);
}

void CChild_querier_list::OnDestroy()
{
	// 1) Kill Timer
	KillTimer(0);

	// 2) List를 모두 지운다.
	m_list_querier.DeleteAllItems();

	// 3) EventManager를 Reset한다.
	SCOPED_LOCK(m_lockable_querier_manager)
	{
		m_pquerier_manager.reset();
	}

	// 4) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CChild_querier_list::SetDBQuerierManager(query::manager_querier* _pquerier_manager)
{
	// 1) Querier Manager를 설정한다.
	SCOPED_LOCK(m_lockable_querier_manager)
	{
		m_pquerier_manager = _pquerier_manager;
	}

	// 2) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_querier_list::UpdateQuerierManager()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)

	// declare) 
	object_ptr<query::manager_querier> pmanager_querier;

	// 1) Connective Manager를 설정한다.
	SCOPED_LOCK(m_lockable_querier_manager)
	{
		pmanager_querier	 = m_pquerier_manager;
	}

	// 2) Manager가 Empty면 모두 지운다.
	if(pmanager_querier.empty())
	{
		m_list_querier.DeleteAllItems();

		// return) 
		return;
	}
	
	// 3) List를 다시 setting한다.
	m_list_querier.SetRedraw(FALSE);

	// declare) 
	int index = 0;
	
	SCOPED_LOCK(pmanager_querier->get_lockable())
	{
		auto iter = m_pquerier_manager->begin();
		auto iter_end = m_pquerier_manager->end();

		for(; iter!=iter_end; ++iter, ++index)
		{
			// declare) 
			Iquerier* pquerier = *iter;

			// check) pConnective가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pquerier==nullptr);

			// - 접속종료된 Connectable을 제거한다. 
			while(index <m_list_querier.GetItemCount())
			{
				// - Connectable을 얻는다.
				Iquerier* pquerier_compare = (Iquerier*)m_list_querier.GetItemData(index);

				// check) 같으면 끝낸다~
				BREAK_IF(pquerier_compare == pquerier);

				// - 지운다~
				m_list_querier.DeleteItem(index);
			}

			// - 새로 추가된 것이면 추가한다.
			if(index>=m_list_querier.GetItemCount())
			{
				// - 새로 추가한다.
				m_list_querier.InsertItem(index, TEXT(""));
				m_list_querier.SetItemData(index, (DWORD_PTR)pquerier);
			}

			UpdateQuerier(index);
		}
	}

	// 4) 아직도 남은 것들은 모조리 지운다.
	for(int	Count=m_list_querier.GetItemCount()-index; Count>0; Count--)
	{
		m_list_querier.DeleteItem(index);
	}

	// 5) Event Count를 써넣는다.
	SetDlgItemInt(IDC_STATIC_QUERIER_COUNT, m_pquerier_manager->count());
	
	// 6) Redraw Off
	m_list_querier.SetRedraw(TRUE);
}

void CChild_querier_list::UpdateQuerier(int p_iIndex)
{
	// check)
	RETURN_IF(p_iIndex<0 || p_iIndex>=m_list_querier.GetItemCount());

	// 1) Connective를 얻는다.
	Iquerier* pquerier = (Iquerier*)m_list_querier.GetItemData(p_iIndex);

	// check) pConnective가 nullptr이면 그냥 되돌린다.
	RETURN_IF(pquerier == nullptr);

	// 2) List를 설정한다.
	TCHAR		text[256]	 = {};
	LV_ITEM		list_item;

	list_item.mask			 = LVIF_TEXT;
	list_item.state			 = 0;
	list_item.cchTextMax	 = 256;
	list_item.iItem			 = p_iIndex;

	// [0] "Name"
	list_item.iSubItem		 = 0;
	auto* pNameable			 = dynamic_cast<object::Inameable*>(pquerier);

	if(pNameable!=nullptr)
	{
		#ifdef _UNICODE
				list_item.pszText = (TCHAR*)encode_string(text, pNameable->name(), 256).data();
		#else
				list_item.pszText = (TCHAR*)pNameable->name().data();
		#endif
	}
	else
	{
		list_item.pszText	 = const_cast<TCHAR*>(TEXT(""));
	}

	m_list_querier.SetItem(&list_item);

	// [1] "Type"
	list_item.iSubItem		 = 1;
	wsprintf(text, TEXT("%hs"), to_string((eOBJECT_TYPE)pquerier->object_type()).data());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [2] "Status"
	list_item.iSubItem		 = 2;
	switch(pquerier->get_object_state())
	{
	case	eOBJECT_STATE::NONE:
			list_item.pszText = const_cast<TCHAR*>(TEXT("NONE"));
			break;
	case	eOBJECT_STATE::STOPPED:
			list_item.pszText = const_cast<TCHAR*>(TEXT("STOPPING"));
			break;
	case	eOBJECT_STATE::START_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("START_PENDING"));
			break;
	case	eOBJECT_STATE::STOP_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("STOP_PENDING"));
			break;
	case	eOBJECT_STATE::RUNNING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("RUNNING"));
			break;
	case	eOBJECT_STATE::CONTINUE_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("CONTINUE_PENDING"));
			break;
	case	eOBJECT_STATE::PAUSE_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("PAUSE_PENDING"));
			break;
	case	eOBJECT_STATE::PAUSED:
			list_item.pszText = const_cast<TCHAR*>(TEXT("PAUSED"));
			break;
	case	eOBJECT_STATE::INITIALIZING_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("INITIALIZING PENDING"));
			break;
	case	eOBJECT_STATE::DESTROYING_PENDING:
			list_item.pszText = const_cast<TCHAR*>(TEXT("DESTROYING PENDING"));
			break;
	case	eOBJECT_STATE::UPDATE:
			list_item.pszText = const_cast<TCHAR*>(TEXT("UPDATE"));
			break;
	case	eOBJECT_STATE::NOT_CONNECT:
			list_item.pszText = const_cast<TCHAR*>(TEXT("NOT CONNECT"));
			break;
	};
	m_list_querier.SetItem(&list_item);

	// 3) 값을 업데이트한다.
	const auto& pstatistics = pquerier->statistics_get_query_info();

	// [3] "Queued"
	list_item.iSubItem		 = 3;
	_stprintf_s(text, 256, TEXT("%llu"), (uint64_t)0);
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [4] "Try"
	list_item.iSubItem		 = 4;
	_stprintf_s(text, 256, TEXT("%llu"), pstatistics.query_try.load());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [5] "Success"
	list_item.iSubItem		 = 5;
	_stprintf_s(text, 256, TEXT("%llu"), pstatistics.query_success.load());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [6] "Fail"
	list_item.iSubItem		 = 6;
	_stprintf_s(text, 256, TEXT("%llu"), pstatistics.query_fail.load());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [7] "Session"
	list_item.iSubItem		 = 7;
	_stprintf_s(text, 256, TEXT("%llu/%llu"), pquerier->statistics_get_using_session(), pquerier->statistics_get_exist_session());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [8] "Alloced"
	list_item.iSubItem		 = 8;
	_stprintf_s(text, 256, TEXT("%llu"), pstatistics.alloc_session.load());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);

	// [9] "Freed"
	list_item.iSubItem		 = 9;
	_stprintf_s(text, 256, TEXT("%llu"), pstatistics.free_session.load());
	list_item.pszText		 = text;
	m_list_querier.SetItem(&list_item);
}

LRESULT CChild_querier_list::OnCGMessageNotify(WPARAM p_wParam, LPARAM /*p_lParam*/)
{
	switch(p_wParam)
	{
	case	CGUPDATE:
			{
				UpdateQuerierManager();
			}
			break;
	};

	return	0;
}

void CChild_querier_list::OnActivate(UINT _nState, CWnd* _pWndOther, BOOL _bMinimized)
{
	// 1) 
	CChildDialog::OnActivate(_nState, _pWndOther, _bMinimized);

	// 2) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_querier_list::OnSize(UINT _nType, int _cx, int _cy)
{
	__super::OnSize(_nType, _cx, _cy);

	if(m_list_querier.m_hWnd != NULL)
	{
		CRect tempRectOriginal;
		GetClientRect(tempRectOriginal);

		// TODO: Add your message handler code here
		CRect tempRect;
		m_list_querier.GetWindowRect(tempRect);

		ScreenToClient(tempRect);

		tempRect.left	 = 0;
		tempRect.right	 = tempRectOriginal.right;
		tempRect.bottom	 = tempRectOriginal.bottom;

		m_list_querier.MoveWindow(tempRect);
	}
}


void CChild_querier_list::OnSizing(UINT _fwSide, LPRECT _pRect)
{
	CChildDialog::OnSizing(_fwSide, _pRect);

	// TODO: Add your message handler code here
}


void CChild_querier_list::OnTimer(UINT_PTR _nIDEvent)
{
	if(_nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// - 상태를 Update한다.
		UpdateQuerierManager();
	}
	
	CChildDialog::OnTimer(_nIDEvent);
}

BOOL CChild_querier_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// Redraw Off
	m_list_querier.SetRedraw(FALSE);

	// 1) Acceptor Info
	m_list_querier.InsertColumn(0, TEXT("Name"),		LVCFMT_LEFT,	120, 0);
	m_list_querier.InsertColumn(1, TEXT("Type"),		LVCFMT_LEFT,	110, 1);
	m_list_querier.InsertColumn(2, TEXT("Status"),	LVCFMT_LEFT,	 70, 2);
	m_list_querier.InsertColumn(3, TEXT("Queued"),	LVCFMT_RIGHT,	 60, 3);
	m_list_querier.InsertColumn(4, TEXT("Try"),		LVCFMT_RIGHT,	 80, 4);
	m_list_querier.InsertColumn(5, TEXT("Success"),	LVCFMT_RIGHT,	 80, 5);
	m_list_querier.InsertColumn(6, TEXT("Fail"),		LVCFMT_RIGHT,	 60, 6);
	m_list_querier.InsertColumn(7, TEXT("Session"),	LVCFMT_RIGHT,	 60, 7);
	m_list_querier.InsertColumn(8, TEXT("Alloced"),	LVCFMT_RIGHT,	 65, 8);
	m_list_querier.InsertColumn(9, TEXT("Freed"),	LVCFMT_RIGHT,	 65, 9);
	m_list_querier.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// Redraw On
	m_list_querier.SetRedraw(TRUE);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) 설정하기.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);

	// 2) Timer 설정하기
	SetTimer(0, 500, NULL);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChild_querier_list::OnBnClickedOpen()
{
	// declare) 
	object_ptr<query::manager_querier> pmanager_querier;

	SCOPED_LOCK(m_lockable_querier_manager)
	{
		pmanager_querier = m_pquerier_manager;
	}

	// check)
	RETURN_IF(pmanager_querier.empty());

	SCOPED_LOCK(pmanager_querier->get_lockable())
	{
		for(int i=0; i<m_list_querier.GetItemCount(); ++i)
		{
			if(m_list_querier.GetItemState(i, LVIS_SELECTED))
			{
				// - Connectable을 얻는다.
				auto pquerier_compare = (Iquerier*)m_list_querier.GetItemData(i);

				try
				{
					pquerier_compare->start();
				}
				catch(...)
				{

				}
			}
		}
	}
}

void CChild_querier_list::OnBnClickedOpenWithParam()
{
	// declare) 
	object_ptr<query::manager_querier> pmanager_querier;

	SCOPED_LOCK(m_lockable_querier_manager)
	{
		pmanager_querier = m_pquerier_manager;
	}

	SCOPED_LOCK(pmanager_querier->get_lockable())
	{
		for(int i=0; i<m_list_querier.GetItemCount(); ++i)
		{
			if(m_list_querier.GetItemState(i, LVIS_SELECTED))
			{
				// - Connectable을 얻는다.
				auto pquerier_compare = (Iquerier*)m_list_querier.GetItemData(i);

				try
				{
					pquerier_compare->start();
				}
				catch(...)
				{

				}
			}
		}
	}
}

void CChild_querier_list::OnBnClickedClose()
{
	// declare) 
	object_ptr<query::manager_querier> pmanager_querier;

	SCOPED_LOCK(m_lockable_querier_manager)
	{
		pmanager_querier = m_pquerier_manager;
	}

	SCOPED_LOCK(pmanager_querier->get_lockable())
	{
		for(int i=0; i<m_list_querier.GetItemCount(); ++i)
		{
			if(m_list_querier.GetItemState(i, LVIS_SELECTED))
			{
				// - Connectable을 얻는다.
				auto pquerier_compare = (Iquerier*)m_list_querier.GetItemData(i);

				try
				{
					pquerier_compare->stop();
				}
				catch(...)
				{

				}
			}
		}
	}
}


}