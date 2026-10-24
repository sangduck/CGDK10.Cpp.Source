#include "pch.h"


shared_buffer message_AREA_ENTER_REQUEST(REQUEST_ID _request_id, AREA_INDEX _area_index)
{
	// define) 
	const size_t MESSAGE_SIZE = 8 + sizeof(REQUEST_ID) + sizeof(AREA_INDEX);

	// 1) alloc buffer
	auto buf = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) message header
	buf.append<uint32_t>(MESSAGE_SIZE);
	buf.append<uint32_t>(eMESSAGE::AREA_ENTER_REQUEST);

	// 3) message body
	buf.append<REQUEST_ID>(_request_id);
	buf.append<AREA_INDEX>(_area_index);

	// check)
	CGASSERT_ERROR(buf.size<uint32_t>() == MESSAGE_SIZE);

	// return)
	return buf;
}

shared_buffer message_AREA_ENTER_RESPONSE(REQUEST_ID _request_id, eRESULT _result)
{
	// define) 
	const size_t MESSAGE_SIZE = 8 + sizeof(REQUEST_ID) + sizeof(eRESULT);

	// 1) alloc buffer
	auto buf = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) message header
	buf.append<uint32_t>(MESSAGE_SIZE);
	buf.append<uint32_t>(eMESSAGE::AREA_ENTER_RESPONSE);

	// 3) message body
	buf.append<REQUEST_ID>(_request_id);
	buf.append<eRESULT>(_result);

	// check)
	CGASSERT_ERROR(buf.size<uint32_t>() == MESSAGE_SIZE);

	// return)
	return buf;
}

shared_buffer message_AREA_ENTITY_EVENT_ENTER(double _tick, area::entity_player* _pentity)
{
	// define) 
	const size_t MESSAGE_SIZE = 8 + sizeof(double) + sizeof(COUNT_T) + sizeof(buffer_view) + sizeof(sAREA_ENTITY_EVENT) + sizeof(sAREA_ENTITY_INFO) + 1024;

	// 1) alloc buffer
	auto buf = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) message header
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::AREA_ENTITY_EVENT);

	// 3) tick now
	buf.append<double>(_tick);

	// 4) entity event count
	buf.append<COUNT_T>(1);

	// 5) append 
	{
		// - splite
		auto buf_temp = buf.remained(skip(sizeof(size_t)));

		// declare) 
		sAREA_ENTITY_EVENT area_entity_event_head;

		// - area_entity_event
		area_entity_event_head.tick = _tick;
		area_entity_event_head.id = 0;
		area_entity_event_head.event_type = eENTITY_EVENT::ENTER;

		// - append
		buf_temp.append<sAREA_ENTITY_EVENT>(area_entity_event_head);
		
		// - serialize
		_pentity->process_entity_serialize_out(buf_temp);

		// - append
		buf.append(buf_temp);
	}

	// 6) message length
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

shared_buffer message_AREA_ENTITY_EVENT_LEAVE(double _tick, AREA_ENTITY_ID _area_entity_id)
{
	// define) 
	const size_t MESSAGE_SIZE = 8 + sizeof(double) + sizeof(COUNT_T) + sizeof(buffer_view) + sizeof(sAREA_ENTITY_EVENT);

	// 1) alloc buffer
	auto buf = alloc_shared_buffer(MESSAGE_SIZE);

	// 2) message header
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::AREA_ENTITY_EVENT);

	// 3) tick now
	buf.append<double>(_tick);

	// 4) entity event count
	buf.append<COUNT_T>(1);

	// 5) entity event list
	{
		// - splite
		auto buf_temp = buf.remained() + offset(sizeof(size_t));

		// declare) 
		sAREA_ENTITY_EVENT area_entity_event_head;

		// - area_entity_event
		area_entity_event_head.tick = _tick;
		area_entity_event_head.id = _area_entity_id;
		area_entity_event_head.event_type = eENTITY_EVENT::LEAVE;

		// - append
		buf_temp.append<sAREA_ENTITY_EVENT>(area_entity_event_head);
		
		// - append
		buf.append(buf_temp);
	}

	// 7) message length
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// check)
	CGASSERT_ERROR(buf.size<uint32_t>() == MESSAGE_SIZE);

	// return)
	return buf;
}

shared_buffer area::group::message_AREA_INFO(REQUEST_ID _request_id) const
{
	// declare) 
	shared_buffer buf;

	SCOPED_LOCK(this->m_lockable_group)
	{
		// 1) alloc buffer
		buf = alloc_shared_buffer(8 + sizeof(REQUEST_ID) + get_size_of(this->m_area_info));

		// 2) message header
		buf.append<uint32_t>();
		buf.append<uint32_t>(eMESSAGE::AREA_INFO);

		// 3) message body
		buf.append<REQUEST_ID>(_request_id);

		// 4) apppend AREA_INFO
		buf.append<sAREA_INFO>(this->m_area_info);
	}

	// 4) set message length
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

shared_buffer area::group::_message_AREA_ENTITY_LIST(REQUEST_ID _request_id, double _tick) const
{
	// declare) 
	shared_buffer buf;

	// 1) alloc buffer
	buf = alloc_shared_buffer(8 + sizeof(REQUEST_ID) + sizeof(double) + sizeof(COUNT_T) + m_map_area_entity.size() * (sizeof(sAREA_ENTITY_INFO) + sizeof(sAREA_ENTITY_STATE) + 256));

	// 2) message header
	buf.append<uint32_t>();
	buf.append<uint32_t>(eMESSAGE::AREA_ENTITY_LIST);

	// 3) message body
	buf.append<REQUEST_ID>(_request_id);

	// 4) tick
	buf.append<double>(_tick);

	// 5) append entity count
	buf.append<COUNT_T>(static_cast<COUNT_T>(m_map_area_entity.size()));

	// 6) append entity
	for (auto& iter : m_map_area_entity)
	{
		iter.second->process_entity_serialize_out(buf);
	}

	// 7) set message length
	buf.front<uint32_t>() = buf.size<uint32_t>();

	// return)
	return buf;
}

AREA_ENTITY_ID area::group::_generate_new_AREA_ENTITY_ID() noexcept
{
	static AREA_ENTITY_ID entity_id_generate = 1;

	auto area_entity_id_new = entity_id_generate++;

	if(area_entity_id_new == 0)
		area_entity_id_new = entity_id_generate++;

	return area_entity_id_new;
}

void area::group::_tick_reset() noexcept
{
	this->m_tick_start = chrono::tick::now();
}

double area::group::_tick_now() const noexcept
{
	auto diff_tick = chrono::tick::now() - this->m_tick_start;

	return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(diff_tick).count()) / 1'000'000'000.0;
}


bool area::group::process_send(shared_buffer&& _buffer, uint64_t /*_option*/)
{
	// check) _buffer->size_이 0이 아닌가?
	RETURN_IF(_buffer.size() == 0, false)

	// check) _buffer->data_가 nullptr이 아닌가?
	RETURN_IF(_buffer.data() == nullptr, false)

	// check) 버퍼가 Overflow되었는가?
	CGASSERT_ERROR(_buffer._is_buffer_overflow() == false);

	// 1) 전송한다.
	SCOPED_LOCK(this->m_cs_map_account)
	{
		for (auto iter = this->m_map_account.begin(), iterEnd = this->m_map_account.end(); iter != iterEnd; ++iter)
		{
			iter->second->psocket->send(_buffer);
		}
	}

	// return)
	return true;
};

void area::group::on_starting(context& /*_context*/)
{
	// 1) enable member entering
	this->enable_member_enter();

	// 2) set executing interval
	this->execute_interval(1ms);

	// 3) reset tick time
	this->_tick_reset();

	// 4) reigister to system scheduler
	system_executor::register_schedulable(this);
}

void area::group::on_stopping()
{
	// 1) disable member entering
	this->disable_member_enter();

	// 2) unreigister from system scheduler
	system_executor::unregister_schedulable(this);

	// 3) 모든 member를 접속 종료시킨다.

}

void area::group::on_execute()
{
	// declare)
	std::vector<shared_buffer> vector_entity_messages;

	SCOPED_LOCK(this->m_cs_vector_entity_messages)
	{
		// 3) swap
		vector_entity_messages.swap(this->m_vector_entity_messages);

		// 4) send queued entity status info
		if (vector_entity_messages.empty() == false)
		{
			// - 첫 번째 buffer는 message header부분
			//vector_entity_messages.at(0) = message_AREA_ENTITY_STATE_HEADER();

			// - 전송
			this->send(vector_entity_messages);
		}
	}
}

result_code area::group::on_member_entering(socket_user* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) get account
	auto paccount = _pmember->get_account();

	// check)
	CGASSERT_ERROR(paccount.exist());

	// check)
	THROW_IF(paccount.empty(), throwable::failure(eRESULT::NOT_READY, 0, "user is not logined"sv));

	// 2) Register Messageable한다.
	_pmember->register_messageable(this);

	// 3) send area info
	_pmember->send(message_AREA_INFO(1));

	// return) 
	return eRESULT::SUCCESS;
}

uintptr_t area::group::on_member_leaving(socket_user* _pmember, uintptr_t _param)
{
	// check)
	CGASSERT_ERROR(_pmember != nullptr);

	// 1) unregister Mediator한다.
	_pmember->unregister_messageable(this);

	SCOPED_LOCK(this->m_cs_map_account)
	{
		// 2) are_entity 정보를 얻는다.
		auto paccout = _pmember->get_account();

		// check) 만약 area_entity가 없다면? 완전 잘못된 것이다.
		if (paccout.exist())
		{
			// - find entity
			auto iter_find = this->m_map_account.find(paccout->account_info.account_index);

			// check) inf not exist
			if (iter_find != this->m_map_account.end())
			{
				// check)
				CGASSERT_ERROR(paccout->area_index_now == m_area_info.id);

				// check)
				CGASSERT_ERROR(paccout == iter_find->second);

				// - remove entity
				this->m_map_account.erase(iter_find);

				if (paccout->parea_entity.exist())
				{
					// - remove 
					auto iter_find_entity = this->m_map_area_entity.find(paccout->parea_entity->area_entity_info.id);

					if (iter_find_entity != this->m_map_area_entity.end())
					{
						// - erase
						this->m_map_area_entity.erase(iter_find_entity);

						// - send 'leave message' to group
						this ->m_send_queue.enqueue({ this, message_AREA_ENTITY_EVENT_LEAVE(this->m_area_info.tick_now, paccout->parea_entity->area_entity_info.id) });
					}
				}
			}
		}
	}

	// 3) send
	this->m_send_queue.send();

	// return) 
	return _param;
}

result_code area::group::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) execute message
	BEGIN_NESTED_CGMESSAGE_MAP
		CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
			ON_CGMESSAGE(eMESSAGE::AREA_ENTER_REQUEST,	on_message_AREA_ENTER_REQUEST,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::AREA_ENTITY_READY,	on_message_AREA_ENTITY_READY,	sMESSAGE_NETWORK);
			ON_CGMESSAGE(eMESSAGE::AREA_ENTITY_EVENT,	on_message_AREA_ENTITY_EVENT,	sMESSAGE_NETWORK);
		END_SUB_CGMESSAGE_MAP
	END_NESTED_CGMESSAGE_MAP

	// return) 
	return eRESULT::DONE;
}

result_code area::group::on_message_AREA_ENTER_REQUEST(sMESSAGE_NETWORK& _msg)
{
	// 1) get socket server
	auto psocket_user = _msg.get_source<socket_user>();

	// 2) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 3) request_id
	auto request_id = buf.extract<REQUEST_ID>();

	try
	{
		// 4) get area id
		auto area_index = buf.extract<AREA_INDEX>();

		// 5) get area
		auto pgroup_area = g_pservice->m_parea_mamanger->find_area(area_index);

		// check)
		THROW_IF(pgroup_area.empty(), throwable::failure(eRESULT::NOT_EXIST, 0, ""sv));

		// 6) can enter?


		// 7) enter!
		auto result = pgroup_area->enter(psocket_user);

		// check)
		THROW_IF(result != eRESULT::SUCCESS, throwable::failure(result, 0, ""sv));
	}
	catch (throwable::failure& _e)
	{
		psocket_user->send(message_AREA_ENTER_RESPONSE(request_id, _e.reason()));
	}
	catch (...)
	{
		psocket_user->closesocket();
	}

	// return) 
	return eRESULT::DONE;
}

result_code area::group::on_message_AREA_ENTITY_READY(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) ENTER시 AREA_INFO를 받은 클라이언트가 로딩을 완료한 후 전송해 오는 메시지
	//       이때 사용자 정보 등을 전달한다.
	//       주의) 입장 퇴장 정보를 놓지지지 않도록 제대로된 동기화 처리에 주의 해야 한다.
	//-----------------------------------------------------------------

	// 1) get socket server
	auto psocket_user = _msg.get_source<socket_user>();

	// 2) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 3) request_id
	auto request_id = buf.extract<REQUEST_ID>();

	// 4) are_entity 정보를 얻는다.
	auto paccount = psocket_user->get_account();

	// check) 만약 area_entity가 없다면? 완전 잘못된 것이다.
	THROW_IF(paccount.empty(), throwable::failure(eRESULT::NOT_READY, 0, ""sv));

	// 5) 신규 플레이어를 등록한다.
	//    신규 플레이어게 Area 전체 플레이어 정보를 전송한 후
	//    Area 전체 플레이어들에게 신규 플레이어의 입장 정보를 전송한다.
	SCOPED_LOCK(this->m_cs_map_account)
	{
		// check)
		CGASSERT_ERROR(paccount->parea_entity.exist());

		// 6) account id를 얻는다.
		auto account_index = paccount->account_info.account_index;

		// 7) 이미 존재하는지를 확인한다.
		auto iter_find_account = this->m_map_account.find(account_index);

		// 8) 존재하지 않으면?
		if (iter_find_account == this->m_map_account.end())
		{
			// - account를 등록한다.
			auto result_insert = this->m_map_account.insert({ account_index, paccount });

			// check) 
			CGASSERT_ERROR(result_insert.second == true);

			// check) 
			THROW_IF(result_insert.second == false, throwable::failure(eRESULT::ALREADY_EXIST, 0, "it fail to insert ared_entity!"sv));

			// declare) 
			object_ptr<area::entity_player> pentity = paccount->parea_entity;

			try
			{
				// - ready enter
				{
					pentity->area_entity_info.id = _generate_new_AREA_ENTITY_ID(); // 새로운 AREA_ENTITY_ID를 발급한다.
					pentity->area_entity_status.tick = 0.0;
					pentity->area_entity_status.translation = position3{ 1200.0, 1500.0, 499.0 };
					pentity->area_entity_status.rotator = rotation3{ 0.0, 0.0, 0.0 };
					pentity->area_entity_status.acceleration = position3{ 0.0, 0.0, 0.0 };
					pentity->area_entity_status.velocity = position3{ 0.0, 0.0, 0.0 };
					pentity->area_entity_last = shared_buffer();
				}

				// - get tick now
				auto tick_now = this->_tick_now();

				// - send user info (새로 들어온 멤버를 추가하기전에 작성한다.)
				this->m_send_queue.enqueue({ std::move(psocket_user), this->_message_AREA_ENTITY_LIST(request_id, tick_now) });

				// check) 이미 등록된 상태인지 확인...
				auto iter_find = this->m_map_area_entity.find(pentity->area_entity_info.id);

				// check)
				THROW_IF(iter_find != this->m_map_area_entity.end(), throwable::failure(eRESULT::ALREADY_EXIST, 0, "ared_entity aleady exist!"sv));

				// - add to area entity list
				this->m_map_area_entity.insert({ pentity->area_entity_info.id, pentity });

				// - reset for new enter
				paccount->area_index_now = m_area_info.id;

				// - send 전체..
				this->m_send_queue.enqueue({ this, message_AREA_ENTITY_EVENT_ENTER(tick_now, pentity) });
			}
			catch(...)
			{
				// - rollback
				this->m_map_account.erase(account_index);

				// reraise)
				throw;
			}
		}
		else
		{
			// 지금은 일단 있을 수 없다!
			// - 재접속 기능을 넣으면 필요할 수 있다.
			CGASSERT_ERROR(false);
		}
	}

	// 5) send
	this->m_send_queue.send();

	// return) 
	return eRESULT::DONE;
}

result_code area::group::on_message_AREA_ENTITY_EVENT(sMESSAGE_NETWORK& _msg)
{
	//-----------------------------------------------------------------
	// 설명) ENTITY의 이벤트 정보 (공격, 타격, 동작, ... 즉각적 이벤트들)
	//       즉시 처리 및 전송이 필요한 데이터들이므로 바로 echo 전송을 수행한다.
	//       받은 message의 tick 자리에 server tick을 써넣어 모든 clinet에 전송한다.
	//-----------------------------------------------------------------
	// 1) get socket server
	auto psocket_user = _msg.get_source<socket_user>();

	// 2) channel info를 얻는다.
	auto buf = _msg.buf_message + offset(8);

	// 3) get server tick
	auto tick_now = this->_tick_now();

	// 4) get tick and set server tick
	{
		// - get pointer of tick
		auto& tick_server_now = buf.extract<double>();

		// - set server tick time
		tick_server_now = tick_now;
	}

	// 5) ENTITY_EVENT의 개수를 얻는다.
	auto event_count = buf.extract<COUNT_T>();

	// check) event 숫자가 0개몇 안된다.
	CGASSERT_ERROR(event_count != 0);

	// check) event숫자가 0개일 수는 없다.
	THROW_IF(event_count == 0, throwable::failure(eRESULT::INVALID_REQUEST, 0, "entity_event must be 1"sv));

	// 6) 전송한다.
	SCOPED_LOCK(this->m_cs_map_account)
	{
		while (event_count > 0)
		{
			// - get buffer
			auto buf_event = buf.extract<shared_buffer>();

			// - get area id
			auto& area_entity_event_head = buf_event.front<sAREA_ENTITY_EVENT>();

			// - entity id에 맞는 entity를 찾는다.
			auto iter_find = this->m_map_area_entity.find(area_entity_event_head.id);

			// check) 
			THROW_IF(iter_find == this->m_map_area_entity.end(), throwable::failure(eRESULT::INVALID_MEMBER, 0, "entity nit exist"sv));

			// - process event by type
			switch (area_entity_event_head.event_type)
			{
			case	eENTITY_EVENT::ENTER:
					this->on_ENTITY_EVENT_ENTER(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::LEAVE:
					this->on_ENTITY_EVENT_LEAVE(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::MOVE_TO:
					this->on_ENTITY_EVENT_MOVE_TO(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::DAMAGE:
					this->on_ENTITY_EVENT_DAMAGE(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::ATTACK:
					this->on_ENTITY_EVENT_ATTACK(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::ACTION:
					this->on_ENTITY_EVENT_ACTION(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::SKILL:
					this->on_ENTITY_EVENT_SKILL(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::DEATH:
					this->on_ENTITY_EVENT_DEATH(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::GET:
					this->on_ENTITY_EVENT_GET(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::USE:
					this->on_ENTITY_EVENT_USE(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			case	eENTITY_EVENT::STATE:
					this->on_ENTITY_EVENT_STATE(psocket_user, iter_find->second, buf_event, tick_now);
					break;

			default:
					break;
			};

			// - de-count
			--event_count;
		}

		// - 바로 전체로 전송한다.
		this->m_send_queue.enqueue({ this, _msg.buf_message });
	}

	// 7) 전송...
	this->m_send_queue.send();

	// return) 
	return eRESULT::DONE;
}

void area::group::on_ENTITY_EVENT_ENTER(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{
	// Error) 클라에서는 이 신호가 올 수 없다.
	CGASSERT_ERROR(false);
}

void area::group::on_ENTITY_EVENT_LEAVE(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{
}

void area::group::on_ENTITY_EVENT_MOVE_TO(socket_user* /*_psocket*/, Ientity* _pentity, shared_buffer& _buf, double /*_tick_now*/)
{
	// 1) skip event status
	/*auto& entity_event_head* = */ _buf.extract<sAREA_ENTITY_EVENT>();

	// 2) get entity status
	auto& entity_status = _buf.extract<sAREA_ENTITY_STATE>();

	// 3) last command
	auto buf_last = alloc_shared_buffer(sizeof(eENTITY_EVENT::TYPE) + sizeof(double) + sizeof(position3));
	buf_last.append<eENTITY_EVENT::TYPE>(eENTITY_EVENT::MOVE_TO);
	buf_last.append<double>(entity_status.tick);
	buf_last.append<position3>(_buf.extract<position3>());

	// 4) set entity
	SCOPED_LOCK(_pentity->cs_area_entity)
	{
		_pentity->area_entity_status = entity_status;
		_pentity->area_entity_last = std::move(buf_last);
	}
}

void area::group::on_ENTITY_EVENT_DAMAGE(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_ATTACK(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_ACTION(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_SKILL(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_DEATH(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_GET(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_USE(socket_user* /*_psocket*/, Ientity* /*_pentity*/, shared_buffer& /*_buf*/, double /*_tick_now*/)
{

}

void area::group::on_ENTITY_EVENT_STATE(socket_user* /*_psocket*/, Ientity* _pentity, shared_buffer& _buf, double _tick_now)
{
	// 1) skip event status
	/*auto& entity_event_head* = */ _buf.extract<sAREA_ENTITY_EVENT>();

	// 2) get status
	auto& entity_status = _buf.extract<sAREA_ENTITY_STATE>();

	// 3) set entity
	SCOPED_LOCK(_pentity->cs_area_entity)
	{
		_pentity->area_entity_status = entity_status;
	}

	// 4) get lag
	auto differ_lag = _tick_now - entity_status.tick;

	if (differ_lag > 0.0001)
	{
		// log)
		LOG_DEBUG << "lag "sv << differ_lag;
	}
}
