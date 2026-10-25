#include "pch.h"

#if defined(_WINSOCK2API_)

#include "cgdk/sdk10/net.socket/socket.crypt/net.io.sendable.tcp_crypt.executable_buffer.h"

using namespace CGDK;

bool net::io::sendable::Ntcp_crypt::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	//-----------------------------------------------------------------
	// 설명) 처리 내용 및 절차.
	// 
	//    1) 일단 Overlapped I/O에 사용될 executable를 Pool에서 할당받는다.
	//    2) 할당 받은 send executable에 sender와 Senderable 그리고 Original
	//       값을 저장시켜놓는다.(나중에 완료 시 사용을 위해)
	//    3) executable를 넣어 Overlapped로 WSASend()함수를 호출한다.
	//    4) 만약 WSASend가  실패했을 경우 모두를 rollback시킨다.
	//
	//-----------------------------------------------------------------
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

	// 1) pool에서 executable를 할당받는다.
	auto pexecutable = make_object<executable_send_buffer>();

	// check) reference count가 1이여야 한다.
	CGASSERT_ERROR(pexecutable->reference_count()==1);

	// 2) executable를 설정한다.
	pexecutable->m_psendable = this;
	pexecutable->m_powner = _powner;
	pexecutable->m_count_message =_count_message;

	// 3) reference count를 증가시킨다.
	pexecutable->add_ref();

	// 4) send overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();

	// declare)
	bool result = false;

	// - the encryption, the handle and WSASend under m_lockable_send_call : the cryptor is a stream cipher (the order of the encryption must be
	//   the order of the WSASends) and the close swaps the handle under it (a handle closed and made again for another socket is not sent to)
	SCOPED_LOCK(this->m_lockable_send_call)
	{
		// 5) send cryptor를 얻는다.
		object_ptr<Icryptor> pcryptor = this->get_cryptor();

		// 6) set buffer
		if (pcryptor.empty())
		{
			pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };
		}
		else
		{
			// - 암호화한다.
			auto buf_crypt = pcryptor->encrypt(_buffer);

			// - set buffer
			pexecutable->m_wsabuf = WSABUF{ buf_crypt.size<ULONG>(), buf_crypt.data() };;
			pexecutable->m_reference = buf_crypt.get_source();
		}

		// 9) send async
		const auto handle_socket = this->native_handle();

		if (handle_socket != INVALID_SOCKET)
			result = api::socket::send_async(handle_socket, &pexecutable->m_wsabuf, 1, nullptr, pexecutable.get());
	}

	// check) Result가 false면 전송실패다.
	if(result == false)
	{
		// 통계) 
		this->statistics_on_error_send();

		// - send Overlapped 카운트를 하나 줄인다.
		this->statistics_on_decrease_async_sending();

		// - Release한다.
		pexecutable->release();

		// return) 전송에 실패했다.
		return false;
	}

	// 통계) (the overlapped count was increased above : once per WSASend)
	this->statistics_on_send_async();

	// return) 성공!!
	return true;
}

#endif