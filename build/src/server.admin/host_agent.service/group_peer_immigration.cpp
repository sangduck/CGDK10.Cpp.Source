#include "pch.h"


void group_peer_immigration::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - create GROUP_IMMIGRATION Server"sv;

	// 1) Register Mediator한다.
	this->register_messageable(::service::instance());
}

void group_peer_immigration::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy GROUP_IMMIGRATION Server"sv;

	// 1) Messageable을 unregister한다.
	this->unregister_messageable(::service::instance());
}

void group_peer_immigration::on_start(context& /*_context*/)
{
	// 1) Enter를 enable한다.
	this->enable_member_enter();
}

void group_peer_immigration::on_stop()
{
}

result_code group_peer_immigration::on_member_entering(socket_peer* /*_pmember*/, sMESSAGE& /*_param*/)
{
	return eRESULT::SUCCESS;
}

void group_peer_immigration::on_member_entered(socket_peer* _pmember, sMESSAGE& /*_param*/)
{
	// 1) _pmember의 Mediator를 this로 설정한다.
	_pmember->register_messageable(this);
}

uintptr_t group_peer_immigration::on_member_leaving(socket_peer* /*_pmember*/, uintptr_t /*_param*/)
{
	// return) 
	return 0;
}

void group_peer_immigration::on_member_leaved(socket_peer* _pmember, uintptr_t /*_result*/)
{
	// 1) _pmember의 Mediator에서 떼낸다.
	_pmember->unregister_messageable(this);
}

result_code group_peer_immigration::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST, on_message_server_system_CERTIFY_REQUEST, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST, on_message_server_system_INSTANCE_CERTIFY_REQUEST, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_ENTER, on_message_server_system_INSTANCE_ENTER, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CLIENT_REQUEST, on_message_server_system_CLIENT_REQUEST, sMESSAGE_NETWORK);
			
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP
	
	// return) 
	return eRESULT::BYPASS;
}

result_code group_peer_immigration::on_message_server_system_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// check) psocket_peer가 nullptr이면 안 된다.
	CGASSERT_ERROR(_msg.get_source<socket_peer>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto psocket_peer = _msg.get_source<socket_peer>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	try
	{
		// 3) get certify info
		auto certify_info = buf_temp.extract<server_system::sCERTIFY_INFO>();
		std::string_view certify_key = (buf_temp.size() != 0) ? buf_temp.extract<std::string_view>() : std::string_view();

		// check) certify key
		if (::service::access_policy_server.get()->is_key_matched(certify_key) == false)
		{
			LOG_WARNING << "(warn) service certify rejected (certify key mismatch)"sv;
			psocket_peer->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::ACCESS_DENIED));
			psocket_peer->disconnect();
			return eRESULT::DONE;
		}

		// 4) set 
		psocket_peer->m_certify_info = std::move(certify_info);

		// 5) 
		psocket_peer->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::SUCCESS));
	}
	catch(...)
	{
		// - send result message

		// - 접속을 종료한다.
		psocket_peer->disconnect();
	}

	// return) 
	return eRESULT::DONE;
}

result_code group_peer_immigration::on_message_server_system_INSTANCE_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) service(server_system::socket::client)의 instance 인증
	//
	//  - message: COUNT_T n x {sHOST, sINSTANCE_KEY}
	//             COUNT_T m x {sSERVICE, sINSTANCE_KEY, iid_t host_instance_id}
	//  - service를 이 host agent의 instance(server list)와 맞춘다. (pid -> rank -> iid)
	//  - 확정된 정보(iid/setting은 host agent의 것)를 응답으로 보낸다.
	//    (service는 응답의 정보로 자기 정보를 바꾼다. 이후 INSTANCE_UPDATE_STATE를 보낸다.)
	//-----------------------------------------------------------------

	// check) psocket_peer가 nullptr이면 안 된다.
	CGASSERT_ERROR(_msg.get_source<socket_peer>() != nullptr);

	// 1) get psocket_peer
	object_ptr<socket_peer> psocket_peer = _msg.get_source<socket_peer>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	try
	{
		// 3) host instances (service가 보낸 host 정보는 쓰지 않는다. 이 host agent가 host이다.)
		auto count_host = buf_temp.extract<COUNT_T>();
		for (; count_host > 0; --count_host)
		{
			[[maybe_unused]] auto host_info = buf_temp.extract<server_system::sHOST>();
			[[maybe_unused]] auto host_instance_key = buf_temp.extract<server_system::sINSTANCE_KEY>();
		}

		// 4) service instances (보통 service 자신 1개)
		auto count_service = buf_temp.extract<COUNT_T>();

		// check)
		THROW_IF(count_service <= 0, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "no service instance"sv));

		// declare)
		std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> map_service_confirmed;
		auto host_iid = ::service::pmanager_instance->self()->get_iid();

		for (; count_service > 0; --count_service)
		{
			// - service info
			auto service_info = buf_temp.extract<server_system::sSERVICE>();
			auto instance_key = buf_temp.extract<server_system::sINSTANCE_KEY>();
			[[maybe_unused]] auto host_instance_id = buf_temp.extract<server_system::iid_t>();
			auto iid_original = service_info.setting.iid;

			// - 이 host agent의 instance를 찾는다. (pid -> rank -> iid)
			auto pinstance_local = ::service::pmanager_instance->process_find_instance_by(service_info.info.pid, service_info.setting.rank);
			if (pinstance_local.empty() && iid_original != 0 && iid_original != server_system::IID_AUTO)
				pinstance_local = dynamic_pointer_cast<server_system::instance_service_local>(::service::pmanager_instance->find_instance(iid_original));

			// check) server list에 없는 service
			THROW_IF(pinstance_local.empty(), throwable::failure(eRESULT::NOT_EXIST, 0, "service is not in the server list of host agent"sv));

			// - 확정 정보 (setting(iid/name/rank/...)은 host agent의 것을 쓴다.)
			SCOPED_LOCK(pinstance_local->m_lockable_instance)
			{
				service_info.setting = pinstance_local->m_instance.info.setting;
			}
			service_info.info.host_iid = host_iid;

			// - remote instance를 만들어 등록한다. (pid/rank로 local instance와 합쳐지고 socket이 연결된다.)
			auto pinstance_remote = make_own<server_system::instance_service_remote>();
			pinstance_remote->m_instance.info = service_info;
			pinstance_remote->m_instance.instance_key = instance_key;
			pinstance_remote->m_instance.host_instance_id = host_iid;
			pinstance_remote->set_socket(object_ptr<socket_peer>(psocket_peer));

			auto pinstance = ::service::pmanager_instance->register_instance(std::move(pinstance_remote));

			// - socket에 instance를 기억한다.
			SCOPED_LOCK(psocket_peer->m_lockable_instance)
			{
				psocket_peer->m_vector_instance.push_back(pinstance);
			}

			// - 응답할 확정 정보
			auto pservice_confirmed = make_object<server_system::service::instance>();
			pservice_confirmed->info = std::move(service_info);
			pservice_confirmed->host_instance_id = host_iid;
			map_service_confirmed.emplace(iid_original, std::move(pservice_confirmed));

			// log)
			LOG_INFO << "  - INSTANCE('"sv << pinstance->get_name() << "') > certified [iid:"sv << pinstance->get_iid() << "]"sv;
		}

		// 5) enter group_peer_server (이후 instance의 메시지를 받는다.)
		auto result = ::service::pgroup_peer_server->enter(psocket_peer);

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result));

		// 6) response (admin self info: 이 host agent)
		server_system::sSERVICE info_host_agent;
		info_host_agent.setting.iid = host_iid;
		info_host_agent.setting.name = "host_agent";

		psocket_peer->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(info_host_agent, {}, map_service_confirmed));
	}
	catch (throwable::failure& _e)
	{
		// log)
		LOG_WARNING << "(warn) fail to certify service instance ("sv << _e.description() << ")"sv;

		// - send result message (NOT_EXIST이면 service는 접속을 끊고 다시 시도한다.)
		psocket_peer->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(_e.reason<eRESULT>()));

		// - 접속을 종료한다.
		psocket_peer->disconnect();
	}
	catch (...)
	{
		// - send result message
		psocket_peer->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - 접속을 종료한다.
		psocket_peer->disconnect();
	}

	// return)
	return eRESULT::DONE;
}

result_code group_peer_immigration::on_message_server_system_INSTANCE_ENTER(sMESSAGE_NETWORK& _msg)
{
	// check) psocket_peer가 nullptr이면 안 된다.
	CGASSERT_ERROR(_msg.get_source<socket_peer>() != nullptr);

	// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	auto psocket_peer = _msg.get_source<socket_peer>();

	// 2) get buf
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// declare)
	std::vector<own_ptr<server_system::Iinstance>> vector_instance_create;

	// 3) count
	auto instance_count = buf_temp.extract<COUNT_T>();

	for (; instance_count > 0; --instance_count)
	{
		// declare)
		own_ptr<server_system::Iinstance> pinstance;

		// - get type
		auto instance_type = buf_temp.front<int>();

		switch (instance_type)
		{
		case	::eINSTANCE_TYPE::HOST:
				pinstance = make_own<server_system::instance_host>();
				break;

		case	::eINSTANCE_TYPE::SERVICE:
				pinstance = make_own<server_system::instance_service_remote>();
				break;

		default:
				throw throwable::failure(eRESULT::INVALID_ARGUMENT);
		}

		// check) 
		CGASSERT_ERROR(pinstance.exist());

		// - set
		pinstance->serialize_in(buf_temp);

		// - set socket
		pinstance->set_socket(std::move(psocket_peer));

		// - push
		vector_instance_create.push_back(std::move(pinstance));

		// check) instance가 최소한 1개는 있어야 한다.
		THROW_IF(vector_instance_create.empty(), throwable::failure(eRESULT::ACCESS_DENIED));
	}

	// declare) catch문에서의 rollback을 위해 여기에 선언
	std::vector<object_ptr<server_system::Iinstance>> vector_instance;

	try
	{
		SCOPED_LOCK(psocket_peer->m_lockable_instance)
		{
			// check)
			THROW_IF(psocket_peer->m_certify_info.dummy != ::eINSTANCE_TYPE::SERVICE && psocket_peer->m_certify_info.dummy != ::eINSTANCE_TYPE::HOST, throwable::failure(eRESULT::INVALID_REQUEST));

			// check) 기존에 host instance가 존재할 리도 없지만 있으면 이중 ENTER 요청이므로 잘못된 것이다.
			CGASSERT_ERROR(psocket_peer->m_vector_instance.empty());

			// check)
			THROW_IF(psocket_peer->m_vector_instance.empty() == false, throwable::failure(eRESULT::INVALID_STATE));

			// 4) ....
			for (auto& iter : vector_instance_create)
			{
				// check)
				CGASSERT_ERROR(iter->socket().exist());

				// - 추가
				auto pinstance = ::service::pmanager_instance->register_instance(std::move(iter));

				// - 먼저 iid를 등록
				vector_instance.push_back(pinstance);
			}

			// 5) iid list를 socket에 저장한다.
			psocket_peer->m_vector_instance = std::move(vector_instance);
		}

		// 7) enter
		auto result = ::service::pgroup_peer_server->enter(psocket_peer);

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result));
	}
	catch (...)
	{
		// - rollback 
		SCOPED_LOCK(psocket_peer->m_lockable_instance)
		{
			vector_instance = std::move(psocket_peer->m_vector_instance);
		}

		// - rollback
		for (auto iter : vector_instance)
		{
			::service::pmanager_instance->unregister_instance(iter->get_iid(), psocket_peer);
		}

		// - close socket
		psocket_peer->closesocket();

		// reraise)
		throw;
	}

	// return)
	return eRESULT::DONE;
}

result_code group_peer_immigration::on_message_server_system_CLIENT_REQUEST([[maybe_unused]] sMESSAGE_NETWORK& _msg)
{
	// check) psocket_peer가 nullptr이면 안 된다.
	CGASSERT_ERROR(_msg.get_source<socket_peer>() != nullptr);

	//// 1) ICGEventObject을 얻는다.(_msg가 Ref를 잡고 있으므로 궅이 object_ptr<>를 사용하지 않는다.)
	//auto psocket_peer = _msg.get_source<socket_peer>();

	//// 2) get buf
	//buffer_view buf_temp = _msg.buf_message + offset(8);

	// return)
	return eRESULT::DONE;
}
