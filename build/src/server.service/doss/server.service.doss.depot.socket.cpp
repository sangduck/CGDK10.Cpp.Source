#include "pch.h"

namespace CGDK
{
	
doss::Idepot::socket::socket() noexcept
{
	// 1) Option을 설정 (Server용 Socket이므로...
	this->maximum_send_buffer_size(32768);
}

doss::Idepot::socket::~socket() noexcept
{
}

void doss::Idepot::socket::reset_depot_info() noexcept
{
}

bool doss::Idepot::socket::send_restoreable(Itransactable* _ptransactable)
{
	// locked) 
	{
		// SCOPED_LOCK) 
		std::lock_guard cs(m_lockable_send_sequence);

		// 1) generate send_sequence
		uint64_t send_sequence = ++m_send_sequence;

		// 2) send
		return send(message_SERVER_DOSS_COMMAND_RESPONSE(send_sequence, _ptransactable));
	}
}

bool doss::Idepot::socket::send_restoreable(eTRANSACTION_TYPE _command, const sCHECKIN_ID& _cid, sREQUEST_ID _rid, eRESULT _result, const shared_buffer& _buf_response)
{
	// locked)
	{
		// SCOPED_LOCK) 
		std::lock_guard cs(m_lockable_send_sequence);

		// 1) generate send_sequence
		uint64_t send_sequence = ++m_send_sequence;

		// 2) send
		return send(message_SERVER_DOSS_COMMAND_RESPONSE(send_sequence, _command, _cid, _rid, _result, _buf_response));
	}
}

bool doss::Idepot::socket::send_restoreable_control_request(eTRANSACTION_TYPE _command, const sCHECKIN_ID& _cid, sREQUEST_ID _rid, eRESULT _result)
{
	// locked)
	{
		// SCOPED_LOCK) 
		std::lock_guard cs(m_lockable_send_sequence);

		// 1) generate send_sequence
		uint64_t send_sequence = ++m_send_sequence;

		// 2) send
		return send(message_SERVER_DOSS_CONTROL_REQUEST(send_sequence, _command, _cid, _rid, _result));
	}
}

void doss::Idepot::socket::process_send_ack(uint64_t _send_sequence) noexcept
{
	// locked) 
	{
		// SCOPED_LOCK) 
		std::lock_guard cs(m_lockable_send_sequence);

		// check)
		CGASSERT_ERROR(m_send_sequence >= _send_sequence);

		// check)
		CGASSERT_ERROR(m_send_sequence_ack < _send_sequence);

		// 1) update send_sequence
		m_send_sequence_ack = _send_sequence;

		// 2) popup ack
		//m_send_sequence_buffer.pop_front();
	}
}

void doss::Idepot::socket::process_receive_ack(uint64_t _receive_sequence) noexcept
{
	// check)
	CGASSERT_ERROR(m_receive_sequence < _receive_sequence);

	// 1) update send_sequence
	m_receive_sequence.store(_receive_sequence);

	// 2) send ack
	send(message_SERVER_DOSS_SEND_ACK(_receive_sequence));
}

void doss::Idepot::socket::on_connect()
{
	// 1) get group
	//    - the start object of the connective (the acceptor of a depot starts sockets in 'group_immigration')
	//    - or the connective itself if it is a group
	object_ptr<Igroup_enterable<socket>> pgroup;

	if (auto pconnective = dynamic_cast<net::io::Iconnective*>(this->m_pconnectable_notify); pconnective != nullptr)
		pgroup = pconnective->start_object<Igroup_enterable<socket>>();

	if (pgroup.empty())
		pgroup = dynamic_cast<Igroup_enterable<socket>*>(this->m_pconnectable_notify);

	// check) 
	THROW_IF(pgroup == nullptr, throwable::failure(eRESULT::NOT_READY), LOG_ERROR << "(err ) doss depot is not initialized[group is NULL]");

	// 2) Group에 Enter한다.
	auto result = pgroup->enter(this);

	// check) Enter에 실패했으면 접속을 종료한다.
	if(result != eRESULT::SUCCESS)
	{
		closesocket();
	}
}

void doss::Idepot::socket::on_disconnect(uint64_t /*_Reason*/)
{
	leave_group();
}

result_code doss::Idepot::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) set source
		_msg.set_source(this);

		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);
	}
	catch(...)
	{
		// log)
		LOG_FATAL << "(excp) doss depot_socket: exception occur on process message";
	}

	// return)
	return eRESULT::BYPASS;
}


}