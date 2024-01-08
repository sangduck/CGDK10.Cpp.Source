#include "pch.h"

namespace CGDK
{


shared_buffer message_SERVER_DOSS_REQUEST_CERTIFY(const sREPOSITORY_ID& _rid, const sREPOSITORY_ID& _rid_remote, uint64_t _receive_sequence_ack)
{
	// 1) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(8+sizeof(sREPOSITORY_ID)+sizeof(sREPOSITORY_ID) + sizeof(uint64_t));

	// 2) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::REQUEST_CERTIFY);
	buf_temp.append<sREPOSITORY_ID>(_rid);
	buf_temp.append<sREPOSITORY_ID>(_rid_remote);
	buf_temp.append<uint64_t>(_receive_sequence_ack);

	// 3) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return)
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_RESPONSE_CERTIFY(const sREPOSITORY_ID& _rid, const std::vector<sREPOSITORY_ID>& _vec_rid, uint64_t _receive_sequence_ack)
{
	// 1) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(8 + sizeof(sREPOSITORY_ID) + get_size_of(_vec_rid) + sizeof(uint64_t));

	// 2) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::RESPONSE_CERTIFY);
	buf_temp.append<sREPOSITORY_ID>(_rid);
	buf_temp.append<std::vector<sREPOSITORY_ID>>(_vec_rid);
	buf_temp.append<uint64_t>(_receive_sequence_ack);

	// 3) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_REQUEST_RECOVER(uint64_t /*_tid*/, const sCHECKIN_ID& /*_cid*/)
{
	// 1) get size of message
	//auto buffer_size = 8+sizeof(eTRANSACTION_TYPE)+sizeof(uint64_t)+ _buf_request.size();
	auto buffer_size = 8;

	// 1) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 2) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::COMMAND_REQUEST_CHECKIN);

	// 3) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return buf_temp;
}

shared_buffer message_SERVER_DOSS_COMMAND_REQUEST(uint64_t _send_sequence, doss::Itransactable* _ptransactable)
{
	// check) 
	CGASSERT_ERROR(_ptransactable->m_cid.id_entity.exist());
	CGASSERT_ERROR(_ptransactable->m_cid.id_session.exist());
	CGASSERT_ERROR(_ptransactable->m_cid.id_entity == _ptransactable->m_pentity->m_eid);

	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(uint64_t) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + get_size_of(_ptransactable->m_buf_request);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);
	
	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::COMMAND_REQUEST);
	buf_temp.append<uint64_t>(_send_sequence);
	buf_temp.append<eTRANSACTION_TYPE>(_ptransactable->m_type);
	buf_temp.append<uint64_t>(_ptransactable->m_option);
	buf_temp.append<sCHECKIN_ID>(_ptransactable->m_cid);
	buf_temp.append<sREQUEST_ID>(_ptransactable->m_rid);
	buf_temp.append(_ptransactable->m_buf_request);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_COMMAND_RESPONSE(uint64_t _send_sequence, doss::Itransactable* _ptransactable)
{
	// check) 
	CGASSERT_ERROR(_ptransactable->m_cid.entity_id().exist());
	CGASSERT_ERROR(_ptransactable->m_cid.session_id().exist());

	// check)
	CGASSERT_ERROR(_ptransactable->m_type != eTRANSACTION_TYPE::CHECKOUT);

	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + sizeof(eRESULT) + get_size_of(_ptransactable->m_buf_request);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::COMMAND_RESPONSE);
	buf_temp.append<uint64_t>(_send_sequence);
	buf_temp.append<eTRANSACTION_TYPE>(_ptransactable->m_type);
	buf_temp.append<sCHECKIN_ID>(_ptransactable->m_cid);
	buf_temp.append<sREQUEST_ID>(_ptransactable->m_rid_source);
	buf_temp.append<eRESULT>(_ptransactable->m_result);
	buf_temp.append(_ptransactable->m_buf_result);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_COMMAND_RESPONSE(uint64_t _send_sequence, eTRANSACTION_TYPE _command, sCHECKIN_ID _cid, sREQUEST_ID _rid_source, result_code _result, shared_buffer _buf_response)
{
	// check)
	CGASSERT_ERROR(_cid.entity_id().exist());
	CGASSERT_ERROR(_cid.session_id().exist());

	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + sizeof(eRESULT) + get_size_of(_buf_response);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::COMMAND_RESPONSE);
	buf_temp.append<uint64_t>(_send_sequence);
	buf_temp.append<eTRANSACTION_TYPE>(_command);
	buf_temp.append<sCHECKIN_ID>(_cid);
	buf_temp.append<sREQUEST_ID>(_rid_source);
	buf_temp.append<eRESULT>(_result);
	buf_temp.append(_buf_response);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_COMMAND_FAIL(uint64_t _send_sequence, doss::Itransactable* _ptransactable)
{
	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + sizeof(eRESULT);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::COMMAND_FAIL);
	buf_temp.append<uint64_t>(_send_sequence);
	buf_temp.append<eTRANSACTION_TYPE>(_ptransactable->m_type);
	buf_temp.append<sCHECKIN_ID>(_ptransactable->m_cid);
	buf_temp.append<sREQUEST_ID>(_ptransactable->m_rid);
	buf_temp.append<eRESULT>(_ptransactable->m_result);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	// return) 
	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_CONTROL_REQUEST(uint64_t _send_sequence, eTRANSACTION_TYPE _command, const sCHECKIN_ID& _cid, sREQUEST_ID _rid, eRESULT _result)
{
	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + sizeof(eRESULT);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::CONTROL_REQUEST);
	buf_temp.append<uint64_t>(_send_sequence);
	buf_temp.append<eTRANSACTION_TYPE>(_command);
	buf_temp.append<sCHECKIN_ID>(_cid);
	buf_temp.append<sREQUEST_ID>(_rid);
	buf_temp.append<eRESULT>(_result);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	return	buf_temp;
}

shared_buffer message_SERVER_DOSS_SEND_ACK(uint64_t _send_sequence)
{
	// 1) get size of message
	auto buffer_size = 8 + sizeof(uint64_t) + sizeof(eTRANSACTION_TYPE) + sizeof(sCHECKIN_ID) + sizeof(sREQUEST_ID) + sizeof(eRESULT);

	// 2) alloc memory buffer
	auto buf_temp = alloc_shared_buffer(buffer_size);

	// 3) write message
	buf_temp.append<uint32_t>();
	buf_temp.append<uint32_t>(eMESSAGE::SERVER::DOSS::SEND_ACK);
	buf_temp.append<uint64_t>(_send_sequence);

	// 4) set message length
	buf_temp.front<uint32_t>() = buf_temp.size<uint32_t>();

	return buf_temp;
}


}