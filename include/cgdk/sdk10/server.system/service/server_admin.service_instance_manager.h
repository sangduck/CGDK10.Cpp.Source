//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{

class server_system::service::manager :
	public                      object::Ninitializable_startable,
	public						object::Nnameable,
	public						Nmessage_transmitter, 
	virtual public				Imessageable,
	public						Ibuffer_serializable
{
public:
			void				register_instance(object_ptr<Ninstance>&& _service_instance);
			bool				unregister_instance(iid_t _iid) noexcept;
			auto				find_instance(iid_t _iid) const -> object_ptr<Ninstance>;
			auto				alloc_instance() { return this->process_alloc_instance(); }
			bool				confirm_instance(const object_ptr<Ninstance>& _phost_instance) { return this->process_confirm_instance(_phost_instance); }
			void				update_instance_state(iid_t _iid, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old);
			void				update_instance_info(iid_t _iid, server_system::sSERVICE&& _service_info);
			void				update_request_result(iid_t _iid, uint32_t _request, uint64_t _request_id, eRESULT _result, std::string_view _message);

private:
	virtual	void				on_registering_instance(const object_ptr<Ninstance>& /*_service_instance*/) {}
	virtual	void				on_register_instance(const object_ptr<Ninstance>& /*_service_instance*/) {}
	virtual	void				on_unregister_instance(const object_ptr<Ninstance>& /*_service_instance*/) noexcept {}
	virtual	void				on_update_instance_info(const object_ptr<Ninstance>& /*_service_instance*/) {}
	virtual	void				on_update_instance_state(const object_ptr<Ninstance>& /*_service_instance*/, eOBJECT_STATE /*_state_new*/, eOBJECT_STATE /*_state_old*/) {}
	virtual	void				on_request_result(const object_ptr<Ninstance>& /*_service_instance*/, uint32_t /*_request*/, uint64_t /*_request_id*/, eRESULT /*_result*/, std::string_view /*_message*/) {}

private:
	virtual result_code			_process_initializing(context& _context) override;
	virtual result_code			_process_destroy() noexcept override;
	virtual result_code			process_message(sMESSAGE& _msg) override;
			result_code			process_message_SERVER_REQUEST_INIT_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			process_message_SERVER_REQUEST_START_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			process_message_SERVER_REQUEST_STOP_SERVICE(sMESSAGE_NETWORK& _msg);
			result_code			process_message_SERVER_REQUEST_DESTROY_SERVICE(sMESSAGE_NETWORK& _msg);


	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;

	virtual auto				process_alloc_instance() -> object_ptr<Ninstance>;
	virtual bool				process_confirm_instance(const object_ptr<Ninstance>& _pservice_instance);

	mutable	lock_object			m_lockable_manager;
			std::map<iid_t, object_ptr<Ninstance>> m_map_service_instances;
};


}