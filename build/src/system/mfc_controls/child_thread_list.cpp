// CGChildThreadList.cpp : implementation file
//

#include "pch.h"
#include <charconv>

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_thread_list, CChildDialog)

CChild_thread_list::CChild_thread_list(api::manager_thread* _pthread_manager, CWnd* pnode_parent)
	: CChildDialog(IDD_DIALOG_THREAD_LIST, pnode_parent),
	m_pmanager_thread(_pthread_manager),
	m_is_enable_control(TRUE)
{
}

CChild_thread_list::~CChild_thread_list()
{
}

void CChild_thread_list::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_THREAD,	m_list_thread);
}


BEGIN_MESSAGE_MAP(CChild_thread_list, CChildDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_THREAD, OnLvnItemchangedListThread)
	ON_BN_CLICKED(IDC_BUTTON_FORCE_EXCEPTION, &CChild_thread_list::OnBnClickedForceException)
	ON_BN_CLICKED(IDC_BUTTON_SNAP_DUMP, &CChild_thread_list::OnBnClickedSnapDump)
	
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_thread_list::OnCGMessageNotify)
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

BOOL CChild_thread_list::CreateChild(api::manager_thread* _pthread_manager, CWnd* _pwnd)
{
	// declare) 
	BOOL	bResult;

	// 1) Thread Manager를 설정한다.
	SetThreadManager(_pthread_manager);

	// 2) 윈도우를 생성한다.
	bResult	 = Create(IDD_DIALOG_THREAD_LIST, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(bResult==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return true;
}

void CChild_thread_list::EnableControl(BOOL _is_enable)
{
	// 1) control의 상태를 설정한다.
	m_is_enable_control = _is_enable;

	// check) 아직 생성되지 않았으면 끝낸다.
	RETURN_IF(GetSafeHwnd()==nullptr);

	// 2) enable...
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(is_enable & _is_enable);
}

BOOL CChild_thread_list::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Acceptor Info
	m_list_thread.InsertColumn(0, TEXT("name"),			LVCFMT_LEFT,	150, 0);
	m_list_thread.InsertColumn(1, TEXT("ID"),			LVCFMT_LEFT,	120, 1);
	m_list_thread.InsertColumn(2, TEXT("Executed"),		LVCFMT_RIGHT,	100, 2);
	m_list_thread.InsertColumn(3, TEXT("Error"),		LVCFMT_RIGHT,	100, 3);
	m_list_thread.InsertColumn(4, TEXT("Executing"),	LVCFMT_RIGHT,	100, 4);
	m_list_thread.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	// 2) Controls
	BOOL is_enable = IsWindowEnabled();
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(is_enable & m_is_enable_control);


	//-----------------------------------------------------------------
	// 2. 값설정.
	//-----------------------------------------------------------------
	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_thread_list::OnLvnItemchangedListThread(NMHDR *pNMHDR, LRESULT *presult)
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
		//api::Nthread*	pthread	 = reinterpret_cast<api::Nthread*>(m_list_thread.GetItemData(pNMLV->iItem));
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)pthread);
	}
	// 4) Item을 해제한 것이다.
	else if(pNMLV->uNewState!=pNMLV->uOldState)
	{
		//SendMessageToTarget((WPARAM)CGACCEPTOR_SET_THREAD, (LPARAM)NULL);
	}

	*presult = 0;
}

void CChild_thread_list::OnNMDblclkListThread(NMHDR* pNMHDR, LRESULT *presult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// check) 
	RETURN_IF(pNMItemActivate->iItem < 0)

	// 1) ISocket을 얻는다.
	//api::Nthread*	pthread		 = reinterpret_cast<api::Nthread*>(m_list_thread.GetItemData(pNMItemActivate->iItem));

	// 2) Message를 날린다.
	//SendMessageToTarget((WPARAM)CGACCEPTOR_OPEN_THREAD, (LPARAM)pthread);

	// MFC기본)
	*presult = 0;
}

void CChild_thread_list::OnEnable(BOOL _is_enable)
{
	CChildDialog::OnEnable(_is_enable);

	// 1) control 버튼
	GetDlgItem(IDC_BUTTON_FORCE_EXCEPTION)->EnableWindow(_is_enable & m_is_enable_control);

	// 2) Thread 개수
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(_is_enable);
	GetDlgItem(IDC_STATIC_THREAD_COUNT)->EnableWindow(_is_enable);

	// 3) Thread List
	m_list_thread.EnableWindow(_is_enable);
}

void CChild_thread_list::OnDestroy()
{
	// 1) Thread List를 reset한다.
	ClearThreadList();

	// 2) 부모 Class의 destroy
	__super::OnDestroy();
}

void CChild_thread_list::SetThreadManager(api::manager_thread* _pthread_manager)
{
	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager_thread);

		m_pmanager_thread = _pthread_manager;
	}

	// 2) Update한다.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_thread_list::ClearThreadList()
{
	// lock)
	std::lock_guard lock(this->m_lockable_manager_thread);

	this->m_pmanager_thread.reset();
}

void CChild_thread_list::UpdateThreadList()
{
	// check) 윈도우가 생성되어 있지 않으면 끝낸다.
	RETURN_IF(m_hWnd == nullptr)

	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Thread Manager를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<api::manager_thread>	pmanager_thread;

	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager_thread);

		pmanager_thread = m_pmanager_thread;
	}

	//-----------------------------------------------------------------
	// 2. Manager가 Empty면 모두 지운다.
	//-----------------------------------------------------------------
	// check) Thread Manager가 nullptr이면 끝낸다.
	if(pmanager_thread.empty())
	{
		// 1) 모조리 지운다.
		m_list_thread.DeleteAllItems();

		// return) 
		return;
	}

	//-----------------------------------------------------------------
	// 3. List를 다시 setting한다.
	//-----------------------------------------------------------------
	// 1) Redraw Off
	m_list_thread.SetRedraw(FALSE);

	// declare) Index를 0으로...
	int index = 0;

	auto tick_now = chrono::tick::now();

	{
		// lock)
		std::lock_guard<std::shared_mutex> lock(pmanager_thread->get_lockable());

		// 2) Thread list Iterator의 begin과 end를 얻는다.
		auto iter = pmanager_thread->begin();
		auto iter_end = pmanager_thread->end();

		for(; iter!=iter_end; ++iter, ++index)
		{
			// 3) pthread를 얻는다.
			api::Nthread* pthread = *iter;

			// check) pthread가 nullptr이면 다음으로 넘긴다.(이럴리는 없다!)
			CONTINUE_IF(pthread==nullptr);

			// 4) 접속종료된 Connectable을 제거한다. 
			while(index <m_list_thread.GetItemCount())
			{
				// - Connectable을 얻는다.
				api::Nthread* pThreadCompare = reinterpret_cast<api::Nthread*>(m_list_thread.GetItemData(index));

				// check) 같으면 끝낸다~
				BREAK_IF(pThreadCompare == pthread);

				// - 지운다~
				m_list_thread.DeleteItem(index);
			}

			// 5) 새로 추가된 것이면 추가한다.
			if(index>=m_list_thread.GetItemCount())
			{
				m_list_thread.InsertItem(index, TEXT(""), -1);
				m_list_thread.SetItemData(index, (DWORD_PTR)pthread);
			}

			// 6) 기존에 있는 것 혹은 새로 추가된 것을 업데이트 한다.
			UpdateThread(index, tick_now);
		}
	}

	// 7) 아직도 남은 것들은 모조리 지운다.
	for(size_t	Count=m_list_thread.GetItemCount()-index; Count>0; Count--)
	{
		// -Item을 지운다.
		m_list_thread.DeleteItem(index);
	}

	// 8) Thread의 개수를 표시한다.
	SetDlgItemInt(IDC_STATIC_THREAD_COUNT, static_cast<int>(pmanager_thread->size()));

	// 9) Redraw On
	m_list_thread.SetRedraw(TRUE);
}

void CChild_thread_list::UpdateThread(int _index, chrono::tick::time_point _tick_now)
{
	// check)
	RETURN_IF(_index < 0 || _index >= m_list_thread.GetItemCount());

	// 1) Thread를 얻는다.
	api::Nthread*	pthread	 = reinterpret_cast<api::Nthread*>(m_list_thread.GetItemData(_index));

	// check) pthread가 nullptr이면 그냥 되돌린다.
	RETURN_IF(pthread == nullptr);

	// 2) Statistics
	const statistics::thread& rStatistics = pthread->get_thread_statistics();
	
	// 3) List를 설정한다.
	TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	char text_mb[MAX_STRING_LENGTH] = {};
	LV_ITEM	list_item;
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

	// 0	"name"
	list_item.iSubItem = 0;
#ifdef _UNICODE
	list_item.pszText = (TCHAR*)encode_string(text, pthread->name().data(), MAX_STRING_LENGTH).data();
#else
	list_item.pszText = const_cast<TCHAR*>(pthread->name().data());
#endif
	m_list_thread.SetItem(&list_item);
	
	// declare) 
	THREAD_HANDLE	id_thread = pthread->statistics().id_thread.load();
	
	// 1	"ID"
	list_item.iSubItem = 1;
	if(id_thread != INVALID_CGHANDLE_VALUE && id_thread != THREAD_HANDLE{})
	{
		list_item.pszText = text;
		std::swprintf(text, MAX_STRING_LENGTH, std::wstring_view(TEXT("%p")).data(), id_thread);
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
	}
	m_list_thread.SetItem(&list_item);
	
	// 3	"Executed"
	list_item.iSubItem = 2;
	set_number_text(rStatistics.count_executed.load());
	m_list_thread.SetItem(&list_item);

	// 4	"Error"
	list_item.iSubItem = 3;
	set_number_text(rStatistics.count_error.load());
	m_list_thread.SetItem(&list_item);

	// 5	"Executing"
	list_item.iSubItem = 4;
	if(rStatistics.is_executing)
	{
		set_number_text(std::chrono::duration_cast<chrono::tick::milliseconds>(_tick_now-rStatistics.tick_last_execute.load()).count());
	}
	else
	{
		list_item.pszText = const_cast<LPWSTR>(TEXT("-"));
	}
	m_list_thread.SetItem(&list_item);

	free(text);
}

LRESULT CChild_thread_list::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGUPDATE:
			{
				UpdateThreadList();
			}
			break;
	};

	return	0;
}

void CChild_thread_list::OnActivate(UINT nState, CWnd* pwnd_other, BOOL bMinimized)
{
	CChildDialog::OnActivate(nState, pwnd_other, bMinimized);

	// 1) Update한다.
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
}

void CChild_thread_list::OnTimer(UINT_PTR nIDEvent)
{
	// 1) Thread list Update
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		UpdateThreadList();
	}

	// 2) MFC Default
	CChildDialog::OnTimer(nIDEvent);
}

void CChild_thread_list::OnBnClickedForceException()
{
	// declare) 
	object_ptr<api::manager_thread>	pmanager_thread;

	// 1) Thread Manager를 설정한다.
	{
		// lock)
		std::lock_guard lock(this->m_lockable_manager_thread);

		pmanager_thread = m_pmanager_thread;
	}

	// check) 
	RETURN_IF(pmanager_thread.empty());

	// 2) 선택된 것을 force_raise_exception한다.
	{
		// lock)
		std::lock_guard<std::shared_mutex> lock(pmanager_thread->get_lockable());

		POSITION pos = m_list_thread.GetFirstSelectedItemPosition();
		while(pos)
		{
			int	nItem = m_list_thread.GetNextSelectedItem(pos);

			// 1) Connectable을 얻는다.
			api::Nthread* pthread = reinterpret_cast<api::Nthread*>(m_list_thread.GetItemData(nItem));

			// check) pConnectable이 nullptr일 수는 없다.
			CGASSERT_FATAL(pthread != nullptr);

			// check) the thread may have ended (and its object released) after the list was updated
			if (std::find(pmanager_thread->begin(), pmanager_thread->end(), pthread) == pmanager_thread->end())
				continue;

			// 2) Thread를 강재로 force_raise_exception한다.
			pthread->force_raise_exception("force raise exception");
		}
	}
}

void CChild_thread_list::OnBnClickedSnapDump()
{
	//-----------------------------------------------------------------
	// 1. Save할 File이름 얻어오기
	//-----------------------------------------------------------------
	// declare) 
	TCHAR dump_filename[MAX_STRING_LENGTH] {};

	CFileDialog	dlg(FALSE, TEXT("Save Dump"), NULL, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXPLORER, TEXT("Dump File (*.dmp)|*.dmp|All Files (*.*)|*.*||"));
	dlg.m_ofn.lpstrTitle = TEXT("Save Dump");
	dlg.m_ofn.lpstrFile = dump_filename;

	// 2) Do Modal해서 창을 띄운다.
	INT_PTR	presult = dlg.DoModal();

	// check) IDOK가 아니면 그냥 끝낸다.
	RETURN_IF(presult != IDOK);


	//-----------------------------------------------------------------
	// 2. Dump를 남긴다.
	//-----------------------------------------------------------------
	char temp[1024]{ "Test" };

	// 1) Dump를 남긴다.
	auto result = snap_dump(temp);

	// check) 
	if(result == false)
	{
		AfxMessageBox(TEXT("Fail to snap Mini dump"), MB_OK);
	}
}


}