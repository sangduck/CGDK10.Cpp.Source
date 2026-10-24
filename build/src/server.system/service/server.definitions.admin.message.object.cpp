#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::server_system::make_message_FACTORY_FACTORY_REGISTER(iid_t _iid, Ifactory* _pfactory)
{
	// 1) get snap
	auto data_snap = get_snap(_pfactory);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_REGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_FACTORY_FACTORY_UNREGISTER(iid_t _iid, Ifactory* _pfactory)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + sizeof(int));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(_pfactory->statistics().id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_FACTORY_FACTORY_UPDATE(iid_t _iid, Ifactory* _pfactory)
{
	// 1) get snap
	auto data_snap = get_snap(_pfactory);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::FACTORY::FACTORY_UPDATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_EXECUTOR_REGISTER(iid_t _iid, Iexecutor* _pexecutor)
{
	// 1) get snap
	auto data_snap = get_snap(_pexecutor);

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_REGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_EXECUTOR_UNREGISTER(iid_t _iid, Iexecutor* _pexecutor)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(_pexecutor->statistics().id);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_EXECUTOR_UPDATE(iid_t _iid, Iexecutor* _pexecutor)
{
	// 1) get snap
	auto data_snap = get_snap(_pexecutor);

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::EXECUTOR_UPDATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

