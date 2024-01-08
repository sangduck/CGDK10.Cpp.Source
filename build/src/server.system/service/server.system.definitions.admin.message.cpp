#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::server_admin::message_FACTORY_FACTORY_REGISTER(iid_t _instance_iid, Ifactory* _pfactory)
{
	// 1) get snap
	auto data_snap = get_snap(_pfactory);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_REGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_FACTORY_FACTORY_UNREGISTER(iid_t _instance_iid, Ifactory* _pfactory)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + sizeof(int));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(_pfactory->statistics().id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_FACTORY_FACTORY_UPDATE(iid_t _instance_iid, Ifactory* _pfactory)
{
	// 1) get snap
	auto data_snap = get_snap(_pfactory);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_UPDATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_EXECUTOR_REGISTER(iid_t _instance_iid, Iexecutor* _pexecutor)
{
	// 1) get snap
	auto data_snap = get_snap(_pexecutor);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_REGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_EXECUTOR_UNREGISTER(iid_t _instance_iid, Iexecutor* _pexecutor)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(_pexecutor->statistics().id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_EXECUTOR_UPDATE(iid_t _instance_iid, Iexecutor* _pexecutor)
{
	// 1) get snap
	auto data_snap = get_snap(_pexecutor);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_UPDATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_CONNECTIVE_REGISTER(iid_t _instance_iid, const net::io::Iconnective* _pconnective)
{
	// 1) get snap
	auto data_snap = CGDK::server_admin::get_snap(_pconnective->statistics_get_connective_info_total());

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(iid_t _instance_iid, const net::io::Iconnective* _pconnective)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(_pconnective->get_connective_id());

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_CONNECTIVE_UPDATE(iid_t _instance_iid, const net::io::Iconnective* _pconnective)
{
	// 1) get snap
	const auto& data_snap = _pconnective->statistics_get_connective_info_total();

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_BINDER_REGISTER(iid_t _instance_iid, net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket)
{
	// check) 
	RETURN_IF(_pstatistics_socket == nullptr, shared_buffer());

	// 2) get snap
	const auto& data_snap = server_admin::get_snap(_pbinder, _pstatistics_socket);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 5) append
	buf_temp.append<server_admin::socket_info_snap>(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_BINDER_UNREGISTER(iid_t _instance_iid, net::io::Ibinder* _pbinder)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append<uint64_t>(reinterpret_cast<uint64_t>(_pbinder));

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_NETWORK_SOCKET_BINDER_UPDATE(iid_t _instance_iid, net::io::Ibinder* _pbinder)
{
	// 1) ...
	auto pstatistics = dynamic_cast<net::io::statistics::Nsocket*>(_pbinder);

	// check) 
	RETURN_IF(pstatistics == nullptr, shared_buffer());

	// 2) get snap
	const auto& data_snap = server_admin::get_snap(_pbinder);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_UPDATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 5) append
	buf_temp.append<server_admin::socket_info_snap>(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_TIME_EVENT_EVENT_REGISTER(iid_t _instance_iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 5) append
	buf_temp.append(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}


shared_buffer CGDK::server_admin::message_SERVER_TIME_EVENT_EVENT_UNREGISTER(iid_t _instance_iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append
	buf_temp.append<uint64_t>(data_snap.updatable.id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_TIME_EVENT_EVENT_EXECUTE(iid_t _instance_iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_instance_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 5) append
	buf_temp.append(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_DUMP_INFO_LIST(iid_t _instance_iid)
{
	// 1) get dump info list
	auto vector_status_snap = dump_manager::instance()->status_snap();

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(vector_status_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO_LIST);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 4) append message body
	buf_temp.append(vector_status_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_DUMP_INFO(iid_t _instance_iid)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(100);

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) append message body


	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SEND_SERVER_STATE_UPDATE(eOBJECT_STATE _status_new, eOBJECT_STATE _status_old)
//{
//	// declare) Size Of Message
//	const size_t size_of_message = 8 + sizeof(eOBJECT_STATE) + sizeof(eOBJECT_STATE);
//
//	// 1) shared_buffer를 할당받는다.
//	auto tempbuffer = alloc_shared_buffer(size_of_message);
//
//	// 2) Message Head부분을 작성한다.
//	tempbuffer.append<uint32_t>();
//	tempbuffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_STATUS_UPDATE);
//
//	// 3) STATUS 정보를 써넣는다.
//	tempbuffer.append<eOBJECT_STATE>(_status_new);
//	tempbuffer.append<eOBJECT_STATE>(_status_old;
//
//	// 4) Message의 길이를 써넣는다.
//	tempbuffer.front<uint32_t>() = tempbuffer.size<uint32_t>();
//
//	// check) 
//	CGASSERT_ERROR(size_of_message == tempbuffer.size());
//
//	// 5) 전송한다.
//	return tempbuffer;
//}

shared_buffer CGDK::server_admin::message_SYSTEM_LOG(iid_t _instance_iid, const LOG_RECORD* _plog_record)
{
	// check)
	CGASSERT_ERROR(_plog_record != nullptr);

	// check)
	THROW_IF(_plog_record == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc memory shared_buffer
	auto buf_temp = alloc_shared_buffer(256 + get_size_of(_instance_iid) + get_size_of(_plog_record));

	// 2) append message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
	buf_temp.append<iid_t>(_instance_iid);

	// 3) append count
	buf_temp.append<COUNT_T>(1);
	buf_temp.append(_plog_record);

	// 3) write message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
};

shared_buffer CGDK::server_admin::message_SYSTEM_LOG(iid_t _instance_iid, const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// 1) get
	auto size = 8 + get_size_of(_instance_iid) + sizeof(COUNT_T);
	for (auto& iter: _vector_log_record)
	{
		size += get_size_of(iter);
	}

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(size);

	// 3) append message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
	buf_temp.append<iid_t>(_instance_iid);

	// 4) append count
	buf_temp.append<COUNT_T>(static_cast<COUNT_T>(_vector_log_record.size()));

	// 5) append log records
	for (auto& iter: _vector_log_record)
	{
		buf_temp.append(iter);
	}

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}





















//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_VERSION_UPDATE(server_admin::iid_t _instance_iid)
//{
//	// 1) get size of message
//	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);
//
//	// 2) alloc shared buffer
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) messag header
//	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_VERSION_UPDATE);
//	buf_temp.append<CGDK::server_admin::iid_t>(_instance_iid);
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return	buf_temp;
//}




shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(iid_t _instance_iid, iid_t _host_iid, const server_admin::sSERVICE_SETTING& _server_setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + get_size_of(_host_iid) + get_size_of(_server_setting);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING);
	buf_temp.append<iid_t>(_instance_iid);
	buf_temp.append<iid_t>(_host_iid);

	// 3) server setting
	buf_temp.append<server_admin::sSERVICE_SETTING>(_server_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_INIT_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE);
	buf_temp.append<iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE);
	buf_temp.append<iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_KILL_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE);
	buf_temp.append<iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_START_SERVICE(server_admin::iid_t _instance_iid, const std::string_view _setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + get_size_of(_setting);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) messabe body
	buf_temp.append(_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_STOP_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}


shared_buffer CGDK::server_admin::message_SERVER_ADMIN_ERROR(server_admin::iid_t _instance_iid, uint32_t _request, int _error_code, std::string_view _error_message)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + get_size_of(_request) + get_size_of(_error_code) + get_size_of(_error_message);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::ERROR_);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) messag body
	buf_temp.append<uint32_t>(_request);
	buf_temp.append<int>(_error_code);
	buf_temp.append<std::string_view>(_error_message);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_REQUEST(uint64_t _pid, server_admin::sRANK _rank)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_pid) + get_size_of(_rank);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_REQUEST);

	// 3) messag body
	buf_temp.append<uint64_t>(_pid);
	buf_temp.append<server_admin::sRANK>(_rank);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(const server_admin::sSERVICE_SETTING& _instance_setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + sizeof(eRESULT) + get_size_of(_instance_setting);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE);

	// 3) messag body
	buf_temp.append<eRESULT>(eRESULT::SUCCESS);
	buf_temp.append<server_admin::sSERVICE_SETTING>(_instance_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_CERTIFY_RESPONSE(eRESULT _result)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + sizeof(eRESULT);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_CERTIFY_RESPONSE);

	// 3) result
	buf_temp.append<eRESULT>(_result);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_ENTER(const std::vector<server_admin::sINSTANCE_SERVICE>& _vector_service_instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_vector_service_instance);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) messag header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_ENTER);

	// 3) messag body
	buf_temp.append(_vector_service_instance);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_LEAVE(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_LEAVE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_REGISTER(Ibuffer_serializable* _pinstance)
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

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UNREGISTER(server_admin::iid_t _instance_iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message를 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UNREGISTER);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE(iid_t _instance_iid, Ibuffer_serializable& _instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + +get_size_of(_instance_iid) + _instance.get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) write context
	_instance.serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_SETTING(iid_t _instance_iid, Ibuffer_serializable& _instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + _instance.get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_SETTING);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) write context
	_instance.serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(iid_t _instance_iid, Ibuffer_serializable& _instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + _instance.get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) write context
	_instance.serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_INFO(server_admin::iid_t _service_iid)
{
	// declare)
	CGDK::server_admin::sSERVICE_INSTANCE_STATUS_UPDATE temp_info;

	// 1) generate data for messasge
	temp_info.vector_factory_update = CGDK::server_admin::get_update_snap(factory::manager::instance());
	temp_info.vector_executor_update = CGDK::server_admin::get_update_snap(executor::manager::instance());
	temp_info.vector_schedulable_update = CGDK::server_admin::get_update_snap(system_executor::schedulable_manager());
	temp_info.vector_connective_update = CGDK::server_admin::get_update_snap(net::io::connective::manager::instance());
	temp_info.vector_binder_update = CGDK::server_admin::get_update_snap(net::io::binder::manager::instance());
	temp_info.connective_info = CGDK::server_admin::get_snap(net::io::statistics::Nconnective::statistics_get_connective_info_total());
	temp_info.traffic_info = CGDK::server_admin::get_snap(net::io::statistics::Nsocket::statistics_get_traffic_info_total());

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_service_iid) + get_size_of(temp_info));

	// 3) write messag header
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_INFO);

	// 4) 
	buf_temp.append<server_admin::iid_t>(_service_iid);

	// 5) write message
	buf_temp.append(temp_info);

	// 6) write message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_STATUS(iid_t _instance_iid, Ibuffer_serializable& _instance)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + _instance.get_size_of();

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATUS);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) Server Info
	_instance.serialize_out(buf_temp);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_STATE(server_admin::iid_t _instance_iid, eOBJECT_STATE _status_new, eOBJECT_STATE _status_old)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + get_size_of(_status_new) + get_size_of(_status_old);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_STATE);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) STATUS 정보를 써넣는다.
	buf_temp.append<eOBJECT_STATE>(_status_new);
	buf_temp.append<eOBJECT_STATE>(_status_old);

	// 4) Message의 길이를 써넣는다.
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// 5) 전송한다.
	return buf_temp;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_INSTANCE_UPDATE_PID(server_admin::iid_t _instance_iid, uint64_t _pid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_instance_iid) + get_size_of(_pid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) Message Head부분을 작성한다.
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::INSTANCE_UPDATE_PID);
	buf_temp.append<server_admin::iid_t>(_instance_iid);

	// 3) STATUS 정보를 써넣는다.
	buf_temp.append<uint64_t>(_pid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// 5) 전송한다.
	return buf_temp;
}

























































//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SEND_CONTROL_INFO()
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_HOST_STATUS_UPDATE(const CGDK::server_admin::sHOST_INSTANCE_STATUS& _host_status)
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
//	buf_temp.append<server_admin::sHOST_INSTANCE_STATUS>(_host_status);
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SEND_HOST_INSTANCE(const CGDK::server_admin::sINSTANCE_HOST& _host_info)
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
//	buf_temp.append<server_admin::sINSTANCE_HOST>(_host_info);
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SEND_HOST_SETTING(const CGDK::server_admin::sHOST_SETTING_INFO& _host_setting)
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
//	buf_temp.append<server_admin::sHOST_SETTING_INFO>(_host_setting);
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SEND_ADMINSERVERADDRESS(std::string_view _address)
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_TRAFFIC_INFO()
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_TRAFFIC_INFO_UPDATE()
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

//shared_buffer CGDK::server_admin::message_SERVER_ADMIN_SERVER_PID_UPDATE(server_admin::iid_t _instance_iid, uint64_t _pid)
//{
//	auto buf_temp = alloc_shared_buffer(8 + sizeof(server_admin::iid_t) + sizeof(uint64_t));
//
//	buf_temp.append<uint32_t>();
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::SERVER_PID_UPDATE);
//
//	buf_temp.append<server_admin::iid_t>(_instance_iid);
//	buf_temp.append<uint64_t>(_pid);
//
//	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();
//
//	// return) 
//	return buf_temp;
//}

shared_buffer CGDK::message_SERVER_CERTIFY_REQUEST(const CGDK::server_admin::sSERVICE_SETTING& _server_setting_info)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_server_setting_info);

	// 2) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::CERTIFY_REQUEST);
	buf_temp.append<server_admin::sSERVICE_SETTING>(_server_setting_info);

	// 4) messag length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_CERTIFY_RESPONSE(eRESULT _result, const CGDK::server_admin::sSERVICE_SETTING& _server_setting_info)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_result) + get_size_of(_server_setting_info);

	// 2) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::CERTIFY_RESPONSE);

	// 4) result & host_instance_info
	buf_temp.append<eRESULT>(_result);
	buf_temp.append<server_admin::sSERVICE_SETTING>(_server_setting_info);

	// 5) messag length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_CERTIFY_RESPONSE(eRESULT _result)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_result);

	// 2) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 3) message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE);
	buf_temp.append<eRESULT>(_result);

	// 4) messag length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}
