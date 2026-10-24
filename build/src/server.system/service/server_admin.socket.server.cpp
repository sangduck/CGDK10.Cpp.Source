#include "pch.h"

using namespace CGDK;


server_system::host::socket::socket()
{
	// 1) set name
	this->name("server_system.host.socket");
}

server_system::host::socket::~socket()
{
}

void server_system::host::socket::register_host_instance(host::Ninstance* _host_instance)
{
	// check)
	CGASSERT_FATAL(_host_instance != nullptr);

	// check)
	THROW_IF(_host_instance == nullptr, throwable::failure(eRESULT::FAIL, 0, ""sv));

	// 1) get_iid
	auto temp_iid = _host_instance->get_iid();

	// check)
	THROW_IF(temp_iid == 0, throwable::failure(eRESULT::INVALID_VALUE, 0, "'host instance iid' is invalid(0)"sv));

	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 2) find existing instance
		auto iter_find = this->m_map_host_instance.find(temp_iid);

		// check) throw if already exist
		THROW_IF(iter_find != this->m_map_host_instance.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "host already registered"sv));

		// 3) register
		this->m_map_host_instance.insert({ temp_iid, _host_instance });

		// 4) set sender
		_host_instance->set_sender(this);
	}
}

bool server_system::host::socket::unregister_host_instance(server_system::iid_t _iid) noexcept
{
	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 1) find
		auto iter_find = this->m_map_host_instance.find(_iid);

		// check)
		RETURN_IF(iter_find == this->m_map_host_instance.end(), false);

		// check)
		RETURN_IF(iter_find->second->get_sender() != this, false);

		// 2) reset info
		iter_find->second->reset_sender();

		// 3) remove
		this->m_map_host_instance.erase(iter_find);
	}

	// return) 
	return true;
}

std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>> server_system::host::socket::unregister_host_instance_all() noexcept
{
	// declare) 
	std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>> map_host_instance;

	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 1) move host instances
		map_host_instance = std::move(m_map_host_instance);

		// 2) remove host instances
		for (auto& iter : map_host_instance)
		{
			// check)
			CGASSERT_ERROR(iter.second.exist());

			// - reset sender
			iter.second->reset_sender();
		}
	}

	// return)
	return map_host_instance;
}

void server_system::host::socket::register_service_instance(service::Ninstance* _service_instance)
{
	// check)
	CGASSERT_FATAL(_service_instance != nullptr);

	// check)
	THROW_IF(_service_instance == nullptr, throwable::failure(eRESULT::FAIL, 0, ""sv));

	// 1) get_iid
	auto temp_iid = _service_instance->get_iid();

	// check)
	THROW_IF(temp_iid == 0, throwable::failure(eRESULT::INVALID_VALUE, 0, "'service instance iid' is invalid(0)"sv));

	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 2) find existing instance
		auto iter_find = this->m_map_service_instance.find(temp_iid);

		// check) throw if already exist
		THROW_IF(iter_find != this->m_map_service_instance.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "service already registered"sv));

		// 3) register
		this->m_map_service_instance.insert({ temp_iid, _service_instance });

		// 4) set sender
		_service_instance->set_sender(this);
	}
}

bool server_system::host::socket::unregister_service_instance(server_system::iid_t _iid) noexcept
{
	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 1) find
		auto iter_find = this->m_map_service_instance.find(_iid);

		// check)
		RETURN_IF(iter_find == this->m_map_service_instance.end(), false);

		// check)
		RETURN_IF(iter_find->second->get_sender() != this, false);

		// 2) reset info
		iter_find->second->reset_sender();

		// 3) remove
		this->m_map_service_instance.erase(iter_find);
	}

	// return)
	return true;
}

std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> server_system::host::socket::unregister_service_instance_all() noexcept
{
	// declare) 
	 std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> map_service_instance;

	SCOPED_LOCK(this->m_lockable_instances)
	{
		// 1) move service instances
		map_service_instance = std::move(m_map_service_instance);

		// 2) remove service instances
		for (auto& iter : map_service_instance)
		{
			// check)
			CGASSERT_ERROR(iter.second.exist());

			// - reset sender
			iter.second->reset_sender();
		}
	}

	// return)
	return map_service_instance;
}


void server_system::host::socket::on_connect()
{
	// 1) set socket option
	this->maximum_message_buffer_size(512 * 1024 * 1024);
	this->maximum_depth_of_send_buffer(4096);
	this->maximum_depth_of_send_buffer_bytes(512 * 1024 * 1024);
}

void server_system::host::socket::on_disconnect(uint64_t /*_reason*/)
{
	// 1) leave group
	this->leave_group();
}

result_code server_system::host::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);
	
	try
	{
		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);

		// 3) message map
		BEGIN_NESTED_CGMESSAGE_MAP
			CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST, on_message_SERVER_ADMIN_CERTIFY_REQUEST, sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST, on_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST, sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
		END_NESTED_CGMESSAGE_MAP
	}
	catch (...)
	{
		// - 잘못된 메시지 (짧거나 깨진 메시지 : 원격 입력이므로 assert하지 않는다. 무시한다.)
		//   (log는 10초에 한 번만 남긴다 : 계속 보내는 상대가 있어도 log가 넘치지 않도록)
		static server_system::log_throttle s_log_invalid_message(std::chrono::seconds(10));
		if (auto suppressed = s_log_invalid_message.try_log(); suppressed.has_value())
			LOG_WARNING << "(warn) invalid admin message ignored (host socket)"sv << ((*suppressed != 0) ? " (+"s + std::to_string(*suppressed) + " suppressed for 10s)" : std::string());
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code server_system::host::socket::on_message_SERVER_ADMIN_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 0) certify info와 key (key는 없을 수 있다.)
		auto buf_recv = _msg.buf_message + offset(8);
		auto certify_info = buf_recv.extract<server_system::sCERTIFY_INFO>();
		std::string_view certify_key = (buf_recv.size() != 0) ? buf_recv.extract<std::string_view>() : std::string_view();

		// check) certify
		THROW_IF(this->process_certify(certify_info, certify_key) == false, throwable::failure(eRESULT::ACCESS_DENIED, 0, "certify key mismatch"sv));

		// 1) get service instance
		auto pservice = server_system::get_service();

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service is not ready!"sv));

		// 1) send response
		auto result = this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::SUCCESS));

		// check)
		THROW_IF(result == false, throwable::failure(eRESULT::FAIL, 0, "fail to send message 'message_SERVER_ADMIN_CERTIFY_RESPONSE'"sv));

		// log)
		LOG_INFO << "> server certified. [ip "sv << to_string(this->remote_endpoint()) << "]"sv;
	}
	catch (throwable::failure& _e)
	{
		// log)
		LOG_ERROR << "< server fail to certifying by ERROR "sv << to_string(_e.reason<eRESULT>()) << "/'"sv << _e.what() << "' [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(_e.reason<eRESULT>()));

		// - close socket
		this->disconnect();
	}
	catch (std::exception& _e)
	{
		// log)
		LOG_ERROR << "< server fail to certifying by ERROR "sv << _e.what() << " [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - close socket
		this->disconnect();
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "< server fail to certifying by EXCEPTION [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - close socket
		this->disconnect();
	}

	// return) 
	return eRESULT::DONE;
}

result_code server_system::host::socket::on_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// 1) get buf
	buffer_view buf_recv = _msg.buf_message + offset(8);

	try
	{
		// 2) get service
		auto pservice_nest = server_system::get_service();

		// check)
		THROW_IF(pservice_nest.empty(), throwable::failure(eRESULT::NOT_READY, 0, "'service' is nullptr"sv));

		// 3) casting service
		auto pservice = dynamic_pointer_cast<server_system::Xservice>(pservice_nest);

		// check)
		THROW_IF(pservice.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service object is not derived from 'server_system::Xservice'"sv));

		// 4) get 
		auto phost_instance_manager = pservice->get_host_instance_manager();

		// check)
		THROW_IF(phost_instance_manager.empty(), throwable::failure(eRESULT::NOT_READY, 0, "host instance manager is nullptr!"sv));

		auto pservice_instance_manager = pservice->get_service_instance_manager();

		// check)
		THROW_IF(pservice_instance_manager.empty(), throwable::failure(eRESULT::NOT_READY, 0, "service instance manager is nullptr!"sv));

		// 5) get group socket server
		auto pgroup_server_socket = phost_instance_manager->get_group_socket();

		// check)
		THROW_IF(pgroup_server_socket.empty(), throwable::failure(eRESULT::FAIL, 0, "'group_server_socket' is not exist"sv));

		// declare)
		std::vector<object_ptr<server_system::host::Ninstance>> vector_host_instance;
		std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>> map_host_instance_update;
		std::vector<object_ptr<server_system::service::Ninstance>> vector_service_instance;
		std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>> map_service_instance_update;

		// 6) extract host instance
		{
			// - get count 
			auto count = buf_recv.extract<uint32_t>();

			for (; count > 0; --count)
			{
				// - make host instance
				auto phost_instance = phost_instance_manager->alloc_instance();

				// - serialize
				phost_instance->serialize_in(buf_recv);

				// - get iid original
				auto iid_original = phost_instance->get_iid();

				// - confirm instance
				auto modified = phost_instance_manager->confirm_instance(phost_instance);

				if (modified == true)
				{
					map_host_instance_update.insert({ iid_original, phost_instance });
				}

				// log)
				LOG_INFO << " - host['"sv << phost_instance->info.setting.name << "'] registered (" << phost_instance->info.setting.iid << ")";

				// - add host instance
				vector_host_instance.push_back(std::move(phost_instance));
			}
		}

		// 7) extract service instance
		{
			// - get count 
			auto count = buf_recv.extract<uint32_t>();

			for (; count > 0; --count)
			{
				// - make service instance
				auto pservice_instance = pservice_instance_manager->alloc_instance();

				// - serialize
				pservice_instance->serialize_in(buf_recv);

				// - get iid original
				auto iid_original = pservice_instance->get_iid();

				// - confirm instance
				auto modified = pservice_instance_manager->confirm_instance(pservice_instance);

				// check) 
				if(modified == true)
					map_service_instance_update.insert({ iid_original, pservice_instance });

				// log)
				LOG_INFO << " - service['"sv << pservice_instance->info.setting.name << "'] registered (iid:" << pservice_instance->info.setting.iid << ")";

				// - add host instance
				vector_service_instance.push_back(std::move(pservice_instance));
			}
		}

		// 8) register instance to manager
		try
		{
			// - map instance
			for (auto& iter : vector_host_instance)
				this->register_host_instance(iter);

			for (auto& iter : vector_service_instance)
				this->register_service_instance(iter);

			// - register host instance
			if (phost_instance_manager.exist())
			{
				// - register instance
				for (auto& iter : vector_host_instance)
					phost_instance_manager->register_instance(iter.get());
			}

			// - register service instance
			if (pservice_instance_manager.exist())
			{
				// - register instance
				for (auto& iter : vector_service_instance)
					pservice_instance_manager->register_instance(iter.get());
			}

			// - send service info
			this->send(make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(pservice->get_service_instance()->info, map_host_instance_update, map_service_instance_update));
		}
		catch (...)
		{
			// - rollback service instance
			if (pservice_instance_manager.exist())
			{
				for (auto& iter : vector_service_instance)
					pservice_instance_manager->unregister_instance(iter);
			}
			for (auto& iter : vector_service_instance)
				this->unregister_service_instance(iter->info.setting.iid);

			// - rollback host instance
			if (phost_instance_manager.exist())
			{
				for (auto& iter : vector_host_instance)
					phost_instance_manager->unregister_instance(iter);
			}

			// - rollback host instance
			for (auto& iter : vector_host_instance)
				this->unregister_host_instance(iter->info.setting.iid);


			// reraise) 
			throw;
		}

		// 9) enter to 
		pgroup_server_socket->enter(this, _msg);
	}
	catch (throwable::failure& _e)
	{   
		// log)
		LOG_ERROR << "< server fail to certifying by ERROR "sv << to_string(_e.reason<eRESULT>()) << "/'"sv << _e.what() << "' [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(_e.reason<eRESULT>()));

		// - close socket
		this->disconnect();
	}
	catch (std::exception& _e)
	{
		// log)
		LOG_ERROR << "< server fail to certifying by ERROR "sv << _e.what() << " [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - close socket
		this->disconnect();
	}
	catch (...)
	{
		// log)
		LOG_ERROR << "< server fail to certifying by EXCEPTION [ip "sv << to_string(this->remote_endpoint()) << "]"sv;

		// - send response
		this->send(server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT::EXCEPTION));

		// - close socket
		this->disconnect();
	}

	// return)
	return eRESULT::DONE;
}

void server_system::sACCESS_POLICY::load(context& _context)
{
	// 1) certify key
	//    ("certify_key_file" > "certify_key" > 환경변수 CGDK_ADMIN_CERTIFY_KEY)
	this->certify_key = get_certify_key_from_setting(_context);

	// 2) allowed addresses
	this->allowed_addresses.clear();
	auto& context_addresses = _context.find("allowed_addresses"sv);
	if (context_addresses.is_valid())
	{
		for (auto& iter : context_addresses.map_nodes)
		{
			// - ':'가 있으면 ipv6, 없으면 ipv4로 읽는다. (make_address는 ipv6를 먼저 시도하므로 쓰지 않는다.)
			auto str_address = iter.second.operator std::string();
			std::error_code error_code;
			net::ip::address address = (str_address.find(':') != std::string::npos) ? net::ip::address(net::ip::make_address_v6(str_address, error_code)) : net::ip::address(net::ip::make_address_v4(str_address, error_code));
			if (!error_code)
				this->allowed_addresses.push_back(normalize(address));
			else
				LOG_WARNING << "(warn) invalid address in 'allowed_addresses': '"sv << iter.second.operator std::string() << "'"sv;
		}
	}
}