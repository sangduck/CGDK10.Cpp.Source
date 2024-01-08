#include "pch.h"


Iobservable_websocket::Iobservable_websocket(const std::string_view _name) :
	Nnameable(_name)
{
}

Iobservable_websocket::~Iobservable_websocket()
{
}

void Iobservable_websocket::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize OBSERVABLE['" << name() << "']";
}

void Iobservable_websocket::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy OBSERVABLE['" << name() << "']";
}

void Iobservable_websocket::on_start(context& /*_context*/)
{
}

void Iobservable_websocket::on_stop()
{
}
