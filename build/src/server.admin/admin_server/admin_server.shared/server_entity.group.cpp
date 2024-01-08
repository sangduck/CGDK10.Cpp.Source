#include "pch.h"


server_entity::group::group(std::string_view _name) :
	Nnameable(_name)
{
}
server_entity::group::~group()
{
}

void server_entity::group::on_initializing(context& /*_context*/)
{
}
void server_entity::group::on_initialize(context& /*_context*/)
{
}
void server_entity::group::on_destroying()
{
}
void server_entity::group::on_destroy()
{
}

void server_entity::group::on_starting(context& /*_context*/)
{
}
void server_entity::group::on_start(context& /*_context*/)
{
	// 1) 입장을 받기 시작~
	enable_member_enter(true);
}

void server_entity::group::on_stopping()
{
	// 1) 입장을 금지
	disable_member_enter();

	// 2) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](server_entity::socket* _pmember)->bool
	{
		_pmember->closesocket(); 
		return true;
	});

	// 3) Leavel All Memberl
	leave_all();
}
void server_entity::group::on_stop()
{
}

result_code server_entity::group::on_member_entering(server_entity::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) 중복검사를 한다.
	auto psocket = find_member([=](server_entity::socket* _iter) { return _iter->m_info_server_setting.id == _pmember->m_info_server_setting.id; });

	// 2) 이미 존재하면 존재하는 Socket의 접속을 종료한다.
	if (psocket.exist())
	{
		// - 접속 종료 메시지를 먼저 전송한다.
		psocket->send(message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(psocket->m_info_server_setting.hid, psocket->m_info_server_setting.id));

		// - 접속을 종료한다.
		psocket->disconnect();
	}

	// return) 
	return eRESULT::SUCCESS;
}

void server_entity::group::on_member_entered(server_entity::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	//// 1) Register Messageable한다.
	//_pmember->register_messageable(this);

	//// 2) ServerID의 Code를 초기화한다.
	//_pmember->m_info_server_setting.id.code	 = 0;

	//// 3) 추가한다.
	//g_pservice->m_pgroup_server_entity_agent->register_instance(_pmember);

	//// 4) Certify query를 수행한다.
	//g_pservice->m_pquerier_server->request_server_certify(_pmember);
}

uintptr_t server_entity::group::on_member_leaving(server_entity::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	//// 1) unregister Mediator한다.
	//_pmember->unregister_messageable(this);

	//// 2) unregister한다.
	//g_pservice->m_pgroup_server_entity_agent->unregister_instance(_pmember);

	//// 3) Certify query를 수행한다.
	//if(_pmember->m_info_server_setting.id.code!=0)
	//{
	//	g_pservice->m_pquerier_server->request_server_close(_pmember);
	//}

	// return) 
	return 0;
}

void server_entity::group::on_member_leaved(server_entity::socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

result_code server_entity::group::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		//// 1) 부모 Class에 메시지 처리부터 먼저 진행한다.
		//EXECUTE_PARENT_CGMESSAGE_MAP(Igroup, _msg);

		// 2) Message Map
		EXECUTE_CGMESSAGE_MAP(_msg);
	}
	catch (...)
	{
		// - Exception을 처리한다.
	}

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

BEGIN_CGMESSAGE_MAP(server_entity::group)
	// 1) Message from Socket Chatting Server
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<eMESSAGE_TYPE>(4))
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_LIST_REQUEST,					on_message_SERVER_LIST_REQUEST,					sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::ACCOUNT_SERVER_START_CONFIRM_REQUEST,	on_message_ACCOUNT_SERVER_START_CONFIRM_REQUEST,	sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_INSTANCE_UPDATE,					on_message_SERVER_SETTING_UPDATE,				sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_INFO_UPDATE,						on_message_SERVER_INFO_UPDATE,					sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE_TYPE::SERVER_STATE_UPDATE,					on_message_SERVER_STATE_UPDATE,					sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP

	// 2) Message from Socket Game Server
	ON_CGMESSAGE(CGNOTIFY_SERVER_INIT_COMPLETE,								on_message_NOTIFY_SERVER_INIT_COMPLETE,	sMESSAGE_NETWORK);
END_CGMESSAGE_MAP

result_code server_entity::group::on_message_NOTIFY_SERVER_INIT_COMPLETE(sMESSAGE_NETWORK& _msg)
{
	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::socket>();

	// check) psource이 nullptr이면 안됀다.
	CGASSERT_ERROR(psource != nullptr);

	// declare)
	auto& info_server_setting = psource->get_server_setting_info();

	// 2) 전송한다.
	psource->send(message_CERTIFY_SERVER_RESPONSE(eRESULT::SUCCESS, info_server_setting.id));

	// 3) 바로 start한다.
	psource->send(message_SERVER_ADMIN_REQUEST_START_SERVICE(info_server_setting.hid, info_server_setting.id, psource->m_str_setting));

	// return)
	return eRESULT::DONE;
}

result_code server_entity::group::on_message_SERVER_LIST_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//try
	//{
	//	// 2) Session Server에 Trade Server 정보를 전송한다.
	//	psource->send(g_pservice->m_pgroup_server_trade->message_SERVER_LIST_SEND());
	//}
	//catch (...)
	//{
	//	// - 문제...
	//	CGASSERT_ERROR(false);

	//	// - 접속을 종료해 버린다.
	//	psource->closesocket();

	//	// Reraise)
	//	throw;
	//}

	// return)
	return eRESULT::DONE;
}

result_code server_entity::group::on_message_ACCOUNT_SERVER_START_CONFIRM_REQUEST(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//// 2) 메시지를 읽는다.
	//auto msg_recv = _msg.buf_message.front<MESSAGE_ACCOUNT_SERVER_START_CONFIRM_REQUEST>();

	//// declare)
	//eRESULT	eResult = eRESULT::SUCCESS;

	//// 3) 값을 설정한다
	//psource->m_account_server_info = msg_recv.account_server_info;

	//// 4) 중복되는 Account Server가 존재하는지 확인한다.
	//scoped_lock(m_lockable_group)
	//{
	//	for (auto& iter : m_container_member)
	//	{
	//		auto	pMember	 = iter.get_member();

	//		// - 이미 실행중인 Market 중에서...
	//		if (pMember->m_pserver_entity->status_server.state == eOBJECT_STATE::RUNNING || pMember->m_pserver_entity->status_server.state == eOBJECT_STATE::START_PENDING)
	//		{
	//			// check) 같은 setting_id가 이미 존재흐는지 확인한다.
	//			if (pMember->m_account_server_info.setting_uid == msg_recv.account_server_info.setting_uid)
	//			{
	//				eResult	 = eRESULT::ALEADY_EXIST;
	//				break;
	//			}

	//			// check) 같은 asid가 존재하는지 확인한다.
	//			if (pMember->m_account_server_info.asid == msg_recv.account_server_info.asid)
	//			{
	//				eResult	 = eRESULT::ALEADY_EXIST;
	//				break;
	//			}

	//			// check) 같은 DB를 사용하고 있는지 화asid가 존재하는지 확인한다.
	//			if (pMember->m_account_server_info.setting_db_source == msg_recv.account_server_info.setting_db_source)
	//			{
	//				eResult	 = eRESULT::ALEADY_EXIST;
	//				break;
	//			}
	//		}
	//	}

	//	// log) 
	//	if (eResult != eRESULT::SUCCESS)
	//	{
	//		LOG_ERROR << "> ACCOUNT_SERVER['" << msg_recv.account_server_info.name << "':" << msg_recv.account_server_info.asid << "] COMMAND['start'] Failed! (" << to_string<char>(eResult) << static_cast<uint64_t>(eResult);
	//	}

	//	// 5) 맞거나 없으면 등록하고 Accept 한다.
	//	psource->send(MESSAGE_ACCOUNT_SERVER_START_CONFIRM_RESPONSE::MAKE_BUFFER(msg_recv.rid, eResult, msg_recv.server_id));
	//}

	// return)
	return eRESULT::DONE;
}

result_code server_entity::group::on_message_SERVER_SETTING_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//try
	//{
	//	// 2) 
	//	auto buf_recv			 = _msg.buf_message + offset(sizeof(MSG_HEADER));
	//	auto info_server_setting = buf_recv.extract<server_admin::sSERVER_SETTING_INFO>();
	//	auto infoServer			 = buf_recv.extract<shared_buffer>();

	//	// check)
	//	THROW_IF(static_cast<eSERVER_TYPE>(info_server_setting.rank.role) != eSERVER_TYPE::ACCOUNT, throwable::failure(eRESULT::NOT_ALLOWED));

	//	// 3) server_entity info를 저장한다.
	//	scoped_lock(psource->m_lockable_server_setting)
	//	{
	//		if (info_server_setting.id.key != 0)
	//		{
	//			// - idCode 값의 유지를 위해 저장해 놓는다.
	//			info_server_setting.id.code = psource->m_info_server_setting.id.code;

	//			// - 저장한다.
	//			psource->m_info_server_setting = info_server_setting;
	//		}

	//		// - infoServer값을 저장한다.
	//		psource->m_info_server = infoServer;
	//	}

	//	//// 4) Session Server에 Trade Server 정보를 전송한다.
	//	//g_pservice->m_pgroup_server_session->send_active(_msg.buf_message);

	//	// 5) pServer Entity를 얻는다.
	//	auto pserver_entity = psource->m_pserver_entity;

	//	// check)
	//	CGASSERT_ERROR(pserver_entity.exist());

	//	// check)
	//	if (pserver_entity.exist())
	//	{
	//		// - 값을 Update한다.

	//		// notify)
	//		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity.get())));
	//		g_pservice->m_pgroup_client->send(pserver_entity->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	//	}
	//}
	//catch (...)
	//{
	//	// - 문제...
	//	CGASSERT_ERROR(false);

	//	// - 접속을 종료해 버린다.
	//	psource->closesocket();

	//	// Reraise)
	//	throw;
	//}

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::group::on_message_SERVER_INFO_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//try
	//{
	//	// 2) 
	//	auto buf_recv    = _msg.buf_message + offset(8);
	//	auto infoServer = buf_recv.extract<shared_buffer>();

	//	// 3) server_entity info를 저장한다.
	//	psource->m_info_server = infoServer;

	//	// 4) 정보를 필요한 곳에 전송한다.
	//	g_pservice->m_pgroup_server_session->send_active(_msg.buf_message);

	//	// 5) pServer Entity를 얻는다.
	//	auto pserver_entity = psource->m_pserver_entity;

	//	// check)
	//	CGASSERT_ERROR(pserver_entity.exist());

	//	// check)
	//	if (pserver_entity.exist())
	//	{
	//		// - 값을 Update한다.

	//		// notify)
	//		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity.get())));
	//		g_pservice->m_pgroup_client->send(pserver_entity->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	//	}
	//}
	//catch (...)
	//{
	//	// - 문제...
	//	CGASSERT_ERROR(false);

	//	// - 접속을 종료해 버린다.
	//	psource->closesocket();

	//	// Reraise)
	//	throw;
	//}

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::group::on_message_SERVER_STATE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Socket 객체를 얻는다.
	//auto psource = _msg.get_source<server_entity::socket>();

	//// check) psource이 nullptr이면 안됀다.
	//CGASSERT_ERROR(psource != nullptr);

	//try
	//{
	//	// 2) 
	//	auto buf_recv	   = _msg.buf_message + offset(8);
	//	auto status_server = buf_recv.extract<eOBJECT_STATE>();

	//	scoped_lock(m_lockable_group)
	//	{
	//		// check) 
	//		CGASSERT_ERROR(psource->m_pserver_entity.exist());

	//		// check) 기존과 상태가 동일하면 그냥 끝낸다.
	//		RETURN_IF(psource->m_pserver_entity->status_server.state == status_server, 1);

	//		// 3) server_entity info를 저장한다.
	//		psource->m_pserver_entity->status_server.state = status_server;

	//		// 4) 상태에 따라 처리
	//		switch(status_server)
	//		{
	//		case	eOBJECT_STATE::NONE:
	//				break;

	//		case	eOBJECT_STATE::STOPPED:
	//				break;

	//		case	eOBJECT_STATE::START_PENDING:
	//				break;

	//		case	eOBJECT_STATE::STOP_PENDING:
	//				break;

	//		case	eOBJECT_STATE::RUNNING:
	//				{
	//					// - Market List를 전송한다.
	//					auto buf_send = g_pservice->m_pgroup_server_trade->message_MARKET_TRADE_REGISTER();

	//					if(buf_send.size() != 0)
	//					{
	//						psource->send(buf_send);
	//					}

	//					// - Notify Server 정보를 전송한다.
	//					psource->send(g_pservice->m_pgroup_server_notify->message_SERVER_LIST_SEND());

	//					// - Session Server에 전달한다.
	//					g_pservice->m_pgroup_server_session->send_active(::message_SERVER_LIST_SEND(psource->get_server_setting_info(), psource->get_server_info()));
	//				}
	//				break;

	//		case	eOBJECT_STATE::CONTINUE_PENDING:
	//				break;

	//		case	eOBJECT_STATE::PAUSE_PENDING:
	//				break;

	//		case	eOBJECT_STATE::PAUSED:
	//				break;

	//		case	eOBJECT_STATE::INITIALIZING_PENDING:
	//				break;

	//		case	eOBJECT_STATE::DESTROYING_PENDING:
	//				break;

	//		case	eOBJECT_STATE::UPDATE:
	//				break;

	//		case	eOBJECT_STATE::COMMIT:
	//				break;

	//		case	eOBJECT_STATE::NOT_CONNECT:
	//				break;

	//		default:
	//				break;
	//		}
	//	}

	//	// 5) DB에 저장한다.
	//	g_pservice->m_pquerier_server->request_server_state(psource, status_server);

	//	// 5) pServer Entity를 얻는다.
	//	auto	pserver_entity	 = psource->m_pserver_entity;

	//	// check)
	//	CGASSERT_ERROR(pserver_entity.exist());

	//	// check)
	//	if(pserver_entity.exist())
	//	{
	//		// - 값을 Update한다.
	//		
	//		// notify)
	//		g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, reinterpret_cast<uintptr_t>(pserver_entity.get())));
	//		g_pservice->m_pgroup_client->send(pserver_entity->message_SERVER_ADMIN_SERVER_INSTANCE_UPDATE());
	//	}
	//}
	//catch (...)
	//{
	//	// - 문제...
	//	CGASSERT_ERROR(false);

	//	// - 접속을 종료해 버린다.
	//	psource->closesocket();

	//	// Reraise)
	//	throw;
	//}

	// return)
	return eRESULT::DONE;
}

shared_buffer server_entity::group::message_SERVER_LIST_SEND() const
{
	// 1) 버퍼를 할당받는다.
	auto bufTemp = alloc_shared_buffer(8192);

	// 2) 메시지를 작성해 넣는다.
	bufTemp.append<uint32_t>();
	bufTemp.append<eMESSAGE_TYPE>(eMESSAGE_TYPE::SERVER_LIST_SEND);

	scoped_lock(m_lockable_group)
	{
		// 3) 갯수를 써넣는다.
		auto&	countRef = bufTemp.append<COUNT_T>();
		COUNT_T	count	 = 0;

		// 4) Server 정보들을 써넣는다. 
		for(auto& iter: m_container_member)
		{
			// check) 
			CGASSERT_ERROR(iter->m_pserver_entity.exist());

			// check)
			CONTINUE_IF(iter->m_pserver_entity->status_server.state != eOBJECT_STATE::RUNNING);

			// - Append
			bufTemp.append(iter.get_member()->get_server_setting_info());
			bufTemp.append(iter.get_member()->get_server_info());

			// - 갯수를 증가.
			++count;
		}

		// 5) 크기를  써넣는다.
		countRef = count;
	}

	// 3) 메시지의 길이를 써넣는다.
	bufTemp.front<uint32_t>() = bufTemp.size<uint32_t>();

	// return)
	return bufTemp;
}
