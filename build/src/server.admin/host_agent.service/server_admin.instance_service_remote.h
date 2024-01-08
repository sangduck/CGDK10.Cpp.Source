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
class server_admin::instance_service_remote : public server_admin::instance_Iservice
{
public:
			instance_service_remote();
	virtual ~instance_service_remote() noexcept;

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
};
