#include "pch.h"
#include "platform.h"

#if defined(_IO_APISET_H_)

namespace CGDK
{

api::io_context_iocp_queued::io_context_iocp_queued() noexcept : 
	m_count_wait(0)
{
}

api::io_context_iocp_queued::~io_context_iocp_queued() noexcept
{
	destroy();
}

bool api::io_context_iocp_queued::create() noexcept
{
	// 1) pa
	auto result = io_context_Niocp::create();

	// check)
	RETURN_IF(result == false, false);

	// 2) 값의 초기화
	m_count_wait = 0;

	// declare) 
	class threads_iocp : public api::threads, public object::Nnameable
	{
	public:
		threads_iocp() { this->name("iocp event pump"sv); }
	};

	// 3) get iocp threads
	auto count_iocp_thread = api::get_core_count() >> 4; // core_count / 8

	// 4) saturate
	if (count_iocp_thread <= 0)
		count_iocp_thread = 1;

	// declare)
	std::vector<object_ptr<threads>> vector_thread;

	// 5) begin thread
	for (int i=0; i<count_iocp_thread; ++i)
	{
		// - create thread
		auto pthread = make_object<threads_iocp>();

		// - begin thread
		pthread->begin_thread(_fn_process_popup_iocp_event, this);

		// - ...
		vector_thread.push_back(std::move(pthread));
	}

	// 6) set
	m_vector_thread = std::move(vector_thread);

	// return)
	return true;
}

CGHANDLE api::io_context_iocp_queued::dispose() noexcept
{
	// 1) dispose 
	auto h_iocp = io_context_Niocp::dispose();

	// check)
	RETURN_IF(h_iocp == INVALID_CGHANDLE_VALUE, h_iocp);

	// 2) 완료를 기다린다.


	// return)
	return h_iocp;
}

result_code api::io_context_iocp_queued::destroy() noexcept
{
	// 1) set iocp handle to INVALID_CGHANGLE_VALUE and get old iocp handle
	auto result = io_context_Niocp::destroy();

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	// 2) threads
	auto vector_thread = std::move(m_vector_thread);

	// 5) wait thread terminate
	for (auto& iter : vector_thread)
	{
		iter->wait();
	}

	// return)
	return eRESULT::SUCCESS;
}

bool api::io_context_iocp_queued::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) handle()이 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(handle() == INVALID_CGHANDLE_VALUE, false);

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

bool api::io_context_iocp_queued::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(handle() == INVALID_CGHANDLE_VALUE, false);

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

		// 2) Post한다.
		{
			// lock)
			std::lock_guard lock(this->m_lockable_complete);

			// - Queueing한다.
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

void api::io_context_iocp_queued::pop_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
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
			--m_count_wait;

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

void api::io_context_iocp_queued::_fn_process_popup_iocp_event(api::io_context_iocp_queued* _pobject)
{
	_pobject->_process_popup_iocp_event();
}

void api::io_context_iocp_queued::_process_popup_iocp_event()
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