#include "pch.h"

using namespace CGDK;


server_system::socket::client::client()
{
	// 1) set name
	this->name("server_system.socket::client");
}

server_system::socket::client::~client()
{
}


void server_system::socket::client::on_connect()
{
	// 1) set socket option
	this->maximum_message_buffer_size(512 * 1024 * 1024);
	this->maximum_depth_of_send_buffer(4096);
	this->maximum_depth_of_send_buffer_bytes(512 * 1024 * 1024);

	// check)
	CGASSERT_ERROR(this->is_client_socket());

	// 2) get instance_manager
	{
		// - register service_instance_local
		auto pservice_nest = server_system::get_service();

		// check)
		RETURN_IF(pservice_nest.empty(), );

		// - casting service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(pservice_nest);

		// check)
		RETURN_IF(pservice.empty(), );

		// - get instance_manager
		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		RETURN_IF(pservice_instance_manager.empty(), );

		// - unregister service instance
		this->register_messageable(pservice_instance_manager);
	}

	// 3) reset certify state of this connection
	this->m_is_instance_certified = false;
	this->m_is_failure_counted = false;

	// 4) make certification message
	server_system::sCERTIFY_INFO certification_info;

	// trace)
	LOG_PROGRESS << log_level_t::important() << "< ADMIN_SERVER[ip:'" << this->remote_endpoint().to_string() << "'] connected";

	// 4) send request message
	this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(certification_info, this->m_certify_key));
}

void server_system::socket::client::on_disconnect(uint64_t /*_reason*/)
{
	// - 인증되기 전에 끊겼으면 (admin/host agent가 거부하고 끊은 경우 등) 실패로 센다.
	if (this->m_is_instance_certified == false && this->m_is_failure_counted == false)
		this->process_certify_failed(eRESULT::FAIL);
	this->m_is_instance_certified = false;

	// trace)
	LOG_PROGRESS << log_level_t::important() << "< ADMIN_SERVER[ip:'" << this->remote_endpoint().to_string() << "'] disconnected";

	this->process_service_instance_close();
}

void server_system::socket::client::process_certify_failed(eRESULT _result)
{
	//-----------------------------------------------------------------
	// 설명) 인증 실패가 계속되면 재접속 간격을 늘린다. (최대 60초)
	//  - 접속 한 번에 한 번만 센다.
	//  - 로그는 1, 2, 4, 8...번째 실패에만 남긴다.
	//-----------------------------------------------------------------

	// check)
	if (this->m_is_failure_counted)
		return;
	this->m_is_failure_counted = true;

	// 1) 기본 재접속 간격 (1초보다 짧으면 1초)
	if (this->m_reconnect_interval_base == chrono::tick::duration::zero())
	{
		this->m_reconnect_interval_base = this->get_reconnect_interval();

		if (this->m_reconnect_interval_base < chrono::tick::seconds{ 1 })
			this->m_reconnect_interval_base = chrono::tick::seconds{ 1 };
	}

	// 2) count
	auto count = ++this->m_count_certify_fail;

	// 3) interval = base * 2^(count-1) (최대 60초)
	const chrono::tick::duration interval_max = chrono::tick::seconds{ 60 };
	auto interval = this->m_reconnect_interval_base;
	for (int i = 1; i < count && interval < interval_max; ++i)
		interval *= 2;
	if (interval > interval_max)
		interval = interval_max;

	this->set_reconnect_interval(interval);

	// log)
	if ((count & (count - 1)) == 0)
	{
		LOG_ERROR << "(err ) fail to certify from ADMIN_SERVER ["sv << to_string(_result) << "("sv << static_cast<int>(_result) << ")] (failed "sv << count << " times, retry after "sv << std::chrono::duration_cast<std::chrono::milliseconds>(interval).count() << "ms)"sv;
	}
}

void server_system::socket::client::process_certify_succeeded()
{
	// 1) reset
	this->m_count_certify_fail = 0;

	// 2) 재접속 간격을 되돌린다.
	if (this->m_reconnect_interval_base != chrono::tick::duration::zero())
		this->set_reconnect_interval(this->m_reconnect_interval_base);
}

void server_system::socket::client::process_service_instance_prepare()
{
	// declare)
	object_ptr<server_system::Xservice> pservice;

	// 1) get service
	{
		// 1) register service_instance_local
		auto pservice_nest = server_system::get_service();

		// check)
		CGASSERT_ERROR(pservice_nest.exist());

		// check)
		THROW_IF(pservice_nest.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service instance is not ready"sv));

		// 2) casting service
		pservice = dynamic_pointer_cast<server_system::Xservice>(pservice_nest);

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service instance is not derived from 'server_system::Xservice' class"sv));
	}

	// 2_ start local service instance
	this->process_start_service_instance_start(pservice);
}

void server_system::socket::client::process_start_service_instance_start(server_system::Xservice* _pservice)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// 1) get instance manager
		auto phost_instance_manager = _pservice->get_host_instance_manager();
		auto pservice_instance_manager = _pservice->get_service_instance_manager();

		// check)
		CGASSERT_ERROR(phost_instance_manager.exist());
		CGASSERT_ERROR(pservice_instance_manager.exist());

		// check)
		THROW_IF(phost_instance_manager.empty(), throwable::failure(eRESULT::BYPASS, 0, "'host instance manager' is empty"sv));

		// check)
		THROW_IF(pservice_instance_manager.empty(), throwable::failure(eRESULT::BYPASS, 0, "'service instance manager' is empty"sv));

		// 2) get service_instance
		auto pservice_local_instance = _pservice->get_service_instance();

		// check)
		THROW_IF(pservice_local_instance.empty(), throwable::failure(eRESULT::BYPASS, 0, "'local service instance' is empty"sv));

		SCOPED_LOCK(pservice_local_instance->get_lockable())
		{
			// declare)
			shared_buffer buf_temp;

			// 3) generate service info packet
			{
				//// - set service info
				//pservice_local_instance->info.status = pservice_local_instance->info.status;

				// - set status info
				pservice_local_instance->info.setting.iid = pservice_local_instance->get_iid();
				pservice_local_instance->info.info.host_iid = 0;
				//pservice_local_instance->info.info.pid = pservice_local_instance->info.info.pid;
				pservice_local_instance->info.status.state = eOBJECT_STATE::NONE;
				pservice_local_instance->info.status.iid = pservice_local_instance->info.setting.iid;
				pservice_local_instance->info.status.vector_factory = server_system::get_snap(factory::manager::instance());
				pservice_local_instance->info.status.vector_executor = server_system::get_snap(executor::manager::instance());
				pservice_local_instance->info.status.vector_schedulable = server_system::get_snap(system_executor::schedulable_manager());
				pservice_local_instance->info.status.vector_connective = server_system::get_snap(net::io::connective::manager::instance());
				pservice_local_instance->info.status.vector_binder = server_system::get_snap(net::io::binder::manager::instance());
				pservice_local_instance->info.status.vector_time_event = server_system::get_snap(time_event::entity_manager::instance());
				pservice_local_instance->info.status.connective_info = server_system::get_snap(net::io::statistics::Nconnective::statistics_get_connective_info_total());
				pservice_local_instance->info.status.traffic_info = server_system::get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());
				pservice_local_instance->info.status.vector_log_record = server_system::get_records_of_default_logger_circular_list();
				pservice_local_instance->info.status.time_update = chrono::time::utc::now();

				// - make message
				buf_temp = server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(phost_instance_manager.get(), pservice_instance_manager.get());
			}

			// 4) send ...
			this->send(buf_temp);

			// 5) register messageable
			factory::manager::instance()->register_messageable(this);
			executor::manager::instance()->register_messageable(this);
			net::io::connective::manager::instance()->register_messageable(this);
			net::io::binder::manager::instance()->register_messageable(this);
			time_event::entity_manager::instance()->register_messageable(this);
			dump_manager::instance()->register_messageable(this);
			server_system::Nstatic_service_logger::get_logger()->register_log_targetable(this);
		}

		// 12) execute interval => 5sec
		this->execute_interval(5s);
	}
}

void server_system::socket::client::process_service_instance_close()
{
	// 1) detach from scheduler
	system_executor::unregister_schedulable(this);

	// 2) unregister messageable
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// - get instance_manager
		auto pservice = server_system::get_service();

		// check)
		if (pservice.exist())
		{
			// - casting service
			auto pservice_nest = dynamic_pointer_cast<server_system::Xservice>(pservice);

			// check)
			if (pservice_nest.exist())
			{
				// 4) get instance_manager
				auto pservice_instance_manager = pservice_nest->get_service_instance_manager();

				// check)
				if (pservice_instance_manager.exist())
				{
					this->unregister_messageable(pservice_instance_manager);
				}

				// - unregister messageable
				SCOPED_LOCK(pservice_nest->m_lockable_setting)
				{
					factory::manager::instance()->unregister_messageable(this);
					executor::manager::instance()->unregister_messageable(this);
					net::io::connective::manager::instance()->unregister_messageable(this);
					net::io::binder::manager::instance()->unregister_messageable(this);
					time_event::entity_manager::instance()->unregister_messageable(this);
					dump_manager::instance()->unregister_messageable(this);
					server_system::Nstatic_service_logger::get_logger()->unregister_log_targetable(this);
				}
			}
		}
	}
}

object_ptr<server_system::service::instance_local> server_system::socket::client::process_get_service_instance() const noexcept
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		return this->m_pservice_instance;
	}
}


result_code server_system::socket::client::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// 3) execute message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE,			on_message_SERVER_ADMIN_CERTIFY_RESPONSE, sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE,on_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE, sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return)
	return eRESULT::BYPASS;
}

result_code server_system::socket::client::process_message(sMESSAGE& _msg)
{
	// 1) execute message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE(eMESSAGE::DUMP,										process_message_SERVER_ADMIN_EXCEPTION_DUMP, sMESSAGE_DUMP);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_REGISTER,					process_message_FACTORY_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,				process_message_FACTORY_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_UPDATE,					process_message_FACTORY_UPDATE, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::THREAD_REGISTER,							process_message_THREAD_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::THREAD_UNREGISTER,							process_message_THREAD_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_REGISTER,							process_message_EXECUTOR_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_UNREGISTER,						process_message_EXECUTOR_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_UPDATE,							process_message_EXECUTOR_UPDATE, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,		process_message_NETWORK_SOCKET_CONNECTIVE_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,	process_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE,		process_message_NETWORK_SOCKET_CONNECTIVE_UPDATE, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,			process_message_NETWORK_SOCKET_BINDER_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,		process_message_NETWORK_SOCKET_BINDER_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_UPDATE,			process_message_NETWORK_SOCKET_BINDER_UPDATE, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,		process_message_SERVER_TIME_EVENT_EVENT_REGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,		process_message_SERVER_TIME_EVENT_EVENT_UNREGISTER, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE,			process_message_SERVER_TIME_EVENT_EVENT_UPDATE, sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE,			process_message_SERVER_TIME_EVENT_EVENT_EXECUTE, sMESSAGE);
	END_NESTED_CGMESSAGE_MAP

	// return) call parent's 'process_message'
	return this->on_message(static_cast<sMESSAGE_NETWORK&>(_msg));
}

result_code server_system::socket::client::on_message_SERVER_ADMIN_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) 값을 설정한다.
	auto result = buf_recv.extract<eRESULT>();

	try
	{
		// 3) 결과에 따라 처리한다.
		switch (result)
		{
		case	eRESULT::SUCCESS:
				this->process_service_instance_prepare();
				break;

		default:
				// - 재접속 간격을 늘린다. (log 포함)
				this->process_certify_failed(result);
				break;
		}
	}
	catch (...)
	{
		// - close socket
		this->closesocket();

		// reraise)
		throw;
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::on_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) get service
		auto pservice = server_system::get_service();

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service instance is not ready"sv));

		// 2) 내용을 읽기 위해...
		shared_buffer buf_recv = _msg.buf_message + offset(8);

		// 3) 값을 설정한다.
		auto result = buf_recv.extract<eRESULT>();

		// 4) 결과에 따라 처리한다.
		switch (result)
		{
		case	eRESULT::SUCCESS:
				{
					// - casting service
					auto pservice_nested = dynamic_pointer_cast<server_system::Xservice>(pservice);

					// check)
					THROW_IF(pservice_nested.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service instance is not ready"sv));

					// - extract setting info
					auto service_info_remote = buf_recv.extract<server_system::sSERVICE>();
					auto map_host_info_confirmed = buf_recv.extract<std::map<server_system::iid_t, server_system::sHOST>>();
					auto map_service_info_confirmed = buf_recv.extract<std::map<server_system::iid_t, server_system::sSERVICE>>();

					// log)
					LOG_PROGRESS << "(!) remote INSTANCE SETTING received."sv << log_endl
						<< "     - iid:"sv << service_info_remote.setting.iid << log_endl
						<< "     - nickname:"sv << service_info_remote.setting.name << log_endl
						<< "     - rank:"sv << service_info_remote.setting.rank.to_string();

					// - register to scheduler 
					pservice_nested->process_socket_client_certified(this, std::move(map_host_info_confirmed), std::move(map_service_info_confirmed));

					// - set remote service info
					this->m_remote_service_info = std::move(service_info_remote);

					// - certified (재접속 간격을 되돌린다.)
					this->m_is_instance_certified = true;
					this->process_certify_succeeded();

					// - register schedulable
					//   (종료 중에 응답이 늦게 오면 system scheduler가 이미 없을 수 있다. 그때는 등록하지 않는다.)
					if (system_executor::schedulable_manager().exist())
						system_executor::register_schedulable(this);
				}
				break;

		case	eRESULT::NOT_EXIST:
				{
					// - 재접속 간격을 늘린다. (log 포함)
					this->process_certify_failed(result);

					// - closesocket
					this->closesocket();
				}
				break;

		default:
				{
					// - 인증 실패는 service를 종료하지 않는다. (설정 실수/일시적인 오류로 운영 중인 service가 내려가지 않도록)
					//   접속을 끊고 늘어난 간격으로 다시 시도한다. (log 포함)
					this->process_certify_failed(result);

					// - closesocket
					this->closesocket();
				}
				break;
		}
	}
	catch (...)
	{
		// - closesocket
		this->closesocket();

		// reraise)
		throw;
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_SERVER_ADMIN_EXCEPTION_DUMP([[maybe_unused]] sMESSAGE_DUMP& _msg)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
	#if defined(_WIN32)
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 1) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// declare) Size Of Message
		const size_t size_of_message = 8 + get_size_of(_msg.dump_info.time_occurred_first) + get_size_of(_msg.dump_info.time_occurred_last) + sizeof(uint64_t) + sizeof(EXCEPTION_RECORD) + get_size_of(_msg.dump_info.dump_filename);

		// 2) shared_buffer
		auto buf_temp = alloc_shared_buffer(size_of_message);

		// 3) Header
		buf_temp.append<uint32_t>();
		buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO);

		// 4) 
		buf_temp.append<chrono::time::utc::time_point>(_msg.dump_info.time_occurred_first);
		buf_temp.append<chrono::time::utc::time_point>(_msg.dump_info.time_occurred_last);
		buf_temp.append<uint64_t>(_msg.dump_info.count_times);
		buf_temp.append<EXCEPTION_RECORD>(_msg.dump_info.sExceptionRecords);
		buf_temp.append<CONTEXT>(_msg.dump_info.sContextRecord);
		buf_temp.append(_msg.dump_info.dump_filename);

		// 5) message size
		buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

		// check) 
		CGASSERT_ERROR(size_of_message == buf_temp.size());

		// 6) send
		this->send(buf_temp);
	#endif
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_FACTORY_REGISTER(sMESSAGE& _msg)
{
	// 1) casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get pfactory
		auto pfactory = reinterpret_cast<Ifactory*>(msg_notify.wParam);

		// 5) sned message
		this->send(make_message_FACTORY_FACTORY_REGISTER(pservice_instance->get_iid(), pfactory));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_FACTORY_UNREGISTER(sMESSAGE& _msg)
{
	// 1) casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get pfactory
		auto pfactory = reinterpret_cast<Ifactory*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pfactory != nullptr);

		// 5) sned message
		this->send(make_message_FACTORY_FACTORY_UNREGISTER(pservice_instance->get_iid(), pfactory));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_FACTORY_UPDATE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_THREAD_REGISTER(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_THREAD_UNREGISTER(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_EXECUTOR_REGISTER(sMESSAGE& _msg)
{
	// 1) casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get executor
		auto pexecutor = reinterpret_cast<Iexecutor*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pexecutor != nullptr);

		// 5) sned message
		this->send(make_message_EXECUTOR_REGISTER(pservice_instance->get_iid(), pexecutor));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_EXECUTOR_UNREGISTER(sMESSAGE& _msg)
{
	// 1) message casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get executor
		auto pexecutor = reinterpret_cast<Iexecutor*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pexecutor != nullptr);

		// 5) sned message
		this->send(make_message_EXECUTOR_UNREGISTER(pservice_instance->get_iid(), pexecutor));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_EXECUTOR_UPDATE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE& _msg)
{
	// 1) message casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get connective
		auto pconnective = reinterpret_cast<net::io::Iconnective*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pconnective != nullptr);

		// 5) sned message
		this->send(make_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(pservice_instance->get_iid(), pconnective));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE& _msg)
{
	// 1) message casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get connective
		auto pconnective = reinterpret_cast<net::io::Iconnective*> (msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pconnective != nullptr);

		// 5) send message
		this->send(make_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(pservice_instance->get_iid(), pconnective));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE& _msg)
{
	// 1) message casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get binder
		auto pbinder = reinterpret_cast<net::io::Ibinder*>(msg_notify.wParam);
		auto pstatistics_socket = reinterpret_cast<net::io::statistics::Nsocket*>(msg_notify.lParam);

		// check)
		CGASSERT_ERROR(pbinder != nullptr);

		// 5) sned message
		this->send(make_message_NETWORK_SOCKET_BINDER_REGISTER(pservice_instance->get_iid(), pbinder, pstatistics_socket));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE& _msg)
{
	// 1) casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get binder
		auto pbinder = reinterpret_cast<net::io::Ibinder*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pbinder != nullptr);

		// 5) sned message
		this->send(make_message_NETWORK_SOCKET_BINDER_UNREGISTER(pservice_instance->get_iid(), pbinder));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_NETWORK_SOCKET_BINDER_UPDATE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg)
{
	// 1) casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get binder
		auto ptime_event = reinterpret_cast<time_event::Ientity*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(ptime_event != nullptr);

		// 5) sned message
		this->send(make_message_SERVER_TIME_EVENT_EVENT_REGISTER(pservice_instance->get_iid(), ptime_event));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg)
{
	// 1) message casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

		// 4) get binder
		auto ptime_event = reinterpret_cast<time_event::Ientity*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(ptime_event != nullptr);

		// 5) sned message
		this->send(make_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(pservice_instance->get_iid(), ptime_event));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::socket::client::process_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& /*_msg*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 2) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), eRESULT::DONE);

		// 3) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), eRESULT::NOT_READY);

	}

	// return)
	return eRESULT::DONE;
}

void server_system::socket::client::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	RETURN_IF(_plog_record.empty());

	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, );

		// 1) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), );

		// 2) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), );

		// 2) send log message
		this->send(server_system::make_message_SYSTEM_LOG(pservice_instance->get_iid(), _plog_record.get()));
	}
}

intptr_t server_system::socket::client::process_execute(intptr_t /*_return*/, size_t /*_Param*/)
{
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		// check)
		RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING, intptr_t(0));

		// 1) reset & get service_instance_local
		auto pservice_instance = this->m_pservice_instance;

		// check)
		RETURN_IF(pservice_instance.empty(), intptr_t(0));

		// 2) get service
		auto pservice = server_system::get_service();

		// check)
		RETURN_IF(pservice.empty(), intptr_t(0));

		// 3) get service
		auto iid = pservice_instance->get_iid();

		// check)
		RETURN_IF(iid == 0, intptr_t(0));

		//// 4) send message
		//this->send(server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(iid, pservice_instance->info.info));
	}

	// return) 
	return intptr_t(0);
}

void server_system::socket::client::process_update_instance_info_status(service::instance_local* /*_pinstance*/, server_system::sSERVICE_STATUS_UPDATE&& /*_service_info_update*/)
{
	// 1) update data
	SCOPED_LOCK(this->m_lockable_pservice)
	{
		//_pinstance->status.vector_process_status.push_back(std::move(_service_info_update.process_status));
		//_pinstance->update_factory_info(std::move(_service_info_update.vector_factory_update));
		//_pinstance->update_executor_info(std::move(_service_info_update.vector_executor_update));
		//_pinstance->update_schedulable_info(std::move(_service_info_update.vector_schedulable_update));
		//_pinstance->update_connective_info(std::move(_service_info_update.vector_connective_update));
		//_pinstance->update_binder_info(std::move(_service_info_update.vector_binder_update));
		//_pinstance->update_connective_statistics(std::move(_service_info_update.connective_info));
		//_pinstance->update_traffic_statistics(std::move(_service_info_update.traffic_info));
	}

	//// notify) to alll
	//this->on_update_instance_info();
}
