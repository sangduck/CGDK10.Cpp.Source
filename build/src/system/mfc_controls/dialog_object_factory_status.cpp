#include "pch.h"
#include <charconv>
#include "Dialog_amount.h"
#include "Dialog_amount_shrink.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_object_factory_status, CDialogEx)

CDialog_object_factory_status::CDialog_object_factory_status(factory::manager* _pmanager, CWnd* pnode_parent) noexcept
	: CDialogEx(IDD_DIALOG_OBJECTFACTORY_STATUS, pnode_parent),
	m_pmanager(_pmanager), 
	m_is_enable_control(TRUE)
{
	for(int i=0; i<16; ++i)
	{
		m_bVisiableTypeStatus.at(i) = true;
	}
}

CDialog_object_factory_status::~CDialog_object_factory_status() noexcept
{
}

void CDialog_object_factory_status::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MEMORYBLOCKPOOL, m_listctrl_object_factory);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL, m_combo_update_interval);
	DDX_Control(pDX, IDC_PROGRESS_MEMORY, m_progressMemoryUsage);
}


BEGIN_MESSAGE_MAP(CDialog_object_factory_status, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDialog_object_factory_status::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialog_object_factory_status::OnBnClickedCancel)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL, &CDialog_object_factory_status::OnCbnSelchangeComboUpdateInterval)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_BN_CLICKED(IDC_BUTTON_UPDATE, &CDialog_object_factory_status::OnBnClickedButtonUpdate)
	ON_BN_CLICKED(IDC_BUTTON_POOL_PREPARE, &CDialog_object_factory_status::OnBnClickedButtonPrepare)
	ON_BN_CLICKED(IDC_BUTTON_POOL_SHRINK, &CDialog_object_factory_status::OnBnClickedButtonShrink)
	ON_BN_CLICKED(IDC_BUTTON_POOL_ENABLE_SUSTAIN, &CDialog_object_factory_status::OnBnClickedButtonEnableSustain)
	ON_BN_CLICKED(IDC_BUTTON_POOL_DISABLE_SUSTAIN, &CDialog_object_factory_status::OnBnClickedButtonDisableSustain)
	ON_BN_CLICKED(IDC_CHECK_ENABLE_AUTO_SUSTAIN, &CDialog_object_factory_status::OnBnClickedCheckEnableSustainManager)
	ON_BN_CLICKED(IDC_CHECK_VISIBLE_MEMORYBLOCK, &CDialog_object_factory_status::OnBnClickedCheckVisibleMemoryBlock)
	ON_BN_CLICKED(IDC_CHECK_VISIBLE_SYSTEM, &CDialog_object_factory_status::OnBnClickedCheckVisibleSystem)
	ON_BN_CLICKED(IDC_CHECK_VISIBLE_USER, &CDialog_object_factory_status::OnBnClickedCheckVisibleUser)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CDialog_object_factory_status::OnCGMessageNotify)
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_object_factory_status message handlers
int	CDialog_object_factory_status::m_update_interval = 2;

BOOL CDialog_object_factory_status::CreateModaless(factory::manager* _pmanager, CWnd* _pparent)
{
	// 1) Manager를 설정한다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		m_pmanager = _pmanager;
	}

	// 2) 생성한다.
	return Create(IDD_DIALOG_OBJECTFACTORY_STATUS, _pparent);
}

void CDialog_object_factory_status::EnableControl(BOOL _is_enable)
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

BOOL CDialog_object_factory_status::OnInitDialog()
{
	CDialogEx::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Object Factory Status
	m_listctrl_object_factory.InsertColumn( 0, TEXT("name"),		LVCFMT_LEFT,	200,  0);
	m_listctrl_object_factory.InsertColumn( 1, TEXT("id"),			LVCFMT_RIGHT,	 32,  1);
	m_listctrl_object_factory.InsertColumn( 2, TEXT("size"),		LVCFMT_RIGHT,	 60,  2);
	m_listctrl_object_factory.InsertColumn( 3, TEXT("GC_type"),		LVCFMT_LEFT,	 80,  3);
	m_listctrl_object_factory.InsertColumn( 4, TEXT("existing"),	LVCFMT_RIGHT,	 60,  4);
	m_listctrl_object_factory.InsertColumn( 5, TEXT("in using"),	LVCFMT_RIGHT,	 60,  5);
	m_listctrl_object_factory.InsertColumn( 6, TEXT("stk.pool"),	LVCFMT_RIGHT,	 60,  6);
	m_listctrl_object_factory.InsertColumn( 7, TEXT("stk.tls"),		LVCFMT_RIGHT,	 50,  7);
	m_listctrl_object_factory.InsertColumn( 8, TEXT("requested"),	LVCFMT_RIGHT,	 70,  8);
	m_listctrl_object_factory.InsertColumn( 9, TEXT("alloc.new"),	LVCFMT_RIGHT,	 70,  9);
	m_listctrl_object_factory.InsertColumn(10, TEXT("alloc.pool"),	LVCFMT_RIGHT,    70, 10);
	m_listctrl_object_factory.InsertColumn(11, TEXT("alloc.tls"),	LVCFMT_RIGHT,	 70, 11);
	m_listctrl_object_factory.InsertColumn(12, TEXT("thr.pool"),	LVCFMT_RIGHT,	 50, 12);
	m_listctrl_object_factory.InsertColumn(13, TEXT("thr.tls"),		LVCFMT_RIGHT,	 50, 13);
	m_listctrl_object_factory.InsertColumn(14, TEXT("free.del"),	LVCFMT_RIGHT,	 70, 14);
	m_listctrl_object_factory.InsertColumn(15, TEXT("free.tls"),	LVCFMT_RIGHT,	 70, 15);
	m_listctrl_object_factory.InsertColumn(16, TEXT("ratio.pool"),	LVCFMT_RIGHT,	 60, 16);
	m_listctrl_object_factory.InsertColumn(17, TEXT("ratio.tls"),	LVCFMT_RIGHT,	 60, 17);
	m_listctrl_object_factory.InsertColumn(18, TEXT("ratio.D/C"),	LVCFMT_RIGHT,	 60, 18);
	m_listctrl_object_factory.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) Icon추가하기
	m_imagelist_object_factory.Create(16, 16, ILC_COLORDDB, static_cast<int>(eFACTORY_TYPE::MAX), static_cast<int>(eFACTORY_TYPE::MAX));

	for(int i=0; i<static_cast<int>(eFACTORY_TYPE::MAX); ++i)
	{
		CBitmap	bitmap;
		bitmap.LoadBitmap(IDB_BITMAP_FACTORYTYPE_DEFAULT+i);
		m_imagelist_object_factory.Add(&bitmap, (COLORREF)NULL);
		bitmap.DeleteObject();
	}

	m_listctrl_object_factory.SetIconSpacing(20, 20);
	m_listctrl_object_factory.SetImageList(&m_imagelist_object_factory, LVSIL_SMALL);

	// 3) 
	m_combo_update_interval.SetCurSel(m_update_interval);

	// 4) CheckButton 설정
	((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_MEMORYBLOCK))->SetCheck(m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_MEMORYBLOCK)));
	((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_SYSTEM))->SetCheck(m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_SYSTEM)));
	((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_USER))->SetCheck(m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_USER)));

	// 5) 
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_POOL_PREPARE)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_SHRINK)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_ENABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_DISABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_CHECK_ENABLE_AUTO_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CDialog_object_factory_status::OnEnable(BOOL is_enable)
{
	// 1) 부모의 OnEnable()함수를 호출한다.
	__super::OnEnable(is_enable);

	// 2) control 버튼들
	GetDlgItem(IDC_BUTTON_POOL_PREPARE)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_SHRINK)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_ENABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_BUTTON_POOL_DISABLE_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);
	GetDlgItem(IDC_CHECK_ENABLE_AUTO_SUSTAIN)->EnableWindow(is_enable & m_is_enable_control);

	// 3) Factory Count
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_POOL_COUNT)->EnableWindow(is_enable);

	// 4) Update control들
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_UPDATE)->EnableWindow(is_enable);	
	m_combo_update_interval.EnableWindow(is_enable);

	// 5) List
	m_listctrl_object_factory.EnableWindow(is_enable);

	// 6) CheckButton 설정
	GetDlgItem(IDC_CHECK_VISIBLE_MEMORYBLOCK)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_VISIBLE_SYSTEM)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_VISIBLE_USER)->EnableWindow(is_enable);
}

void CDialog_object_factory_status::OnDestroy()
{
	// 1) reset한다.
	ClearFactoryList();

	// 2) 부모 Class의 OnDestroy
	__super::OnDestroy();
}

void CDialog_object_factory_status::OnBnClickedOk()
{
	KillTimer(0);

	OnOK();
}

void CDialog_object_factory_status::OnBnClickedCancel()
{
	KillTimer(0);

	OnCancel();
}

void CDialog_object_factory_status::ClearFactoryList()
{
	// lock) 
	std::lock_guard lock(this->m_lockable_pmanager);

	m_pmanager.reset();
}

void CDialog_object_factory_status::UpdateFactoryList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Factory Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Factory Manager를 읽어 저장해 놓는다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) Connective Manager가 nullptr이면 끝낸다.
	if(pmanager.empty())
	{
		// 1) 모조리 지운다.
		m_listctrl_object_factory.DeleteAllItems();

		// return) 
		return;
	}


	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_listctrl_object_factory.SetRedraw(FALSE);

	// declare) Index를 0으로...
	int index = 0;

	SCOPED_UNIQUE_LOCK(pmanager->get_lockable())
	{
		// 2) Factory list Iterator의 begin과 end를 얻는다.
		auto iter = pmanager->begin();
		auto iter_end = pmanager->end();

		for(; iter!=iter_end; ++iter)
		{
			// 3) pfactory를 얻는다.
			Ifactory* pfactory = *iter;

			// check) pfactory가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pfactory == nullptr);

			// 2) get factory statistics()
			const auto& factory_statistics = pfactory->statistics();

			// check) 정의된 FactoryType이 아닐 경우 끝낸다.
			CONTINUE_IF(factory_statistics.type < eFACTORY_TYPE::DEFAULT || factory_statistics.type >= eFACTORY_TYPE::MAX);

			// check) 
			CONTINUE_IF(m_bVisiableTypeStatus.at(static_cast<int>(pfactory->type()))==false);

			// 4) 추가한다.
			if(index >= m_listctrl_object_factory.GetItemCount())
			{
				// - 이름을 얻는다.
				auto str_name = pfactory->name<TCHAR>();

				// - 값을 추가하고 데이터를 설정한다.
				m_listctrl_object_factory.InsertItem(index, str_name.c_str(), static_cast<int>(pfactory->type()));

				TCHAR text[MAX_STRING_LENGTH] = {};
				LV_ITEM list_item;
				list_item.mask = LVIF_TEXT;
				list_item.state = 0;
				list_item.cchTextMax = MAX_STRING_LENGTH;
				list_item.iItem = index;

				// 1	"id"
				list_item.iSubItem = 1;
				wsprintf(text, TEXT("%d"), factory_statistics.id);
				list_item.pszText = text;
				m_listctrl_object_factory.SetItem(&list_item);

				// 2	"size"
				list_item.iSubItem = 2;
				CString str_size(to_string_scaled_byte(static_cast<uint64_t>(pfactory->object_size())).c_str());
				list_item.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(str_size));
				m_listctrl_object_factory.SetItem(&list_item);

				// - set item
				m_listctrl_object_factory.SetItemData(index, (DWORD_PTR)pfactory);
			}
			else if(m_listctrl_object_factory.GetItemData(index)!=(DWORD_PTR)pfactory)
			{
				// - 이름을 얻는다.
				auto str_text = pfactory->name<TCHAR>();

				// - 변경되었으면 name도 바꾸어야 한다.
				LV_ITEM list_item;
				list_item.mask			 = LVIF_TEXT | LVIF_IMAGE;
				list_item.state			 = 0;
				list_item.cchTextMax	 = MAX_STRING_LENGTH;
				list_item.iItem			 = index;
				list_item.iImage		 = static_cast<int>(pfactory->type());
				list_item.iSubItem		 = 0;
				list_item.pszText		 = const_cast<LPTSTR>(str_text.c_str());
				m_listctrl_object_factory.SetItem(&list_item);

				TCHAR text[MAX_STRING_LENGTH] = {};
				list_item.mask			 = LVIF_TEXT;
				list_item.state			 = 0;
				list_item.cchTextMax	 = MAX_STRING_LENGTH;
				list_item.iItem			 = index;

				// 1	"id"
				list_item.iSubItem = 1;
				wsprintf(text, TEXT("%d"), factory_statistics.id);
				list_item.pszText = text;
				m_listctrl_object_factory.SetItem(&list_item);

				// 2	"size"
				list_item.iSubItem = 2;
				CString str_size(to_string_scaled_byte(static_cast<uint64_t>(pfactory->object_size())).c_str());
				list_item.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(str_size));
				m_listctrl_object_factory.SetItem(&list_item);

				// - set item
				m_listctrl_object_factory.SetItemData(index, (DWORD_PTR)pfactory);
			}

			// 5) Update한다.
			update_factory_status(index);
			
			// 6) Index 증가
			++index;
		}

		// declare) 
		PROCESS_MEMORY_COUNTERS_EX	sInfoMemoryCounters;
		sInfoMemoryCounters.cb = sizeof(sInfoMemoryCounters);

		// 1) memory 정보를 얻는다.
		GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS)&sInfoMemoryCounters, sizeof(sInfoMemoryCounters));

		// 2) Progress Bar Range를 설정한다.
		m_progressMemoryUsage.SetRange32(0, static_cast<int>(pmanager->sustain_threshold(2) / (1024.0f*1024.0f)));
		m_progressMemoryUsage.SetPos(static_cast<int>(sInfoMemoryCounters.PrivateUsage / (1024.0f*1024.0f)));
		SetDlgItemText(IDC_STATIC_MEMORY_WORKINGSET,	CString(to_string_scaled_byte(static_cast<uint64_t>(sInfoMemoryCounters.WorkingSetSize)).c_str()));
		SetDlgItemText(IDC_STATIC_MEMORY_PRIVATE,		CString(to_string_scaled_byte(static_cast<uint64_t>(sInfoMemoryCounters.PrivateUsage)).c_str()));
		SetDlgItemText(IDC_STATIC_SUSTAIN_LEVEL1,		CString(to_string_scaled_byte(static_cast<uint64_t>(pmanager->sustain_threshold(0))).c_str()));
		SetDlgItemText(IDC_STATIC_SUSTAIN_LEVEL2,		CString(to_string_scaled_byte(static_cast<uint64_t>(pmanager->sustain_threshold(1))).c_str()));
		SetDlgItemText(IDC_STATIC_SUSTAIN_LEVEL3,		CString(to_string_scaled_byte(static_cast<uint64_t>(pmanager->sustain_threshold(2))).c_str()));
	}

	// 6) 아직도 남은 것들은 모조리 지운다.
	for(int	Count=m_listctrl_object_factory.GetItemCount()-index; Count>0; Count--)
	{
		// -Item을 지운다.
		m_listctrl_object_factory.DeleteItem(index);
	}

	// 7) Factory의 수를 표시한다.
	SetDlgItemInt(IDC_STATIC_POOL_COUNT, static_cast<UINT>(pmanager->size()));

	// 8) Redraw On
	m_listctrl_object_factory.SetRedraw(TRUE);
}

void CDialog_object_factory_status::update_factory_status(int _index)
{
	// check) 
	RETURN_IF(_index < 0 || _index >= m_listctrl_object_factory.GetItemCount());

	bool is_factory_sustain = true;

	// 1) get factory sustain status
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		if (m_pmanager.exist())
		{
			is_factory_sustain = m_pmanager->is_enable_sustain();
		}
	}

	// 2) get factory
	auto pfactory = reinterpret_cast<Ifactory*>(m_listctrl_object_factory.GetItemData(_index));

	// check) 
	RETURN_IF(pfactory == nullptr);

	// 2) get factory statistics()
	const auto& factory_statistics = pfactory->statistics();

	// 2) List를 설정한다.
	TCHAR text[MAX_STRING_LENGTH]={};
	char text_mb[MAX_STRING_LENGTH]={};
	LV_ITEM list_item;
	auto set_number_text = [&](auto _value)
	{
		if constexpr (std::is_integral_v<std::remove_cvref_t<decltype(_value)>> && std::is_unsigned_v<std::remove_cvref_t<decltype(_value)>>)
		*std::to_chars(text_mb, text_mb + (std::size(text_mb)) - 1, static_cast<uint64_t>(_value)).ptr = 0;
	else if constexpr (std::is_integral_v<std::remove_cvref_t<decltype(_value)>>)
		*std::to_chars(text_mb, text_mb + (std::size(text_mb)) - 1, static_cast<int64_t>(_value)).ptr = 0;
	else
		*std::to_chars(text_mb, text_mb + (std::size(text_mb)) - 1, _value).ptr = 0;
		list_item.pszText = const_cast<TCHAR*>(encode_string(text, std::string_view(text_mb), MAX_STRING_LENGTH).data());
	};
	list_item.mask = LVIF_TEXT;
	list_item.state = 0;
	list_item.cchTextMax = MAX_STRING_LENGTH;
	list_item.iItem = _index;

	// 3, TEXT("GC_type")
	list_item.iSubItem = 3;
	if (pfactory->type() <= eFACTORY_TYPE::POOL_USER)
	{
		wsprintf(text, TEXT("%hs"), to_string(factory_statistics.gc_type).data());
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
	}
	m_listctrl_object_factory.SetItem(&list_item);

	// 4, TEXT("existing")
	list_item.iSubItem = 4;
	set_number_text(factory_statistics.now.existing);
	m_listctrl_object_factory.SetItem(&list_item);

	// 5, TEXT("in using")
	list_item.iSubItem = 5;
	set_number_text(factory_statistics.now.in_using);
	m_listctrl_object_factory.SetItem(&list_item);

	// 6, TEXT("stack.pool")
	if(pfactory->type() <= eFACTORY_TYPE::POOL_USER)
	{
		list_item.iSubItem = 6;
		wsprintf(text, TEXT("%d"), static_cast<int>(pfactory->stacked_count()));
		list_item.pszText = text;
		m_listctrl_object_factory.SetItem(&list_item);
	}
	else
	{
		list_item.iSubItem = 6;
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));
		m_listctrl_object_factory.SetItem(&list_item);
	}

	// 8, TEXT("requested")
	list_item.iSubItem = 8;
	set_number_text(factory_statistics.now.alloc_create + factory_statistics.now.alloc_stack + factory_statistics.now.alloc_stack_tls);
	m_listctrl_object_factory.SetItem(&list_item);

	// 9, TEXT("alloc.new")
	list_item.iSubItem = 9;
	set_number_text(factory_statistics.now.alloc_create);
	m_listctrl_object_factory.SetItem(&list_item);

	//14,TEXT("free.del")
	list_item.iSubItem = 14;
	set_number_text(factory_statistics.now.free_delete);
	m_listctrl_object_factory.SetItem(&list_item);

	// 6, TEXT("stack.tls")
	if (pfactory->type() <= eFACTORY_TYPE::POOL_USER)
	{
		list_item.iSubItem = 7;
		set_number_text(factory_statistics.now.tls_stack_depth);
		m_listctrl_object_factory.SetItem(&list_item);
	}

	if (pfactory->type() <= eFACTORY_TYPE::POOL_USER && factory_statistics.gc_type != eFACTORY_GC_TYPE::NO_POOLING)
	{
		//10,TEXT("alloc.pool")
		list_item.iSubItem = 10;
		set_number_text(factory_statistics.now.alloc_stack);
		m_listctrl_object_factory.SetItem(&list_item);

		//12,TEXT("thr.pool")
		list_item.iSubItem = 12;
		if (factory_statistics.now.existing_limits != INT64_MAX)
		{
			set_number_text(factory_statistics.now.existing_limits);
		}
		else
		{
			list_item.pszText = const_cast<LPWSTR>(TEXT(""));
		}
		m_listctrl_object_factory.SetItem(&list_item);

		//16,TEXT("ratio.pool")
		list_item.iSubItem = 16;
		int64_t	f_allocated_total = factory_statistics.now.alloc_create + factory_statistics.now.alloc_stack + factory_statistics.now.alloc_stack_tls;
		if (f_allocated_total)
		{
			std::swprintf(text, std::size(text), std::wstring_view(TEXT("%6.3f%%")).data(), (float)factory_statistics.now.alloc_stack / (float)f_allocated_total * 100.0f);
			list_item.pszText = text;
		}
		else
		{
			list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
		}
		m_listctrl_object_factory.SetItem(&list_item);

		if (factory_statistics.gc_type == eFACTORY_GC_TYPE::TLS_POOLING || factory_statistics.gc_type == eFACTORY_GC_TYPE::TLS_POOLING_NO_GC)
		{
			//11,TEXT("alloc.tls")
			list_item.iSubItem = 11;
			set_number_text(factory_statistics.now.alloc_stack_tls);
			list_item.pszText = text;
			m_listctrl_object_factory.SetItem(&list_item);

			//13,TEXT("thr.tls")
			list_item.iSubItem = 13;
			set_number_text(factory_statistics.now.tls_stack_depth_threshold);
			list_item.pszText = text;
			m_listctrl_object_factory.SetItem(&list_item);

			//15,TEXT("free.tls")
			list_item.iSubItem = 15;
			set_number_text(factory_statistics.now.free_stack_tls);
			list_item.pszText = text;
			m_listctrl_object_factory.SetItem(&list_item);

			//17,TEXT("ratio.tls")
			list_item.iSubItem = 17;
			if (f_allocated_total)
			{
				std::swprintf(text, std::size(text), std::wstring_view(TEXT("%6.3f%%")).data(), (float)factory_statistics.now.alloc_stack_tls / (float)f_allocated_total * 100.0f);
				list_item.pszText = text;
			}
			else
			{
				list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
			}
			m_listctrl_object_factory.SetItem(&list_item);
		}
		else
		{
			// - set empty string
			list_item.pszText = const_cast<LPWSTR>(TEXT(""));

			//11,TEXT("alloc.tls")
			list_item.iSubItem = 11;
			m_listctrl_object_factory.SetItem(&list_item);

			//13,TEXT("thr.tls")
			list_item.iSubItem = 13;
			m_listctrl_object_factory.SetItem(&list_item);

			//15,TEXT("free.tls")
			list_item.iSubItem = 15;
			m_listctrl_object_factory.SetItem(&list_item);

			//17,TEXT("ratio.tls")
			list_item.iSubItem = 17;
			m_listctrl_object_factory.SetItem(&list_item);
		}
	}
	else
	{
		// - set empty string
		list_item.pszText = const_cast<LPWSTR>(TEXT(""));

		//10,TEXT("alloc.pool")
		list_item.iSubItem = 10;
		m_listctrl_object_factory.SetItem(&list_item);

		//11,TEXT("alloc.tls")
		list_item.iSubItem = 11;
		m_listctrl_object_factory.SetItem(&list_item);

		//12,TEXT("thr.pool")
		list_item.iSubItem = 12;
		m_listctrl_object_factory.SetItem(&list_item);

		//13,TEXT("thr.tls")
		list_item.iSubItem = 13;
		m_listctrl_object_factory.SetItem(&list_item);

		//15,TEXT("free.tls")
		list_item.iSubItem = 15;
		m_listctrl_object_factory.SetItem(&list_item);

		//16,TEXT("ratio.pool")
		list_item.iSubItem = 16;
		m_listctrl_object_factory.SetItem(&list_item);

		//17,TEXT("ratio.tls")
		list_item.iSubItem = 17;
		m_listctrl_object_factory.SetItem(&list_item);
	}

	//18,TEXT("ratio.D/C")
	list_item.iSubItem = 18;
	int64_t	fAllocatedCreate = factory_statistics.now.alloc_create;
	if (fAllocatedCreate != 0)
	{
		std::swprintf(text, std::size(text), std::wstring_view(TEXT("%6.3f%%")).data(), (float)factory_statistics.now.free_delete / fAllocatedCreate * 100.0f);
		list_item.pszText = text;
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
	}
	m_listctrl_object_factory.SetItem(&list_item);
}

void CDialog_object_factory_status::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 선택된 Interval을 얻어낸다.
	int	selected = m_combo_update_interval.GetCurSel();

	// 2) 해당하는 Update Interval로 설정한다.
	SetUpdateInterval(selected);
}

void CDialog_object_factory_status::OnBnClickedButtonUpdate()
{
	UpdateFactoryList();
}

void CDialog_object_factory_status::OnBnClickedButtonPrepare()
{
	//-----------------------------------------------------------------
	// 1. Amount를 읽어낸다.
	//-----------------------------------------------------------------
	// declare) 
	CDialogAmount dlg;

	// 1) 량을 입력받는다.
	INT_PTR	bResult = dlg.DoModal();

	// check) 
	RETURN_IF(bResult != IDOK);

	// 2) 값을 읽어낸다.
	int	nAmount = dlg.m_amount;


	//-----------------------------------------------------------------
	// 2. Amount 만큼 prepare한다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Factory Manager를 읽어 저장해 놓는다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 2) Redraw Off
	m_listctrl_object_factory.SetRedraw(FALSE);

	// 3) prepare한다.
	for(int i=0; i<m_listctrl_object_factory.GetItemCount(); ++i)
	{
		// check) 선택된 것이 아니라면 다음~
		CONTINUE_IF(m_listctrl_object_factory.GetItemState(i, LVIS_SELECTED)==0);

		// - Factory을 얻는다.
		auto pfactory_find = reinterpret_cast<Ifactory*>(m_listctrl_object_factory.GetItemData(i));

		// check) 
		CONTINUE_IF(pfactory_find == nullptr);

		// declare) 
		object_ptr<Ifactory> pfactory;

		// - Factory을 Factory Manager에서 찾는다.(이렇게 한 번 더 manager에서 찾는 이유는 Thread-Safe 문제 때문임.)
		SCOPED_SHARED_LOCK(pmanager->get_lockable())
		{
			pfactory = pmanager->find(pfactory_find);
		}

		// check) 
		CONTINUE_IF(pfactory.empty());

		// - prepare한다.
		pfactory->prepare(nAmount);

		// - 표시정보를 Update한다.
		update_factory_status(i);
	}

	// 4) Redraw On
	m_listctrl_object_factory.SetRedraw(TRUE);
}

void CDialog_object_factory_status::OnBnClickedButtonShrink()
{
	// declare) 
	CDialogAmountShrink	dlg;

	// 1) 량을 입력받는다.
	INT_PTR	bResult = dlg.DoModal();

	// check) 
	RETURN_IF(bResult != IDOK);

	// 2) 값을 읽어낸다.
	int	nAmount	 = dlg.m_amount;

	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Factory Manager를 읽어 저장해 놓는다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 2) Redraw Off
	m_listctrl_object_factory.SetRedraw(FALSE);

	// 3) prepare한다.
	for(int i=0; i<m_listctrl_object_factory.GetItemCount(); ++i)
	{
		// check) 선택된 것이 아니라면 다음~
		CONTINUE_IF(m_listctrl_object_factory.GetItemState(i, LVIS_SELECTED)==0);

		// - Factory을 얻는다.
		auto pfactory_find = reinterpret_cast<Ifactory*>(m_listctrl_object_factory.GetItemData(i));

		// check) 
		CONTINUE_IF(pfactory_find == nullptr);

		// declare) 
		object_ptr<Ifactory> pfactory;

		// - Factory을 Factory Manager에서 찾는다.(이렇게 한 번 더 manager에서 찾는 이유는 Thread-Safe 문제 때문임.)
		SCOPED_SHARED_LOCK(pmanager->get_lockable())
		{
			pfactory = pmanager->find(pfactory_find);
		}

		// check) 
		CONTINUE_IF(pfactory.empty());

		// - prepare한다.
		pfactory->shrink(nAmount);

		// - 표시정보를 Update한다.
		update_factory_status(i);
	}

	// 4) Redraw On
	m_listctrl_object_factory.SetRedraw(TRUE);
}

void CDialog_object_factory_status::OnBnClickedButtonEnableSustain()
{
	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Factory Manager를 읽어 저장해 놓는다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 2) Redraw Off
	pmanager->enable_sustain();
}

void CDialog_object_factory_status::OnBnClickedButtonDisableSustain()
{
	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Factory Manager를 읽어 저장해 놓는다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 2) Redraw Off
	pmanager->disable_sustain();
}

void CDialog_object_factory_status::OnBnClickedCheckEnableSustainManager()
{
	// declare) 
	object_ptr<factory::manager> pmanager;

	// 1) Connective Manager를 설정한다.
	{
		// lock) 
		std::lock_guard lock(this->m_lockable_pmanager);

		pmanager = m_pmanager;
	}

	// check) 
	RETURN_IF(pmanager.empty());

	// 2) Button의 상태를 읽어들인다.
	int	iButtonState = ((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_MEMORYBLOCK))->GetCheck();

	// check) 
	RETURN_IF((iButtonState!=0) == pmanager->is_enable_sustain())

	// 3) setting한다.
	pmanager->enable_sustain(iButtonState!=0);
}

void CDialog_object_factory_status::OnBnClickedCheckVisibleMemoryBlock()
{
	// 1) Button의 상태를 읽어들인다.
	int	iButtonState = ((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_MEMORYBLOCK))->GetCheck();

	// check) 
	RETURN_IF((iButtonState!=0)==m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_MEMORYBLOCK)))

	// 2) 값을 써넣는다.
	m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_MEMORYBLOCK)) = (iButtonState!=0);

	// 3) 표시정보를 Update한다.
	UpdateFactoryList();
}

void CDialog_object_factory_status::OnBnClickedCheckVisibleSystem()
{
	// 1) Button의 상태를 읽어들인다.
	int	iButtonState = ((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_SYSTEM))->GetCheck();

	// check) 
	RETURN_IF((iButtonState!=0) == m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_SYSTEM)))

	// 2) 값을 써넣는다.
	m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_SYSTEM)) = (iButtonState!=0);

	// 3) 표시정보를 Update한다.
	UpdateFactoryList();
}

void CDialog_object_factory_status::OnBnClickedCheckVisibleUser()
{
	// 1) Button의 상태를 읽어들인다.
	int	iButtonState = ((CButton*)GetDlgItem(IDC_CHECK_VISIBLE_USER))->GetCheck();

	// check) 
	RETURN_IF((iButtonState != 0) == m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_USER)))

	// 2) 값을 써넣는다.
	m_bVisiableTypeStatus.at(static_cast<int>(eFACTORY_TYPE::POOL_USER)) = (iButtonState!=0);

	// 3) 표시정보를 Update한다.
	UpdateFactoryList();
}

void CDialog_object_factory_status::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) 상태를 Update한다.
		UpdateFactoryList();
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CDialog_object_factory_status::SetUpdateInterval(int _interval)
{
	// 1) 
	m_update_interval = _interval;
	
	// 2) 
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

LRESULT CDialog_object_factory_status::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateFactoryList();
			}
			break;
	};

	return	0;
}
void CDialog_object_factory_status::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	// 1) Activate한다.
	CDialogEx::OnActivate(nState, pwnd_other, bMinimized);

	// 2) 타이머를 설정한다.
	SetUpdateInterval(m_update_interval);

	// 3) 값을 설정.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}


}
