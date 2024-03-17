#include "pch.h"
#if defined(_WIN32)
	#include "Userenv.h"
	#include "Wtsapi32.h"
#elif defined(__linux__)
	#include <sys/wait.h>
	#include <pthread.h>
#else
#endif

server_admin::instance_Iservice::instance_Iservice()
{
	// 1) create observable
	this->pobservable_websocket_peer_overview_info = make_own<observable_websocket_peer_overview_info>("observable_websocket_peer_overview_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_overview_info);
	this->pobservable_websocket_peer_overview_info->m_pservice_entity = this;
	this->pobservable_websocket_peer_factory_info = make_own<observable_websocket_peer_factory_info>("observable_websocket_peer_factory_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_factory_info);
	this->pobservable_websocket_peer_factory_info->m_pservice_entity = this;
	this->pobservable_websocket_peer_executor_info = make_own<observable_websocket_peer_executor_info>("observable_websocket_peer_executor_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_executor_info);
	this->pobservable_websocket_peer_executor_info->m_pservice_entity = this;
	this->pobservable_websocket_peer_network_info = make_own<observable_websocket_peer_network_info>("observable_websocket_peer_network_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_network_info);
	this->pobservable_websocket_peer_network_info->m_pservice_entity = this;
	this->pobservable_websocket_peer_querier_info = make_own<observable_websocket_peer_querier_info>("observable_websocket_peer_querier_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_querier_info);
	this->pobservable_websocket_peer_querier_info->m_pservice_entity = this;
	this->pobservable_websocket_peer_crash_info = make_own<observable_websocket_peer_crash_info>("observable_websocket_peer_crash_info"); // Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_peer_crash_info);
	this->pobservable_websocket_peer_crash_info->m_pservice_entity = this;
}

server_admin::instance_Iservice::~instance_Iservice() noexcept
{
}

void server_admin::instance_Iservice::set_instance_info(server_admin::sINSTANCE_SERVICE&& _instance_service)
{
	scoped_lock(this->m_cs_instance)
	{
		this->m_instance = std::move(_instance_service);
	}
}

void server_admin::instance_Iservice::reset_instance_status_info()
{
}

void server_admin::instance_Iservice::update_instance_info(server_admin::sSERVICE_INSTANCE_STATUS_UPDATE&& _service_info_update)
{
	// 1) update data
	scoped_lock(this->m_cs_instance)
	{
		this->m_instance.status.vector_process_status.push_back(std::move(_service_info_update.process_status));
		this->m_instance.update_factory_info(std::move(_service_info_update.vector_factory_update));
		this->m_instance.update_executor_info(std::move(_service_info_update.vector_executor_update));
		this->m_instance.update_schedulable_info(std::move(_service_info_update.vector_schedulable_update));
		this->m_instance.update_connective_info(std::move(_service_info_update.vector_connective_update));
		this->m_instance.update_binder_info(std::move(_service_info_update.vector_binder_update));
		this->m_instance.update_connective_statistics(std::move(_service_info_update.connective_info));
		this->m_instance.update_traffic_statistics(std::move(_service_info_update.traffic_info));
	}

	// notify) to alll
	this->pobservable_websocket_peer_overview_info->request_update_webpage_time_event();
	this->pobservable_websocket_peer_executor_info->request_update_webpage_executable();
	this->pobservable_websocket_peer_factory_info->request_update_webpage_factory();
	this->pobservable_websocket_peer_executor_info->request_update_webpage_executor();
	this->pobservable_websocket_peer_network_info->request_update_webpage_connective();
	this->pobservable_websocket_peer_network_info->request_update_webpage_binder();
}

void server_admin::instance_Iservice::update_revision_info()
{
	// check)
	RETURN_IF(this->m_instance.info.str_path.empty() == false);

	// 2) URL정보를 읽어서 설정한다.
	if (this->m_instance.info.revision.url.empty() == true)
	{
#ifdef _USE_SVN_CONTROL
		// declare) 
		svn_client_info2_t tempClientInfo;

		// - 정보를 읽는다.
		auto result = server_admin::SVN::GetInfo(info_server_setting.str_path.c_str(), tempClientInfo);

		if (result.IsSuccess())
		{
			// - URL을 읽어넣는다.
			__convert_string(info_server_setting.strURL, tempClientInfo.URL);

			// - Version은 Default로...
			info_server_setting.iRevision = -1;
		}
#endif
	}
}

eOBJECT_STATE server_admin::instance_Iservice::set_instance_state(eOBJECT_STATE _state) noexcept
{
	// declare)
	eOBJECT_STATE state_old;

	scoped_lock(this->m_cs_instance)
	{
		// 1) Old State를 얻어 놓느다.
		state_old = this->m_instance.status.state;

		// check) Old와 동일하면 그냥 끝낸다.
		RETURN_IF(state_old == _state, state_old)

		// 2) 변경한다.
		this->m_instance.status.state = _state;

		// 3) is_preserve_start가 true인지를 확인한다.
		if (_state == eOBJECT_STATE::STOPPED && this->m_is_preserve_start == true)
		{
			// - 먼저 false로 변경한 후...
			this->m_is_preserve_start = false;

			// - start한다.
			this->request_service_start(this->m_str_confugure);
		}
		else if (_state == eOBJECT_STATE::RUNNING)
		{
			this->m_is_preserve_start = false;
		}

		// 4) Setting String을 clear한다.
		this->m_str_confugure.clear();
	}

	// 5) notify message
	service::pgroup_websocket->notify_to_web_controls_INSTANCE_STATUS_UPDATE(this);

	// return) 
	return state_old;
}

void server_admin::instance_Iservice::on_observer_registered(socket_websocket* _pobserver, observer_channel_t /*_ocid*/, const observer_data_t& /*_observer_data*/)
{
	// 1) register mediator
	_pobserver->register_messageable(this);

	// 2) get selected web page
	auto pobservable_webpage = _get_pobservable_webpage(_pobserver->m_webpage_selected);

	// 3) set...
	_pobserver->m_webpage_ptarget = pobservable_webpage;
	_pobserver->send(_pobserver->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE_B, pobservable_webpage->m_webpage_str_target));// "BSCF"
}

uintptr_t server_admin::instance_Iservice::on_observer_unregistering(socket_websocket* _pobserver, observer_channel_t /*_ocid*/)
{
	// 1) register mediator
	_pobserver->unregister_messageable(this);

	// return) 
	return 0;
}

object_ptr<Iobservable_websocket> server_admin::instance_Iservice::_get_pobservable_webpage(char _select) const noexcept
{
	// declare) 
	object_ptr<Iobservable_websocket> pobservable_webpage;

	switch (_select)
	{
	case	'A':
			pobservable_webpage = pobservable_websocket_peer_overview_info;
			break;

	case	'B':
			pobservable_webpage = pobservable_websocket_peer_factory_info;
			break;

	case	'C':
			pobservable_webpage = pobservable_websocket_peer_executor_info;
			break;

	case	'D':
			pobservable_webpage = pobservable_websocket_peer_network_info;
			break;

	case	'E':
			pobservable_webpage = pobservable_websocket_peer_querier_info;
			break;

	case	'F':
			pobservable_webpage = pobservable_websocket_peer_crash_info;
			break;

	default:
			pobservable_webpage = pobservable_websocket_peer_overview_info;
			break;
	};

	// return)
	return pobservable_webpage;
}

result_code server_admin::instance_Iservice::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		// - soekct
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE,	on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO,		on_message_SERVER_ADMIN_INSTANCE_UPDATE,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_LIST_UPDATE,	on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,		on_message_SERVER_TIME_EVENT_EVENT_REGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,	on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,							on_message_SYSTEM_LOG,								sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::DUMP,									on_message_DUMP,									sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_LIST_UPDATE,			on_message_FACTORY_FACTORY_LIST_UPDATE,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_REGISTER,				on_message_FACTORY_FACTORY_REGISTER,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,				on_message_FACTORY_FACTORY_UNREGISTER,				sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_LIST_UPDATE,					on_message_EXECUTOR_LIST_UPDATE,					sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_REGISTER,						on_message_EXECUTOR_REGISTER,						sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::EXECUTOR_UNREGISTER,						on_message_EXECUTOR_UNREGISTER,						sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_LIST_UPDATE,	on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,	on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,	on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_LIST_UPDATE,		on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,		on_message_NETWORK_SOCKET_BINDER_REGISTER,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,		on_message_NETWORK_SOCKET_BINDER_UNREGISTER,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE,		on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE,		sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP

		// - websocket
		ON_CGMESSAGE(eMESSAGE::SYSTEM::WEBSOCKET,	on_message_http_websocket_REQUEST, sMESSAGE_HTTP_WEBSOCKET);
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::BYPASS;
}

result_code server_admin::instance_Iservice::on_message_http_websocket_REQUEST(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) websocket message 
	sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 3) get message head
	buffer_view buf_temp = msg.buf_message;

	auto message_type = buf_temp.extract_text<char>(size(4));
	auto pmember = msg.get_source<socket_websocket>();

	if (message_type[0] == 'B')
	{
		switch(message_type[1])
		{
		case 'A':
				{
					// - get observable
					auto pobservable_webpage = _get_pobservable_webpage(message_type[2]);

					// check) 
					RETURN_IF(pobservable_webpage.empty(), eRESULT::DONE);

					// check) same webpage?
					RETURN_IF(pmember->is_observable(pobservable_webpage, OCID_FRAME3), eRESULT::DONE);

					// - unregister old observable
					pmember->unsubscribe(OCID_FRAME3);

					// - set..
					pmember->m_webpage_ptarget = pobservable_webpage;
					pmember->m_webpage_selected = pobservable_webpage->m_webpage_index;

					// - request 
					pmember->send(pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE_B, pobservable_webpage->m_webpage_str_target)); // "BSCF"
				}
				break;

		case	'B':
				{
					// declare) 
					auto pobservable_webpage = std::move(pmember->m_webpage_ptarget);

					// check)
					RETURN_IF(pobservable_webpage.empty(), eRESULT::DONE);

					// - register member
					pobservable_webpage->subscribe(pmember, OCID_FRAME3);
				}
				break;
		};
	}

	// return)
	return eRESULT::BYPASS;
}

result_code server_admin::instance_Iservice::on_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(sMESSAGE_NETWORK& _msg)
{
	// check) 
	CGASSERT_ERROR(_msg.buf_message.size() == (8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE) + sizeof(eOBJECT_STATE)));

	// check) 
	THROW_IF(_msg.buf_message.size() != (8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE) + sizeof(eOBJECT_STATE)), throwable::failure());

	// 1) find service and state
	auto iid = _msg.buf_message.front<server_admin::iid_t>(8);

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		auto state_new = _msg.buf_message.front<eOBJECT_STATE>(8 + sizeof(server_admin::iid_t));
		auto state_old = _msg.buf_message.front<eOBJECT_STATE>(8 + sizeof(server_admin::iid_t) + sizeof(eOBJECT_STATE));

		// 3) update service_instace_stage update
		this->set_instance_state(state_new);

		// 3) notify message
		service::pgroup_client->send(_msg.buf_message);

		// log) 
		LOG_INFO << "  - INSTANCE('"sv << this->get_name() << "') state changed ["sv << to_string<char>(state_old) << "] -> ["sv << to_string<char>(state_new) << "]"sv;
	}
	catch (...)
	{
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_SERVER_ADMIN_INSTANCE_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 5) extract data
		auto temp_info = buf_temp.extract<server_admin::sSERVICE_INSTANCE_STATUS_UPDATE>();

		// 6) update
		this->update_instance_info(std::move(temp_info));

		// 7) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_SERVER_TIME_EVENT_EVENT_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_time_event_info(buf_temp.extract<std::vector<server_admin::time_event_update_snap>>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_overview_info->request_update_webpage_time_event();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.register_time_event_info(buf_temp.extract<server_admin::time_event_snap>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_overview_info->request_update_webpage_time_event();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.unregister_time_event_info(buf_temp.extract<uint64_t>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_overview_info->request_update_webpage_time_event();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_FACTORY_FACTORY_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 4) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_factory_info(buf_temp.extract<std::vector<server_admin::factory_update_snap>>());
		}

		// 5) notify to web
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executable();
		this->pobservable_websocket_peer_factory_info->request_update_webpage_factory();

		// 6) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_FACTORY_FACTORY_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.register_factory_info(buf_temp.extract<server_admin::factory_snap>());
		}

		// 4) nofity
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executable();
		this->pobservable_websocket_peer_factory_info->request_update_webpage_factory();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_FACTORY_FACTORY_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.unregister_factory_info(buf_temp.extract<uint64_t>());
		}

		// 4) notify to web 
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executable();
		this->pobservable_websocket_peer_factory_info->request_update_webpage_factory();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_EXECUTOR_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_executor_info(buf_temp.extract<std::vector<server_admin::executor_update_snap>>());
		}

		// 4) notify web 
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executor();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_EXECUTOR_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 4) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.register_executor_info(buf_temp.extract<server_admin::executor_snap>());
		}

		// 5) notify to web
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executor();

		// 6) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}
result_code server_admin::instance_Iservice::on_message_EXECUTOR_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.unregister_executor_info(buf_temp.extract<uint64_t>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_executor_info->request_update_webpage_executor();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_CONNECTIVE_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 4) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_connective_info(buf_temp.extract<std::vector<server_admin::connective_info_update_snap>>());
		}

		// 5) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_connective();

		// 6) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 4) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.register_connective_info(buf_temp.extract<server_admin::connective_info_snap>());
		}

		// 5) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_connective();

		// 6) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}
result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 4) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.unregister_connective_info(buf_temp.extract<uint64_t>());
		}

		// 5) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_connective();

		// 6) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_BINDER_LIST_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_binder_info(std::move(buf_temp.extract<std::vector<server_admin::socket_info_update_snap>>()));
		}

		// 4) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_binder();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.register_binder_info(std::move(buf_temp.extract<server_admin::socket_info_snap>()));
		}

		// 4) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_binder();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.unregister_binder_info(buf_temp.extract<uint64_t>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_binder();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE_NETWORK& _msg)
{
	// 1) 
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) get instance
	auto iid = buf_temp.extract<server_admin::iid_t>();

	// check)
	CGASSERT_ERROR(iid == this->get_iid());

	// check)
	RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	try
	{
		// 3) set data
		scoped_lock(this->m_cs_instance)
		{
			this->m_instance.update_connective_statistics(buf_temp.extract<statistics::unit_connective_snap>());
		}

		// 4) notify to web
		this->pobservable_websocket_peer_network_info->request_update_webpage_connective();

		// 5) send to admin server
		service::pgroup_client->send(_msg.buf_message);
	}
	catch (...)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) buffer for extracting
	//buffer_view buf_temp = _msg.buf_message + offset(8);

	//// 2) get iid
	//auto iid = buf_temp.extract<server_admin::iid_t>();

	//// check)
	//CGASSERT_ERROR(iid == this->get_iid());

	//// check)
	//RETURN_IF(iid != this->get_iid(), eRESULT::BYPASS);

	//// 3) extract log_record
	//auto list_log_record = buf_temp.extract<circular_list<object_ptr<LOG_RECORD>>>();

	//// 5) notify to web (log를 놓지는 경우가 없도록 하기 위해 여기에..)
	//this->pobservable_websocket_peer_overview_info->add_log_records(list_log_record);

	//// 6) set data
	//scoped_lock(this->m_cs_instance)
	//{
	//	this->m_instance.add_log_records(std::move(list_log_record));
	//}

	//// 7) send to admin server
	//service::pgroup_client->send(_msg.buf_message);

	// return)
	return eRESULT::DONE;
}

result_code server_admin::instance_Iservice::on_message_DUMP(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}






















std::string_view GetSVNActionString(int _Notify)
{
	// declare) 
	std::string_view	strAction;

	// 1) Action에 따라...
	switch (_Notify)
	{
#ifdef _USE_SVN_CONTROL
	// Adding a path to revision control.
	case	svn_wc_notify_add:
			strAction = "Add";
			break;

	// Copying a versioned path.
	case	svn_wc_notify_copy:
			strAction = "Copy";
			break;

	// Deleting a versioned path.
	case	svn_wc_notify_delete:
			strAction = "Delete";
			break;

	// Restoring a missing path from the pristine text-base.
	case	svn_wc_notify_restore:
			strAction = "Restore";
			break;

	// Reverting a modified path.
	case	svn_wc_notify_revert:
			strAction = "Revert";
			break;

	// A revert operation has failed.
	case	svn_wc_notify_failed_revert:
			strAction = "Failed revert";
			break;

	// Resolving a conflict.
	case	svn_wc_notify_resolved:
			strAction = "Resolved";
			break;

	// Skipping a path.
	case	svn_wc_notify_skip:
			strAction = "Skip";
			break;

	// Got a delete in an update.
	case	svn_wc_notify_update_delete:
			strAction = "Update delete";
			break;

	// Got an add in an update.
	case	svn_wc_notify_update_add:
			strAction = "Update add";
			break;

	// Got any other action in an update.
	case	svn_wc_notify_update_update:
			strAction = "Update update";
			break;

	// The last notification in an update (including updates of externals).
	case	svn_wc_notify_update_completed:
			strAction = "Update completed";
			break;

	// Updating an external module.
	case	svn_wc_notify_update_external:
			strAction = "Update external";
			break;

	// The last notification in a status (including status on externals).
	case	svn_wc_notify_status_completed:
			strAction = "Status completed";
			break;

	// Running status on an external module.
	case	svn_wc_notify_status_external:
			strAction = "Status external";
			break;

	// Committing a modification.
	case	svn_wc_notify_commit_modified:
			strAction = "Commit modified";
			break;

	// Committing an addition.
	case	svn_wc_notify_commit_added:
			strAction = "Commit added";
			break;

	// Committing a deletion.
	case	svn_wc_notify_commit_deleted:
			strAction = "Commit deleted";
			break;

	// Committing a replacement.
	case	svn_wc_notify_commit_replaced:
			strAction = "Commit replaced";
			break;

	// Transmitting post-fix text-delta data for a file.
	case	svn_wc_notify_commit_postfix_txdelta:
			strAction = "Commit postfix txdelta";
			break;

	// Processed a single revision's blame.
	case	svn_wc_notify_blame_revision:
			strAction = "Blame revision";
			break;

	// Locking a path. @since New in 1.2.
	case	svn_wc_notify_locked:
			strAction = "Locked";
			break;

	// Unlocking a path. @since New in 1.2.
	case	svn_wc_notify_unlocked:
			strAction = "Unlocked";
			break;

	// Failed to lock a path. @since New in 1.2.
	case	svn_wc_notify_failed_lock:
			strAction = "Failed lock";
			break;

	// Failed to unlock a path. @since New in 1.2.
	case	svn_wc_notify_failed_unlock:
			strAction = "Failed unlock";
			break;

	// Tried adding a path that already exists. @since New in 1.5.
	case	svn_wc_notify_exists:
			strAction = "Exists";
			break;

	// Changelist name set. @since New in 1.5.
	case	svn_wc_notify_changelist_set:
			strAction = "Changelist set";
			break;

	// Changelist name cleared. @since New in 1.5.
	case	svn_wc_notify_changelist_clear:
			strAction = "Changelist clear";
			break;

	// Warn user that a path has moved from one changelist to another. @since New in 1.5. @deprecated As of 1.7, separate clear and set notifications are sent.
	case	svn_wc_notify_changelist_moved:
			strAction = "Changelist moved";
			break;

	// A merge operation (to path) has begun.  See #svn_wc_notify_t.merge_range. @since New in 1.5.
	case	svn_wc_notify_merge_begin:
			strAction = "Merge begin";
			break;

	// A merge operation (to path) from a foreign repository has begun. See #svn_wc_notify_t.merge_range.  @since New in 1.5.
	case	svn_wc_notify_foreign_merge_begin:
			strAction = "Foreign merge begin";
			break;

	// Replace notification. @since New in 1.5.
	case	svn_wc_notify_update_replace:
			strAction = "Update replace";
			break;

	// Property added. @since New in 1.6.
	case	svn_wc_notify_property_added:
			strAction = "Property added";
			break;

	// Property updated. @since New in 1.6.
	case	svn_wc_notify_property_modified:
			strAction = "Property modified";
			break;

	// Property deleted. @since New in 1.6.
	case	svn_wc_notify_property_deleted:
			strAction = "Property deleted";
			break;

	// Nonexistent property deleted. @since New in 1.6.
	case	svn_wc_notify_property_deleted_nonexistent:
			strAction = "Property deleted nonexistent";
			break;

	// Revprop set. @since New in 1.6.
	case	svn_wc_notify_revprop_set:
			strAction = "Revprop set";
			break;

	// Revprop deleted. @since New in 1.6.
	case	svn_wc_notify_revprop_deleted:
			strAction = "Revprop deleted";
			break;

	// The last notification in a merge. @since New in 1.6.
	case	svn_wc_notify_merge_completed:
			strAction = "Merge completed";
			break;

	// The path is a tree-conflict victim of the intended action (*not* a persistent tree-conflict from an earlier operation, but *this* operation caused the tree-conflict). @since New in 1.6.
	case	svn_wc_notify_tree_conflict:
			strAction = "Tree conflict";
			break;

	// The path is a subdirectory referenced in an externals definition which is unable to be operated on.  @since New in 1.6.
	case	svn_wc_notify_failed_external:
			strAction = "Failed external";
			break;

	// Starting an update operation.  @since New in 1.7.
	case	svn_wc_notify_update_started:
			strAction = "Update started";
			break;

	// An update tried to add a file or directory at a path where a separate working copy was found.  @since New in 1.7.
	case	svn_wc_notify_update_skip_obstruction:
			strAction = "Update skip obstruction";
			break;

	// An explicit update tried to update a file or directory that doesn't live in the repository and can't be brought in. @since New in 1.7.
	case	svn_wc_notify_update_skip_working_only:
			strAction = "Update skip working only";
			break;

	// An update tried to update a file or directory to which access could not be obtained. @since New in 1.7.
	case	svn_wc_notify_update_skip_access_denied:
			strAction = "Update skip access denied";
			break;

	// An update operation removed an external working copy. @since New in 1.7.
	case	svn_wc_notify_update_external_removed:
			strAction = "Update external removed";
			break;

	// A node below an existing node was added during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_add:
			strAction = "Update shadowed add";
			break;

	// A node below an existing node was updated during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_update:
			strAction = "Update shadowed update";
			break;

	// A node below an existing node was deleted during update. @since New in 1.7.
	case	svn_wc_notify_update_shadowed_delete:
			strAction = "Update shadowed delete";
			break;

	// The mergeinfo on path was updated.  @since New in 1.7.
	case	svn_wc_notify_merge_record_info:
			strAction = "Merge record info";
			break;

	// A working copy directory was upgraded to the latest format. @since New in 1.7.
	case	svn_wc_notify_upgraded_path:
			strAction = "Upgraded path";
			break;

	// Mergeinfo describing a merge was recorded. @since New in 1.7.
	case	svn_wc_notify_merge_record_info_begin:
			strAction = "Merge record info begin";
			break;

	// Mergeinfo was removed due to elision. @since New in 1.7.
	case	svn_wc_notify_merge_elide_info:
			strAction = "Merge elide info";
			break;

	// A file in the working copy was patched. @since New in 1.7.
	case	svn_wc_notify_patch:
			strAction = "Patch";
			break;

	// A hunk from a patch was applied. @since New in 1.7.
	case	svn_wc_notify_patch_applied_hunk:
			strAction = "Patch applied hunk";
			break;

	// A hunk from a patch was rejected. @since New in 1.7.
	case	svn_wc_notify_patch_rejected_hunk:
			strAction = "Patch rejected hunk";
			break;

	// A hunk from a patch was found to already be applied. @since New in 1.7.
	case	svn_wc_notify_patch_hunk_already_applied:
			strAction = "Patch hunk aleady applied";
			break;

	// Committing a non-overwriting copy (path is the target of the copy, not the source). @since New in 1.7.
	case	svn_wc_notify_commit_copied:
			strAction = "Commit copied";
			break;

	// Committing an overwriting (replace) copy (path is the target of the copy, not the source). @since New in 1.7.
	case	svn_wc_notify_commit_copied_replaced:
			strAction = "Commit copied replaced";
			break;

	// The server has instructed the client to follow a URL redirection. @since New in 1.7.
	case	svn_wc_notify_url_redirect:
			strAction = "URL redirect";
			break;

	// The operation was attempted on a path which doesn't exist. @since New in 1.7.
	case	svn_wc_notify_path_nonexistent:
			strAction = "Pathnonexistent";
			break;

	// Removing a path by excluding it. @since New in 1.7.
	case	svn_wc_notify_exclude:
			strAction = "Exclude";
			break;

	// Operation failed because the node remains in conflict @since New in 1.7.
	case	svn_wc_notify_failed_conflict:
			strAction = "Failed Conflict";
			break;

	// Operation failed because an added node is missing @since New in 1.7.
	case	svn_wc_notify_failed_missing:
			strAction = "Failed missing";
			break;

	// Operation failed because a node is out of date @since New in 1.7.
	case	svn_wc_notify_failed_out_of_date:
			strAction = "Failed out of date";
			break;

	// Operation failed because an added parent is not selected @since New in 1.7.
	case	svn_wc_notify_failed_no_parent:
			strAction = "Failed no parent";
			break;

	// Operation failed because a node is locked by another user and/or working copy.  @since New in 1.7.
	case	svn_wc_notify_failed_locked:
			strAction = "Failed locked";
			break;

	// Operation failed because the operation was forbidden by the server. @since New in 1.7.
	case	svn_wc_notify_failed_forbidden_by_server:
			strAction = "Failed forbidden by server";
			break;

	// The operation skipped the path because it was conflicted. @since New in 1.7.
	case	svn_wc_notify_skip_conflicted:
			strAction = "Skip conflicted";
			break;

	// Just the lock on a file was removed during update. @since New in 1.8.
	case	svn_wc_notify_update_broken_lock:
			strAction = "Update broken lock";
			break;

	// Operation failed because a node is obstructed. @since New in 1.8.
	case	svn_wc_notify_failed_obstruction:
			strAction = "Failed obstruction";
			break;

	// Conflict resolver is starting. This can be used by clients to detect when to display conflict summary information, for example. @since New in 1.8.
	case	svn_wc_notify_conflict_resolver_starting:
			strAction = "Conflict resolver starting";
			break;

	// Conflict resolver is done. This can be used by clients to detect when to display conflict summary information, for example. @since New in 1.8.
	case	svn_wc_notify_conflict_resolver_done:
			strAction = "Conflict resolver done";
			break;

	// The current operation left local changes of something that was deleted The changes are available on (and below) the notified path @since New in 1.8. svn_wc_notify_left_local_modifications, A copy from a foreign repository has started @since New in 1.8.
	case	svn_wc_notify_foreign_copy_begin:
			strAction = "Foreign copy begin";
			break;

	// A move in the working copy has been broken, i.e. degraded into a copy + delete. The notified path is the move source (the deleted path). ### TODO: Provide path to move destination as well? @since New in 1.8.
	case	svn_wc_notify_move_broken:
			strAction = "Move broken";
			break;

	// Running cleanup on an external module. @since New in 1.9. */
	case	svn_wc_notify_cleanup_external:
			strAction = "Cleanup external";
			break;

	// The operation failed because the operation (E.g. commit) is only valid if the operation includes this path. @since New in 1.9. */
	case	svn_wc_notify_failed_requires_target:
			strAction = "Failed requires target";
			break;

	//  info on an external module. @since New in 1.9. */
	case	svn_wc_notify_info_external:
			strAction = "Info external";
			break;

	// Finalizing commit.@since New in 1.9. */
	case	svn_wc_notify_commit_finalizing:
			strAction = "Commit finalizing";
			break;
#endif
	default:
			strAction = "-";
			break;
	};

	// return) 
	return	strAction;
}

#ifdef _USE_SVN_CONTROL
void server_admin::instance_Iservice::on_svn_notify(server_admin::Isvn_command * /*_baton*/, const svn_wc_notify_t * _notify, apr_pool_t * /*_pool*/)
{
	// check)
	RETURN_IF(_notify == nullptr);
	
	// log)
	LOG_INFO << "  - ["sv << GetSVNActionString(_notify->action) << "] "sv << _notify->path;
}

void server_admin::instance_Iservice::on_svn_procgress(apr_off_t /*_progress*/, apr_off_t /*_total*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_pool*/)
{
	// 제거
}

svn_error_t * server_admin::instance_Iservice::on_svn_log(const char ** /*_log_msg*/, const char ** /*_tmp_file*/, const apr_array_header_t * /*_commit_items*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_pool*/)
{
	// 제거
	return nullptr;
}

svn_error_t * server_admin::instance_Iservice::on_svn_conflict_resolver(svn_wc_conflict_result_t ** /*_result*/, const svn_wc_conflict_description2_t * /*_description*/, server_admin::Isvn_command * /*_baton*/, apr_pool_t * /*_result_pool*/, apr_pool_t * /*_scratch_pool*/)
{
	// 제거
	return nullptr;
}

svn_error_t * server_admin::instance_Iservice::on_svn_cancel(server_admin::Isvn_command * /*_cancel_baton*/)
{
	// 제거
	return nullptr;
}

void server_admin::instance_Iservice::on_svn_complete(server_admin::Isvn_command * /*_cancel_baton*/, const SVN_RESULT& /*_result*/)
{
	// 제거
}
#endif

