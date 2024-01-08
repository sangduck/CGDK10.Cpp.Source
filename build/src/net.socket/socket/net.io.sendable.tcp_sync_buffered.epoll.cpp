#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;


void net::io::sendable::Ntcp_sync_buffered::QUEUE_SEND::clear() noexcept
{
	// 1) c;ear array node
	array_node.clear();
	bytes_data = 0;
}

bool net::io::sendable::Ntcp_sync_buffered::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_sync_buffered::process_sendable')"sv));

	// 1) get buffer size
	auto bytes_requested = _buffer.size();

	// declare)
	CGSOCKET handle_socket;

	scoped_lock(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

		// 2) get socket native handle
		handle_socket = this->native_handle();

		// check) if invalid socket, return false 
		RETURN_IF(handle_socket == INVALID_SOCKET, false);

		// check) m_flag_sending이 signal된 상태(true)라면 EPOLLOUT이 뜰 때까지 대기해야 한다.
		if (this->m_flag_sending == true)
		{
			// check) quque overflowed?
			if ((this->m_queue_pending.array_node.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= this->m_bytes_max_queued) )
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				THROW_IF(this->exception_on_send_overflow(), throwable::failure(eRESULT::OVERFLOW_ERROR, 0, "occure send overflow"sv));

				// return)
				return false;
			}

			// - BYTES_SEND_GATHER_BUFFER_MIN보다 작으면 복사를 한다.
			if (bytes_requested <= BYTES_SEND_GATHER_BUFFER_MIN)
			{
				// - is buffer enought to copy (create new)
				if (this->m_queue_pending.array_node.empty() || this->m_queue_pending.array_node.back().bytes_remained < bytes_requested)
				{
					// - get buffer size
					auto size_buffer = (this->m_queue_pending.array_node.empty()) ? BYTES_SEND_GATHER_BUFFER_MIN : BYTES_SEND_GATHER_BUFFER_MAX;

					// - ....
					auto buf_data = alloc_shared_buffer(size_buffer);

					// - queuing
					this->m_queue_pending.array_node.push_back(QUEUE_SEND::NODE{ buf_data, size_buffer, buf_data.get_source(), 0, 0 });
				}

				// - get end...
				QUEUE_SEND::NODE& data_node = this->m_queue_pending.array_node.back();

				// check)
				CGASSERT_ERROR(data_node.bytes_remained >= bytes_requested)

				// - copy~
				memcpy(data_node.buf_send.data() + data_node.buf_send.size(), _buffer.data(), bytes_requested);
				data_node.buf_send.add_size(bytes_requested);
				data_node.bytes_remained -= bytes_requested;
				data_node.bytes_data += bytes_requested;
				data_node.count_message += _count_message;
				this->m_queue_pending.bytes_data += _buffer.size();
			}
			// - BYTES_SEND_GATHER_BUFFER_MIN보다 크면 통채로 추가한다.
			else
			{
				// - ...
				this->m_queue_pending.array_node.push_back(QUEUE_SEND::NODE{ _buffer, 0, std::move(_powner), bytes_requested, _count_message });
				this->m_queue_pending.bytes_data += _buffer.size();
			}

			// return) 
			return true;
		}

		// check) EPOLLOUT이 이미 설정된 상태면 안됀다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// - set flag_sending
		this->m_flag_sending = true;
	}

	// check) EPOLLOUT이 이미 설정된 상태면 안됀다.
	CGASSERT_ERROR((this->m_epoll_event_flag& EPOLLOUT) == 0);

	// - temporary buffer
	auto buf_now = _buffer;

	// check) 
	CGASSERT_ERROR(buf_now.size() != 0);

	// - request send
	auto bytes_sended = ::send(handle_socket, buf_now.data(), buf_now.size(), MSG_DONTWAIT | MSG_NOSIGNAL);

	// case) send completed~
	if (bytes_sended == buf_now.size<int>())
	{
		// statistics) 
		this->statistics_on_send_bytes(bytes_requested);
		this->statistics_on_send_message(_count_message);

		// - shutdown if send_disconnected
		if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
		{
			// - set linger Option for graceful disconnect
			api::socket::set_linger_option(this->native_handle(), 0);

			// - send shutdown
			api::socket::shutdown(this->native_handle(), SD_SEND);
		}
	}
	// case) send partial - retry
	else if (bytes_sended >= 0)
	{
		// check)
		CGASSERT_ERROR(static_cast<std::size_t>(bytes_sended) < buf_now.size());

		// - 전송한 만큼 이동시킨다.
		buf_now += offset(bytes_sended);

		// - push front processed buffers
		scoped_lock(this->m_lockable_sending)
		{
			// - queuing to head
			this->m_queue_pending.array_node.push_front(QUEUE_SEND::NODE{ buf_now, 0, std::move(_powner), bytes_requested, _count_message });
		}
	}
	else
	{
		// - get last error
		auto error_code = api::get_last_error();

		// case) bytes_sended < 0 && error ==  EWOULDBLOCK (wait for EPOLLOUT)
		if (error_code == EWOULDBLOCK)
		{
			// - push front processed buffers
			scoped_lock(this->m_lockable_sending)
			{
				// - queuing to head
				this->m_queue_pending.array_node.push_front(QUEUE_SEND::NODE{ buf_now, 0, std::move(_powner), bytes_requested, _count_message });
				this->m_queue_pending.bytes_data += bytes_requested;
			}
		}
		// case) fail to send
		else
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics) 
			this->statistics_on_error_send();

			scoped_lock(this->m_lockable_sending)
			{
				this->m_flag_sending = false;
			}

			// return) fail
			return false;
		}
	}

	scoped_lock(this->m_lockable_sending)
	{
		// check)
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// statistics) 
		this->statistics_on_increase_async_sending();

		// - set EPOLLOUT event flag
		this->post_epoll_event_EPOLLOUT();
	}

	// return)
	return true;
}

uint32_t net::io::sendable::Ntcp_sync_buffered::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	// declare) 
	container_t<QUEUE_SEND::NODE> queue_send;

	// 1) get queued SENDING_INFO
	scoped_lock(this->m_lockable_sending)
	{
		// 설명) EPOLLOUT 이벤트는 중복 실행될 수 있으므로 적절한 처리가 필요하다.
		//  결론을 말하면 m_queue_pending.array_node.empty() 면 바로 리턴한다. m_flag_sending는 true일수도 있고 false일 수도 있다.
		// 
		//   EPOLL 이벤트가 중복되었을 때 증상을 정리하자면
		// 
		//    case 1) EPOLLOUT 이벤트의 처리상 문제로 실행도중 중복 진입한 경우.
		//            실행도중 다시 진입한 경우 m_flag_sending == true이고 m_queue_pending.array_node은 empty()가 된다.
		//             => 이럴 경우 그냥 return하면 된다.
		//
		//    case 2) EPOLLOUT 이벤트가 처리되고 난 다음 중복 진입한 경우.
		//            모두 처리하고 나간 상태이기 때문에 m_flag_sending == false가 되고 m_queue_pending.array_node는 empty() 상태가 된ㄷㄷ.
		//             => 이런 경욷도 그냥 return만 하면 된다.
		//
		//  중복 이벤트가 발생하더라도 절대로 발생해서는 안돼는 상황으르 설명하자면...
		//
		//    case 1) m_queue_pending.array_node가 empty()가 아닌데 m_flag_sending가 false인 상황
		//    case 1) m_queue_sending.array_node이 empty()가 아닌 상황. 
		//            m_queue_sending.array_node는 메모리 중복 할당을 피하기 위한 단순 버퍼의 역할만 한다.따라서 항상 empty상태여야 한다.
		//

		// check)
		RETURN_IF(this->m_queue_pending.array_node.empty(), 0);

		// check) 
		CGASSERT_ERROR(this->m_flag_sending == true);

		// check)
		CGASSERT_ERROR(this->m_queue_sending.array_node.empty() == true);

		// - swap data
		this->m_queue_sending.array_node.swap(this->m_queue_pending.array_node);
		this->m_queue_pending.bytes_data = 0;
		queue_send.swap(this->m_queue_sending.array_node);
	}

	// declare) 
	msghdr msg_send;

	// 2) prepare ...
	// - alloc iovec buffer
	shared_buffer buf_msg_iov = alloc_shared_buffer(sizeof(iovec) * queue_send.size());

	// - store ...
	std::size_t bytes_sending = 0;
	iovec* piter_dest = buf_msg_iov.data<iovec>();
	for (auto iter : queue_send)
	{
		piter_dest->iov_base = iter.buf_send.data();
		piter_dest->iov_len = iter.buf_send.size();
		bytes_sending += iter.buf_send.size();

		++piter_dest;
	}

	// - set buffer		
	msg_send.msg_name = nullptr;
	msg_send.msg_namelen = 0;
	msg_send.msg_iov = buf_msg_iov.data<iovec>();
	msg_send.msg_iovlen = queue_send.size();
	msg_send.msg_control = nullptr;
	msg_send.msg_controllen = 0;
	msg_send.msg_flags = 0;

	for (int i=0;;++i)
	{
		// check)
		CGASSERT_ERROR(bytes_sending != 0);

		// 3) send 한다.
		auto bytes_result = ::sendmsg(native_handle(), &msg_send, MSG_DONTWAIT | MSG_NOSIGNAL);

		// 4-1) [case] send completed~
		if (bytes_result == static_cast<int>(bytes_sending))
		{
			// - process sended buffer
			for (auto iter = queue_send.begin(), iter_end = queue_send.end(); iter != iter_end; ++iter, queue_send.pop_front())
			{
				// statistics) 
				this->statistics_on_send_bytes(iter->bytes_data);
				this->statistics_on_send_message(iter->count_message);
			}

			// - clear and swap buffer
			queue_send.clear();
			queue_send.swap(this->m_queue_sending.array_node);

			// - shutdown if send_disconnected
			if (socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(this->native_handle(), 0);

				// - send shutdown
				api::socket::shutdown(this->native_handle(), SD_SEND);
			}

			// break) 
			break;
		}
		// // 4-2) [case] it send partial (bytes_result > 0)
		else if (bytes_result > 0)
		{
			// check) 
			CGASSERT_ERROR(static_cast<int>(bytes_sending) > bytes_result);

			// - recalculate byte sending
			bytes_sending -= bytes_result;

			// check)
			CGASSERT_ERROR(bytes_sending != 0);

			// - process sended buffer
			for (auto iter = queue_send.begin(), iter_end = queue_send.end(); iter != iter_end; ++iter, queue_send.pop_front(), ++msg_send.msg_iov, --msg_send.msg_iovlen)
			{
				auto bytes_iter = iter->buf_send.size<int>();

				if (bytes_iter > bytes_result)
				{
					iter->buf_send += offset(bytes_result);
					msg_send.msg_iov->iov_base = iter->buf_send.data();
					msg_send.msg_iov->iov_len = iter->buf_send.size();
					break;
				}

				bytes_result -= bytes_iter;

				// statistics) 
				this->statistics_on_send_bytes(iter->bytes_data);
				this->statistics_on_send_message(iter->count_message);
			}

			// check) 
			CGASSERT_ERROR(queue_send.empty() == false);
			CGASSERT_ERROR(msg_send.msg_iovlen != 0);

			// continue) 
			continue;
		}
		else
		{
			// - get last error
			auto error_code = api::get_last_error();

			// 4-3) [case] it send not at all but not failure, need waiting for EPOLLOUT (EWOULDBLOCK)
			if (error_code == EWOULDBLOCK)
			{
				// - move remained to m_queue_pending.array_node
				scoped_lock(this->m_lockable_sending)
				{
					this->m_queue_pending.array_node.splice(this->m_queue_pending.array_node.begin(), queue_send);
					this->m_queue_pending.bytes_data += bytes_sending;
				}

				// - swap
				this->m_queue_sending.array_node.swap(queue_send);

				// break)
				break;
			}
			// 4-4) [case] failure
			else
			{
				// - set last error
				this->m_last_socket_error = error_code;

				// statistics) 
				this->statistics_on_error_send();

				// - closesocket (abortive close)
				this->closesocket();

				// - clear send queue
				scoped_lock(this->m_lockable_sending)
				{
					_process_clear_send_buffers();
				}

				// break)
				break;;
			}
		}
	}

	// declare) 
	bool flag_sending;

	// 5) set event flag
	scoped_lock(this->m_lockable_sending)
	{
		// check) EPOLLOUT이 이미 설정된 상태면 안됀다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// - set 
		flag_sending = !this->m_queue_pending.array_node.empty();

		// - set flag
		this->m_flag_sending = flag_sending;
	}

	// - post epoll event
	if (flag_sending == false)
	{
		// statistics) 
		this->statistics_on_decrease_async_sending();

		// return)
		return 0;
	}
	else
	{
		return EPOLLOUT;
	}
}

void net::io::sendable::Ntcp_sync_buffered::_process_clear_send_buffers()
{
	for ([[maybe_unused]] auto& iter : this->m_queue_pending.array_node)
	{
		this->process_complete_sendable(1, 0, 0);
	}

	this->m_queue_pending.clear();
	this->m_queue_sending.clear();
}

void net::io::sendable::Ntcp_sync_buffered::process_complete_sendable( uintptr_t /*_result*/, std::size_t /*_transferred_bytes*/, std::size_t /*_transferred_message*/)
{
}

void net::io::sendable::Ntcp_sync_buffered::process_close_sender()
{
	// check)
	bool do_shutdown;

	scoped_lock(this->m_lockable_sending)
	{
		do_shutdown = !this->m_flag_sending;
	}

	if (do_shutdown)
	{
		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(this->native_handle(), 0);

		// - 접속을 종료한다.
		api::socket::shutdown(this->native_handle(), SD_SEND);
	}
}

void net::io::sendable::Ntcp_sync_buffered::process_reset_sendable() noexcept
{
	// 1) prepare & reset sending status
	scoped_lock(this->m_lockable_sending)
	{
		// - prepare and reset send buffer
		this->_process_clear_send_buffers();

		// - reset flag 'sending'
		this->m_flag_sending = false;
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
}

#endif