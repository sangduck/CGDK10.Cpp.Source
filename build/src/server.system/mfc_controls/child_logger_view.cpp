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

IMPLEMENT_DYNAMIC(CChild_logger_view, CChildDialog)

CChild_logger_view::CChild_logger_view(CWnd* pParent /*=NULL*/)
	: CChildDialog(IDD_DIALOG_LOGVIEW, pParent),
	m_pqueue_log_record_queuing(),
	m_pqueue_log_record_process(),
	m_capacity(2048)
{
	// 2) 
	__zero_memory(m_count_log, sizeof(m_count_log));

	// 3) Default Color를 설정한다.
	m_rgb_text[static_cast<int>(log_type_t::info)]		 = RGB(0x00, 0x00, 0x00);
	m_is_bold[static_cast<int>(log_type_t::info)]		 = false;

	m_rgb_text[static_cast<int>(log_type_t::progress)]	 = RGB(0x00, 0x6f, 0x00);
	m_is_bold[static_cast<int>(log_type_t::progress)]	 = false;

	m_rgb_text[static_cast<int>(log_type_t::debug)]		 = RGB(0x7f, 0x7f, 0x7f);
	m_is_bold[static_cast<int>(log_type_t::debug)]		 = false;

	m_rgb_text[static_cast<int>(log_type_t::fatal)]		 = RGB(0xff, 0x00, 0x00);
	m_is_bold[static_cast<int>(log_type_t::fatal)]		 = true;

	m_rgb_text[static_cast<int>(log_type_t::error)]		 = RGB(0xff, 0x40, 0x40);
	m_is_bold[static_cast<int>(log_type_t::error)]		 = false;

	m_rgb_text[static_cast<int>(log_type_t::warning)]	 = RGB(0xcf, 0x40, 0x40);
	m_is_bold[static_cast<int>(log_type_t::warning)]	 = false;

	m_rgb_text[static_cast<int>(log_type_t::user)]		 = RGB(0xaf, 0xaf, 0x00);
	m_is_bold[static_cast<int>(log_type_t::user)]		 = false;

	m_rgb_text[static_cast<int>(log_type_t::system)]	 = RGB(0x00, 0x00, 0xff);
	m_is_bold[static_cast<int>(log_type_t::system)]		 = false;
}

CChild_logger_view::~CChild_logger_view()
{
}

void CChild_logger_view::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOGVIEW, m_list_log);
}

result_code CChild_logger_view::process_message(sMESSAGE& _msg)
{
	if(_msg.message == eMESSAGE::SYSTEM::LOG_)
	{
		on_message_Log((sMESSAGE_LOG&)_msg);
	}

	// return) Log는 무조건 false를 리턴한다.
	return eRESULT::BYPASS;
}

result_code CChild_logger_view::on_message_Log(sMESSAGE_LOG& _msg)
{
	// 1) trace...
	trace(_msg.plog_record);

	// return) 
	return eRESULT::BYPASS;
}

BEGIN_MESSAGE_MAP(CChild_logger_view, CChildDialog)
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_LBN_SELCHANGE(IDC_LIST_LOGVIEW, &CChild_logger_view::OnLbnSelchangeListLog)
	ON_LBN_DBLCLK(IDC_LIST_LOGVIEW, &CChild_logger_view::OnLbnDblclkListLog)
	ON_LBN_ERRSPACE(IDC_LIST_LOGVIEW, &CChild_logger_view::OnLbnErrspaceListLog)
	ON_LBN_SELCANCEL(IDC_LIST_LOGVIEW, &CChild_logger_view::OnLbnSelcancelListLog)
	ON_NOTIFY(NM_THEMECHANGED, IDC_LIST_LOGVIEW, &CChild_logger_view::OnNMThemeChangedListLog)
	ON_NOTIFY(NM_THEMECHANGED, IDC_LIST_LOGVIEW, &CChild_logger_view::OnNMThemeChangedListLog)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_logger_view::OnCGMessageNotify)
	ON_MESSAGE(WM_ADD_STRING, &CChild_logger_view::OnAddString)
END_MESSAGE_MAP()


// CChild_logger_view message handlers


BOOL CChild_logger_view::OnInitDialog()
{
	CChildDialog::OnInitDialog();

	ShowList(0, SW_SHOW);
	m_count_post_message = 0;

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_logger_view::OnSizing(UINT fwSide, LPRECT pRect)
{
	CGDIALOG::OnSizing(fwSide, pRect);

	// 1) Width의 한계를 설정한다.

	// 2) Height의 한계를 설정한다.
}

void CChild_logger_view::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	if(m_list_log.m_hWnd!=NULL)
	{
		// 1) 전체 크기를 먼저 얻는다.
		CRect tempRectOriginal;
		GetClientRect(tempRectOriginal);

		// declare) 
		CRect tempRect;

		// 2) 
		auto pwnd_log = GetDlgItem(IDC_LIST_LOGVIEW);

		// 2) Tab의 위치를 수정한다.
		pwnd_log->GetWindowRect(tempRect);

		ScreenToClient(tempRect);

		tempRect.left	 = 0;
		tempRect.right	 = tempRectOriginal.right;
		//tempRect.top;
		tempRect.bottom	 = tempRectOriginal.bottom;

		pwnd_log->MoveWindow(tempRect);
	}
}

void CChild_logger_view::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);
	
	scoped_lock(m_lockable_log_info)
	{
		// check) 
		m_list_log.EnableWindow(is_enable);
	}

	GetDlgItem(IDC_STATIC_LOG_TOTAL)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_SYSTEM)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_PROGRESS)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_INFO)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_FATAL)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_ERROR)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_LOG_WARNING)->EnableWindow(is_enable);
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
}

void CChild_logger_view::OnDestroy()
{
	circular_list<object_ptr<LOG_RECORD>> pqueue_log_record_queuing;
	circular_list<object_ptr<LOG_RECORD>> pqueue_log_record_process;

	scoped_lock(m_lockable_queue_string)
	{
		pqueue_log_record_queuing = std::move(m_pqueue_log_record_queuing);
		pqueue_log_record_process = std::move(m_pqueue_log_record_process);
	}

	__super::OnDestroy();
}

LRESULT CChild_logger_view::OnAddString(WPARAM /*_wParam*/, LPARAM /*_lParam*/)
{
	// 1) count
	--m_count_post_message;

	// check)
	CGASSERT_ERROR(m_count_post_message >= 0)

	// declare)
	int	count_added[static_cast<size_t>(log_type_t::max_)];

	// 2) clear
	__zero_memory(count_added, sizeof(int) * static_cast<size_t>(log_type_t::max_));

	// 3) add log record to listctrl
	scoped_lock(m_lockable_log_info)
	{
		// Disable) 
		m_list_log.SetRedraw(0);

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
		m_list_log.SetRedraw(1);
	}

	// 4) update log count info
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

	// 5) 전체 Log 갯수를 표시한다.
	SetDlgItemInt(IDC_STATIC_LOG_TOTAL, static_cast<int>(m_count_log_total));
	
	// return) 
	return	0;
}

void CChild_logger_view::ProcessAddString(LOG_RECORD* _plog_record, int (&_count_added)[static_cast<size_t>(log_type_t::max_)])
{
	// check) 
	CGASSERT_ERROR(_plog_record != nullptr);

	// check) 
	RETURN_IF(_plog_record == nullptr);

	// declare) 
	auto pitem = make_object<CListBox_log::ITEM_LOG>();

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
	}
	else
	{
		// - Item..
		pitem->str_head[0] = NULL;
	}

	// 3) Type에 따라 View를 update한다.
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

	// 5) ...
	auto buf_log = pitem->plog_record->buf_message;

	// 6) string을 읽어들인다.
	auto vector_string = buf_log.extract<std::vector<std::basic_string_view<TCHAR>>>();

	// 7) PUSH!!
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
			auto pitem_next = make_object<CListBox_log::ITEM_LOG>();

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

void CChild_logger_view::ProcessAddString(ITEM_LOG&& _item_log)
{
	// lock) 
	{
		// scoped_lock 
		std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

		// - Filter가 있으면 Filtering검사를 수행한다.
		if(m_log_info.plog_filter.exist())
		{
			// - Filtering 검사를 수행한다.
			bool result = m_log_info.plog_filter->process_filtering(_item_log.plog_record);

			// check) ...
			RETURN_IF(result == false);
		}
		
		// - String을 추가한다.
		AddString(&m_list_log, _item_log.pitem);
	}
}

BOOL CChild_logger_view::CreateChild(const void* /*_pInfo*/, CWnd* _pWnd)
{
	// 1) Statistics의 포인터를 저장~
	//m_pstatistics	 = _pInfo;

	// 2) 윈도우를 생성한다.
	BOOL result = Create(IDD_DIALOG_LOGVIEW, _pWnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return	result;
}

void CChild_logger_view::AddString(CListBox_log* _lParam, CListBox_log::ITEM_LOG* _pitem)
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

void CChild_logger_view::clear()
{
	// 1) queue를 비운다.
	scoped_lock(m_lockable_queue_string)
	{
		m_pqueue_log_record_queuing.clear();
	}

	// 2) ListBox를 비운다.
	scoped_lock(m_lockable_log_info)
	{
		m_list_log.ResetContent();
	}
}

void CChild_logger_view::set_log_filter(Ilog_filter* _plog_filter)
{
	// scoped_lock) 
	std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

	// 1) set filter
	m_log_info.plog_filter = _plog_filter;
}

void CChild_logger_view::reset_log_filter()
{
	// scoped_lock) 
	std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

	// 1) reset 
	m_log_info.plog_filter.reset();
}

LRESULT CChild_logger_view::OnCGMessageNotify(WPARAM _wParam, LPARAM /*_lParam*/)
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

void CChild_logger_view::OnLbnSelchangeListLog()
{
}

void CChild_logger_view::OnLbnDblclkListLog()
{
}

void CChild_logger_view::OnLbnErrspaceListLog()
{
}

void CChild_logger_view::OnLbnSelcancelListLog()
{
}

void CChild_logger_view::OnNMThemeChangedListLog(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = 0;
}

void CChild_logger_view::ShowList(int /*_tab*/, int p_iCmdShow)
{
	// scoped_lock) 
	std::lock_guard<lockable<>> cs(this->m_lockable_log_info);

	// 1) show list
	m_list_log.ShowWindow(p_iCmdShow);
}

void CChild_logger_view::SetHorizontalExtent(int _cx)
{
	m_list_log.SetHorizontalExtent(_cx);
}

void CChild_logger_view::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _vector_log_record must be not null
	RETURN_IF(_vector_log_record.empty(),);

	scoped_lock(m_lockable_queue_string)
	{
		// 1) insert LOG_RECORD
		for (auto& iter: _vector_log_record)
		{
			// check) 
			CONTINUE_IF(iter->buf_message.empty());

			// - queuing 
			m_pqueue_log_record_queuing.push_back(iter);
		}
	}

	// count
	auto count_post = ++m_count_post_message;

	// check)
	RETURN_IF(count_post > 32);

	// 7) PostMessage
	PostMessage(WM_ADD_STRING, 0, 0);
}


}
