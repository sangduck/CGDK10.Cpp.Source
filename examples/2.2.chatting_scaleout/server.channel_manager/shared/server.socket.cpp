#include "pch.h"


void server::socket::on_connect()
{
	// Trace) 
	LOG_INFO << log_level_t::higher() << "> Lobby Server Connected ["sv << remote_endpoint().to_string() << "]";

	// View)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_CONNECT, (DWORD_PTR)this));

	// 1) Enter groupUserImmigration
	g_pservice->pgroup_server_Immigration->enter(this);
}

void server::socket::on_disconnect(uint64_t /*_reason*/)
{
	// View)
	transmit_message_to(g_pservice, sMESSAGE_NOTIFY(eMESSAGE::WINDOWS::NOTIFY_UPDATE, CGNOTIFY_LOBBYSERVER_DISCONNECT, (DWORD_PTR)this));

	// 1) Leave from group
	this->leave_group();

	// Trace)
	LOG_INFO << log_level_t::higher() << "< Lobby Server disconnected ["sv << remote_endpoint().to_string() << "]"sv;
}

result_code server::socket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) set source
	_msg.set_source(this);

	try
	{
		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);
	}
	catch (...)
	{
		// Error!)
		CGASSERT_ERROR(false);
	}

	// return) 
	return eRESULT::BYPASS;
}
