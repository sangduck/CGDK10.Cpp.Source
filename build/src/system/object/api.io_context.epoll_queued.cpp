#include "pch.h"
#include "platform.h"

#if defined(__ANDROID__) || defined(__linux__)
#include <sys/eventfd.h>
#include <iostream>

namespace CGDK
{

api::io_context_epoll_queued::io_context_epoll_queued() :
	m_count_wait(0)
{
}

api::io_context_epoll_queued::~io_context_epoll_queued() noexcept
{
	this->destroy();
}

bool api::io_context_epoll_queued::create(int _hint_size) noexcept
{
	// check) 
	RETURN_IF(this->m_count_attached == true, false);

	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이 아니면 이미 만들어졌다는 뜻이다.
	RETURN_IF(this->m_handle_epoll != INVALID_CGHANDLE_VALUE, false);

	// 1) epoll객체를 생성한다.
	static_cast<void>(_hint_size);
	auto h_epoll = epoll_create1(EPOLL_CLOEXEC);		// - EPOLL_CLOEXEC : not inherited by fork/exec (size hint is ignored by linux 2.6.8+)

	// check) epoll를 적절하게 생성했는가?
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, false);

	// 2) 값을 설정한다.
	this->m_handle_epoll = h_epoll;
	this->m_count_attached = 0;
	this->m_count_wait = 0;

	// declare) 
	class threads_epoll : public api::threads, public object::Nnameable
	{
	public:
		threads_epoll() : object::Nnameable() { this->name("epoll event pump"sv); }
	};

	// 3) make thread for epoll
	auto pthread = make_object<threads_epoll>();

	// 4) begin thread
	pthread->begin_thread(_fn_process_popup_epoll_event, this);

	// 5) set
	this->m_pthread = std::move(pthread);

	// return)
	return true; 
}

CGHANDLE api::io_context_epoll_queued::dispose() noexcept
{
	// 1) dispose 
	auto handle_epoll = io_context_Nepoll::dispose();

	// check)
	RETURN_IF(handle_epoll == INVALID_CGHANDLE_VALUE, m_handle_epoll);


	// return)
	return handle_epoll;
}

result_code api::io_context_epoll_queued::destroy() noexcept
{
	// 1) m_handle_epoll을 NULL로 바꾼다.
	auto h_epoll = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// check) m_handle_epoll이 INVALID_CGHANDLE_VALUE이면 생성되지 않았다는 의미이다.
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, INVALID_CGHANDLE_VALUE);

	// 2) release all executable
	this->_process_remove_all_handle(h_epoll);

	// declare)
	object_ptr<api::threads> pthread;

	// 3) get threads and wait terminate
	SCOPED_LOCK(this->m_lockable_complete)
	{
		pthread = std::move(this->m_pthread);
	}

	// check)
	CGASSERT_FATAL(pthread.exist());

	// 4) make executable for terminating
	auto pexecutable_terminator = make_object<executable_terminator>();

	// 5) create post_event_fd
	auto terminate_handle = this->_process_prepare_terminate(h_epoll, pexecutable_terminator);

	// 6) wait thread terminate
	pthread->wait();

	// 7) unregister post_event_fd
	this->_process_complete_terminate(h_epoll, terminate_handle);

	// 8) release the executables not executed (referenced by post_executable())
	{
		CONTAINER_EXECUTABLE_QUEUE queue_executable;
		{
			std::lock_guard lock(this->m_lockable_complete);
			queue_executable = std::move(this->m_queue_executable);
			this->m_queue_executable.clear();
		}
		for (auto& iter : queue_executable)
		{
			if (auto pexecutable = reinterpret_cast<Iexecutable*>(iter.data.ptr); pexecutable != nullptr)
				pexecutable->release();
		}
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool api::io_context_epoll_queued::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable == nullptr, false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.data.ptr = reinterpret_cast<void*>(_pexecutable);
		result.events = static_cast<uint32_t>(_completion_key);

		// 2) push한다.
		SCOPED_LOCK(this->m_lockable_complete)
		{
			// - Queueing한다.
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

bool api::io_context_epoll_queued::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable.empty(), false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.data.ptr = reinterpret_cast<void*>(_pexecutable.get());
		result.events = static_cast<uint32_t>(_completion_key);

		// 2) push한다.
		SCOPED_LOCK(this->m_lockable_complete)
		{
			// - Queueing한다.
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

void api::io_context_epoll_queued::pop_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
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

void api::io_context_epoll_queued::_fn_process_popup_epoll_event(api::io_context_epoll_queued* _pobject)
{
	// declare)
	object_ptr<api::io_context_epoll_queued> hold(_pobject);

	// 1) thread run
	_pobject->_process_popup_epoll_event();
}

void api::io_context_epoll_queued::_process_popup_epoll_event()
{
	// declare) 
	const int MAX_EVENTS = 1024;

	// declare) 
	epoll_event list_epoll_event[MAX_EVENTS];

	while(this->m_handle_epoll != INVALID_CGHANDLE_VALUE)
	{
		try
		{
			for (;;)
			{
				// 1) get epoll handle
				CGHANDLE handle_epoll = this->m_handle_epoll;

				// check) 
				BREAK_IF(handle_epoll == INVALID_CGHANDLE_VALUE);

				// 2) get epoll events
				auto count = ::epoll_wait(handle_epoll, list_epoll_event, MAX_EVENTS, -1);

				// check) 
				CONTINUE_IF(count == 0);

				// check) 
				CONTINUE_IF(count < 0);

				// declare)
				int count_signal = 0;

				// 3) post executable
				SCOPED_LOCK(this->m_lockable_complete)
				{
					// - reserve
					this->m_queue_executable.reserve(this->m_queue_executable.size() + count);

					// - push
					auto iter = list_epoll_event;
					auto iter_end = list_epoll_event + count;

					for (; iter < iter_end; ++iter)
					{
						iter->data.ptr = _process_get_executable(iter->data.u64);

						this->m_queue_executable.push_back(*reinterpret_cast<RESULT*>(iter));
					}

					// - get signal count
					count_signal = __select_min(this->m_count_wait, count);
				}

				// 4) signal
				for (; count_signal != 0; --count_signal)
				{
					this->m_cv_complete.notify_one();
				}
			}
		}
		catch (...)
		{

		}
	}
}

}
#endif