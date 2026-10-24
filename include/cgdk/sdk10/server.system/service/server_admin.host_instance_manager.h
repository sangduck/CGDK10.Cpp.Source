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

class server_system::host::manager : 
	public                      object::Ninitializable_startable,
	public						object::Nnameable,
	public						Nmessage_transmitter, 
	virtual public				Imessageable,
	public						Ibuffer_serializable
{
public:
			void				register_instance(object_ptr<Ninstance>&& _phost_instance);
			bool				unregister_instance(iid_t _iid) noexcept;
			auto				find_instance(iid_t _iid) const noexcept -> object_ptr<Ninstance>;
			auto				alloc_instance() { return this->process_alloc_instance(); }
			bool				confirm_instance(const object_ptr<Ninstance>& _phost_instance) { return this->process_confirm_instance(_phost_instance); }
			auto				get_group_socket() const noexcept -> object_ptr<group_socket> { std::lock_guard lock(this->m_lockable_manager); return this->m_pgroup_socket; }
			void				update_instance_state(iid_t _iid, eOBJECT_STATE _state_new, eOBJECT_STATE _state_old);
			void				update_instance_info(iid_t _iid, server_system::sHOST&& _host_info);

private:
	virtual	void				on_registering_instance(const object_ptr<Ninstance>& /*_phost_instance*/) {}
	virtual	void				on_register_instance(const object_ptr<Ninstance>& /*_phost_instance*/) {}
	virtual	void				on_unregister_instance(const object_ptr<Ninstance>& /*_phost_instance*/) noexcept {}
	virtual	void				on_update_instance(const object_ptr<Ninstance>& /*_phost_instance*/) noexcept {}
	virtual	void				on_update_instance_info(const object_ptr<Ninstance>& /*_phost_instance*/) {}
	virtual	void				on_update_instance_state(const object_ptr<Ninstance>& /*_phost_instance*/, eOBJECT_STATE /*_state_new*/, eOBJECT_STATE /*_state_old*/) {}

private:
	virtual result_code			_process_initializing(context& _context) override;
	virtual result_code			_process_destroy() noexcept override;
	virtual result_code			process_message(CGDK::sMESSAGE&) override;

	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;

	virtual auto				process_alloc_instance() -> object_ptr<Ninstance>; // { return make_own<host_instance>(); }
	virtual auto				process_alloc_group_socket(context& _context) -> own_ptr<group_socket>;
	virtual bool				process_confirm_instance(const object_ptr<Ninstance>& _phost_instance);

	mutable	lock_object			m_lockable_manager;
			std::map<iid_t, object_ptr<Ninstance>> m_map_host_instances;
			own_ptr<host::group_socket> m_pgroup_socket;
};


}