//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                               MFC Controls                                *
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
// CCGChildLogView
//
// 1. CCGChildLogView란!
//    1) Socket의 상태정보를 표시하는 Dialog
//    2) DoModal()로 띄우는 것으로 작성하였다.
// 
// 
//-----------------------------------------------------------------------------
class CGMFCCONTROL_EXT_CLASS CListBox_log : public CListBox
{
	DECLARE_DYNAMIC(CListBox_log)

// constructor/destructor)
public:
			CListBox_log();
	virtual ~CListBox_log();


// definitions) 
public:
	struct ITEM_LOG : virtual public Ireferenceable
	{
	public:
				TCHAR			str_head[24];
				const TCHAR*	str_message;
				COLORREF		color_text;
				BOOL			is_bold;
				object_ptr<LOG_RECORD>	plog_record;
	public:
		ITEM_LOG() noexcept : str_head{NULL}, str_message(nullptr), color_text(0), is_bold(FALSE), plog_record() {}
		~ITEM_LOG() noexcept {}
	};

// public) 
public:
			int					AddString		(ITEM_LOG* _pitem_log);
			int					AddString		(LOG_RECORD* _plog_record, COLORREF _color_text, BOOL _is_bold);
			int					AddString		(const chrono::time::utc::time_point& _time_log, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold);
			int					AddString		(std::basic_string_view<TCHAR> _str_head, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold);
			bool				UpdateString	(int _index, const chrono::time::utc::time_point& _time_log, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold);
			bool				UpdateString	(int _index, std::basic_string_view<TCHAR> _str_head, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold);
			void				DeleteItemAfter	(int _index);

			// - 선택한 줄(선택이 없으면 전체)을 "[time] message" 텍스트로 클립보드에 복사한다. (Ctrl+C)
			bool				CopyToClipboard	(bool _selected_only = true);
			// - 전체 선택 (Ctrl+A, 다중 선택 listbox 일 때만)
			void				SelectAll		();

// MFC) Virtual Functions
protected:
	virtual BOOL				PreCreateWindow(CREATESTRUCT& cs);
	virtual void				DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void				DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);
	virtual void				MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

// MFC) Windows Message
protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void				OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg LRESULT				OnCopyMessage(WPARAM _wParam, LPARAM _lParam);	// WM_COPY
};



}