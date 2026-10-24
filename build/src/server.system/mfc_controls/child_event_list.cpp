#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_event_list, CChildDialog)

CChild_event_list::CChild_event_list(time_event::entity_manager* _pentity_manager /*=nullptr*/, CWnd* pParent /*=nullptr*/)
	: CChildDialog(IDD_DIALOG_EVENT_LIST, pParent),
	m_bEnableControl(TRUE)
{
	this->SetEventManager(_pentity_manager);
}

CChild_event_list::~CChild_event_list()
{
}

void CChild_event_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_EVENT, m_listctrlEvent);
}

BEGIN_MESSAGE_MAP(CChild_event_list, CChildDialog)
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_DELETE_EVENT, &CChild_event_list::OnBnClickedButtonDeleteEvent)
	ON_BN_CLICKED(IDC_BUTTON_make_object_EVENT, &CChild_event_list::OnBnClickedButtonNewEvent)
	ON_BN_CLICKED(IDC_CHECK_DELETE_DONE_EVENT, &CChild_event_list::OnBnClickedCheckDeleteDoneEvent)
	ON_BN_CLICKED(IDC_BUTTON_suspend_time_event, &CChild_event_list::OnBnClickedButtonSuspendEvent)
	ON_BN_CLICKED(IDC_BUTTON_resume_time_event, &CChild_event_list::OnBnClickedButtonResumeEvent)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_EVENT, &CChild_event_list::OnNMDblclkListEvent)
END_MESSAGE_MAP()

BOOL CChild_event_list::CreateChild(time_event::entity_manager* _pentity_manager, CWnd* _pWnd)
{
	// 1) Event Manager를 설정한다.
	this->SetEventManager(_pentity_manager);

	// 2) 윈도우를 생성한다.
	auto result = this->Create(IDD_DIALOG_EVENT_LIST, _pWnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	this->ShowWindow(SW_SHOW);

	// return) 성공!!!
	return TRUE;
}


void CChild_event_list::SetEventManager(time_event::entity_manager* _pentity_manager)
{
	// 1) 값을 설정한다.
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		this->m_pentity_manager = _pentity_manager;
	}

	// 2) Update한다.
	if(this->m_hWnd != nullptr)
	{
		this->PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_event_list::EnableControl(BOOL _bEnable)
{
	this->m_bEnableControl = _bEnable;

	RETURN_IF(this->GetSafeHwnd() == nullptr);

	BOOL is_enable = this->IsWindowEnabled();

	this->GetDlgItem(IDC_BUTTON_make_object_EVENT)->EnableWindow(_bEnable && is_enable);
	this->GetDlgItem(IDC_BUTTON_DELETE_EVENT)->EnableWindow(_bEnable && is_enable);
	this->GetDlgItem(IDC_BUTTON_suspend_time_event)->EnableWindow(_bEnable && is_enable);
	this->GetDlgItem(IDC_BUTTON_resume_time_event)->EnableWindow(_bEnable && is_enable);
	this->GetDlgItem(IDC_CHECK_DELETE_DONE_EVENT)->EnableWindow(_bEnable && is_enable);
}
		   

void CChild_event_list::UpdateEvent(int _iIndex)
{
	// check)
	RETURN_IF(_iIndex<0 || _iIndex >= this->m_listctrlEvent.GetItemCount());

	// 1) Connective를 얻는다.
	auto* pevent = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(_iIndex);

	// check) pConnective가 nullptr이면 그냥 되돌린다.
	RETURN_IF(pevent == nullptr);

	// 2) List를 설정한다.
	TCHAR	text[256];
	LV_ITEM	list_item;

	list_item.mask			 = LVIF_TEXT;
	list_item.state			 = 0;
	list_item.cchTextMax	 = 256;
	list_item.iItem			 = _iIndex;

	// [0] "Name"
	list_item.iSubItem		 = 0;
	list_item.pszText		 = (TCHAR*)pevent->name().data();
	this->m_listctrlEvent.SetItem(&list_item);

	// [1] "Type"
	list_item.iSubItem		 = 1;
	wsprintf(text, TEXT("%llu"), pevent->entity_setting().type);
	list_item.pszText		 = text;
	this->m_listctrlEvent.SetItem(&list_item);


	// [2] "State"
	list_item.iSubItem		 = 2;
	wsprintf(text, TEXT("%hs"), to_string(pevent->entity_status().state).data());
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);

	// [3] "Setter"
	list_item.iSubItem		 = 3;
	wsprintf(text, TEXT("%hs"), to_string(pevent->entity_setting().setter).data());
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);

	// [4] "Set Time"
	list_item.iSubItem		 = 4;
	{
		auto time_setup = pevent->entity_status().time_setup;

		if(time_setup != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			// - convert to tm
			auto temp_tm = chrono::to_tm(time_setup);

			// - to_string
			wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec);
			list_item.pszText = text;
		}
		else
		{
			list_item.pszText = const_cast<TCHAR*>(TEXT("-"));
		}
	}
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);

	// [5] "Next Time"
	list_item.iSubItem = 5;
	{
		// - get next time
		auto time_next = pevent->entity_status().time_next;

		// - 
		if (time_next != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			// - convert to tm
			auto temp_tm = chrono::to_tm(time_next);

			// - to string
			wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"),
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec);
			list_item.pszText = text;
		}
		else
		{
			list_item.pszText = const_cast<TCHAR*>(TEXT("-"));
		}
	}
	this->m_listctrlEvent.SetItem(&list_item);

	// [6] "Try Time"
	list_item.iSubItem = 6;
	{
		chrono::time::local::time_point	time_executed = pevent->entity_status().time_last_try;

		if(time_executed != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			// - convert to tm
			auto temp_tm = chrono::to_tm(time_executed);

			// - to string
			wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec);
			list_item.pszText = text;
		}
		else
		{
			list_item.pszText = const_cast<TCHAR*>(TEXT("-"));
		}
	}
	this->m_listctrlEvent.SetItem(&list_item);

	// [7] "Try"
	list_item.iSubItem = 7;
	wsprintf(text, TEXT("%lld"), pevent->entity_status().count_try.load());
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);

	// [8] "Exe Time"
	list_item.iSubItem = 8;
	{
		chrono::time::local::time_point	time_executed = pevent->entity_status().time_last_succeeded;

		if (time_executed != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			auto temp_tm = chrono::to_tm(time_executed);

			wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
				temp_tm.tm_year,
				temp_tm.tm_mon,
				temp_tm.tm_mday,
				temp_tm.tm_hour,
				temp_tm.tm_min,
				temp_tm.tm_sec);
			list_item.pszText = text;
		}
		else
		{
			list_item.pszText = const_cast<TCHAR*>(TEXT("-"));
		}
	}
	this->m_listctrlEvent.SetItem(&list_item);
				
	// [9] "Succeeded"
	list_item.iSubItem = 9;
	wsprintf(text, TEXT("%lld"), pevent->entity_status().count_succeeded.load());
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);

	// [10] "Failed"
	list_item.iSubItem = 10;
	wsprintf(text, TEXT("%lld"), pevent->entity_status().count_failed.load());
	list_item.pszText = text;
	this->m_listctrlEvent.SetItem(&list_item);
}

void CChild_event_list::UpdateEventManager()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(this->m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(this->IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Event Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<time_event::entity_manager> pentity_manager;
	
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		pentity_manager = this->m_pentity_manager;
	}


	//-----------------------------------------------------------------
	// 3. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	if(pentity_manager.empty())
	{
		// 1) 모조리 지운다.
		this->m_listctrlEvent.DeleteAllItems();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	this->m_listctrlEvent.SetRedraw(FALSE);

	// declare) 
	int index = 0;
	
	SCOPED_LOCK(pentity_manager->get_lockable())
	{
		// 2) Event Manager의 Iterator의 begin과 end를 얻는다.
		auto iter = pentity_manager->begin();
		auto iter_end = pentity_manager->end();

		for(; iter!=iter_end; ++iter, ++index)
		{
			// 3) pevent를 얻는다.
			const time_event::Ientity* pevent = *iter;

			// check) pConnective가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pevent == nullptr);

			// 4) 접속종료된 Connectable을 제거한다. 
			while(index < this->m_listctrlEvent.GetItemCount())
			{
				// - Connectable을 얻는다.
				time_event::Ientity* pEventCompare = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(index);

				// check) 같으면 끝낸다~
				BREAK_IF(pEventCompare == pevent);

				// - 지운다~
				this->m_listctrlEvent.DeleteItem(index);
			}

			// 5) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
			if(index>= this->m_listctrlEvent.GetItemCount())
			{
				// - 새로 추가한다.
				this->m_listctrlEvent.InsertItem(index, TEXT(""));
				this->m_listctrlEvent.SetItemData(index, (DWORD_PTR)pevent);
			}

			// 6) 값을 업데이트한다.
			this->UpdateEvent(index);
		}
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(int	Count= this->m_listctrlEvent.GetItemCount()-index; Count>0; Count--)
	{
		this->m_listctrlEvent.DeleteItem(index);
	}

	// 8) Event Count를 표시한다.
	this->SetDlgItemInt(IDC_STATIC_EVENT_COUNT, static_cast<int>(pentity_manager->size()));

	// 9) IDC_CHECK_DELETE_DONE_EVENT Check Box를 설정한다.
	((CButton*)GetDlgItem(IDC_CHECK_DELETE_DONE_EVENT))->SetCheck(pentity_manager->is_delete_done_entity() ? BST_CHECKED : BST_UNCHECKED);
	
	// 10) Redraw Off
	this->m_listctrlEvent.SetRedraw(TRUE);
}


BOOL CChild_event_list::OnInitDialog()
{
	this->CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Acceptor Info
	this->m_listctrlEvent.InsertColumn(0, TEXT("Name"),					LVCFMT_LEFT,	200, 0);
	this->m_listctrlEvent.InsertColumn(1, TEXT("Type"),					LVCFMT_LEFT,	 90, 1);
	this->m_listctrlEvent.InsertColumn(2, TEXT("Status"),				LVCFMT_LEFT,	 70, 2);
	this->m_listctrlEvent.InsertColumn(3, TEXT("Setter"),				LVCFMT_LEFT,	 60, 3);
	this->m_listctrlEvent.InsertColumn(4, TEXT("Set Time"),				LVCFMT_LEFT,	120, 4);
	this->m_listctrlEvent.InsertColumn(5, TEXT("Next Time"),			LVCFMT_LEFT,	120, 5);
	this->m_listctrlEvent.InsertColumn(6, TEXT("Last Try Time "),		LVCFMT_LEFT,	120, 6);
	this->m_listctrlEvent.InsertColumn(7, TEXT("Try"),					LVCFMT_RIGHT,	 40, 7);
	this->m_listctrlEvent.InsertColumn(8, TEXT("Last Succeeded Time "),	LVCFMT_LEFT,	120, 8);
	this->m_listctrlEvent.InsertColumn(9, TEXT("Success"),				LVCFMT_RIGHT,	 40, 9);
	this->m_listctrlEvent.InsertColumn(10,TEXT("Fail"),					LVCFMT_RIGHT,	 40, 10);
	this->m_listctrlEvent.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) enable
	BOOL is_enable = this->IsWindowEnabled();
	this->GetDlgItem(IDC_BUTTON_make_object_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_DELETE_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_suspend_time_event)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_resume_time_event)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_CHECK_DELETE_DONE_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) 설정하기.
	this->PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);

	// 2) Timer 설정하기
	this->SetTimer(0, 500, NULL);

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_event_list::OnEnable(BOOL is_enable)
{
	this->CChildDialog::OnEnable(is_enable);

	this->GetDlgItem(IDC_STATIC_EVENT_COUNT)->EnableWindow(is_enable);

	this->GetDlgItem(IDC_BUTTON_make_object_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_DELETE_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_suspend_time_event)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_BUTTON_resume_time_event)->EnableWindow(this->m_bEnableControl && is_enable);
	this->GetDlgItem(IDC_CHECK_DELETE_DONE_EVENT)->EnableWindow(this->m_bEnableControl && is_enable);

	this->m_listctrlEvent.EnableWindow(is_enable);

	this->GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
}

void CChild_event_list::OnDestroy()
{
	// 1) EventManager를 reset한다.
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		this->m_pentity_manager.reset();
	}

	// 2) 부모 Class의 on_destroy
	this->__super::OnDestroy();
}

LRESULT CChild_event_list::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
	case	CGACCEPTOR_READY:
	case	CGACCEPTOR_CLOSE:
	case	CGACCEPTOR_START:
	case	CGACCEPTOR_STOP:
			break;
	case	CGUPDATE:
			{
				this->UpdateEventManager();
			}
			break;
	};

	return	0;

}

void CChild_event_list::OnSizing(UINT fwSide, LPRECT pRect)
{
	this->__super::OnSizing(fwSide, pRect);

	// 1) Width의 한계를 설정한다.

	// 2) Height의 한계를 설정한다.
}

void CChild_event_list::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if(this->m_listctrlEvent.m_hWnd!=NULL)
	{
		CRect tempRectOriginal;
		this->GetClientRect(tempRectOriginal);

		// TODO: Add your message handler code here
		CRect tempRect;
		this->m_listctrlEvent.GetWindowRect(tempRect);

		this->ScreenToClient(tempRect);

		tempRect.left = 0;
		tempRect.right = tempRectOriginal.right;
		tempRect.bottom = tempRectOriginal.bottom;

		this->m_listctrlEvent.MoveWindow(tempRect);
	}
}
void CChild_event_list::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(this->IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(this->IsWindowVisible() == FALSE)

		// 1) 상태를 Update한다.
			this->UpdateEventManager();
	}
	
	this->CChildDialog::OnTimer(nIDEvent);
}

void CChild_event_list::OnBnClickedButtonNewEvent()
{
}

void CChild_event_list::OnBnClickedButtonDeleteEvent()
{
	// declare) 
	object_ptr<time_event::entity_manager> pentity_manager;
	
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		// - setting...
		pentity_manager = this->m_pentity_manager;
	}

	// check) pEventManagr가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pentity_manager.empty())

	// Redraw Off
	this->m_listctrlEvent.SetRedraw(FALSE);

	for(int i=0 ;i< this->m_listctrlEvent.GetItemCount() ;++i)
	{
		if(this->m_listctrlEvent.GetItemState(i, LVIS_SELECTED))
		{
			// 1) Connectable을 얻는다.
			auto* pobject = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(i);

			// check) pobject가 nullptr일 수는 없다.
			CGASSERT_ERROR(pobject != nullptr);

			// 2) Event 객체를 unregister한다.
			pentity_manager->unregister_entity(pobject);

			// 3) Item을 지운다.
			this->m_listctrlEvent.DeleteItem(i);

			// 4) 지웠으니 하나를 다시 물린다.(++될 것을 감안한 것임...)
			--i;
		}
	}

	// 6) Event Count를 써넣는다.
	this->SetDlgItemInt(IDC_STATIC_EVENT_COUNT, static_cast<int>(pentity_manager->size()));

	// Redraw Off
	this->m_listctrlEvent.SetRedraw(TRUE);
}

void CChild_event_list::OnBnClickedCheckDeleteDoneEvent()
{
	// declare) 
	object_ptr<time_event::entity_manager> pentity_manager;
	
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		// - setting...
		pentity_manager = this->m_pentity_manager;
	}

	// check) pentity_manager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pentity_manager.empty())

	// 1) DeleteDonEvent 상태를 설정한다.
	SCOPED_LOCK(pentity_manager->get_lockable())
	{
		pentity_manager->enable_delete_done_entity(((CButton*)GetDlgItem(IDC_CHECK_DELETE_DONE_EVENT))->GetCheck() == BST_CHECKED);
	}
}

void CChild_event_list::OnBnClickedButtonSuspendEvent()
{
	// declare) 
	object_ptr<time_event::entity_manager> pentity_manager;
	
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		// - setting...
		pentity_manager = this->m_pentity_manager;
	}

	// check) pentity_manager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pentity_manager.empty())

	SCOPED_LOCK(pentity_manager->get_lockable())
	{
		POSITION pos = this->m_listctrlEvent.GetFirstSelectedItemPosition();
		while(pos)
		{
			int	nItem = this->m_listctrlEvent.GetNextSelectedItem(pos);

			// 1) Connectable을 얻는다.
			auto* pobject_find = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(nItem);

			// check) pobject_find이 nullptr일 수는 없다.
			CGASSERT_FATAL(pobject_find != nullptr);

			// check) 찾지 못했으면 끝낸다.
			CONTINUE_IF(pentity_manager->_is_exist(pobject_find) == false);
						
			// 3) 찾은 Event를 suspend한다.
			pobject_find->pause();
		}
	}

	// 3) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_event_list::OnBnClickedButtonResumeEvent()
{
	// declare) 
	object_ptr<time_event::entity_manager> pentity_manager;
	
	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		pentity_manager = this->m_pentity_manager;
	}

	// check) pentity_manager가 nullptr이면 그냥 끝낸다.
	RETURN_IF(pentity_manager.empty())

	SCOPED_LOCK(pentity_manager->get_lockable())
	{
		POSITION pos = this->m_listctrlEvent.GetFirstSelectedItemPosition();

		while(pos)
		{
			int	nItem = this->m_listctrlEvent.GetNextSelectedItem(pos);

			// 1) Connectable을 얻는다.
			auto* pobject_find = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(nItem);

			// check) pobject_find이 nullptr일 수는 없다.
			CGASSERT_FATAL(pobject_find != nullptr);

			// check) 찾지 못했으면 끝낸다.
			CONTINUE_IF(pentity_manager->_is_exist(pobject_find) == false);
						
			// 3) 찾은 Event를 resume한다.
			pobject_find->resume();
		}
	}

	// 3) Update한다.
	this->PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_event_list::OnNMDblclkListEvent(NMHDR* pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// declare) 
	object_ptr<time_event::Ientity>	pevent;

	SCOPED_LOCK(this->m_lockable_event_manager)
	{
		// check) EventManager가 nullptr이면 끝낸다.
		RETURN_IF(this->m_pentity_manager.empty());

		// - Event object를 얻는다.
		pevent = (time_event::Ientity*)this->m_listctrlEvent.GetItemData(pNMItemActivate->iItem);
	}

	// check) pevent가 nullptr이면 끝낸다.
	RETURN_IF(pevent.empty());

	// 2) 
	CTimeEventInfoDlg dlg(pevent);

	dlg.DoModal();


	// - ...
	*pResult = 0;
}


}