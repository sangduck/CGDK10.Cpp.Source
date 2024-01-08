#include "pch.h"


client::web::Iobservable::Iobservable(const std::string_view _name) :
	Nnameable(_name)
{
}

client::web::Iobservable::~Iobservable()
{
}

void client::web::Iobservable::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize OBSERVABLE['" << name() << "']";
}

void client::web::Iobservable::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy OBSERVABLE['" << name() << "']";
}

void client::web::Iobservable::on_start(context& /*_context*/)
{
}

void client::web::Iobservable::on_stop()
{
}
