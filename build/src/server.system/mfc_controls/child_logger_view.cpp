#include "pch.h"
#include <cstring>
#include "log_filter_dlg.h"

#pragma warning(disable:4407)

#define	WM_ADD_STRING	(CGDK::eMESSAGE::WINDOWS::NOTIFY+10)
#define	TIMER_ID_FLUSH_LOG	0x4C4F47	// 'LOG' (CChildDialog 가 쓰는 timer id 0 과 겹치지 않게)

namespace CGDK
{

#ifdef _UNICODE
	#define	MFC_CHARACTER_SET	eCHARACTER_SET::UCS2
#else
	#define	MFC_CHARACTER_SET	eCHARACTER_SET::MBCS
#endif

// - UI 유입 제어 설정 (프로세스 공용. 바이너리 호환을 위해 인스턴스 멤버 대신 파일 정적으로 둔다.)
namespace
{
	std::atomic<uint32_t>	s_flush_interval_ms{ 50 };			// flush 타이머 주기(ms)
	std::atomic<size_t>		s_queue_capacity{ 10000 };			// 대기 큐 상한 (넘치면 오래된 레코드를 버린다)
	std::atomic<size_t>		s_max_records_per_flush{ 1000 };	// 타이머 한 번에 리스트박스로 옮기는 최대 레코드 수
	std::atomic<size_t>		s_count_dropped{ 0 };				// 버린 레코드 누계
	std::atomic<size_t>		s_count_dropped_reported{ 0 };		// 리스트박스에 알린 누계
}

void CChild_logger_view::flush_interval(uint32_t _ms) noexcept { s_flush_interval_ms = (_ms < 10) ? 10 : _ms; }
uint32_t CChild_logger_view::flush_interval() noexcept { return s_flush_interval_ms; }
void CChild_logger_view::queue_capacity(size_t _count) noexcept { s_queue_capacity = (_count < 16) ? 16 : _count; }
size_t CChild_logger_view::queue_capacity() noexcept { return s_queue_capacity; }
void CChild_logger_view::max_records_per_flush(size_t _count) noexcept { s_max_records_per_flush = (_count < 1) ? 1 : _count; }
size_t CChild_logger_view::max_records_per_flush() noexcept { return s_max_records_per_flush; }
size_t CChild_logger_view::dropped_count() noexcept { return s_count_dropped; }

IMPLEMENT_DYNAMIC(CChild_logger_view, CChildDialog)

CChild_logger_view::CChild_logger_view(CWnd* pParent /*=NULL*/)
	: CChildDialog(IDD_DIALOG_LOGVIEW, pParent),
	m_queue_log_record_queuing(),
	m_queue_log_record_process(),
	m_capacity(2048)
{
	// 2) 
	std::memset(this->m_count_log, 0, sizeof(m_count_log));

	// 3) Default Color를 설정한다.
	this->m_rgb_text[static_cast<int>(log_type_t::info)]	 = RGB(0x00, 0x00, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::info)]		 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::progress)] = RGB(0x00, 0x6f, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::progress)]	 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::debug)]	 = RGB(0x7f, 0x7f, 0x7f);
	this->m_is_bold[static_cast<int>(log_type_t::debug)]	 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::fatal)]	 = RGB(0xff, 0x00, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::fatal)]	 = true;

	this->m_rgb_text[static_cast<int>(log_type_t::error)]	 = RGB(0xff, 0x40, 0x40);
	this->m_is_bold[static_cast<int>(log_type_t::error)]	 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::warning)]	 = RGB(0xcf, 0x40, 0x40);
	this->m_is_bold[static_cast<int>(log_type_t::warning)]	 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::user)]	 = RGB(0xaf, 0xaf, 0x00);
	this->m_is_bold[static_cast<int>(log_type_t::user)]		 = false;

	this->m_rgb_text[static_cast<int>(log_type_t::system)]	 = RGB(0x00, 0x00, 0xff);
	this->m_is_bold[static_cast<int>(log_type_t::system)]	 = false;
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
		this->on_message_Log((sMESSAGE_LOG&)_msg);
	}

	// return) Log는 무조건 false를 리턴한다.
	return eRESULT::BYPASS;
}

result_code CChild_logger_view::on_message_Log(sMESSAGE_LOG& _msg)
{
	// 1) trace...
	this->trace(object_ptr<LOG_RECORD>(_msg.plog_record));

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
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CChild_logger_view::OnInitDialog()
{
	this->CChildDialog::OnInitDialog();

	this->ShowList(0, SW_SHOW);

	// 1) flush 타이머를 건다. (큐에 쌓인 로그는 이 타이머에서만 리스트박스로 옮긴다.)
	const uint32_t interval_ms = s_flush_interval_ms.load();
	this->SetTimer(TIMER_ID_FLUSH_LOG, interval_ms, nullptr);
	this->m_count_post_message = static_cast<int>(interval_ms);

	// return)
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_logger_view::OnTimer(UINT_PTR _id_event)
{
	// check) flush 타이머가 아니면 기본 처리
	if (_id_event != TIMER_ID_FLUSH_LOG)
	{
		__super::OnTimer(_id_event);
		return;
	}

	// 1) 주기 설정이 바뀌었으면 타이머를 다시 건다.
	const uint32_t interval_ms = s_flush_interval_ms.load();
	if (static_cast<uint32_t>(this->m_count_post_message.load()) != interval_ms)
	{
		this->SetTimer(TIMER_ID_FLUSH_LOG, interval_ms, nullptr);
		this->m_count_post_message = static_cast<int>(interval_ms);
	}

	// 2) 큐 → 리스트박스
	this->ProcessFlush();
}

void CChild_logger_view::OnSizing(UINT fwSide, LPRECT pRect)
{
	this->CGDIALOG::OnSizing(fwSide, pRect);

	// 1) Width의 한계를 설정한다.

	// 2) Height의 한계를 설정한다.
}

void CChild_logger_view::OnSize(UINT nType, int cx, int cy)
{
	this->__super::OnSize(nType, cx, cy);

	if(this->m_list_log.m_hWnd != NULL)
	{
		// 1) 전체 크기를 먼저 얻는다.
		CRect tempRectOriginal;
		this->GetClientRect(tempRectOriginal);

		// declare) 
		CRect tempRect;

		// 2) 
		auto pwnd_log = this->GetDlgItem(IDC_LIST_LOGVIEW);

		// 2) Tab의 위치를 수정한다.
		pwnd_log->GetWindowRect(tempRect);

		this->ScreenToClient(tempRect);

		tempRect.left = 0;
		tempRect.right = tempRectOriginal.right;
		//tempRect.top;
		tempRect.bottom = tempRectOriginal.bottom;

		pwnd_log->MoveWindow(tempRect);
	}
}

void CChild_logger_view::OnEnable(BOOL is_enable)
{
	this->CChildDialog::OnEnable(is_enable);
	
	SCOPED_LOCK(this->m_lockable_log_info)
	{
		// check) 
		this->m_list_log.EnableWindow(is_enable);
	}

	this->GetDlgItem(IDC_STATIC_LOG_TOTAL)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_SYSTEM)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_PROGRESS)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_INFO)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_FATAL)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_ERROR)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_WARNING)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_DEBUG)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_LOG_ETC)->EnableWindow(is_enable);

	this->GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T4)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T5)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T6)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T7)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T8)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T9)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T10)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T11)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T13)->EnableWindow(is_enable);
	this->GetDlgItem(IDC_STATIC_T14)->EnableWindow(is_enable);
}

void CChild_logger_view::OnDestroy()
{
	// 1) flush 타이머를 끈다.
	this->KillTimer(TIMER_ID_FLUSH_LOG);

	circular_list<object_ptr<LOG_RECORD>> queue_log_record_queuing;
	circular_list<object_ptr<LOG_RECORD>> queue_log_record_process;

	SCOPED_LOCK(this->m_lockable_queue_string)
	{
		queue_log_record_queuing = std::move(this->m_queue_log_record_queuing);
		queue_log_record_process = std::move(this->m_queue_log_record_process);
	}

	queue_log_record_queuing.clear();
	queue_log_record_process.clear();

	__super::OnDestroy();
}

LRESULT CChild_logger_view::OnAddString(WPARAM /*_wParam*/, LPARAM /*_lParam*/)
{
	// - (호환용) 예전처럼 WM_ADD_STRING 을 받으면 바로 한 번 flush 한다.
	this->ProcessFlush();

	// return)
	return	0;
}

void CChild_logger_view::ProcessFlush()
{
	// declare)
	int	count_added[static_cast<size_t>(log_type_t::max_)];

	// 1) clear
	std::memset(count_added, 0, sizeof(int) * static_cast<size_t>(log_type_t::max_));

	// 2) add log record to listctrl
	SCOPED_LOCK(this->m_lockable_log_info)
	{
		// 1) 대기 큐에서 이번에 처리할 만큼만 가져온다.
		//    (밀린 것을 한 번에 다 처리하면 그동안 UI 가 멈추므로 한 번에 옮기는 수에 상한을 둔다. 나머지는 다음 타이머에서.)
		const size_t count_max = s_max_records_per_flush.load();

		SCOPED_LOCK(this->m_lockable_queue_string)
		{
			// check) Process Queue는 비어있어야만 한다.
			CGASSERT_ERROR(this->m_queue_log_record_process.empty());

			if (this->m_queue_log_record_queuing.size() <= count_max)
			{
				// - 전부 가져온다. (swap)
				this->m_queue_log_record_process.swap(m_queue_log_record_queuing);
			}
			else
			{
				// - 앞에서 count_max 개만 가져온다.
				for (size_t i = 0; i < count_max; ++i)
				{
					this->m_queue_log_record_process.push_back(std::move(this->m_queue_log_record_queuing.front()));
					this->m_queue_log_record_queuing.pop_front();
				}
			}
		}

		// 2) 큐가 넘쳐 버린 레코드가 있으면 한 줄로 알린다.
		const size_t count_dropped = s_count_dropped.load();
		const size_t count_dropped_reported = s_count_dropped_reported.exchange(count_dropped);
		const bool has_dropped = (count_dropped != count_dropped_reported);

		// check) 처리할 것이 없으면 그냥 돌아간다. (타이머는 주기적으로 오므로 대부분 여기서 끝난다.)
		if (this->m_queue_log_record_process.empty() && has_dropped == false)
			return;

		// Disable)
		this->m_list_log.SetRedraw(0);

		// - 추가 전 상태 (배치가 끝난 뒤 한 번만 스크롤/삭제하기 위해)
		const int top_before = (std::max)(this->m_list_log.GetTopIndex(), 0);
		const int count_before = (std::max)(this->m_list_log.GetCount(), 0);

		// 3) 버린 레코드 안내
		if (has_dropped)
		{
			TCHAR text[128];
			_stprintf_s(text, std::size(text), TEXT("... %llu log lines dropped (ui queue full, total %llu)"), static_cast<unsigned long long>(count_dropped - count_dropped_reported), static_cast<unsigned long long>(count_dropped));
			this->m_list_log.AddString(TEXT(""), text, m_rgb_text[static_cast<int>(log_type_t::warning)], TRUE);
			++count_added[static_cast<int>(log_type_t::warning)];
		}

		// 4) 레코드를 리스트박스에 추가한다. (줄마다 스크롤/삭제는 하지 않는다.)
		while(!this->m_queue_log_record_process.empty())
		{
			// 1) 제일 앞 MSG처리
			ProcessAddString(this->m_queue_log_record_process.front(), count_added);

			// 2) 제일 앞 제거
			this->m_queue_log_record_process.pop_front();
		}

		// 5) 최대한계치를 넘은 만큼 앞에서 한 번에 지운다. (배치당 1회)
		const int count_now = this->m_list_log.GetCount();
		int count_delete = count_now - static_cast<int>(this->m_capacity);
		if (count_delete < 0)
			count_delete = 0;
		for (int i = 0; i < count_delete; ++i)
			this->m_list_log.DeleteString(0);

		// 6) 추가된 줄 수만큼 한 번만 스크롤한다. (꼬리를 따라간다. 배치당 1회)
		const int count_final = this->m_list_log.GetCount();
		int top_new = top_before + (count_now - count_before) - count_delete;
		if (top_new > count_final - 1)
			top_new = count_final - 1;
		if (top_new < 0)
			top_new = 0;
		this->m_list_log.SetTopIndex(top_new);

		// Enable)
		this->m_list_log.SetRedraw(1);
	}

	// 4) update log count info
	for(int i= 0; i<static_cast<int>(log_type_t::max_); ++i)
	{
		// - get added log count at i
		int	count = count_added[i];

		// check) continue if 0
		CONTINUE_IF(count == 0);

		// - add log count
		this->m_count_log[i] += count;
		this->m_count_log_total += count;

		// - 화면 갱신
		this->SetDlgItemInt(IDC_STATIC_LOG_INFO + i, m_count_log[i]);
	}

	// 5) 전체 Log 개수를 표시한다.
	this->SetDlgItemInt(IDC_STATIC_LOG_TOTAL, static_cast<int>(m_count_log_total));
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
		auto temp_dtf = chrono::to_date_time(_plog_record->time_occurred);

		// - make string
		std::swprintf(pitem->str_head, std::size(pitem->str_head), std::wstring_view(TEXT("[%4u-%02u-%02u %02u:%02u:%02u]")).data(), temp_dtf.t_year, temp_dtf.t_month, temp_dtf.t_day, temp_dtf.t_hour, temp_dtf.t_minute, temp_dtf.t_second);
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
	SCOPED_LOCK(this->m_lockable_log_info)
	{
		// - 
		auto iter = vector_string.begin();
		auto iter_end = vector_string.end();

		// - string을 설정한다.
		pitem->str_message = iter->data();

		// - 첫 번째 줄을 추가한다.
		this->ProcessAddString(ITEM_LOG(pitem->plog_record, pitem));
		++_count_added[static_cast<int>(log_type)];

		// - 두 번째 줄 이후를 추가한다.
		for (++iter; iter!=iter_end; ++iter)
		{
			// declare) 
			auto pitem_next = make_object<CListBox_log::ITEM_LOG>();

			// - ...
			pitem_next->str_head[0] = NULL;
			pitem_next->str_message = iter->data();
			pitem_next->plog_record = pitem->plog_record;
			pitem_next->color_text = pitem->color_text;
			pitem_next->is_bold = pitem->is_bold;

			// - 두 번째 이후 줄을 추가한다.
			this->ProcessAddString(ITEM_LOG(pitem->plog_record, pitem_next));
		}
	}
}

void CChild_logger_view::ProcessAddString(ITEM_LOG&& _item_log)
{
	// note) Filter 검사는 process_log(로깅 스레드)에서 큐에 넣기 전에 끝낸다. (걸러진 레코드는 여기까지 오지 않는다.)

	// lock)
	SCOPED_LOCK(this->m_lockable_log_info)
	{
		// - String을 추가한다.
		this->AddString(&m_list_log, _item_log.pitem);
	}
}

BOOL CChild_logger_view::CreateChild(const void* /*_pInfo*/, CWnd* _pWnd)
{
	// 1) Statistics의 포인터를 저장~
	//m_pstatistics	 = _pInfo;

	// 2) 윈도우를 생성한다.
	BOOL result = this->Create(IDD_DIALOG_LOGVIEW, _pWnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	this->ShowWindow(SW_SHOW);

	// return) 성공!!!
	return	result;
}

void CChild_logger_view::AddString(CListBox_log* _lParam, CListBox_log::ITEM_LOG* _pitem)
{
	// 1) Log에 String을 추가한다.
	//    (최대한계치 초과분 삭제와 스크롤은 줄마다 하지 않고 ProcessFlush 에서 배치당 한 번만 한다.)
	_lParam->AddString(_pitem);
}

void CChild_logger_view::clear()
{
	// 1) queue를 비운다.
	SCOPED_LOCK(this->m_lockable_queue_string)
	{
		this->m_queue_log_record_queuing.clear();
	}

	// 2) ListBox를 비운다.
	SCOPED_LOCK(m_lockable_log_info)
	{
		this->m_list_log.ResetContent();
	}
}

void CChild_logger_view::set_log_filter(Ilog_filter* _plog_filter)
{
	// lock) (filter 는 process_log 가 m_lockable_queue_string 아래에서 읽으므로 두 lock 을 모두 잡는다. 순서: log_info → queue_string)
	std::lock_guard lock(this->m_lockable_log_info);
	std::lock_guard lock_queue(this->m_lockable_queue_string);

	// 1) set filter
	this->m_log_info.plog_filter = _plog_filter;
}

void CChild_logger_view::reset_log_filter()
{
	// lock)
	std::lock_guard lock(this->m_lockable_log_info);
	std::lock_guard lock_queue(this->m_lockable_queue_string);

	// 1) reset
	this->m_log_info.plog_filter.reset();
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
	// lock) 
	std::lock_guard lock(this->m_lockable_log_info);

	// 1) show list
	this->m_list_log.ShowWindow(p_iCmdShow);
}

void CChild_logger_view::SetHorizontalExtent(int _cx)
{
	this->m_list_log.SetHorizontalExtent(_cx);
}

void CChild_logger_view::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_plog_record.empty(),);

	// check)
	RETURN_IF(_plog_record->buf_message.empty());

	SCOPED_LOCK(this->m_lockable_queue_string)
	{
		// 1) Filter 검사는 여기(로깅 스레드)에서 큐에 넣기 전에 한다.
		//    (UI 스레드에서 UCS2 변환까지 마친 뒤 거르던 것을 앞당겨, 걸러질 레코드는 큐에 들어가지도 않는다.)
		if (this->m_log_info.plog_filter.exist())
		{
			RETURN_IF(this->m_log_info.plog_filter->process_filtering(_plog_record.get()) == false);
		}

		// 2) 큐 상한을 넘으면 가장 오래된 레코드부터 버린다. (UI 가 밀려도 메모리가 무한히 늘지 않게)
		const size_t capacity = s_queue_capacity.load();

		while (this->m_queue_log_record_queuing.size() >= capacity && this->m_queue_log_record_queuing.empty() == false)
		{
			this->m_queue_log_record_queuing.pop_front();
			++s_count_dropped;
		}

		// 3) queuing
		this->m_queue_log_record_queuing.push_back(std::move(_plog_record));
	}

	// note) 리스트박스로 옮기는 것은 flush 타이머(TIMER_ID_FLUSH_LOG)가 맡는다.
	//       줄마다 PostMessage 하면 posted message 가 끊이지 않아 WM_PAINT/WM_TIMER 가 굶으므로 하지 않는다.
}


}
