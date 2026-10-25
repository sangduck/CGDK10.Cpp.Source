#include "pch.h"

#if defined(_WINSOCK2API_)
using namespace CGDK;

bool net::io::sendable::Ntcp_crypt_async_gather_buffered::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages, const net::sockaddr& /*_paddress*/)
{
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT_FATAL(_buffer.size() != 0);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.size() == 0)
		return false;

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT_ERROR(_buffer.data() != nullptr);

	// check) _buffer.data()가 NULL이 아닌가?
	if(_buffer.data() == nullptr)
		return false;

	// declare)
	CGSOCKET handle_socket;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) if socket is not connected, return false (after lock)
		if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
			return false;

		// 1) get socket native handle
		handle_socket = this->native_handle();

		// check) if invalid socket, return false 
		if(handle_socket == INVALID_SOCKET)
			return false;

		// check) queue overflowed? (while sending : it would be queued. before the encryption : the keystream is not used for a dropped send)
		if (this->m_flag_sending == true && ((this->m_queue_pending.array_buffer.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= this->m_bytes_max_queued)))
		{
			// statistics)
			this->statistics_on_error_send();

			// check)
			this->process_on_send_overflow();

			// return)
			return false;
		}

		// 2) encrypt (in the lock : the cryptor is a stream cipher, so the order of the encryption must be the order of the bytes on the wire.
		//    encrypted out of the lock, two senders could queue in the other order and the peer decrypts garbage)
		buffer_view buffer_encrypt;
		object_ptr<Ireferenceable> powner_encrypt;

		if (auto pcryptor = this->get_cryptor(); pcryptor.empty())
		{
			buffer_encrypt = _buffer;
			powner_encrypt = std::move(_powner);
		}
		else
		{
			auto buf_crypted = pcryptor->encrypt(_buffer);

			buffer_encrypt = buf_crypted;
			powner_encrypt = std::move(buf_crypted.get_source());
		}

		// 3) 전송 처리 중(m_flag_sending이 signal된 상태(true))라면 queuing 처리만 하고 끝낸다.
		if (this->m_flag_sending == true)
		{
			// - queuing to pending_queue
			this->m_queue_pending.push_back(buffer_encrypt, std::move(powner_encrypt), _count_messages);

			// return) 
			return true;
		}

		// 3) set flag_sending true
		this->m_flag_sending = true;

		// - handle은 lock 안에서 읽고 send까지 lock 안에서 한다.
		//   (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
		// 4) queuing to sending buffer
		{
			CGASSERT_ERROR(this->m_queue_sending.array_buffer.empty());
			CGASSERT_ERROR(this->m_queue_sending.array_node.empty());

			this->m_queue_sending.array_buffer.emplace_back(buffer_encrypt.size<ULONG>(), buffer_encrypt.data());
			this->m_queue_sending.array_node.emplace_back(std::move(powner_encrypt));
			this->m_queue_sending.bytes_data = buffer_encrypt.size();
			this->m_queue_sending.count_message = _count_messages;
		}

		// 5) prepare executable
		this->m_pexecutable_send_buffer_list->m_psendable = this;

		// 6) reference count를 증가시킨다.
		this->m_pexecutable_send_buffer_list->add_ref();

		// statistics) send overlapped 카운트를 하나 증가시킨다.
		this->statistics_on_increase_async_sending();

		// 7) send async
		auto result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), 1, nullptr, this->m_pexecutable_send_buffer_list.get());

		// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
		if (result == false && api::get_last_error() != WSA_IO_PENDING)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// statistics)
			this->statistics_on_error_send();

			// - send overlapped 카운트를 하나 줄인다.
			this->statistics_on_decrease_async_sending();

			// - reset 
			this->m_pexecutable_send_buffer_list->m_psendable.reset();

			// - release 'm_pexecutable_send_buffer_list'
			this->m_pexecutable_send_buffer_list->release();

			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return false;
		}

		// statistics) 
		this->statistics_on_send_async();

		// return)
		return true;
	}

	// return) (not reached)
	return false;
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t /*_count_messages*/)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message((_transferred_bytes != 0) ? this->m_queue_sending.count_message : 0);

	// statistics) the overlapped send completed (it was never decreased : statistics_get_now_async_io() of a socket that had sent stayed > 0)
	this->statistics_on_decrease_async_sending();

	// check) 이미 destroy 된 경우 더 이상 처리하지 않고 그냥 끝낸다. (destroy되어 clear 처리된 경우 bytes_data가 0일 것이다.)
	if(this->m_queue_sending.bytes_data == 0)
		return;

	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 1) clear seding queue
		this->m_queue_sending.clear();

		// - handle은 lock 안에서 읽고 send까지 lock 안에서 한다.
		//   (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다 : 닫힌 뒤 같은 값으로 다시 만들어진 다른 socket에 보내지 않도록)
		const auto handle_socket = this->native_handle();

		// check) 
		if (handle_socket == INVALID_SOCKET)
		{
			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// check) return if no pended data
		if (this->m_queue_pending.array_buffer.empty())
		{
			// - reset queued
			this->m_flag_sending = false;

			// - shutdown if send_disconnected
			if (socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(handle_socket, 0);

				// - send shutdown
				api::socket::shutdown(handle_socket, SD_SEND);
			}

			// return)
			return;
		}

		// - move 'm_queue_pending' -> 'm_queue_sending' and clear 'm_queue_pending'
		this->m_queue_sending.reset_and_swap(std::move(this->m_queue_pending));

		// - send (lock 안)
		// 2) prepare executable
		this->m_pexecutable_send_buffer_list->m_psendable = this;

		// 3) reference count를 증가시킨다.
		this->m_pexecutable_send_buffer_list->add_ref();

		// statistics) send overlapped 카운트를 하나 증가시킨다.
		this->statistics_on_increase_async_sending();

		// 4) send async
		auto result = api::socket::send_async(handle_socket, &this->m_queue_sending.array_buffer.front(), static_cast<uint32_t>(this->m_queue_sending.array_buffer.size()), nullptr, this->m_pexecutable_send_buffer_list.get());

		// check) result가 false이고 WOULD BLOCK이 아닐 때만 전송 실패이다.
		if (result == false && api::get_last_error() != WSA_IO_PENDING)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// statistics)
			this->statistics_on_error_send();

			// - send overlapped 카운트를 하나 줄인다.
			this->statistics_on_decrease_async_sending();

			// - reset 
			this->m_pexecutable_send_buffer_list->m_psendable.reset();

			// - release 'm_pexecutable_send_buffer_list'
			this->m_pexecutable_send_buffer_list->release();

			// - process reset sendable
			this->process_reset_sendable();

			// return) fail to send
			return;
		}

		// statistics) 
		this->statistics_on_send_async();
	}
}

std::size_t net::io::sendable::Ntcp_crypt_async_gather_buffered::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_close_sender()
{
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) 보내는 중이면 다 보낸 뒤(process_complete_sendable) shutdown한다.
		RETURN_IF(this->m_queue_sending.array_buffer.empty() == false);

		// - handle은 lock 안에서 읽는다. (close는 같은 lock 안에서 handle을 INVALID_SOCKET으로 바꾼다)
		const auto handle_socket = this->native_handle();

		// check)
		RETURN_IF(handle_socket == INVALID_SOCKET);

		// - linger Option을 Graceful disconnect로 설정한다.
		api::socket::set_linger_option(handle_socket, 0);

		// - 접속을 종료한다.
		api::socket::shutdown(handle_socket, SD_SEND);
	}
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - reset send buffer
		this->m_queue_pending.clear();

		// - reset sending buffer and flag (a WSASend that failed or a close while sending left m_flag_sending true : the socket reused
		//   for another connection queued every send and never sent again. its completion, if any, sees the cleared queue and returns)
		this->m_queue_sending.clear();
		this->m_flag_sending = false;
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());

	// 3) reset executable
	if (this->m_pexecutable_send_buffer_list.empty())
	{
		this->m_pexecutable_send_buffer_list = make_object<executable_send_buffer_list>();
	}
}

#endif
