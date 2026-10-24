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

	// 2) destroy the default logger
	//    - the queued logs are passed to the child loggers (they are written when the child loggers are destroyed)
	if (plogger_default.exist())
	{
		if (auto pdestroyable = dynamic_cast<object::Idestroyable*>(plogger_default.get()); pdestroyable != nullptr)
			pdestroyable->destroy();
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
		plogger = _plogger;
		set_default_logger(std::move(_plogger));

	}
	else
	{
		// - get old logger
		auto plogger_old = get_default_logger();

		// check)
		if (plogger_old.exist())
		{
			plogger = dynamic_pointer_cast<logger_hub>(plogger_old);
			return plogger;
		}

		// - create default logger
		auto plogger_hub = make_object<logger_hub_archive>();

		// - start logger
		plogger_hub->initialize();

		// - 
		plogger = plogger_hub;

		// - set logger
		set_default_logger(std::move(plogger_hub));
	}

	// return) 
	return plogger;
}