#include "pch.h"

#if defined(_WINSOCK2API_)

#include "net.io.sendable.executable_send_buffer.h"

using namespace CGDK;

bool net::io::sendable::Ntcp_async_rio::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& /*_paddress*/)
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

	// check) if socket is not connected, return false
	if(this->socket_state() < eSOCKET_STATE::ESTABLISHED)
		return false;

	// 1) get socket native handle
	CGSOCKET handle_socket = native_handle();

	// check) 
	if(handle_socket == INVALID_SOCKET)
		return false;

	// 2) async send and return result
	return process_sendable_async(handle_socket, _buffer, std::move(_powner), _count_message);
}

bool net::io::sendable::Ntcp_async_rio::process_sendable_async(CGSOCKET _handle_socket, const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message)
{
	// 1) alloc executable object
	auto pexecutable = make_object<executable_send_buffer>();

	// check)
	CGASSERT_ERROR(pexecutable->reference_count() == 1);

	// 2) prepare executable object
	pexecutable->m_psendable = this;
	pexecutable->m_powner = std::move(_powner);
	pexecutable->m_count_message = _count_message;

	// 3) increase reference count
	pexecutable->add_ref();

	// 4) reset param & set temporary buffer (for reserve during send operation)
	pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data()};

	// statistics)
	this->statistics_on_increase_async_sending();

	// 5) send~
	auto result = api::socket::send_async(_handle_socket, &pexecutable->m_wsabuf, 1, nullptr, pexecutable.get());

	// check) 
	if (result == false)
	{
		// statistics)
		this->statistics_on_error_send();

		// statistics)
		this->statistics_on_decrease_async_sending();

		// - rollback (release)
		pexecutable->release();

		// return) fail to send
		return false;
	}

	// statistics) 
	this->statistics_on_send_async();

	// return)
	return true;
}

void net::io::sendable::Ntcp_async_rio::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics)
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);

	// statistics)
	this->statistics_on_decrease_async_sending();
}

void net::io::sendable::Ntcp_async_rio::process_reset_sendable() noexcept
{
}

#endif