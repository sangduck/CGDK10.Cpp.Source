// CGChildThreadList.cpp : implementation file
//

#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace  CGDK
{

IMPLEMENT_DYNAMIC(CChild_schedulable_list, CChildDialog)

CChild_schedulable_list::CChild_schedulable_list(schedulable::manager* _pschedulable_manager, CWnd* pnode_parent)
	: CChildDialog(IDD_DIALOG_SCHEDULABLE_LIST, pnode_parent),
	m_pexecutor_schedulable(_pschedulable_manager),
	m_is_enable_control(TRUE)
{
}

CChild_schedulable_list::~CChild_schedulable_list()
{
}

void CChild_schedulable_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SCHEDULABLE,	m_list_schedulable);
}


BEGIN_MESSAGE_MAP(CChild_schedulable_list, CChildDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_SCHEDULABLE, OnLvnItemchangedListSchedulable)
	ON_BN_CLICKED(IDC_BUTTON_UNREGISTER_SCHEDULABLE, &CChild_schedulable_list::OnBnClickedUnregisterSchedulable)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_schedulable_list::OnCGMessageNotify)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CChild_schedulable_list::CreateChild(schedulable::manager* _pschedulable_manager, CWnd* _pwnd)
{
	// declare) 
	BOOL bResult;

	// 1) Thread Manager를 설정한다.
	SetSchedulableList(_pschedulable_manager);

	// 2) 윈도우를 생성한다.
	bResult	 = Create(IDD_DIALOG_SCHEDULABLE_LIST, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(bResult==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return true;
}

void CChild_schedulable_list::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd()==nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(is_enable & _is_enable);
}

BOOL CChild_schedulable_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Acceptor Info
	m_list_schedulable.InsertColumn(0, TEXT("name"),		LVCFMT_LEFT,	120, 0);
	m_list_schedulable.InsertColumn(1, TEXT("Class"),		LVCFMT_LEFT,	160, 1);
	m_list_schedulable.InsertColumn(2, TEXT("Interval"),	LVCFMT_RIGHT,	 80, 2);
	m_list_schedulable.InsertColumn(3, TEXT("Next"),		LVCFMT_RIGHT,	100, 3);
	m_list_schedulable.InsertColumn(4, TEXT("Concurrent"),	LVCFMT_RIGHT,	100, 4);
	m_list_schedulable.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) Controls
	BOOL	is_enable	 = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_UNREGISTER_SCHEDULABLE)->EnableWindow(is_enable & m_is_enable_control);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return	TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_schedulable_list::OnLvnItemchangedListSchedulable(NMHDR *pNMHDR, LRESULT *presult)
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
		//api::thread*	pthread	 = reinterpret_cast<api::thread*>(m_list_schedulable.GetItemData(pNMLV->iItem));
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)pthread);
	}
	// 4) Item을 해제한 것이다.
	else if(pNMLV->uNewState!=pNMLV->uOldState)
	{
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)NULL);
	}

	*presult = 0;
}

void CChild_schedulable_list::OnNMDblclkListThread(NMHDR* pNMHDR, LRESULT *presult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// 1) ISocket을 얻는다.
	//api::thread*	pthread		 = reinterpret_cast<api::thread*>(m_list_schedulable.GetItemData(pNMItemActivate->iItem));

	// 2) Message를 날린다.
	//SendMessageToTarget((WPARAM)CGACCEPTOR_OPEN_THREAD, (LPARAM)pthread);

	// MFC기본)
	*presult = 0;
}

void CChild_schedulable_list::OnEnable(BOOL _is_enable)
{
	CChildDialog::OnEnable(_is_enable);

	// 1) control 버튼
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(_is_enable & m_is_enable_control);

	// 2) Thread 개수
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(_is_enable);
	GetDlgItem(IDC_STATIC_SCHEDULABLE_COUNT)->EnableWindow(_is_enable);

	// 3) Thread List
	m_list_schedulable.EnableWindow(_is_enable);
}

void CChild_schedulable_list::OnDestroy()
{
	// 1) Thread List를 reset한다.
	ClearSchedulableList();

	// 2) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_schedulable_list::SetSchedulableList(schedulable::manager* _pschedulable_manager)
{
	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager_thread);

		m_pexecutor_schedulable = _pschedulable_manager;
	}

	// 2) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_schedulable_list::ClearSchedulableList()
{
	// lock)
	std::lock_guard lock(this->m_lockable_manager_thread);

	// 1) reset Thread Manager
	this->m_pexecutor_schedulable.reset();
}

void CChild_schedulable_list::UpdateSchedulableList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(this->m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(this->IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Thread Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<schedulable::manager> pschedulable_manager;

	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager_thread);

		pschedulable_manager = m_pexecutor_schedulable;
	}


	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) Thread Manager가 nullptr이면 끝낸다.
	if(pschedulable_manager.empty())
	{
		// 1) 모조리 지운다.
		m_list_schedulable.DeleteAllItems();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_list_schedulable.SetRedraw(FALSE);

	// declare) Index를 0으로...
	int index = 0;

	auto tick_now = chrono::tick::now();

	// 2) Thread list Iterator의 begin과 end를 얻는다.
	SCOPED_LOCK(pschedulable_manager->get_lockable())
	{
		auto iter = pschedulable_manager->begin();
		auto iter_end = pschedulable_manager->end();

		for(; iter != iter_end; ++iter, ++index)
		{
			// 3) pschedulable를 얻는다.
			Ischedulable* pschedulable = *iter;

			// check) pschedulable가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pschedulable==nullptr);

			// 4) 접속종료된 Connectable을 제거한다. 
			while(index <m_list_schedulable.GetItemCount())
			{
				// - Connectable을 얻는다.
				auto* pSchedulableCompare = reinterpret_cast<Ischedulable*>(m_list_schedulable.GetItemData(index));

				// check) 같으면 끝낸다~
				BREAK_IF(pSchedulableCompare == pschedulable);

				// - 지운다.
				m_list_schedulable.DeleteItem(index);
			}

			// 5) 새로 추가된 것이면 추가한다.
			if(index>=m_list_schedulable.GetItemCount())
			{
				m_list_schedulable.InsertItem(index, TEXT(" -"), -1);
				m_list_schedulable.SetItemData(index, (DWORD_PTR)pschedulable);
			}

			// 6) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
			UpdateSchedulableList(index, tick_now);
		}
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(size_t	Count=m_list_schedulable.GetItemCount()-index; Count>0; Count--)
	{
		// -Item을 지운다.
		m_list_schedulable.DeleteItem(index);
	}

	// 8) Thread의 개수를 표시한다.
	SetDlgItemInt(IDC_STATIC_SCHEDULABLE_COUNT, static_cast<int>(pschedulable_manager->size()));

	// 9) Redraw On
	m_list_schedulable.SetRedraw(TRUE);
}

void CChild_schedulable_list::UpdateSchedulableList(int _index, chrono::tick::time_point /*_tick_now*/)
{
	// check)
	RETURN_IF(_index<0 || _index>=m_list_schedulable.GetItemCount());

	// 1) Thread를 얻는다.
	auto* pschedulable = reinterpret_cast<Ischedulable*>(m_list_schedulable.GetItemData(_index));

	// check) pschedulable가 nullptr이면 그냥 되돌린다. 
	RETURN_IF(pschedulable == nullptr);

	// 2) get nameable
	object::Inameable* pnameable = dynamic_cast<object::Inameable*>(pschedulable);

	// 3) List를 설정한다.
	TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	LV_ITEM list_item;

	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = MAX_STRING_LENGTH;
	list_item.iItem = _index;

	// 0	"name"
	list_item.iSubItem = 0;
#ifdef _UNICODE
	list_item.pszText = const_cast<LPWSTR>((pnameable != nullptr) ? encode_string(text, pnameable->name().data(), MAX_STRING_LENGTH).data() : TEXT(" - "));
#else
	list_item.pszText = const_cast<TCHAR*>((pnameable != nullptr) ? pnameable->name().data() : TEXT(" - "));
#endif

	m_list_schedulable.SetItem(&list_item);

	// 1. "Type"
	list_item.iSubItem = 1;
	std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%hs")).data(), typeid(*pschedulable).name());
	for(int i=0;text[i]!=NULL;++i)	if(text[i]=='*') {text[i]=NULL;break;}
	list_item.pszText = &text[13];
	m_list_schedulable.SetItem(&list_item);

	auto* pschedulable_executable = dynamic_cast<Nschedulable*>(pschedulable);

	if (pschedulable_executable!=nullptr)
	{
		// - Interval을 구한다.
		auto millisecInterval = std::chrono::duration_cast<chrono::tick::milliseconds>(pschedulable_executable->execute_interval()).count();

		if(millisecInterval<1000)
		{
			std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%I64ums")).data(), millisecInterval);
		}
		else
		{
			auto secInterval = millisecInterval/1000;
			millisecInterval %= 1000;

			if(millisecInterval != 0)
			{
				std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%I64usec %I64ums")).data(), secInterval, millisecInterval);
			}
			else
			{
				std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%I64usec")).data(), secInterval);
			}
		}

		// 2. "Interval"
		list_item.iSubItem = 2;
		list_item.pszText = text;
		m_list_schedulable.SetItem(&list_item);

		//const auto& timeNext = pschedulable_executable->next_tick();
		//std::swprintf(text, std::size(text), std::wstring_view(TEXT("%s")).data(), timeNext.);

		// 3. "Next Tick"
		list_item.iSubItem = 3;
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
		m_list_schedulable.SetItem(&list_item);
	}
	else
	{
		// 2. "Interval"
		list_item.iSubItem = 2;
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
		m_list_schedulable.SetItem(&list_item);

		// 3. "Next Tick"
		list_item.iSubItem = 3;
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
		m_list_schedulable.SetItem(&list_item);
	}

	// 2. 
	Iexecutable* pexecutable = dynamic_cast<Iexecutable*>(pschedulable);

	// 4 "Concurrent Execution"
	list_item.iSubItem = 4;
	if (pexecutable != nullptr)
	{
		std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%d")).data(), 0);
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
	}
	m_list_schedulable.SetItem(&list_item);

	free(text);
}

LRESULT CChild_schedulable_list::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateSchedulableList();
			}
			break;
	};

	return	0;
}

void CChild_schedulable_list::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CChildDialog::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_schedulable_list::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		UpdateSchedulableList();
	}

	//-----------------------------------------------------------------
	// 2. MFC Default
	//-----------------------------------------------------------------
	CChildDialog::OnTimer(nIDEvent);
}


void CChild_schedulable_list::OnBnClickedUnregisterSchedulable()
{
	////-----------------------------------------------------------------
	//// 1. Save할 File이름 얻어오기
	////-----------------------------------------------------------------
	//// declare) 
	//INT_PTR		presult;
	//TCHAR		tempFileName[MAX_STRING_LENGTH]	 = {NULL, };

	//CFileDialog	dlg(FALSE, TEXT("Save Dump"), NULL, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXPLORER, TEXT("Dump File (*.dmp)|*.dmp|All Files (*.*)|*.*||"));
	//dlg.m_ofn.lpstrTitle		= TEXT("Save Dump");
	//dlg.m_ofn.lpstrFile			= tempFileName;

	//// 2) Do Modal해서 창을 띄운다.
	//presult	 = dlg.DoModal();

	//// check) IDOK가 아니면 그냥 끝낸다.
	//RETURN_IF(presult!=IDOK);


	////-----------------------------------------------------------------
	//// 2. Dump를 남긴다.
	////-----------------------------------------------------------------
	//// declare) 
	//bool	result;

	//// 1) Dump를 남긴다.
	//result = CGDK::snap_dump(tempFileName);

	//// check) 
	//if(result==false)
	//{
	//	AfxMessageBox(TEXT("Fail to snap Mini dump"), MB_OK);
	//}
}


}