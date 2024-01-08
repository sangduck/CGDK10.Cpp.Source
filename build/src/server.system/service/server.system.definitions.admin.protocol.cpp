#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::server_admin::message_SERVER_ADMIN_CERTIFY_REQUEST(const CGDK::server_admin::sCERTIFY_INFO& _certify_info)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_certify_info);

	// 2) alloc shared buffer
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 3) message head
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_REQUEST);
	temp_buffer.append<CGDK::server_admin::sCERTIFY_INFO>(_certify_info);

	// 4) messag length
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}

shared_buffer CGDK::server_admin::message_SERVER_ADMIN_CERTIFY_RESPONSE(eRESULT _result)
{
	// 1) get size of message
	const std::size_t size_of_message = 8 + get_size_of(_result);

	// 2) alloc shared buffer
	auto temp_buffer = alloc_shared_buffer(size_of_message);

	// 3) message head
	temp_buffer.append<uint32_t>();
	temp_buffer.append<uint32_t>(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE);
	temp_buffer.append<eRESULT>(_result);

	// 4) messag length
	temp_buffer.front<uint32_t>() = temp_buffer.size<uint32_t>();

	// check)
	CGASSERT_ERROR(size_of_message == temp_buffer.size());

	// return) 
	return temp_buffer;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_INIT_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_START_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_STOP_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE);

	// return)
	return buf_temp;
}

shared_buffer CGDK::message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE()
{
	// 1) alloc shared buffer
	auto buf_temp = alloc_shared_buffer(8);

	// 2) write message
	buf_temp.append<uint32_t>(8);
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE);

	// return)
	return buf_temp;
}
