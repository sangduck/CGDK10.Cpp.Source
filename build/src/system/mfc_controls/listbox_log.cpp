#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

#ifdef _UNICODE
	#define	MFC_CHAR_ENCODING	eCHARACTER_SET::UCS2
#else
	#define	MFC_CHAR_ENCODING	eCHARACTER_SET::MBCS
#endif

namespace CGDK
{

CFont	g_fontBold;

IMPLEMENT_DYNAMIC(CListBox_log, CListBox)

CListBox_log::CListBox_log()
{
	if(g_fontBold.m_hObject==NULL)
	{
		LOGFONT	lf;

		lf.lfWidth			 = 0;
		lf.lfEscapement		 = 0;
		lf.lfOrientation	 = 0;
		lf.lfItalic			 = 0;
		lf.lfUnderline		 = 0;
		lf.lfStrikeOut		 = 0;
		lf.lfCharSet		 = 0;
		lf.lfOutPrecision	 = 0;
		lf.lfClipPrecision	 = 0;
		lf.lfQuality		 = 0;
		lf.lfPitchAndFamily	 = 0;

		//lf.lfWeight			 = FW_BOLD;
		//lf.lfHeight			 = 13;
		//STRCPY(lf.lfFaceName, TEXT("Tahoma"));

		lf.lfWeight			 = FW_BOLD;
		lf.lfHeight			 = 15;
		__strcpy(lf.lfFaceName, TEXT("Calibri"));

		//lf.lfWeight			 = FW_BOLD;
		//lf.lfHeight			 = 14;
		//STRCPY(lf.lfFaceName, TEXT("System"));

		g_fontBold.CreateFontIndirect(&lf);
	}
}

CListBox_log::~CListBox_log()
{
}


BEGIN_MESSAGE_MAP(CListBox_log, CListBox)
END_MESSAGE_MAP()


BOOL CListBox_log::PreCreateWindow(CREATESTRUCT& cs)
{
	// 1) HasString과 OwnerdrawVariable을 추가한다.
	cs.style	|= (LBS_OWNERDRAWVARIABLE | LBS_EXTENDEDSEL);

	// 2) CListBox의 원래 PreCreateWindow
	return CListBox::PreCreateWindow(cs);
}

void CListBox_log::DrawItem(LPDRAWITEMSTRUCT lpDIS) 
{
	// check) 
	RETURN_IF(static_cast<int>(lpDIS->itemID) < 0);

	// 1) DC를 얻는다.
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);


	//-----------------------------------------------------------------
	// 1. 배경을 그린다.
	//-----------------------------------------------------------------
	// 1) 선택된 경우 배경을 칠한다.
	if ((lpDIS->itemState & ODS_SELECTED))
	{
		CBrush brush(::GetSysColor(COLOR_MENUBAR));	//  && (lpDIS->itemAction & ODA_DRAWENTIRE)
		pDC->FillRect(&lpDIS->rcItem, &brush);
	}

	// 2) 선택되지 않았을 경우 윈도우 기본 색으로 채운다.
	if (!(lpDIS->itemState & ODS_SELECTED))
	{
		CBrush brush(::GetSysColor(COLOR_WINDOW));
		pDC->FillRect(&lpDIS->rcItem, &brush);
	}	 	


	//-----------------------------------------------------------------
	// 2. Text를 그리기 위한 설정을 준비한다.
	//-----------------------------------------------------------------
	// declare) 
	int			nBkMode	 = 0;	// 복구를 위해...
	COLORREF	crText	 = 0;	// 복구를 위해...

	// 1) ItemData를 얻는다.
	ITEM_LOG*	pitem_log	 = reinterpret_cast<ITEM_LOG*>(lpDIS->itemData);

	// 2) Text의 배경을 투명하도록 한다. (그리고 이전 배경색을 저장한다.)
	nBkMode = pDC->SetBkMode(TRANSPARENT);

	// 3) Text format을 셋업한다.
	UINT nFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	if (GetStyle() & LBS_USETABSTOPS)
	{
		nFormat |= DT_EXPANDTABS;
	}

	// 4) Rect를 임시로 저장한다.
	CRect rect = lpDIS->rcItem;


	//-----------------------------------------------------------------
	// 3. Time Text를 출력한다.
	//-----------------------------------------------------------------
	// 1) Header부분은 Gray로 한다.
	crText = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

	// 2) 최종적으로 그린다.
	pDC->DrawText(pitem_log->str_head, -1, &rect, nFormat);

	// 3) 위치를 이동한다.
	rect.left	+= 120;


	//-----------------------------------------------------------------
	// 4. Log Text를 출력한다.
	//-----------------------------------------------------------------
	// declare) 
	CFont* pfontOld = nullptr;

	// 1) ItemData가 0이 아닐 경우...
	if (lpDIS->itemState & ODS_DISABLED)
		crText = pDC->SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
	else
		crText = pDC->SetTextColor(pitem_log->color_text);

	// 2) Font를 설정한다.
	if(pitem_log->is_bold)
	{
		pfontOld = pDC->SelectObject(&g_fontBold);
	}

	// 3) 최종적으로 그린다.
	if(pitem_log->str_message != nullptr)
	{
		pDC->DrawText(pitem_log->str_message, -1, &rect, nFormat);
	}

	// 4) 색상을 되돌린다.
	pDC->SetTextColor(crText); 
	pDC->SetBkMode(nBkMode);
	if(pfontOld!=nullptr)
	{
		pDC->SelectObject(pfontOld);
	}
}

void CListBox_log::DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
	// 1) ItemData를 얻는다.
	ITEM_LOG* pitem_log = reinterpret_cast<ITEM_LOG*>(lpDeleteItemStruct->itemData);

	if(pitem_log!=nullptr)
	{
		pitem_log->release();

		lpDeleteItemStruct->itemData = NULL;
	}

	// 2) ListBox의 DeleteItem 호출
	CListBox::DeleteItem(lpDeleteItemStruct);
}

void CListBox_log::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	lpMIS->itemHeight = ::GetSystemMetrics(SM_CYMENUCHECK);
}

int CListBox_log::AddString(ITEM_LOG* _pitem_log)
{
	// 1) String을 추가한다.
	int nItem = __super::AddString(TEXT(""));

	// 2) ItemData를 설정한다.
	SetItemData(nItem, (DWORD_PTR)_pitem_log);

	// 3) 
	_pitem_log->add_ref();

	// return) 
	return nItem;
}

int CListBox_log::AddString(LOG_RECORD* _pLogRecord, COLORREF _color_text, BOOL _is_bold)
{
	// 1) ITEM_LOG를 생성한다.
	object_ptr<ITEM_LOG> pitem_log = _make_object<ITEM_LOG>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	// 2) get system time
	auto time_temp = chrono::to_date_time(_pLogRecord->time_occure);

	// 3) setting
	__sprintf(pitem_log->str_head, TEXT("[%4d/%02d/%02d %02d:%02d:%02d]"),
		time_temp.t_year,
		time_temp.t_month,
		time_temp.t_day,
		time_temp.t_hour,
		time_temp.t_minute,
		time_temp.t_second
		);

	pitem_log->color_text = _color_text;
	pitem_log->is_bold = _is_bold;
	pitem_log->plog_record = _pLogRecord;

	// 3) String을 추가한다.
	return AddString(pitem_log);
}


int CListBox_log::AddString(const chrono::time::utc::time_point& _time_log, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold)
{
	// 1) ITEM_LOG를 생성한다.
	object_ptr<ITEM_LOG> pitem_log = _make_object<ITEM_LOG>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	// 2) get system time
	auto time_temp = chrono::to_date_time(_time_log);

	// 3) setting
	__sprintf(pitem_log->str_head, TEXT("[%4d/%02d/%02d %02d:%02d:%02d]"), 
		time_temp.t_year,
		time_temp.t_month,
		time_temp.t_day,
		time_temp.t_hour,
		time_temp.t_minute,
		time_temp.t_second
	);

	// 4) set  to...
	pitem_log->color_text = _color_text;
	pitem_log->is_bold = _is_bold;

	// 5) make log_record
	auto plog_record = _make_object<LOG_RECORD>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	plog_record->type = log_type_t::user;
	plog_record->set_message(_message);
	plog_record->time_occure = _time_log;

	pitem_log->plog_record = plog_record;

	// 6) String을 추가한다.
	return AddString(pitem_log);
}

int CListBox_log::AddString(std::basic_string_view<TCHAR> _str_head, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold)
{
	// 1) ITEM_LOG를 생성한다.
	object_ptr<ITEM_LOG> pitem_log = _make_object<ITEM_LOG>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	// 2) setting
	__strcpy(pitem_log->str_head, _str_head);

	// 3) 새로운 Log Record 객체를 생성한다.
	pitem_log->color_text = _color_text;
	pitem_log->is_bold = _is_bold;

	// 4) make log_record
	auto plog_record = _make_object<LOG_RECORD>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);

	plog_record->type = log_type_t::user;
	plog_record->set_message(_message);
	plog_record->time_occure = chrono::time::utc::now();

	pitem_log->plog_record = plog_record;

	// 5) String을 추가한다.
	return AddString(pitem_log);
}

bool CListBox_log::UpdateString(int _index, const chrono::time::utc::time_point& _time_log, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold)
{
	if(_index<GetCount())
	{
		ITEM_LOG* pitem_log = reinterpret_cast<ITEM_LOG*>(GetItemData(_index));

		// check)
		RETURN_IF(pitem_log == nullptr, false)

		// 1) to system_time
		auto temp_time = chrono::to_date_time(_time_log);

		// 2) setting
		__sprintf(pitem_log->str_head, TEXT("[%4d/%02d/%02d %02d:%02d:%02d]"),
			temp_time.t_year,
			temp_time.t_month,
			temp_time.t_day,
			temp_time.t_hour,
			temp_time.t_minute,
			temp_time.t_second
		);

		// 2) 기타 값들을 설정한다.
		pitem_log->plog_record->set_message(_message, MFC_CHAR_ENCODING);
		pitem_log->color_text = _color_text;
		pitem_log->is_bold	 = _is_bold;
	}
	else
	{
		// - 추가한다.
		AddString(_time_log, _message, _color_text, _is_bold);

		// Scroll한다.
		SetTopIndex(GetTopIndex() + 1);
	}

	return true;
}

bool CListBox_log::UpdateString(int _index, std::basic_string_view<TCHAR> _str_head, std::basic_string_view<TCHAR> _message, COLORREF _color_text, BOOL _is_bold)
{
	if (_index < GetCount())
	{
		ITEM_LOG* pitem_log = reinterpret_cast<ITEM_LOG*>(GetItemData(_index));

		// check)
		RETURN_IF(pitem_log == nullptr, false)

		// 1) setting
		__strcpy(pitem_log->str_head, _str_head);

		// 2) 기타 값들을 설정한다.
		pitem_log->plog_record->set_message(_message, MFC_CHAR_ENCODING);
		pitem_log->color_text = _color_text;
		pitem_log->is_bold	 = _is_bold;
	}
	else
	{
		// - 추가한다.
		AddString(_str_head, _message, _color_text, _is_bold);

		// Scroll한다.
		SetTopIndex(GetTopIndex() + 1);
	}

	return true;
}

void CListBox_log::DeleteItemAfter(int _index)
{
	while(DeleteString(_index) != LB_ERR);
}


}