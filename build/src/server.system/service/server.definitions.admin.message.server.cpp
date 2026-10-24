#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::server_system::make_message_SERVER_TIME_EVENT_EVENT_REGISTER(iid_t _iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_REGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 5) append
	buf_temp.append(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}


shared_buffer CGDK::server_system::make_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(iid_t _iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append<uint64_t>(data_snap.updatable.id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_TIME_EVENT_EVENT_EXECUTE(iid_t _iid, time_event::Ientity* _ptime_event)
{
	// 1) get time_entity info
	const auto data_snap = get_snap(_ptime_event);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::TIME_EVENT::EVENT_EXECUTE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 5) append
	buf_temp.append(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_DUMP_INFO_LIST(iid_t _iid)
{
	// 1) get dump info list
	auto vector_status_snap = dump_manager::instance()->status_snap();

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(vector_status_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO_LIST);
	buf_temp.append<server_system::iid_t>(_iid);

	// 4) append message body
	buf_temp.append(vector_status_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_DUMP_INFO(iid_t _iid)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(100);

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::DUMP_INFO);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append message body


	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_SEND_SERVER_STATE_UPDATE(eOBJECT_STATE _status_new, eOBJECT_STATE _status_old)
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

shared_buffer CGDK::server_system::make_message_SYSTEM_LOG(iid_t _iid, const LOG_RECORD* _plog_record)
{
	// check)
	CGASSERT_ERROR(_plog_record != nullptr);

	// check)
	THROW_IF(_plog_record == nullptr, throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) alloc memory shared_buffer
	auto buf_temp = alloc_shared_buffer(256 + get_size_of(_iid) + get_size_of(_plog_record));

	// 2) append message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
	buf_temp.append<iid_t>(_iid);

	// 3) append count
	buf_temp.append<COUNT_T>(1);
	buf_temp.append(_plog_record);

	// 3) write message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return buf_temp;
};

shared_buffer CGDK::server_system::make_message_SYSTEM_LOG(iid_t _iid, const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// 1) get
	auto size = 8 + get_size_of(_iid) + sizeof(COUNT_T);
	for (auto& iter: _vector_log_record)
	{
		size += get_size_of(iter);
	}

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(size);

	// 3) append message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
	buf_temp.append<iid_t>(_iid);

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

//shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_VERSION_UPDATE(server_system::iid_t _iid)
//{
//	// 1) get size of message
//	const std::size_t size_of_message = 8 + get_size_of(_iid);
//
//	// 2) alloc shared buffer
//	auto buf_temp = alloc_shared_buffer(size_of_message);
//
//	// 2) message header
//	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
//	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_VERSION_UPDATE);
//	buf_temp.append<CGDK::server_system::iid_t>(_iid);
//
//	// check)
//	CGASSERT_ERROR(size_of_message == buf_temp.size());
//
//	// return) 
//	return	buf_temp;
//}

