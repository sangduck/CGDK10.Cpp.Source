#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_thread_status, CDialogEx)

CDialog_thread_status::CDialog_thread_status(api::manager_thread* _pthread_manager, executor::manager* _pexecutor_manager, system_executor* _pDefaultExecutor, CWnd* pnode_parent)
	: CDialogEx(IDD_DIALOG_THREAD_STATUS, pnode_parent),
	m_pmanager_thead(_pthread_manager),
	m_pmanager_executor(_pexecutor_manager),
	m_pdefault_executor(_pDefaultExecutor)
{
}

CDialog_thread_status::~CDialog_thread_status()
{
	try
	{
		scoped_lock(m_pmanager_thead)
		{
			CGASSERT_ERROR(m_pmanager_thead.empty());
		}
	}
	catch(...)
	{
	}

}

void CDialog_thread_status::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL, m_combo_update_interval);
	DDX_Control(pDX, IDC_TAB_MENU, m_tabMenu);
}


BEGIN_MESSAGE_MAP(CDialog_thread_status, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDialog_thread_status::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialog_thread_status::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL, &CDialog_thread_status::OnCbnSelchangeComboUpdateInterval)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CDialog_thread_status::OnBnClickedButtonUpdate)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CDialog_thread_status::OnCGMessageNotify)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MENU, &CDialog_thread_status::OnTcnSelchangeTabMenu)
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB_MENU, &CDialog_thread_status::OnTcnSelchangingTabMenu)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_thread_status message handlers
int	CDialog_thread_status::m_update_inverval = 2;

BOOL CDialog_thread_status::CreateModaless(api::manager_thread* _pthread_manager, executor::manager* _pexecutor_manager, system_executor* _pDefaultExecutor, CWnd* _pparent)
{
	scoped_lock(m_pmanager_thead)
	{
		m_pmanager_thead = _pthread_manager;
		m_pmanager_executor = _pexecutor_manager;
		m_pdefault_executor = _pDefaultExecutor;
	}

	return Create(IDD_DIALOG_THREAD_STATUS, _pparent);
}

void CDialog_thread_status::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_child_thread_list.EnableControl(_is_enable);
	m_child_executor_list.EnableControl(_is_enable);
	m_child_schedulable_list.EnableControl(_is_enable);
}


BOOL CDialog_thread_status::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Thread List를 생성한다.
	m_child_thread_list.CreateChild(m_pmanager_thead, this);
	m_child_thread_list.Resize(0,0);
	m_child_thread_list.Move(20, 90);

	// 2) executor List
	m_child_executor_list.CreateChild(m_pmanager_executor, this);;
	m_child_executor_list.Resize(0, 0);
	m_child_executor_list.Move(20, 90);

	// 3) Schedulable List
	m_child_schedulable_list.CreateChild((m_pdefault_executor!=nullptr) ? m_pdefault_executor->schedulable_manager().get() : nullptr, this);
	m_child_schedulable_list.Resize(0, 0);
	m_child_schedulable_list.Move(20, 90);

	// 4) Window를 Show한다.
	m_child_thread_list.ShowWindow(SW_SHOW);
	m_child_executor_list.ShowWindow(SW_HIDE);
	m_child_schedulable_list.ShowWindow(SW_HIDE);

	// 5) Tab control
	m_tabMenu.InsertItem(0, TEXT("Thread"));
	m_tabMenu.InsertItem(1, TEXT("Executor"));
	m_tabMenu.InsertItem(2, TEXT("Schedulable"));
	m_tabMenu.SetCurSel(0);


	//-----------------------------------------------------------------
	// 2. start
	//-----------------------------------------------------------------
	// 1) Update Combo의 값을 설정하고 타이머를 설정한다.
	m_combo_update_interval.SetCurSel(m_update_inverval);

	// 2) Timer를 설정한다.
	SetUpdateInterval(m_update_inverval);

	// 3) Thread Manager를 설정한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDialog_thread_status::OnBnClickedOk()
{
	// 1) Connective List를 Release 한다.
	m_child_thread_list.ClearThreadList();
	m_child_executor_list.ClearExecutorList();
	m_child_schedulable_list.ClearSchedulableList();

	// 2) 타이머를 끈다.
	KillTimer(0);

	// 3) OnOk
	OnOK();
}

void CDialog_thread_status::OnBnClickedCancel()
{
	// 1) Connective List를 Release 한다.
	m_child_thread_list.ClearThreadList();
	m_child_executor_list.ClearExecutorList();
	m_child_schedulable_list.ClearSchedulableList();

	// 2) 타이머를 끈다.
	KillTimer(0);

	// 3) OnCancel
	OnCancel();
}

void CDialog_thread_status::SetManager(api::manager_thread* _pthread_manager, executor::manager* _pexecutor_manager, system_executor* _pDefaultExecutor)
{
	// 1) 설정한다.
	scoped_lock(m_pmanager_thead)
	{
		m_pmanager_thead = _pthread_manager;
		m_pmanager_executor = _pexecutor_manager;
		m_pdefault_executor = _pDefaultExecutor;
	}

	// 2) 
	auto* pSchedulable = (_pDefaultExecutor!=nullptr) ? _pDefaultExecutor->schedulable_manager().get() : nullptr;

	// 3) Connectable Manager를 설정한다.
	m_child_thread_list.SetThreadManager(_pthread_manager);
	m_child_executor_list.SetExecutorManager(_pexecutor_manager);
	m_child_schedulable_list.SetSchedulableList(pSchedulable);

	// 4) Update한다.
	if (m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CDialog_thread_status::UpdateThreadStatus()
{
	// declare)
	size_t countThread = 0;
	size_t countExecutor = 0;
	size_t count_schedulable = 0;
	size_t countSingleExecution = 0;

	// 1) 값 얻기
	scoped_lock(m_pmanager_thead)
	{
		if(m_pmanager_thead.exist())
		{
			countThread = m_pmanager_thead->size();
		}	

		if(m_pmanager_executor.exist())
		{
			countExecutor = m_pmanager_executor->size();
		}

		if(m_pdefault_executor!=nullptr)
		{
			count_schedulable = (m_pdefault_executor->schedulable_manager().exist()) ? m_pdefault_executor->schedulable_manager()->size() : 0;
			countSingleExecution = (m_pdefault_executor->scheduler().exist()) ? m_pdefault_executor->scheduler()->pended_count() : 0;
		}
	}

	// 2) 
	SetDlgItemInt(IDC_STATIC_THREADS_COUNT, static_cast<int>(countThread));
	SetDlgItemInt(IDC_STATIC_EXECUTOR_COUNT, static_cast<int>(countExecutor));
	SetDlgItemInt(IDC_STATIC_SCHEDULABLE_COUNT, static_cast<int>(count_schedulable));
	SetDlgItemInt(IDC_STATIC_SINGLEEXECUTION_COUNT, static_cast<int>(countSingleExecution));

	// 3) ...
	switch (m_tabMenu.GetCurSel())
	{
	case	0:
			m_child_thread_list.UpdateThreadList();
			break;

	case	1:
			m_child_executor_list.UpdateExecutorList();
			break;

	case	2:
			m_child_schedulable_list.UpdateSchedulableList();
			break;
	}
}

void CDialog_thread_status::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 선택된 Interval을 얻는다.
	int	selected	 = m_combo_update_interval.GetCurSel();

	// 2) 
	SetUpdateInterval(selected);
}

void CDialog_thread_status::OnBnClickedButtonUpdate()
{
	UpdateThreadStatus();
}

void CDialog_thread_status::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// - 상태를 Update한다.
		UpdateThreadStatus();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CDialog_thread_status::SetUpdateInterval(int _interval)
{
	// 1) Interval 값을 저장한다.
	m_update_inverval	 = _interval;

	// 2) Interval에 따라 Timer를 설정한다.
	switch(m_update_inverval)
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


void CDialog_thread_status::SetViewStatus(bool _enable_sustain)
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


LRESULT CDialog_thread_status::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
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

void CDialog_thread_status::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Connective를 List에 다시 추가한다.
	scoped_lock(m_pmanager_thead)
	{
		SetManager(m_pmanager_thead, m_pmanager_executor, m_pdefault_executor);
	}

	// 2) Timer를 설정한다.
	SetUpdateInterval(m_update_inverval);

	// 3) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CDialog_thread_status::OnDestroy()
{
	// 1) 제거한다.
	m_child_thread_list.ClearThreadList();
	m_child_thread_list.DestroyWindow();
	m_child_executor_list.ClearExecutorList();
	m_child_executor_list.DestroyWindow();
	m_child_schedulable_list.ClearSchedulableList();
	m_child_schedulable_list.DestroyWindow();

	// 2) 타이머를 끈다.
	KillTimer(0);

	// 3) Thread Manager를 reset한다.
	scoped_lock(m_pmanager_thead)
	{
		m_pmanager_thead.reset();
		m_pmanager_executor.reset();
		m_pdefault_executor	 = nullptr;
	}

	// 3) 부모 Class의 destroy
	__super::OnDestroy();
}

void CDialog_thread_status::OnTcnSelchangeTabMenu(NMHDR* /*pNMHDR*/, LRESULT *presult)
{
	// 1) 
	auto	iSelect	 = m_tabMenu.GetCurSel();

	switch (iSelect)
	{
	case	0:
		m_child_thread_list.UpdateThreadList();
		m_child_thread_list.ShowWindow(SW_SHOW);
		break;

	case	1:
		m_child_executor_list.UpdateExecutorList();
		m_child_executor_list.ShowWindow(SW_SHOW);
		break;

	case	2:
		m_child_schedulable_list.UpdateSchedulableList();
		m_child_schedulable_list.ShowWindow(SW_SHOW);
		break;
	}

	*presult = 0;
}


void CDialog_thread_status::OnTcnSelchangingTabMenu(NMHDR* /*pNMHDR*/, LRESULT *presult)
{
	// 1) 
	auto	iSelect	 = m_tabMenu.GetCurSel();

	switch(iSelect)
	{
	case	0:
			m_child_thread_list.ShowWindow(SW_HIDE);
			break;

	case	1:
			m_child_executor_list.ShowWindow(SW_HIDE);
			break;

	case	2:
			m_child_schedulable_list.ShowWindow(SW_HIDE);
			break;
	}

	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*presult = 0;
}


}