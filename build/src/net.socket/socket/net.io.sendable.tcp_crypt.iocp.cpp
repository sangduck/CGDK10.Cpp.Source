#include "pch.h"

#if defined(_WINSOCK2API_)

#include "cgdk/sdk10/net.socket/socket/net.io.sendable.tcp_crypt.executable_buffer.h"

using namespace CGDK;

bool net::io::sendable::Ntcp_crypt::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
{
	//-----------------------------------------------------------------
	// 설명) 처리 내용 및 절차.
	// 
	//    1) 일단 Overlapped I/O에 사용될 executable를 Pool에서 할당받는다.
	//    2) 할당 받은 send executable에 sender와 Senderable 그리고 Original
	//       값을 저장시켜놓는다.(나중에 완료시 사용을 위해)
	//    3) executable를 넣어 Overlapped로 WSASend()함수를 호출한다.
	//    4) 만약 WSASend가  실패했을 경우 모두를 rollback시킨다.
	//
	//-----------------------------------------------------------------
	// check) _buffer.size()이 0이 아닌가?
	CGASSERT(_buffer.size() != 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	RETURN_IF(_buffer.size() == 0, false);

	// check) _buffer.data()가 NULL이 아닌가?
	CGASSERT(_buffer.data() != nullptr, false);

	// check) _buffer.data()가 NULL이 아닌가?
	THROW_IF(_buffer.data() == nullptr, throwable::failure(eRESULT::INVALID_MESSAGE_BUFFER_IS_NULL, 0, "buffer.data is null ('net::io::sendable::Ntcp_crypt::process_sendable')"sv));

	// check) if socket is not connected, return false (before lock)
	RETURN_IF(this->socket_state() < eSOCKET_STATE::ESTABLISHED, false);

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

	// 5) send cryptor를 얻는다.
	object_ptr<Iencoder> pcryptor_send = get_encoder();

	// 6) set buffer
	if (pcryptor_send.empty())
	{
		pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };
	}
	else
	{
		// - 암호화된 message를 저장할 shared_buffer를 생성한다.
		auto buf_crypt = alloc_shared_buffer(_buffer.size<int>());

		// - 암호화처리
		buf_crypt = pcryptor_send->encode(buf_crypt, _buffer);

		// - set buffer
		pexecutable->m_wsabuf = WSABUF{ buf_crypt.size<ULONG>(), buf_crypt.data() };;
		pexecutable->m_reference = buf_crypt.get_source();
	}

	// 9) send asnc
	auto result = api::socket::send_async(native_handle(), &pexecutable->m_wsabuf, 1, nullptr, pexecutable.get());

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

	// 통계) 
	this->statistics_on_increase_async_sending();

	// return) 성공!!
	return true;
}

#endif