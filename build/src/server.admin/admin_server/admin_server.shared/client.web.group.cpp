#include "pch.h"


client::web::group::group(const std::string_view _name) :
	Nnameable(_name)
{
	//m_pobservable_websocket_system_info = make_own<observable_websocket_system_info>("observable_websocket_system_info"); Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_system_info);
	//m_pobservable_websocket_host_info   = make_own<observable_websocket_host_info>("observable_websocket_host_info");	  Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_host_info);
	//m_pobservable_websocket_server_info = make_own<observable_websocket_server_info>("observable_websocket_server_info"); Nattachable<Iobject_state>::process_attach(m_pobservable_websocket_server_info);
}

client::web::group::~group()
{
}

void client::web::group::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize GROUP['" << name() << "']";

	// 1) Register MediatorÇÑ´Ù.
	register_messageable(g_pservice);
}

void client::web::group::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy GROUP['" << name() << "']";

	// 1) register mediator
	unregister_messageable(g_pservice);
}

void client::web::group::on_start(context& /*_context*/)
{
	// 1) enable entering
	enable_member_enter();
}

void client::web::group::on_stop()
{
	// 1) disable entering
	disable_member_enter();
}

result_code client::web::group::on_member_entering(socket* /*_pmember*/, sMESSAGE& /*_msg*/)
{
	// return)
	return eRESULT::SUCCESS;
}

void client::web::group::on_member_entered(socket* _pmember, sMESSAGE& /*_msg*/)
{
	//// 1) register to observable
	//m_pobservable_websocket_system_info->subscribe(_pmember, OCID_SYSTEM);

	// 2) register _pmenber
	_pmember->register_messageable(this);

	//// 3) set initial page
	//_pmember->m_webpage_ptarget = m_pobservable_websocket_host_info;
	//_pmember->send(_pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE, m_pobservable_websocket_host_info->m_webpage_str_target));
}

uintptr_t client::web::group::on_member_leaving(socket* _pmember, sMESSAGE& /*_msg*/)
{
	// 1) unregister to observable
	_pmember->unsubscribe(OCID_SYSTEM);

	// 2) uregister _pmenber
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void client::web::group::on_member_leaved(socket* /*_pmember*/, sMESSAGE& /*_msg*/, uintptr_t /*_result*/)
{
}

result_code client::web::group::on_message(sMESSAGE_NETWORK& /*_msg*/)
{
	//// 1) web message 
	//sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	//// 2) get message head
	//auto message_type = msg.buf_message.front_text<char>(size(4));
	//auto pmember = msg.get_source<socket*>();

	//// 3) 
	//if (message_type[0] == 'A')
	//{
	//	switch(message_type[1])
	//	{
	//	case 'A':
	//			{
	//				// declare) 
	//				object_ptr<Iobservable_websocket> pobservable_webpage;

	//				switch (message_type[2])
	//				{
	//				case	'A':
	//						pobservable_webpage = m_pobservable_websocket_host_info;
	//						break;
	//				case	'B':
	//						pobservable_webpage = m_pobservable_websocket_server_info;
	//						break;
	//				};

	//				// check) same webpage?
	//				RETURN_IF(pmember->is_observable(pobservable_webpage, OCID_FRAME1), 1);

	//				// - unregister old observable
	//				pmember->unsubscribe(OCID_FRAME1);

	//				// - set..
	//				pmember->m_webpage_ptarget = pobservable_webpage;

	//				// - request 
	//				pmember->send(pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE, pobservable_webpage->m_webpage_str_target));
	//			}
	//			break;

	//	case	'B':
	//			{
	//				// declare) 
	//				auto pobservable_webpage = std::move(pmember->m_webpage_ptarget);

	//				// check)
	//				RETURN_IF(pobservable_webpage.empty(), 1);

	//				// 1) register member
	//				pobservable_webpage->subscribe(pmember, OCID_FRAME1);
	//			}
	//			break;
	//	};
	//}

	// return) 
	return eRESULT::DONE;
}

void client::web::group::notify_to_web_controls_SERVER_ENTER(server_entity::instance* /*_pserver_instance*/)
{
	//m_pobservable_websocket_server_info->notify_to_web_controls_SERVER_ENTER(_pserver_instance);
	//m_pobservable_websocket_host_info->notify_to_web_controls_SERVER_ENTER(_pserver_instance);
}
void client::web::group::notify_to_web_controls_SERVER_LEAVE(server_entity::instance* /*_pserver_instance*/)
{
	//m_pobservable_websocket_server_info->notify_to_web_controls_SERVER_LEAVE(_pserver_instance);
	//m_pobservable_websocket_host_info->notify_to_web_controls_SERVER_LEAVE(_pserver_instance);
}

void client::web::group::notify_to_web_controls_SERVER_STATUS_UPDATE(server_entity::instance* /*_pserver_instance*/)
{
	//m_pobservable_websocket_server_info->notify_to_web_controls_SERVER_STATUS_UPDATE(_pserver_instance);
	//m_pobservable_websocket_host_info->notify_to_web_controls_SERVER_STATUS_UPDATE(_pserver_instance);
}

void client::web::group::notify_to_web_controls_SERVER_STATUS_UPDATE()
{
	//m_pobservable_websocket_server_info->notify_to_web_controls_SERVER_STATUS_UPDATE();
	//m_pobservable_websocket_host_info->notify_to_web_controls_SERVER_STATUS_UPDATE();
}
