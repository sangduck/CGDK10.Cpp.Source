#include "pch.h"

using namespace CGDK;


void net::io::Irestore_server::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE /*_socket_state*/)
{
	// 1) process_on_suspend함수를 호출한다. (여기서 true가 돌아오면 suspend 되는 것이다.)
	this->process_on_suspend(_disconnect_reason);

	// 2) call 'on_disconnect'
	this->on_disconnect(_disconnect_reason);
}

bool net::io::Irestore_server::process_on_suspend(uint64_t _disconnect_reason)
{
	// check) restore_manager가 없으면 false를 리턴한다.
	RETURN_IF(this->m_prestore_manager.empty(), false);

	// 1) restore manager에 suspend를 요청한다.
	return this->m_prestore_manager->suspend_restorable(this, _disconnect_reason);
}

result_code net::io::Irestore_server::process_message(sMESSAGE& _msg)
{
	// declare) 
	result_code	result = eRESULT::BYPASS;

	if (is_network_message(_msg.message))
	{
		// 1) casting
		sMESSAGE_NETWORK& msg_network = (sMESSAGE_NETWORK&)_msg;

		// 2) eMESSAGE::NETWORK::SOCKET::RESTORE일 경우 그에 대한 처리를 수행한다.
		//      Client측에서 RequestIORestre()을 요청하면 Server에 이 메시지를 전달된다.
		//      Server 측은 다음과 같은 처리를 한다.
		//      - [restore_server_id_t]를 읽어 이 Server에 대한 요청이 맞는가를 확인한다.
		//      - [restore_server_id_t]를 읽어 이에 해당하는 Restorable을 회복한다.
		//         해당 Restorable이 존재하지 않을 경우 신규 생성한다.
		//      - ackReceived와 ack_sent를 읽어 회복[Restore]가능한지 여부를 확인한다.
		//      - 회복[Restore]가 가능하다면 필요한 정보를 재송신한다.
		//      - 필요한 정보의 재송신이 끝나면 Client측에서 필요한 정보를 재송신을 대기한다.
		if(msg_network.buf_message.front<uint32_t>(4) == eMESSAGE::NETWORK::SOCKET::REQUEST_IO_RESTORE)
		{
			// check)
			CGASSERT_ERROR(this->m_prestorable.empty());

			// - restore_manager가 nullptr이면 기본 restore_manager를 사용한다.
			if(this->m_prestore_manager.empty())
			{
				this->m_prestore_manager = restore_manager::instance();
			}

			// - prestorable_server을 얻는다. (여기서 새로운 [restore_client_io_t]를 발급받는다.)
			this->m_prestore_manager->restore_restorable(this, msg_network.buf_message);

			// return)
			return eRESULT::BYPASS;
		}

		// 3) m_prestorable에 메시지 전달
		if(this->m_prestorable.exist())
		{
			auto result_nested = this->m_prestorable->process_message(msg_network);

			// check) result가 eRESULT::NONE이 아니면 여기서 바로 return한다.
			RETURN_IF(result_nested != eRESULT::NONE, result_nested);
		}

		// 4) call 'on_message'
		result = this->on_message(msg_network);
	}

	// return)
	return result;
}
