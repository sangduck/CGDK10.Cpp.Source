#include "pch.h"

#if defined(_WINSOCK2API_)

#include "net.io.sendable.executable_send_buffer.h"

using namespace CGDK;

#pragma warning(disable:4100)


bool net::io::sendable::Nudp::process_sendable( const buffer_view _buffer, object_ptr<Ireferenceable>&& _powner, std::size_t _count_message, const net::sockaddr& _address)
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

	// 4) Invalid socket인가?
	if(this->native_handle() == INVALID_SOCKET)
	{
		LOG_DEBUG << "(err ) CGDK.net.socket: socket handle is INVALID_HANDLE " __CGFUNCTION__;
		return false;
	}


	//-----------------------------------------------------------------
	// 1. send준비를 한다.
	//-----------------------------------------------------------------
	// 1) Pool에서 executable를 할당받는다.
	auto pexecutable = make_object<executable_send_buffer>();

	// check) Reference count가 1이여야 한다.
	CGASSERT_ERROR(pexecutable->reference_count()==1);

	// 2) executable을 준비한다.
	pexecutable->m_psendable = this;
	pexecutable->m_powner = std::move(_powner);
	pexecutable->m_count_message = _count_message;
	pexecutable->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() };
	pexecutable->m_sockaddr = _address;


	// 3) Reference count를 증가시킨다.
	pexecutable->add_ref();

	// 4) send Overlapped 카운트를 하나 증가시킨다.
	this->statistics_on_increase_async_sending();


	//-----------------------------------------------------------------
	// 2. Overlapped I/O로 send한다.
	//-----------------------------------------------------------------
	// 1) send 한다.
	auto result = api::socket::send_to_async(this->native_handle(), &pexecutable->m_wsabuf, 1, nullptr, (::sockaddr*)&pexecutable->m_sockaddr, _address.length, pexecutable.get());

	// check) Result가 false면 send 못한거다..
	if(result == false)
	{
		// 통계) send Error를 더한다.
		this->statistics_on_error_send();

		// trace) 
		LOG_DEBUG << "(err ) CGDK.net.socket: fail to call 'sent_to' function " __CGFUNCTION__;

		// - send Overlapped 카운트를 하나 줄인다.
		this->statistics_on_decrease_async_sending();

		// - 실패했으므로 Transacter를 Release한다.
		pexecutable->release();

		// return) false를 return한다.
		return false;
	}

	// statistics) 
	this->statistics_on_send_async();


	// return) 성공!!
	return true;
}

#if defined(_MSC_VER)
	#pragma warning(default:4100)
#endif

void net::io::sendable::Nudp::process_complete_sendable( uintptr_t /*_result*/, std::size_t _transferred_bytes, std::size_t _transferred_message)
{
	// statistics) 
	this->statistics_on_send_bytes(_transferred_bytes);
	this->statistics_on_send_message(_transferred_message);

	// statistics)
	this->statistics_on_decrease_async_sending();
}

void net::io::sendable::Nudp::process_reset_sendable() noexcept
{
}


#endif