#include "pch.h"
#include "log_filter_dlg.h"

#pragma warning(disable:4407)

#define	WM_ADD_STRING	(CGDK::eMESSAGE::WINDOWS::NOTIFY+10)

namespace CGDK
{

#ifdef _UNICODE
	#define	MFC_CHARACTER_SET	eCHARACTER_SET::UCS2
#else
	#define	MFC_CHARACTER_SET	eCHARACTER_SET::MBCS
#endif

IMPLEMENT_DYNAMIC(CChild_logger_view_ex, CChildDialog)

CChild_logger_view_ex::CChild_logger_view_ex(CWnd* pParent /*=NULL*/)
	: CChildDialog(IDD_DIALOG_LOGVIEW_EX, pParent),
	m_pqueue_log_record_queuing(),
	m_pqueue_log_record_process(),
	m_capacity(2048)
{
	// 2) 
	__zero_memory(this->m_count_log, sizeof(this->m_count_log));

	// declare) 
	LOGLIST_INFO tempLogList;

	// 3) 추가를 위해 값을 설정한다.
	lstrcpy(tempLogList.stName.data(), TEXT("Total"));
	tempLogList.plist_log = &this->m_list_log;
	tempLogList.iCount = 0;

	// 3) 추가한다.
	scoped_lock(this->m_lockable_log_info)
	{
		this->m_vector_log_list.push_back(tempLogList);
	}

	// 4) Default Color를 설정한다.
	this->m_rgb_text[static_cast<int>(log_type_t::info)] = RGB(0x00, 0x00, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::info)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::progress)] = RGB(0x00, 0x6f, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::progress)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::debug)] = RGB(0x7f, 0x7f, 0x7f);
	this->m_is_bold[static_cast<int>(log_type_t::debug)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::fatal)] = RGB(0xff, 0x00, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::fatal)] = true;

	this->m_rgb_text[static_cast<int>(log_type_t::error)] = RGB(0xff, 0x40, 0x40);
	this->m_is_bold[static_cast<int>(log_type_t::error)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::warning)] = RGB(0xcf, 0x40, 0x40);
	this->m_is_bold[static_cast<int>(log_type_t::warning)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::user)] = RGB(0xaf, 0xaf, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::user)] = false;

	this->m_rgb_text[static_cast<int>(log_type_t::system)] = RGB(0x00, 0x00, 0xff);
	this->m_is_bold[static_cast<int>(log_type_t::system)] = false;
}

CChild_logger_view_ex::~CChild_logger_view_ex()
{
	// locked) 
	{
		// scoped_lock
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// 1) 생성된 모든 LogView를 제거한다.
		for(auto iter=this->m_vector_log_list.begin()+1; iter!=this->m_vector_log_list.end(); ++iter)
		{
			SAFE_DELETE((*iter).plist_log);
		}

		// 2) LogList를 clear한다.
		this->m_vector_log_list.clear();
	}
}

void CChild_logger_view_ex::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_LOG, m_tabLog);
	DDX_Control(pDX, IDC_LIST_LOGVIEW, m_list_log);
}

result_code CChild_logger_view_ex::process_message(sMESSAGE& _msg)
{
	if(_msg.message == eMESSAGE::SYSTEM::LOG_)
	{
		this->on_message_Log((sMESSAGE_LOG&)_msg);
	}

	// return) Log는 무조건 eRESULT::BYPASS를 리턴한다.
	return eRESULT::BYPASS;
}

result_code CChild_logger_view_ex::on_message_Log(sMESSAGE_LOG& _msg)
{
	// 1) trace...
	this->trace(_msg.plog_record);

	// return) 
	return eRESULT::BYPASS;
}

BEGIN_MESSAGE_MAP(CChild_logger_view_ex, CChildDialog)
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_LBN_SELCHANGE(IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnLbnSelchangeListLog)
	ON_LBN_DBLCLK(IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnLbnDblclkListLog)
	ON_LBN_ERRSPACE(IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnLbnErrspaceListLog)
	ON_LBN_SELCANCEL(IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnLbnSelcancelListLog)
	ON_NOTIFY(NM_THEMECHANGED, IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnNMThemeChangedListLog)
	ON_NOTIFY(NM_THEMECHANGED, IDC_LIST_LOGVIEW, &CChild_logger_view_ex::OnNMThemeChangedListLog)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_logger_view_ex::OnCGMessageNotify)
	ON_MESSAGE(WM_ADD_STRING, &CChild_logger_view_ex::OnAddString)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_LOG, &CChild_logger_view_ex::OnTcnSelchangeTabInfo)
	ON_NOTIFY(TCN_SELCHANGING, IDC_TAB_LOG, &CChild_logger_view_ex::OnTcnSelchangingTabInfo)
	ON_BN_CLICKED(IDC_BUTTON_LOG_make_object, &CChild_logger_view_ex::OnBnClickedNewLog)
	ON_BN_CLICKED(IDC_BUTTON_EDIT_FILTER, &CChild_logger_view_ex::OnBnClickedEditFilter)
	ON_BN_CLICKED(IDC_BUTTON_LOG_CLOSE, &CChild_logger_view_ex::OnBnClickedLogClose)
	ON_BN_CLICKED(IDC_BUTTON_LOG_SAVE, &CChild_logger_view_ex::OnBnClickedLogSave)
	ON_BN_CLICKED(IDC_BUTTON_LOG_SAVE_SELECTED, &CChild_logger_view_ex::OnBnClickedLogSaveSelected)
END_MESSAGE_MAP()


// CChild_logger_view_ex message handlers


BOOL CChild_logger_view_ex::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	// 1) 기본 Log의 위치를 구해 놓는다.
	CRect tempRect;
	this->m_list_log.GetWindowRect(tempRect);
	ScreenToClient(tempRect);

	scoped_lock(this->m_lockable_log_info)
	{
		for(std::vector<LOGLIST_INFO>::iterator iter=m_vector_log_list.begin(); iter!=m_vector_log_list.end(); ++iter)
		{
			// check) 
			LOGLIST_INFO& rListInfo = *iter;

			// 2) Tab을 추가한다.
			m_tabLog.InsertItem(m_tabLog.GetItemCount(), rListInfo.stName.data());

			// check) 
			CONTINUE_IF(rListInfo.plist_log != nullptr);

			// declare) 
			CListBox_log* plog_box = nullptr;

			// 3) LogBox를 생성한다.
			plog_box = new CListBox_log();

			try
			{
				// 4) Log ListBox Cotrol을 생성한다.
				auto result = plog_box->Create(WS_CHILD | WS_BORDER | WS_VSCROLL, tempRect, this, 0);
		
				// check) 제대로 생성했나?
				THROW_IF(result == 0, throwable::failure(eRESULT::DOMAIN_ERROR, 0, "fail to create log box!!"sv));

				// 6) Font를 설정한다.
				plog_box->SetFont(m_list_log.GetFont());

				// 7) Log ListBox를 설정한다.
				rListInfo.plist_log = plog_box;
			}
			catch(...)
			{
				plog_box->DestroyWindow();

				delete plog_box;
			}
		}
	}

	// 8) 선택
	m_tabLog.SetCurSel(0);

	ShowList(0, SW_SHOW);

	SetDlgItemInt(IDC_STATIC_LOG_COUNT, 0);

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_logger_view_ex::OnSizing(UINT fwSide, LPRECT pRect)
{
	CGDIALOG::OnSizing(fwSide, pRect);

	// 1) Width의 한계를 설정한다.

	// 2) Height의 한계를 설정한다.
}

void CChild_logger_view_ex::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if(m_list_log.m_hWnd!=NULL)
	{
		// 1) 전체 크기를 먼저 얻는다.
		CRect tempRectOriginal;
		GetClientRect(tempRectOriginal);

		// declare) 
		CRect tempRect;

		// 2) Tab의 위치를 수정한다.
		m_tabLog.GetWindowRect(tempRect);

		ScreenToClient(tempRect);

		tempRect.left	 = 0;
		tempRect.right	 = tempRectOriginal.right;
		tempRect.bottom	 = tempRectOriginal.bottom;

		m_tabLog.MoveWindow(tempRect);

		// 3) List들의 크기를 수정한다.
		scoped_lock(m_lockable_log_info)
		{
			for(auto& iter:m_vector_log_list)
			{
				iter.plist_log->GetWindowRect(tempRect);

				ScreenToClient(tempRect);

				tempRect.left	 = 5;
				tempRect.right	 = tempRectOriginal.right-5;
				tempRect.bottom	 = tempRectOriginal.bottom-5;

				iter.plist_log->MoveWindow(tempRect);
			}
		}
	}
}

void CChild_logger_view_ex::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	m_tabLog.EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_LOG_NAME)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_EDIT_FILTER)->EnableWindow(is_enable);

	GetDlgItem(IDC_BUTTON_LOG_CLOSE)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_LOG_SAVE)->EnableWindow(is_enable);
	GetDlgItem(IDC_BUTTON_LOG_SAVE_SELECTED)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_COUNT)->EnableWindow(is_enable);
	
	
	scoped_lock(m_lockable_log_info)
	{
		for(std::vector<LOGLIST_INFO>::iterator iter=m_vector_log_list.begin(); iter!=m_vector_log_list.end(); ++iter)
		{
			// check) 
			CONTINUE_IF((*iter).plist_log == nullptr);

			iter->plist_log->EnableWindow(is_enable);
		}
	}

	GetDlgItem(IDC_STATIC_LOG_TOTAL)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_SYSTEM)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_PROGRESS)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_INFO)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_FATAL)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_ERROR)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_DEBUG)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_ETC)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T4)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T5)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T6)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T7)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T8)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T9)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T10)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T11)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T13)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T14)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T15)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T16)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T17)->EnableWindow(is_enable);
}

void CChild_logger_view_ex::OnDestroy()
{
	__super::OnDestroy();
}

LRESULT CChild_logger_view_ex::OnAddString(WPARAM /*_wParam*/, LPARAM /*_lParam*/)
{
	// declare)
	int	count_added[static_cast<size_t>(log_type_t::max_)];

	// 1) clear
	__zero_memory(count_added, sizeof(int) * static_cast<size_t>(log_type_t::max_));

	// 2) add log record to listctrl
	scoped_lock(m_lockable_log_info)
	{
		// Disable) 
		{
			auto iter_selected = m_vector_log_list.begin() + m_tabLog.GetCurSel();

			iter_selected->plist_log->SetRedraw(0);
		}

		scoped_lock(m_lockable_queue_string)
		{
			// check) Process Queue는 비어있어야만 한다.
			CGASSERT_ERROR(m_pqueue_log_record_process.empty());

			// - swap
			m_pqueue_log_record_process.swap(m_pqueue_log_record_queuing);
		}

		while(!m_pqueue_log_record_process.empty())
		{
			// 1) 제일 앞 MSG처리
			ProcessAddString(m_pqueue_log_record_process.front(), count_added);

			// 2) 제일 앞 제거
			m_pqueue_log_record_process.pop_front();
		}

		// Enable) 
		{
			auto iter_selected = m_vector_log_list.begin() + m_tabLog.GetCurSel();

			iter_selected->plist_log->SetRedraw(1);
		}
	}

	// 3) update log count info
	for(int i= 0; i<static_cast<int>(log_type_t::max_); ++i)
	{
		// - get added log count at i
		int	count = count_added[i];

		// check) continue if 0
		CONTINUE_IF(count == 0);

		// - add log count
		m_count_log[i] += count;
		m_count_log_total += count;

		// - 화면 갱신
		SetDlgItemInt(IDC_STATIC_LOG_INFO + i, m_count_log[i]);
	}

	// 3) 전체 Log 갯수를 표시한다.
	SetDlgItemInt(IDC_STATIC_LOG_TOTAL, static_cast<int>(m_count_log_total));
	
	// return) 
	return	0;
}

void CChild_logger_view_ex::ProcessAddString(LOG_RECORD* _plog_record, int (&_count_added)[static_cast<size_t>(log_type_t::max_)])
{
	// check) 
	CGASSERT_ERROR(_plog_record != nullptr);

	// check) 
	RETURN_IF(_plog_record == nullptr);

	// declare) 
	object_ptr<CListBox_log::ITEM_LOG>	pitem = make_object<CListBox_log::ITEM_LOG>();

	// 1) Type을 얻는다.
	auto log_type = _plog_record->type & 0xffff;

	// 2) Log Time을 만든다.
	if ((uint32_t)(_plog_record->type & log_type_t::continue_) == 0)
	{
		// - convert to date_time_fraction
		auto temp_dtf = chrono::to_date_time(_plog_record->time_occure);

		// - make string
		__sprintf(pitem->str_head, TEXT("[%4u-%02u-%02u %02u:%02u:%02u]"),
			temp_dtf.t_year,
			temp_dtf.t_month,
			temp_dtf.t_day,
			temp_dtf.t_hour,
			temp_dtf.t_minute,
			temp_dtf.t_second);

		// - Type에 따라 View를 update한다.
		if (log_type >= log_type_t::info && log_type < log_type_t::max_)
		{
			pitem->color_text = m_rgb_text[static_cast<int>(log_type)];
			pitem->is_bold = (_plog_record->level >= log_level_t::higher()) || m_is_bold[static_cast<int>(log_type)];
		}
		else
		{
			pitem->color_text = RGB(0, 0, 0);
			pitem->is_bold = false;
		}
	}
	else
	{
		// - Item..
		pitem->str_head[0] = NULL;
	}

	// 3) CHAR_ENCODING이 다를 경우 변환한다.

	// 4) Log Message를 저장한다.
	//    - _plog_record의 CHAR_ENCODING값이 MFC_CHARACTER_SET과 같을 경우 그대로 사용하고
	//	  - 다를 경우 문자인코딩을 변경하여 새로 만든다.
	if (_plog_record->char_encoding == MFC_CHARACTER_SET)
	{
		pitem->plog_record = _plog_record;
	}
	else
	{
		pitem->plog_record = _plog_record->clone(MFC_CHARACTER_SET);
	}

	// 5)
	shared_buffer buf_log = pitem->plog_record->buf_message;

	// 5) string을 읽어들인다.
	auto vector_string = buf_log.extract<std::vector<std::basic_string_view<TCHAR>>>();

	// 6) PUSH!!
	scoped_lock(m_lockable_log_info)
	{
		// - 
		auto iter = vector_string.begin();
		auto iter_end = vector_string.end();

		// - string을 설정한다.
		pitem->str_message = iter->data();

		// - 첫번째 줄을 추가한다.
		ProcessAddString(ITEM_LOG(pitem->plog_record, pitem));
		++_count_added[static_cast<int>(log_type)];

		// - 두번째 줄 이후를 추가한다.
		for (++iter; iter!=iter_end; ++iter)
		{
			// declare) 
			object_ptr<CListBox_log::ITEM_LOG>	pitem_next = make_object<CListBox_log::ITEM_LOG>();

			// - ...
			pitem_next->str_head[0]	 = NULL;
			pitem_next->str_message	 = iter->data();
			pitem_next->plog_record	 = pitem->plog_record;
			pitem_next->color_text	 = pitem->color_text;
			pitem_next->is_bold		 = pitem->is_bold;

			// - 두번째 이후 줄을 추가한다.
			ProcessAddString(ITEM_LOG(pitem->plog_record, pitem_next));
		}
	}
}

void CChild_logger_view_ex::ProcessAddString(ITEM_LOG&& _item_log)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		for(auto& iter:m_vector_log_list)
		{
			// - Filter가 있으면 Filtering검사를 수행한다.
			if(iter.plog_filter.exist())
			{
				// - Filtering 검사를 수행한다.
				auto result = iter.plog_filter->process_filtering(_item_log.plog_record);

				// check) Filtering되었다면 다음...
				CONTINUE_IF(result == false);
			}
		
			// - String을 추가한다.
			AddString(iter.plist_log, _item_log.pitem);

			// - 갯수를 증가시킨다.
			++(iter.iCount);
		}
	}
}



BOOL CChild_logger_view_ex::CreateChild(const void* /*_pInfo*/, CWnd* _pWnd)
{
	// 1) Statistics의 포인터를 저장~
	//m_pstatistics	 = _pInfo;

	// 2) 윈도우를 생성한다.
	auto result = Create(IDD_DIALOG_LOGVIEW_EX, _pWnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return result;
}

void CChild_logger_view_ex::AddString(CListBox_log* _lParam, CListBox_log::ITEM_LOG* _pitem)
{
	// 1) Log에 String을 추가한다.
	_lParam->AddString(_pitem);

	// 2) Top Index를 얻어놓는다.
	int	iTopIndex = _lParam->GetTopIndex();

	// 3) 최대한계치 이상추가가 되었으면 제거한다.
	if(_lParam->GetCount() > m_capacity)
	{
		_lParam->DeleteString(0);
	}

	// 4) Scroll한다.
	_lParam->SetTopIndex(iTopIndex+1);
}


LRESULT CChild_logger_view_ex::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
{
	switch(_wParam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
			}
			break;

	case	CGACCEPTOR_READY:
			{
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
			}
			break;

	case	CGACCEPTOR_START:
			{
			}
			break;

	case	CGACCEPTOR_STOP:
			{
			}
			break;
	};

	return	0;

}

void CChild_logger_view_ex::OnBnClickedNewLog()
{
	// Declare0 
	INT_PTR	result;

	// 1) create
	CLogFilterDlg dlg;

	// 2) DoModal
	result = dlg.DoModal();

	// check) 
	RETURN_IF(result != IDOK);

	// 3) 처리...
	AddLogList(dlg.m_strName.data(), dlg.m_pFilter, true);
}

void CChild_logger_view_ex::OnBnClickedEditFilter()
{
	// 1) 선택된 LogList를 얻는다.
	int	iSel = m_tabLog.GetCurSel();

	// check) 
	RETURN_IF(iSel < 1);

	LOGLIST_INFO rLogList;
	
	scoped_lock(m_lockable_log_info)
	{
		// check) 
		RETURN_IF(iSel >= (int)m_vector_log_list.size());

		rLogList = m_vector_log_list.at(iSel);
	}

	object_ptr<log_filter> pFilter = dynamic_pointer_cast<log_filter>(rLogList.plog_filter);

	// check) 
	RETURN_IF(pFilter.empty());
	
	// 2) create
	CLogFilterDlg dlg;

	// 3) 값을 설정
	lstrcpy(dlg.m_strName.data(), rLogList.stName.data());
	dlg.m_pFilter = pFilter;

	// 4) DoModal
	auto result = dlg.DoModal();

	// check)
	RETURN_IF(result != IDOK);

	// 5) 값을 설정한다.
	scoped_lock(m_lockable_log_info)
	{
		// check) 
		RETURN_IF(iSel >= (int)m_vector_log_list.size());

		m_vector_log_list.at(iSel).plog_filter = dlg.m_pFilter;
	}
}

void CChild_logger_view_ex::OnBnClickedLogClose()
{
	int	iSelect = m_tabLog.GetCurSel();

	// check) Total이면 지울 수 없다.
	RETURN_IF(iSelect<1 || iSelect>=(int)m_vector_log_list.size());

	// declare) 
	TCHAR tempString[MAX_TRACE_STRING_LENGTH];

	// 1) LogListInfo를 얻는다.
	const LOGLIST_INFO&	rLogListInfo = m_vector_log_list.at(iSelect);

	// 2) String 만들기.
	wsprintf(tempString, TEXT("Do you really want to delete Log View tab [%s]?"), rLogListInfo.stName);
	
	// check) 진짜 지울건지 다시 한번 확인한다.
	RETURN_IF(AfxMessageBox(tempString, MB_OKCANCEL)!=IDOK);

	// 3) 지운다.
	RemoveLogList(iSelect);

	// 4) 새로운 Tab으로 설정한다.
	if(iSelect==m_tabLog.GetItemCount())
	{
		--iSelect;
	}

	// 4) 새로운 Tab으로...
	m_tabLog.SetCurSel(iSelect);
	ShowList(iSelect, SW_SHOW);
}


void CChild_logger_view_ex::OnBnClickedLogSave()
{
	//-----------------------------------------------------------------
	// 1. Save할 File이름 얻어오기
	//-----------------------------------------------------------------
	// declare) 
	INT_PTR	pResult;
	TCHAR temp_file_name[MAX_STRING_LENGTH] {};

	CFileDialog	dlg(FALSE, TEXT("Save Log"), NULL, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXPLORER, TEXT("Log File (*.Log)|*.log|Text File (*.txt)|*.txt|All Files (*.*)|*.*||"));
	dlg.m_ofn.lpstrTitle = TEXT("Save Log");
	dlg.m_ofn.lpstrFile  = temp_file_name;

	// 2) Do Modal해서 창을 띄운다.
	pResult = dlg.DoModal();

	// check) IDOK가 아니면 그냥 끝낸다.
	RETURN_IF(pResult != IDOK);


	//-----------------------------------------------------------------
	// 2. Save하기
	//-----------------------------------------------------------------
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR* str_text = new TCHAR[MAX_TRACE_STRING_LENGTH];

	try
	{
		// 1) file 객체를 생성한다.
		auto pfile = make_object<archive::file>();

		// 2) File을 open한다.
	#ifdef _UNICODE
		pfile->start(archive::file::START_PARAMETER{ encode_string<char>(std::wstring_view(temp_file_name)), eFILE_OPERATION::APPEND_ASYNC });
	#else
		pfile->start(archive::file::START_PARAMETER{ temp_file_name, eFILE_OPERATION::APPEND_ASYNC });
	#endif

		// 3) 모든 Data를 저장한다.
		scoped_lock(m_lockable_log_info)
		{
			// - 현재의 Selection을 얻는다.
			int	iSel = m_tabLog.GetCurSel();

			// check) 
			THROW_IF(iSel<0 || iSel>=(int)m_vector_log_list.size(), throwable::failure(eRESULT::OUT_OF_RANGE, 0, "selection is out of range"sv));

			// - 현재 선택된 LogList Info를 얻는다.
			LOGLIST_INFO& rLogListInfo = m_vector_log_list.at(iSel);

			// - 설정한다.
			int count = rLogListInfo.plist_log->GetCount();

			for(int i=0; i<count; ++i)
			{
				// declare) 
				int string_length;

				// - String을 얻는다.
				string_length = rLogListInfo.plist_log->GetText(i, str_text);

			#ifdef _UNICODE
				LOG_USER << "Text: "sv << encode_string<char>(std::wstring_view(str_text));
			#else
				LOG_USER << "Text: " << str_text;
			#endif

				// check) 
				BREAK_IF(string_length == LB_ERR);

				// - 저장한다.
				pfile->write(std::wstring_view(str_text, string_length));
				pfile->write(std::wstring_view(TEXT("\r\n")));
			}

			// - 닫는다.
			pfile->destroy();
		}
	}

	catch(std::exception& e)
	{
		SAFE_CLOSEHANDLE(hFile);

		wsprintf(str_text, TEXT("Fail to Save [%hs]"), e.what());

		AfxMessageBox(str_text, MB_OK);
	}
	
	catch(...)
	{
		SAFE_CLOSEHANDLE(hFile);

		AfxMessageBox(TEXT("Fail to Save [?]"), MB_OK);
	}

	delete[]	str_text;
}

void CChild_logger_view_ex::OnBnClickedLogSaveSelected()
{
	//-----------------------------------------------------------------
	// 1. Save할 File이름 얻어오기
	//-----------------------------------------------------------------
	// declare) 
	INT_PTR pResult;
	TCHAR temp_file_name[MAX_STRING_LENGTH] {};

	CFileDialog	dlg(FALSE, TEXT("Save Selected Log"), NULL, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING | OFN_EXPLORER, TEXT("Log File (*.Log)|*.log|Text File (*.txt)|*.txt|All Files (*.*)|*.*||"));
	dlg.m_ofn.lpstrTitle = TEXT("Save Selected Log");
	dlg.m_ofn.lpstrFile	= temp_file_name;

	// 2) Do Modal해서 창을 띄운다.
	pResult = dlg.DoModal();

	// check) IDOK가 아니면 그냥 끝낸다.
	RETURN_IF(pResult != IDOK);


	//-----------------------------------------------------------------
	// 2. Save하기
	//-----------------------------------------------------------------
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR* str_text = new TCHAR[MAX_TRACE_STRING_LENGTH];

	try
	{
		// 1) make archive::file
		auto pfile = make_object<archive::file>();

		// 2) File을 open한다.
	#ifdef _UNICODE
		pfile->start(archive::file::START_PARAMETER{ encode_string<char>(std::wstring_view(temp_file_name)), eFILE_OPERATION::APPEND_ASYNC });
	#else
		pfile->start(archive::file::START_PARAMETER{ temp_file_name, eFILE_OPERATION::APPEND_ASYNC });
	#endif

		// 3) 모든 Data를 저장한다.
		scoped_lock(m_lockable_log_info)
		{
			// - 현재의 Selection을 얻는다.
			int	iSel = m_tabLog.GetCurSel();

			// check) 
			THROW_IF(iSel<0 || iSel>=(int)m_vector_log_list.size(), throwable::failure(eRESULT::OUT_OF_RANGE, 0, "selection is out of range"sv));

			// - 현재 선택된 LogList Info를 얻는다.
			LOGLIST_INFO& rLogListInfo = m_vector_log_list.at(iSel);

			// - 선택된 Text의 갯수를 얻는다.
			int iCountSelected = rLogListInfo.plist_log->GetSelCount();

			// Case) 전혀 선택된 것이 없다면 전체를 저장한다.
			if(iCountSelected==0)
			{
				// - 설정한다.
				int	count = rLogListInfo.plist_log->GetCount();

				for(int i=0; i<count; ++i)
				{
					// - String을 얻는다.
					auto string_length = rLogListInfo.plist_log->GetText(i, str_text);

				#ifdef _UNICODE
					LOG_USER << "text: "sv << encode_string<char>(std::wstring_view(str_text));
				#else
					LOG_USER << "text: "sv << str_text;
				#endif
					// check) 
					BREAK_IF(string_length == LB_ERR);

					// - 저장한다.
					pfile->write(std::wstring_view(str_text, string_length));
					pfile->write(std::wstring_view(TEXT("\r\n")));
				}
			}
			// Case) 선택된 것이 있다면 선택된 것만 저장한다.
			else
			{
				CArray<int,int>	arrayListSel;
				arrayListSel.SetSize(iCountSelected);
				rLogListInfo.plist_log->GetSelItems(iCountSelected, arrayListSel.GetData());

				for( int i=0; i< iCountSelected; i++ )
				{
					// - String을 얻는다.
					auto string_length = rLogListInfo.plist_log->GetText(arrayListSel.GetAt(i), str_text);

					// check) 
					BREAK_IF(string_length==LB_ERR);

					// - 저장한다.
					pfile->write(std::wstring_view(str_text, string_length));
					pfile->write(std::wstring_view(TEXT("\r\n")));
				}
 			}
			
			// - 닫는다.
			pfile->destroy();
		}
	}

	catch(std::exception& e)
	{
		SAFE_CLOSEHANDLE(hFile);

		wsprintf(str_text, TEXT("Fail to Save [%hs]"), e.what());

		AfxMessageBox(str_text, MB_OK);
	}
	
	catch(...)
	{
		SAFE_CLOSEHANDLE(hFile);

		AfxMessageBox(TEXT("Fail to Save [?]"), MB_OK);
	}

	// - 할당해제
	delete[] str_text;
}

void CChild_logger_view_ex::OnLbnSelchangeListLog()
{
	// TODO: Add your control notification handler code here
}

void CChild_logger_view_ex::OnLbnDblclkListLog()
{
	// TODO: Add your control notification handler code here
}

void CChild_logger_view_ex::OnLbnErrspaceListLog()
{
	// TODO: Add your control notification handler code here
}

void CChild_logger_view_ex::OnLbnSelcancelListLog()
{
	// TODO: Add your control notification handler code here
}

void CChild_logger_view_ex::OnNMThemeChangedListLog(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = 0;
}

void CChild_logger_view_ex::OnTcnSelchangingTabInfo(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;

	ShowList(m_tabLog.GetCurSel(), SW_HIDE);
}

void CChild_logger_view_ex::OnTcnSelchangeTabInfo(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;

	int	iCurSel = m_tabLog.GetCurSel();

	ShowList(iCurSel, SW_SHOW);

	scoped_lock(m_lockable_log_info)
	{
		SetDlgItemInt(IDC_STATIC_LOG_COUNT, (m_vector_log_list.begin()+iCurSel)->iCount);
	}
}

void CChild_logger_view_ex::ShowList(int _tab_index, int p_iCmdShow)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// check) 
		RETURN_IF(_tab_index < 0 || _tab_index >= (int)m_vector_log_list.size());
	
		// 1) 해당 LogListInfo를 얻는다.
		LOGLIST_INFO& rLogListInfo = m_vector_log_list.at(_tab_index);

		// check) 
		RETURN_IF(rLogListInfo.plist_log == nullptr);

		// 2) 
		rLogListInfo.plist_log->ShowWindow(p_iCmdShow);
	}
}

void CChild_logger_view_ex::AddLogList(const TCHAR* _name, Ilog_filter* _plog_filter, bool _bFocus)
{
	// check) 
	RETURN_IF(_name == nullptr);

	// declare) 
	bool bCreateWindow = m_hWnd!=NULL;

	// 1) 기본 Log의 위치를 구해 놓는다.
	CRect tempRect;

	if(bCreateWindow)
	{
		this->m_list_log.GetWindowRect(tempRect);
		this->ScreenToClient(tempRect);
	}

	// declare) ListBox
	CListBox_log* plog_box = nullptr;

	try
	{
		if(bCreateWindow)
		{
			// 2) LogBox를 생성한다.
			plog_box = new CListBox_log();

			// 3) Log ListBox를 생성한다.
			auto result = plog_box->Create(WS_CHILD | WS_BORDER | WS_VSCROLL, tempRect, this, 0);

			// check) 제대로 생성했나?
			THROW_IF(result == 0, throwable::failure(eRESULT::DOMAIN_ERROR, 0, "fail to create log box!!"sv));

			// 4) Font를 설정한다.
			plog_box->SetFont(this->m_list_log.GetFont());
			
			// 5) Tab을 새로 추가한다.
			auto tabNow = this->m_tabLog.InsertItem(this->m_tabLog.GetItemCount(), _name);

			// 6) Focus를 맞춤.
			if(_bFocus)
			{
				ShowList(this->m_tabLog.GetCurSel(), SW_HIDE);
				plog_box->ShowWindow(SW_SHOW);
				this->m_tabLog.SetCurSel(tabNow);
				this->SetDlgItemInt(IDC_STATIC_LOG_COUNT, 0);
			}
		}
	}
	catch(...)
	{
		SAFE_DELETE(plog_box);

		throw;
	}

	// 7) 추가를 위해 값을 설정한다.
	LOGLIST_INFO tempLogList;

	lstrcpy(tempLogList.stName.data(), _name);
	tempLogList.plist_log = plog_box;
	tempLogList.plog_filter = _plog_filter;
	tempLogList.iCount = 0;

	scoped_lock(this->m_lockable_log_info)
	{
		this->m_vector_log_list.push_back(tempLogList);
	}
}

void CChild_logger_view_ex::RemoveLogList(const TCHAR* _name)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		for(int i=0; i<(int)this->m_vector_log_list.size(); ++i)
		{
			LOGLIST_INFO& rLogListInfo = this->m_vector_log_list.at(i);

			if(lstrcmp(rLogListInfo.stName.data(), _name)==0)
			{
				this->RemoveLogList(i);
				break;
			}
		}
	}
}

void CChild_logger_view_ex::RemoveLogList(int _tab_index)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// check) 
		ERROR_RETURN_IF(_tab_index <1 || _tab_index >= (int)this->m_vector_log_list.size(), , LOG_ERROR << "(err ) fail to removing log_view tab out of range["sv << _tab_index << "]"sv);

		// 1) Tab을 제거한다.
		this->m_tabLog.DeleteItem(_tab_index);

		// 2) 해당 LogListInfo를 얻는다.
		LOGLIST_INFO rLogListInfo = this->m_vector_log_list.at(_tab_index);

		// 3) List에서 제거한다.
		this->m_vector_log_list.erase(this->m_vector_log_list.begin()+ _tab_index);

		// 4) 지운다.
		if(rLogListInfo.plist_log != nullptr)
		{
			rLogListInfo.plist_log->DestroyWindow();
			delete rLogListInfo.plist_log;
		}
	}
}

void CChild_logger_view_ex::set_log_filter(const TCHAR* _name, Ilog_filter* _plog_filter)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		for(int i=0; i<(int)this->m_vector_log_list.size(); ++i)
		{
			LOGLIST_INFO& rLogListInfo = this->m_vector_log_list.at(i);

			if(lstrcmp(rLogListInfo.stName.data(), _name)==0)
			{
				rLogListInfo.plog_filter = _plog_filter;
				break;
			}
		}
	}
}

void CChild_logger_view_ex::set_log_filter(int _tab_index, Ilog_filter* _plog_filter)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// check) 
		ERROR_RETURN_IF(_tab_index <0 || _tab_index >= (int)this->m_vector_log_list.size(), , LOG_ERROR << "(err ) fail to removing log_view tab out of range["sv << _tab_index << "]"sv);

		// 1) 해당 LogListInfo를 얻는다.
		LOGLIST_INFO& rLogListInfo = this->m_vector_log_list.at(_tab_index);

		// 2) Log Filter를 재설정한다.
		rLogListInfo.plog_filter = _plog_filter;
	}
}

void CChild_logger_view_ex::reset_log_filter(const TCHAR* _name)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		for(int i=0; i<(int)this->m_vector_log_list.size(); ++i)
		{
			LOGLIST_INFO& rLogListInfo = this->m_vector_log_list.at(i);

			if(lstrcmp(rLogListInfo.stName.data(), _name)==0)
			{
				rLogListInfo.plog_filter.reset();
				break;
			}
		}
	}
}

void CChild_logger_view_ex::Clear()
{
}

void CChild_logger_view_ex::reset_log_filter(int _tab_index)
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// check) 
		ERROR_RETURN_IF(_tab_index < 0 || _tab_index >= (int)m_vector_log_list.size(), , LOG_ERROR << "(err ) fail to removing log_view tab out of range["sv << _tab_index << "]"sv);

		// 1) 해당 LogListInfo를 얻는다.
		LOGLIST_INFO& rLogListInfo = this->m_vector_log_list.at(_tab_index);

		// 2) Log Filter를 reset한다.
		rLogListInfo.plog_filter.reset();
	}
}

void CChild_logger_view_ex::SetHorizontalExtent(int _cx)
{
	this->m_list_log.SetHorizontalExtent(_cx);
}

void CChild_logger_view_ex::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _vector_log_record must be not null
	RETURN_IF(_vector_log_record.empty(),);

	scoped_lock(this->m_lockable_queue_string)
	{
		// 1) insert LOG_RECORD
		for (auto& iter : _vector_log_record)
		{
			// check) 
			CONTINUE_IF(iter->buf_message.empty());

			// - queuing 
			this->m_pqueue_log_record_queuing.push_back(iter);
		}

		// check) queuing된 것이 있을 때면 PostMessage한다.
		RETURN_IF(this->m_pqueue_log_record_queuing.size() > 1);
	}

	// 7) PostMessage
	PostMessage(WM_ADD_STRING, 0, 0);
}


}
