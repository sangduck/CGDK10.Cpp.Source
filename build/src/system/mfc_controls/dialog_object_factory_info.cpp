#include "pch.h"


#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_object_factory_info, CDialogEx)

CDialog_object_factory_info::CDialog_object_factory_info(Ifactory* _pFactory, CWnd* pnode_parent /*=NULL*/)
	: CDialogEx(IDD_DIALOG_OBJECTFACTORY_INFO, pnode_parent),
	m_pobject_factory(_pFactory),
	m_is_enable_control(TRUE)
{

}

CDialog_object_factory_info::~CDialog_object_factory_info()
{
}

void CDialog_object_factory_info::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_OBJECT, m_listctrl_object);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL, m_combo_update_interval);
}


BEGIN_MESSAGE_MAP(CDialog_object_factory_info, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDialog_object_factory_info::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialog_object_factory_info::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL, &CDialog_object_factory_info::OnCbnSelchangeComboUpdateInterval)
	ON_WM_TIMER()
	ON_WM_ENABLE()

	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CDialog_object_factory_info::OnBnClickedButtonUpdate)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CDialog_object_factory_info::OnCGMessageNotify)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_object_factory_info message handlers
int	CDialog_object_factory_info::m_update_inverval = 2;

BOOL CDialog_object_factory_info::CreateModaless(Ifactory* _pObjectFactory, CWnd* _pparent)
{
	// 1) Manager를 설정한다.
	scoped_lock(m_pobject_factory)
	{
		m_pobject_factory = _pObjectFactory;
	}

	// 2) 생성한다.
	return Create(IDD_DIALOG_OBJECTFACTORY_INFO, _pparent);
}

void CDialog_object_factory_info::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd() == nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_POOL_PREPARE)->EnableWindow(is_enable & _is_enable);
	GetDlgItem(IDC_BUTTON_POOL_SHRINK)->EnableWindow(is_enable & _is_enable);
	GetDlgItem(IDC_BUTTON_POOL_ENABLE_SUSTAIN)->EnableWindow(is_enable & _is_enable);
	GetDlgItem(IDC_BUTTON_POOL_DISABLE_SUSTAIN)->EnableWindow(is_enable & _is_enable);
	GetDlgItem(IDC_CHECK_ENABLE_AUTO_SUSTAIN)->EnableWindow(is_enable & _is_enable);
}


BOOL CDialog_object_factory_info::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Object List
	m_listctrl_object.InsertColumn(0, TEXT("Ref."),			LVCFMT_LEFT,	 60, 0);
	m_listctrl_object.InsertColumn(1, TEXT("alloc. Location"),	LVCFMT_LEFT,	300, 1);
	m_listctrl_object.InsertColumn(2, TEXT("alloc. Line"),		LVCFMT_LEFT,	 80, 2);
	m_listctrl_object.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) 
	m_combo_update_interval.SetCurSel(m_update_inverval);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return	TRUE;  // return TRUE unless you set the focus to a control
}

void CDialog_object_factory_info::OnEnable(BOOL is_enable)
{
	// 1) 부모의 OnEnable()함수를 호출한다.
	__super::OnEnable(is_enable);

	// 2) control 버튼들
	//GetDlgItem(IDC_BUTTON_POOL_PREPARE)->EnableWindow(is_enable & m_is_enable_control);
	//GetDlgItem(IDC_BUTTON_POOL_SHRINK)->EnableWindow(is_enable & m_is_enable_control);
	//GetDlgItem(IDC_BUTTON_POOL_ENABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	//GetDlgItem(IDC_BUTTON_POOL_DISABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	//GetDlgItem(IDC_CHECK_ENABLE_AUTO_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);

	//// 3) Factory Count
	//GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	//GetDlgItem(IDC_STATIC_POOL_COUNT)->EnableWindow(is_enable);

	//// 4) Update control들
	//GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	//GetDlgItem(IDC_BUTTON_UPDATE)->EnableWindow(is_enable);
	//m_combo_update_interval.EnableWindow(is_enable);

	//// 5) List
	//m_listctrl_object_factory.EnableWindow(is_enable);

	//// 6) CheckButton 설정
	//GetDlgItem(IDC_CHECK_VISIBLE_MEMORYBLOCK)->EnableWindow(is_enable);
	//GetDlgItem(IDC_CHECK_VISIBLE_SYSTEM)->EnableWindow(is_enable);
	//GetDlgItem(IDC_CHECK_VISIBLE_USER)->EnableWindow(is_enable);
}

void CDialog_object_factory_info::OnDestroy()
{
	// 1) reset한다.
	ClearObjectFactoryInfo();

	// 2) 부모 Class의 destroy
	__super::OnDestroy();
}

void CDialog_object_factory_info::OnBnClickedOk()
{
	KillTimer(0);

	OnOK();
}

void CDialog_object_factory_info::OnBnClickedCancel()
{
	KillTimer(0);

	OnCancel();
}

void CDialog_object_factory_info::ClearObjectFactoryInfo() noexcept
{
	scoped_lock(m_pobject_factory)
	{
		m_pobject_factory.reset();
	}
}

void CDialog_object_factory_info::UpdateObjectFactoryInfo()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Thread Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<Ifactory> pobject_factory;

	// 1) Thread Manager를 설정한다.
	scoped_lock(m_pobject_factory)
	{
		pobject_factory = m_pobject_factory;
	}


	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) Thread Manager가 nullptr이면 끝낸다.
	if(m_pobject_factory.empty())
	{
		// 1) 모조리 지운다.
		m_listctrl_object.DeleteAllItems();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_listctrl_object.SetRedraw(FALSE);

	//// declare) Index를 0으로...
	//int		index	 = 0;

	//chrono::tick::time_point	tick_now	 = chrono::tick::now();

	//// 2) Thread list Iterator의 begin과 end를 얻는다.
	//for(auto iter=pexecutor_schedulable->get_iterator(); iter.is_ended()==false; ++iter, ++index)
	//{
	//	// 3) pSchedulable를 얻는다.
	//	Ischedulable* pSchedulable	 = *iter;

	//	// check) pSchedulable가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
	//	CONTINUE_IF(pSchedulable==nullptr);

	//	// 4) 접속종료된 Connectable을 제거한다. 
	//	while(index <m_listctrl_object.GetItemCount())
	//	{
	//		// - Connectable을 얻는다.
	//		auto* pSchedulableCompare	 = reinterpret_cast<Ischedulable*>(m_listctrl_object.GetItemData(index));

	//		// check) 같으면 끝낸다~
	//		BREAK_IF(pSchedulableCompare == pSchedulable);

	//		// - 지운다.
	//		m_listctrl_object.DeleteItem(index);
	//	}

	//	// 5) 새로 추가된 것이면 추가한다.
	//	if(index>=m_listctrl_object.GetItemCount())
	//	{
	//		// - Nameable을 얻는다.
	//		auto*	pnameable	 = dynamic_cast<object::Inameable*>(pSchedulable);

	//		// - 값을 추가하고 데이타를 설정한다.
	//		if(pnameable!=nullptr)
	//		{
	//			#ifdef _UNICODE
	//				#ifdef _USE_CHAR_STRING
	//					wchar_t	str_name[MAX_STRING_LENGTH];
	//					MBS_TO_WCS(str_name, MAX_STRING_LENGTH, pnameable->name(), MAX_STRING_LENGTH);
	//					m_listctrl_object.InsertItem(index,  str_name, -1);
	//				#else
	//					m_listctrl_object.InsertItem(index, pnameable->name(), -1);
	//				#endif
	//			#else
	//				#ifdef _USE_CHAR_STRING
	//					m_listctrl_object.InsertItem(index, pnameable->name(), -1);
	//				#else
	//					char	str_name[MAX_STRING_LENGTH];
	//					WCS_TO_MBS(str_name, MAX_STRING_LENGTH, pnameable->name(), MAX_STRING_LENGTH);
	//					m_listctrl_object.InsertItem(index,  str_name, -1);
	//				#endif
	//			#endif
	//		}
	//		else
	//		{
	//			m_listctrl_object.InsertItem(index, TEXT("No name"), -1);
	//		}

	//		m_listctrl_object.SetItemData(index, (DWORD_PTR)pSchedulable);
	//	}

	//	// 6) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
	//	UpdateSchedulableList(index, tick_now);
	//}

	//// 7) 아직도 남은 것들은 모조리 지운다.
	//for(size_t	Count=m_listctrl_object.GetItemCount()-index; Count>0; Count--)
	//{
	//	// -Item을 지운다.
	//	m_listctrl_object.DeleteItem(index);
	//}

	//// 8) Thread의 갯수를 표시한다.
	//SetDlgItemInt(IDC_STATIC_SCHEDULABLE_COUNT, static_cast<int>(pexecutor_schedulable->schedulable_count()));

	// 9) Redraw On
	m_listctrl_object.SetRedraw(TRUE);
}

void CDialog_object_factory_info::UpdateObjectFactoryInfo(int _index, chrono::tick::time_point /*_tick_now*/)
{
	// check)
	RETURN_IF(_index<0 || _index>=m_listctrl_object.GetItemCount());

	// 1) Object를 얻는다.
	auto* preferenceable = reinterpret_cast<Ireferenceable*>(m_listctrl_object.GetItemData(_index));

	// check) preferenceable가 nullptr이면 그냥 되돌린다. 
	RETURN_IF(preferenceable == nullptr);

	//// 2) List를 설정한다.
	//TCHAR		text[MAX_STRING_LENGTH]={};
	//LV_ITEM		list_item;

	//list_item.mask			 = LVIF_TEXT;
	//list_item.state			 = 0;
	//list_item.cchTextMax	 = MAX_STRING_LENGTH;
	//list_item.iItem			 = _index;

	//// 0	"name"
	//list_item.iSubItem		 = 0;
	//object::Inameable*	pnameable	 = dynamic_cast<object::Inameable*>(preferenceable);
	//if(pnameable!=nullptr)
	//{
	//	list_item.pszText		 = const_cast<TCHAR*>(pnameable->name());
	//}
	//else
	//{
	//	list_item.pszText		 = TEXT(" - ");
	//}
	//m_listctrl_object.SetItem(&list_item);

	//// 1. "Type"
	//list_item.iSubItem		 = 1;
	//__sprintf(text, TEXT("%s"), typeid(*preferenceable).name());
	//for(int i=0;text[i]!=NULL;++i)	if(text[i]=='*') {text[i]=NULL;break;}
	//list_item.pszText		 = &text[13];
	//m_listctrl_object.SetItem(&list_item);

	//auto*	pReferenceableExecutable	 = dynamic_cast<Nschedulable*>(preferenceable);

	//if (pReferenceableExecutable!=nullptr)
	//{
	//	// - Interval을 구한다.
	//	auto	millisecInterval	 = std::chrono::duration_cast<chrono::milliseconds>(pReferenceableExecutable->execute_interval()).count();

	//	if(millisecInterval<1000)
	//	{
	//		__sprintf(text, TEXT("%I64ums"), millisecInterval);
	//	}
	//	else
	//	{
	//		auto	secInterval	 = millisecInterval/1000;
	//		millisecInterval	%= 1000;

	//		if(millisecInterval != 0)
	//		{
	//			__sprintf(text, TEXT("%I64usec %I64ums"), secInterval, millisecInterval);
	//		}
	//		else
	//		{
	//			__sprintf(text, TEXT("%I64usec"), secInterval);
	//		}
	//	}

	//	// 2. "Interval"
	//	list_item.iSubItem		 = 2;
	//	list_item.pszText		 = text;
	//	m_listctrl_object.SetItem(&list_item);

	//	//const auto&	timeNext	 = preferenceable->next_tick();
	//	//__sprintf(text, TEXT("%s"), timeNext.);

	//	// 3. "Next Tick"
	//	list_item.iSubItem		 = 3;
	//	list_item.pszText = TEXT("");
	//	m_listctrl_object.SetItem(&list_item);
	//}
	//else
	//{
	//	// 2. "Interval"
	//	list_item.iSubItem		 = 2;
	//	list_item.pszText		 = TEXT("");
	//	m_listctrl_object.SetItem(&list_item);

	//	// 3. "Next Tick"
	//	list_item.iSubItem		 = 3;
	//	list_item.pszText		 = TEXT("");
	//	m_listctrl_object.SetItem(&list_item);
	//}

	//// 2. 
	//Iexecutable*	pexecutable	 = dynamic_cast<Iexecutable*>(pSchedulable);

	//// 4 "Concurrent Execution"
	//list_item.iSubItem		 = 4;
	//if (pexecutable!=nullptr)
	//{
	//	__sprintf(text, TEXT("%d"), 0);
	//	list_item.pszText	 = text;
	//}
	//else
	//{
	//	list_item.pszText	 = TEXT("");
	//}
	//m_listctrl_object.SetItem(&list_item);
}

void CDialog_object_factory_info::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 선택된 Inteval을 얻어낸다.
	int	selected = m_combo_update_interval.GetCurSel();

	// 2) 해당하는 Update Interval로 설정한다.
	SetUpdateInterval(selected);
}

void CDialog_object_factory_info::OnBnClickedButtonUpdate()
{
	UpdateObjectFactoryInfo();
}

void CDialog_object_factory_info::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) 상태를 Update한다.
		UpdateObjectFactoryInfo();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CDialog_object_factory_info::SetUpdateInterval(int _interval)
{
	// 1) 
	m_update_inverval = _interval;

	// 2) 
	switch (m_update_inverval)
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

LRESULT CDialog_object_factory_info::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch (_wParam)
	{
	case	CGUPDATE:
	{
		UpdateObjectFactoryInfo();
	}
	break;
	};

	return	0;
}

void CDialog_object_factory_info::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}


}