#include "pch.h"
#include "platform.h"


#if defined(__ANDROID__) || defined(__linux__)

namespace CGDK
{

api::io_context_rio::io_context_rio() noexcept :
	m_handle_iocp(INVALID_CGHANDLE_VALUE),
	m_count_attached(0),
	m_count_wait(0)
{
}

api::io_context_rio::~io_context_rio() noexcept
{
	destroy();
}

bool api::io_context_rio::create() noexcept
{
	// check) already exist?
	RETURN_IF(handle() != INVALID_CGHANDLE_VALUE, false);

	// 1) create iocp
	auto h_iocp = ::CreateIoCompletionPort(INVALID_CGHANDLE_VALUE, NULL, 0, 0);

	// check) 
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, false);

	// 2) set values
	this->m_handle_iocp = h_iocp;
	this->m_count_attached = 0;
	this->m_count_wait = 0;

	// declare) 
	class threads_iocp : public api::threads, public object::Nnameable
	{
	public:
		threads_iocp() : object::Nnameable("iocp event pump") {}
	};

	// 3) 
	auto pthread = make_object<threads_iocp>();

	// 4) begin thread
	pthread->begin_thread(_fn_process_popup_iocp_event, this);

	// 5) set
	this->m_pthread = std::move(pthread);

	// return)
	return true;
}

result_code api::io_context_rio::destroy() noexcept
{
	// 1) set iocp handle to INVALID_CGHANGLE_VALUE and get old iocp handle
	auto h_iocp = this->m_handle_iocp.exchange(INVALID_CGHANDLE_VALUE);
	
	// check) return if iocp handle is not exist
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, result_code(eRESULT::FAIL));

	// 2) close iocp
	auto result = ::CloseHandle(h_iocp);

	// check) 
	RETURN_IF(result == false, eRESULT::FAIL);

	// 3) 
	auto pthread = std::move(this->m_pthread);

	// 3) wait thread end
	pthread->wait();

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool api::io_context_rio::attach(CGHANDLE _handle) noexcept
{
	// check) m_handle_iocp가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->handle() == INVALID_CGHANDLE_VALUE, false);

	// 1) 연결한다.
	auto result = ::CreateIoCompletionPort(reinterpret_cast<CGHANDLE>(_handle), this->m_handle_iocp, reinterpret_cast<uintptr_t>(_handle), 0);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result == INVALID_CGHANDLE_VALUE, false);

	// 3) attach개수를 증가시킴.
	++this->m_count_attached;

	// return)
	return true;
}

bool api::io_context_rio::detach(CGHANDLE _handle) noexcept
{
	// check) m_handle_iocp가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->handle() == INVALID_CGHANDLE_VALUE, false);

	// 1) 연결한다.
	auto result = ::CreateIoCompletionPort(reinterpret_cast<CGHANDLE>(_handle), NULL, uintptr_t(NULL), 0);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result == INVALID_CGHANDLE_VALUE, false);

	// 2) attach개수를 감소시킴.
	--this->m_count_attached;

	// return) 성공!!!
	return true;
}

bool api::io_context_rio::cancel_io() noexcept
{
	return cancel_io(this->handle()) != 0;
}

bool api::io_context_rio::cancel_io(CGHANDLE _handle) noexcept
{
	return ::CancelIo(_handle) != 0;
}

bool api::io_context_rio::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) handle()이 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->handle() == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable == nullptr, false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.per_handle_key = _completion_key;
		result.pexecutable = _pexecutable;
		result.result_ = 0;
		result.bytes_transfered = _bytes_transferred;

		// 2) Post한다.
		{
			// lock)
			std::lock_guard lock(this->m_lockable_complete);

			// - queueing한다.
			this->m_queue_executable.push_back(result);

			// - add_ref한다.
			_pexecutable->add_ref();
		}

		// 3) Signal시킨다.
		this->m_cv_complete.notify_one();
	}
	catch (...)
	{
		// return)
		return false;
	}

	// return)
	return true;
}

bool api::io_context_rio::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->handle() == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable.empty(), false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.per_handle_key = _completion_key;
		result.pexecutable = _pexecutable;
		result.result_ = 0;
		result.bytes_transfered = _bytes_transferred;

		// 2) push 한다.
		{
			// lock)
			std::lock_guard lock(this->m_lockable_complete);

			// - ㅂqueueing한다.
			this->m_queue_executable.push_back(result);

			// - add_ref한다.
			_pexecutable->add_ref();
		}

		// 3) push후 notify
		this->m_cv_complete.notify_one();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return) 
	return true;
}

void api::io_context_rio::pop_queued_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
{
	CONDITIONAL_LOCK(this->m_lockable_complete, lock_wait)
	{
		// 1) 개수가 0개면 wait...
		if (this->m_queue_executable.size() == 0 && _ticks_wait != chrono::tick::duration::zero())
		{
			// - 
			++this->m_count_wait;

			// - wait
			auto result = this->m_cv_complete.wait_for(lock_wait, _ticks_wait);

			// - 
			--this->m_count_wait;

			// check)
			RETURN_IF(result == std::cv_status::timeout);
		}

		// check)
		RETURN_IF(this->m_queue_executable.empty());

		// 2) count만큼 Queueing된 Executable을 pop해낸다.
		_result = std::move(this->m_queue_executable.front());

		// 3) pop front
		this->m_queue_executable.pop_front();
	}
}

void api::io_context_rio::_fn_process_popup_iocp_event(api::io_context_rio* _pobject)
{
	_pobject->_process_popup_iocp_event();
}

void api::io_context_rio::_process_popup_iocp_event()
{
	// declare) 
	const int MAX_EVENTS = 1024;

	// 1) alloc buffer
	auto buf_overlapped_event = alloc_shared_buffer(sizeof(OVERLAPPED_ENTRY) * MAX_EVENTS);

	// 2) get 
	auto* list_overlapped_event = buf_overlapped_event.data<OVERLAPPED_ENTRY>();

	for (;;)
	{
		// 3) get epoll handle
		CGHANDLE iocp_handle = this->handle();

		ULONG count = 0;

		// check) 
		BREAK_IF(iocp_handle == INVALID_CGHANDLE_VALUE);

		// 4) get epoll events
		auto result = GetQueuedCompletionStatusEx(
			iocp_handle,
			list_overlapped_event,
			MAX_EVENTS,
			&count,
			INFINITE, // static_cast<DWORD>(std::chrono::duration_cast<chrono::tick::milliseconds>(_ticks_wait).count()),
			FALSE
		);

		// check) 
		BREAK_IF(result == false);

		// check) 
		CONTINUE_IF(count <= 0);

		// declare)
		int count_signal = 0;

		// 5) post executable
		{
			// lock)
			std::lock_guard lock(this->m_lockable_complete);

			// - reserve
			this->m_queue_executable.reserve(this->m_queue_executable.size() + count);

			// - push
			auto iter = list_overlapped_event;
			auto iter_end = list_overlapped_event + count;

			for (; iter != iter_end; ++iter)
			{
				this->m_queue_executable.push_back(*reinterpret_cast<RESULT*>(iter));
			}

			// - get signal count
			count_signal = __select_min(this->m_count_wait, static_cast<int>(count));
		}

		// 6) signal
		for (; count_signal != 0; --count_signal)
		{
			this->m_cv_complete.notify_one();
		}
	}
}

}
#endif