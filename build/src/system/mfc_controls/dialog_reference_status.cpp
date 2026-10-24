// DialogReferenceStatus.cpp : implementation file
//

#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

// CDialog_reference_status dialog
IMPLEMENT_DYNAMIC(CDialog_reference_status, CDialogEx)

CDialog_reference_status::CDialog_reference_status(Ireferenceable* _preference /*=nullptr*/, CWnd* pnode_parent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_REFERENCE_STATUS, pnode_parent),
	m_pobject(_preference)
{

}

CDialog_reference_status::~CDialog_reference_status()
{
	try
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pobject);

		CGASSERT_ERROR(m_pobject.empty());
	}
	catch(...)
	{
	}
}

void CDialog_reference_status::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL, m_combo_update_interval);
}

BEGIN_MESSAGE_MAP(CDialog_reference_status, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDialog_reference_status::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialog_reference_status::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL, &CDialog_reference_status::OnCbnSelchangeComboUpdateInterval)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CDialog_reference_status::OnBnClickedButtonUpdate)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CDialog_reference_status::OnCGMessageNotify)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_reference_status message handlers
int	CDialog_reference_status::m_update_interval	 = 2;

BOOL CDialog_reference_status::CreateModaless(Ireferenceable* _preference, CWnd* _pparent)
{
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pobject);

		m_pobject = _preference;
	}

	return Create(IDD_DIALOG_REFERENCE_STATUS, _pparent);
}

void CDialog_reference_status::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_child_reference_list.EnableControl(_is_enable);
}


BOOL CDialog_reference_status::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Connectable Manager를 생성한다.
	m_child_reference_list.CreateChild(m_pobject, this);
	m_child_reference_list.Move(30, 70);

	// 2) Window를 Show한다.
	m_child_reference_list.ShowWindow(SW_SHOW);


	//-----------------------------------------------------------------
	// 2. setting하기
	//-----------------------------------------------------------------
	// 1) Reference Manager를 설정한다.
	set_object(m_pobject);


	//-----------------------------------------------------------------
	// 3. start
	//-----------------------------------------------------------------
	// 1) Update Combo의 값을 설정하고 타이머를 설정한다.
	m_combo_update_interval.SetCurSel(m_update_interval);

	// 2) Timer를 설정한다.
	SetUpdateInterval(m_update_interval);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDialog_reference_status::OnBnClickedOk()
{
	// 1) Connective List를 Release 한다.
	m_child_reference_list.ClearReferenceList();

	// 2) 타이머를 끈다.
	KillTimer(0);

	// 3) OnOk
	OnOK();
}

void CDialog_reference_status::OnBnClickedCancel()
{
	// 1) Connective List를 Release 한다.
	m_child_reference_list.ClearReferenceList();

	// 2) 타이머를 끈다.
	KillTimer(0);

	// 3) OnCancel
	OnCancel();
}

void CDialog_reference_status::set_object(Ireferenceable* _preference)
{
	// 1) 설정한다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pobject);

		m_pobject = _preference;
	}

	// 2) Connectable Manager를 설정한다.
	m_child_reference_list.set_object(_preference);

	// 3) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CDialog_reference_status::UpdateThreadStatus()
{
	// 1) Reference List를 갱신한다.
	m_child_reference_list.UpdateReferenceList();
}

void CDialog_reference_status::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 선택된 Interval을 얻는다.
	int	selected	 = m_combo_update_interval.GetCurSel();

	// 2) 
	SetUpdateInterval(selected);
}

void CDialog_reference_status::OnBnClickedButtonUpdate()
{
	UpdateThreadStatus();
}

void CDialog_reference_status::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// - 상태를 Update한다.
		UpdateThreadStatus();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CDialog_reference_status::SetUpdateInterval(int _interval)
{
	// 1) Interval 값을 저장한다.
	m_update_interval	 = _interval;

	// 2) Interval에 따라 Timer를 설정한다.
	switch(m_update_interval)
	{
	// No Update
	case	0:
			KillTimer(0);
			break;

	// 1sec
	case	1:
			SetTimer(0, 1000, nullptr);
			break;

	// 2sec
	case	2:
			SetTimer(0, 2000, nullptr);
			break;

	// 5sec
	case	3:
			SetTimer(0, 5000, nullptr);
			break;

	// 10sec
	case	4:
			SetTimer(0, 10000, nullptr);
			break;

	// 30sec
	case	5:
			SetTimer(0, 30000, nullptr);
			break;

	// 1min
	case	6:
			SetTimer(0, 60000, nullptr);
			break;

	};
}


void CDialog_reference_status::SetViewStatus(bool _enable_sustain)
{
	//GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_ADDRESS)->EnableWindow(_enable_sustain);
	//GetDlgItem(IDC_STATIC_ACCEPTOR_BIND_PORT)->EnableWindow(_enable_sustain);
	//GetDlgItem(IDC_STATIC_PREPARED)->EnableWindow(_enable_sustain);

	//GetDlgItem(IDC_STATIC_TRY)->EnableWindow(_enable_sustain);
	//GetDlgItem(IDC_STATIC_CONNECTED)->EnableWindow(_enable_sustain);
	//GetDlgItem(IDC_STATIC_FAIL_CONNECTED)->EnableWindow(_enable_sustain);
	//GetDlgItem(IDC_STATIC_DISCONNECTED)->EnableWindow(_enable_sustain);

	m_combo_update_interval.EnableWindow(_enable_sustain);
	GetDlgItem(IDC_BUTTON_UPDATE)->EnableWindow(_enable_sustain);
}


LRESULT CDialog_reference_status::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateThreadStatus();
			}
			break;
	};

	return	0;
}

void CDialog_reference_status::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pwnd_other, bMinimized);

	//-----------------------------------------------------------------
	// 1. Connective를 새로 설정한다.
	//-----------------------------------------------------------------
	// 1) Connective를 List에 다시 추가한다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pobject);

		set_object(m_pobject);
	}

	// 2) Timer를 설정한다.
	SetUpdateInterval(m_update_interval);


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CDialog_reference_status::OnDestroy()
{
	// 1) 제거한다.
	m_child_reference_list.ClearReferenceList();

	// 2) Reference Manager를 reset한다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pobject);

		m_pobject.reset();
	}

	// 3) 부모 Class의 destroy
	__super::OnDestroy();
}


}
