#include "pch.h"


void group_control::on_initializing(context& /*_context*/)
{
}

void group_control::on_initialize(context& /*_context*/)
{
	// 1) Tracer에 붙인다.
	service::get_logger()->register_log_targetable(service::instance());

	// log) 
	LOG_INFO << " - initialize GROUP['"sv << name() << "']"sv;
}

void group_control::on_destroying()
{
}

void group_control::on_destroy()
{
	// 1) Tracer에 붙인다.
	service::get_logger()->unregister_log_targetable(service::instance());

	// log) 
	LOG_INFO << " - destroy GROUP['"sv << name() << "']"sv;
}

void group_control::on_starting(context& /*_context*/)
{
}

void group_control::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	this->enable_member_enter();
}

void group_control::on_stopping()
{
}

void group_control::on_stop()
{
	// 1) Enter를 Disable한다.
	this->disable_member_enter();
}

result_code group_control::on_member_entering(socket_server* /*_pmember*/, sMESSAGE& /*_param*/)
{
	//// 2) send success messageS
	//_pmember->send(message_SERVER_ADMIN_SUCCCESS(0));

	// return) 
	return eRESULT::SUCCESS;
}

void group_control::on_member_entered(socket_server* _pmember, sMESSAGE& /*_param*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_control::on_member_leaving(socket_server* _pmember, uintptr_t /*_param*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);

	// return) 
	return	0;
}

void group_control::on_member_leaved(socket_server* /*_pmember*/, uintptr_t /*_result*/)
{
}

#ifdef _USE_SVN_CONTROL
void group_control::on_svn_notify(server_admin::Isvn_command* /*_baton*/, const svn_wc_notify_t* /*_notify*/, apr_pool_t* /*_pool*/)
{
	// 제거
}

void group_control::on_svn_procgress(apr_off_t /*_progress*/, apr_off_t /*_total*/, server_admin::Isvn_command* /*_baton*/, apr_pool_t* /*_pool*/)
{
	// 제거
}

svn_error_t* group_control::on_svn_log(const char** /*_log_msg*/, const char** /*_tmp_file*/, const apr_array_header_t* /*_commit_items*/, server_admin::Isvn_command* /*_baton*/, apr_pool_t* /*_pool*/)
{
	// 제거
	return		nullptr;
}

svn_error_t* group_control::on_svn_conflict_resolver(svn_wc_conflict_result_t** /*_result*/, const svn_wc_conflict_description2_t* /*_description*/, server_admin::Isvn_command* /*_baton*/, apr_pool_t* /*_result_pool*/, apr_pool_t* /*_scratch_pool*/)
{
	// 제거
	return		nullptr;
}

svn_error_t* group_control::on_svn_cancel(server_admin::Isvn_command* /*_cancel_baton*/)
{
	// 제거
	return		nullptr;
}

void group_control::on_svn_complete(server_admin::Isvn_command* /*_cancel_baton*/, const SVN_RESULT& /*_result*/)
{
}
#endif

result_code group_control::process_message(sMESSAGE& _msg)
{
	// 1) Message를 실행...
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
			//ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::SEND_HOST_SETTING,			on_message_SERVER_HOST_SETTING,						sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_REGISTER,			on_message_SERVER_ADMIN_INSTANCE_REGISTER,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UNREGISTER,			on_message_SERVER_ADMIN_INSTANCE_UNREGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE,				on_message_SERVER_ADMIN_INSTANCE_UPDATE,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_VERSION_UPDATE,		on_message_SERVER_ADMIN_REQUEST_VERSION_UPDATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE,			on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE,		on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE,		on_message_SERVER_ADMIN_REQUEST_START_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE,			on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE,		on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE,		on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE,			on_message_SERVER_ADMIN_REQUEST_KILL_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_LOG,					on_message_SERVER_ADMIN_REQUEST_LOG,				sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP

		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,				on_message_SERVER_TIME_EVENT_EVENT_REGISTER,		sMESSAGE);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,		sMESSAGE);
		ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE,				on_message_SERVER_TIME_EVENT_EVENT_EXECUTE,			sMESSAGE);
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::BYPASS;
}

result_code group_control::on_message_SERVER_HOST_SETTING(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check)
	//CGASSERT_ERROR(_msg.buf_message.size() > 8);

	//// 1) 값 읽기...
	//auto host_setting = _msg.buf_message.front<server_admin::sHOST_SETTING>(8);

	//// 2) ....
	//service::set_host_setting(host_setting);

	//// 3) control에 전달한다.
	//send(_msg.buf_message);

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_INSTANCE_REGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	////// check) 길이가 맞지 않으면 Throw한다.
	////THROW_IF(_msg.buf_message.size()<(8 + sizeof(net::sockaddr) + sizeof(net::sockaddr) + sizeof(server_admin::SERVER_SETTING) + sizeof(int)), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// declare)
	//auto buf_read = _msg.buf_message + offset(8);

	//// 2) Identification
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//// 3) 정보를 읽어들인다.
	//auto info_server_register = buf_read.extract<server_admin::sSERVICE_INSTANCE_INFO>();
	//auto bUpdate = buf_read.extract<int>();

	//try
	//{
	//	// 3) 임시 객체를 생성한다.
	//	auto pserver_register_info = make_object<server_admin::instance_service_local>();

	//	// 4) 값을 복사한다.
	//	pserver_register_info->set_server_register_info(info_server_register);

	//	// 5) 추가한다.
	//	bool result = service::register_service_instance(pserver_register_info);

	//	// check) 추가에 실패했다면 Error를 전송하고 끝낸다.
	//	if(result == false)
	//	{
	//		_msg.get_source<socket_server>()->send(message_SERVER_ADMIN_ERROR(0, (uint64_t)INVALID_PROCESS_HANDLE));
	//	}

	//	// 6) '즉시 Update'가 true로 설정되어 있으면 바로 Update한다.
	//	if (bUpdate != 0)
	//	{
	//		pserver_register_info->request_update();
	//	}
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_REGISTER, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_REGISTER, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_INSTANCE_UNREGISTER(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size()!=8+sizeof(net::sockaddr)+sizeof(server_admin::iid_t)+sizeof(uint64_t)+sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// declare)
	//auto buf_read = _msg.buf_message + offset(8);

	//// 2) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// 3) Register Info
	//	auto pregister_info = service::find_service(id_server);

	//	// check) 찾지 못했다면..
	//	RETURN_IF(pregister_info.empty(), 1);

	//	// 4) 해당 Server Info를 찾았다면 unregister한다.
	//	service::unregister_service_instance(pregister_info);
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UNREGISTER, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}
	
	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_INSTANCE_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	////// check) 길이가 맞지 않으면 Throw한다.
	////THROW_IF(_msg.buf_message.size() < (8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t) +sizeof(server_admin::sSERVICE_INSTANCE_INFO)+sizeof(int)), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// declare)
	//auto buf_read = _msg.buf_message + offset(8);

	//// 2) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//// 3) SystemID와 SystemCode를 얻는다.
	//auto info_server_register = buf_read.extract<server_admin::sSERVICE_INSTANCE_INFO>();
	//auto bUpdate = buf_read.extract<int>();

	//try
	//{
	//	// 3) 해당 Server Info를 찾았다면 unregister한다.
	//	auto pitem_server = service::update_server_setting(info_server_register);

	//	// 4) Update...
	//	if (bUpdate != 0)
	//	{
	//		pitem_server->request_update();
	//	}
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::SERVER_INSTANCE_UPDATE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_VERSION_UPDATE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size() < 8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// declare)
	//auto buf_read = _msg.buf_message + offset(8);

	//// 2) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// 3) 해당 Server Info를 찾았다면 unregister한다.
	//	auto result = service::request_version_update(id_server);

	//	// check) 실패했을 경우 그 결과를 통보해 준다.
	//	if (result == false)
	//	{
	//		//psource->send();
	//	}

	//	// 4) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_VERSION_UPDATE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_VERSION_UPDATE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size()!=8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다.
	//	THROW_IF(host_iid.mac != service::m_host_instance_info.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = service::request_service_initialize(id_server);

	//	// check) 
	//	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch(...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size()!=8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다.
	//	THROW_IF(host_iid.mac != service::m_host_instance_info.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = service::request_service_destroy(id_server);

	//	// check) 
	//	THROW_IF(result!=false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size() < 8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();
	//auto str_setting = buf_read.extract<std::string>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다.
	//	THROW_IF(host_iid.mac != service::m_host_instance_info.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = service::request_service_start(id_server, str_setting);

	//	// check) 
	//	THROW_IF(result == false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size() != 8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다.
	//	THROW_IF(host_iid.mac != service::m_host_instance_info.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = service::request_service_stop(id_server);

	//	// check) 
	//	THROW_IF(result != false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size() != 8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다. (host_iid.i64Code의 내용이 Local의 Address이다.)
	//	THROW_IF(host_iid.mac != service::m_host_instance_info.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = service::request_service_pause(id_server);

	//	// check) 
	//	THROW_IF(result != false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size()!=8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다.
	//	THROW_IF(host_iid.mac != server_admin::instance_manager::self()->m_instance.host_iid.mac, throwable::failure(eRESULT::FAIL));

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = server_admin::instance_manager::request_service_continue(id_server);

	//	// check) 
	//	THROW_IF(result != false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_KILL_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	//// check) 길이가 맞지 않으면 Throw한다.
	//THROW_IF(_msg.buf_message.size()!=8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t), std::exception());

	//// 1) get source (source socket_server)
	//auto* psource = _msg.get_source<socket_server>();

	//// 2) 임시 버퍼
	//auto buf_read = _msg.buf_message + offset(8);

	//// 3) Host ID/Server ID
	//auto host_iid = buf_read.extract<server_admin::iid_t>();
	//auto id_server = buf_read.extract<server_admin::iid_t>();

	//try
	//{
	//	// check) Host ID를 자신인가를 확인한다. (host_iid.i64Code의 내용이 Local의 Address이다.)
	//	RETURN_IF(host_iid.mac != server_admin::instance_manager::self()->m_instance.host_iid.mac, 1);

	//	// 4) 접속된 모든 Server에 접속요청신호를 전송한다.
	//	auto result = server_admin::instance_manager::request_service_kill(id_server);

	//	// check) 
	//	THROW_IF(result != false, throwable::failure(eRESULT::FAIL));

	//	// 5) 결과를 전송한다.
	//}
	//catch (throwable::failure& _e)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE, _e.reason<int>(), _e.description()));
	//}
	//catch (...)
	//{
	//	psource->send(server_admin::message_SERVER_ADMIN_ERROR(host_iid, id_server, eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE, static_cast<int>(eRESULT::EXCEPTION), "unknowned"));
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_ADMIN_REQUEST_LOG(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) Log File을 얻는다.
	//object_ptr<logger_hub_archive> plogger = dynamic_pointer_cast<logger_hub_archive>(service::get_logger());

	//// check) LogFile이 없으면 1을 리턴한다.
	//RETURN_IF(plogger.empty(), eRESULT::DONE);

	//// 2) Source를 얻는다.
	//auto* psource = _msg.get_source<socket_server>();

	//try
	//{
	//	// 3) Log의 Position을 얻는다.
	//	auto position = _msg.buf_message.front<uint64_t>(8);

	//	// 4) Position의 차이가 많이 나면 Position값을 수정한다.
	//	if(position > plogger->get_log_count())
	//	{
	//		psource->send(message_SERVER_ADMIN_REQUEST_LOG(0));
	//		position = 0;
	//	}
	//	else if((plogger->get_log_count()-position)>512)
	//	{
	//		position = plogger->get_log_count()-512;
	//		psource->send(message_SERVER_ADMIN_REQUEST_LOG(position-1));
	//	}

	//	// declare) 
	//	std::vector<object_ptr<LOG_RECORD>>	vector_log_record;

	//	// 4) Position 이후의 Log정보를 읽는다.
	//	vector_log_record = plogger->get_stored_log(position);

	//	// check) 아무 버퍼가 없으면 끝낸다.
	//	RETURN_IF(vector_log_record.empty(), eRESULT::DONE);

	//	// 5) 모두 전송한다.
	//	auto iter = vector_log_record.begin();
	//	auto iterEnd = vector_log_record.end();

	//	for(; iter!=iterEnd; ++iter)
	//	{
	//		psource->send(server_admin::message_SYSTEM_LOG(*iter));
	//	}
	//}
	//catch (throwable::failure& /*_e*/)
	//{
	//}
	//catch (...)
	//{
	//}

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg)
{
	// 1) Event 객체를 얻는다.
	auto* pevent = _msg.get_source<time_event::Ientity>();

	// check) 
	RETURN_IF(pevent == nullptr, eRESULT::DONE);

	// 2) 전송한다.
	send(message_SERVER_TIME_EVENT_EVENT_REGISTER(pevent));

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg)
{
	// 1) Event 객체를 얻는다.
	auto* pevent = _msg.get_source<time_event::Ientity>();

	// check) 
	RETURN_IF(pevent == nullptr, eRESULT::DONE);

	// 2) 전송한다.
	send(message_SERVER_TIME_EVENT_EVENT_UNREGISTER(pevent));

	// return) 
	return eRESULT::DONE;
}

result_code group_control::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& _msg)
{
	// 1) Event 객체를 얻는다.
	auto* pevent = _msg.get_source<time_event::Ientity>();

	// check) 
	RETURN_IF(pevent == nullptr, eRESULT::DONE);

	// 2) 전송한다.
	this->send(message_SERVER_TIME_EVENT_EVENT_EXECUTE(pevent));

	// return) 
	return eRESULT::DONE;
}

shared_buffer group_control::message_SERVER_TIME_EVENT_EVENT_LIST() const
{
	// locked) 
	{
		// scoped_lock) 
		std::lock_guard<lockable<>&> cs(time_event::entity_manager::instance()->get_lockable());

		// 1) Message의 길이를 얻는다.
		size_t size_of_message = 8 + sizeof(int);

		// 2) ...
		for (auto& iter : *time_event::entity_manager::instance())
		{
			size_of_message += get_size_of(iter->entity_setting());
			size_of_message += get_size_of(iter->entity_status());
		}

		// 3) 버퍼를 할당받는다.
		auto buf_temp = alloc_shared_buffer(size_of_message);

		// 4) 추가한다.
		buf_temp.append<uint32_t>();
		buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST);

		// 5) Event Info의 갯수를 써넣는다.
		buf_temp.append<int>(static_cast<int>(time_event::entity_manager::instance()->size()));
	
		// 6) Event Info를 써넣는다.
		for(auto& iter: *time_event::entity_manager::instance())
		{
			buf_temp.append<time_event::sENTITY_SETTING>(iter->entity_setting());
			buf_temp.append<time_event::sENTITY_STATUS>(iter->entity_status());
		}

		// 7) Message의 길이를 설정한다.
		buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

		// check) 
		CGASSERT_ERROR(size_of_message == buf_temp.size());

		// return) 
		return buf_temp;
	}
}

shared_buffer group_control::message_SERVER_TIME_EVENT_EVENT_REGISTER(time_event::Ientity* _pevent) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + get_size_of(_pevent->entity_setting()) + get_size_of(_pevent->entity_status());

	// 2) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER);
	buf_temp.append<time_event::sENTITY_SETTING>(_pevent->entity_setting());
	buf_temp.append<time_event::sENTITY_STATUS>(_pevent->entity_status());

	// 4) Message 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer group_control::message_SERVER_TIME_EVENT_EVENT_UNREGISTER(time_event::Ientity* _pevent) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + get_size_of(_pevent->entity_status());

	// 2) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER);
	buf_temp.append<time_event::sENTITY_STATUS>(_pevent->entity_status());

	// 4) Message 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer group_control::message_SERVER_TIME_EVENT_EVENT_EXECUTE(time_event::Ientity* _pevent) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + get_size_of(_pevent->entity_setting()) + get_size_of(_pevent->entity_status());

	// 2) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE);
	buf_temp.append<time_event::sENTITY_SETTING>(_pevent->entity_setting());
	buf_temp.append<time_event::sENTITY_STATUS>(_pevent->entity_status());

	// 4) Message 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer group_control::message_SERVER_ADMIN_SUCCCESS(uint64_t _authority) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + sizeof(_authority);

	// 2) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SUCCESS);
	buf_temp.append<uint64_t>(_authority);

	// 4) Message 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer group_control::message_SERVER_ADMIN_REQUEST_LOG(uint64_t _index) const
{
	// 1) Message의 길이를 얻는다.
	auto size_of_message = 8 + sizeof(server_admin::iid_t) + sizeof(server_admin::iid_t) + sizeof(_index);

	// 1) 버퍼를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message를 작성한다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_LOG);
	buf_temp.append<server_admin::iid_t>();	// host_iid
	buf_temp.append<server_admin::iid_t>();	// id_server
	buf_temp.append<uint64_t>(_index);

	// 3) Message 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}
