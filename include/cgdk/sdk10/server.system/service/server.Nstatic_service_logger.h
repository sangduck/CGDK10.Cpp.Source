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
// Nstatic_service_logger
//
// 
// ----------------------------------------------------------------------------
class server_system::Nstatic_service_logger
{
protected:
			Nstatic_service_logger() noexcept;
	virtual	~Nstatic_service_logger() noexcept;

public:
	static	auto				get_logger() noexcept -> object_ptr<logger_hub>;
	static	auto				initialize_logger() -> object_ptr<logger_hub>;
	static	auto				initialize_logger(object_ptr<logger_hub>&& _plogger) -> object_ptr<logger_hub>;

protected:
	static	void				reset_logger() noexcept;
};


}