#include "pch.h"
#include "platform.h"
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

	// check) _pexecutable이 nullptr이면 안 된다.
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

bool api::io_context_iocp_direct::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// - 넘겨받은 참조를 그대로 completion에 넘긴다. (add_ref하고 호출한 쪽이 release하는 한 쌍을 줄인다)

	// 1) get handle
	auto iocp_handle = this->handle();

	// check)
	RETURN_IF(iocp_handle == INVALID_CGHANDLE_VALUE, false);

	// check)
	auto pexecutable = _pexecutable.get();
	RETURN_IF(pexecutable == nullptr, false);

	// 2) post (실패하면 참조는 _pexecutable에 남아 있다 : 호출한 쪽에서 해제된다)
	RETURN_IF(::PostQueuedCompletionStatus(iocp_handle, _bytes_transferred, _completion_key, pexecutable) == 0, false);

	// 3) 참조는 completion을 처리하는 thread로 넘어갔다.
	//    (그 thread가 이미 실행하고 release했을 수 있으므로 객체는 건드리지 않고 포인터만 비운다)
	_pexecutable.detach();

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

// - RESULT has the layout of OVERLAPPED_ENTRY (GetQueuedCompletionStatusEx() writes the array of RESULT)
static_assert(sizeof(api::io_context::RESULT) == sizeof(OVERLAPPED_ENTRY));
static_assert(offsetof(api::io_context::RESULT, per_handle_key) == offsetof(OVERLAPPED_ENTRY, lpCompletionKey));
static_assert(offsetof(api::io_context::RESULT, pexecutable) == offsetof(OVERLAPPED_ENTRY, lpOverlapped));
static_assert(offsetof(api::io_context::RESULT, result_) == offsetof(OVERLAPPED_ENTRY, Internal));
static_assert(offsetof(api::io_context::RESULT, bytes_transfered) == offsetof(OVERLAPPED_ENTRY, dwNumberOfBytesTransferred));

int api::io_context_iocp_direct::pop_results(RESULT* _presult, int _max_result, chrono::tick::duration _ticks_wait) noexcept
{
	// 1) get iocp handle
	CGHANDLE iocp_handle = handle();

	// check)
	if (iocp_handle == INVALID_CGHANDLE_VALUE || _presult == nullptr || _max_result <= 0)
		return 0;

	// 2) get wait time
	DWORD tick_wait = (_ticks_wait == chrono::tick::duration::max()) ? INFINITE : static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(_ticks_wait).count());

	// 3) 완료를 한 번에 여러 개 받는다. (GetQueuedCompletionStatus()는 syscall 한 번에 하나)
	ULONG count = 0;
	auto result = ::GetQueuedCompletionStatusEx(
		iocp_handle,
		reinterpret_cast<LPOVERLAPPED_ENTRY>(_presult),
		static_cast<ULONG>((std::min)(_max_result, MAX_EVENT_BATCH)),
		&count,
		tick_wait,
		FALSE);

	// check) timeout(WAIT_TIMEOUT) 또는 iocp가 닫힘
	if (result == FALSE)
		return 0;

	// 4) result값 보정
	//    - OVERLAPPED_ENTRY::Internal은 그 완료 packet의 NTSTATUS다. GetQueuedCompletionStatus()는 이것이 실패(NT_SUCCESS가 아님 : 음수)면
	//      FALSE를 돌려주므로 같은 규칙으로 성공 0 / 실패 1 로 바꾼다.
	//    - lpOverlapped->Internal을 읽으면 안 된다 : executable은 I/O와 PostQueuedCompletionStatus() 양쪽에 쓰이므로
	//      post된 executable의 OVERLAPPED에는 이전 I/O의 실패 상태가 남아 있을 수 있다. (post packet의 Internal은 0)
	for (ULONG i = 0; i < count; ++i)
	{
		_presult[i].result_ = (static_cast<LONG>(static_cast<ULONG>(static_cast<ULONG_PTR>(_presult[i].result_))) < 0) ? 1 : 0;
	}

	// return)
	return static_cast<int>(count);
}


}

#endif