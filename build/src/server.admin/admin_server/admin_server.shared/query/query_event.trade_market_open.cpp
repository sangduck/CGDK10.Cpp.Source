#include "pch.h"


query_event::makret_open::makret_open() noexcept
{
}

query_event::makret_open::~makret_open() noexcept
{
}

intptr_t query_event::makret_open::process_execute(_In_ intptr_t /*_return*/, _In_ size_t /*_param*/)
{
	//// 1) 
	//auto		iter	 = m_vectorEventSever.begin();
	//auto		iterPost = iter;
	//auto		iterEnd	 = m_vectorEventSever.end();
	auto		timeNow	 = TIME::SYSTEM::now();

	//try
	//{
	//	while (iter != iterEnd)
	//	{
	//		// Declare) 
	//		CCGBuffer		bufquery	 = MEM_POOL_ALLOC(1024 + 512 * (MAX_QUERY_COUNT*4+8));

	//		// 1) Begin Tran
	//	#if defined(_DB_MSSQL)
	//		bufquery.append_text<_CHAR>(_CGTEXT("BEGIN TRANSACTION;"));
	//	#elif defined(_DB_MYSQL)
	//		bufquery.append_text<_CHAR>(_CGTEXT("START TRANSACTION;"));
	//	#endif

	//		// 2) 쿼리문의 기본문을 추가
	//		bufquery.append_text<_CHAR>(_CGTEXT("INSERT INTO EventTrade (teid,oid,eid,asid,type,aid,S_oid,T_oid,Pr_p,Tr_p,Pr_a,Tr_a,Tr_am,St,eOType,eOCon,time_occure) VALUES "));

	//		// Declare)
	//		int	countInsert = 0;

	//		// - Insert 내용을 작성한다.
	//		for (; iter != iterEnd && countInsert < MAX_QUERY_COUNT; ++iter)
	//		{

	//			for(auto& iterItem: (*iter)->vectorEventProcess)
	//			{
	//				// Check) eOPERATION::NOTIFY_ONLY는 DB에 저장하지 않는다. 따랏 Continue~
	//				CONTINUE_IF(iterItem.event_type == eEVENT_TYPE::INFO_SEPERATE);

	//				// Check) TRADE_EVENT를 검사한다.
	//				CHECK_TRADE_EVENT(iterItem);

	//				// Declare) 
	//				_CHAR			str_eid[64];
	//				const _CHAR*	pstr_eid;

	//				// - eid_Target
	//				if (iterItem.eid != 0)
	//				{
	//					__to_string(str_eid, iterItem.eid);
	//					pstr_eid = str_eid;
	//				}
	//				else
	//				{
	//					pstr_eid = _CGTEXT("null");
	//				}

	//				// Declare) 
	//				_CHAR	tempA[64];
	//				_CHAR	tempB[64];
	//				_CHAR	tempC[64];

	//				// - query를 요청한다.                [teid] [oid][e][a/eT][aid]  [soid][tteid][poid     ][Item  ] [s/ot/oc][time_occure]
	//				bufquery.append_text<_CHAR>(_CGTEXT("(%I64u,%I64u,%s,%u,%u,%I64u,%I64u,%I64u,%I64u,%I64u,%s,%s,%s,%u,%u,%u,'%d-%d-%d %d:%d:%d.%d'),"),
	//					(uint64_t)iterItem.teid,
	//					(uint64_t)iterItem.toid.oid,
	//					pstr_eid,
	//					(uint32_t)iterItem.asid,
	//					(uint32_t)iterItem.event_type,
	//					(uint64_t)iterItem.toid.aid.key,
	//					(uint64_t)iterItem.Source_oid,
	//					(uint64_t)iterItem.Target_oid,
	//					(uint64_t)iterItem.itemPrice_poid,
	//					(uint64_t)iterItem.itemTrade_poid,
	//					iterItem.item_price.amount.to_string(tempA, 64),
	//					iterItem.item_trade.amount.to_string(tempB, 64),
	//					iterItem.item_trade_Modify.to_string(tempC, 64),
	//					(uint32_t)iterItem.status,
	//					(uint32_t)iterItem.order_price_type,
	//					(uint32_t)iterItem.order_condition,
	//					iterItem.time_occure.get_year(), iterItem.time_occure.get_month(), iterItem.time_occure.get_day(), iterItem.time_occure.get_hour(), iterItem.time_occure.get_minute(), iterItem.time_occure.get_second(), iterItem.time_occure.get_millisecond()
	//				);
	//			}

	//			// - Insert 수증가
	//			++countInsert;
	//		}

	//		// - 추가된 것이 있어야...
	//		if(countInsert!=0)
	//		{
	//			// - 마지막 ','은 뺀다.
	//			bufquery.len -= sizeof(_CHAR);

	//			// - 마지막에 NULL을 추가한다.
	//			bufquery.append<_CHAR>(';');

	//		#if defined(_DB_MSSQL)
	//			bufquery.append_text<_CHAR>(_CGTEXT("COMMIT TRANSACTION;"));
	//		#elif defined(_DB_MYSQL)
	//			bufquery.append_text<_CHAR>(_CGTEXT("COMMIT;"));
	//		#endif

	//			// - 마지막에 NULL을 추가한다.
	//			bufquery.append<_CHAR>(NULL);

	//			// - query를 요청한다.
	//			auto	hResult = open_query(std::string_view(bufquery.buf));

	//			// Check)
	//			CGASSERT_ERROR(hResult == S_OK);

	//			// Check) query에 실패했으면 끝낸다.
	//			ERROR_THROW_IF(hResult != S_OK, throwable::failure(eOBJECT_RESULT::FAIL), );

	//			// - query를 닫는다.
	//			close_query();
	//		}

	//		// - 사후 처리
	//		for (; iterPost != iter; ++iterPost)
	//		{
	//			// - Result를 설정한다.
	//			(*iterPost)->eResult = eOBJECT_RESULT::SUCCESS;

	//			// - Trade Complete
	//			m_pmarket->_ProcessTradeComplete(*iterPost);
	//		}
	//	}

	//	// 7) Clear한다.
	//	m_vectorEventSever.clear();
	//}
	//catch (throwable::failure& /*_e*/)
	//{
	//	// Check)
	//	CGASSERT_ERROR(false);

	//	// Log) 
	//	LOG_ERROR(nullptr, _CGTEXT("Fail to query 'INSERT INTO EventTrade'"));
	//}
	//catch (...)
	//{
	//	// Check)
	//	CGASSERT_ERROR(false);

	//	// Log) 
	//	LOG_ERROR(nullptr, _CGTEXT("Fail to query 'INSERT INTO EventTrade'"));
	//}

	// Declare)
	SERVER_EVENT_LOG*	event_server_log	 = new SERVER_EVENT_LOG();

	event_server_log->time_occure	 = timeNow;
	event_server_log->eServerType	 = static_cast<eSERVER_TYPE>(m_pSocket->m_info_server_setting.rank.role) ;
	event_server_log->event_type	 = eEVENT_TYPE::MARKET_STATE;
	event_server_log->state			 = eOBJECT_STATE::RUNNING;
	event_server_log->id_server		 = m_pSocket->m_info_server_setting.id;
	event_server_log->addrServer	 = m_pSocket->get_peer_address();

	// Log) 
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_ADD_SERVER_EVENT_LOG, reinterpret_cast<uintptr_t>(event_server_log)));

	// Return)
	return	0;
}
