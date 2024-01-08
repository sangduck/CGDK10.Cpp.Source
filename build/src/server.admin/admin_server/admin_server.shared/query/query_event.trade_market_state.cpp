#include "pch.h"


query_event::market_state::market_state() noexcept
{
}

query_event::market_state::~market_state() noexcept
{
}

void query_event::market_state::process_execute_query()
{
	//// 1) 
	//auto		iter	 = m_vectorEventSever.begin();
	//auto		iterPost = iter;
	//auto		iterEnd	 = m_vectorEventSever.end();
	//auto		time_now	 = chrono::time::utc::now();

	//try
	//{
	//	while (iter != iterEnd)
	//	{
	//		// declare) 
	//		shared_buffer		buf_query	 = alloc_shared_buffer(1024 + 512 * (MAX_QUERY_COUNT*4+8));

	//		// 1) Begin Tran
	//	#if defined(_DB_MSSQL)
	//		buf_query.append_text<char>("BEGIN TRANSACTION;");
	//	#elif defined(_DB_MYSQL)
	//		buf_query.append_text<char>("START TRANSACTION;");
	//	#endif

	//		// 2) 쿼리문의 기본문을 추가
	//		buf_query.append_text<char>("INSERT INTO EventTrade (teid,oid,eid,asid,eType,aid,S_oid,T_oid,Pr_p,Tr_p,Pr_a,Tr_a,Tr_am,St,eOType,eOCon,timeOccure) VALUES ");

	//		// declare)
	//		int	countInsert = 0;

	//		// - Insert 내용을 작성한다.
	//		for (; iter != iterEnd && countInsert < MAX_QUERY_COUNT; ++iter)
	//		{

	//			for(auto& iterItem: (*iter)->vectorEventProcess)
	//			{
	//				// check) eOPERATION::NOTIFY_ONLY는 DB에 저장하지 않는다. 따랏 Continue~
	//				CONTINUE_IF(iterItem.event_type == eEVENT_TYPE::INFO_SEPERATE);

	//				// check) sTRADE_EVENT를 검사한다.
	//				CHECK_TRADE_EVENT(iterItem);

	//				// declare) 
	//				char		str_eid[64];
	//				const char*	pstr_eid;

	//				// - eid_Target
	//				if (iterItem.eid != 0)
	//				{
	//					__to_string(str_eid, iterItem.eid);
	//					pstr_eid = str_eid;
	//				}
	//				else
	//				{
	//					pstr_eid = "null";
	//				}

	//				// declare) 
	//				char	tempA[64];
	//				char	tempB[64];
	//				char	tempC[64];

	//				// - query를 요청한다.                [teid] [oid][e][a/eT][aid]  [soid][tteid][poid     ][Item  ] [s/ot/oc][time_occure]
	//				buf_query.append_text<char>("(%I64u,%I64u,%s,%u,%u,%I64u,%I64u,%I64u,%I64u,%I64u,%s,%s,%s,%u,%u,%u,'%d-%d-%d %d:%d:%d.%d'),",
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
	//			buf_query.len -= sizeof(char);

	//			// - 마지막에 NULL을 추가한다.
	//			buf_query.append<char>(';');

	//		#if defined(_DB_MSSQL)
	//			buf_query.append_text<char>("COMMIT TRANSACTION;");
	//		#elif defined(_DB_MYSQL)
	//			buf_query.append_text<char>("COMMIT;");
	//		#endif

	//			// - 마지막에 NULL을 추가한다.
	//			buf_query.append<char>(NULL);

	//			// - query를 요청한다.
	//			execute_query(std::string_view(buf_query.buf));

	//			// - query를 닫는다.
	//			close_query();
	//		}

	//		// - 사후 처리
	//		for (; iterPost != iter; ++iterPost)
	//		{
	//			// - Result를 설정한다.
	//			(*iterPost)->eResult = eRESULT::SUCCESS;

	//			// - Trade Complete
	//			m_pmarket->_ProcessTradeComplete(*iterPost);
	//		}
	//	}

	//	// 7) Clear한다.
	//	m_vectorEventSever.clear();
	//}
	//catch (throwable::failure& /*_e*/)
	//{
	//	// check)
	//	CGASSERT_ERROR(false);

	//	// log) 
	//	LOG_ERROR(nullptr, "Fail to query 'INSERT INTO EventTrade'");
	//}
	//catch (...)
	//{
	//	// check)
	//	CGASSERT_ERROR(false);

	//	// log) 
	//	LOG_ERROR(nullptr, "Fail to query 'INSERT INTO EventTrade'");
	//}

	// 7) reset 
	m_check_duplicate = false;
	m_psocket.reset();
}
