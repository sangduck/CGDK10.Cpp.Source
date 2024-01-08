#include "pch.h"

using namespace CGDK;


server_admin::socket_to_admin::socket_to_admin():
	net::io::connect_requestable::Ntcp_reconnect("socket_to_admin")
{
}

server_admin::socket_to_admin::~socket_to_admin()
{
}

void server_admin::socket_to_admin::set_service(server_system::service* _pservice) noexcept
{
	// check)
	CGASSERT_ERROR(_pservice != nullptr);

	scoped_lock(this->m_lockable_notify)
	{
		this->m_pservice = _pservice;
	}
}

void server_admin::socket_to_admin::reset_service() noexcept
{
	scoped_lock(this->m_lockable_notify)
	{
		this->m_pservice.reset();
	}
}

intptr_t server_admin::socket_to_admin::process_execute(intptr_t /*_return*/, size_t /*_Param*/)
{
	scoped_lock(this->m_lockable_notify)
	{
		// 1) get service
		auto pservice = this->m_pservice;

		// check)
		RETURN_IF(pservice.empty(), intptr_t(0));

		// 2) send
		this->send(server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(pservice->setting().iid));
	}

	// return) 
	return intptr_t(0);
}

void server_admin::socket_to_admin::on_connect()
{
	// 1) set socket option
	this->maximum_message_buffer_size(512 * 1024 * 1024);
	this->maximum_depth_of_send_buffer(4096);
	this->maximum_depth_of_send_buffer_bytes(512 * 1024 * 1024);

	// 2) make certification message
	server_admin::sCERTIFY_INFO certification_info;
	certification_info.instance_type = server_admin::eINSTANCE_TYPE::SERVICE;

	// 3) send request message
	this->send(server_admin::message_SERVER_ADMIN_CERTIFY_REQUEST(certification_info));
}

void server_admin::socket_to_admin::on_disconnect(uint64_t /*_reason*/)
{
	// 1) unregister messageable
	scoped_lock(this->m_lockable_notify)
	{
		// - register...
		factory::manager::instance()->unregister_messageable(this);
		executor::manager::instance()->unregister_messageable(this);
		net::io::connective::manager::instance()->unregister_messageable(this);
		net::io::binder::manager::instance()->unregister_messageable(this);
		time_event::entity_manager::instance()->unregister_messageable(this);
		dump_manager::instance()->unregister_messageable(this);

		// - log targetable
		server_system::Nstatic_service_logger::get_logger()->unregister_log_targetable(this);
	}

	// 2) detach from scheduler
	system_executor::unregister_schedulable(this);
}

result_code server_admin::socket_to_admin::process_message(sMESSAGE& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// 3) execute message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE(eMESSAGE::DUMP,									on_message_SERVER_ADMIN_EXCEPTION_DUMP,				sMESSAGE_DUMP);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_REGISTER,				on_message_FACTORY_REGISTER,						sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_UNREGISTER,			on_message_FACTORY_UNREGISTER,						sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::FACTORY::FACTORY_UPDATE,				on_message_FACTORY_UPDATE,							sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::THREAD_REGISTER,						on_message_THREAD_REGISTER,							sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::THREAD_UNREGISTER,						on_message_THREAD_UNREGISTER,						sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_REGISTER,						on_message_EXECUTOR_REGISTER,						sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_UNREGISTER,					on_message_EXECUTOR_UNREGISTER,						sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::EXECUTOR_UPDATE,						on_message_EXECUTOR_UPDATE,							sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER,	on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER,		sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER,on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER,	sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE,	on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE,		sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER,		on_message_NETWORK_SOCKET_BINDER_REGISTER,			sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER,	on_message_NETWORK_SOCKET_BINDER_UNREGISTER,		sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::NETWORK::SOCKET::BINDER_UPDATE,		on_message_NETWORK_SOCKET_BINDER_UPDATE,			sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER,	on_message_SERVER_TIME_EVENT_EVENT_REGISTER,		sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER,	on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER,		sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_UPDATE,		on_message_SERVER_TIME_EVENT_EVENT_UPDATE,			sMESSAGE);
		CGMESSAGE_TYPE(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE,		on_message_SERVER_TIME_EVENT_EVENT_EXECUTE,			sMESSAGE);

	END_NESTED_CGMESSAGE_MAP

	// return) call parent's 'process_message'
	return net::io::messageable::tcp::Ndefault::process_message(_msg);
}

result_code server_admin::socket_to_admin::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	// return)
	return eRESULT::BYPASS;
}

#if defined(_WIN32)
result_code server_admin::socket_to_admin::on_message_SERVER_ADMIN_EXCEPTION_DUMP(sMESSAGE_DUMP& _msg)
{
	// declare) Size Of Message
	const size_t size_of_message = 8 + get_size_of(_msg.dump_info.time_occure_first) + get_size_of(_msg.dump_info.time_occure_last) + sizeof(uint64_t) + sizeof(EXCEPTION_RECORD) + get_size_of(_msg.dump_info.dump_filename);

	// 1) shared_buffer를 할당한다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Header값을 써넣는다.
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO);

	// 3) 값을 써넣는다.
	buf_temp.append<chrono::time::utc::time_point>(_msg.dump_info.time_occure_first);
	buf_temp.append<chrono::time::utc::time_point>(_msg.dump_info.time_occure_last);
	buf_temp.append<uint64_t>(_msg.dump_info.count_times);
	buf_temp.append<EXCEPTION_RECORD>(_msg.dump_info.sExceptionRecords);
	buf_temp.append<CONTEXT>(_msg.dump_info.sContextRecord);
	buf_temp.append(_msg.dump_info.dump_filename);

	// 4) Message의 길이를 쓴다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check) 
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// 5) 전송한다.
	this->send(buf_temp);

	// return) 
	return eRESULT::DONE;
}
#elif defined(__linux__)
result_code server_admin::socket_to_admin::on_message_SERVER_ADMIN_EXCEPTION_DUMP(sMESSAGE_DUMP& /*_msg*/)
{
	// 임시) 
	CGASSERT_ERROR(false);

	// return) 
	return eRESULT::DONE;
}
#else
	#error "[CGDK10] Not supported Android Platform"
#endif

result_code server_admin::socket_to_admin::on_message_FACTORY_REGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get pfactory
		auto pfactory = reinterpret_cast<Ifactory*>(msg_notify.wParam);

		// 2) sned message
		this->send(message_FACTORY_FACTORY_REGISTER(this->m_pservice->setting().iid, pfactory));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_FACTORY_UNREGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get pfactory
		auto pfactory = reinterpret_cast<Ifactory*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pfactory != nullptr);

		// 2) sned message
		this->send(message_FACTORY_FACTORY_UNREGISTER(this->m_pservice->setting().iid, pfactory));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_FACTORY_UPDATE(sMESSAGE& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_THREAD_REGISTER(sMESSAGE& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_THREAD_UNREGISTER(sMESSAGE& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_EXECUTOR_REGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get executor
		auto pexecutor = reinterpret_cast<Iexecutor*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pexecutor != nullptr);

		// 2) sned message
		this->send(message_EXECUTOR_REGISTER(this->m_pservice->setting().iid, pexecutor));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_EXECUTOR_UNREGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get executor
		auto pexecutor = reinterpret_cast<Iexecutor*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pexecutor != nullptr);

		// 2) sned message
		this->send(message_EXECUTOR_UNREGISTER(this->m_pservice->setting().iid, pexecutor));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_EXECUTOR_UPDATE(sMESSAGE& /*_msg*/)
{
	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get connective
		auto pconnective = reinterpret_cast<net::io::Iconnective*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pconnective != nullptr);

		// 2) sned message
		this->send(message_NETWORK_SOCKET_CONNECTIVE_REGISTER(this->m_pservice->setting().iid, pconnective));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get connective
		auto pconnective = reinterpret_cast<net::io::Iconnective* > (msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pconnective != nullptr);

		// 2) send message
		this->send(message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(this->m_pservice->setting().iid, pconnective));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(sMESSAGE& /*_msg*/)
{
	scoped_lock(this->m_lockable_notify)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_BINDER_REGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 주의) 여기에서 _pbinder는 생성자에서 호출되는 것이므로 dynamic_cast를 하면 오류가 발생할 수 있다는 점을 유의해야 한다.

		// 1) get binder
		auto pbinder = reinterpret_cast<net::io::Ibinder*>(msg_notify.wParam);
		auto pstatistics_socket = reinterpret_cast<net::io::statistics::Nsocket*>(msg_notify.lParam);

		// check)
		CGASSERT_ERROR(pbinder != nullptr);

		// 2) sned message
		this->send(message_NETWORK_SOCKET_BINDER_REGISTER(this->m_pservice->setting().iid, pbinder, pstatistics_socket));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_BINDER_UNREGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get binder
		auto pbinder = reinterpret_cast<net::io::Ibinder*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(pbinder != nullptr);

		// 2) sned message
		this->send(message_NETWORK_SOCKET_BINDER_UNREGISTER(this->m_pservice->setting().iid, pbinder));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_NETWORK_SOCKET_BINDER_UPDATE(sMESSAGE& /*_msg*/)
{
	scoped_lock(this->m_lockable_notify)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_SERVER_TIME_EVENT_EVENT_REGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get binder
		auto ptime_event = reinterpret_cast<time_event::Ientity*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(ptime_event != nullptr);

		// 2) sned message
		this->send(message_SERVER_TIME_EVENT_EVENT_REGISTER(this->m_pservice->setting().iid, ptime_event));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE& _msg)
{
	// - casting
	auto& msg_notify = static_cast<sMESSAGE_NOTIFY&>(_msg);

	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, eRESULT::NOT_READY);

		// 1) get binder
		auto ptime_event = reinterpret_cast<time_event::Ientity*>(msg_notify.wParam);

		// check)
		CGASSERT_ERROR(ptime_event != nullptr);

		// 2) sned message
		this->send(message_SERVER_TIME_EVENT_EVENT_UNREGISTER(this->m_pservice->setting().iid, ptime_event));
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_SERVER_TIME_EVENT_EVENT_UPDATE(sMESSAGE& /*_msg*/)
{
	scoped_lock(this->m_lockable_notify)
	{
	}

	// return)
	return eRESULT::DONE;
}

result_code server_admin::socket_to_admin::on_message_SERVER_TIME_EVENT_EVENT_EXECUTE(sMESSAGE& /*_msg*/)
{
	scoped_lock(this->m_lockable_notify)
	{
	}

	// return)
	return eRESULT::DONE;
}

void server_admin::socket_to_admin::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	scoped_lock(this->m_lockable_notify)
	{
		// check)
		RETURN_IF(this->m_pservice == nullptr, );

		// - send log message
		this->send(server_admin::message_SYSTEM_LOG(this->m_pservice->setting().iid, _vector_log_record));
	}
}
