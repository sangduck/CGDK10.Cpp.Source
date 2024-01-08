//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Control Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ⓒ2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// server::group_session
//
//-----------------------------------------------------------------------------
namespace time_event_server
{

class manager :
	public						Nobject_state,
	public						Ninitializable,
	public						Nstartable,
	public						net::io::messageable::Nbase,
	public						Nnameable
{
public:
			 manager(std::string_view _name);
	virtual ~manager() noexcept;

public:
			void				register_time_event		(operation* _ptime_event);
			void				unregister_time_event	(operation* _ptime_event) noexcept;
			void				update_time_event		(operation* _ptime_event);
			void				store_time_event_result	(server_operation* _pserver_operation);
			void				done_time_event_result	(operation* _ptime_event);
			object_ptr<operation> find_time_event		(uint64_t _idEvent) const noexcept;

private:
	// 1) initialize/destroy
	virtual	void				on_initializing			(context& _context) override;
	virtual	void				on_initialize			(context& _context) override;
	virtual	void				on_destroying			() override;
	virtual	void				on_destroy				() override;

	// 2) start/Stop
	virtual	void				on_starting				(context& _context) override;
	virtual	void				on_start				(context& _context) override;
	virtual	void				on_stopping				() override;
	virtual	void				on_stop					() override;

protected:
	// Declare Message Map) 
	DECLARE_CGMESSAGE_MAP

	// 1) Message 처리 함수
	virtual result_code			on_message(sMESSAGE_NETWORK& _msg) override;

	// 2) Message별 처리 함수
			result_code			on_message_SERVER_TIME_EVENT_REGISTER_EVENT	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UNREGISTER(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_EXECUTE	(sMESSAGE_NETWORK& _msg);
			result_code			on_message_SERVER_TIME_EVENT_EVENT_UPDATE	(sMESSAGE_NETWORK& _msg);

public:
			shared_buffer		message_SERVER_ADMIN_SERVER_TIME_EVENT();

private:
			uint64_t			_process_alloc_event_id					() noexcept;
			void				_ProcessAllocEventID_New				(time_event_server::operation* _ptime_event) noexcept;
			void				_ProcessAllocEventID_Update				(time_event_server::operation* _ptime_event) noexcept;
			void				_ProcessCompleteTimeEventRegister		(operation* _ptime_event, eRESULT _result);
			void				_ProcessCompleteTimeEventUnregister		(operation* _ptime_event);
			void				_process_complete_time_event_update		(operation* _ptime_event);
			//friend class		query_event::time_event_register;
			//friend class		query_event::time_event_unregister;
			//friend class		query_event::time_event_update;
			friend class		querier_server;

			lockable<>			m_csid;
			uint64_t			m_idEvent_Last;
};


}