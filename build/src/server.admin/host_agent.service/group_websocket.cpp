#include "pch.h"


group_websocket::group_websocket()
{
	this->m_pobservable_websocket_system_info = make_own<observable_websocket_system_info>();
	this->m_pobservable_websocket_system_info->name("observable_websocket_system_info");
	this->attach_child_object(this->m_pobservable_websocket_system_info);

	this->m_pobservable_websocket_host_info = make_own<observable_websocket_host_info>();
	this->m_pobservable_websocket_host_info->name("observable_websocket_host_info");
	this->attach_child_object(this->m_pobservable_websocket_host_info);

	this->m_pobservable_websocket_peer_info = make_own<observable_websocket_peer_info>();
	this->m_pobservable_websocket_peer_info->name("observable_websocket_peer_info");
	this->attach_child_object(this->m_pobservable_websocket_peer_info);
}

group_websocket::~group_websocket()
{
}

void group_websocket::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize GROUP['"sv << name() << "']"sv;

	// 1) Register MediatorÇÑ´Ù.
	this->register_messageable(::service::instance());
}

void group_websocket::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy GROUP['"sv << name() << "']"sv;

	// 1) register mediator
	this->unregister_messageable(::service::instance());
}

void group_websocket::on_start(context& /*_context*/)
{
	// 1) enable entering
	this->enable_member_enter();
}

void group_websocket::on_stop()
{
	// 1) disable entering
	this->disable_member_enter();
}

result_code group_websocket::on_member_entering(socket_websocket* /*_pmember*/, sMESSAGE& /*_param*/)
{
	// return)
	return eRESULT::SUCCESS;
}

void group_websocket::on_member_entered(socket_websocket* _pmember, sMESSAGE& /*_param*/)
{
	// 1) register to observable
	this->m_pobservable_websocket_system_info->subscribe(_pmember, OCID_SYSTEM);

	// 2) register _pmenber
	_pmember->register_messageable(this);

	// 3) set initial page
	_pmember->m_webpage_ptarget = this->m_pobservable_websocket_host_info;
	_pmember->send(_pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE, this->m_pobservable_websocket_host_info->m_webpage_str_target));
}

uintptr_t group_websocket::on_member_leaving(socket_websocket* _pmember, uintptr_t /*_param*/)
{
	// 1) unregister to observable
	_pmember->unsubscribe(OCID_SYSTEM);

	// 2) unregister _pmenber
	_pmember->unregister_messageable(this);

	// return) 
	return 0;
}

void group_websocket::on_member_leaved(socket_websocket* /*_pmember*/, uintptr_t /*_result*/)
{
}

result_code group_websocket::on_message(sMESSAGE_NETWORK& _msg)
{
	// 1) websocket message 
	sMESSAGE_HTTP_WEBSOCKET& msg = static_cast<sMESSAGE_HTTP_WEBSOCKET&>(_msg);

	// 2) get message head
	auto message_type = msg.buf_message.front_text<char>(size(4));
	auto pmember = msg.get_source<socket_websocket>();

	// 3) 
	if (message_type[0] == 'A')
	{
		switch(message_type[1])
		{
		case 'A':
				{
					// declare) 
					object_ptr<Iobservable_websocket> pobservable_webpage;

					switch (message_type[2])
					{
					case	'A':
							pobservable_webpage = this->m_pobservable_websocket_host_info;
							break;
					case	'B':
							pobservable_webpage = this->m_pobservable_websocket_peer_info;
							break;
					};

					// check) same webpage?
					RETURN_IF(pmember->is_observable(pobservable_webpage, OCID_FRAME1), 1);

					// - unregister old observable
					pmember->unsubscribe(OCID_FRAME1);

					// - set..
					pmember->m_webpage_ptarget = pobservable_webpage;

					// - request 
					pmember->send(pmember->make_webmessage_set_main_page(MESSAGE_WEB_SET_FRAME_PAGE, pobservable_webpage->m_webpage_str_target));
				}
				break;

		case	'B':
				{
					// declare) 
					auto pobservable_webpage = std::move(pmember->m_webpage_ptarget);

					// check)
					RETURN_IF(pobservable_webpage.empty(), 1);

					// 1) register member
					pobservable_webpage->subscribe(pmember, OCID_FRAME1);
				}
				break;
		};
	}

	// return) 
	return eRESULT::BYPASS;
}

void group_websocket::notify_to_web_controls_INSTANCE_ENTER(server_system::Iinstance* _pinstance)
{
	this->m_pobservable_websocket_peer_info->notify_to_web_controls_INSTANCE_ENTER(_pinstance);
	this->m_pobservable_websocket_host_info->notify_to_web_controls_INSTANCE_ENTER(_pinstance);
}
void group_websocket::notify_to_web_controls_INSTANCE_LEAVE(server_system::Iinstance* _pinstance)
{
	this->m_pobservable_websocket_peer_info->notify_to_web_controls_INSTANCE_LEAVE(_pinstance);
	this->m_pobservable_websocket_host_info->notify_to_web_controls_INSTANCE_LEAVE(_pinstance);
}

void group_websocket::notify_to_web_controls_INSTANCE_STATUS_UPDATE(server_system::Iinstance* _pinstance)
{
	this->m_pobservable_websocket_peer_info->notify_to_web_controls_INSTANCE_STATUS_UPDATE(_pinstance);
	this->m_pobservable_websocket_host_info->notify_to_web_controls_INSTANCE_STATUS_UPDATE(_pinstance);
}

void group_websocket::notify_to_web_controls_INSTANCE_STATUS_UPDATE()
{
	this->m_pobservable_websocket_peer_info->notify_to_web_controls_INSTANCE_STATUS_UPDATE();
	this->m_pobservable_websocket_host_info->notify_to_web_controls_INSTANCE_STATUS_UPDATE();
}
