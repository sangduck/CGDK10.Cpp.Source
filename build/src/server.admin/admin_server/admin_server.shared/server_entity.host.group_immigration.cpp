#include "pch.h"


server_entity::host::group_immigration::group_immigration(std::string_view _name) :
	group::list_sender<socket>(),
	net::io::messageable::Nbase(),
	Nmessage_transmitter(),
	Nobject_state(),
	Ninitializable(),
	Nstartable(),
	Nnameable(_name)
{
}

server_entity::host::group_immigration::~group_immigration()
{
}

void server_entity::host::group_immigration::on_initializing(context& /*_context*/)
{
}

void server_entity::host::group_immigration::on_initialize(context& /*_context*/)
{
}

void server_entity::host::group_immigration::on_destroying()
{
}

void server_entity::host::group_immigration::on_destroy()
{
}

void server_entity::host::group_immigration::on_starting(context& /*_context*/)
{
}

void server_entity::host::group_immigration::on_start(context& /*_context*/)
{
	// 1) 입장을 받기 시작~
	enable_member_enter(true);
}

void server_entity::host::group_immigration::on_stopping()
{
	// 1) 실행 등록 해제한다.
	system_executor::unregister_schedulable(this);

	// 2) 입장을 금지
	disable_member_enter();

	// 3) 모든 Group에 접속한 모든 Member를 모두 접속 종료시킨다.
	for_each_member([=](server_entity::host::socket* _pmember)->bool
	{
		_pmember->closesocket();
		return true;
	});

	// 4) Leavel All Memberl
	leave_all();
}

void server_entity::host::group_immigration::on_stop()
{
}

result_code server_entity::host::group_immigration::on_member_entering(server_entity::host::socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void server_entity::host::group_immigration::on_member_entered(server_entity::host::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) Register Messageable한다.
	_pmember->register_messageable(this);

	// 2) register schedulable
	if (get_member_count() == 1)
	{
		// - interval = 10s
		execute_interval(10s);

		// - register
		system_executor::register_schedulable(this);
	}
}

uintptr_t server_entity::host::group_immigration::on_member_leaving(server_entity::host::socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) unregister schedulable
	if (get_member_count() == 1)
	{
		system_executor::unregister_schedulable(this);
	}

	// 2) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	// return) 
	return	0;
}

void server_entity::host::group_immigration::on_member_leaved(server_entity::host::socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

void server_entity::host::group_immigration::on_execute()
{
}

result_code server_entity::host::group_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) Message Map
	EXECUTE_CGMESSAGE_MAP(_msg);

	// return) Message 처리를 하지 못했다!!!
	return eRESULT::BYPASS;
}

// Message Map)
BEGIN_CGMESSAGE_MAP(server_entity::host::group_immigration)
	// 1) from server_entity::host
	CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST,		on_message_SERVER_ADMIN_CERTIFY_REQUEST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_HOST_SETTING,	on_message_SERVER_ADMIN_SEND_HOST_SETTING,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::HOST_STATUS_UPDATE,	on_message_SERVER_ADMIN_HOST_STATUS_UPDATE,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_LIST,	on_message_SERVER_ADMIN_SERVER_INSTANCE_LIST,	sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST,		on_message_SERVER_TIME_EVENT_EVENT_LIST,		sMESSAGE_NETWORK);
		ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SUCCESS,				on_message_SERVER_ADMIN_SUCCESS,				sMESSAGE_NETWORK);
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code server_entity::host::group_immigration::on_message_SERVER_ADMIN_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// check) check message length
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) check message length
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) get source socket
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) temporary buffer
	buffer_view buf_message = _msg.buf_message + offset(8);

	// 3) get host info
	auto info_host = buf_message.extract<server_admin::sHOST_INFO>();

	// 4) get client type
	auto type = buf_message.extract<int>();

	// check)
	if (type != 1)
	{
		// - closesocket
		psource->closesocket();

		// return)
		return eRESULT::DONE;
	}

	// 5) set host info
	psource->set_host_info(info_host);

	// 6) send response message
	psource->send(server_admin::message_SERVER_ADMIN_CERTIFY_RESPONSE());

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group_immigration::on_message_SERVER_ADMIN_SEND_HOST_SETTING(sMESSAGE_NETWORK& _msg)
{
	// check) Message의 길이를 검사한다.
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) Message의 길이를 검사한다
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Register Infof를 얻는다.
	auto host_setting = _msg.buf_message.front<server_admin::sHOST_SETTING>(8);

	// 3) 설정한다.
	psource->set_host_setting(host_setting);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group_immigration::on_message_SERVER_ADMIN_HOST_STATUS_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// check) Message의 길이를 검사한다.
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) Message의 길이를 검사한다
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Register Infof를 얻는다.
	auto host_status = _msg.buf_message.front<server_admin::sHOST_STATUS>(8);

	// 3) 설정한다.
	psource->set_host_status(host_status);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group_immigration::on_message_SERVER_ADMIN_SERVER_INSTANCE_LIST(sMESSAGE_NETWORK& _msg)
{
	// check) Message의 길이를 검사한다.
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) Message의 길이를 검사한다
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) 읽기용 버퍼 포인터를 생성한다.
	auto buf_read = _msg.buf_message + offset(8);

	// 3) host ID를 얻는다.
	auto hid = buf_read.extract<server_admin::sHOST_ID>();

	// check) host ID가 같은지 확인한다.
	CGASSERT_ERROR(psource->m_info_host.hid == hid);

	// declare)
	vector<object_ptr<server_entity::instance>> vector_server_register;
	object_ptr<server_entity::manager_instance> pmanager_server_instance = g_pservice->m_pmanager_server_instance;

	// 4) get count
	auto count = buf_read.extract<COUNT_T>();

	// 5) reserve
	vector_server_register.reserve(count);

	for (auto i = 0; i < count; ++i)
	{
		// - server setting/status
		auto server_info = buf_read.extract<server_admin::server_info>();

		// - register to...
		auto pserver_instance = pmanager_server_instance->register_instance(server_info.registered, psource);

		// - register to...
		pmanager_server_instance->enter_instance(std::move(server_info), psource);

		// - push
		vector_server_register.push_back(pserver_instance);
	}

	// 4) 서버 정보들을 읽어 들여 등록한다.
	scoped_lock(psource->m_lockable_data)
	{
		psource->m_vector_server_instance = std::move(vector_server_register);
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group_immigration::on_message_SERVER_TIME_EVENT_EVENT_LIST(sMESSAGE_NETWORK& _msg)
{
	// check) Message의 길이를 검사한다.
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) Message의 길이를 검사한다
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// 2) Event의 갯수를 얻는다.
	int count = _msg.buf_message.front<int>(8);
	const auto*	pevent_info = (count!=0) ? &_msg.buf_message.front<time_event::sEVENT_SETTING>(12) : nullptr;

	// 3) 설정
	psource->set_event_list(count, pevent_info);

	// return) 
	return eRESULT::DONE;
}

result_code server_entity::host::group_immigration::on_message_SERVER_ADMIN_SUCCESS(sMESSAGE_NETWORK& _msg)
{
	// check) Message의 길이를 검사한다.
	CGASSERT_ERROR(_msg.buf_message.size() > 8);

	// check) Message의 길이를 검사한다
	THROW_IF(_msg.buf_message.size() <= 8, throwable::failure(eRESULT::INVALID_MESSAGE_LENGTH, 0, "invalid message length" __FUNCTION__));

	// 1) Socket 객체를 얻는다.
	auto psource = _msg.get_source<server_entity::host::socket>();

	// notify)
	g_pservice->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_HOST_SOCKET_UPDATE, reinterpret_cast<uintptr_t>(psource)));
	g_pservice->m_pgroup_client->send(MESSAGE_ADMIN_HOST_SOCKET_UPDATE::MAKE_BUFFER(psource->m_info_host, psource->m_status_host));

	// 2) Enter한다.
	g_pservice->m_pgroup_server_entity_agent->enter(psource);

	// return)
	return eRESULT::DONE;
}

shared_buffer server_entity::host::group_immigration::message_SERVER_ADMIN_LOGIN_ACCEPT(const server_admin::sLOGIN_ACCEPT& _rAdminServerInfo)
{
	// 1) shared_buffer할당.
	auto temp_buffer = alloc_shared_buffer(8 + sizeof(server_admin::sLOGIN_ACCEPT));

	// 2) 값을 설정한다.
	temp_buffer.append<uint32_t>(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(server_admin::sLOGIN_ACCEPT));
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::LOGIN_ACCEPT);
	temp_buffer.append<server_admin::sLOGIN_ACCEPT>(_rAdminServerInfo);
	
	// return)
	return temp_buffer;
}
