//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Server Admin Host Agent                          *
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
// ----------------------------------------------------------------------------
//
// instance::instance
//
// ----------------------------------------------------------------------------
class server_system::instance_service_local : public	server_system::instance_service
{
public:
			instance_service_local();
	virtual ~instance_service_local() noexcept;

	virtual	bool				request_service_initialize() override;
	virtual	bool				request_service_destroy() override;
	virtual	bool				request_service_start(const std::string& _setting) override;
	virtual	bool				request_service_stop() override;
	virtual	bool				request_service_continue() override;
	virtual	bool				request_service_pause() override;
	virtual	bool				request_service_kill() override;
	virtual	bool				request_service_update() override;
	virtual	bool				request_service_check_out() override;
	virtual	bool				request_service_switch() override;
	virtual	void				set_instance_info(server_system::sSERVICE&& _service_info) override;
	virtual	void				reset_instance_status_info() override;
	virtual	void				update_application_status() override;

			void				load(const std::string_view p_str_path = "."sv);
			void				save();
private:
	virtual bool				process_sservice_disconnected(socket_peer* _psocket) override;

			PROCESS_HANDLE		process_handle{ INVALID_PROCESS_HANDLE };
			std::atomic<int>	status{ 0 };
			object_ptr<api::performance_counter> performance_processor;

			// - service type SERVICE/DOCKER: status check (no process handle)
			bool				process_check_service_running();
			std::chrono::steady_clock::time_point m_time_check_service_status{};
};
