#include "pch.h"

namespace CGDK
{

void net::socket::web_server::on_connect(net::io::Iconnective* _pconnective)
{
	// 1) get entitable
	auto pentitable = dynamic_cast<server::web::Ientitable*>(_pconnective);

	// 2) set entitiable
	if(pentitable != nullptr)
	{
		m_pweb_entity = pentitable->get_web_entity();
	}
}

void net::socket::web_server::on_disconnect(uint64_t /*_disconnect_reason*/)
{
	m_pweb_entity.reset();
}

result_code net::socket::web_server::on_message(sMESSAGE_HTTP_REQUEST& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// check) 
	THROW_IF(m_pweb_entity.empty(), throwable::failure(eRESULT::NOT_READY));

	// 3) get web entity
	auto pentity = get_web_entity();
	const auto& webserver_context = get_webserver_context();

	// check) 
	THROW_IF(pentity.empty(), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 4) execute request
	pentity->request(_msg, webserver_context); // ¿©±â

	// return) ...
	return eRESULT::BYPASS;
}

result_code net::socket::web_server::on_message(sMESSAGE_HTTP_WEBSOCKET& _msg)
{
	// 1) set source
	_msg.set_source(this);

	// 2) dispatch message
	TRANSMIT_MESSAGE(_msg);

	// return) 
	return eRESULT::BYPASS;
}


}