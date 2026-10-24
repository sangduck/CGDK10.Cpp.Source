#include "pch.h"

using namespace CGDK;


bool net::io::Irestorable_server::process_send( shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option)
{
	// 1) RestoreServer를 복사한다.
	auto prestore_server = this->m_prestore_server;

	// check) RestoreServer가 nullptr이면 false를 리턴한다.
	RETURN_IF(prestore_server.empty(), false)

	// 2) send한다.
	return prestore_server->send({ std::move(_buffer), _count_message }, _option);
}

bool net::io::Irestorable_server::disconnect(uint64_t _disconnect_reason) noexcept
{
	// 1) set socket send_disconnect
	auto result = this->set_socket_state_if(eSOCKET_STATE::ESTABLISHED, eSOCKET_STATE::SEND_DISCONNECTED);

	// check) 
	RETURN_IF(result == false, false);

	// check)
	CGASSERT_ERROR((_disconnect_reason & DISCONNECT_REASON_MASK) == 0);

	// 2) add DISCONNECT_ACTIVE flag to disconnect_reason 
	_disconnect_reason &= ~DISCONNECT_REASON_MASK;
	_disconnect_reason |= DISCONNECT_REASON_ACTIVE;

	// 3) add disconnect reason flag
	this->m_disconnect_reason |= _disconnect_reason;

	// 4) get sender_closer interface
	auto psender_closer = dynamic_cast<net::io::Isender_closser*>(this);

	// 5) close
	if(psender_closer != nullptr)
	{
		psender_closer->process_close_sender();
	}
	else
	{
		// - set linger option for graceful close
		api::socket::set_linger_option(this->native_handle(), 0);

		// - shutdown socket
		return api::socket::shutdown(this->native_handle(), SD_SEND);
	}

	// return) 
	return true;
}

void net::io::Irestorable_server::process_reset_connectable() noexcept
{
	this->m_id_restore_server.reset();
	this->m_id_restore_client.reset();

	this->process_reset_io_restorable();
	this->m_prestore_server.reset();
}

void net::io::Irestorable_server::process_create_socket(ADDRESS_FAMILY /*_address_family*/)
{
}

void net::io::Irestorable_server::process_prepare_socket(ADDRESS_FAMILY /*_address_family*/)
{
}

void net::io::Irestorable_server::process_dispose_socket() noexcept
{
}

void net::io::Irestorable_server::process_request_connect (const net::sockaddr& _address)
{
	this->on_request_connect(_address);
}

bool net::io::Irestorable_server::process_complete_connect (uintptr_t _result)
{
	// 1) _result이 0이 아닐 경우 Fail이므로 on_fail_connect를 호출한다.
	if (_result != 0)
	{
		// - socket Status를 disconnect로
		this->socket_state(eSOCKET_STATE::CLOSED);

		// - on_fail_connect()함수를 호출한다.
		try
		{
			this->process_on_fail_connect(this->m_disconnect_reason | DISCONNECT_REASON_FAIL);	// - the reason of the close (ex. DISCONNECT_REASON_SUSTAIN_TIMEOUT : connect timeout)
		}
		catch (std::exception& e)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}
		catch (...)
		{
			// trace) prepare receive에 실패했다!
			LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

			// declare)
			throwable::failure	e(eRESULT::EXCEPTION);

			// - call 'exception handler'
			throwable::execute_exception_handler(this, e, __FUNCTION__);
		}

		// return) 실패!
		return false;
	}

	// 2) 접속된 socket의 상태를 CONNECTED된 것으로 설정한다.
	this->set_socket_state_if(eSOCKET_STATE::SYN, eSOCKET_STATE::ESTABLISHED);

	// declare)
	bool result = true;

	try
	{
		this->process_on_connect();
	}
	// exception) 
	catch (std::exception& e)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// E1) process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) 접속해제과정을 진행한다.
		if (this->m_pconnectable_notify != nullptr)
		{
			this->m_pconnectable_notify->process_connective_disconnect(this);
		}

		// - Result를 false로...
		result = false;
	}
	catch (...)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// E1) process_disconnect한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// E2) 접속해제과정을 진행한다.
		if (this->m_pconnectable_notify != nullptr)
		{
			this->m_pconnectable_notify->process_connective_disconnect(this);
		}

		// - Result를 false로...
		result = false;
	}

	// return) 
	return	result;
}

bool net::io::Irestorable_server::process_complete_disconnect()
{
	// 1) Closed상태로 socket State를 바꾸고 이전 State를 얻어온다.
	eSOCKET_STATE socket_state_old = socket_state(eSOCKET_STATE::CLOSED);

	// check) socket이 이미 Clozed상태라면 돌려보낸다.
	RETURN_IF(socket_state_old == eSOCKET_STATE::CLOSED, false);

	try
	{
		this->process_on_disconnect(this->m_disconnect_reason, socket_state_old);
	}
	catch (std::exception& e)
	{
		// trace) prepare receive에 실패했다!
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}
	catch (...)
	{
		// trace) prepare receive에 실패했다!
		LOG_DEBUG << "(excp) CGDK.net.socket: exception complete " __CGFUNCTION__;

		// declare)
		throwable::failure e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);
	}

	// - 접속 종료 처리한다.
	this->process_close_socket();

	// return) 
	return true;
}

bool net::io::Irestorable_server::process_close_socket(uint64_t _disconnect_reason) noexcept
{
	// 1) store restore_server
	auto prestore_server = this->m_prestore_server;

	// check) 
	RETURN_IF(prestore_server.empty(), false);

	// 2) set 'disconnect_reason'
	this->m_disconnect_reason |= _disconnect_reason;

	// 3) call 'process_close_socket'
	return prestore_server->process_close_socket(_disconnect_reason);
}

void net::io::Irestorable_server::process_request_io_restore(result_code _result, sequence_id_t _ack_received)
{
	// 1) set socket state
	this->socket_state(eSOCKET_STATE::SYN);

	switch (_result.value<eRESULT>())
	{
	// case RESTORE] Restore일 경우에만 Restore 처리를 수행한다.
	case	eRESULT::RESTORE:
			{
				SCOPED_LOCK(this->m_lockable_list_sending)
				{
					// check)
					CGASSERT_ERROR(_ack_received != 0);

					// - _ack_received를 1줄인다.
					--_ack_received;

					// - Pop할 개수를 계산한다.
					auto count = _ack_received - this->m_ack_sent;

					// check) Queuing된 개수보다 많으면 잘못된 것이다.
					CGASSERT_ERROR(count <= this->m_list_sending.size());

					// - Pop할 개수만큼 메시지를 제거한다.
					for (; count>0; --count)
					{
						this->m_list_sending.pop_front();
					}
					this->m_ack_sent = _ack_received;
				}

				// - 먼저 IORESTORE의 Response message를 전송한다.
				this->send(this->message_RESPONSE_IO_RESTORE(_result));

				// - process_on_restoring
				this->process_on_restoring();

				SCOPED_LOCK(this->m_lockable_list_sending)
				{
					for (auto& iter : this->m_list_sending)
					{
						// - restore_client_io_t를 변경된 값으로 교체한다.
						iter.front<restore_client_io_t>(sizeof(uint32_t) * 3 + sizeof(restore_server_id_t)) = this->m_id_restore_client;

						// - 전송한다.
						this->send(iter);
					}
				}
			}
			break;

	// case RESTORE] 그 외의 경우는 그냥 reset처리한다.
	case	eRESULT::RESET:
	default:
			{
				SCOPED_LOCK(this->m_lockable_list_sending)
				{
					this->m_ack_received = 1;
					this->m_ack_sent = 0;
					this->m_list_sending.clear();
				}

				// - 먼저 IORESTORE의 Response message를 전송한다.
				this->send(this->message_RESPONSE_IO_RESTORE(_result));
			}
			break;
	}

	// 3) Restore message의 마지막을 전송한다. (ack_sent가 0이면 최초 접속이다.)
	this->send(message_RESTORE_COMPLETE(_result));
}

void net::io::Irestorable_server::process_response_io_restore(result_code /*_result*/, sequence_id_t /*_ack_received*/)
{
	// check)
	CGASSERT_ERROR(false);

	// check) 
	RETURN_IF(this->m_prestore_server.empty());
}

void  net::io::Irestorable_server::process_complete_io_restore(result_code _result)
{
	// check)
	RETURN_IF(this->m_prestore_server.empty());

	switch (_result.value<eRESULT>())
	{
	case	eRESULT::RESTORE:
			this->process_on_restore();
			break;

	case	eRESULT::RESET:
			this->process_complete_connect(0);
			break;

	default:
			this->process_complete_connect(1);	// on_fail_connect
			break;
	};
}

void net::io::Irestorable_server::process_on_connect()
{
	this->on_connect();
}

void net::io::Irestorable_server::process_on_disconnect(uint64_t _disconnect_reason, eSOCKET_STATE /*_socket_state*/)
{
	this->on_disconnect(_disconnect_reason); 
}

void net::io::Irestorable_server::process_on_fail_connect(uint64_t _disconnect_reason)
{
	this->on_fail_connect(_disconnect_reason);
}

void net::io::Irestorable_server::process_on_restoring()
{
	this->on_restoring();
}

void net::io::Irestorable_server::process_on_restore()
{
	// 1) 'suspended' flag => false
	this->m_flag_suspended = false;

	// 2) call 'on_restore'
	this->on_restore(); 
}

void net::io::Irestorable_server::process_on_suspend(uint64_t _disconnect_reason)
{
	// 1) 'suspended' flag => true
	this->m_flag_suspended = true;

	// 2) store suspended time
	this->m_tick_suspend = chrono::tick::now();

	// 3) call 'on_suspend'
	this->on_suspend(_disconnect_reason);
}
