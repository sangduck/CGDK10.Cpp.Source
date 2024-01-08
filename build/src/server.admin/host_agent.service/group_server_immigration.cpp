#include "pch.h"


void group_server_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - create GROUP_IMMIGRATION Server"sv;

	// 1) Register Mediator한다.
	this->register_messageable(service::instance());
}

void group_server_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy GROUP_IMMIGRATION Server"sv;

	// 1) Messageable을 unregister한다.
	this->unregister_messageable(service::instance());
}

void group_server_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	this->enable_member_enter();
}

void group_server_immigration::on_stop()
{
}

result_code group_server_immigration::on_member_entering(socket_server* /*_pmember*/, sMESSAGE& /*_param*/)
{
	return eRESULT::SUCCESS;
}

void group_server_immigration::on_member_entered(socket_server* _pmember, sMESSAGE& /*_param*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_server_immigration::on_member_leaving(socket_server* /*_pmember*/, uintptr_t /*_param*/)
{
	// return) 
	return 0;
}

void group_server_immigration::on_member_leaved(socket_server* _pmember, uintptr_t /*_result*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);
}

result_code group_server_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST, on_message_SERVER_ADMIN_CERTIFY_REQUEST, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST, on_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_ENTER, on_message_SERVER_ADMIN_INSTANCE_ENTER, sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP
	
	// return) 
	return eRESULT::BYPASS;
}

result_code group_server_immigration::on_message_SERVER_ADMIN_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// check) psocket_server가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_server>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto psocket_server = _msg.get_source<socket_server>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	try
	{
		// 3) get certify info
		auto certify_info = buf_temp.extract<server_admin::sCERTIFY_INFO>();

		// 4) set 
		psocket_server->m_certify_info = std::move(certify_info);

		// 5) 
		psocket_server->send(server_admin::message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::SUCCESS));
	}
	catch(...)
	{
		// - send result message

		// - 접속을 종료한다.
		psocket_server->disconnect();
	}

	// return) 
	return eRESULT::DONE;
}

result_code group_server_immigration::on_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// check) psocket_server가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_server>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto psocket_server = _msg.get_source<socket_server>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	try
	{
		// 3) get certify info
		auto pid = buf_temp.extract<uint64_t>();
		auto rank = buf_temp.extract<server_admin::sRANK>();

		// 4) find instance
		auto pinstance_service_local = service::pmanager_instance->process_find_instance_by(pid, rank);

		// declare) 
		shared_buffer buf_send;

		// 5) 
		if (pinstance_service_local.exist())
		{
			// - make response message
			scoped_lock(pinstance_service_local->m_cs_instance)
			{
				buf_send = server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(pinstance_service_local->m_instance.setting);
			}
		}
		else
		{
			// - make response message
			buf_send = server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT::NOT_EXIST);
		}

		// - send 
		psocket_server->send(buf_send);
	}
	catch (throwable::failure& _e)
	{
		// - send result message
		psocket_server->send(server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(_e.reason()));

		// - 접속을 종료한다.
		psocket_server->disconnect();
	}
	catch (...)
	{
		// - send result message
		psocket_server->send(server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - 접속을 종료한다.
		psocket_server->disconnect();
	}

	// return) 
	return eRESULT::DONE;
}

result_code group_server_immigration::on_message_SERVER_ADMIN_INSTANCE_ENTER(sMESSAGE_NETWORK& _msg)
{
	// check) psocket_server가 nullptr이면 안됀다.
	CGASSERT_ERROR(_msg.get_source<socket_server>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto psocket_server = _msg.get_source<socket_server>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// declare)
	std::vector<own_ptr<server_admin::Iinstance>> vector_instance_create;

	// 3) count
	auto instance_count = buf_temp.extract<COUNT_T>();

	for (; instance_count > 0; --instance_count)
	{
		// declare)
		own_ptr<server_admin::Iinstance> pinstance;

		// - get type
		auto instance_type = buf_temp.front<int>();

		switch (instance_type)
		{
		case	server_admin::eINSTANCE_TYPE::HOST:
				pinstance = make_own<server_admin::instance_host>();
				break;

		case	server_admin::eINSTANCE_TYPE::SERVICE:
				pinstance = make_own<server_admin::instance_service_remote>();
				break;

		default:
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
		}

		// check) 
		CGASSERT_ERROR(pinstance.exist());

		// - set
		pinstance->serialize_in(buf_temp);

		// - set socket
		pinstance->set_socket(std::move(psocket_server));

		// - push
		vector_instance_create.push_back(std::move(pinstance));

		// check) instance가 최소한 1개는 있어야 한다.
		THROW_IF(vector_instance_create.empty(), throwable::failure(eRESULT::ACCESS_DENIED));
	}

	// declare) catch문에서의 rollback을 위해 여기에 선언
	std::vector<object_ptr<server_admin::Iinstance>> vector_instance;

	try
	{
		scoped_lock(psocket_server->m_cs_instance)
		{
			// check)
			THROW_IF(psocket_server->m_certify_info.instance_type != server_admin::eINSTANCE_TYPE::SERVICE && psocket_server->m_certify_info.instance_type != server_admin::eINSTANCE_TYPE::HOST, throwable::failure(eRESULT::INVALID_REQUEST));

			// check) 기존에 host instance가 존재할 리도 없지만 있으면 이중 ENTER 요청이므로 잘못된 것이다.
			CGASSERT_ERROR(psocket_server->m_vector_instance.empty());

			// check)
			THROW_IF(psocket_server->m_vector_instance.empty() == false, throwable::failure(eRESULT::INVALID_STATE));

			// 4) ....
			for (auto& iter : vector_instance_create)
			{
				// check)
				CGASSERT_ERROR(iter->socket().exist());

				// - 추가
				auto pinstance = service::pmanager_instance->register_instance(std::move(iter));

				// - 먼저 iid를 등록
				vector_instance.push_back(pinstance);
			}

			// 5) iid list를 socket에 저장한다.
			psocket_server->m_vector_instance = std::move(vector_instance);
		}

		// 7) enter
		auto result = service::pgroup_server->enter(psocket_server);

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result));
	}
	catch (...)
	{
		// - rollback 
		scoped_lock(psocket_server->m_cs_instance)
		{
			vector_instance = std::move(psocket_server->m_vector_instance);
		}

		// - rollback
		for (auto iter : vector_instance)
		{
			service::pmanager_instance->unregister_instance(iter->get_iid(), psocket_server);
		}

		// - close socket
		psocket_server->closesocket();

		// reraise)
		throw;
	}

	// return)
	return eRESULT::DONE;
}