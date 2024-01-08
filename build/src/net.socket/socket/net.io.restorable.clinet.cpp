#include "pch.h"

using namespace CGDK;


void net::io::Nrestorable_client::process_on_connect()
{
	this->on_socket_connect();
}

void net::io::Nrestorable_client::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE /*_socket_state*/)
{
	// 1)  process_on_suspend함수를 호출한다. (여기서 true가 돌아오면 suspend 되는 것이다.)
	auto result = this->process_on_suspend(_disconnect_reason);

	// 2) result가 false라면 바로 on_disconnect()함수를 호출한다.
	if (result == false)
	{
		// - Restore를 리셋한다.
		scoped_lock(m_list_sending)
		{
			this->m_ack_received = 0;
			this->m_ack_sended = 0;
			this->m_list_sending.clear();
		}

		// - disconnect한다.
		this->on_disconnect(_disconnect_reason);
	}

	// 3) 물리적 접속 종료
	this->on_socket_disconnect();
}

void net::io::Nrestorable_client::process_on_fail_connect(uint64_t _disconnect_reason)
{
	this->on_fail_connect(_disconnect_reason); 
}

void net::io::Nrestorable_client::process_on_restoring()
{
	this->on_restoring(); 
}

void net::io::Nrestorable_client::process_on_restore()
{
	// 1) 상태를 Established상태로 변경한다.
	this->m_flag_suspended = false;

	// 2) on_restore함수를 호출한다.
	this->on_restore();
}

bool net::io::Nrestorable_client::process_on_suspend(uint64_t _disconnect_reason)
{
	// check) Active close이면 무조건 강제 종료한다.
	RETURN_IF((_disconnect_reason & DISCONNECT_REASON_ACTIVE) != 0, false);

	// 1) 상태를 suspend상태로 변경한다.
	this->m_flag_suspended = true;

	// 2) suspend된 시간을 저장한다.
	this->m_tick_suspend = chrono::tick::now();

	// declare)
	bool result = true;

	try
	{
		// 2) on_suspend함수를 호출한다.
		this->on_suspend(_disconnect_reason);
	}
	catch(...)
	{
		result = false;
	}

	// return)
	return result;
}

void net::io::Nrestorable_client::process_request_io_restore(result_code /*_result*/, SEQUENCE_ID /*_ack_received*/)
{
	// 1) Statust를 설정한다.
	this->socket_state(eSOCKET_STATE::SYN);
}

void net::io::Nrestorable_client::process_response_io_restore(result_code _result, SEQUENCE_ID _ack_received)
{
	switch (_result.value<eRESULT>())
	{
	// case RESTORE] Restore일 경우에만 Restore 처리를 수행한다.
	case	eRESULT::RESTORE:
			{
				scoped_lock(this->m_list_sending)
				{
					// check)
					CGASSERT_ERROR(_ack_received != 0);

					// - _ack_received를 1줄인다.
					--_ack_received;

					// - Pop할 갯수를 계산한다.
					auto count = _ack_received - m_ack_sended;

					// check) Queuing된 갯수보다 많으면 잘못된 것이다.
					CGASSERT_ERROR(count <= this->m_list_sending.size());

					// - Pop할 갯수만큼 메시지를 제거한다.
					for (; count>0; --count)
					{
						this->m_list_sending.pop_front();
					}
					this->m_ack_sended = _ack_received;
				}

				// - process_on_restoring
				this->process_on_restoring();

				scoped_lock(this->m_list_sending)
				{
					for (auto& iter : this->m_list_sending)
					{
						// - RESTORE_CLIENT_ID를 변경된 값으로 교체한다.
						iter.front<RESTORE_CLIENT_ID>(sizeof(uint32_t) * 3 + sizeof(RESTORE_SERVER_ID)) = this->m_id_restore_client;

						// - 전송한다.
						this->send(iter);
					}
				}
			}
			break;

	// case RESTORE] 그 외의 경우는 그냥 reset처리한다.
	case	eRESULT::RESET:
	default:
			scoped_lock(this->m_list_sending)
			{
				this->m_ack_received = 1;
				this->m_ack_sended = 0;
				this->m_list_sending.clear();
			}
			break;
	}

	// 2) Restore message의 마지막을 전송한다. (ack_sended가 0이면 최초 접속이다.)
	this->send(message_RESTORE_COMPLETE(_result));
}

void  net::io::Nrestorable_client::process_complete_io_restore(result_code _result)
{
	switch (_result.value<eRESULT>())
	{
	case	eRESULT::RESTORE:
			this->process_on_restore();
			break;

	case	eRESULT::RESET:
			this->on_connect();
			break;

	default:
			this->on_fail_connect(0);
			this->disconnect();
			break;
	};
}
