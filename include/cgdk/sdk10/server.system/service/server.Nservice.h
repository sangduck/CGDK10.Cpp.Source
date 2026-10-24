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
// ----------------------------------------------------------------------------
//
// Nservice
//
//
// ----------------------------------------------------------------------------
class server_system::Nservice :
	public						Nstatic_service_logger,
	public						Iservice,
	public						object::Ninitializable_startable,
	public						object::Npausable,
	virtual public				Imessageable,
	virtual public				Nmessage_transmitter
{
public:
			Nservice() noexcept;
			Nservice(const std::string_view _name) noexcept;
	virtual ~Nservice() noexcept;

	[[nodiscard]] auto&			host_instance() noexcept { return this->m_phost_instance; }
	[[nodiscard]] const			auto& host_instance() const noexcept { std::lock_guard lock(this->m_lockable_setting); return this->m_phost_instance; }
			void				host_instance(object_ptr<server_system::host::instance_local>&& _phost_instance);

	[[nodiscard]] auto			get_service_instance() const noexcept { std::lock_guard lock(this->m_lockable_setting); return this->m_pservice_instance.get_object_ptr(); }
			void				set_service_instance(own_ptr<server_system::service::instance_local>&& _pservice_instance);
			bool				wait_destroy(const chrono::tick::duration& _wait = chrono::tick::duration::max());

protected:
	virtual	void				on_initialize_setting(context& _context);
	virtual void				on_start_setting(context& _context);

	virtual	result_code			process_initialize(context& _context) override;
	virtual	result_code			process_destroy() noexcept override;
	virtual	result_code			process_start(context& _context) override;
	virtual	result_code			process_stop() noexcept override;
	virtual	void				process_pause() override;
	virtual	void				process_resume() override;

	virtual result_code			_process_initializing(context& _context) override;
	virtual void				_process_initializing_service_instance(context& _context);
	virtual void				_process_initializing_service_setting(context& _context);
	virtual void				_process_initializing_loggers(context& _context);
	virtual void				_process_initialize_complete(context& _context) override;
	virtual result_code			_process_starting(context& _context) override;
	virtual	void				_process_start_complete(context& _context) override;
	virtual	void				_process_stop_complete() noexcept override;
	virtual	void				_process_destroy_complete() noexcept override;

	virtual	void				process_state_changed(eOBJECT_STATE _state_new, eOBJECT_STATE _state_old) override;

	virtual	void				process_set_name(std::string_view _name) override;
	[[nodiscard]] virtual bool	process_is_this(std::string_view _str_compare) const noexcept override;
	[[nodiscard]] virtual auto	process_get_name() const noexcept -> std::string override;

	[[nodiscard]] virtual auto	process_alloc_service_instance_local(context& _context) -> own_ptr<server_system::service::instance_local>;
	[[nodiscard]] virtual auto	process_alloc_service_instance_manager(context& _context) -> own_ptr<server_system::service::manager>;
	[[nodiscard]] virtual auto	process_alloc_host_instance_manager(context& _context) -> own_ptr<server_system::host::manager>;
	
	mutable	lock_object			m_lockable_setting;
			object_ptr<server_system::host::instance_local> m_phost_instance;
			own_ptr<server_system::service::instance_local> m_pservice_instance;
			std::string			m_name_pending;		// name set before the service instance is created (applied when it is created)
			std::vector<own_ptr<Ilogger>> m_vector_logger;

			std::mutex			m_lockable_destroy;
			std::condition_variable	m_cv_destroy;
};


}