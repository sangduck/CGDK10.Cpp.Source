// CGChildReferenceList.cpp : implementation file
//

#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{
// CChild_reference_list dialog

IMPLEMENT_DYNAMIC(CChild_reference_list, CChildDialog)

CChild_reference_list::CChild_reference_list(Ireferenceable* _object /*=nullptr*/, CWnd* _pparent /*=nullptr*/)
	: CChildDialog(IDD_DIALOG_REFERENCE_LIST, _pparent),
	m_pobject(_object),
	m_is_enable_control(TRUE)
{

}

CChild_reference_list::~CChild_reference_list()
{
}

void CChild_reference_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REFERENCE, m_listctrl_reference);
}


BEGIN_MESSAGE_MAP(CChild_reference_list, CChildDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_REFERENCE, OnLvnItemchangedListReference)
	
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_reference_list::OnCGMessageNotify)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()


// CChild_reference_list message handlers
BOOL CChild_reference_list::CreateChild(Ireferenceable* _object, CWnd* _pwnd)
{
	// 1) Reference Manager를 설정한다.
	set_object(_object);

	// 2) 윈도우를 생성한다.
	auto bResult = Create(IDD_DIALOG_REFERENCE_LIST, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(bResult==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return TRUE;
}

void CChild_reference_list::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd() == nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable & _is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable & _is_enable);
}

BOOL CChild_reference_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Acceptor Info
	m_listctrl_reference.InsertColumn(0, TEXT("name"),		LVCFMT_LEFT,	150, 0);
	m_listctrl_reference.InsertColumn(1, TEXT("Ptr"),		LVCFMT_LEFT,	 80, 1);
	m_listctrl_reference.InsertColumn(2, TEXT("Ref"),		LVCFMT_RIGHT,	 80, 2);
	m_listctrl_reference.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_reference_list::OnLvnItemchangedListReference(NMHDR *pNMHDR, LRESULT *presult)
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
		//Ireferenceable*	pthread	 = (Ireferenceable*)m_listctrl_reference.GetItemData(pNMLV->iItem);
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)pthread);
	}
	// 4) Item을 해제한 것이다.
	else if(pNMLV->uNewState!=pNMLV->uOldState)
	{
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)NULL);
	}

	*presult = 0;
}

void CChild_reference_list::OnEnable(BOOL is_enable)
{
	// 1) Default
	CChildDialog::OnEnable(is_enable);

	// 2) Thread 개수
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_REFERENCE_COUNT)->EnableWindow(is_enable);

	// 3) Thread List
	m_listctrl_reference.EnableWindow(is_enable);
}

void CChild_reference_list::OnDestroy()
{
	// 1) Thread List를 reset한다.
	ClearReferenceList();

	// 2) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_reference_list::set_object(Ireferenceable* _object)
{
	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_object);

		m_pobject = _object;
	}

	// 2) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_reference_list::ClearReferenceList()
{
	// lock)
	std::lock_guard lock(this->m_lockable_object);

	// 1) reset한다.
	m_pobject.reset();
}

void CChild_reference_list::UpdateReferenceList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Thread Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<Ireferenceable> pobject;

	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_object);

		pobject = m_pobject;
	}


	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) Thread Manager가 nullptr이면 끝낸다.
	if(pobject.empty())
	{
		// 1) 모조리 지운다.
		m_listctrl_reference.DeleteAllItems();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_listctrl_reference.SetRedraw(FALSE);

	// 2) Count를 구해놓는다.
	int countReferenceCount = pobject->reference_count();
	int countReferenceObject = 0;

	// 9) Thread의 개수를 표시한다.
	SetDlgItemInt(IDC_STATIC_REFERENCE_COUNT, countReferenceCount);
	SetDlgItemInt(IDC_STATIC_REFERENCE_LIST_COUNT, countReferenceObject);

	// 10) Redraw On
	m_listctrl_reference.SetRedraw(TRUE);
}

void CChild_reference_list::UpdateReference(int _index)
{
	// check)
	RETURN_IF(_index<0 || _index >= m_listctrl_reference.GetItemCount());


}

LRESULT CChild_reference_list::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateReferenceList();
			}
			break;
	};

	return	0;
}

void CChild_reference_list::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CChildDialog::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_reference_list::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		UpdateReferenceList();
	}

	//-----------------------------------------------------------------
	// 2. MFC Default
	//-----------------------------------------------------------------
	CChildDialog::OnTimer(nIDEvent);
}


}