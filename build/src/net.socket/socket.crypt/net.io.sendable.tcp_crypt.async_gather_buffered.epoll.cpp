#include "pch.h"

#if defined(_SYS_EPOLL_H)

#include <climits>

// - sendmsg의 최대 buffer 수
#if defined(IOV_MAX)
	static constexpr std::size_t CGDK_SEND_MAX_IOV = IOV_MAX;
#else
	static constexpr std::size_t CGDK_SEND_MAX_IOV = 1024;
#endif

using namespace CGDK;


void net::io::sendable::Ntcp_crypt_async_gather_buffered::QUEUE_SEND::clear() noexcept
{
	// 1) c;ear array node
	this->array_node.clear();
	this->bytes_data = 0;
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::QUEUE_SEND::push_back(const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	// declare)
	auto bytes_requested = _buffer.size();

	// - BYTES_SEND_GATHER_BUFFER_MIN보다 작으면 복사를 한다.
	if (this->array_node.size() > COUNT_SEND_GATHER_START_DEPTH && bytes_requested <= BYTES_SEND_GATHER_BUFFER_MIN)
	{
		// check) 
		CGASSERT_ERROR(this->array_node.empty() == false);

		// - is buffer enough to copy (create new)
		if (this->array_node.back().bytes_remained < bytes_requested)
		{
			// - shift count
			auto shift_count = static_cast<uint32_t>(this->array_node.size() - COUNT_SEND_GATHER_START_DEPTH - 1) * 2;

			// - saturate_max
			SATURATE_MAX(shift_count, BYTES_SEND_GATHER_SHIFT_MAX);

			// - get buffer size
			auto size_buffer = (BYTES_SEND_GATHER_BUFFER_MIN << shift_count);

			// check)
			CGASSERT_ERROR(size_buffer <= BYTES_SEND_GATHER_BUFFER_MAX);

			// - alloc new buffer
			auto buf_new = alloc_shared_buffer(size_buffer);

			// - queue allocated buffer
			this->array_node.push_back(NODE{ buf_new, buf_new.get_source(), 0, 0, size_buffer });
		}

		// - get end...
		auto& data_node = this->array_node.back();

		// check)
		CGASSERT_ERROR(data_node.bytes_remained >= bytes_requested);

		// - copy~
		memcpy(data_node.buf_send.data() + data_node.buf_send.size(), _buffer.data(), bytes_requested);
		data_node.buf_send.add_size(bytes_requested);
		data_node.bytes_data += bytes_requested;
		data_node.count_message += _count_messages;
		data_node.bytes_remained -= bytes_requested;
		this->bytes_data += bytes_requested;
	}
	// - BYTES_SEND_GATHER_BUFFER_MIN보다 크면 버퍼 통째로 추가한다.
	else
	{
		// - check 
		if (this->array_node.empty() == false)
		{
			// - get back
			auto& data_back = this->array_node.back();

			if (data_back.powner == _powner && data_back.buf_send.get_back_ptr() == _buffer.data())
			{
				// - expand only
				data_back.buf_send.add_size(bytes_requested);
				data_back.bytes_data += bytes_requested;
				data_back.count_message += _count_messages;

				// return) 
				return;
			}
		}

		// - ...
		this->array_node.push_back(NODE{ _buffer, std::move(_powner), bytes_requested, _count_messages, 0 });
		this->bytes_data += bytes_requested;
	}
}

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
	buffer_view buffer_encrypt;
	object_ptr<Ireferenceable> powner_encrypt;

	// 1) send cryptor를 얻는다.
	object_ptr<Icryptor> pcryptor = this->get_cryptor();

	// 2) set buffer
	if (pcryptor.empty())
	{
		buffer_encrypt = _buffer;
		powner_encrypt = std::move(_powner);
	}
	else
	{
		// - 암호화한다.
		auto buf_crypted = pcryptor->encrypt(_buffer);

		buffer_encrypt = buf_crypted;
		powner_encrypt = std::move(std::move(buf_crypted.get_source()));
	}

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

		// check) m_flag_sending이 signal된 상태(true)라면 EPOLLOUT이 뜰 때까지 대기해야 한다.
		if (this->m_flag_sending == true)
		{
			// check) queue overflowed?
			if ((this->m_queue_pending.array_node.size() >= this->m_max_depth_of_send_buffer) || (this->m_queue_pending.bytes_data >= m_bytes_max_queued) )
			{
				// statistics)
				this->statistics_on_error_send();

				// check)
				this->process_on_send_overflow();

				// return)
				return false;
			}

			// - queuing to pending_queue
			this->m_queue_pending.push_back(buffer_encrypt, std::move(powner_encrypt), _count_messages);
			this->m_queue_pending.bytes_data += buffer_encrypt.size();

			// return) 
			return true;
		}

		// - set flag_sending
		this->m_flag_sending = true;
	}

	// return)
	return this->process_sendable_async(handle_socket, buffer_encrypt, std::move(powner_encrypt), _count_messages);
}

bool net::io::sendable::Ntcp_crypt_async_gather_buffered::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_messages)
{
	// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
	CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

	// - temporary buffer
	auto buf_now = _buffer;

	// declare)
	auto bytes_requested = buf_now.size();

	// check) 
	CGASSERT_ERROR(buf_now.size() != 0);

	for(;;)
	{
		// - request send
		auto bytes_sent = ::send(_handle_socket, buf_now.data(), buf_now.size(), MSG_DONTWAIT | MSG_NOSIGNAL);

		// case) send completed~
		if (bytes_sent == buf_now.size<int>())
		{
			// - call 'process_complete_sendable'
			this->process_complete_sendable(0, bytes_requested, _count_messages);

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
			{
				// - set linger Option for graceful disconnect
				api::socket::set_linger_option(this->native_handle(), 0);

				// - send shutdown
				api::socket::shutdown(this->native_handle(), SD_SEND);
			}

			//break) 
			break;
		}
		// case) send partial - retry
		else if (bytes_sent >= 0)
		{
			// check)
			CGASSERT_ERROR(static_cast<std::size_t>(bytes_sent) < buf_now.size());

			// - 전송한 만큼 이동시킨다.
			buf_now += offset(bytes_sent);

			// - retry
			continue;
		}
		else
		{
			// - get last error
			auto error_code = api::get_last_error();

			// case) bytes_sent < 0 && error ==  EWOULDBLOCK (wait for EPOLLOUT)
			if (error_code == EWOULDBLOCK)
			{
				// - push front processed buffers
				SCOPED_LOCK(this->m_lockable_sending)
				{
					// - queuing to head
					this->m_queue_pending.array_node.push_front(QUEUE_SEND::NODE{ buf_now, std::move(_powner), bytes_requested, _count_messages, 0 });
					this->m_queue_pending.bytes_data += bytes_requested;
				}

				// break)
				break;
			}
			// case) fail to send
			else
			{
				// - set last error
				this->m_last_socket_error = error_code;

				// -get last error
				this->m_last_socket_error = api::get_last_error();

				// statistics) 
				this->statistics_on_error_send();

				// - closesocket
				this->process_close_socket(DISCONNECT_REASON_FAIL);

				// return) 전송에 실패했다.
				return false;
			}
		}
	}

	SCOPED_LOCK(m_lockable_sending)
	{
		// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
		CGASSERT_ERROR((this->m_epoll_event_flag & EPOLLOUT) == 0);

		// statistics) 
		this->statistics_on_increase_async_sending();

		// - set EPOLLOUT event flag
		this->post_epoll_event_EPOLLOUT();
	}

	// return)
	return true;
}

uint32_t net::io::sendable::Ntcp_crypt_async_gather_buffered::process_recover_sendable( intptr_t /*_event*/, std::size_t /*_param*/)
{
	// declare) 
	container_t<QUEUE_SEND::NODE> queue_send;
	
	// 1) get queued SENDING_INFO
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// 
		// 설명) EPOLLOUT 이벤트는 중복 실행될 수 있으므로 적절한 처리가 필요하다.
		//  결론을 말하면 m_queue_pending.empty() 면 바로 리턴한다. m_flag_sending는 true일수도 있고 false일 수도 있다.
		// 
		//   EPOLL 이벤트가 중복되었을 때 증상을 정리하자면
		// 
		//    case 1) EPOLLOUT 이벤트의 처리상 문제로 실행도중 중복 진입한 경우.
		//            실행도중 다시 진입한 경우 m_flag_sending == true이고 m_queue_pending은 empty()가 된다.
		//             => 이럴 경우 그냥 return하면 된다.
		//
		//    case 2) EPOLLOUT 이벤트가 처리되고 난 다음 중복 진입한 경우.
		//            모두 처리하고 나간 상태이기 때문에 m_flag_sending == false가 되고 m_queue_pending는 empty() 상태가 된다.
		//             => 이런 경욷도 그냥 return만 하면 된다.
		//
		//  중복 이벤트가 발생하더라도 절대로 발생해서는 안 되는 상황으로 설명하자면...
		//
		//    case 1) m_queue_pending가 empty()가 아닌데 m_flag_sending가 false인 상황
		//    case 1) m_queue_sending이 empty()가 아닌 상황. 
		//            m_queue_sending는 메모리 중복 할당을 피하기 위한 단순 버퍼의 역할만 한다.따라서 항상 empty상태여야 한다.
		//
	
		// check)
		if (this->m_queue_pending.array_node.empty())
		{
			// statistics) 
			this->statistics_on_decrease_async_sending();

			// check)
			CGASSERT_ERROR(this->m_queue_pending.bytes_data == 0);
	
			// - set flag
			this->m_flag_sending = false;
	
			// return)
			return 0;
		}
	
		// check) 
		CGASSERT_ERROR(this->m_flag_sending == true);
	
		// check)
		CGASSERT_ERROR(this->m_queue_sending.array_node.empty() == true);
	
		// - swap data
		this->m_queue_sending.array_node.swap(this->m_queue_pending.array_node);
		this->m_queue_pending.bytes_data = 0;

		// - sendmsg는 한번에 IOV_MAX(linux: 1024)개의 buffer까지만 보낼 수 있다. (넘으면 sendmsg가 실패한다.)
		//   넘는 buffer는 m_queue_pending으로 되돌려 다음 EPOLLOUT에서 보낸다. (순서는 유지된다.)
		if (this->m_queue_sending.array_node.size() > CGDK_SEND_MAX_IOV)
		{
			auto iter_split = std::next(this->m_queue_sending.array_node.begin(), CGDK_SEND_MAX_IOV);
			this->m_queue_pending.array_node.splice(this->m_queue_pending.array_node.end(), this->m_queue_sending.array_node, iter_split, this->m_queue_sending.array_node.end());

			for (const auto& iter : this->m_queue_pending.array_node)
				this->m_queue_pending.bytes_data += iter.buf_send.size();
		}

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

	for (;;)
	{
		// check)
		CGASSERT_ERROR(bytes_sending != 0);

		// 3) send 한다.
		auto bytes_result = ::sendmsg(this->native_handle(), &msg_send, MSG_DONTWAIT | MSG_NOSIGNAL);

		// 4-1) [case] send completed~
		if (bytes_result == static_cast<int>(bytes_sending))
		{
			// - process sent buffer
			for (auto iter = queue_send.begin(), iter_end = queue_send.end(); iter != iter_end; ++iter)
				this->process_complete_sendable(0, iter->bytes_data, iter->count_message);

			// - clear and swap buffer
			queue_send.clear();
			queue_send.swap(this->m_queue_sending.array_node);

			// - shutdown if send_disconnected
			if (this->socket_state() == eSOCKET_STATE::SEND_DISCONNECTED)
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

			// - process sent buffer
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
				process_complete_sendable(0, iter->bytes_data, iter->count_message);
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
				// - move remained to m_queue_pending
				SCOPED_LOCK(this->m_lockable_sending)
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

				// - closesocket
				this->process_close_socket(DISCONNECT_REASON_FAIL);

				// - clear send queue
				SCOPED_LOCK(this->m_lockable_sending)
				{
					this->_process_clear_send_buffers();
				}

				// break)
				break;
			}
		}
	}

	// declare) 
	bool flag_sending;

	// 5) set event flag
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// check) EPOLLOUT이 이미 설정된 상태면 안 된다.
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

std::size_t net::io::sendable::Ntcp_crypt_async_gather_buffered::process_get_sending_queued_bytes() const noexcept
{
	return this->m_queue_pending.bytes_data;
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_close_sender()
{
	// check)
	bool do_shutdown;

	SCOPED_LOCK(this->m_lockable_sending)
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

void net::io::sendable::Ntcp_crypt_async_gather_buffered::process_reset_sendable() noexcept
{
	// 1) ...
	SCOPED_LOCK(this->m_lockable_sending)
	{
		// - reset send buffer
		this->m_queue_pending.clear();

		// - reset sending buffer
		if (this->m_flag_sending == false)
			this->m_queue_sending.clear();
	}

	// 2) reset socket option
	this->maximum_send_buffer_size(net::io::socket::Ntcp::default_maximum_send_buffer_size());
}

void net::io::sendable::Ntcp_crypt_async_gather_buffered::_process_clear_send_buffers()
{
	for ([[maybe_unused]] auto& iter : this->m_queue_pending.array_node)
		this->process_complete_sendable(1, 0, 0);

	this->m_flag_sending = false;

	this->m_queue_pending.clear();
	this->m_queue_sending.clear();
}

#endif
