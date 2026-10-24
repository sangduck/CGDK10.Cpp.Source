#include "pch.h"

using namespace CGDK;

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_UPDATE_SETTING(iid_t _iid, iid_t _host_iid, const server_system::sSERVICE::sSETTING& _server_setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_host_iid) + get_size_of(_server_setting);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_UPDATE_SETTING);
	buf_temp.append<iid_t>(_iid);
	buf_temp.append<iid_t>(_host_iid);

	// 3) server setting
	buf_temp.append<server_system::sSERVICE::sSETTING>(_server_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_INIT_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_INIT_SERVICE);
	buf_temp.append<iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return	buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_DESTROY_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_DESTROY_SERVICE);
	buf_temp.append<iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_KILL_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_KILL_SERVICE);
	buf_temp.append<iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_START_SERVICE(server_system::iid_t _iid, const context& _setting)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_setting);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_START_SERVICE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) message body
	buf_temp.append<context>(_setting);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_STOP_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_STOP_SERVICE);
	buf_temp.append<server_system::iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_PAUSE_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_PAUSE_SERVICE);
	buf_temp.append<server_system::iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_CONTINUE_SERVICE(server_system::iid_t _iid)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_CONTINUE_SERVICE);
	buf_temp.append<server_system::iid_t>(_iid);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}


shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_ERROR(server_system::iid_t _iid, uint32_t _request, int _error_code, std::string_view _error_message)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_request) + get_size_of(_error_code) + get_size_of(_error_message);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::ERROR_);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) message body
	buf_temp.append<uint32_t>(_request);
	buf_temp.append<int>(_error_code);
	buf_temp.append<std::string_view>(_error_message);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_SERVER_ADMIN_REQUEST_RESULT(server_system::iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message)
{
	// declare) Size Of Message
	const std::size_t size_of_message = 8 + get_size_of(_iid) + get_size_of(_request) + get_size_of(_request_id) + get_size_of(_result) + get_size_of(_message);

	// 1) shared_buffer를 할당받는다.
	auto buf_temp = alloc_shared_buffer(size_of_message);

	// 2) message header
	buf_temp.append<uint32_t>(static_cast<uint32_t>(size_of_message));
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::ADMIN::REQUEST_RESULT);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) message body
	buf_temp.append<uint32_t>(_request);
	buf_temp.append<uint64_t>(_request_id);
	buf_temp.append<eRESULT>(_result);
	buf_temp.append<std::string_view>(_message);

	// check)
	CGASSERT_ERROR(size_of_message == buf_temp.size());

	// return) 
	return buf_temp;
}

shared_buffer CGDK::server_system::append_request_id(const shared_buffer& _message, uint64_t _request_id)
{
	// check)
	CGASSERT_ERROR(_message.size() >= 8);

	// 1) copy message + request id
	const std::size_t size_of_message = _message.size() + sizeof(uint64_t);
	auto buf_temp = alloc_shared_buffer(size_of_message);
	memcpy(buf_temp.data(), _message.data(), _message.size());
	buf_temp.set_size(_message.size());
	buf_temp.append<uint64_t>(_request_id);

	// 2) message size
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(size_of_message);

	// return)
	return buf_temp;
}

uint64_t CGDK::server_system::extract_request_id(buffer_view& _remained) noexcept
{
	// check) no request id (old admin server)
	if (_remained.size() < sizeof(uint64_t))
		return 0;

	// return)
	return _remained.extract<uint64_t>();
}
