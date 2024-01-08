#include "pch.h"

namespace CGDK
{


doss::Ndepot::Ndepot()
{
}

doss::Ndepot::Ndepot(const std::string_view _name) : doss::Idepot(_name)
{
}

doss::Ndepot::~Ndepot()
{
}

void doss::Ndepot::checkout_all() noexcept
{
}

void doss::Ndepot::on_initializing(context& /*_context*/)
{
	m_id_repository = sREPOSITORY_ID::generate();
}

void doss::Ndepot::on_initialize(context& _context)
{
	// 1) create object
	auto pgroup_socket_immigration = make_own<Idepot::group_immigration>("groupSocketImmigration");
	auto pacceptor_remository = make_own<net::acceptor<depot::socket>>("acceptor");

	try
	{
		// 2) initialize group socket immigration
		pgroup_socket_immigration->initialize(_context);
		pgroup_socket_immigration->set_next_group(this);

		// 3) initialize acceptor
		pacceptor_remository->initialize(_context);
		pacceptor_remository->set_start_object(pgroup_socket_immigration);
	}
	catch(...)
	{
		pgroup_socket_immigration->set_next_group(nullptr);
		pacceptor_remository->set_start_object(nullptr);

		// reraise) 
		throw;
	}

	// 4) set objects
	m_pgroup_socket_immigration	 = std::move(pgroup_socket_immigration);
	m_pacceptor_remository = std::move(pacceptor_remository);
}

void doss::Ndepot::on_destroying()
{
	// 1) destroy acceptor
	m_pacceptor_remository->set_start_object(nullptr);
	m_pacceptor_remository.destroy();

	// 2) destroy group socket immigration
	m_pgroup_socket_immigration->set_next_group(nullptr);
	m_pgroup_socket_immigration.destroy();

	// 3) ...
	checkout_all();
}

void doss::Ndepot::on_destroy()
{
	// 1) 
	m_id_repository.reset();
}

// 2) start/stop
void doss::Ndepot::on_starting(context& /*_context*/)
{
}

void doss::Ndepot::on_start(context& _context)
{
	// 1) Enter를 Enable한다.
	enable_member_enter();

	// 2) port 값이 없으면 기본 값을 설정한다.
	if (auto& context_find = _context["acceptor"sv]["local endpoint"sv]["port"sv]; context_find.is_invalid() || ((uint16_t)context_find) == 0)
	{
		context_find = 31001;
	}

	// 3) group Socket Immigration을 시작한다.
	m_pgroup_socket_immigration->start(_context);

	// 4) acceptor Repository를 시작한다.
	m_pacceptor_remository->start(_context);
}

void doss::Ndepot::on_stopping()
{
	// 1) acceptor Repository를 종료한다.
	m_pacceptor_remository->stop();

	// 2) group Socket Immigration을 종료한다.
	m_pgroup_socket_immigration->stop();

	// 3) Enter를 먼저 Disable한다.
	disable_member_enter();
}

void doss::Ndepot::on_stop()
{
}

result_code doss::Ndepot::on_member_entering(member_t* /*_pmember*/, sMESSAGE& /*_context*/)
{
	// return) 
	return eRESULT::SUCCESS;
}

void doss::Ndepot::on_member_entered(member_t* _pmember, sMESSAGE& _context)
{
	// check)
	LOG_INFO << " > doss DEPOT_REMOTE['"sv << this->name() << "']: remote depot entered ("sv << _pmember->remote_endpoint().address().to_string<char>() << ")"sv;

	// 1) 
	_pmember->register_messageable(this);

	// declare)
	auto& msg = (sMESSAGE_NETWORK&)_context;

	// 2) 데이터를 읽기 위해 임시 버퍼...
	auto buf_temp = msg.buf_message + offset(8);

	// 3) Repository ID를 얻는다.
	/*auto& rid =*/ buf_temp.extract<sREPOSITORY_ID>();
	auto rid_remote = buf_temp.extract<sREPOSITORY_ID>();
	/*auto receive_sequence_ack =*/ buf_temp.extract<uint64_t>();

	// 4) rid_remote값이 존재하고 현Repository와 값이 동일하면 복원을 시도한다.
	if(rid_remote.exist() && rid_remote != this->m_id_repository)
	{
		// - 먼저 접속된 Member들 중에 rid가 동일한 것이 있는지 확인한다.(자기 자신은 제외한다.)
		auto psocket_exist = process_find_member(rid_remote, _pmember);

		// - Member 중에 있으면 기존 Member를 이어받고 기존의 접속에게는 접속해제를 요구한다.
		//    - 이런 경우는 일반적으로 Ghost 접속일 경우라고 할수 있다.
		if(psocket_exist.exist())
		{
			// - set remote_repository_id
			_pmember->m_rid_remote = rid_remote;
			_pmember->m_id_session = sSESSION_ID(rid_remote, sOBJECT_ID(0llu));

			// - 기존 접속은 종료한다.
			psocket_exist->closesocket();
		}
		// - Member 중에 없으면 저장된 데이터들에서 찾는다.
		else
		{
			//if(rid_send.exist())
			//{
			//	// check)
			//	LOG_INFO << " > doss DEPOT_REMOTE['" << this->name() << "']: Recover from existing Socket";
			//	LOG_INFO << "     rid : " << rid_remote.key_depot << "/" << rid_remote.key_certify << "' ";
			//	LOG_INFO << "     remote rid : " << rid_remote.key_depot << "/" << rid_remote.key_certify << "' ";
			//}
			//else
			//{
			//	// check)
			//	LOG_INFO << " > doss DEPOT_REMOTE['" << this->name() << "']: Reset Remote Repository";
			//	LOG_INFO << "     rid : " << rid_remote.key_depot << "/" << rid_remote.key_certify << "' ";
			//	LOG_INFO << "     remote rid : " << rid_remote.key_depot << "/" << rid_remote.key_certify << "' ";
			//}
		}
	}

	// check)
	LOG_INFO << " > doss DEPOT_REMOTE['"sv << this->name() << "']: transfer from existing Socket"sv;
	LOG_INFO << "     repository_id : "sv << rid_remote.object_id();

	// 5) 비어있으면 초기화를 한다.
	if (rid_remote.empty())
	{
		// - reset depot_info
		_pmember->reset_depot_info();

		// - remote repository_id를 할당한다.
		rid_remote = sREPOSITORY_ID::generate();

		// - set remote_repository_id
		_pmember->m_rid_remote = rid_remote;
		_pmember->m_id_session = sSESSION_ID(rid_remote, sOBJECT_ID(0llu));

		// - clear checkin info

		// log) 
		LOG_INFO << " > doss DEPOT_REMOTE['"sv << this->name() << "']: reset remote repository"sv;
		LOG_INFO << "     repository_id : "sv, rid_remote.object_id();
	}

	// check)
	if (this->m_id_repository.empty())
	{
		// - closesocket
		_pmember->closesocket();

		// rturn) 
		return;
	}

	// 6) 위임받은 repository_id에 대한 정보를 얻는다.
	std::vector<sREPOSITORY_ID> vec_id_repository;
	vec_id_repository.push_back(this->m_id_repository);

	// 7) 결과 Recover 정보를 전송한다.
	_pmember->send(message_SERVER_DOSS_RESPONSE_CERTIFY(rid_remote, vec_id_repository, _pmember->m_receive_sequence));

}

uintptr_t doss::Ndepot::on_member_leaving(member_t* _pmember)
{
	// 1) _pmember의 this를 unregister_messageable한다.
	_pmember->unregister_messageable(this);

	//// 2) 추가한다.
	//scoped_lock(m_map_stored)
	//{
	//	if(_pmember->m_rid_remote.exist() && !_pmember->m_list_entity_checkined.empty())
	//	{
	//		// - 추가한다.
	//		auto result = m_map_stored.insert(std::make_pair(_pmember->m_rid_remote, circular_list<sCHECKIN_ID>()));

	//		// check) 추가에 실패해서는 절다 안됀다!
	//		CGASSERT_ERROR(result.second == true);

	//		// - 데이터 처리
	//		_pmember->m_list_entity_checkined.swap(result.first->second);
	//	}
	//}

	// check)
	LOG_INFO << " < doss DEPOT_REMOTE['"sv << this->name() << "']: remote depot leaved ("sv << _pmember->remote_endpoint().address().to_string<char>() << ")"sv;

	// return) 
	return	0;
}

void doss::Ndepot::on_member_leaved(member_t* /*_pmember*/, uintptr_t /*_result*/)
{
}

BEGIN_CGMESSAGE_MAP(doss::Ndepot)
	CGMESSAGE_TYPE_SUB_BUFFER_NETWORK
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::COMMAND_REQUEST,	on_message_SERVER_DOSS_COMMAND_REQUEST,	sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::COMMAND_CANCEL,	on_message_SERVER_DOSS_COMMAND_CANCEL,	sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::COMMAND_FAIL,		on_message_SERVER_DOSS_COMMAND_FAIL,	sMESSAGE_NETWORK)
		ON_CGMESSAGE(eMESSAGE::SERVER::DOSS::SEND_ACK,			on_message_SERVER_DOSS_SEND_ACK,		sMESSAGE_NETWORK)
	END_SUB_CGMESSAGE_MAP
END_CGMESSAGE_MAP

result_code doss::Ndepot::on_message(sMESSAGE_NETWORK& _context)
{
	try
	{
		// 1) Message Map
		EXECUTE_CGMESSAGE_MAP(_context);
	}
	catch(...)
	{
		// log)
		LOG_FATAL << "(excp) doss depot_remote["sv << this->name() << "]: exception occur on process message"sv;
	}

	// return) 
	return eRESULT::BYPASS;
}

result_code doss::Ndepot::on_message_SERVER_DOSS_COMMAND_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// declare) 
	auto psocket = _msg.get_source<depot::socket>();

	// 1) ...
	auto buf_temp = _msg.buf_message + offset(8);

	// 2) ...
	auto recv_seq = buf_temp.extract<uint64_t>();
	auto option = buf_temp.extract<uint64_t>();
	auto type = buf_temp.extract<eTRANSACTION_TYPE>();
	auto cid = buf_temp.extract<sCHECKIN_ID>();
	auto rid = buf_temp.extract<sREQUEST_ID>();

	// declare) 
	object_ptr<Ientity> pentity;

	try
	{
		// 3) get repository
		auto prepository = this->get_repository(buf_temp);

		// check) throw exception if repository is not exist
		THROW_IF(prepository.empty(), throwable::failure(eRESULT::NOT_EXIST));

		// 4) get entity 
		pentity = prepository->get_entity(cid.id_entity);

		// check)
		THROW_IF(pentity.empty(), throwable::failure(eRESULT::NOT_EXIST));

		// 4) get value
		auto buf_request = buf_temp.extract<shared_buffer>();

		// 5) Transactable 객체를 생성한다.
		auto ptransactable = make_object<Itransactable>();
		ptransactable->m_type = type;
		ptransactable->m_rid_source = rid;
		ptransactable->m_rid = sREQUEST_ID(psocket->get_session_id(), 0);
		ptransactable->m_option = option;
		ptransactable->m_buf_request = buf_temp;
		ptransactable->m_prepository = prepository;
		ptransactable->m_pentity = pentity;
		ptransactable->m_psession = this;
		ptransactable->m_cid = cid;

		// 6) request transaction
		pentity->request_transaction(ptransactable);
	}
	catch(...)
	{
		// - Error를 통보해 준다.
		psocket->send_restoreable(type, cid, rid, eRESULT::FAIL, shared_buffer());
	}

	// 7) process receive_ack
	psocket->process_receive_ack(recv_seq);

	// return) 
	return eRESULT::DONE;
}

result_code doss::Ndepot::on_message_SERVER_DOSS_COMMAND_CANCEL(sMESSAGE_NETWORK& /*_msg*/)
{
	// return) 
	return eRESULT::DONE;
}

result_code doss::Ndepot::on_message_SERVER_DOSS_COMMAND_FAIL(sMESSAGE_NETWORK& _msg)
{
	// declare) 
	auto psocket = _msg.get_source<depot::socket>();

	// 1) ...
	auto buf_temp = _msg.buf_message + offset(8);

	// 2) ...
	auto recv_seq = buf_temp.extract<uint64_t>();
	auto command  = buf_temp.extract<eTRANSACTION_TYPE>();
	auto cid = buf_temp.extract<sCHECKIN_ID>();
	/*auto rid =*/ buf_temp.extract<sREQUEST_ID>();
	/*auto result =*/ buf_temp.extract<eRESULT>();

	try
	{
		switch (command)
		{
		case	eTRANSACTION_TYPE::CHECKIN:
				{
					// - get repository
					auto prepository = this->get_repository(buf_temp);

					// check) throw exception if repository is not exist
					THROW_IF(prepository.empty(), throwable::failure(eRESULT::NOT_EXIST));

					// - find entity
					auto pentity = prepository->find_entity(cid.id_entity);

					// check) 
					THROW_IF(pentity.empty(), throwable::failure(eRESULT::NOT_EXIST));

					// - cancel checkin
					prepository->process_cancel_checkin(pentity, cid);
				}
				break;

		default:
				break;

		}
	}
	catch(...)
	{
	}

	// 7) process ack
	psocket->process_receive_ack(recv_seq);

	// return) 
	return eRESULT::DONE;
}

result_code doss::Ndepot::on_message_SERVER_DOSS_SEND_ACK(sMESSAGE_NETWORK& _msg)
{
	// declare) 
	auto psocket = _msg.get_source<depot::socket>();

	// 1) ...
	auto buf_temp = _msg.buf_message + offset(8);

	// 2) ...
	auto receive_seq = buf_temp.extract<uint64_t>();

	// 3) set...
	psocket->process_send_ack(receive_seq);

	// return) 
	return eRESULT::DONE;
}

void doss::Ndepot::process_entity_transaction_request(doss::Itransactable* /*_ptransactable*/) noexcept
{
}

void doss::Ndepot::process_entity_transaction_complete(doss::Itransactable* _ptransactable) noexcept
{
	// check) nothing if type is 'eTRANSACTION_TYPE::CHECKOUT'
	RETURN_IF(_ptransactable->m_type == eTRANSACTION_TYPE::CHECKOUT);

	// check) 
	CGASSERT_ERROR(_ptransactable->m_pentity.exist());

	// check) throw exception if repository of entity is not exist
	RETURN_IF(_ptransactable->m_pentity.empty());
	
	// check) 
	CGASSERT_ERROR(_ptransactable->m_pentity->m_prepository.exist());
	
	// check) throw exception if repository of entity is not exist
	RETURN_IF(_ptransactable->m_pentity->m_prepository.empty());
	
	// 2) find socket depot
	auto psocket_depot = process_find_member(_ptransactable->m_rid.session_id());

	// check) 
	RETURN_IF(psocket_depot.empty());
	
	// 3) send RESPONSE message
	psocket_depot->send_restoreable(_ptransactable);
}

void doss::Ndepot::process_entity_set(Itransactable* _ptransactable) // const sCHECKIN_ID& /*_cid*/, Ientity* /*_pentity*/)
{
	scoped_lock(m_lockable_map_cid)
	{
		m_map_cid.insert({ _ptransactable->m_cid, _ptransactable->m_rid.session_id()});
	}
}

bool doss::Ndepot::process_entity_reset(Ientity* /*_pentity*/, const sCHECKIN_ID& _cid, bool _is_force) noexcept
{
	// check)
	RETURN_IF(_cid.empty(), false);

	// check)
	RETURN_IF(_is_force == false, true);

	// declare)
	sSESSION_ID id_session;

	// 1) get session_id
	scoped_lock(m_lockable_map_cid)
	{
		// - get cid
		auto iter_find = m_map_cid.find(_cid);

		// check)
		CGASSERT_ERROR(iter_find != m_map_cid.end());

		// - get repository_index
		id_session = iter_find->second;

		// - erase
		m_map_cid.erase(iter_find);
	}

	// 2) get socket
	auto psocket = process_find_member(id_session);

	// check)
	RETURN_IF(psocket.empty(), false);

	// 3) notify
	psocket->send_restoreable_control_resquest(eTRANSACTION_TYPE::CHECKOUT, _cid, sREQUEST_ID(), eRESULT::SUCCESS);

	// return) 
	return true;
}

object_ptr<doss::depot::socket> doss::Ndepot::process_find_member(const sSESSION_ID& _session_id) const noexcept
{
	// declare) 
	object_ptr<doss::depot::socket> psocket_depot;

	scoped_lock(this->m_lockable_group)
	{
		for(auto& iter: this->m_container_member)
		{
			// check) _pred가 false면 전송하지 않는다.
			if (iter.member()->get_session_id() == _session_id)
			{
				psocket_depot = iter.member();
				break;
			}
		}
	}

	// return)
	return psocket_depot;
}

object_ptr<doss::depot::socket> doss::Ndepot::process_find_member(const sREPOSITORY_ID& _repository_id, member_t* _pmember_self) const noexcept
{
	// declare)
	object_ptr<member_t> psocket_exist;

	scoped_lock(m_lockable_group)
	{
		for (auto iter = begin(); iter != end(); ++iter)
		{
			// check)
			CONTINUE_IF(iter->empty());

			// check)
			CONTINUE_IF(iter->member() == _pmember_self);

			// sREPOSITORY_ID가 같은가?
			if (iter->member()->m_rid_remote == _repository_id)
			{
				psocket_exist = iter->member();
				break;
			}
		}
	}

	// return)
	return psocket_exist;
}


}