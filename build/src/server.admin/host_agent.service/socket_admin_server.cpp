#include "pch.h"


void socket_admin_server::on_initialize(context& /*_context*/)
{
	// log)
	LOG_INFO << " - initialize SOCKET['"sv << name() << "']"sv;
}

void socket_admin_server::on_destroy()
{
	// log)
	LOG_INFO << " - destroy SOCKET['"sv << name() << "']"sv;
}

void socket_admin_server::on_start(context& _context)
{
	// 1) certify key ('certify_key'가 없으면 환경변수 CGDK_ADMIN_CERTIFY_KEY) / remote endpoint (reload에서 비교한다.)
	//    (lock : on_connect(socket thread)가 읽는다.)
	{
		auto certify_key = _get_certify_key(_context);
		auto str_remote_endpoint = _get_remote_endpoint_string(_context);

		std::lock_guard lock(this->m_lockable_setting);
		this->m_certify_key = std::move(certify_key);
		this->m_str_remote_endpoint = std::move(str_remote_endpoint);
	}

	// 2) reconnect
	this->enable_reconnect();
}

std::string socket_admin_server::_get_certify_key(context& _context)
{
	// - "certify_key_file" > "certify_key" > 환경변수 CGDK_ADMIN_CERTIFY_KEY
	return server_system::get_certify_key_from_setting(_context);
}

std::string socket_admin_server::_get_remote_endpoint_string(context& _context)
{
	auto& context_endpoint = _context.find("remote_endpoint"sv);
	if (context_endpoint.is_invalid())
		return std::string();

	auto& context_address = context_endpoint.find("address"sv);
	auto& context_port = context_endpoint.find("port"sv);
	return (context_address.is_valid() ? context_address.operator std::string() : std::string()) + ":" + (context_port.is_valid() ? std::to_string(static_cast<int64_t>(context_port)) : std::string());
}

bool socket_admin_server::apply_setting(context& _context)
{
	// check) 시작하지 않았으면 (start할 때 읽는다.)
	if (this->get_object_state() != eOBJECT_STATE::RUNNING)
		return false;

	// 1) 바뀌었나?
	auto certify_key = _get_certify_key(_context);
	auto str_remote_endpoint = _get_remote_endpoint_string(_context);

	bool is_same_key = false;
	{
		std::lock_guard lock(this->m_lockable_setting);
		is_same_key = (certify_key == this->m_certify_key);
		if (is_same_key && str_remote_endpoint == this->m_str_remote_endpoint)
			return false;
	}

	// log)
	LOG_INFO << " - 'socket_to_admin_server' setting changed (remote_endpoint: '"sv << str_remote_endpoint << "', certify_key: "sv << (is_same_key ? "same"sv : "changed"sv) << "). reconnect"sv;

	// 2) 다시 시작한다. (stop: 재접속을 끄고 접속을 끊는다. start: 새 주소/key로 접속한다.)
	this->stop();
	this->start(_context);

	// return)
	return true;
}

void socket_admin_server::on_stop()
{
	this->disable_reconnect();
}

void socket_admin_server::on_connect()
{
	// 1) reset certified
	this->m_is_certified = false;
	this->m_is_failure_counted = false;

	// 2) send certify request
	server_system::sCERTIFY_INFO certify_info;
	certify_info.dummy = ::eINSTANCE_TYPE::HOST;

	std::string certify_key;
	{
		std::lock_guard lock(this->m_lockable_setting);
		certify_key = this->m_certify_key;
	}
	this->send(server_system::make_message_SERVER_ADMIN_CERTIFY_REQUEST(certify_info, certify_key));
}

void socket_admin_server::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	// 1) 인증되기 전에 끊겼으면 (admin server가 거부하고 끊은 경우 등) 실패로 센다.
	if (this->m_is_certified == false && this->m_is_failure_counted == false)
		this->process_certify_failed(eRESULT::FAIL);

	// 2) reset certified
	this->m_is_certified = false;
}

bool socket_admin_server::send_if_certified(const shared_buffer& _buffer)
{
	// check)
	RETURN_IF(this->m_is_certified == false, false);

	// 1) send
	return this->send(_buffer);
}

void socket_admin_server::process_certify_failed(eRESULT _result)
{
	//-----------------------------------------------------------------
	// 설명) 인증 실패가 계속되면 재접속 간격을 늘린다. (최대 60초)
	//  - 로그는 1, 2, 4, 8...번째 실패에만 남긴다.
	//-----------------------------------------------------------------

	// 1) 기본 재접속 간격
	if (this->m_reconnect_interval_base == chrono::tick::duration::zero())
	{
		this->m_reconnect_interval_base = this->get_reconnect_interval();

		// - 1초보다 짧으면 1초로 한다.
		if (this->m_reconnect_interval_base < chrono::tick::seconds{ 1 })
			this->m_reconnect_interval_base = chrono::tick::seconds{ 1 };
	}

	// 2) count (접속 한 번에 한 번만 센다.)
	if (this->m_is_failure_counted)
		return;
	this->m_is_failure_counted = true;
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

void socket_admin_server::process_certify_succeeded()
{
	// 1) reset
	this->m_count_certify_fail = 0;

	// 2) 재접속 간격을 되돌린다.
	if (this->m_reconnect_interval_base != chrono::tick::duration::zero())
		this->set_reconnect_interval(this->m_reconnect_interval_base);
}

void socket_admin_server::send_request_result(server_system::iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message)
{
	// - 요청에 대한 응답이므로 인증 여부와 상관없이 보낸다. (INSTANCE_CERTIFY_RESPONSE보다 요청이 먼저 올 수 있다.)
	this->send(server_system::make_message_SERVER_ADMIN_REQUEST_RESULT(_iid, _request, _request_id, _result, _message));
}

result_code socket_admin_server::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) set source
		_msg.set_source(this);

		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);

		// 3) execute message map
		BEGIN_NESTED_CGMESSAGE_MAP
			CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE,			on_message_server_system_CERTIFY_RESPONSE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE,	on_message_server_system_INSTANCE_CERTIFY_RESPONSE,	sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING,		on_message_server_system_REQUEST_UPDATE_SETTING,	sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE,		on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE,	on_message_server_system_REQUEST_SERVICE,			sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
		END_NESTED_CGMESSAGE_MAP
	}
	catch(...)
	{
	}

	// return)
	return eRESULT::BYPASS;
}

result_code socket_admin_server::on_message_server_system_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) 값을 설정한다.
	auto result = buf_recv.extract<eRESULT>();

	// 3) 결과에 따라 처리한다.
	switch (result)
	{
	case	eRESULT::SUCCESS:
			{
				// - host와 service instance들을 인증 요청한다. (INSTANCE_CERTIFY_REQUEST)
				this->send(this->process_make_message_INSTANCE_CERTIFY_REQUEST());
			}
			break;

	default:
			{
				// - 재접속 간격을 늘린다. (log 포함)
				this->process_certify_failed(result);
			}
			break;
	}

	// return)
	return eRESULT::DONE;
}

shared_buffer socket_admin_server::process_make_message_INSTANCE_CERTIFY_REQUEST()
{
	//-----------------------------------------------------------------
	// 설명) admin server로 보내는 instance 인증 요청
	//  - message: COUNT_T n x host::Ninstance     (이 host agent 자신 1개)
	//             COUNT_T m x service::Ninstance  (server list의 service들)
	//  - (server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST와 같은 layout)
	//-----------------------------------------------------------------

	// declare)
	auto& phost_self = ::service::pmanager_instance->self();
	std::vector<object_ptr<server_system::instance_service>> vector_service;

	// 1) host iid
	auto host_iid = phost_self->get_iid();

	// 2) service instance들을 얻는다.
	SCOPED_LOCK(::service::pmanager_instance->m_lockable_instance)
	{
		for (auto& iter : ::service::pmanager_instance->m_map_instance)
		{
			auto pinstance_service = dynamic_pointer_cast<server_system::instance_service>(object_ptr<server_system::Iinstance>(iter.second));

			// check)
			if (pinstance_service.empty())
				continue;

			vector_service.push_back(std::move(pinstance_service));
		}
	}

	// 3) size of message
	std::size_t size_of_message = 8;
	size_of_message += sizeof(COUNT_T);
	size_of_message += phost_self->m_instance.get_size_of();
	size_of_message += sizeof(COUNT_T);
	for (auto& iter : vector_service)
	{
		SCOPED_LOCK(iter->m_lockable_instance)
		{
			iter->m_instance.host_instance_id = host_iid;
			size_of_message += iter->m_instance.get_size_of();
		}
	}

	// 4) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 5) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST);

	// 6) host instance (host agent 자신)
	buf_temp.append<COUNT_T>(1);
	phost_self->m_instance.serialize_out(buf_temp);

	// 7) service instances
	buf_temp.append<COUNT_T>(static_cast<COUNT_T>(vector_service.size()));
	for (auto& iter : vector_service)
	{
		SCOPED_LOCK(iter->m_lockable_instance)
		{
			iter->m_instance.serialize_out(buf_temp);
		}
	}

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

result_code socket_admin_server::on_message_server_system_INSTANCE_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) admin server의 instance 인증 응답
	//  - SUCCESS: sSERVICE(admin), map<iid_original, sHOST>, map<iid_original, sSERVICE>
	//             (map에는 admin server가 변경한 instance만 들어 있다.)
	//  - 실패   : eRESULT만
	//-----------------------------------------------------------------

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) result
	auto result = buf_recv.extract<eRESULT>();

	// check)
	if (result != eRESULT::SUCCESS)
	{
		// - 재접속 간격을 늘린다. (log 포함)
		this->process_certify_failed(result);

		// - NOT_EXIST이면 접속을 끊고 다시 시도한다. (reconnect)
		if (result == eRESULT::NOT_EXIST)
			this->closesocket();

		// return)
		return eRESULT::DONE;
	}

	// 3) extract
	auto service_info_admin = buf_recv.extract<server_system::sSERVICE>();
	auto map_host_info_confirmed = buf_recv.extract<std::map<server_system::iid_t, server_system::sHOST>>();
	auto map_service_info_confirmed = buf_recv.extract<std::map<server_system::iid_t, server_system::sSERVICE>>();

	// log)
	LOG_PROGRESS << "(!) certified to ADMIN_SERVER ['"sv << service_info_admin.setting.name << "' iid:"sv << service_info_admin.setting.iid << "]"sv;

	// 4) 변경된 host 정보 (iid 변경은 지원하지 않는다. name/rank 등만 적용한다.)
	for (auto& iter : map_host_info_confirmed)
	{
		auto& phost_self = ::service::pmanager_instance->self();

		// check)
		if (iter.first != phost_self->get_iid())
			continue;

		if (iter.second.setting.iid != iter.first)
		{
			LOG_WARNING << "(warn) ADMIN_SERVER changed HOST iid ["sv << iter.first << "] -> ["sv << iter.second.setting.iid << "] (ignored)"sv;
			iter.second.setting.iid = iter.first;
		}

		SCOPED_LOCK(phost_self->m_lockable_instance)
		{
			phost_self->m_instance.info.setting = std::move(iter.second.setting);
		}
	}

	// 5) 변경된 service 정보
	for (auto& iter : map_service_info_confirmed)
	{
		auto pinstance_service = dynamic_pointer_cast<server_system::instance_service>(::service::pmanager_instance->find_instance(iter.first));

		// check)
		if (pinstance_service.empty())
			continue;

		if (iter.second.setting.iid != iter.first)
		{
			LOG_WARNING << "(warn) ADMIN_SERVER changed SERVICE iid ["sv << iter.first << "] -> ["sv << iter.second.setting.iid << "] (ignored)"sv;
			iter.second.setting.iid = iter.first;
		}

		SCOPED_LOCK(pinstance_service->m_lockable_instance)
		{
			pinstance_service->m_instance.info.setting.rank = iter.second.setting.rank;
		}
	}

	// 6) certified (이후 상태 변경을 admin server로 전달한다.)
	this->m_is_certified = true;
	this->process_certify_succeeded();

	// return)
	return eRESULT::DONE;
}

result_code socket_admin_server::on_message_server_system_REQUEST_UPDATE_SETTING(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) admin server의 setting 변경 요청
	//  - message: iid_t, iid_t host_iid, sSERVICE::sSETTING
	//  - host agent의 setting을 바꾸고 접속된 service에도 전달한다.
	//-----------------------------------------------------------------

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) extract
	auto iid = buf_recv.extract<server_system::iid_t>();
	[[maybe_unused]] auto host_iid = buf_recv.extract<server_system::iid_t>();
	auto setting = buf_recv.extract<server_system::sSERVICE::sSETTING>();
	auto request_id = server_system::extract_request_id(buf_recv);

	// 3) find instance
	auto pinstance_service = dynamic_pointer_cast<server_system::instance_service_local>(::service::pmanager_instance->find_instance(iid));

	// check)
	if (pinstance_service.empty())
	{
		LOG_WARNING << "(warn) REQUEST_UPDATE_SETTING: instance not found [iid:"sv << iid << "]"sv;
		this->send_request_result(iid, eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING, request_id, eRESULT::NOT_EXIST, "instance not found"sv);
		return eRESULT::DONE;
	}

	// 4) update setting (iid는 바꾸지 않는다.)
	setting.iid = iid;

	object_ptr<socket_peer> psocket;
	SCOPED_LOCK(pinstance_service->m_lockable_instance)
	{
		pinstance_service->m_instance.info.setting = std::move(setting);
		psocket = pinstance_service->m_psocket;
	}

	// 5) 접속된 service에 전달한다.
	if (psocket.exist())
		psocket->send(server_system::make_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(iid, ::service::pmanager_instance->self()->get_iid(), pinstance_service->m_instance.info.setting));

	// log)
	LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') setting updated by ADMIN_SERVER"sv;

	// 6) result
	this->send_request_result(iid, eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING, request_id, eRESULT::SUCCESS, psocket.exist() ? "applied"sv : "saved (service is not connected)"sv);

	// return)
	return eRESULT::DONE;
}

result_code socket_admin_server::on_message_server_system_REQUEST_SERVICE(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) admin server의 service 제어 요청
	//  - message: iid_t [, context (START)] [, uint64_t request_id]
	//  - iid로 instance를 찾아 request_service_xxx()를 호출한다.
	//-----------------------------------------------------------------

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);
	auto message_id = _msg.buf_message.front<uint32_t>(4);

	// 2) iid / context(START) / request id(optional)
	auto iid = buf_recv.extract<server_system::iid_t>();

	context context_start;
	if (message_id == eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE)
		context_start = buf_recv.extract<context>();

	auto request_id = server_system::extract_request_id(buf_recv);

	// 3) find instance
	auto pinstance_service = dynamic_pointer_cast<server_system::instance_service>(::service::pmanager_instance->find_instance(iid));

	// check)
	if (pinstance_service.empty())
	{
		LOG_WARNING << "(warn) REQUEST(" << message_id << ") from ADMIN_SERVER: instance not found [iid:"sv << iid << "]"sv;
		this->send_request_result(iid, message_id, request_id, eRESULT::NOT_EXIST, "instance not found"sv);
		return eRESULT::DONE;
	}

	// 4) request
	bool result = false;
	switch (message_id)
	{
	case	eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE:
			result = pinstance_service->request_service_initialize();
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE:
			result = pinstance_service->request_service_destroy();
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE:
			result = pinstance_service->request_service_kill();
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE:
			{
				// - context -> json (request_service_start는 json 문자열을 받는다.)
				std::string str_setting;
				if (context_start.is_valid())
					str_setting = context_start.to_json().to_string<char>();

				result = pinstance_service->request_service_start(str_setting);
			}
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE:
			result = pinstance_service->request_service_stop();
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE:
			result = pinstance_service->request_service_pause();
			break;

	case	eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE:
			result = pinstance_service->request_service_continue();
			break;

	default:
			break;
	}

	// log)
	LOG_INFO << "  - INSTANCE('"sv << pinstance_service->get_name() << "') REQUEST("sv << message_id << ") from ADMIN_SERVER "sv << (result ? "accepted"sv : "failed"sv);

	// 5) result
	this->send_request_result(iid, message_id, request_id, result ? eRESULT::SUCCESS : eRESULT::FAIL, result ? "accepted"sv : "failed (see host agent log)"sv);

	// return)
	return eRESULT::DONE;
}
