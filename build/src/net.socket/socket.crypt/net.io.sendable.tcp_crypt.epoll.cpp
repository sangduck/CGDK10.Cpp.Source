#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;


bool net::io::sendable::Ntcp_crypt::process_sendable(const buffer_view _buffer, object_ptr<Ireferenceable>&& /*_powner*/, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
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

	// check) if socket is not connected, return false (before lock)
	if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// declare)
	shared_buffer buffer_crypt;

	// 2) send cryptor를 얻는다.
	object_ptr<Icryptor> pcryptor_send = this->get_cryptor();

	// 3) send_cryptor가 있을 경우 cryption을 수행한다.
	if(pcryptor_send.exist())
		buffer_crypt = pcryptor_send->encrypt(_buffer); // - 암호화한다.
	else
		buffer_crypt = _buffer;

	// 5) send
	auto bytes_sent = ::send(native_handle(), buffer_crypt.data(), buffer_crypt.size(), MSG_DONTWAIT | MSG_NOSIGNAL);
		
	// check) bytes_sent가 0보다 작으면 전송에 실패한 것이다.
	if(bytes_sent >= 0)
	{
		this->process_complete_sendable(0, static_cast<uint32_t>(bytes_sent), _count_message);
	}
	else
	{
		// - get last error code
		auto error_code = api::get_last_error();

		// check)
		if (error_code == EWOULDBLOCK)
		{
			// statistics) 
			this->statistics_on_increase_async_sending();

			// - set EPOLLOUT event flag
			this->post_epoll_event_EPOLLOUT();
		}
		else
		{
			// - set last error
			this->m_last_socket_error = error_code;

			// statistics) 
			this->statistics_on_error_send();

			// - process_complete_sendable을 호출한다.(실패이므로 _result값은 1로, _transfered_byte는 0으로 전달한다.)
			this->process_complete_sendable(1, 0, 0);

			// return) 전송에 실패했다.
			return	false;
		}
	}

	// return) 성공!!
	return true;
}

#endif