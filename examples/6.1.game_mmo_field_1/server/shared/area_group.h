//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ⓒ 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once

class area::group :
	public Igroup_user,
	public	schedulable::Nexecutable
{
private:
	virtual	void				on_starting(context& _context) override;
	virtual	void				on_stopping() override;
	virtual	result_code			on_member_entering(socket_user* _pmember, sMESSAGE& _msg) override;
	virtual	uintptr_t			on_member_leaving(socket_user* _pmember, uintptr_t _param) override;
	virtual	void				on_execute() override;
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;
			result_code			on_message_AREA_ENTER_REQUEST(sMESSAGE_NETWORK& _msg);
			result_code			on_message_AREA_ENTITY_READY(sMESSAGE_NETWORK& _msg);
			result_code			on_message_AREA_ENTITY_EVENT(sMESSAGE_NETWORK& _msg);

			void				on_ENTITY_EVENT_ENTER(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_LEAVE(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_MOVE_TO(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_DAMAGE(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_ATTACK(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_ACTION(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_SKILL(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_DEATH(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_GET(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_USE(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			void				on_ENTITY_EVENT_STATE(socket_user* _psocket, Ientity* _pentity, shared_buffer& _buf, double _tick_now);
			
public:
	virtual	bool				process_send(shared_buffer&& _buffer, uint64_t _option);

			shared_buffer		message_AREA_INFO(REQUEST_ID _request_id) const;
			shared_buffer		_message_AREA_ENTITY_LIST(REQUEST_ID _request_id, double _tick) const;
	static	AREA_ENTITY_ID		_generate_new_AREA_ENTITY_ID() noexcept;
			void				_tick_reset() noexcept;
			double				_tick_now() const noexcept;

			lock_object			m_cs_area_info;
			sAREA_INFO			m_area_info;
			chrono::tick::time_point m_tick_start;

			lock_object			m_cs_map_account;
			std::map<ACCOUNT_INDEX, object_ptr<account>> m_map_account;
			std::map<AREA_ENTITY_ID, object_ptr<area::Ientity>> m_map_area_entity;

			lock_object			m_cs_vector_entity_messages;
			std::vector<shared_buffer> m_vector_entity_messages;
			uint32_t			m_count_entity_messages;

			OBJ<net::concurrent_send_queue> m_send_queue;

			//std::vector<sAREA_PORTAL> m_vector_link; // portal의 gate (world object에서 얻는다.)
};
