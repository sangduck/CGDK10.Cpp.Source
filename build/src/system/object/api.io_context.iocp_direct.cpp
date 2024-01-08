#include "pch.h"
#include <chrono>


#if defined(_IO_APISET_H_)

namespace CGDK
{

api::io_context_iocp_direct::io_context_iocp_direct() noexcept
{
}

api::io_context_iocp_direct::~io_context_iocp_direct() noexcept
{
	destroy();
}

bool api::io_context_iocp_direct::create() noexcept
{
	// 1) pa
	auto result = io_context_Niocp::create();

	// check)
	RETURN_IF(result == false, false);

	// return)
	return true;
}

CGHANDLE api::io_context_iocp_direct::dispose() noexcept
{
	// 1) dispose 
	auto h_iocp = io_context_Niocp::dispose();

	// check)
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, h_iocp);


	// return)
	return h_iocp;
}

result_code api::io_context_iocp_direct::destroy() noexcept
{
	// 1) set iocp handle to INVALID_CGHANGLE_VALUE and get old iocp handle
	auto result = io_context_Niocp::destroy();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) ...


	// return)
	return eRESULT::SUCCESS;
}

bool api::io_context_iocp_direct::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// 1) get handle
	auto iocp_handle = this->handle();

	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(iocp_handle == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안됀다.
	RETURN_IF(_pexecutable == nullptr, false);

	// 2) add_ref한다.
	_pexecutable->add_ref();

	// 3) post 한다.
	auto result = ::PostQueuedCompletionStatus(iocp_handle, _bytes_transferred, _completion_key, _pexecutable);

	if (result == 0)
	{
		// - rollback
		_pexecutable->release();

		// return)
		return false;
	}

	// return)
	return true;
}

void api::io_context_iocp_direct::pop_executable(RESULT& _result, [[maybe_unused]] chrono::tick::duration _ticks_wait) noexcept
{
	// 1) get epoll handle
	CGHANDLE iocp_handle = handle();

	// check) 
	RETURN_IF(iocp_handle == INVALID_CGHANDLE_VALUE);

	// 2) get wait time
	DWORD tick_wait = (_ticks_wait == chrono::tick::duration::max()) ? INFINITE : static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(_ticks_wait).count());

	// 2) get epoll events
	auto result = ::GetQueuedCompletionStatus(
		iocp_handle,
		reinterpret_cast<LPDWORD>(&_result.bytes_transfered),
		reinterpret_cast<PULONG_PTR>(&_result.per_handle_key),
		reinterpret_cast<LPOVERLAPPED*>(&_result.pexecutable),
		tick_wait);

	// 2) result값 보정
	//    GetQueuendCom...()함수가 성공했을 때0이 아닌값을 리턴한다. 실패했을 때 0이 리턴된다.
	//    하지만 _result.result_값은 성공일 경우 0, 실패일 경우 error code를 넣도록 되어 있다.
	//    따라서 결과값을 반전해 줘야 한다. 
	_result.result_ = (result == 0);
}


}

#endif