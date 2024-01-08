#include "pch.h"

using namespace CGDK;


server_system::Nstatic_service_logger::Nstatic_service_logger() noexcept
{
	initialize_logger();
}

server_system::Nstatic_service_logger::~Nstatic_service_logger() noexcept
{
	reset_default_logger();
}

object_ptr<logger_hub> server_system::Nstatic_service_logger::get_logger() noexcept
{
	// 1) get default logger
	auto plogger = get_default_logger();

	// 2) ...
	if (plogger.empty())
	{
		plogger = initialize_logger();
	}

	// 3) casting
	auto plogger_hub = dynamic_pointer_cast<logger_hub>(plogger);

	// return) 
	return plogger_hub;
}

void server_system::Nstatic_service_logger::reset_logger() noexcept
{
	// 1) unregister default logger
	auto plogger_default = reset_default_logger();

	// 2) flush logger
	if (plogger_default.exist())
	{
		plogger_default->trace(nullptr);
	}
}

object_ptr<logger_hub> server_system::Nstatic_service_logger::initialize_logger()
{
	return initialize_logger(std::move(object_ptr<logger_hub_archive>()));
}

object_ptr<logger_hub> server_system::Nstatic_service_logger::initialize_logger(object_ptr<logger_hub>&& _plogger)
{
	object_ptr<logger_hub> plogger;

	// 1)货肺款 instance甫 积己茄促.
	if(_plogger.exist())
	{
		// - set logger
		set_default_logger(_plogger);

		// - 
		plogger = _plogger;
	}
	else
	{
		// - get old logger
		auto plogger_old = get_default_logger();

		// check)
		RETURN_IF(plogger_old.exist(), dynamic_pointer_cast<logger_hub>(plogger_old));

		// - create default logger
		auto plogger_hub = make_object<logger_hub_archive>();

		// - start logger
		plogger_hub->initialize();

		// - set logger
		set_default_logger(plogger_hub);

		// - 
		plogger = plogger_hub;
	}

	// return) 
	return plogger;
}