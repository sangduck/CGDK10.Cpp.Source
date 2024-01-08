#include "pch.h"

using namespace CGDK;


server_system::service::service() :
	server_system::Nservice()
{
}

server_system::service::service(const std::string_view _name) :
	server_system::Nservice(_name)
{
}

server_system::service::~service()
{
}

shared_buffer message_SERVER_ADMIN_INSTANCE_ENTER(const server_admin::sSERVICE_SETTING& _service_setting, const server_admin::sSERVICE_INSTANCE_INFO& _service_instance_info)
{
	// declare) 
	std::vector<server_admin::sINSTANCE_SERVICE> vector_instance;

	// 1) ..
	{
		// declare) 
		server_admin::sINSTANCE_SERVICE temp_instance_info;

		// - set value
		temp_instance_info.setting = _service_setting;
		temp_instance_info.info = _service_instance_info;
		temp_instance_info.status.iid = temp_instance_info.setting.iid;
		temp_instance_info.status.state = eOBJECT_STATE::NONE;
		temp_instance_info.status.pid = temp_instance_info.info.pid;
		temp_instance_info.status.vector_factory = server_admin::get_snap(factory::manager::instance());
		temp_instance_info.status.vector_executor = server_admin::get_snap(executor::manager::instance());
		temp_instance_info.status.vector_schedulable = server_admin::get_snap(system_executor::schedulable_manager());
		temp_instance_info.status.vector_connective = server_admin::get_snap(net::io::connective::manager::instance());
		temp_instance_info.status.vector_binder = server_admin::get_snap(net::io::binder::manager::instance());
		temp_instance_info.status.vector_time_event = server_admin::get_snap(time_event::entity_manager::instance());
		temp_instance_info.status.connective_info = server_admin::get_snap(net::io::statistics::Nconnective::statistics_get_connective_info_total());
		temp_instance_info.status.traffic_info = server_admin::get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());
		temp_instance_info.status.vector_log_record = server_admin::get_records_of_default_logger_circular_list();
		temp_instance_info.status.time_update = chrono::time::utc::now();

		// - inser service instance info
		vector_instance.push_back(temp_instance_info);
	}

	// 2) make message and return
	return server_admin::message_SERVER_ADMIN_INSTANCE_ENTER(vector_instance);
}


result_code server_system::service::process_message(sMESSAGE& _msg)
{
	// 1) message map
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE,			on_message_SERVER_ADMIN_CERTIFY_RESPONSE,			sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE,on_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE,	sMESSAGE_NETWORK);
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

result_code server_system::service::on_message_SERVER_ADMIN_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) get admin socket
	auto psocket_admin_socket = _msg.get_source<server_admin::socket_to_admin>();

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) 값을 설정한다.
	auto result = buf_recv.extract<eRESULT>();

	// 3) 결과에 따라 처리한다.
	switch(result)
	{
	case	eRESULT::SUCCESS:
			{
				// - get pid and rank
				auto pid = this->m_service_instance_info.pid;
				auto rank = this->m_service_setting.rank;

				// - send ...
				psocket_admin_socket->send(server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(pid, rank));
			}
			break;

	default:
			{
				// log)
				LOG_ERROR << "(err ) fail to certify from ADMIN_SERVER ["sv << to_string<char>(result) << "("sv << static_cast<int>(result) << ")]"sv;

				// 닫는다.
				destroy();
			}
			break;
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) get admin socket
	auto psocket_admin_socket = _msg.get_source<server_admin::socket_to_admin>();

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) 값을 설정한다.
	auto result = buf_recv.extract<eRESULT>();

	// 3) 결과에 따라 처리한다.
	switch(result)
	{
	case	eRESULT::SUCCESS:
			{
				// 4) extract setting info
				auto service_setting = buf_recv.extract<server_admin::sSERVICE_SETTING>();

				// log)
				LOG_PROGRESS << "(!) INSTANCE SETTING received."sv << log_endl
							 << "     - iid:"sv << service_setting.iid << log_endl
							 << "     - nickname:"sv << service_setting.nickname << log_endl
							 << "     - rank:"sv << service_setting.rank.to_string() << log_endl;

				// 5) set set...
				this->setting(std::move(service_setting));
			}
			[[fallthrough]];

	case	eRESULT::NOT_EXIST:
			{

				// 6) call 'on_server_certified'
				this->on_server_certified(_msg.buf_message);

				// 7) send host info
				scoped_lock(psocket_admin_socket->m_lockable_notify)
				{
					// - register messageable
					factory::manager::instance()->register_messageable(psocket_admin_socket);
					executor::manager::instance()->register_messageable(psocket_admin_socket);
					net::io::connective::manager::instance()->register_messageable(psocket_admin_socket);
					net::io::binder::manager::instance()->register_messageable(psocket_admin_socket);
					time_event::entity_manager::instance()->register_messageable(psocket_admin_socket);
					dump_manager::instance()->register_messageable(psocket_admin_socket);
					server_system::Nstatic_service_logger::get_logger()->register_log_targetable(psocket_admin_socket);

					// - 접속되자 마자 ServerInfo를 전송한다.
					psocket_admin_socket->send(::message_SERVER_ADMIN_INSTANCE_ENTER(this->m_service_setting, this->m_service_instance_info));
				}

				// 8) execute interval => 5sec
				psocket_admin_socket->execute_interval(5s);

				// 9) register to scheduler 
				system_executor::register_schedulable(psocket_admin_socket);
			}
			break;

	default:
			{
				// log)
				LOG_ERROR << "(err ) fail to certify from ADMIN_SERVER ["sv << to_string<char>(result) << "("sv << static_cast<int>(result) << ")]"sv;

				// 닫는다.
				destroy();
			}
			break;
	}

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(sMESSAGE_NETWORK& _msg)
{
	// check) shared_buffer의 data_가 nullptr면 안됀다!!!
	CGASSERT(_msg.buf_message.data() != nullptr, int(0));

	// declare)
	shared_buffer buf_read = _msg.buf_message + offset(8);

	// 1) iid
	auto iid = buf_read.extract<server_admin::iid_t>();
	auto host_iid = buf_read.extract<server_admin::iid_t>();

	// 2) sSERVICE_SETTING Info를 설정한다.
	auto temp_service_setting = buf_read.extract<server_admin::sSERVICE_SETTING>();

	// trace) 
	LOG_INFO << "> SERVER SETTING['"sv << temp_service_setting.nickname << "'] received"sv;
	LOG_INFO << "  - iid: "sv << iid;
	LOG_INFO << "  - host_id: "sv << host_iid;
	LOG_INFO << "  - nickmake: '"sv << temp_service_setting.nickname << "'";
	LOG_INFO << "  - rank: "sv << temp_service_setting.rank.to_string();

	if (iid != this->m_service_setting.iid)
	{
		LOG_WARNING << "  (!) iid changed! ["sv << this->m_service_setting.iid << "] -> ["sv << temp_service_setting.iid << "]"sv;
	}

	// 4) server setting Info를 설정한다.
	this->setting(std::move(temp_service_setting));
	this->m_service_instance_info.host_iid = host_iid;

	// 5) Service정보를 Update한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::SERVER_STATUS_UPDATE, reinterpret_cast<uintptr_t>(nullptr)));

	// return) 
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Notify한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE));

	// 2) execute 'initialize' command
	initialize(); // 여기(왜 initialize()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 주의) 여기서 destory()를 직접 호출하면 된다.
	//       - connect_requestable socket의 송수신 중에 destroy를 호출할 경우 종료처리가 꼬일 수 있다.
	//       - 왜냐면 socket의 on_message 과정에서 소멸을 기다리는 것이 되므로 데드락에 걸리는 셈이다.
	//       - 따라서 꼭 post를 사용해서 종료처리르 해야 한다.

	// 1) Notify한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE));

	// 2) hold
	object_ptr<service> pself(this);

	// 3) execute 'destroy' command
	system_executor::post(make_executable([=] { pself->destroy(); }));

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_START_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Notify한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE));

	// 2) execute 'start' command
	this->start(); // 여기(왜 start()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) notify to service
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE));

	// 2) execute 'stop' command
	this->stop();

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Notify한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE));

	// 2) execute 'pause' command
	this->pause(); // 여기(왜 pause()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(sMESSAGE_NETWORK& /*_msg*/)
{
	// 1) Notify한다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE));

	// 2) execute 'resume' command
	this->resume(); // 여기(왜 continue()를 호출하지 않게 해 놓았었는지 의문) 

	// return)
	return eRESULT::DONE;
}

result_code server_system::service::on_message_SYSTEM_LOG(sMESSAGE_NETWORK& _msg)
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
	plogger->trace(vector_log_record);

	// return)
	return eRESULT::DONE;
}

void server_system::service::process_state_changed(eOBJECT_STATE _status_new, eOBJECT_STATE _state_old)
{
	// Message) Openning상태라고 Message를 날린다.
	transmit_message(sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, eMESSAGE::SERVER::ADMIN::UPDATE_SERVICE_STATUS, static_cast<uintptr_t>(_status_new)));

	// declare) 
	object_ptr< server_admin::socket_to_admin> psocket_to_admin;

	// 1) get socket
	scoped_lock(this->m_psocket_to_admin)
	{
		psocket_to_admin = this->m_psocket_to_admin;
	}

	// 2) HostAgent에 전송한다.
	if(psocket_to_admin.exist())
	{
		try
		{
			// declare)
			auto temp_buffer = server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(this->m_service_setting.iid, _status_new, _state_old);

			// - 전송한다.
			psocket_to_admin->send(temp_buffer);
		}
		catch (...)
		{
		}
	}
}

result_code server_system::service::_process_initializing(context& _context)
{
	// 1) on_service_setting
	on_service_setting(_context);

	// declare) 
	context context_socket_admin;
	uint64_t iid = 0;

	// 2) service
	auto context_service = _context.find("service");


	if(context_service.is_valid())
	{
		// declare)
		server_admin::sSERVICE_SETTING temp_setting = std::move(this->m_service_setting);

		auto context_setting = context_service.find("setting");

		if (context_setting.is_valid())
		{
			// - setting.name
			{
				// - get 'name'
				auto context_name = context_setting.find("name"sv);

				// - set 'setting name'
				if (context_name.is_valid())
				{
					// - get name
					std::string str_name = context_name;

					// - name을 설정한다.
					set_name(str_name);

					// - name을 m_server_setting_info에  설정한다.
					temp_setting.nickname = str_name;

					// trace) 
					LOG_INFO << "   + setting.name: '"sv << str_name << "'"sv;
				}
			}

			// - setting.comment
			{
				// - get 'comment'
				auto context_comment = context_setting.find("comment"sv);

				// - set 'comment'
				if (context_comment.is_valid())
				{
					// - get 'comment' string
					std::string str_comment = context_comment;

					// - 'comment'를 m_server_setting_info에  설정한다.
					temp_setting.nickname = str_comment;

					// trace) 
					LOG_INFO << "   + setting.comment: '"sv << str_comment << "'"sv;
				}
			}

			// - setting.rank
			{
				// - get 'rank' context
				auto context_rank = context_setting.find("rank"sv);

				// - rank 가 있는가?
				if (context_rank.is_valid())
				{
					// declare) 
					server_admin::sRANK rank = temp_setting.rank;

					// - get 'group'
					{
						auto context_group = context_rank.find("group"sv);
						if (context_group.is_valid())
						{
							// - set value
							rank.group = context_group;

							// trace) 
							LOG_INFO << "   + setting.rank.group: "sv << rank.group;
						}
					}

					// - get 'field'
					{
						auto context_field = context_rank.find("field"sv);
						if (context_field.is_valid())
						{
							// - set value
							rank.field = context_field;

							// trace) 
							LOG_INFO << "   + setting.rank.field: "sv << rank.field;
						}
					}

					// - get 'role'
					{
						auto context_role = context_rank.find("role"sv);
						if (context_role.is_valid())
						{
							// - set value
							rank.role = context_role;

							// trace) 
							LOG_INFO << "   + setting.rank.role: "sv << rank.role;
						}
					}

					// - get 'index'
					{
						auto context_index = context_rank.find("index"sv);
						if (context_index.is_valid())
						{
							// - set value
							rank.index = context_index;

							// trace) 
							LOG_INFO << "   + setting.rank.index: "sv << rank.index;
						}
					}

					// - set rank
					temp_setting.rank = rank;

					// trace) 
					LOG_PROGRESS << "   ! servicee.rank: " 
								 << temp_setting.rank.group
								 << "." << temp_setting.rank.field
								 << "." << temp_setting.rank.role
								 << "." << temp_setting.rank.index;
				}
			}
		}

		// - id
		{
			// - name 필드를 얻는다.
			auto context_iid = context_service.find("iid"sv);

			// - name 필드가 있으면 Name으로 설정한다.
			if (context_iid.is_valid())
			{
				iid = (uint64_t)context_iid;

				// trace) 
				LOG_INFO << "   + instance.iid: "sv << iid;
			}
		}

		// - admin_server
		{
			context_socket_admin = context_service.find("socket_to_admin"sv);
		}

		// - apply to setting
		this->m_service_setting = std::move(temp_setting);
	}

	// - set default value
	{
		// - set unique iid (임시 random값)
		if (iid == 0)
		{
			iid = (((uint64_t)rand()) << 32) || rand();
		}

		// - set iid
		this->m_service_setting.iid = iid;

		// trace) 
		LOG_PROGRESS << "   - instance.iid: "sv << iid;

		// - set process id
		this->m_service_instance_info.pid = api::get_process_id();
	}

	// 4) pre initinailzaing
	_process_initializing_pre(_context);

	// 5) on_initializing 함수를 호출한다.
	on_initializing(_context);

	// 6) pre initinailzaing
	_process_initializing_post(_context);

	// 7) default executor 
	system_executor::initialize_instance(_context);

	// 8) socket_admin을 시작한다.
	if(context_socket_admin.is_valid())
	{
		// - get socket_to_admin socket
		auto psocket_to_admin = make_own<server_admin::socket_to_admin>();

		// - get value
		std::string str_value = context_socket_admin;
		std::string str_value_lower = __to_lower(std::string_view(str_value));

		// - A. default
		if (str_value_lower == "default"sv)
		{
			// trace) 
			LOG_PROGRESS << "   + socket to admin: default(localhost:22044)'"sv;

			psocket_to_admin->enable_reconnect();
			psocket_to_admin->start({ { net::ip::address_v4::loopback(), 22044 }, {}, chrono::tick::seconds{5}, 0, 0, 0});
		}
		// - B. start socket_admin
		else
		{
			// trace) 
			LOG_PROGRESS << "   + socket to admin: 'custom address'"sv;

			psocket_to_admin->start(context_service);
		}

		// - socket_admin에 register_messageable한다.
		psocket_to_admin->register_messageable(this);

		scoped_lock(this->m_psocket_to_admin)
		{
			this->m_psocket_to_admin = std::move(psocket_to_admin);
			this->m_psocket_to_admin->set_service(this);

		}
	}
	else
	{
		LOG_WARNING << "   ! 'socket_to_admin' don't started. "sv;
	}

	// return)
	return eRESULT::SUCCESS;
}

void server_system::service::_process_initializing_pre(context& /*_context*/)
{
}

void server_system::service::_process_initializing_post(context& /*_context*/)
{
}

result_code server_system::service::_process_initialize(context& _context)
{
	// 1) initialize pre
	_process_initialize_pre(_context);

	// 2) process initialize
	auto result = Nservice::_process_initialize(_context);

	// 3) initialize post
	_process_initialize_post(_context);

	// return)
	return result;
}

void server_system::service::_process_initialize_pre(context& /*_context*/)
{
}

void server_system::service::_process_initialize_post(context& /*_context*/)
{
}

result_code server_system::service::_process_destroying() noexcept
{
	// 1) ...
	_process_destroy_pre();

	// 2) on_destroying 함수를 호출한다.
	on_destroying();

	// 3) ...
	_process_destroy_post();

	// return)
	return eRESULT::SUCCESS;
}

void server_system::service::_process_destroying_pre() noexcept
{
}

void server_system::service::_process_destroying_post() noexcept
{
}

result_code server_system::service::_process_destroy() noexcept
{
	// 1) ...
	_process_destroy_pre();

	// 2) ...
	Nservice::_process_destroy();

	// 3)...
	_process_destroy_post();

	// declare) 
	own_ptr<server_admin::socket_to_admin> psocket_to_admin;

	// 4) get socket admin
	scoped_lock(m_psocket_to_admin)
	{
		// - move out socket_admin
		psocket_to_admin = std::move(m_psocket_to_admin);

		// - reset pservice 
		if (psocket_to_admin.exist())
		{
			psocket_to_admin->reset_service();
		}
	}

	// 5) destroy socket admin
	if (psocket_to_admin.exist())
	{
		psocket_to_admin->disable_reconnect();
		psocket_to_admin->closesocket();
	}

	// return)
	return eRESULT::SUCCESS;
}

void server_system::service::_process_destroy_pre() noexcept
{
}

void server_system::service::_process_destroy_post() noexcept
{
}

void server_system::service::send_to_admin_server(const shared_buffer& _buffer)
{
	// declare) 
	object_ptr<server_admin::socket_to_admin> psocket_to_admin;

	scoped_lock(this->m_psocket_to_admin)
	{
		// check) 
		RETURN_IF(this->m_psocket_to_admin.empty());

		// - copy
		psocket_to_admin = this->m_psocket_to_admin;
	}

	// 1) send
	psocket_to_admin->send(_buffer);
}
