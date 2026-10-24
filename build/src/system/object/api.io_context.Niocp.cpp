#include "pch.h"
#include "platform.h"

#if defined(_IO_APISET_H_)

namespace CGDK
{

api::io_context_Niocp::io_context_Niocp() noexcept :
	m_handle_iocp(INVALID_CGHANDLE_VALUE),
	m_handle_iocp_created(INVALID_CGHANDLE_VALUE),
	m_count_attached(0)
{
}

api::io_context_Niocp::~io_context_Niocp() noexcept
{
	this->destroy();
}

bool api::io_context_Niocp::create() noexcept
{
	// check) already exist?
	RETURN_IF(this->m_handle_iocp != INVALID_CGHANDLE_VALUE || this->m_handle_iocp_created != INVALID_CGHANDLE_VALUE, false);

	// 1) create iocp
	auto h_iocp = ::CreateIoCompletionPort(INVALID_CGHANDLE_VALUE, NULL, 0, 0);

	// check) 
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, false);

	// 2) set values
	this->m_handle_iocp = h_iocp;
	this->m_handle_iocp_created = h_iocp;
	this->m_count_attached = 0;

	// return)
	return true;
}

CGHANDLE api::io_context_Niocp::dispose() noexcept
{
	// 1) iocp handle
	auto h_iocp = this->m_handle_iocp.exchange(INVALID_CGHANDLE_VALUE);

	// check) return if iocp handle is not exist
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, h_iocp);

	// 2) cancel i/o
	cancel_io(h_iocp);

	// return)
	return h_iocp;
}

result_code api::io_context_Niocp::destroy() noexcept
{
	// 1) dispose first (m_handle_iocp를 INVALID로 바꾸고 진행 중인 i/o를 취소한다)
	//    (이전 : 여기서 먼저 exchange해서 dispose()가 INVALID를 받아 cancel_io를 하지 않았다.)
	this->dispose();

	// 2) get handle created (m_handle_iocp을 INVALID_CGHANDLE_VALUE로 바꾼다.)
	auto h_iocp_create = this->m_handle_iocp_created.exchange(INVALID_CGHANDLE_VALUE);

	// check) m_handle_iocp이 INVALID_CGHANDLE_VALUE이면 생성되지 않았다는 의미이다.
	RETURN_IF(h_iocp_create == INVALID_CGHANDLE_VALUE, eRESULT::FAIL);

	// 3) close iocp
	auto result = ::CloseHandle(h_iocp_create);

	// check) 
	RETURN_IF(result == false, eRESULT::FAIL);

	// return)
	return eRESULT::SUCCESS;
}

bool api::io_context_Niocp::attach(CGHANDLE _handle) noexcept
{
	// check) m_handle_iocp가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(handle() == INVALID_CGHANDLE_VALUE, false);

	// 1) 연결한다.
	auto result = ::CreateIoCompletionPort(reinterpret_cast<CGHANDLE>(_handle), m_handle_iocp, reinterpret_cast<uintptr_t>(_handle), 0);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result == INVALID_CGHANDLE_VALUE, false);

	// 3) attach개수를 증가시킴.
	++m_count_attached;

	// return)
	return true;
}

bool api::io_context_Niocp::detach(CGHANDLE _handle) noexcept
{
	// check) m_handle_iocp가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(handle() == INVALID_CGHANDLE_VALUE, false);

	// 1) 연결한다.
	auto result = ::CreateIoCompletionPort(reinterpret_cast<CGHANDLE>(_handle), NULL, uintptr_t(NULL), 0);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result == INVALID_CGHANDLE_VALUE, false);

	// 2) attach개수를 감소시킴.
	--m_count_attached;

	// return) 성공!!!
	return true;
}

bool api::io_context_Niocp::cancel_io() noexcept
{
	return cancel_io(handle())!=0;
}

bool api::io_context_Niocp::cancel_io(CGHANDLE _handle) noexcept
{
	return ::CancelIo(_handle)!=0;
}


}
#endif