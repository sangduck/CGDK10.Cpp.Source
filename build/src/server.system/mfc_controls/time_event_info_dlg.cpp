// time_eventInfoDlg.cpp : implementation file
//

#include "pch.h"
#include "afxdialogex.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CTimeEventInfoDlg, CDialogEx)

CTimeEventInfoDlg::CTimeEventInfoDlg(time_event::Ientity* _pEvent)
	: CDialogEx(IDD_DIALOG_EVENT_INFO, nullptr)
{
	SetEventInfo(_pEvent);
}

CTimeEventInfoDlg::~CTimeEventInfoDlg()
{
}

void CTimeEventInfoDlg::SetEventInfo(time_event::Ientity* _pentity)
{
	m_pevent_entity = _pentity;

	if(m_hWnd != NULL)
	{
		UpdateEntityInfo();
	}
}


void CTimeEventInfoDlg::UpdateEntityInfo()
{
	// declare) 
	object_ptr<time_event::Ientity>	pentity;

	scoped_lock(m_pevent_entity)
	{
		pentity = m_pevent_entity;
	}

	// check)
	if(pentity.exist())
	{
		EnableEventContorls(true);

		auto& entity_setting = pentity->entity_setting();
		auto& entity_status = pentity->entity_status();

		SetDlgItemTextA(m_hWnd, IDC_EDIT_NAME, entity_setting.name.c_str());

		SetDlgItemInt(IDC_EDIT_EVENT_LEVEL, entity_setting.level);
		m_comboEventType.SetCurSel((int)entity_setting.type);
		SetDlgItemText(IDC_STATIC_STATE, to_string<TCHAR>(entity_status.state).data());
		m_comboEventSetter.SetCurSel((int)entity_setting.setter);

		UpdateAsEntity(pentity);

		//switch(entity_setting.type)
		//{
		//case	time_event::eTYPE::ONCE:
		//		UpdateAsEvent_Once(dynamic_cast<time_event::object::once*>(pentity.get()));
		//		break;
		//case	time_event::eTYPE::ITERATION:
		//		UpdateAsEvent_Iterration(dynamic_cast<time_event::object::iteration*>(pentity.get()));
		//		break;
		//case	time_event::eTYPE::START_END:
		//		UpdateAsEvent_StartEnd(dynamic_cast<time_event::object::start_end*>(pentity.get()));
		//		break;
		//case	time_event::eTYPE::START_ITERATION_END:
		//		UpdateAsEvent_StartIterrationEnd(dynamic_cast<time_event::object::start_iteration_end*>(pentity.get()));
		//		break;
		//case	time_event::eTYPE::SCHEDULE:
		//		update_as_event_schedule(dynamic_cast<time_event::object::schedule*>(pentity.get()));
		//		break;
		//case	time_event::eTYPE::CUSTOM:
		//		UpdateAsEvent_Custum(pentity);
		//		break;
		//}

		SetDlgItemInt(IDC_STATIC_EVENT_ID, (UINT)entity_status.id);

		if(entity_status.time_setup != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			chrono::time::local::time_point time_temp = entity_status.time_setup;
			m_datatime_SETUP_YMD.SetTime(&(SYSTEMTIME&)time_temp);
			m_datatime_SETUP_MHS.SetTime(&(SYSTEMTIME&)time_temp);
		}
		else
		{
			m_datatime_SETUP_YMD.SetTime((LPSYSTEMTIME)&entity_status.time_setup);
			m_datatime_SETUP_MHS.SetTime((LPSYSTEMTIME)&entity_status.time_setup);
		}

		if(entity_status.time_last_try != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			chrono::time::local::time_point time_temp = entity_status.time_last_try;
			m_datatime_LastTry_YMD.SetTime(&(SYSTEMTIME&)time_temp);
			m_datatime_LastTry_MHS.SetTime(&(SYSTEMTIME&)time_temp);

			m_datatime_LastTry_YMD.EnableWindow(true);
			m_datatime_LastTry_MHS.EnableWindow(true);
		}
		else
		{
			m_datatime_LastTry_YMD.SetTime((LPSYSTEMTIME)&entity_status.time_last_try);
			m_datatime_LastTry_MHS.SetTime((LPSYSTEMTIME)&entity_status.time_last_try);

			m_datatime_LastTry_YMD.EnableWindow(false);
			m_datatime_LastTry_MHS.EnableWindow(false);
		}

		if(entity_status.time_last_succeeded != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			chrono::time::local::time_point time_temp = entity_status.time_last_succeeded;
			m_datatime_LastSucceeded_YMD.SetTime(&(SYSTEMTIME&)time_temp);
			m_datatime_LastSucceeded_MHS.SetTime(&(SYSTEMTIME&)time_temp);

			m_datatime_LastSucceeded_YMD.EnableWindow(true);
			m_datatime_LastSucceeded_MHS.EnableWindow(true);
		}
		else
		{
			m_datatime_LastSucceeded_YMD.SetTime((LPSYSTEMTIME)&entity_status.time_last_succeeded);
			m_datatime_LastSucceeded_MHS.SetTime((LPSYSTEMTIME)&entity_status.time_last_succeeded);

			m_datatime_LastSucceeded_YMD.EnableWindow(false);
			m_datatime_LastSucceeded_MHS.EnableWindow(false);
		}

		if(entity_status.time_last_failed != chrono::time::utc::time_point(chrono::time::duration::zero()))
		{
			chrono::time::local::time_point time_temp = entity_status.time_last_failed;
			m_datatime_LastFailed_YMD.SetTime(&(SYSTEMTIME&)time_temp);
			m_datatime_LastFailed_MHS.SetTime(&(SYSTEMTIME&)time_temp);

			m_datatime_LastFailed_YMD.EnableWindow(false);
			m_datatime_LastFailed_MHS.EnableWindow(false);
		}
		else
		{
			m_datatime_LastFailed_YMD.SetTime((LPSYSTEMTIME)&entity_status.time_last_failed);
			m_datatime_LastFailed_MHS.SetTime((LPSYSTEMTIME)&entity_status.time_last_failed);

			m_datatime_LastFailed_YMD.EnableWindow(false);
			m_datatime_LastFailed_MHS.EnableWindow(false);
		}

		TCHAR temp_string[256] = { NULL, };
		SetDlgItemText(IDC_STATIC_LASTTRY_COUNT,	to_string_scaled_byte(temp_string, 256, (int64_t)entity_status.count_try).data());
		SetDlgItemText(IDC_STATIC_LASTSUCCEDED,		to_string_scaled_byte(temp_string, 256, (int64_t)entity_status.count_succeeded).data());
		SetDlgItemText(IDC_STATIC_EVENT_LASTFAILED,	to_string_scaled_byte(temp_string, 256, (int64_t)entity_status.count_failed).data());

		// Event Log를 표시한다.
		UpdateEntityLog(pentity);
	}
	else
	{
		EnableEventContorls(false);
	}
}

void CTimeEventInfoDlg::UpdateAsEntity(time_event::Ientity* /*_pentity*/)
{
	//// 1) Disable Redraw
	//m_listctrl_event_info.SetRedraw(FALSE);

	//// 2) 먼저 Colume을 모두 지운다.
	//m_listctrl_event_info.DeleteAllItems();
	//while(m_listctrl_event_info.DeleteColumn(0)==TRUE);

	//// 3) Colume을 설정한다.
	//m_listctrl_event_info.InsertColumn(0,  TEXT("Event"),		LVCFMT_LEFT,	120, 0);
	//m_listctrl_event_info.InsertColumn(1,  TEXT("Time"),		LVCFMT_LEFT,	120, 1);
	//m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);

	//// declare) 
	//TCHAR		text[256];
	//LV_ITEM		list_item;

	//list_item.mask			 = LVIF_TEXT;
	//list_item.state			 = 0;
	//list_item.cchTextMax	 = 256;

	//// 4) Iteration Gap
	//{
	//	auto	timeEvent	 = _pentity->get_next_time();

	//	// - 

	//	// - Event
	//	list_item.iSubItem		 = 0;
	//	list_item.pszText		 = TEXT("Next");
	//	m_listctrl_event_info.InsertItem(&list_item);

	//	// - Time
	//	list_item.iSubItem		 = 1;
	//	list_item.pszText		 = text;
	//	wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
	//		timeEvent.get_year<uint32_t>(),
	//		timeEvent.get_month<uint32_t>(),
	//		timeEvent.get_day<uint32_t>(),
	//		timeEvent.get_hour<uint32_t>(),
	//		timeEvent.get_minute<uint32_t>(),
	//		timeEvent.get_second<uint32_t>());

	//	m_listctrl_event_info.SetItem(&list_item);
	//}

	//// 5) enable Redraw
	//m_listctrl_event_info.SetRedraw(TRUE);
}

//void CTimeEventInfoDlg::UpdateAsEvent_Iterration(time_event::object::iteration* _pentity)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while(m_listctrl_event_info.DeleteColumn(0)==TRUE);
//
//	// 3) Colume을 설정한다.
//	m_listctrl_event_info.InsertColumn(0,  TEXT("Event"),		LVCFMT_LEFT,	120, 0);
//	m_listctrl_event_info.InsertColumn(1,  TEXT("Time"),		LVCFMT_LEFT,	120, 1);
//	m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);
//
//	// declare) 
//	TCHAR		text[256];
//	LV_ITEM		list_item;
//
//	list_item.mask			 = LVIF_TEXT;
//	list_item.state			 = 0;
//	list_item.cchTextMax	 = 256;
//
//	// 4) Iteration Gap
//	{
//		auto timeEvent = _pentity->interval();
//
//		// - 
//		list_item.iItem			 = 0;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Iteration Interval");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%d/%d/%d %d:%d:%d"), 
//			timeEvent.get_year(),
//			timeEvent.get_month(),
//			timeEvent.get_day(),
//			timeEvent.get_hour(),
//			timeEvent.get_minute(),
//			timeEvent.get_second());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 5) Iteration Times
//	{
//		int iRemainedTimes = _pentity->times();
//
//		// - 
//		list_item.iItem			 = 1;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Iteration times");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%d"), iRemainedTimes);
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 6) enable Redraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}
//
//void CTimeEventInfoDlg::UpdateAsEvent_StartEnd(time_event::object::start_end* _pentity)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while(m_listctrl_event_info.DeleteColumn(0)==TRUE);
//
//	// 3) Colume을 설정한다.
//	m_listctrl_event_info.InsertColumn(0,  TEXT("Event"),		LVCFMT_LEFT,	120, 0);
//	m_listctrl_event_info.InsertColumn(1,  TEXT("Time"),		LVCFMT_LEFT,	120, 1);
//	m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);
//
//	// declare) 
//	TCHAR		text[256];
//	LV_ITEM		list_item;
//
//	list_item.mask			 = LVIF_TEXT;
//	list_item.state			 = 0;
//	list_item.cchTextMax	 = 256;
//
//	// 4) start Time
//	{
//		auto	timeEvent	 = _pentity->event_start_time();
//
//		// - 
//		list_item.iItem			 = 0;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Start");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
//			timeEvent.get_year<uint32_t>(),
//			timeEvent.get_month<uint32_t>(),
//			timeEvent.get_day<uint32_t>(),
//			timeEvent.get_hour<uint32_t>(),
//			timeEvent.get_minute<uint32_t>(),
//			timeEvent.get_second<uint32_t>());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 5) End Time
//	{
//		auto	timeEvent	 = _pentity->event_end_time();
//
//		// - 
//		list_item.iItem			 = 1;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("End");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
//			timeEvent.get_year<uint32_t>(),
//			timeEvent.get_month<uint32_t>(),
//			timeEvent.get_day<uint32_t>(),
//			timeEvent.get_hour<uint32_t>(),
//			timeEvent.get_minute<uint32_t>(),
//			timeEvent.get_second<uint32_t>());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 6) enable Redraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}
//
//void CTimeEventInfoDlg::UpdateAsEvent_StartIterrationEnd(time_event::object::start_iteration_end* _pentity)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while(m_listctrl_event_info.DeleteColumn(0)==TRUE);
//
//	// 3) Colume을 설정한다.
//	m_listctrl_event_info.InsertColumn(0,  TEXT("Event"),		LVCFMT_LEFT,	120, 0);
//	m_listctrl_event_info.InsertColumn(1,  TEXT("Time"),		LVCFMT_LEFT,	120, 1);
//	m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);
//
//	// declare) 
//	TCHAR		text[256];
//	LV_ITEM		list_item;
//
//	list_item.mask			 = LVIF_TEXT;
//	list_item.state			 = 0;
//	list_item.cchTextMax	 = 256;
//
//	// 4) start Time
//	{
//		auto	timeEvent	 = _pentity->event_start_time();
//
//		// - 
//		list_item.iItem			 = 0;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Start");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
//			timeEvent.get_year<uint32_t>(),
//			timeEvent.get_month<uint32_t>(),
//			timeEvent.get_day<uint32_t>(),
//			timeEvent.get_hour<uint32_t>(),
//			timeEvent.get_minute<uint32_t>(),
//			timeEvent.get_second<uint32_t>());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 5) Iteration Gap
//	{
//		auto	timeEvent	 = _pentity->interval();
//
//		// - 
//		list_item.iItem			 = 1;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Iteration Interval");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%d/%d/%d %d:%d:%d"), 
//			timeEvent.get_year(),
//			timeEvent.get_month(),
//			timeEvent.get_day(),
//			timeEvent.get_hour(),
//			timeEvent.get_minute(),
//			timeEvent.get_second());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 6) Iteration Times
//	{
//		int		iRemainedTimes	 = _pentity->remained_count();
//
//		// - 
//		list_item.iItem			 = 2;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("Iteration times");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%d"), iRemainedTimes);
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//
//	// 7) End Time
//	{
//		auto	timeEvent	 = _pentity->event_end_time();
//
//		// - 
//		list_item.iItem			 = 3;
//
//		// - Event
//		list_item.iSubItem		 = 0;
//		list_item.pszText		 = TEXT("End");
//		m_listctrl_event_info.InsertItem(&list_item);
//
//		// - Time
//		list_item.iSubItem		 = 1;
//		list_item.pszText		 = text;
//		wsprintf(text, TEXT("%u/%u/%u %u:%u:%u"), 
//			timeEvent.get_year<uint32_t>(),
//			timeEvent.get_month<uint32_t>(),
//			timeEvent.get_day<uint32_t>(),
//			timeEvent.get_hour<uint32_t>(),
//			timeEvent.get_minute<uint32_t>(),
//			timeEvent.get_second<uint32_t>());
//		m_listctrl_event_info.SetItem(&list_item);
//	}
//
//	// 8) enable Redraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}
//
//void CTimeEventInfoDlg::update_as_event_schedule(time_event::object::schedule* _pentity)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while (m_listctrl_event_info.DeleteColumn(0) == TRUE);
//
//	// 3) Colume을 설정한다.
//	m_listctrl_event_info.InsertColumn(0, TEXT("Next Time"), LVCFMT_LEFT, 150, 0);
//	m_listctrl_event_info.InsertColumn(1, TEXT("Interval"), LVCFMT_RIGHT, 150, 1);
//	m_listctrl_event_info.InsertColumn(2, TEXT("Remained"), LVCFMT_RIGHT, 100, 2);
//	m_listctrl_event_info.InsertColumn(3, TEXT("Executed"), LVCFMT_RIGHT, 100, 3);
//	m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);
//
//	// 4) 
//	scoped_lock(_pentity->m_queue_event)
//	{
//		// declare) 
//		TCHAR		text[256];
//		LV_ITEM		list_item;
//
//		list_item.mask = LVIF_TEXT;
//		list_item.state = 0;
//		list_item.cchTextMax = 256;
//		list_item.iItem = 0;
//
//		auto	iter	 = _pentity->m_queue_event.begin();
//		auto	iter_end	 = _pentity->m_queue_event.end();
//
//		for (; iter != iter_end; ++iter, ++list_item.iItem)
//		{
//			// - Next Time
//			list_item.iSubItem = 0;
//			to_string(text, (*iter).get_next_time());
//			list_item.pszText = text;
//			m_listctrl_event_info.InsertItem(&list_item);
//
//			// - Interval
//			list_item.iSubItem = 1;
//			to_string(text, (*iter).interval());
//			list_item.pszText = text;
//			m_listctrl_event_info.SetItem(&list_item);
//
//			// - Times
//			list_item.iSubItem = 2;
//			int	interval = (*iter).times();
//			if (interval >= 0)
//			{
//				wsprintf(text, TEXT("%d"), interval);
//				list_item.pszText = text;
//			}
//			else
//			{
//				list_item.pszText = TEXT("Infinite");
//			}
//			m_listctrl_event_info.SetItem(&list_item);
//
//			// - Executed
//			list_item.iSubItem = 3;
//			wsprintf(text, TEXT("%d"), (*iter).executed_count());
//			list_item.pszText = text;
//			m_listctrl_event_info.SetItem(&list_item);
//		}
//	}
//
//	// 5) enable Redraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}
//
//void CTimeEventInfoDlg::update_as_event_schedule_function(time_event::object::schedule_function* _pentity)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while (m_listctrl_event_info.DeleteColumn(0) == TRUE);
//
//	// 3) Colume을 설정한다.
//	m_listctrl_event_info.InsertColumn(0, TEXT("Next Time"), LVCFMT_LEFT, 150, 0);
//	m_listctrl_event_info.InsertColumn(1, TEXT("Interval"), LVCFMT_RIGHT, 150, 1);
//	m_listctrl_event_info.InsertColumn(2, TEXT("Remained"), LVCFMT_RIGHT, 100, 2);
//	m_listctrl_event_info.InsertColumn(3, TEXT("Executed"), LVCFMT_RIGHT, 100, 3);
//	m_listctrl_event_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_UNDERLINEHOT);
//
//	// 4) 
//	scoped_lock(_pentity->m_queue_event)
//	{
//		// declare) 
//		TCHAR		text[256];
//		LV_ITEM		list_item;
//
//		list_item.mask = LVIF_TEXT;
//		list_item.state = 0;
//		list_item.cchTextMax = 256;
//		list_item.iItem = 0;
//
//		auto	iter = _pentity->m_queue_event.begin();
//		auto	iter_end = _pentity->m_queue_event.end();
//
//		for (; iter != iter_end; ++iter, ++list_item.iItem)
//		{
//			// - Next Time
//			list_item.iSubItem = 0;
//			to_string(text, (*iter)->get_next_time());
//			list_item.pszText = text;
//			m_listctrl_event_info.InsertItem(&list_item);
//
//			// - Interval
//			list_item.iSubItem = 1;
//			to_string(text, (*iter)->interval());
//			list_item.pszText = text;
//			m_listctrl_event_info.SetItem(&list_item);
//
//			// - Times
//			list_item.iSubItem = 2;
//			int	interval = (*iter)->times();
//			if (interval >= 0)
//			{
//				wsprintf(text, TEXT("%d"), interval);
//				list_item.pszText = text;
//			}
//			else
//			{
//				list_item.pszText = TEXT("Infinite");
//			}
//			m_listctrl_event_info.SetItem(&list_item);
//
//			// - Executed
//			list_item.iSubItem = 3;
//			wsprintf(text, TEXT("%d"), (*iter)->executed_count());
//			list_item.pszText = text;
//			m_listctrl_event_info.SetItem(&list_item);
//		}
//	}
//
//	// 5) enable Redraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}
//
//void CTimeEventInfoDlg::UpdateAsEvent_Custum(time_event::Ientity* /*_pentity*/)
//{
//	// 1) Disable Redraw
//	m_listctrl_event_info.SetRedraw(FALSE);
//
//	// 2) 먼저 Colume을 모두 지운다.
//	m_listctrl_event_info.DeleteAllItems();
//	while(m_listctrl_event_info.DeleteColumn(0)==TRUE);
//
//	// 3) ...
//
//
//	// 4) enable Rdraw
//	m_listctrl_event_info.SetRedraw(TRUE);
//}


void CTimeEventInfoDlg::EnableEventContorls(bool _bEnable)
{
	// 1) Name
	GetDlgItem(IDC_STATIC_TAG_NAME)->EnableWindow(_bEnable);
	GetDlgItem(IDC_EDIT_NAME)->EnableWindow(_bEnable);

	// 2) setting
	GetDlgItem(IDC_STATIC_FRAME_SETTING)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_TAG_TYPE)->EnableWindow(_bEnable);
	m_comboEventType.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_TAG_LEVEL)->EnableWindow(_bEnable);
	GetDlgItem(IDC_EDIT_EVENT_LEVEL)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_TAG_SETTER)->EnableWindow(_bEnable);
	m_comboEventSetter.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_TAG_STATE)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_STATE)->EnableWindow(_bEnable);
	//GetDlgItem(IDC_STATIC_STATE)->EnableWindow(_bEnable);

	// 3) 
	m_listctrl_event_info.EnableWindow(_bEnable);
	m_listctrl_event_log.EnableWindow(_bEnable);

	// 4) Event sID
	GetDlgItem(IDC_STATIC_TAG_ID)->EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_EVENT_ID)->EnableWindow(_bEnable);

	// 5) ...
	GetDlgItem(IDC_STATIC_TAG_SETUP)->EnableWindow(_bEnable);
	m_datatime_SETUP_YMD.EnableWindow(_bEnable);
	m_datatime_SETUP_MHS.EnableWindow(_bEnable);

	GetDlgItem(IDC_STATIC_TAG_LAST_TRY)->EnableWindow(_bEnable);
	m_datatime_LastTry_YMD.EnableWindow(_bEnable);
	m_datatime_LastTry_MHS.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_LASTTRY_COUNT)->EnableWindow(_bEnable);

	GetDlgItem(IDC_STATIC_TAG_LAST_SUCCEDED)->EnableWindow(_bEnable);
	m_datatime_LastSucceeded_YMD.EnableWindow(_bEnable);
	m_datatime_LastSucceeded_MHS.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_LASTSUCCEDED)->EnableWindow(_bEnable);

	GetDlgItem(IDC_STATIC_TAG_LAST_FAILED)->EnableWindow(_bEnable);
	m_datatime_LastFailed_YMD.EnableWindow(_bEnable);
	m_datatime_LastFailed_MHS.EnableWindow(_bEnable);
	GetDlgItem(IDC_STATIC_EVENT_LASTFAILED)->EnableWindow(_bEnable);
}


void CTimeEventInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_EVENT_TYPE, m_comboEventType);
	DDX_Control(pDX, IDC_COMBO_EVENT_SETTER, m_comboEventSetter);
	DDX_Control(pDX, IDC_LIST_EVENT_SETTINGS, m_listctrl_event_info);
	DDX_Control(pDX, IDC_LIST_EVENT_LOG, m_listctrl_event_log);

	DDX_Control(pDX, IDC_DATETIMEPICKER_SETUP_YMD, m_datatime_SETUP_YMD);
	DDX_Control(pDX, IDC_DATETIMEPICKER_SETUP_HMS, m_datatime_SETUP_MHS);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTTRY_YMD, m_datatime_LastTry_YMD);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTTRY_HMS, m_datatime_LastTry_MHS);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTSUCCEDED_YMD, m_datatime_LastSucceeded_YMD);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTSUCCEDED_HMS, m_datatime_LastSucceeded_MHS);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTFAILED_YMD, m_datatime_LastFailed_YMD);
	DDX_Control(pDX, IDC_DATETIMEPICKER_LASTFAILED_HMS, m_datatime_LastFailed_MHS);
}

BEGIN_MESSAGE_MAP(CTimeEventInfoDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CTimeEventInfoDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CTimeEventInfoDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CTimeEventInfoDlg message handlers
void CTimeEventInfoDlg::OnBnClickedOk()
{
	// 1) Timer를 제거한다.
	KillTimer(0);
	
	CDialogEx::OnOK();
}

void CTimeEventInfoDlg::OnBnClickedCancel()
{
	// 1) Timer를 제거한다.
	KillTimer(0);

	CDialogEx::OnCancel();
}

BOOL CTimeEventInfoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 1) Combo Event Type
	m_comboEventType.AddString(TEXT("EVENT_TYPE_ONCE"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_ITERATION"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_START_END"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_START_ITERATION_END"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_SCHEDULE"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_SCHEDULE_FUNCTION"));
	m_comboEventType.AddString(TEXT("EVENT_TYPE_CUSTOM"));

	// 2) 
	m_comboEventSetter.AddString(TEXT("EVENT_SETTER_LOCAL"));
	m_comboEventSetter.AddString(TEXT("EVENT_SETTER_ADMIN"));
	m_comboEventSetter.AddString(TEXT("EVENT_SETTER_MAX"));
		
	// 3) 
	UpdateEntityInfo();

	// 4) Timer를 설정한다.
	SetTimer(0, 1000, 0);

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CTimeEventInfoDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) 상태를 Update한다.
		UpdateEntityInfo();
	}

	// 3) ...
	CDialogEx::OnTimer(nIDEvent);
}

void CTimeEventInfoDlg::UpdateEntityLog(time_event::Ientity* _pentity)
{
	// check)
	RETURN_IF(_pentity == nullptr);

	m_listctrl_event_log.SetRedraw(FALSE);

	auto& list_event_result_log = _pentity->entity_result_log();

#ifdef _UNICODE
	#if defined(_USE_CHAR_STRING)
		TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	#endif
#else
	#ifndef _USE_CHAR_STRING
		TCHAR* text = reinterpret_cast<TCHAR*>(malloc(MAX_STRING_LENGTH * sizeof(TCHAR)));
	#endif
#endif

	scoped_lock(list_event_result_log)
	{
		int iIndex = 0;

		for(auto& iter: list_event_result_log->sub_plog)
		{
		#ifdef _UNICODE
			#if defined(_USE_CHAR_STRING)
				encode_string(text, iter->buf_message.data<const char>(sizeof(COUNT_T)), MAX_STRING_LENGTH);
				m_listctrl_event_log.UpdateString(iIndex++, iter->time_occure, text, RGB(0,0,0), true);
			#elif defined(_USE_WCHAR_STRING)
				m_listctrl_event_log.UpdateString(iIndex++, iter->time_occure, iter->buf_message.data_ + sizeof(COUNT_T), RGB(0,0,0), true);
			#else
				#error
			#endif
		#else
			#if defined(_USE_CHAR_STRING)
				m_listctrl_event_log.UpdateString(iIndex++, iter->time_occure, iter->buf_message.data_ + sizeof(COUNT_T), RGB(0,0,0), true);
			#elif defined(_USE_WCHAR_STRING)
				encode_string(text, iter->buf_message.data<const wchar_t>(sizeof(COUNT_T)), MAX_STRING_LENGTH);
				m_listctrl_event_log.UpdateString(iIndex++, iter->time_occure, text, RGB(0,0,0), true);
			#else
				#error
			#endif
		#endif

			for(auto& iterMessage:iter->sub_plog)
			{
				COLORREF	color_text;
				bool		is_bold	 = iterMessage->level>=log_level_t::higher();

				switch(iterMessage->type)
				{
				case	log_type_t::info:
						color_text	 = RGB(0x40, 0x40, 0x40);
						break;

				case	log_type_t::progress:
						color_text	 = RGB(0x00, 0x6f, 0x00);
						break;

				case	log_type_t::debug:
						color_text	 = RGB(0x7f, 0x7f, 0x7f);
						break;

				case	log_type_t::fatal:
						color_text	 = RGB(0x9f, 0, 0);
						is_bold		 = true;
						break;

				case	log_type_t::error:
						color_text	 = RGB(0xff, 0x00, 0x00);
						break;

				case	log_type_t::user:
						color_text	 = RGB(0x40, 0x40, 0x40);
						break;

				case	log_type_t::system:
						color_text	 = RGB(0x00, 0x00, 0xff);
						break;

				default:
						color_text	 = RGB(0x40, 0x40, 0x40);
						break;
				}

		#ifdef _UNICODE
			#if defined(_USE_CHAR_STRING)
				encode_string(text, (const char*)iter->buf_message.data() + sizeof(COUNT_T), MAX_STRING_LENGTH);
				m_listctrl_event_log.UpdateString(iIndex++, std::basic_string_view<TCHAR>(TEXT("")), text, color_text, is_bold);
			#elif defined(_USE_WCHAR_STRING)
				m_listctrl_event_log.UpdateString(iIndex++, std::basic_string_view<TCHAR>(TEXT("")), iterMessage->buf_message.data_ + sizeof(COUNT_T), color_text, is_bold);
			#else
				#error
			#endif
		#else
			#if defined(_USE_CHAR_STRING)
				m_listctrl_event_log.UpdateString(iIndex++, std::basic_string_view<TCHAR>(TEXT("")), iterMessage->buf_message.data_ + sizeof(COUNT_T), color_text, is_bold);
			#elif defined(_USE_WCHAR_STRING)
				encode_string(text, iter->(const wchar_t*)iter->buf_message.data_ + sizeof(COUNT_T), MAX_STRING_LENGTH);
				m_listctrl_event_log.UpdateString(iIndex++, std::basic_string_view<TCHAR>(TEXT("")), text, color_text, is_bold);
			#else
				#error
			#endif
		#endif
			}
		}

		m_listctrl_event_log.DeleteItemAfter(iIndex);
	}

	m_listctrl_event_log.SetRedraw(TRUE);

	::free(text);
}


}