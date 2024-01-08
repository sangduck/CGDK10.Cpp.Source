#include "pch.h"


void socket_admin_server::on_initialize(context& /*_context*/)
{
	// log) 
	LOG_INFO << " - initialize SOCKET['"sv << name() << "']"sv;
}

void socket_admin_server::on_destroy()
{
	// log) 
	LOG_INFO << " - destroy SOCKET['"sv << name() << "']"sv;
}

void socket_admin_server::on_start(context& /*_context*/)
{
	this->enable_reconnect();
}

void socket_admin_server::on_stop()
{
	this->disable_reconnect();
}

void socket_admin_server::on_connect()
{
	server_admin::sCERTIFY_INFO certify_info;
	certify_info.instance_type = server_admin::eINSTANCE_TYPE::HOST;

	this->send(server_admin::message_SERVER_ADMIN_CERTIFY_REQUEST(certify_info));
}

void socket_admin_server::on_disconnect(uint64_t /*_disconnect_reason*/)
{
}

result_code socket_admin_server::on_message(sMESSAGE_NETWORK& _msg)
{
	try
	{
		// 1) set source
		_msg.set_source(this);

		// 2) dispatch message
		TRANSMIT_MESSAGE(_msg);

		// 3) execute message map
		BEGIN_NESTED_CGMESSAGE_MAP
			CGMESSAGE_TYPE_SUB(eMESSAGE::SYSTEM::NETWORK, ((sMESSAGE_BUFFER&)_msg).buf_message.front<uint32_t>(4))
				ON_CGMESSAGE(eMESSAGE::SERVER::ADMIN::CERTIFY_RESPONSE, on_message_SERVER_ADMIN_CERTIFY_RESPONSE, sMESSAGE_NETWORK);
			END_SUB_CGMESSAGE_MAP
		END_NESTED_CGMESSAGE_MAP
	}
	catch(...)
	{
	}

	// return)
	return eRESULT::BYPASS;
}

result_code socket_admin_server::on_message_SERVER_ADMIN_CERTIFY_RESPONSE(sMESSAGE_NETWORK& _msg)
{
	// 1) get admin socket
	auto psocket_admin_socket = _msg.get_source<server_admin::socket_to_admin>();

	// 1) 내용을 읽기 위해...
	shared_buffer buf_recv = _msg.buf_message + offset(8);

	// 2) 값을 설정한다.
	auto result = buf_recv.extract<eRESULT>();

	// 3) 결과에 따라 처리한다.
	switch (result)
	{
	case	eRESULT::SUCCESS:
			{
				// 1) send host info
				//scoped_lock(psocket_admin_socket->m_lockable_notify)
				//{
				//	//// declare) 
				//	//std::vector<server_admin::sINSTANCE_HOST> vector_host_instance;
				//	//std::vector<server_admin::sINSTANCE_SERVICE> vector_service_instance;

				//	//scoped_lock(service::pmanager_instance->m_cs_instance)
				//	//{
				//	//	for (auto& iter : service::pmanager_instance->m_map_instance)
				//	//	{
				//	//		vector_host_instance.push_back(iter.second->m_instance);
				//	//	}
				//	//}

				//	//// - 접속되자 마자 ServerInfo를 전송한다.
				//	//psocket_admin_socket->send(server_admin::message_SERVER_ADMIN_INSTANCE_ENTER(vector_host_instance, vector_service_instance));
				//}

				// 3) execute interval => 5sec
				psocket_admin_socket->execute_interval(5s);

				// 4) register to scheduler
				system_executor::register_schedulable(psocket_admin_socket);
			}
			break;

	default:
			{
				// log)
				LOG_ERROR << "(err ) fail to certify from ADMIN_SERVER ["sv << to_string<char>(result) << "("sv << static_cast<int>(result) << ")]"sv;
			}
			break;
	}

	// 4) enter to controlss

	// return)
	return eRESULT::DONE;
}
