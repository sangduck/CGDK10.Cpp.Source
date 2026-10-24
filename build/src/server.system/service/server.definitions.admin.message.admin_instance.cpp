#include "pch.h"

using namespace CGDK;

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(const server_system::host::manager* _phost_instance_manager, const server_system::service::manager* _pservice_instance_manager)
{
	// - get size of message
	auto size_of_message = 8 + get_size_of(*_phost_instance_manager) + get_size_of(*_pservice_instance_manager);

	// - alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// - message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST);

	// - message body
	buf_temp.append(*_phost_instance_manager);
	buf_temp.append(*_pservice_instance_manager);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(const server_system::sSERVICE& _service_remote, const std::map<server_system::iid_t, object_ptr<server_system::host::Ninstance>>& _map_host_info_confirmed, const std::map<server_system::iid_t, object_ptr<server_system::service::Ninstance>>& _map_service_info_confirmed)
{
	// declare) Size Of Message
	std::size_t size_of_message = 8 + sizeof(eRESULT) + get_size_of(_service_remote);
	size_of_message += sizeof(COUNT_T);
	for (auto& iter : _map_host_info_confirmed)
	{
		size_of_message += sizeof(server_system::iid_t);
		size_of_message += get_size_of(iter.second->info);
	}
	size_of_message += sizeof(COUNT_T);
	for (auto& iter : _map_service_info_confirmed)
	{
		size_of_message += sizeof(server_system::iid_t);
		size_of_message += get_size_of(iter.second->info);
	}

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE);

	// 3) message body
	buf_temp.append<eRESULT>(eRESULT::SUCCESS);
	buf_temp.append<server_system::sSERVICE>(_service_remote);
	buf_temp.append<COUNT_T>(static_cast<COUNT_T>(_map_host_info_confirmed.size()));
	for (auto& iter : _map_host_info_confirmed)
	{
		buf_temp.append<server_system::iid_t>(iter.first);
		buf_temp.append<server_system::sHOST>(iter.second->info);
	}
	buf_temp.append<COUNT_T>(static_cast<COUNT_T>(_map_service_info_confirmed.size()));
	for (auto& iter : _map_service_info_confirmed)
	{
		buf_temp.append<server_system::iid_t>(iter.first);
		buf_temp.append<server_system::sSERVICE>(iter.second->info);
	}

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT _result)
{
	// check)
	CGASSERT_ERROR(_result != eRESULT::SUCCESS);

	// declare) Size Of Message
	const std::size_t size_of_message = 8 + sizeof(eRESULT);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE);

	// 3) result
	buf_temp.append<eRESULT>(_result);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_ENTER(const server_system::sSERVICE& _service_info)
//{
//	// declare) 
//	std::vector<server_system::sSERVICE> vector_instance;
//
//	// 1) make service info
//	vector_instance.push_back(_service_info);
//
//	// 2) make message and return
//	return server_system::make_message_SERVER_ADMIN_INSTANCE_ENTER(vector_instance);
//}
//
//server_system::sSERVICE CGDK::update_service_info(server_system::sSERVICE&& _service_info)
//{
//	// declare) 
//	server_system::sSERVICE temp_servuce = _service_info;
//
//	// - set value
//	temp_servuce.status.iid = temp_servuce.info.iid;
//	temp_servuce.status.state = eOBJECT_STATE::NONE;
//	temp_servuce.status.pid = temp_servuce.status.pid;
//
//	temp_servuce.status.vector_factory = server_system::get_snap(factory::manager::instance());
//	temp_servuce.status.vector_executor = server_system::get_snap(executor::manager::instance());
//	temp_servuce.status.vector_schedulable = server_system::get_snap(system_executor::schedulable_manager());
//	temp_servuce.status.vector_connective = server_system::get_snap(net::io::connective::manager::instance());
//	temp_servuce.status.vector_binder = server_system::get_snap(net::io::binder::manager::instance());
//	temp_servuce.status.vector_time_event = server_system::get_snap(time_event::entity_manager::instance());
//	temp_servuce.status.connective_info = server_system::get_snap(net::io::statistics::Nconnective::statistics_get_connective_info_total());
//	temp_servuce.status.traffic_info = server_system::get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());
//	temp_servuce.status.vector_log_record = server_system::get_records_of_default_logger_circular_list();
//	temp_servuce.status.time_update = chrono::time::utc::now();
//
//	// 2) make message and return
//	return return temp_servuce;
//}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_ENTER(const std::vector<server_system::sSERVICE>& _vector_service_info)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_vector_service_info);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_ENTER);

	// 3) message body
	buf_temp.append(_vector_service_info);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_LEAVE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_LEAVE);
	buf_temp.append<server_system::iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_REGISTER(Ibuffer_serializable* _pinstance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + _pinstance->get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_REGISTER);

	// 3) server setting
	_pinstance->serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UNREGISTER(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message를 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE(iid_t _iid, Ibuffer_serializable& _instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + +get_size_of(_iid) + _instance.get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) write context
	_instance.serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_SETTING(iid_t _iid, const server_system::sSERVICE::sSETTING& _setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_setting);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_SETTING);
	buf_temp.append<server_system::iid_t>(_iid);
	buf_temp.append<server_system::sSERVICE::sSETTING>(_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(iid_t _iid, const server_system::sSERVICE& _service_info)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_service_info);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO);
	buf_temp.append<server_system::iid_t>(_iid);
	buf_temp.append<server_system::sSERVICE>(_service_info);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(iid_t _iid, const server_system::sSERVICE::sSTATUS& _service_status)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_service_status);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATUS);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) write context
	buf_temp.append<server_system::sSERVICE::sSTATUS>(_service_status);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_HOST_UPDATE_STATUS(server_system::iid_t _host_iid, const SYSTEM::sSTATUS& _system_status, const sHOST_WEB_STATUS* _pweb_status)
{
	// - (host iid, processor_usage, thread_count, process_count, handle_count, bytes_memory_usage, bytes_traffic)
	//   + (web status : [uint32 field count][uint64 x field count]) (extract_host_web_status)
	constexpr uint32_t WEB_STATUS_FIELDS = 6;
	const std::size_t size_of_message = 8 + sizeof(server_system::iid_t) + sizeof(uint64_t) * 6 + ((_pweb_status != nullptr) ? sizeof(uint32_t) + sizeof(uint64_t) * WEB_STATUS_FIELDS : 0);

	auto buf_temp = alloc_shared_buffer(size_of_message);
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::HOST_UPDATE_STATUS);
	buf_temp.append<server_system::iid_t>(_host_iid);
	buf_temp.append<uint64_t>(_system_status.processor_usage);
	buf_temp.append<uint64_t>(_system_status.thread_count);
	buf_temp.append<uint64_t>(_system_status.process_count);
	buf_temp.append<uint64_t>(_system_status.handle_count);
	buf_temp.append<uint64_t>(_system_status.bytes_memory_usage);
	buf_temp.append<uint64_t>(_system_status.bytes_traffic);

	// - web status
	if (_pweb_status != nullptr)
	{
		buf_temp.append<uint32_t>(WEB_STATUS_FIELDS);
		buf_temp.append<uint64_t>(_pweb_status->web_connections);
		buf_temp.append<uint64_t>(_pweb_status->web_timeout_request);
		buf_temp.append<uint64_t>(_pweb_status->web_timeout_idle);
		buf_temp.append<uint64_t>(_pweb_status->websocket_connections);
		buf_temp.append<uint64_t>(_pweb_status->websocket_timeout_request);
		buf_temp.append<uint64_t>(_pweb_status->websocket_timeout_idle);
	}

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

std::optional<CGDK::server_system::sHOST_WEB_STATUS> CGDK::server_system::extract_host_web_status(buffer_view& _buffer)
{
	// check) 없다. (이전 host_agent)
	if (_buffer.size() < sizeof(uint32_t))
		return std::nullopt;

	// - field 수 (모르는 field는 건너뛴다. 모자라면 없는 것으로 본다.)
	const auto count = _buffer.extract<uint32_t>();
	if (static_cast<uint64_t>(count) * sizeof(uint64_t) > _buffer.size())
		return std::nullopt;

	uint64_t values[6] = {};
	for (uint32_t i = 0; i < count; ++i)
	{
		const auto value = _buffer.extract<uint64_t>();
		if (i < 6)
			values[i] = value;
	}

	sHOST_WEB_STATUS status;
	status.web_connections = values[0];
	status.web_timeout_request = values[1];
	status.web_timeout_idle = values[2];
	status.websocket_connections = values[3];
	status.websocket_timeout_request = values[4];
	status.websocket_timeout_idle = values[5];
	return status;
}

CGDK::SYSTEM::sSTATUS CGDK::server_system::extract_system_status(buffer_view& _buffer)
{
	SYSTEM::sSTATUS status;
	status.processor_usage = _buffer.extract<uint64_t>();
	status.thread_count = _buffer.extract<uint64_t>();
	status.process_count = _buffer.extract<uint64_t>();
	status.handle_count = _buffer.extract<uint64_t>();
	status.bytes_memory_usage = _buffer.extract<uint64_t>();
	status.bytes_traffic = _buffer.extract<uint64_t>();
	status.time_update = chrono::time::utc::now();
	return status;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(server_system::iid_t _service_iid)
{
	// declare)
	CGDK::server_system::sSERVICE_STATUS_UPDATE temp_info;

	// 1) generate data for message
	temp_info.vector_factory_update = CGDK::server_system::get_update_snap(factory::manager::instance());
	temp_info.vector_executor_update = CGDK::server_system::get_update_snap(executor::manager::instance());
	temp_info.vector_schedulable_update = CGDK::server_system::get_update_snap(system_executor::schedulable_manager());
	temp_info.vector_connective_update = CGDK::server_system::get_update_snap(net::io::connective::manager::instance());
	temp_info.vector_binder_update = CGDK::server_system::get_update_snap(net::io::binder::manager::instance());
	temp_info.connective_info = CGDK::server_system::get_snap(net::io::statistics::Nconnective::statistics_get_connective_info_total());
	temp_info.traffic_info = CGDK::server_system::get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_service_iid) + get_size_of(temp_info));

	// 3) write message header
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATUS);

	// 4) 
	buf_temp.append<server_system::iid_t>(_service_iid);

	// 5) write message
	buf_temp.append(temp_info);

	// 6) write message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(server_system::iid_t _iid, eOBJECT_STATE _status_new, eOBJECT_STATE _status_old, const server_system::sSERVICE* _service_info)
{
	// declare)
	bool server_info_exist = _service_info != nullptr;

	// declare) size of message
	std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_status_new) + get_size_of(_status_old) + get_size_of(server_info_exist);

	if (_service_info != nullptr)
		size_of_message += get_size_of(*_service_info);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) STATUS 정보를 써넣는다.
	buf_temp.append<eOBJECT_STATE>(_status_new);
	buf_temp.append<eOBJECT_STATE>(_status_old);

	if (server_info_exist)
	{
		buf_temp.append<bool>(true);
		buf_temp.append<server_system::sSERVICE>(*_service_info);
	}
	else
	{
		buf_temp.append<bool>(false);
	}

	// 4) Message의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// 5) 전송한다.
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_INSTANCE_UPDATE_PID(server_system::iid_t _iid, uint64_t _pid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_pid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_PID);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) STATUS 정보를 써넣는다.
	buf_temp.append<uint64_t>(_pid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// 5) 전송한다.
	return buf_temp;
}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SEND_CONTROL_INFO()
//{
//	// declare) Size Of Message
//	const std::size_t size_of_message = 8;
//
//	// 1) shared_buffer를 할당받는다.
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) Message Head부분을 작성한다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SEND_CONTROL_INFO);
//
//	// 3) Message 길이를 설정한다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_HOST_STATUS_UPDATE(const CGDK::server_system::sHOST_INSTANCE_STATUS& _host_status)
//{
//	// declare) Size Of Message
//	const std::size_t size_of_message = 8 + get_size_of(_host_status);
//
//	// 1) shared_buffer를 할당받는다.
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) Message Head부분을 작성한다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::HOST_STATUS_UPDATE);
//
//	// 3) Host Status를 써넣는다.
//	buf_temp.append<server_system::sHOST_INSTANCE_STATUS>(_host_status);
//
//	// 4) Message 길이를 설정한다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SEND_HOST_INSTANCE(const CGDK::server_system::sINSTANCE_HOST& _host_info)
//{
//	// declare) Size Of Message
//	const std::size_t size_of_message = 8 + get_size_of(_host_info);
//
//	// 1) shared_buffer를 할당받는다.
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) Message Head부분을 작성한다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SEND_HOST_INSTANCE_INFO);
//
//	// 3) Host Info를 써넣는다.
//	buf_temp.append<server_system::sINSTANCE_HOST>(_host_info);
//
//	// 4) Message의 길이를 써넣는다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SEND_HOST_SETTING(const CGDK::server_system::sHOST& _host_setting)
//{
//	// declare) Size Of Message
//	const std::size_t size_of_message = 8 + get_size_of(_host_setting);
//
//	// 1) shared_buffer를 할당받는다.
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) Message Head부분을 작성한다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SEND_HOST_SETTING);
//
//	// 3) Host setting을 써넣는다.
//	buf_temp.append<server_system::sHOST>(_host_setting);
//
//	// 4) Message의 길이를 써넣는다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS(std::string_view _address)
//{
//	// 1) shared_buffer를 할당받는다.
//	auto buf_temp = alloc_shared_buffer(8 + 512);
//
//	// 2) Message Head부분을 작성한다.
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SEND_ADMINSERVER_ADDRESS);
//
//	// 3) Address를 써넣는다.
//	buf_temp.append(_address);
//
//	// 4) Message의 길이를 써넣는다.
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_TRAFFIC_INFO()
//{
//	// 1) get snap data
//	auto data_snap = get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());
//
//	// 2) alloc buffer
//	auto buf_temp = alloc_shared_buffer(8 + sizeof(int) + get_size_of(data_snap));
//
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::TRAFFIC_INFO);
//	buf_temp.append<int>(g_statistics_time);
//	buf_temp.append<statistics::unit_traffic_snap>(data_snap);
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE()
//{
//	// 1) get snap data
//	auto data_snap = get_update_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());
//
//	// 2) alloc buffer
//	auto buf_temp = alloc_shared_buffer(8 + sizeof(int) + get_size_of(data_snap));
//
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::TRAFFIC_INFO_UPDATE);
//	buf_temp.append<int>(g_statistics_time);
//	buf_temp.append<statistics::unit_traffic_snap>(data_snap);
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SERVER_PID_UPDATE(server_system::iid_t _iid, uint64_t _pid)
//{
//	auto buf_temp = alloc_shared_buffer(8 + sizeof(server_system::iid_t) + sizeof(uint64_t));
//
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_PID_UPDATE);
//
//	buf_temp.append<server_system::iid_t>(_iid);
//	buf_temp.append<uint64_t>(_pid);
//
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::make_message_SERVER_ADMIN_CERTIFY_REQUEST(const server_system::sSERVICE& _service_info)
//{
//	// 1) get size of message
//	const std::size_t size_of_message = 8 + get_size_of(_service_info);
//
//	// 2) alloc shared buffer
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 3) message head
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST);
//
//	// 4) message body
//	buf_temp.append<server_system::sSERVICE>(_service_info);
//
//	// 5) message length
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}

//shared_buffer CGDK::make_message_SERVER_CERTIFY_RESPONSE(const CGDK::server_system::sSERVICE& _service_info)
//{
//	// 1) get size of message
//	const std::size_t size_of_message = 8 + sizeof(eRESULT) + get_size_of(_service_info);
//
//	// 2) alloc shared buffer
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 3) message head
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE);
//
//	// 4) result & message body
//	buf_temp.append<eRESULT>(eRESULT::SUCCESS);
//	buf_temp.append<server_system::sSERVICE>(_service_info);
//
//	// 5) message length
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}
//
//shared_buffer CGDK::make_message_`SERVER_CERTIFY_RESPONSE(eRESULT _result)
//{
//	// check)
//	CGASSERT_ERROR(_result != eRESULT::SUCCESS);
//
//	// 1) get size of message
//	const std::size_t size_of_message = 8 + get_size_of(_result);
//
//	// 2) alloc shared buffer
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 3) message head
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE);
//	buf_temp.append<eRESULT>(_result);
//
//	// 4) message length
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return buf_temp;
//}
