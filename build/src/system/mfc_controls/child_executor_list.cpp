// CGChildExecutorList.cpp : implementation file
//

#include "pch.h"
#include <cinttypes>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_executor_list, CChildDialog)

CChild_executor_list::CChild_executor_list(executor::manager* _pexecutor_manager, CWnd* pnode_parent)
	: CChildDialog(IDD_DIALOG_EXECUTOR_LIST, pnode_parent),
	m_pmanager_executor(_pexecutor_manager),
	m_is_enable_control(TRUE)
{
}

CChild_executor_list::~CChild_executor_list()
{
}

void CChild_executor_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_EXECUTOR, m_listctrl_executor);
}

BEGIN_MESSAGE_MAP(CChild_executor_list, CChildDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_THREAD, OnLvnItemchangedListExecutor)
	ON_BN_CLICKED(IDC_BUTTON_FORCE_EXCEPTION, &CChild_executor_list::OnBnClickedForceException)
	ON_BN_CLICKED(IDC_BUTTON_SNAP_DUMP, &CChild_executor_list::OnBnClickedSnapDump)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_executor_list::OnCGMessageNotify)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CChild_executor_list::CreateChild(executor::manager* _pexecutor_manager, CWnd* _pwnd)
{
	// 1) executor Manager를 설정한다.
	SetExecutorManager(_pexecutor_manager);

	// 2) 윈도우를 생성한다.
	auto bResult = Create(IDD_DIALOG_EXECUTOR_LIST, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(bResult==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return true;
}

void CChild_executor_list::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd() == nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(is_enable & _is_enable);
}

BOOL CChild_executor_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) acceptor info
	m_listctrl_executor.InsertColumn(0, TEXT("name"),		LVCFMT_LEFT,	150, 0);
	m_listctrl_executor.InsertColumn(1, TEXT("Type"),		LVCFMT_LEFT,	160, 1);
	m_listctrl_executor.InsertColumn(2, TEXT("Threads"),	LVCFMT_RIGHT,	 80, 2);
	m_listctrl_executor.InsertColumn(3, TEXT("Executed"),	LVCFMT_RIGHT,	120, 3);
	m_listctrl_executor.InsertColumn(4, TEXT("Pended"),		LVCFMT_RIGHT,	100, 4);
	m_listctrl_executor.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) controls
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(is_enable & m_is_enable_control);

	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);

	// return) 
	return TRUE;
}

void CChild_executor_list::OnLvnItemchangedListExecutor(NMHDR *pNMHDR, LRESULT *presult)
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
		//Iexecutor* pexecutor = reinterpret_cast<Iexecutor*>(m_listctrl_executor.GetItemData(pNMLV->iItem));
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)pexecutor);
	}
	// 4) Item을 해제한 것이다.
	else if(pNMLV->uNewState!=pNMLV->uOldState)
	{
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)NULL);
	}

	*presult = 0;
}

void CChild_executor_list::OnNMDblclkListExecutor(NMHDR* pNMHDR, LRESULT *presult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// 1) ISocket을 얻는다.
	//auto pexecutor = reinterpret_cast<Iexecutor*>(m_listctrl_executor.GetItemData(pNMItemActivate->iItem));

	// 2) Message를 날린다.
	//SendMessageToTarget((WPARAM)CGACCEPTOR_OPEN_THREAD, (LPARAM)pexecutor);

	// MFC기본)
	*presult = 0;
}

void CChild_executor_list::OnEnable(BOOL _is_enable)
{
	CChildDialog::OnEnable(_is_enable);

	// 1) control 버튼
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(_is_enable & m_is_enable_control);

	// 2) executor 개수
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(_is_enable);
	GetDlgItem(IDC_STATIC_THREAD_COUNT)->EnableWindow(_is_enable);

	// 3) executor List
	m_listctrl_executor.EnableWindow(_is_enable);
}

void CChild_executor_list::OnDestroy()
{
	// 1) executor List를 reset한다.
	ClearExecutorList();

	// 2) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_executor_list::SetExecutorManager(executor::manager* _pexecutor_manager)
{
	{
		// lock)
		std::lock_guard lock(m_lockable_manager_executor);

		// 1) executor Manager를 설정한다.
		m_pmanager_executor = _pexecutor_manager;
	}

	// 2) Update한다.
	if(m_hWnd != nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_executor_list::ClearExecutorList()
{
	// lock)
	std::lock_guard lock(m_lockable_manager_executor);

	// 1) reset한다.
	m_pmanager_executor.reset();
}

void CChild_executor_list::UpdateExecutorList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. executor Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<executor::manager> pmanager_executor;

	// 1) executor Manager를 설정한다.
	{
		// lock) 
		std::lock_guard lock(m_lockable_manager_executor);

		// - set
		pmanager_executor = m_pmanager_executor;
	}

	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) executor Manager가 nullptr이면 끝낸다.
	if(pmanager_executor.empty())
	{
		// 1) 모조리 지운다.
		m_listctrl_executor.DeleteAllItems();

		// return) 
		return;
	}

	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_listctrl_executor.SetRedraw(FALSE);

	// declare) Index를 0으로...
	int index = 0;

	auto tick_now = chrono::tick::now();

	SCOPED_UNIQUE_LOCK(pmanager_executor->get_lockable())
	{
		// 2) executor list Iterator의 begin과 end를 얻는다.
		auto iter = pmanager_executor->begin();
		auto iter_end = pmanager_executor->end();

		for(; iter!=iter_end; ++iter, ++index)
		{
			// 3) pexecutor를 얻는다.
			const Iexecutor* pexecutor = *iter;

			// check) pexecutor가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pexecutor==nullptr);

			// 4) 접속종료된 Connectable을 제거한다. 
			while(index < m_listctrl_executor.GetItemCount())
			{
				// - Connectable을 얻는다.
				const Iexecutor* pexecutor_compare = reinterpret_cast<Iexecutor*>(m_listctrl_executor.GetItemData(index));

				// check) 같으면 끝낸다~
				BREAK_IF(pexecutor_compare == pexecutor);

				// - 지운다~
				m_listctrl_executor.DeleteItem(index);
			}

			// 5) 새로 추가된 것이면 추가한다.
			if(index>= m_listctrl_executor.GetItemCount())
			{
				m_listctrl_executor.InsertItem(index, TEXT(""), -1);
				m_listctrl_executor.SetItemData(index, (DWORD_PTR)pexecutor);
			}

			// 6) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
			UpdateExecutorList(index, tick_now);
		}
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(size_t	Count= m_listctrl_executor.GetItemCount()-index; Count>0; Count--)
	{
		// -Item을 지운다.
		m_listctrl_executor.DeleteItem(index);
	}

	// 8) Thread의 개수를 표시한다.
	SetDlgItemInt(IDC_STATIC_THREAD_COUNT, static_cast<int>(pmanager_executor->size()));

	// 9) Redraw On
	m_listctrl_executor.SetRedraw(TRUE);
}

void CChild_executor_list::UpdateExecutorList(int _index, chrono::tick::time_point /*_tick_now*/)
{
	// check)
	RETURN_IF(_index < 0 || _index >= m_listctrl_executor.GetItemCount());

	// 1) Thread를 얻는다.
	auto pexecutor = reinterpret_cast<Iexecutor*>(m_listctrl_executor.GetItemData(_index));

	// check) pexecutor가 nullptr이면 그냥 되돌린다.
	RETURN_IF(pexecutor == nullptr);

	// 2) Statistics
	//const statistics::thread&	rStatistics	 = pexecutor->GetThreadStatistics();
	auto* pnameable = dynamic_cast<object::Inameable*>(pexecutor);
	auto* pobject_identifiable = dynamic_cast<object::Iidentifiable*>(pexecutor);

	// 3) List를 설정한다.
	TCHAR* text  = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	LV_ITEM list_item;

	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = 1024;
	list_item.iItem = _index;

	// 0."name"
	list_item.iSubItem = 0;
#ifdef _UNICODE
	list_item.pszText = const_cast<LPWSTR>((pnameable != nullptr) ? encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH).data() : TEXT(" - "));
#else
	list_item.pszText = const_cast<TCHAR*>((pnameable != nullptr) ? pnameable->name().data() : TEXT(" - "));
#endif
	m_listctrl_executor.SetItem(&list_item);

	// 1."type"
	list_item.iSubItem = 1;
	if (pnameable != nullptr)
	{
		wsprintf(text, TEXT("%hs"), to_string(pobject_identifiable->object_type()).data());
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("N/A"));
	}
	m_listctrl_executor.SetItem(&list_item);

	// - get statistics
	auto& statistics = pexecutor->statistics();

	// 2."threads"
	list_item.iSubItem = 2;
	if (statistics.now.thread_count != 0)
	{
		std::swprintf(text, 1024, L"%" PRIu64, static_cast<uint64_t>(statistics.now.thread_count.load()));
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
	}
	m_listctrl_executor.SetItem(&list_item);
	
	// 3."executed"
	list_item.iSubItem = 3;
	std::swprintf(text, 1024, L"%" PRIu64, static_cast<uint64_t>(statistics.now.count_execute.load()));
	list_item.pszText = text;
	m_listctrl_executor.SetItem(&list_item);

	// 4."pended"
	list_item.iSubItem = 4;
	auto pqueue = dynamic_cast<executor::Iqueue*>(pexecutor);

	if (pqueue != nullptr)
	{
		std::swprintf(text, 1024, L"%" PRIu64, static_cast<uint64_t>(pqueue->pended_count()));
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
	}
	m_listctrl_executor.SetItem(&list_item);

	//// 5."Executing"
	//list_item.iSubItem = 5;
	//if(rStatistics.is_executing)
	//{
	//	I64TOSTR_S(std::chrono::duration_cast<chrono::milliseconds>(_tick_now-rStatistics.tick_last_execute).count(), text, 256, 10);
	//}
	//else
	//{
	//	list_item.pszText = TEXT("-");
	//}

	m_listctrl_executor.SetItem(&list_item);

	free(text);
}

LRESULT CChild_executor_list::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateExecutorList();
			}
			break;
	};

	return	0;
}

void CChild_executor_list::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CChildDialog::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_executor_list::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		if(IsWindowEnabled() == FALSE)
			return;

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		if(IsWindowVisible() == FALSE)
			return;

		UpdateExecutorList();
	}

	//-----------------------------------------------------------------
	// 2. MFC Default
	//-----------------------------------------------------------------
	CChildDialog::OnTimer(nIDEvent);
}

void CChild_executor_list::OnBnClickedForceException()
{
	// declare) 
	object_ptr<executor::manager> pmanager_executor;

	// 1) Thread Manager를 설정한다.
	SCOPED_LOCK(m_lockable_manager_executor)
	{
		pmanager_executor = m_pmanager_executor;
	}

	// check) 
	RETURN_IF(pmanager_executor.empty());

	// 2) 선택된 것을 force_raise_exception한다.
	SCOPED_LOCK(pmanager_executor->get_lockable())
	{
		POSITION pos = m_listctrl_executor.GetFirstSelectedItemPosition();
		while(pos)
		{
			auto nItem = this->m_listctrl_executor.GetNextSelectedItem(pos);

			// 1) Connectable을 얻는다.
			auto pexecutor = reinterpret_cast<Iexecutor*>(this->m_listctrl_executor.GetItemData(nItem));

			// check) pConnectable이 nullptr일 수는 없다.
			CGASSERT_FATAL(pexecutor != nullptr);
		
			// 2) Thread를 강재로 force_raise_exception한다.
			SCOPED_LOCK(pexecutor->get_lockable())
			{
				for (auto& iter : pexecutor->get_threads())
					iter->force_raise_exception("Force raise exception");
			}
		}
	}
}

void CChild_executor_list::OnBnClickedSnapDump()
{
	//-----------------------------------------------------------------
	// 1. Save할 File이름 얻어오기
	//-----------------------------------------------------------------
	// declare) 
	INT_PTR presult;
	TCHAR tempFileName[MAX_STRING_LENGTH] {NULL, };

	CFileDialog	dlg(FALSE, TEXT("Save Dump"), NULL, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXPLORER, TEXT("Dump File (*.dmp)|*.dmp|All Files (*.*)|*.*||"));
	dlg.m_ofn.lpstrTitle = TEXT("Save Dump");
	dlg.m_ofn.lpstrFile = tempFileName;

	// 2) Do Modal해서 창을 띄운다.
	presult = dlg.DoModal();

	// check) IDOK가 아니면 그냥 끝낸다.
	if(presult!=IDOK)
		return;
	
	//-----------------------------------------------------------------
	// 1. Dump를 남긴다.
	//-----------------------------------------------------------------
	// 1) Snap Dump를 남긴다.
	auto result = CGDK::snap_dump(std::string(CT2A(tempFileName)));

	// check) 
	if(result==false)
		AfxMessageBox(TEXT("Fail to snap Mini dump"), MB_OK);
}


}