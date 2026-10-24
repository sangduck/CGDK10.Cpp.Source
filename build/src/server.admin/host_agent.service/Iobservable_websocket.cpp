#include "pch.h"


Iobservable_websocket::Iobservable_websocket()
{
}

Iobservable_websocket::~Iobservable_websocket()
{
}

void Iobservable_websocket::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize OBSERVABLE['"sv << name() << "']"sv;
}

void Iobservable_websocket::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy OBSERVABLE['"sv << name() << "']"sv;
}

void Iobservable_websocket::on_start(context& /*_context*/)
{
}

void Iobservable_websocket::on_stop()
{
}
