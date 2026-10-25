#include "pch.h"
#include "platform.h"

#if defined(__ANDROID__) || defined(__linux__)
#include <sys/eventfd.h>
#include <iostream>
#include <algorithm>
#include <exception>
#include <thread>

namespace CGDK
{

api::io_context_epoll_direct::io_context_epoll_direct()
{
}

class api::io_context_epoll_direct::executable_post : virtual public Iexecutable, virtual public object::Idestroyable
{
public:
			CGHANDLE			m_h_epoll{ INVALID_CGHANDLE_VALUE };
			api::io_context_epoll_direct* m_pthis{ nullptr };

			CGHANDLE			m_event_fd_post{ INVALID_CGHANDLE_VALUE };
			CONTAINER_EXECUTABLE_QUEUE	m_queue_executable;
			bool				m_is_signaled{ false };			// - eventfd에 신호했다. queue가 빌 때까지 다시 쓰지 않는다. (lock 안에서 읽고 쓴다)
			std::atomic<int>	m_count_signaling{ 0 };			// - lock 밖에서 eventfd에 쓰고 있는 post() 수 (destroy()는 0이 된 뒤 fd를 닫는다)
	mutable	std::mutex			m_lockable_complete;

	// - 한번에 꺼내 실행하는 최대 수 (queue가 길 때만 여러 개를 꺼낸다)
	static	constexpr std::size_t MAX_EXECUTE_BATCH = 16;

public:
			void				initialize(api::io_context_epoll_direct* _pthis, CGHANDLE _h_epoll);
	virtual	result_code			destroy() noexcept override;
			size_t				get_ququed_count() const noexcept { std::lock_guard lock(this->m_lockable_complete); return this->m_queue_executable.size();}


			bool				post(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable, bool _is_adopt) noexcept;
			// - batch handoff (executor::io_context::BATCH_HANDOFF) : the executables of a batch after a blocking one go back to the front of the queue
	static	void				_handoff(executor::io_context::BATCH_HANDOFF& _batch) noexcept;
	virtual	intptr_t process_execute(intptr_t _result, size_t _param) override;
};


void api::io_context_epoll_direct::executable_post::initialize(api::io_context_epoll_direct* _pthis, CGHANDLE _h_epoll)
{
	// declare)
	CGHANDLE event_fd_post;

	// 1) create post event_fd
	//    - semaphore가 아니다 : post()는 queue가 비어 있다가 처음 들어올 때만 쓰고, process_execute()는 queue가 비었을 때 한번 읽어 0으로 만든다.
	//      (level-triggered : queue에 남은 것이 있는 동안은 계속 신호 상태다)
	event_fd_post = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

	// 2) register executable
	const auto event_data = _pthis->_process_register_handle(event_fd_post, this);

	// declare)
	epoll_event evt;
	evt.events = EPOLLIN;
	evt.data.u64 = event_data;

	try
	{
		// 4) EPoll과 Socket을 연결한다.
		auto result = ::epoll_ctl(_h_epoll, EPOLL_CTL_ADD, event_fd_post, &evt);

		// check) 
		THROW_IF(result < 0, throwable::failure(eRESULT::FAIL, 0, "fail to add epoll event handle (epoll_ctl)"sv));

		// 5) store
		this->m_event_fd_post = event_fd_post;
		this->m_h_epoll = _h_epoll;
		this->m_pthis = _pthis;
	}
	catch (...)
	{
		// - rollback
		_pthis->_process_unregister_handle(event_fd_post);

		// reraise) 
		throw;
	}

}

result_code api::io_context_epoll_direct::executable_post::destroy() noexcept
{
	// 1) m_event_fd_post를 lock 안에서 비운다.
	//    (post()는 lock 안에서 fd를 확인하고 쓴다 : 닫힌(다른 곳에서 다시 쓰일 수 있는) fd에 쓰지 않도록)
	CGHANDLE event_fd_post;
	{
		std::lock_guard lock(this->m_lockable_complete);
		event_fd_post = this->m_event_fd_post;
		this->m_event_fd_post = INVALID_CGHANDLE_VALUE;
	}

	// 2) lock 밖에서 eventfd에 쓰는 중인 post()가 끝나기를 기다린다. (닫은 fd에 쓰지 않도록)
	//    (fd를 비운 뒤에는 새로 쓰기 시작하는 post()가 없다 : 수는 줄기만 한다)
	while (this->m_count_signaling.load(std::memory_order_acquire) != 0)
		std::this_thread::yield();

	if (event_fd_post != INVALID_CGHANDLE_VALUE)
	{
		// - unregister
		this->m_pthis->_process_unregister_handle(event_fd_post);

		// declare)
		epoll_event evt;

		// - epoll과 socket을 연결한다.
		::epoll_ctl(this->m_h_epoll, EPOLL_CTL_DEL, event_fd_post, &evt);

		// - close eventfd
		::close(event_fd_post);
	}

	// - release the executables not executed (referenced by post())
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

	return eRESULT::SUCCESS;
}

bool api::io_context_epoll_direct::executable_post::post(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, Iexecutable* _pexecutable, bool _is_adopt) noexcept
{
	// - _is_adopt : 호출한 쪽의 참조를 넘겨받는다. (add_ref하지 않는다. 실패하면 참조는 호출한 쪽에 남는다)
	//   (예외를 던지지 않는다 : noexcept인 post_executable()에서 불린다)

	// 1)  make RESULT data
	RESULT result;
	result.data.ptr = reinterpret_cast<void*>(_pexecutable);
	result.events = static_cast<uint32_t>(_completion_key);

	// declare) 신호할 eventfd (lock 밖에서 쓴다)
	CGHANDLE event_fd_signal = INVALID_CGHANDLE_VALUE;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_complete);

		// check) destroy되었으면 실패 (fd는 lock 안에서 확인한다)
		RETURN_IF(this->m_event_fd_post == INVALID_CGHANDLE_VALUE, false);

		// 2) queuing
		try
		{
			this->m_queue_executable.push_back(result);
		}
		catch (...)
		{
			return false;
		}

		// 3) add_ref
		if (_is_adopt == false)
			_pexecutable->add_ref();

		// 4) queue가 비어 있다가 처음 들어온 것일 때만 신호한다. (이미 신호된 상태면 queue가 빌 때까지 신호가 남아 있다)
		//    - destroy()가 fd를 닫지 않도록 쓰는 동안 m_count_signaling을 올려 둔다. (lock 안에서 올린다 : destroy()와 순서가 맞는다)
		if (this->m_is_signaled == false)
		{
			this->m_is_signaled = true;
			event_fd_signal = this->m_event_fd_post;
			this->m_count_signaling.fetch_add(1, std::memory_order_relaxed);
		}
	}

	// 5) notify (lock 밖에서 쓴다 : 다른 thread의 post()가 이 syscall 동안 lock을 기다리지 않도록)
	if (event_fd_signal != INVALID_CGHANDLE_VALUE)
	{
		if (::eventfd_write(event_fd_signal, 1) < 0)
		{
			// - 실패 (non-blocking eventfd는 넘칠 때만 실패한다 : 일어나지 않는다)
			//   항목은 queue에 남아 있다. 다음 post()가 다시 신호하도록 한다.
			std::lock_guard lock(this->m_lockable_complete);
			this->m_is_signaled = false;
		}

		this->m_count_signaling.fetch_sub(1, std::memory_order_release);
	}

	// return)
	return true;
}

intptr_t api::io_context_epoll_direct::executable_post::process_execute(intptr_t _result, size_t _param)
{
	// declare)
	Iexecutable* array_executable[MAX_EXECUTE_BATCH];
	std::size_t count_executable = 0;

	SCOPED_LOCK(this->m_lockable_complete)
	{
		// check) destroy되었으면 끝낸다.
		//    (destroy()는 같은 lock 안에서 fd를 비우고 lock 밖에서 닫는다 : 닫힌(다른 곳에서 다시 쓰일 수 있는) fd를 읽지 않도록. EFD_NONBLOCK)
		RETURN_IF(this->m_event_fd_post == INVALID_CGHANDLE_VALUE, 0);

		// 1) 꺼낼 수
		//    - queue가 짧으면 1개만 꺼낸다. eventfd는 queue가 빌 때까지 신호 상태이므로 다른 thread들이 나머지를 나눠 실행한다.
		//    - queue가 길면 여러 개를 한번에 꺼낸다. (epoll_wait와 lock의 횟수를 줄인다)
		const auto size_queue = this->m_queue_executable.size();
		count_executable = (size_queue == 0) ? 0 : std::clamp<std::size_t>(size_queue / 8, 1, MAX_EXECUTE_BATCH);

		// 2) pop
		for (std::size_t i = 0; i < count_executable; ++i)
		{
			array_executable[i] = reinterpret_cast<Iexecutable*>(this->m_queue_executable.front().data.ptr);
			this->m_queue_executable.pop_front();
		}

		// 3) queue가 비었으면 eventfd를 읽어 신호를 지운다.
		//    - lock 안에서 지운다 : 이후의 post()는 m_is_signaled == false를 보고 다시 신호한다. (신호를 잃지 않는다)
		//    - 이미 0이면 EAGAIN이다. (먼저 꺼낸 thread가 지웠거나 post()가 아직 쓰기 전)
		if (this->m_queue_executable.empty())
		{
			eventfd_t temp_eventfd;
			::eventfd_read(this->m_event_fd_post, &temp_eventfd);

			this->m_is_signaled = false;
		}
	}

	// 4) execute
	//    - 하나가 exception을 던져도 나머지는 실행하고 release한다. (첫 exception은 끝난 뒤 다시 던진다)
	std::exception_ptr pexception;

	// - the rest of the batch goes back to the queue when one blocks (executor::io_context::BATCH_HANDOFF : the other workers take it)
	executor::io_context::BATCH_HANDOFF batch_handoff{ &executable_post::_handoff, this, array_executable, static_cast<int>(count_executable), 0, nullptr };
	const bool is_batch_handoff = (count_executable > 1);
	if (is_batch_handoff)
		executor::io_context::batch_handoff_begin(batch_handoff);

	for (std::size_t i = 0; i < count_executable; ++i)
	{
		batch_handoff.index = static_cast<int>(i);

		// - get executable
		auto pexecutable = array_executable[i];

		// check)
		CONTINUE_IF(pexecutable == nullptr);

		try
		{
			// - release the reference added by post() after the execution (also on exception)
			struct release_on_exit { Iexecutable* p; ~release_on_exit() noexcept { p->release(); } } release_executable{ pexecutable };

			// - send defer (auto) : the consecutive sends of the posted executable to one socket are sent at once
			send_defer::auto_scope scope_send_defer;

			pexecutable->process_execute(_result, _param);
		}
		catch (...)
		{
			if (!pexception)
				pexception = std::current_exception();
		}
	}

	// - the batch ended
	if (is_batch_handoff)
		executor::io_context::batch_handoff_end(batch_handoff);

	// reraise)
	if (pexception)
		std::rethrow_exception(pexception);

	// return)
	return 0;
}

void api::io_context_epoll_direct::executable_post::_handoff(executor::io_context::BATCH_HANDOFF& _batch) noexcept
{
	// - the executables after the blocking one go back to the front of the queue (in their order) and are cleared in the batch
	auto* pthis = static_cast<executable_post*>(_batch.pobject);
	auto** parray = static_cast<Iexecutable**>(_batch.presults);

	// declare)
	CGHANDLE event_fd_signal = INVALID_CGHANDLE_VALUE;

	{
		// lock)
		std::lock_guard lock(pthis->m_lockable_complete);

		// check) destroyed : they stay in the batch
		RETURN_IF(pthis->m_event_fd_post == INVALID_CGHANDLE_VALUE);

		// - from the back (push_front keeps their order)
		bool is_queued = false;
		for (int i = _batch.count - 1; i > _batch.index; --i)
		{
			CONTINUE_IF(parray[i] == nullptr);

			RESULT result;
			result.data.ptr = reinterpret_cast<void*>(parray[i]);
			result.events = 0;

			try
			{
				pthis->m_queue_executable.push_front(result);
			}
			catch (...)
			{
				break;
			}

			// - the reference of post() moves to the queue
			parray[i] = nullptr;
			is_queued = true;
		}

		// check)
		RETURN_IF(is_queued == false);

		// - signal (the queue is not empty : level triggered until a pop empties it)
		if (pthis->m_is_signaled == false)
		{
			pthis->m_is_signaled = true;
			event_fd_signal = pthis->m_event_fd_post;
			pthis->m_count_signaling.fetch_add(1, std::memory_order_relaxed);
		}
	}

	// - notify (out of the lock)
	if (event_fd_signal != INVALID_CGHANDLE_VALUE)
	{
		::eventfd_write(event_fd_signal, 1);
		pthis->m_count_signaling.fetch_sub(1, std::memory_order_release);
	}
}


api::io_context_epoll_direct::~io_context_epoll_direct() noexcept
{
	this->destroy();
}

bool api::io_context_epoll_direct::create(int _hint_size) noexcept
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

	// declare) 
	auto pexecutable_post = make_own<executable_post>();

	// 2) initialize post_executable
	pexecutable_post->initialize(this, h_epoll);

	// 3) 값을 설정한다.
	this->m_handle_epoll = h_epoll;
	this->m_count_attached = 0;
	this->m_pexecutable_post = std::move(pexecutable_post);

	// return)
	return true; 
}

CGHANDLE api::io_context_epoll_direct::dispose() noexcept
{
	// 1) dispose 
	auto handle_epoll = io_context_Nepoll::dispose();

	// check)
	RETURN_IF(handle_epoll == INVALID_CGHANDLE_VALUE, m_handle_epoll);


	// return)
	return handle_epoll;
}

result_code api::io_context_epoll_direct::destroy() noexcept
{
	// 1) m_handle_epoll을 NULL로 바꾼다.
	auto h_epoll = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// check) m_handle_epoll이 INVALID_CGHANDLE_VALUE이면 생성되지 않았다는 의미이다.
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, INVALID_CGHANDLE_VALUE);

	// 2) release all executable
	this->_process_remove_all_handle(h_epoll);

	// 3) make executable for terminating
	auto pexecutable_terminator = make_object<executable_terminator>();

	// 4) create post_event_fd
	auto terminate_handle = this->_process_prepare_terminate(h_epoll, pexecutable_terminator);

	// 5) destroy event post
	//    - 객체는 해제하지 않는다. (post_executable()은 참조 count 없이 raw pointer로 쓴다 : io_context가 소멸될 때 해제된다)
	this->m_pexecutable_post.destroy();

	// 6) unregister post_event_fd
	this->_process_complete_terminate(h_epoll, terminate_handle);

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool api::io_context_epoll_direct::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안 된다.
	RETURN_IF(_pexecutable == nullptr, false);

	// 1) get executable_post
	//    - raw pointer (모든 thread가 post마다 같은 참조 count를 올리고 내리지 않도록)
	//      m_pexecutable_post는 destroy()에서 해제하지 않고 io_context가 소멸될 때 해제한다.
	auto pexecutable_post = this->m_pexecutable_post.get();

	// check)
	RETURN_IF(pexecutable_post == nullptr, false);

	// 2) push한다. (add_ref)
	return pexecutable_post->post(_bytes_transferred, _completion_key, _pexecutable, false);
}

bool api::io_context_epoll_direct::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check)
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check)
	auto pexecutable = _pexecutable.get();
	RETURN_IF(pexecutable == nullptr, false);

	// 1) get executable_post (raw pointer : 위와 같다)
	auto pexecutable_post = this->m_pexecutable_post.get();

	// check)
	RETURN_IF(pexecutable_post == nullptr, false);

	// 2) push한다. (넘겨받은 참조를 그대로 넘긴다 : add_ref/release 한 쌍을 줄인다. 실패하면 참조는 _pexecutable에 남는다)
	RETURN_IF(pexecutable_post->post(_bytes_transferred, _completion_key, pexecutable, true) == false, false);

	// 3) 참조는 실행하는 thread로 넘어갔다. (이미 실행되고 release되었을 수 있으므로 객체는 건드리지 않고 포인터만 비운다)
	_pexecutable.detach();

	// return)
	return true;
}

void api::io_context_epoll_direct::pop_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
{
	// declare)
	epoll_event event_epoll;

	// 1) get epoll event
	const auto count = this->pop_events(&event_epoll, 1, _ticks_wait);

	// check) 0이면 timeout이다. (event_epoll은 채워지지 않았다 : _result는 invalid로 남는다)
	RETURN_IF(count <= 0);

	// 2) get executable (add_ref)
	this->get_result(event_epoll, _result);
}

int api::io_context_epoll_direct::pop_events(epoll_event* _events, int _max_events, chrono::tick::duration _ticks_wait) noexcept
{
	// 1) get epoll handle
	CGHANDLE handle_epoll = this->m_handle_epoll;

	// check)
	RETURN_IF(handle_epoll == INVALID_CGHANDLE_VALUE, 0);

	// 2) get wait time
	int tick_millisecond_wait = (_ticks_wait == chrono::tick::duration::max()) ? -1 : static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(_ticks_wait).count());

	// 3) get epoll events
	const auto count = ::epoll_wait(handle_epoll, _events, _max_events, tick_millisecond_wait);

	// return) 0 : timeout, 실패(EINTR, 닫힌 epoll handle)
	return (count > 0) ? count : 0;
}

void api::io_context_epoll_direct::get_result(const epoll_event& _event, RESULT& _result) const noexcept
{
	// 1) event flag
	_result.events = _event.events;

	// 2) get executable (add_ref. 닫힌 handle의 event면 nullptr)
	_result.data.ptr = this->_process_get_executable(_event.data.u64);
}

[[nodiscard]] size_t api::io_context_epoll_direct::queued_count() const noexcept 
{
	return this->m_pexecutable_post->get_ququed_count();
}


}
#endif