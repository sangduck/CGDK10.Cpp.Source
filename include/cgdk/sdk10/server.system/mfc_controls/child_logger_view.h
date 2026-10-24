//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            Server MFC Controls                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// CChild_logger_view
//
// 1. CChild_logger_view란!
//    1) Socket의 상태정보를 표시하는 Dialog
//    2) DoModal()로 띄우는 것으로 작성하였다.
// 
// 
//-----------------------------------------------------------------------------
class CGSERVERMFCCONTROL_EXT_CLASS CChild_logger_view : 
	public						CChildDialog,
	public						Ilogger
{
	DECLARE_DYNAMIC(CChild_logger_view)

public:
			CChild_logger_view(CWnd* pParent = nullptr);
	virtual ~CChild_logger_view();

	struct LOGLIST_INFO
	{
	public:
		object_ptr<Ilog_filter>			plog_filter;

		circular_list<sMESSAGE_LOG>		queue_string_0;
		circular_list<sMESSAGE_LOG>		queue_string_1;
		circular_list<sMESSAGE_LOG>*	pqueue_string_stack;
		circular_list<sMESSAGE_LOG>*	pqueue_string_process;
	public:
		LOGLIST_INFO() noexcept : plog_filter(), queue_string_0{}, queue_string_1{}, pqueue_string_stack(&queue_string_0), pqueue_string_process(&queue_string_1)	{}
	};

protected:
	virtual void				DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL				OnInitDialog();

private:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void				OnEnable(BOOL is_enable);
	afx_msg	void				OnDestroy();
	afx_msg void				OnLbnSelchangeListLog();
	afx_msg void				OnLbnDblclkListLog();
	afx_msg void				OnLbnErrspaceListLog();
	afx_msg void				OnLbnSelcancelListLog();
	afx_msg void				OnNMThemeChangedListLog(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void				OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void				OnSize(UINT nType, int cx, int cy);
	afx_msg	LRESULT				OnCGMessageNotify(WPARAM _wParam, LPARAM _lParam);
	afx_msg	LRESULT				OnAddString(WPARAM _wParam, LPARAM _lParam);
	afx_msg void				OnTimer(UINT_PTR _id_event);

public:
	virtual	result_code			process_message(sMESSAGE& _msg);
			result_code			on_message_Log(sMESSAGE_LOG& _msg);

public:
			BOOL				CreateChild(const void* _pInfo=nullptr, CWnd* _pwnd=nullptr);

			void				AddString(CListBox_log* _lParam, CListBox_log::ITEM_LOG* _pitem);
			void				clear();

			void				set_log_filter(Ilog_filter* _plog_filter);
			void				reset_log_filter();

			void				SetTextColor(log_type_t _log_type, COLORREF _ref_color, bool _bold = false) { m_rgb_text[(int)_log_type] = _ref_color; m_is_bold[(int)_log_type] = _bold;}
			COLORREF			GetTextColor(log_type_t _log_type) { return m_rgb_text[(int)_log_type];}
			COLORREF			GetTextBold(log_type_t _log_type) { return m_is_bold[(int)_log_type];}
																		  
			size_t				capacity() const noexcept { return m_capacity; }
			void				capacity(size_t _capacity) noexcept	{ m_capacity = _capacity; }

			// - UI 유입 제어 (프로세스 공용 설정)
			//   로그는 로깅 스레드에서 큐에만 쌓이고, UI 스레드가 flush_interval 주기의 타이머로 한 번에
			//   max_records_per_flush 개까지만 리스트박스로 옮긴다. (줄마다 PostMessage 하지 않는다.)
			//   큐가 queue_capacity 를 넘으면 가장 오래된 레코드부터 버린다. (dropped_count 로 누계 확인)
			//   note) 이 클래스는 서버 대화상자에 값으로 포함되므로 바이너리 호환을 위해 인스턴스 멤버를 늘리지 않고 정적 설정으로 둔다.
	static	void				flush_interval(uint32_t _ms) noexcept;
	static	uint32_t			flush_interval() noexcept;
	static	void				queue_capacity(size_t _count) noexcept;
	static	size_t				queue_capacity() noexcept;
	static	void				max_records_per_flush(size_t _count) noexcept;
	static	size_t				max_records_per_flush() noexcept;
	static	size_t				dropped_count() noexcept;

			void				SetHorizontalExtent(int _cx);

	virtual	void				process_log(object_ptr<LOG_RECORD>&& _plog_record) override;

protected:
			CListBox_log		m_list_log;

			struct ITEM_LOG
			{
			public:
				object_ptr<LOG_RECORD> plog_record;
				object_ptr<CListBox_log::ITEM_LOG> pitem;

			public:
				ITEM_LOG(LOG_RECORD* _plog_record, CListBox_log::ITEM_LOG* _pitem) : plog_record(_plog_record), pitem(_pitem)	{}
			};

	mutable	lock_object			m_lockable_queue_string;
			circular_list<object_ptr<LOG_RECORD>> m_queue_log_record_queuing;
			circular_list<object_ptr<LOG_RECORD>> m_queue_log_record_process;
			std::atomic<int>	m_count_post_message = 0;	// (현재 flush 타이머에 걸어 둔 주기(ms). 이름은 바이너리 호환을 위해 유지)
			size_t				m_capacity;

	mutable	lock_object			m_lockable_log_info;
			LOGLIST_INFO		m_log_info;

			int					m_count_log[static_cast<int>(log_type_t::max_)];
			COLORREF			m_rgb_text[static_cast<int>(log_type_t::max_)];
			bool				m_is_bold[static_cast<int>(log_type_t::max_)];
			
protected:
			void				ShowList(int _iTab, int p_iCmdShow);
			void				ProcessFlush();
			void				ProcessAddString(LOG_RECORD* _plog_record, int(&_count_added)[static_cast<size_t>(log_type_t::max_)]);
			void				ProcessAddString(ITEM_LOG&& _item_log);

	virtual void				process_dispose() noexcept override	{}
};


}