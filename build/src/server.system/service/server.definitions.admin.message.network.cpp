#include "pch.h"

using namespace CGDK;


shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_CONNECTIVE_REGISTER(iid_t _iid, const net::io::Iconnective* _pconnective)
{
	// 1) get snap
	auto data_snap = CGDK::server_system::get_snap(_pconnective->statistics_get_connective_info_total());

	// 2) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 3) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_REGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 4) append
	buf_temp.append(data_snap);

	// 5) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_CONNECTIVE_UNREGISTER(iid_t _iid, const net::io::Iconnective* _pconnective)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(_pconnective->get_connective_id());

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_CONNECTIVE_UPDATE(iid_t _iid, const net::io::Iconnective* _pconnective)
{
	// 1) get snap
	const auto& data_snap = _pconnective->statistics_get_connective_info_total();

	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::CONNECTIVE_UPDATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append(data_snap);

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_BINDER_REGISTER(iid_t _iid, net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket)
{
	// check) 
	RETURN_IF(_pstatistics_socket == nullptr, shared_buffer());

	// 2) get snap
	const auto& data_snap = server_system::get_snap(_pbinder, _pstatistics_socket);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 5) append
	buf_temp.append<server_system::socket_info_snap>(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_BINDER_UNREGISTER(iid_t _iid, net::io::Ibinder* _pbinder)
{
	// 1) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + sizeof(uint64_t));

	// 2) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER);
	buf_temp.append<server_system::iid_t>(_iid);

	// 3) append
	buf_temp.append<uint64_t>(reinterpret_cast<uint64_t>(_pbinder));

	// 4) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}

shared_buffer CGDK::server_system::make_message_NETWORK_SOCKET_BINDER_UPDATE(iid_t _iid, net::io::Ibinder* _pbinder)
{
	// 1) ...
	auto pstatistics = dynamic_cast<net::io::statistics::Nsocket*>(_pbinder);

	// check) 
	RETURN_IF(pstatistics == nullptr, shared_buffer());

	// 2) get snap
	const auto& data_snap = server_system::get_snap(_pbinder);

	// 3) alloc buffer
	auto buf_temp = alloc_shared_buffer(8 + get_size_of(_iid) + get_size_of(data_snap));

	// 4) append header info
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::NETWORK::SOCKET::BINDER_UPDATE);
	buf_temp.append<server_system::iid_t>(_iid);

	// 5) append
	buf_temp.append<server_system::socket_info_snap>(data_snap);

	// 6) set message length
	buf_temp.front<uint32_t>() = static_cast<uint32_t>(buf_temp.size());

	// return)
	return buf_temp;
}
