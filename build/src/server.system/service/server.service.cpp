#include "pch.h"
#include <iostream>

#if defined(_WIN32)
	#include <conio.h>
#endif

using namespace CGDK;


server_system::Xservice::Xservice() :
	server_system::Nservice()
{
}

server_system::Xservice::Xservice(const std::string_view _name) :
	server_system::Nservice(_name)
{
}

server_system::Xservice::~Xservice()
{
}

void server_system::Xservice::set_host_instance_manager(own_ptr<server_system::host::manager>&& _phost_instance_manager)
{
	// check)
	CGASSERT_ERROR(_phost_instance_manager.exist())

		// check)
		THROW_IF(_phost_instance_manager.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "host_instance is nullptr"sv));

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) register messageable
		_phost_instance_manager->register_messageable(this);

		// 2) set instance manager
		this->m_phost_instance_manager = std::move(_phost_instance_manager);
	}
}

void server_system::Xservice::reset_host_instance_manager() noexcept
{
	// declare) 
	own_ptr<server_system::host::manager> phost_instance_manager;

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) set instance manager
		phost_instance_manager = std::move(this->m_phost_instance_manager);

		// check)
		RETURN_IF(phost_instance_manager.empty(), );

		// 2) register messageable
		phost_instance_manager->unregister_messageable(this);
	}

	// 3) unregister host instance manager
	phost_instance_manager->destroy();
}

void server_system::Xservice::set_service_instance_manager(own_ptr<server_system::service::manager>&& _pservice_instance_manager)
{
	// check)
	CGASSERT_ERROR(_pservice_instance_manager.exist())

	// check)
	THROW_IF(_pservice_instance_manager.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT, 0, "host_instance is nullptr"sv));

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) register messageable
		_pservice_instance_manager->register_messageable(this);

		// 2) set instance manager
		this->m_pservice_instance_manager = std::move(_pservice_instance_manager);
	}
}

void server_system::Xservice::reset_service_instance_manager() noexcept
{
	// declare) 
	own_ptr<server_system::service::manager> pservice_instance_manager;

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) set instance manager
		pservice_instance_manager = std::move(this->m_pservice_instance_manager);

		// check)
		RETURN_IF(pservice_instance_manager.empty(), );

		// 2) register messageable
		pservice_instance_manager->unregister_messageable(this);
	}

	// 3) unregister service instance manager
	pservice_instance_manager->destroy();
}

result_code server_system::Xservice::process_message(sMESSAGE& _msg)
{
	// 1) message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING,	on_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE,		on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE,	on_message_SERVER_ADMIN_REQUEST_START_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE,		on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE,	on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE,	on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE,		sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE,	on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SYSTEM::LOG_,							on_message_SYSTEM_LOG,								sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return)
	return eRESULT::BYPASS;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(sMESSAGE_NETWORK& _msg)
{
	// check) shared_buffer의 data_가 nullptr면 안 된다!!!
	CGASSERT(_msg.buf_message.data() != nullptr, int(0));

	// declare)
	shared_buffer buf_read = _msg.buf_message + offset(8);

	// 1) iid
	auto iid = buf_read.extract<server_system::iid_t>();
	auto host_iid = buf_read.extract<server_system::iid_t>();

	// 2) sSERVICE::sSETTING Info를 설정한다.
	auto temp_service_setting = buf_read.extract<server_system::sSERVICE::sSETTING>();

	// trace) 
	LOG_INFO << "> SERVER SETTING['"sv << temp_service_setting.name << "'] received"sv;
	LOG_INFO << "  - iid: "sv << iid;
	LOG_INFO << "  - host_id: "sv << host_iid;
	LOG_INFO << "  - nickmake: '"sv << temp_service_setting.name << "'";
	LOG_INFO << "  - rank: "sv << temp_service_setting.rank.to_string();

	server_system::iid_t setting_iid;
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// declare)
		auto pservice_instance = this->m_pservice_instance.get();

		// - get setting_iid
		setting_iid = pservice_instance->get_iid();

		// - server setting Info를 설정한다.
		pservice_instance->info.setting = std::move(temp_service_setting);
		pservice_instance->info.info.host_iid = host_iid;
	}

	if (iid != setting_iid)
	{
		LOG_WARNING << "  (!) iid changed! ["sv << iid << "] -> ["sv << setting_iid << "]"sv;
	}

	// 3) Service정보를 Update한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_STATUS_UPDATE, reinterpret_cast<uintptr_t>(nullptr)));

	// return) 
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Notify한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE));

	// 2) execute 'initialize' command
	this->initialize(); // 여기(왜 initialize()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& _msg)
{
	// 1) get message
	auto buf_recv = _msg.buf_message + offset(8);

	// 2) 
	auto iid = buf_recv.extract<server_system::iid_t>();

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service instance
		auto pservice_instance = this->m_pservice_instance.get_object_ptr();

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::BYPASS);

		// check)
		RETURN_IF(pservice_instance->info.setting.iid != iid, eRESULT::BYPASS);
	}

	// 주의) 여기서 destory()를 직접 호출하면 된다.
	//       - connect_requestable socket의 송수신 중에 destroy를 호출할 경우 종료처리가 꼬일 수 있다.
	//       - 왜냐면 socket의 on_message 과정에서 소멸을 기다리는 것이 되므로 데드락에 걸리는 셈이다.
	//       - 따라서 꼭 post를 사용해서 종료처리르 해야 한다.

	// 3) Notify한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE));

	// 4) hold
	object_ptr<Xservice> pself(this);

	// 5) execute 'destroy' command
	system_executor::post(make_executable([=] { pself->destroy(); }));

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& _msg)
{
	// 1) get message
	auto buf_recv = _msg.buf_message + offset(8);

	// 2) 
	auto iid = buf_recv.extract<server_system::iid_t>();
	auto context_start = buf_recv.extract<context>();

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service instance
		auto pservice_instance = this->m_pservice_instance.get_object_ptr();

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::BYPASS);

		// check)
		RETURN_IF(pservice_instance->info.setting.iid != iid, eRESULT::BYPASS);

		// - set context_start
		if(context_start.is_valid())
		{
			pservice_instance->info.setting.context_start = context_start;
		}
	}

	// 3) Notify한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE));

	// 4) execute 'start' command
	this->start(context_start); // 여기(왜 start()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& _msg)
{
	// 1) get message
	auto buf_recv = _msg.buf_message + offset(8);

	// 2) 
	auto iid = buf_recv.extract<server_system::iid_t>();

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service instance
		auto pservice_instance = this->m_pservice_instance.get_object_ptr();

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::BYPASS);

		// check)
		RETURN_IF(pservice_instance->info.setting.iid != iid, eRESULT::BYPASS);
	}

	// 3) notify to service
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE));

	// 4) execute 'stop' command
	this->stop();

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(sMESSAGE_NETWORK& _msg)
{
	// 1) get message
	auto buf_recv = _msg.buf_message + offset(8);

	// 2) 
	auto iid = buf_recv.extract<server_system::iid_t>();

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service instance
		auto pservice_instance = this->m_pservice_instance.get_object_ptr();

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::BYPASS);

		// check)
		RETURN_IF(pservice_instance->info.setting.iid != iid, eRESULT::BYPASS);
	}

	// 3) Notify한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE));

	// 4) execute 'pause' command
	this->pause(); // 여기(왜 pause()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(sMESSAGE_NETWORK& _msg)
{
	// 1) get message
	auto buf_recv = _msg.buf_message + offset(8);

	// 2) 
	auto iid = buf_recv.extract<server_system::iid_t>();

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service instance
		auto pservice_instance = this->m_pservice_instance.get_object_ptr();

		// check) 
		RETURN_IF(pservice_instance.empty(), eRESULT::BYPASS);

		// check)
		RETURN_IF(pservice_instance->info.setting.iid != iid, eRESULT::BYPASS);
	}

	// 3) Notify한다.
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE));

	// 4) execute 'resume' command
	this->resume(); // 여기(왜 continue()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::Xservice::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
{
	// 1) get logger
	auto plogger = get_default_logger();

	// check)
	RETURN_IF(plogger.empty(), eRESULT::DONE);

	// 1) buffer for extracting
	buffer_view buf_temp = _msg.buf_message + offset(8);

	// 2) extract
	auto vector_log_record = buf_temp.extract<std::vector<object_ptr<LOG_RECORD>>>();

	// 3) set data
	for (auto& iter : vector_log_record)
		plogger->trace(std::move(iter));

	// return)
	return eRESULT::DONE;
}

void server_system::Xservice::on_server_certified()
{
	// 1) get service_instance
	auto pservice_instance = this->get_service_instance();

	// check)
	RETURN_IF(pservice_instance.empty(), );

	// 2) notify update...
	this->send_to_admin_server(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(pservice_instance->info.setting.iid, this->get_object_state(), eOBJECT_STATE::INITIALIZING_PENDING, &pservice_instance->info));

	// notify) 
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::CERTIFIED, 0, 0));
}

object_ptr<server_system::socket::client> server_system::Xservice::process_prepare_socket_client(context& _context)
{
	// 1) alloc socket client
	return this->process_alloc_socket_client(_context);
}

void server_system::Xservice::process_close_socket_client() noexcept
{
	// declare) 
	object_ptr<server_system::socket::client> psocket_to_admin;

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// check)
		RETURN_IF(this->m_psocket_to_admin.empty(), );

		// 1) get socket_to_admin
		psocket_to_admin = std::move(this->m_psocket_to_admin);
	}

	// 2) remove old socket
	psocket_to_admin->unregister_messageable(this);
	psocket_to_admin->disable_reconnect();
	psocket_to_admin->closesocket();
}

void server_system::Xservice::process_socket_client_certified(server_system::socket::client* _psocket_to_admin, std::map <server_system::iid_t, server_system::sHOST >&& _map_host_info_confirmed, std::map<server_system::iid_t, server_system::sSERVICE>&& _map_service_info_confirmed)
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// 1) register messageable
		_psocket_to_admin->register_messageable(this);

		// 2) set socket 
		this->m_psocket_to_admin = _psocket_to_admin;

		// 3) get host instance
		auto phost_instance_manager = this->m_phost_instance_manager.get_object_ptr();

		if (phost_instance_manager.exist())
		{
			for (auto& iter : _map_host_info_confirmed)
			{
				// - find host instance
				auto phost_instance = phost_instance_manager->find_instance(iter.first);

				// check)
				CONTINUE_IF(phost_instance.empty());

				// - get pre ids
				auto iid_pre = phost_instance->info.setting.iid;

				// - update service instance info
				phost_instance->info = std::move(iter.second);

				// trace) 
				LOG_INFO << "   ! host instance(" << phost_instance->info.setting.iid <<") updated";

				if (phost_instance->info.setting.iid != iid_pre)
					LOG_INFO << "     = iid: " << to_string(iid_pre) << " -> " << to_string(phost_instance->info.setting.iid);
			}
		}

		// 4) get service instance
		auto pservice_instance_manager = this->m_pservice_instance_manager.get_object_ptr();

		if (pservice_instance_manager.exist())
		{
			for (auto& iter : _map_service_info_confirmed)
			{
				// - find service instance
				auto pservice_instance = pservice_instance_manager->find_instance(iter.first);

				// check)
				CONTINUE_IF(pservice_instance.empty());
				
				// - get pre ids
				auto iid_pre = pservice_instance->info.setting.iid;
				auto rank_pre = pservice_instance->info.setting.rank;

				// - update service instance info
				pservice_instance->info = std::move(iter.second);

				LOG_INFO << "   ! service instance(" << pservice_instance->info.setting.iid << ") updated";

				if (pservice_instance->info.setting.iid != iid_pre)
					LOG_INFO << "     = iid: " << to_string(iid_pre) << " -> " << to_string(pservice_instance->info.setting.iid);

				rank_pre.to_string();
				if (pservice_instance->info.setting.rank != rank_pre)
					LOG_INFO << "     = rank: " << rank_pre.to_string() << " -> " << pservice_instance->info.setting.rank.to_string();
			}
		}
	}

	// 5) call 'on_server_certified'
	this->on_server_certified();
}

object_ptr<server_system::socket::client> server_system::Xservice::process_alloc_socket_client(context& /*_context*/)
{
	return make_object<server_system::socket::client>();
}

result_code server_system::Xservice::_process_initializing(context& _context)
{
	// 1) 'service' context
	auto& context_service = _context.find("service");

	// 2) create service instance
	this->_process_initializing_service_instance(context_service);

	// 3) on_initialize_setting
	this->on_initialize_setting(_context);

	// 4) store context
	SCOPED_LOCK(this->m_lockable_setting)
	{
		if (this->m_pservice_instance.exist())
		{
			this->m_pservice_instance->info.setting.context_initialize = _context;
		}
	}

	// check) 
	if(context_service.is_valid())
	{
		// - initializing setting
		this->_process_initializing_service_setting(context_service);

		// - initializeing instance manager
		this->_process_initializing_instance_manager(context_service);

		// - initializing logger
		this->_process_initializing_loggers(context_service);
	}

	// 5) pre initializing
	this->_process_initializing_pre(_context);

	// 6) on_initializing 함수를 호출한다.
	this->on_initializing(_context);

	// 7) pre initializing
	this->_process_initializing_post(_context);

	// 8) default executor 
	system_executor::initialize_instance(_context);

	// 9) socket_admin을 시작한다.
	this->_process_initializing_socket_to_admin(_context);

	// return)
	return eRESULT::SUCCESS;
}

void server_system::Xservice::_process_initializing_instance_manager(context& _context)
{
	// declare)
	bool is_create_host_instance_manager = false;
	object_ptr<server_system::host::manager> phost_instance_manager;
	bool is_create_service_instance_manager = false;
	object_ptr<server_system::service::manager> pservice_instance_manager;

	try
	{
		// 1) get or make instance manager
		SCOPED_LOCK(this->m_lockable_setting)
		{
			// - make host instance
			if (this->m_phost_instance_manager.empty())
			{
				// - create 
				this->set_host_instance_manager(this->process_alloc_host_instance_manager(_context));

				// - set flag
				is_create_host_instance_manager = true;
			}
			phost_instance_manager = this->m_phost_instance_manager.get_object_ptr();

			// - make service instance
			if (this->m_pservice_instance_manager.empty())
			{
				// - set service instance manager
				this->set_service_instance_manager(this->process_alloc_service_instance_manager(_context));

				// - set flag
				is_create_service_instance_manager = true;
			}
			pservice_instance_manager = this->m_pservice_instance_manager.get_object_ptr();
		}

		// check) 
		CGASSERT_ERROR(pservice_instance_manager.exist());

		// check) 
		CGASSERT_ERROR(phost_instance_manager.exist());

		// 2) initialize host instance manager
		phost_instance_manager->initialize(_context);

		try
		{
			// 3) initialize services instance manager
			pservice_instance_manager->initialize(_context);

			// 4) register local host instance
			if (this->m_phost_instance.exist())
			{
				phost_instance_manager->register_instance(this->m_phost_instance);
			}
			auto& pservice_instance = this->m_pservice_instance;

			// 5) register local service instance
			if (pservice_instance.exist())
			{
				pservice_instance_manager->register_instance(pservice_instance.get_object_ptr());
			}
		}
		catch (...)
		{
			if (this->m_pservice_instance.exist())
			{
				pservice_instance_manager->unregister_instance(this->m_pservice_instance->get_iid());
			}

			if (this->m_phost_instance.exist())
			{
				phost_instance_manager->unregister_instance(this->m_phost_instance->get_iid());
			}

			// - destroy host instance manager
			phost_instance_manager->destroy();

			// reraise)
			throw;
		}
	}
	catch (...)
	{
		// - rollback (service instance manager)
		if (is_create_service_instance_manager)
		{
			this->reset_service_instance_manager();
		}

		// - rollback (host instance manager)
		if (is_create_host_instance_manager)
		{
			this->reset_host_instance_manager();
		}

		// reraise)
		throw;
	}
}

void server_system::Xservice::_process_initializing_socket_to_admin(context& _context)
{
	// 1) 'service' context
	auto& context_service = _context.find("service");

	// check) 
	RETURN_IF(context_service.is_invalid(), );

	// 2) get 'socket_to_admin' context
	auto& context_socket_admin = context_service.find("socket_to_admin"sv);

	// check)
	if(context_socket_admin.is_invalid())
	{
		LOG_WARNING << "   (!) 'socket_to_admin' didn't start. "sv;
		return;
	} 

	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - make socket_to_admin
		auto psocket_to_admin = this->process_prepare_socket_client(context_service);

		// - get initializing parameters from context
		std::string str_value = context_socket_admin;
		std::string str_value_lower = __to_lower(std::string_view(str_value));

		// - certify key ('socket_to_admin'.'certify_key_file' > 'certify_key' > 환경변수 CGDK_ADMIN_CERTIFY_KEY)
		psocket_to_admin->m_certify_key = server_system::get_certify_key_from_setting(context_socket_admin);

		// - enable reconnect
		psocket_to_admin->enable_reconnect();

		try
		{
			// - case A. default
			if (str_value_lower == "default"sv)
			{
				// trace) 
				LOG_PROGRESS << "   + socket to admin: default(localhost:22044)'"sv;

				psocket_to_admin->start({ { net::ip::address_v4::loopback(), 22044 }, {}, chrono::tick::seconds{5}, 0, 0, 0 });
			}
			// - case B. start socket_admin
			else
			{
				// trace) 
				LOG_PROGRESS << "   + socket to admin: 'custom address'"sv;

				psocket_to_admin->start(context_socket_admin);
			}
		}
		catch (...)
		{
			// - rollback
			psocket_to_admin->disable_reconnect();
			psocket_to_admin->closesocket();

			// reraise)
			throw;
		}
	}
}

void server_system::Xservice::_process_initializing_pre(context& /*_context*/)
{
	// declare) 
	server_system::iid_t iid{ 0 };
	std::string service_name;
	server_system::sREVISION service_revision;
	server_system::sRANK service_rank;
	std::string service_comment;

	// 1) getr
	SCOPED_LOCK(this->m_lockable_setting)
	{
		// - get service_instace
		auto& pservice_instance = this->m_pservice_instance;

		if (pservice_instance.exist())
		{
			// 1) get service setting
			auto& service_setting = pservice_instance->info.setting;

			service_setting.version.build = SVN_COMMIT_REVISION;
			service_setting.version.build_time = from_string<chrono::time::utc::time_point>(std::string_view(SVN_TIME_NOW_STRING), "%d/%d/%d %d:%d:%d"sv);

			iid = service_setting.iid;
			service_name = service_setting.name;
			service_revision = service_setting.revision;
			service_rank = service_setting.rank;
			service_comment = service_setting.comment;
		}
		else
		{
			iid = 0;
			service_name = this->name();
		}
	}

	// trace) 
	LOG_SYSTEM << "-----------------------------------------------------------------"sv << log_level_t::important();
	if (service_name.empty() == false)
	{
		LOG_SYSTEM << " "sv;
		LOG_SYSTEM << "                      "sv << service_name << log_level_t::important();
	}
	LOG_SYSTEM << " "sv;
	LOG_SYSTEM << "@ CGDK: "sv << CGDK_CONFIGURATION;
	LOG_SYSTEM << " "sv;
	LOG_SYSTEM << "@ SVN_URL: "sv << service_revision.url;
	LOG_SYSTEM << "@ SVN_UPDATE_REVISION: "sv << service_revision.revision;
	LOG_SYSTEM << "@ SVN_DATE: "sv << to_string(service_revision.time);
	LOG_SYSTEM << "@ compile: "sv << __DATE__ << " "sv << __TIME__;
	LOG_SYSTEM << " "sv;
	LOG_SYSTEM << "@ system uuid: "sv << api::get_system_unique_id();
	LOG_SYSTEM << " "sv;
	if (iid != 0)
	{
		LOG_SYSTEM << "@ parameters"sv;
		LOG_SYSTEM << "  + name: "sv << service_name << log_level_t::important();
		LOG_SYSTEM << "  + id_server.id: "sv << iid;
		LOG_SYSTEM << "  + rank.group: "sv << service_rank.group;
		LOG_SYSTEM << "  + rank.field: "sv << service_rank.field;
		LOG_SYSTEM << "  + rank.role: "sv << service_rank.role;
		LOG_SYSTEM << "  + rank.index: "sv << service_rank.index;
		LOG_SYSTEM << "  + comment: "sv << service_comment;
	}
	LOG_SYSTEM << "-----------------------------------------------------------------"sv << log_level_t::important();
}

void server_system::Xservice::_process_initializing_post(context& /*_context*/)
{
}

result_code server_system::Xservice::_process_initialize(context& _context)
{
	// 1) initialize pre
	this->_process_initialize_pre(_context);

	// 2) process initialize
	auto result = this->Nservice::_process_initialize(_context);

	// 3) initialize post
	this->_process_initialize_post(_context);

	// return)
	return result;
}

void server_system::Xservice::_process_initialize_pre(context& /*_context*/)
{
}

void server_system::Xservice::_process_initialize_post(context& /*_context*/)
{
}

result_code server_system::Xservice::_process_starting(context& _context)
{
	// 1) default executor 
	system_executor::initialize_instance(_context);

	// 2) on_start_setting
	this->on_start_setting(_context);

	// 3) on_starting
	this->on_starting(_context);

	// 4) start instance_manager
	SCOPED_LOCK(this->m_lockable_setting)
	{
		try
		{
			// - start host instance manager
			if (this->m_phost_instance_manager.exist())
			{
				this->m_phost_instance_manager->start(_context);
			}

			// - start service instance manager
			if (this->m_pservice_instance_manager.exist())
			{
				this->m_pservice_instance_manager->start(_context);

			}
		}
		catch (...)
		{
			if (this->m_pservice_instance_manager.exist())
			{
				this->m_pservice_instance_manager->stop();
			}

			if (this->m_phost_instance_manager.exist())
			{
				this->m_phost_instance_manager->stop();
			}

			// reraise)
			throw;
		}
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code server_system::Xservice::_process_stopping() noexcept
{
	try
	{
		// 1) stop service_instance
		if (this->m_pservice_instance_manager.exist())
		{
			// - stop service instance manager
			this->m_pservice_instance_manager->stop();
		}

		// 2) stop host instance manager
		if (this->m_phost_instance_manager.exist())
		{
			this->m_phost_instance_manager->stop();
		}

		auto psocket_to_admin = this->m_psocket_to_admin;
		if (psocket_to_admin.exist())
		{
			this->m_psocket_to_admin->process_service_instance_close();
		}

		// 3) on stopping
		this->on_stopping();
	}
	catch (...)
	{
		CGASSERT_ERROR(false);
	}
	
	return result_code(eRESULT::SUCCESS); 
}


result_code server_system::Xservice::_process_destroying() noexcept
{
	// 1) ...
	this->_process_destroy_pre();

	// 2) on_destroying 함수를 호출한다.
	this->on_destroying();

	// 3) ...
	this->_process_destroy_post();

	// return)
	return eRESULT::SUCCESS;
}

void server_system::Xservice::_process_destroying_pre() noexcept
{
	// trace) 
	LOG_PROGRESS << "<< _process_destroying_pre"sv;
}

void server_system::Xservice::_process_destroying_post() noexcept
{
	// trace) 
	LOG_PROGRESS << "<< _process_destroying_post"sv;
}

result_code server_system::Xservice::_process_destroy() noexcept
{
	// 1) ...
	this->_process_destroy_pre();

	// 2) close socket
	this->process_close_socket_client();

	// 3) reset server_system instance manager
	this->reset_service_instance_manager();
	this->reset_host_instance_manager();

	// 4) ...
	this->Nservice::_process_destroy();

	// 5)...
	this->_process_destroy_post();

	// trace) 
	LOG_PROGRESS << "<< _process_destroy"sv;

	// return)
	return eRESULT::SUCCESS;
}

void server_system::Xservice::_process_destroy_pre() noexcept
{
	// trace) 
	LOG_PROGRESS << "<< _process_destroy_pre'"sv;
}

void server_system::Xservice::_process_destroy_post() noexcept
{
	// trace) 
	LOG_PROGRESS << "<< _process_destroy_post'"sv;
}

void server_system::Xservice::process_state_changed(eOBJECT_STATE _state_new, eOBJECT_STATE _state_old)
{
	// 1) run ... 
	this->Nservice::process_state_changed(_state_new, _state_old);

	try
	{
		// 2) get socket to admin
		auto psocket_to_admin = get_socket_to_admin();

		// check)
		RETURN_IF(psocket_to_admin.empty(), );

		// declare)
		server_system::iid_t iid;
		shared_buffer buf_send_state_changed;

		// declare) 
		object_ptr<server_system::service::instance_local> pservice_instance;

		// 3) get service instance
		SCOPED_LOCK(this->m_lockable_setting)
		{
			// check)
			RETURN_IF(this->m_pservice_instance.empty(), );

			// - get 
			pservice_instance = this->m_pservice_instance.get_object_ptr();
		}

		// 4) get service info
		SCOPED_LOCK(pservice_instance->get_lockable())
		{
			// - get iid
			iid = pservice_instance->get_iid();

			// - make state change notify message
			buf_send_state_changed = server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(iid, _state_new, _state_old, &pservice_instance->info);
		}

		// 6) send service state for notification
		psocket_to_admin->send(buf_send_state_changed);
	}
	catch (...)
	{
	}
}

object_ptr<server_system::socket::client> server_system::Xservice::get_socket_to_admin()
{
	SCOPED_LOCK(this->m_lockable_setting)
	{
		return this->m_psocket_to_admin;
	}
}

void server_system::Xservice::send_to_admin_server(const shared_buffer& _buffer)
{
	// declare) 
	auto psocket_to_admin = this->get_socket_to_admin();

	// check)
	RETURN_IF(psocket_to_admin.empty(), );

	// 1) send
	psocket_to_admin->send(_buffer);
}

//-----------------------------------------------------------------------------
//
// run (콘솔 / windows service)
//
//  - 같은 실행 파일을 콘솔로도, windows service로도 실행할 수 있다.
//  - 인자가 없으면 콘솔로 실행한다. (ctrl-X로 종료)
//  - windows service 관련 인자 (server.service.h의 'run' 설명 참조)
//    --install [name] [options...] / --uninstall [name] / --start [name] / --stop [name]
//    --service <name> <folder> [options...] (SCM이 실행할 때 사용한다.)
//  - options
//    --setting <file> : setting 파일 (지정하지 않으면 run의 _setting_file. 상대 경로는 실행 폴더 기준)
//    그 외            : get_run_arguments()로 얻을 수 있다.
//    ('--install'의 options는 service 실행 인자로 등록되어 service가 시작될 때 전달된다.)
//
//-----------------------------------------------------------------------------
namespace
{
	// - setting 파일의 'service.setting.stop_grace_period'(초)를 읽는다. (없거나 읽지 못하면 0)
	uint32_t _read_stop_grace_period(std::string_view _setting_file) noexcept
	{
		try
		{
			context context_setting;
			context_setting.from_json(api::file::read_file(_setting_file));

			auto& context_value = context_setting["service"]["setting"]["stop_grace_period"];
			return context_value.is_valid() ? static_cast<uint32_t>(context_value) : 0;
		}
		catch (...)
		{
			return 0;
		}
	}
}

#if defined(_WIN32)
#include <ntsecapi.h>
#include <algorithm>
#include <cctype>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#include <shellapi.h>

namespace
{
	std::string _get_exe_path()
	{
		wchar_t path[MAX_PATH * 4]{};
		auto size = ::GetModuleFileNameW(nullptr, path, static_cast<DWORD>(std::size(path)));
		return server_system::utf16_to_utf8(std::wstring_view(path, size));
	}

	std::string _get_current_folder()
	{
		wchar_t path[MAX_PATH * 4]{};
		auto size = ::GetCurrentDirectoryW(static_cast<DWORD>(std::size(path)), path);
		return server_system::utf16_to_utf8(std::wstring_view(path, size));
	}

	std::string _get_file_name(const std::string& _path)
	{
		auto pos = _path.find_last_of("\\/");
		return (pos == std::string::npos) ? _path : _path.substr(pos + 1);
	}

	// - 기본 service 이름 ('<exe 이름(확장자 제외)>.<현재 폴더 이름>')
	std::string _get_default_windows_service_name()
	{
		auto exe_name = _get_file_name(_get_exe_path());
		auto pos_ext = exe_name.find_last_of('.');
		if (pos_ext != std::string::npos)
			exe_name.resize(pos_ext);

		return exe_name + "." + _get_file_name(_get_current_folder());
	}

	// - '--install'의 옵션 (service 실행 인자로 등록하지 않는다.)
	struct sINSTALL_OPTION
	{
		std::string			account;					// - 실행 계정 (비어 있으면 LocalSystem)
		std::string			password;
		bool				has_password = false;
		bool				restart = true;				// - 실패하면 다시 시작한다.
	};

	std::string _to_lower(std::string_view _text)
	{
		std::string result(_text);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char _ch) { return static_cast<char>(std::tolower(_ch)); });
		return result;
	}

	// - 기본 제공/가상 계정인가? (비밀번호와 'Log on as a service' 권한이 필요 없다.)
	bool _is_builtin_account(std::string_view _account)
	{
		auto account = _to_lower(_account);

		return account.empty()
			|| account == "localsystem" || account == ".\\localsystem"
			|| account == "nt authority\\system"
			|| account == "nt authority\\localservice"
			|| account == "nt authority\\networkservice"
			|| account.rfind("nt service\\", 0) == 0		// - virtual account
			|| account.back() == '$';						// - gMSA
	}

	// - 콘솔에서 비밀번호를 입력받는다. (입력은 화면에 보이지 않는다.)
	std::string _read_password(std::string_view _account)
	{
		std::cout << "password for '" << _account << "': " << std::flush;

		std::string password;
		for (;;)
		{
			auto ch = _getch();

			// - enter
			if (ch == '\r' || ch == '\n')
				break;

			// - backspace
			if (ch == '\b')
			{
				if (password.empty() == false)
					password.pop_back();
				continue;
			}

			// - 기능키 (두 번째 값은 버린다.)
			if (ch == 0 || ch == 0xE0)
			{
				(void)_getch();
				continue;
			}

			password.push_back(static_cast<char>(ch));
		}
		std::cout << "\n";

		return password;
	}

	// - 계정에 'Log on as a service'(SeServiceLogonRight) 권한을 준다.
	bool _grant_service_logon_right(const std::string& _account)
	{
		// declare) ".\user"는 로컬 계정 이름만으로 찾는다.
		std::string account = (_account.rfind(".\\", 0) == 0) ? _account.substr(2) : _account;

		// 1) SID
		BYTE buf_sid[SECURITY_MAX_SID_SIZE]{};
		DWORD size_sid = sizeof(buf_sid);
		wchar_t domain[256]{};
		DWORD size_domain = static_cast<DWORD>(std::size(domain));
		SID_NAME_USE sid_type;

		if (::LookupAccountNameW(nullptr, server_system::utf8_to_utf16(account).c_str(), buf_sid, &size_sid, domain, &size_domain, &sid_type) == FALSE)
		{
			std::cout << "error! fail to find account '" << _account << "' (error " << ::GetLastError() << ")\n";
			return false;
		}

		// 2) LSA policy
		LSA_OBJECT_ATTRIBUTES attributes{};
		LSA_HANDLE hpolicy = nullptr;
		if (::LsaOpenPolicy(nullptr, &attributes, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &hpolicy) != 0)
		{
			std::cout << "error! fail to open LSA policy (run as administrator)\n";
			return false;
		}

		// 3) add right
		wchar_t right[] = L"SeServiceLogonRight";
		LSA_UNICODE_STRING lsa_right{ static_cast<USHORT>((std::size(right) - 1) * sizeof(wchar_t)), static_cast<USHORT>(sizeof(right)), right };
		auto status = ::LsaAddAccountRights(hpolicy, buf_sid, &lsa_right, 1);
		::LsaClose(hpolicy);

		// check)
		if (status != 0)
		{
			std::cout << "error! fail to grant 'Log on as a service' to '" << _account << "' (error " << ::LsaNtStatusToWinError(status) << ")\n";
			return false;
		}

		// return)
		return true;
	}

	int _install_windows_service(const std::string& _service_name, const std::vector<std::string>& _options, sINSTALL_OPTION& _install_option)
	{
		// declare) image path ("<exe>" --service "<name>" "<folder>" [options...])
		const auto quote = [](std::string_view _argument) { return server_system::sSERVICE::sSETTING::quote_argument(_argument); };
		auto folder = _get_current_folder();
		auto command = quote(_get_exe_path()) + " --service " + quote(_service_name) + " " + quote(folder);

		for (const auto& iter : _options)
			command += " " + quote(iter);

		// 1) account (기본 제공 계정이 아니면 비밀번호와 'Log on as a service' 권한이 필요하다.)
		bool is_builtin_account = _is_builtin_account(_install_option.account);
		if (is_builtin_account == false)
		{
			if (_install_option.has_password == false)
				_install_option.password = _read_password(_install_option.account);

			if (_grant_service_logon_right(_install_option.account) == false)
				return 1;
		}

		// 2) SCM
		auto hscm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
		if (hscm == nullptr)
		{
			std::cout << "error! fail to open service control manager. (run as administrator) (error " << ::GetLastError() << ")\n";
			return 1;
		}

		// 3) create service (자동 시작)
		auto wservice_name = server_system::utf8_to_utf16(_service_name);
		auto wcommand = server_system::utf8_to_utf16(command);
		auto waccount = server_system::utf8_to_utf16(_install_option.account);
		auto wpassword = server_system::utf8_to_utf16(_install_option.password);
		const wchar_t* paccount = waccount.empty() ? nullptr : waccount.c_str();
		const wchar_t* ppassword = (is_builtin_account) ? nullptr : wpassword.c_str();
		auto hservice = ::CreateServiceW(hscm, wservice_name.c_str(), wservice_name.c_str(), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, wcommand.c_str(), nullptr, nullptr, nullptr, paccount, ppassword);

		// - 비밀번호는 더 이상 보관하지 않는다.
		std::fill(wpassword.begin(), wpassword.end(), L'\0');
		std::fill(_install_option.password.begin(), _install_option.password.end(), '\0');
		_install_option.password.clear();

		if (hservice == nullptr)
		{
			std::cout << "error! fail to install service '" << _service_name << "' (error " << ::GetLastError() << ")\n";
			::CloseServiceHandle(hscm);
			return 1;
		}

		// 4) description
		std::string description = "CGDK service (" + folder + ")";
		auto wdescription = server_system::utf8_to_utf16(description);
		SERVICE_DESCRIPTIONW service_description{ wdescription.data() };
		::ChangeServiceConfig2W(hservice, SERVICE_CONFIG_DESCRIPTION, &service_description);

		// 5) failure actions (실패하면 5초/10초/30초 후 다시 시작한다. 실패 횟수는 하루가 지나면 초기화된다.)
		if (_install_option.restart)
		{
			SC_ACTION actions[3] = { { SC_ACTION_RESTART, 5'000 }, { SC_ACTION_RESTART, 10'000 }, { SC_ACTION_RESTART, 30'000 } };
			SERVICE_FAILURE_ACTIONSW failure_actions{ 24 * 60 * 60, nullptr, nullptr, static_cast<DWORD>(std::size(actions)), actions };

			if (::ChangeServiceConfig2W(hservice, SERVICE_CONFIG_FAILURE_ACTIONS, &failure_actions) == FALSE)
				std::cout << "warning! fail to set failure actions (error " << ::GetLastError() << ")\n";

			// - 비정상 종료(crash)뿐 아니라 오류 코드로 멈춘 경우(SERVICE_STOPPED + exit code)에도 다시 시작한다.
			SERVICE_FAILURE_ACTIONS_FLAG failure_actions_flag{ TRUE };
			::ChangeServiceConfig2W(hservice, SERVICE_CONFIG_FAILURE_ACTIONS_FLAG, &failure_actions_flag);
		}

		::CloseServiceHandle(hservice);
		::CloseServiceHandle(hscm);

		// 6) event source (logger_system)
		if (logger_system::register_event_source(_service_name) == false)
			std::cout << "warning! fail to register event source '" << _service_name << "'\n";

		std::cout << "success on 'install' service '" << _service_name << "'\n";
		std::cout << "  " << command << "\n";
		std::cout << "  account: " << (_install_option.account.empty() ? std::string("LocalSystem") : _install_option.account) << "\n";
		std::cout << "  restart on failure: " << (_install_option.restart ? "yes (5s, 10s, 30s)" : "no") << "\n";
		return 0;
	}

	int _uninstall_windows_service(const std::string& _service_name)
	{
		// 1) SCM
		auto hscm = ::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (hscm == nullptr)
		{
			std::cout << "error! fail to open service control manager. (run as administrator) (error " << ::GetLastError() << ")\n";
			return 1;
		}

		// 2) open service
		auto hservice = ::OpenServiceW(hscm, server_system::utf8_to_utf16(_service_name).c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
		if (hservice == nullptr)
		{
			std::cout << "error! fail to open service '" << _service_name << "' (error " << ::GetLastError() << ")\n";
			::CloseServiceHandle(hscm);
			return 1;
		}

		// 3) 실행 중이면 멈춘다. (최대 60초 기다린다.)
		SERVICE_STATUS status{};
		if (::ControlService(hservice, SERVICE_CONTROL_STOP, &status))
		{
			for (int i = 0; i < 600 && ::QueryServiceStatus(hservice, &status) && status.dwCurrentState != SERVICE_STOPPED; ++i)
				std::this_thread::sleep_for(100ms);
		}

		// 4) delete
		auto result = ::DeleteService(hservice) != FALSE;
		if (result)
			std::cout << "success on 'uninstall' service '" << _service_name << "'\n";
		else
			std::cout << "error! fail to uninstall service '" << _service_name << "' (error " << ::GetLastError() << ")\n";

		::CloseServiceHandle(hservice);
		::CloseServiceHandle(hscm);

		// 5) event source (logger_system)
		if (result)
			logger_system::unregister_event_source(_service_name);

		return result ? 0 : 1;
	}
}
#endif

#if defined(__linux__)
#include <spawn.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <cerrno>

extern char** environ;

namespace
{
	// - '--install'의 옵션 (service 실행 인자로 등록하지 않는다.)
	struct sINSTALL_OPTION
	{
		std::string			account;					// - 실행 계정 (systemd 'User='. 비어 있으면 root)
		std::string			password;					// - (linux에서는 쓰지 않는다.)
		bool				has_password = false;
		bool				restart = true;				// - 실패하면 다시 시작한다. (systemd 'Restart=on-failure')
	};

	std::string _get_exe_path()
	{
		char path[4096]{};
		auto size = ::readlink("/proc/self/exe", path, sizeof(path) - 1);
		return (size > 0) ? std::string(path, static_cast<std::size_t>(size)) : std::string();
	}

	std::string _get_current_folder()
	{
		char path[4096]{};
		return (::getcwd(path, sizeof(path)) != nullptr) ? std::string(path) : std::string(".");
	}

	std::string _get_file_name(const std::string& _path)
	{
		auto pos = _path.find_last_of('/');
		return (pos == std::string::npos) ? _path : _path.substr(pos + 1);
	}

	// - 기본 service 이름 ('<exe 이름(확장자 제외)>.<현재 폴더 이름>')
	std::string _get_default_service_name()
	{
		auto exe_name = _get_file_name(_get_exe_path());
		auto pos_ext = exe_name.find_last_of('.');
		if (pos_ext != std::string::npos && pos_ext != 0)
			exe_name.resize(pos_ext);

		return exe_name + "." + _get_file_name(_get_current_folder());
	}

	// - 명령을 실행하고 종료 코드를 돌려준다. (실행하지 못하면 -1)
	int _run_command(const std::vector<std::string>& _arguments)
	{
		// declare) argv
		std::vector<std::string> arguments = _arguments;
		std::vector<char*> argv;
		for (auto& iter : arguments)
			argv.push_back(iter.data());
		argv.push_back(nullptr);

		// 1) spawn
		pid_t pid = 0;
		if (::posix_spawnp(&pid, argv[0], nullptr, nullptr, argv.data(), environ) != 0)
			return -1;

		// 2) wait
		int status = 0;
		pid_t result_wait;
		do
		{
			result_wait = ::waitpid(pid, &status, 0);
		} while (result_wait < 0 && errno == EINTR);

		// return)
		return (result_wait == pid && WIFEXITED(status)) ? WEXITSTATUS(status) : -1;
	}

	// - systemd unit의 인자 (따옴표로 감싸고 '\', '"'는 escape, '%'는 '%%', '$'는 '$$')
	std::string _quote_systemd(std::string_view _argument)
	{
		std::string result = "\"";
		for (auto ch : _argument)
		{
			if (ch == '\\' || ch == '"')
				result += '\\';
			else if (ch == '%')
				result += '%';
			else if (ch == '$')
				result += '$';

			result += ch;
		}
		result += '"';
		return result;
	}

	std::string _get_unit_file(const std::string& _service_name)
	{
		return "/etc/systemd/system/" + _service_name + ".service";
	}

	int _install_linux_service(const std::string& _service_name, const std::vector<std::string>& _options, sINSTALL_OPTION& _install_option, uint32_t _stop_grace_period)
	{
		// declare) ExecStart ("<exe>" --service "<name>" "<folder>" [options...])
		auto folder = _get_current_folder();
		auto command = _quote_systemd(_get_exe_path()) + " --service " + _quote_systemd(_service_name) + " " + _quote_systemd(folder);

		for (const auto& iter : _options)
			command += " " + _quote_systemd(iter);

		// check) linux에서는 비밀번호를 쓰지 않는다.
		if (_install_option.has_password)
			std::cout << "warning! '--password' is not used on linux. (ignored)\n";

		// 1) unit file
		auto unit_file = _get_unit_file(_service_name);
		{
			std::ofstream file(unit_file, std::ios::out | std::ios::trunc);
			if (file.is_open() == false)
			{
				std::cout << "error! fail to write '" << unit_file << "' (run as root) (errno " << errno << ")\n";
				return 1;
			}

			file << "[Unit]\n";
			file << "Description=CGDK service " << _service_name << " (" << folder << ")\n";
			file << "After=network-online.target\n";
			file << "Wants=network-online.target\n";
			file << "\n";
			file << "[Service]\n";
			file << "Type=simple\n";
			file << "WorkingDirectory=" << folder << "\n";
			file << "ExecStart=" << command << "\n";
			file << "KillSignal=SIGTERM\n";
			if (_stop_grace_period != 0)
				file << "TimeoutStopSec=" << _stop_grace_period << "\n";	// - setting의 stop_grace_period (없으면 systemd 기본값)
			if (_install_option.restart)
			{
				file << "Restart=on-failure\n";
				file << "RestartSec=5\n";
			}
			else
			{
				file << "Restart=no\n";
			}
			if (_install_option.account.empty() == false)
				file << "User=" << _install_option.account << "\n";
			file << "\n";
			file << "[Install]\n";
			file << "WantedBy=multi-user.target\n";
		}

		// 2) reload & enable (부팅할 때 자동 시작)
		_run_command({ "systemctl", "daemon-reload" });
		if (_run_command({ "systemctl", "enable", _service_name + ".service" }) != 0)
		{
			std::cout << "error! fail to 'systemctl enable " << _service_name << "'\n";
			return 1;
		}

		std::cout << "success on 'install' service '" << _service_name << "' (" << unit_file << ")\n";
		std::cout << "  " << command << "\n";
		std::cout << "  account: " << (_install_option.account.empty() ? std::string("root") : _install_option.account) << "\n";
		std::cout << "  restart on failure: " << (_install_option.restart ? "yes (5s)" : "no") << "\n";
		return 0;
	}

	int _uninstall_linux_service(const std::string& _service_name)
	{
		// 1) stop & disable
		_run_command({ "systemctl", "disable", "--now", _service_name + ".service" });

		// 2) unit file
		auto unit_file = _get_unit_file(_service_name);
		if (::unlink(unit_file.c_str()) != 0)
		{
			std::cout << "error! fail to remove '" << unit_file << "' (run as root) (errno " << errno << ")\n";
			return 1;
		}

		// 3) reload
		_run_command({ "systemctl", "daemon-reload" });

		std::cout << "success on 'uninstall' service '" << _service_name << "'\n";
		return 0;
	}

	int _control_linux_service(const std::string& _service_name, std::string_view _command)
	{
		auto result = _run_command({ "systemctl", std::string(_command), _service_name + ".service" }) == 0;
		std::cout << (result ? "success on '" : "error! fail to '") << _command << "' service '" << _service_name << "'\n";
		return result ? 0 : 1;
	}

	// - SIGTERM/SIGINT (systemctl stop)
	std::atomic<bool> g_is_linux_service_stop_requested{ false };

	void _signal_handler_linux_service_stop(int) noexcept
	{
		g_is_linux_service_stop_requested = true;
	}
}
#endif

int server_system::Xservice::run(int _argc, char** _argv, std::string_view _setting_file, const std::function<void(int)>& _fn_console_key)
{
	// declare) 실행 인자 (UTF-8)
	std::vector<std::string> arguments;
#if defined(_WIN32)
	// - windows: argv는 시스템 코드 페이지(ANSI)이므로 GetCommandLineW로 UTF-8 인자를 얻는다.
	(void)_argc;
	(void)_argv;
	{
		int count = 0;
		auto pargv = ::CommandLineToArgvW(::GetCommandLineW(), &count);
		if (pargv != nullptr)
		{
			for (int i = 1; i < count; ++i)
				arguments.push_back(server_system::utf16_to_utf8(pargv[i]));

			::LocalFree(pargv);
		}
	}
#else
	if (_argv != nullptr)
	{
		for (int i = 1; i < _argc; ++i)
		{
			if (_argv[i] != nullptr)
				arguments.emplace_back(_argv[i]);
		}
	}
#endif

	// 1) command ('--install', '--uninstall', '--start', '--stop', '--service')
	std::string command;
	std::size_t index = 0;

	if (!arguments.empty() && (arguments[0] == "--install"sv || arguments[0] == "--uninstall"sv || arguments[0] == "--start"sv || arguments[0] == "--stop"sv || arguments[0] == "--service"sv))
		command = arguments[index++];

	// 2) service name, folder
	std::string service_name;
	std::string service_folder;

	if (command == "--service"sv)
	{
		// check)
		if (arguments.size() < 3)
		{
			std::cout << "error! usage) --service <name> <folder> [options...]\n";
			return 1;
		}

		service_name = arguments[index++];
		service_folder = arguments[index++];
	}
	else if (!command.empty() && index < arguments.size() && arguments[index].rfind("--"sv, 0) != 0)
	{
		service_name = arguments[index++];
	}

	// 3) options (나머지 인자. '--install'이면 service 실행 인자로 등록된다.)
	//    - '--setting <file>'
	//    - '--install'의 옵션('--account', '--password', '--no-restart')은 service 실행 인자에 넣지 않는다.
	//    - 모르는 '--' 옵션은 경고한다. (get_run_arguments()로는 그대로 전달된다.)
	std::vector<std::string> options;
	std::string setting_file(_setting_file);
	bool is_setting_file_given = false;
	this->m_run_arguments.clear();
	this->m_run_warnings.clear();

	sINSTALL_OPTION install_option;

	for (std::size_t i = index; i < arguments.size(); ++i)
	{
		const auto& option = arguments[i];

		// - 값이 필요한 옵션의 값을 얻는다.
		const auto get_value = [&](std::string& _value) -> bool
		{
			if (i + 1 >= arguments.size())
			{
				std::cout << "error! '" << option << "' needs a value\n";
				return false;
			}

			_value = arguments[++i];
			return true;
		};

		if (option == "--setting"sv)
		{
			// check) 두 번 이상 주면 마지막 값을 쓴다.
			if (is_setting_file_given)
				this->m_run_warnings.push_back("'--setting' is given more than once. (the last one is used)");

			RETURN_IF(get_value(setting_file) == false, 1);
			is_setting_file_given = true;

			options.push_back(option);
			options.push_back(setting_file);
		}
		else if (option == "--account"sv || option == "--password"sv || option == "--no-restart"sv)
		{
			// check) '--install'에서만 쓴다.
			if (command != "--install"sv)
				this->m_run_warnings.push_back("'" + option + "' is used only with '--install'. (ignored)");

			if (option == "--account"sv)
			{
				RETURN_IF(get_value(install_option.account) == false, 1);
			}
			else if (option == "--password"sv)
			{
				RETURN_IF(get_value(install_option.password) == false, 1);
				install_option.has_password = true;
			}
			else
			{
				install_option.restart = false;
			}
		}
		else
		{
			// check) 명령은 첫 번째 인자여야 한다.
			if (option == "--install"sv || option == "--uninstall"sv || option == "--start"sv || option == "--stop"sv || option == "--service"sv)
				this->m_run_warnings.push_back("'" + option + "' must be the first argument. (ignored)");
			// check) 모르는 '--' 옵션
			else if (option.rfind("--"sv, 0) == 0)
				this->m_run_warnings.push_back("unknown option '" + option + "'. (passed to get_run_arguments())");

			options.push_back(option);
			this->m_run_arguments.push_back(option);
		}
	}

	this->m_run_setting_file = setting_file;

	// - '--install'/'--uninstall'/'--start'/'--stop'은 콘솔에 경고를 출력한다. (그 외는 initialize할 때 로그로 남긴다.)
	if (command.empty() == false && command != "--service"sv)
	{
#if defined(_WIN32)
		// - 출력은 UTF-8이다. (한글 이름/경로)
		::SetConsoleOutputCP(CP_UTF8);
#endif
		for (const auto& iter : this->m_run_warnings)
			std::cout << "warning! " << iter << "\n";
	}

	// - 실제로 적용된 실행 인자를 service info(info.arguments)에 보관한다. ('--service <name> <folder>'는 제외. setting.arguments는 요청한 값이므로 바꾸지 않는다.)
	SCOPED_LOCK(this->m_lockable_setting)
	{
		if (this->m_pservice_instance.exist())
			this->m_pservice_instance->info.info.arguments = options;
	}

#if defined(_WIN32)
	// 4) windows service 명령
	if (!command.empty())
	{
		if (service_name.empty())
			service_name = _get_default_windows_service_name();

		if (command == "--install"sv)
			return _install_windows_service(service_name, options, install_option);

		if (command == "--uninstall"sv)
			return _uninstall_windows_service(service_name);

		if (command == "--start"sv)
		{
			auto result = api::service_start(std::wstring_view(server_system::utf8_to_utf16(service_name)), 0, nullptr) != 0;
			std::cout << (result ? "success on 'start' service '" : "error! fail to start service '") << service_name << "'\n";
			return result ? 0 : 1;
		}

		if (command == "--stop"sv)
		{
			auto result = api::service_stop(std::wstring_view(server_system::utf8_to_utf16(service_name))) != 0;
			std::cout << (result ? "success on 'stop' service '" : "error! fail to stop service '") << service_name << "'\n";
			return result ? 0 : 1;
		}

		if (command == "--service"sv)
			return this->process_run_windows_service(service_name, service_folder, setting_file);
	}
#elif defined(__linux__)
	// 4) linux service 명령 (systemd)
	if (!command.empty())
	{
		if (service_name.empty())
			service_name = _get_default_service_name();

		if (command == "--install"sv)
			return _install_linux_service(service_name, options, install_option, _read_stop_grace_period(setting_file));

		if (command == "--uninstall"sv)
			return _uninstall_linux_service(service_name);

		if (command == "--start"sv)
			return _control_linux_service(service_name, "start"sv);

		if (command == "--stop"sv)
			return _control_linux_service(service_name, "stop"sv);

		if (command == "--service"sv)
			return this->process_run_linux_service(service_name, service_folder, setting_file);
	}
#else
	// check) service 명령은 windows/linux에서만 지원한다.
	if (!command.empty())
	{
		std::cout << "error! '" << command << "' is not supported on this platform\n";
		return 1;
	}
#endif

	// 5) console
	return this->process_run_console(setting_file, _fn_console_key);
}

bool server_system::Xservice::process_run_initialize(std::string_view _setting_file)
{
	// - 실행 인자의 경고를 로그로 남긴다.
	for (const auto& iter : this->m_run_warnings)
	{
		LOG_WARNING << "(warn) Xservice: "sv << iter;
	}

	try
	{
		// declare)
		context context_initialize;

		// 1) setting 파일을 읽는다.
		try
		{
			context_initialize.from_json(api::file::read_file(_setting_file));
		}
		catch (const std::exception& _e)
		{
			LOG_ERROR << "(err ) Xservice: fail to read setting file '"sv << _setting_file << "' ("sv << _e.what() << ")"sv;
		}
		catch (...)
		{
			LOG_ERROR << "(err ) Xservice: fail to read setting file '"sv << _setting_file << "'"sv;
		}

		// 2) initialize ('start'는 컨트롤 서버가 요청한다.)
		auto result = this->initialize(context_initialize);

		// check) (result_code는 실패일 때 true이다. SUCCESS == 0)
		RETURN_IF(result != eRESULT::SUCCESS, false);

		// 3) start (is_start_on_run)
		if (this->is_start_on_run(context_initialize))
			this->start(context_initialize);
	}
	catch (...)
	{
		return false;
	}

	// return)
	return true;
}

bool server_system::Xservice::is_start_on_run(context& _context)
{
	// - 'socket_to_admin' 설정이 없으면 바로 start한다. (있으면 컨트롤 서버가 start를 요청한다.)
	return _context["service"]["socket_to_admin"].is_invalid();
}

uint32_t server_system::Xservice::process_get_stop_wait_milliseconds() noexcept
{
	// - service setting의 stop_grace_period(초). 없으면 60초
	uint32_t stop_grace_period = 0;

	SCOPED_LOCK(this->m_lockable_setting)
	{
		if (this->m_pservice_instance.exist())
			stop_grace_period = this->m_pservice_instance->info.setting.stop_grace_period;
	}

	return (stop_grace_period != 0) ? stop_grace_period * 1'000 : 60'000;
}

void server_system::Xservice::on_console_key(int _key)
{
	switch (_key)
	{
	case	'C':
	case	'c':
			net::io::connective::manager::print_statistics();
			break;

	case	'D':
	case	'd':
			dump_manager::print_statistics();
			break;

	case	'E':
	case	'e':
			executor::manager::print_statistics();
			break;

	case	'F':
	case	'f':
			factory::manager::print_statistics();
			break;

	case	'S':
	case	's':
			schedulable::manager::print_statistics();
			break;

	case	'T':
	case	't':
			time_event::entity_manager::print_statistics();
			break;

	default:
			break;
	}
}

int server_system::Xservice::process_run_console(std::string_view _setting_file, const std::function<void(int)>& _fn_console_key)
{
	// 1) 콘솔 로거를 등록한다.
	auto plogger_console = make_own<logger_console>();
	plogger_console->initialize();

	auto plogger_hub = this->get_logger();
	if (plogger_hub.exist())
		plogger_hub->register_log_targetable(plogger_console);

	// 2) initialize & start
	this->process_run_initialize(_setting_file);

	// 3) 종료될 때까지 기다린다. (1초 단위로 ctrl-X의 입력을 확인한다.)
	while (this->wait_destroy(1s) == false)
	{
		// check)
		if (_kbhit() == 0)
			continue;

		// - key
		auto ch = _getch();

		// check) ctrl-X를 입력하면 종료한다.
		if (ch == 24)
			break;

		// - key
		this->on_console_key(ch);

		if (_fn_console_key)
			_fn_console_key(ch);
	}

	// 4) destroy
	this->destroy();

	// 5) 콘솔 로거를 해제한다.
	if (plogger_hub.exist())
		plogger_hub->unregister_log_targetable(plogger_console);

	// return)
	return 0;
}

#if defined(_WIN32)
int server_system::Xservice::process_run_windows_service(std::string_view _service_name, std::string_view _folder, std::string_view _setting_file)
{
	// 1) 작업 폴더 (SCM은 system32 폴더에서 실행한다.)
	if (::SetCurrentDirectoryW(server_system::utf8_to_utf16(_folder).c_str()) == FALSE)
		return 1;

	// 2) 설정
	this->m_windows_service_name = _service_name;
	this->m_windows_service_setting_file = _setting_file;
	this->m_is_windows_service_stop_requested = false;
	m_s_pwindows_service = this;
	m_s_is_windows_service_mode = true;

	// 3) SCM에 연결한다. (service가 멈출 때까지 블록된다.)
	auto wservice_name = server_system::utf8_to_utf16(this->m_windows_service_name);
	SERVICE_TABLE_ENTRYW service_table[] =
	{
		{ wservice_name.data(), fwindows_service_main },
		{ nullptr, nullptr }
	};

	auto result = ::StartServiceCtrlDispatcherW(service_table);

	// 4) reset
	m_s_pwindows_service = nullptr;

	// return)
	return (result != FALSE) ? 0 : 1;
}

void WINAPI server_system::Xservice::fwindows_service_main(DWORD /*_argc*/, LPWSTR* /*_argv*/)
{
	// check)
	RETURN_IF(m_s_pwindows_service == nullptr);

	// 1) run
	m_s_pwindows_service->process_windows_service_main();
}

void server_system::Xservice::process_windows_service_main()
{
	// 1) service control handler를 등록한다.
	this->m_hwindows_service_status = ::RegisterServiceCtrlHandlerExW(server_system::utf8_to_utf16(this->m_windows_service_name).c_str(), fwindows_service_handler, this);

	// check)
	RETURN_IF(this->m_hwindows_service_status == nullptr);

	// 2) START_PENDING (initialize는 db 접속 등으로 오래 걸릴 수 있다.)
	this->process_set_windows_service_status(SERVICE_START_PENDING, 0, 120'000);

	// 3) event log (logger_system. 콘솔이 없으므로 fatal/error/warning을 이벤트 로그에도 남긴다. source: service 이름)
	auto plogger_hub = this->get_logger();
	{
		auto plogger_system = make_own<logger_system>();

		logger_system::INITIALIZE_PARAMETER parameter;
		parameter.source = this->m_windows_service_name;
		plogger_system->initialize(parameter);

		if (plogger_hub.exist())
			plogger_hub->register_log_targetable(plogger_system);

		this->m_plogger_system = std::move(plogger_system);
	}

	// - event log를 해제한다.
	const auto release_logger_system = [&]()
	{
		if (this->m_plogger_system.exist())
		{
			if (plogger_hub.exist())
				plogger_hub->unregister_log_targetable(this->m_plogger_system);

			this->m_plogger_system.reset();
		}
	};

	// 4) initialize & start
	auto result = this->process_run_initialize(this->m_windows_service_setting_file);

	// check) 실패하면 멈춘다.
	if (!result)
	{
		LOG_ERROR << "(err ) Xservice: fail to initialize service '"sv << this->m_windows_service_name << "'"sv;

		this->destroy();
		release_logger_system();
		this->process_set_windows_service_status(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
		return;
	}

	// 5) RUNNING
	this->process_set_windows_service_status(SERVICE_RUNNING);

	// 6) 멈춤 요청이 오거나 service가 destroy될 때까지 기다린다. (컨트롤 서버가 종료시킬 수도 있다.)
	while (this->wait_destroy(1s) == false)
	{
		if (this->m_is_windows_service_stop_requested)
			break;
	}

	// 7) destroy
	this->process_set_windows_service_status(SERVICE_STOP_PENDING, 0, this->process_get_stop_wait_milliseconds());

	this->destroy();

	release_logger_system();

	// 8) STOPPED
	this->process_set_windows_service_status(SERVICE_STOPPED);
}

DWORD WINAPI server_system::Xservice::fwindows_service_handler(DWORD _control, DWORD /*_event_type*/, LPVOID /*_pevent_data*/, LPVOID _pcontext)
{
	// declare)
	auto pservice = reinterpret_cast<Xservice*>(_pcontext);

	switch (_control)
	{
	case	SERVICE_CONTROL_STOP:
	case	SERVICE_CONTROL_SHUTDOWN:
			// - 멈춤을 요청한다. (destroy는 service main thread에서 한다.)
			pservice->m_is_windows_service_stop_requested = true;
			pservice->process_set_windows_service_status(SERVICE_STOP_PENDING, 0, pservice->process_get_stop_wait_milliseconds());
			return NO_ERROR;

	case	SERVICE_CONTROL_INTERROGATE:
			return NO_ERROR;

	default:
			return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

void server_system::Xservice::process_set_windows_service_status(uint32_t _state, uint32_t _exit_code, uint32_t _wait_hint) noexcept
{
	// check)
	RETURN_IF(this->m_hwindows_service_status == nullptr);

	// declare)
	static std::atomic<DWORD> s_check_point{ 0 };

	// 1) status
	SERVICE_STATUS status{};
	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = _state;
	status.dwControlsAccepted = (_state == SERVICE_START_PENDING || _state == SERVICE_STOPPED) ? 0 : (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
	status.dwWin32ExitCode = _exit_code;
	status.dwServiceSpecificExitCode = (_exit_code == ERROR_SERVICE_SPECIFIC_ERROR) ? 1 : 0;
	status.dwCheckPoint = (_state == SERVICE_RUNNING || _state == SERVICE_STOPPED) ? 0 : ++s_check_point;
	status.dwWaitHint = _wait_hint;

	// 2) set
	::SetServiceStatus(this->m_hwindows_service_status, &status);
}
#endif

#if defined(__linux__)
int server_system::Xservice::process_run_linux_service(std::string_view _service_name, std::string_view _folder, std::string_view _setting_file)
{
	// 1) 작업 폴더
	if (::chdir(std::string(_folder).c_str()) != 0)
		return 1;

	// 2) syslog (logger_system. source: service 이름)
	auto plogger_hub = this->get_logger();
	{
		auto plogger_system = make_own<logger_system>();

		logger_system::INITIALIZE_PARAMETER parameter;
		parameter.source = _service_name;
		plogger_system->initialize(parameter);

		if (plogger_hub.exist())
			plogger_hub->register_log_targetable(plogger_system);

		this->m_plogger_system = std::move(plogger_system);
	}

	// - syslog를 해제한다.
	const auto release_logger_system = [&]()
	{
		if (this->m_plogger_system.exist())
		{
			if (plogger_hub.exist())
				plogger_hub->unregister_log_targetable(this->m_plogger_system);

			this->m_plogger_system.reset();
		}
	};

	// 3) initialize & start
	auto result = this->process_run_initialize(_setting_file);

	// check) 실패하면 오류 코드로 끝낸다. (systemd 'Restart=on-failure'면 다시 시작한다.)
	if (!result)
	{
		LOG_ERROR << "(err ) Xservice: fail to initialize service '"sv << _service_name << "'"sv;

		this->destroy();
		release_logger_system();
		return 1;
	}

	// - SIGTERM/SIGINT로 멈춘다. (systemctl stop은 SIGTERM을 보낸다.)
	//   (initialize 중에 dump_manager가 SIGTERM 핸들러를 설치하므로 initialize 후에 설치한다.)
	g_is_linux_service_stop_requested = false;
	struct sigaction action {};
	action.sa_handler = _signal_handler_linux_service_stop;
	sigemptyset(&action.sa_mask);
	sigaction(SIGTERM, &action, nullptr);
	sigaction(SIGINT, &action, nullptr);

	// 5) 멈춤 요청(SIGTERM)이 오거나 service가 destroy될 때까지 기다린다. (컨트롤 서버가 종료시킬 수도 있다.)
	while (this->wait_destroy(1s) == false)
	{
		if (g_is_linux_service_stop_requested)
			break;
	}

	// 6) destroy
	this->destroy();

	release_logger_system();

	// return)
	return 0;
}
#endif
