#include "pch.h"

using namespace CGDK;


CGDK::shared_buffer _message_UPDATE_BEGIN_REQUEST(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, const CGDK::update::sUPDATE_REQUEST_INFO& _update_request_info)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id) + CGDK::get_size_of(_update_request_info);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) write message head
	buf_temp.append<uint32_t>(static_cast<uint32_t>(MESSAGE_SIZE));
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::BEGIN_REQUEST);

	// 3) 
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::sUPDATE_REQUEST_INFO>(_update_request_info);

	// 4) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_END(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, eRESULT _result)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id) + CGDK::get_size_of(_result);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) write message head
	buf_temp.append<uint32_t>(static_cast<uint32_t>(MESSAGE_SIZE));
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::END);

	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<eRESULT>(_result);

	// 4) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_REQUEST(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, const CGDK::resource::sUPDATABLE_FILE_META_INFO& _file_meta_info)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id) + CGDK::get_size_of<uint32_t>(_file_meta_info);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 3) write message head
	buf_temp.append<uint32_t>(static_cast<uint32_t>(MESSAGE_SIZE));
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_REQUEST);

	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::resource::sUPDATABLE_FILE_META_INFO>(_file_meta_info);

	// 4) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_BEGIN(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, CGDK::update::UPDATABLE_ID _updatable_id, uint64_t _pos_begin, CGDK::resource::eFILE_FLAG _file_flag)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id) + CGDK::get_size_of(_updatable_id) + CGDK::get_size_of(_pos_begin) + CGDK::get_size_of(_file_flag);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 3) write message head
	buf_temp.append<uint32_t>(static_cast<uint32_t>(MESSAGE_SIZE));
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_BEGIN);

	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::UPDATABLE_ID>(_updatable_id);
	buf_temp.append<uint64_t>(_pos_begin);
	buf_temp.append<CGDK::resource::eFILE_FLAG>(_file_flag);

	// 4) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_BLOCK(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, uint32_t _size_block)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) write message head
	buf_temp.append<uint32_t>(MESSAGE_SIZE + _size_block);
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_BLOCK);

	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);

	// 4) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_END(CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::SENDER_ID _sender_id, CGDK::update::UPDATABLE_ID _updatable_id, eRESULT _result)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_sender_id) + CGDK::get_size_of(_updatable_id) + CGDK::get_size_of(_result);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) write message head
	buf_temp.append<uint32_t>(MESSAGE_SIZE);
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_END);

	// 3) write message body
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::UPDATABLE_ID>(_updatable_id);
	buf_temp.append<eRESULT>(_result);

	// 3) write message size
	CGASSERT_ERROR(buf_temp.size() == MESSAGE_SIZE);

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_END(CGDK::update::SENDER_ID _sender_id, CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::UPDATABLE_ID _updatable_id, eRESULT _result, CGDK::MD5 _md5)
{
	// declare)
	const auto MESSAGE_SIZE = 8 + CGDK::get_size_of(_sender_id) + CGDK::get_size_of(_receiver_id) + CGDK::get_size_of(_updatable_id) + CGDK::get_size_of(_result) + get_size_of(_md5);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) write message head
	buf_temp.append<uint32_t>(MESSAGE_SIZE);
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_END);

	// 3) write message body
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::UPDATABLE_ID>(_updatable_id);
	buf_temp.append<eRESULT>(_result);
	buf_temp.append<CGDK::MD5>(_md5);

	// 4) write message size
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_BEGIN_RESPONSE(CGDK::update::SENDER_ID _sender_id, CGDK::update::RECEIVER_ID _receiver_id, eRESULT _result)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(64);

	// 2) write message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::BEGIN_RESPONSE);

	// 3) 
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<eRESULT>(_result);

	// 4) write message size
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_RESPONSE(CGDK::update::SENDER_ID _sender_id, CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::UPDATABLE_ID _updatable_id, const CGDK::sUPDATABLE_PREPARE_INFO& _updatable_prepare_info)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(64 + CGDK::get_size_of(_updatable_prepare_info));

	// 2) write message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_RESPONSE);

	// 3) 
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::UPDATABLE_ID>(_updatable_id);
	buf_temp.append<CGDK::sUPDATABLE_PREPARE_INFO>(_updatable_prepare_info);

	// 4) write message size
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}

CGDK::shared_buffer _message_UPDATE_UPDATABLE_BEGIN(CGDK::update::SENDER_ID _sender_id, CGDK::update::RECEIVER_ID _receiver_id, CGDK::update::UPDATABLE_ID _updatable_id, eRESULT _result, uint64_t _pos_begin)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(64);

	// 2) write message head
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(CGDK::eMESSAGE::UPDATE::UPDATABLE_BEGIN);

	// 3) ...
	buf_temp.append<CGDK::update::SENDER_ID>(_sender_id);
	buf_temp.append<CGDK::update::RECEIVER_ID>(_receiver_id);
	buf_temp.append<CGDK::update::UPDATABLE_ID>(_updatable_id);
	buf_temp.append<eRESULT>(_result);

	buf_temp.append<uint64_t>(_pos_begin);

	// 4) write message size
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}
