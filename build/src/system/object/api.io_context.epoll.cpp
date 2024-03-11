#include "pch.h"


#if defined(__ANDROID__) || defined(__linux__)

namespace CGDK
{

api::io_context_epoll::io_context_epoll() :
	m_handle_epoll(INVALID_CGHANDLE_VALUE),
	m_count_attached(0),
	m_count_wait(0)
{
}

api::io_context_epoll::~io_context_epoll() noexcept
{
	this->destroy();
}

bool api::io_context_epoll::create(int _hint_size) noexcept
{
	// check) 
	RETURN_IF(this->m_count_attached == true, false);


	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이 아니면 이미 만들어졌다는 뜻이다.
	RETURN_IF(this->m_handle_epoll != INVALID_CGHANDLE_VALUE, false);

	// 1) epoll객체를 생성한다.
	auto h_epoll = epoll_create(_hint_size);

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
		threads_epoll() : object::Nnameable("epoll event pump") {}
	};

	// 3) 
	auto pthread = make_object<threads_epoll>();

	// 4) begin thread
	pthread->begin_thread(_fn_process_popup_epoll_event, this);

	// 5) set
	this->m_pthread = std::move(pthread);

	// return)
	return true; 
}

CGHANDLE api::io_context_epoll::dispose() noexcept
{
// return)
	return this->m_handle_epoll;
}

result_code api::io_context_epoll::destroy() noexcept
{
	// 1) m_handle_epoll을 NULL로 바꾼다.
	auto h_epoll = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// check) m_handle_epoll이 INVALID_CGHANDLE_VALUE이면 생성되지 않았다는 의미이다.
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, INVALID_CGHANDLE_VALUE);

	// 2) release all executable
	auto* iter = this->m_manager_attached;
	auto* iter_end = this->m_manager_attached + BUCKET_COUNT;

	// declare)
	std::vector<NODE> vector_register;

	for (; iter != iter_end; ++iter)
	{
		scoped_unique_lock(iter->cs_register)
		{
			// check)
			CGASSERT_ERROR(vector_register.empty());

			// - swap
			vector_register.swap(iter->vector_register);
		}

		// check)
		CONTINUE_IF(vector_register.empty());

		// - _handle을 제거한다.
		for (auto& iter_nested: vector_register)
		{
			// declare) 
			::epoll_event evt;

			// - detach handle from epoll
			::epoll_ctl(h_epoll, EPOLL_CTL_DEL, iter_nested.handle, &evt);

			// - releasee executable
			(iter_nested.pexecutable)->release();
		}
		vector_register.clear();
	}

	// 3) get threas and wait terminate
	auto pthread = std::move(this->m_pthread);

	// check)
	CGASSERT_ERROR(pthread.exist());

	// 4) destroy epoll handle
	auto result = ::close(h_epoll);

	// check) 제대로 close를 하지 않았다면 false를 리턴한다.(성공했다면 0을 리턴한다.)
	RETURN_IF(result != 0, result_code(eRESULT::FAIL));

	// 5) wait thread terminate
	scoped_lock(this->m_lock_complete) // 진행중인 this->m_queue_executable이 있을 수 있으므로...
	{
		pthread->terminate_all();
	}

	// return)
	return result_code(eRESULT::SUCCESS);
}

bool api::io_context_epoll::attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// 1) register executable
	this->_process_register_handle(_handle, _pexecutable);

	// declare) 
	epoll_event evt;
	evt.events = _event;
	evt.data.ptr = reinterpret_cast<void*>(_handle);

	// 2) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_ADD, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다. "Could not add device fd to epoll instance. errno=%d", errno);
	if (result < 0)
	{
		// - unregister
		this->_process_unregister_handle(_handle);

		// return) 
		return false;
	}

	// 3) attach갯수를 증가시킴.
	++this->m_count_attached;

	// return)
	return true;
}

bool api::io_context_epoll::detach(CGHANDLE _handle) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// 1) unregister executable
	this->_process_unregister_handle(_handle);

	// declare) 
	epoll_event evt;

	// 2) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_DEL, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result != 0, false);

	// 3) attach갯수를 감소시킴.
	--this->m_count_attached;

	// return)
	return true;
}

bool api::io_context_epoll::modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable)  noexcept
{
	// check) 
	// 1) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// 2) _pxecutable을 update한다.
	if (_pexecutable != nullptr)
	{
		// - ...
		auto result = this->_process_update_handle(_handle, _pexecutable);

		// check) 
		RETURN_IF(result == false, false);
	}

	// declare) 
	epoll_event evt;
	evt.events = _event;
	evt.data.ptr = reinterpret_cast<void*>(_handle);

	// 3) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_MOD, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다.
	RETURN_IF(result != 0, false);

	// return)
	return true;
}

bool api::io_context_epoll::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안됀다.
	RETURN_IF(_pexecutable == nullptr, false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.data.ptr = reinterpret_cast<void*>(_pexecutable);
		result.events = static_cast<uint32_t>(_completion_key);

		// 2) push한다.
		scoped_lock(this->m_lock_complete)
		{
			// - Queuning한다.
			this->m_queue_executable.push_back(result);

			// - add_ref한다.
			_pexecutable->add_ref();
		}

		// 3) push후 notify
		m_cv_complete.notify_one();
	}
	catch(...)
	{
		// return)
		return false;
	}

	// return)
	return true;
}

bool api::io_context_epoll::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// check) _pexecutable이 nullptr이면 안됀다.
	RETURN_IF(_pexecutable.empty(), false);

	try
	{
		// 1)  make RESULT data
		RESULT result;
		result.data.ptr = reinterpret_cast<void*>(_pexecutable.get());
		result.events = static_cast<uint32_t>(_completion_key);

		// 2) push한다.
		scoped_lock(this->m_lock_complete)
		{
			// - Queuning한다.
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

void api::io_context_epoll::_process_register_handle(CGHANDLE _handle, Iexecutable* _pexecutable)
{
	// 1) get bucket
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	scoped_unique_lock(bucket.cs_register)
	{
		// 2) emplace_back
		bucket.vector_register.emplace_back(_handle, _pexecutable);

		// 3) add ref
		_pexecutable->add_ref();
	}
}

void api::io_context_epoll::_process_unregister_handle(CGHANDLE _handle) noexcept
{
	// 1) get bucket
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	scoped_unique_lock_named(bucket.cs_register, cs)
	{
		// 2) find handle node
		auto iter = bucket.vector_register.begin();
		auto iter_end = bucket.vector_register.end();

		for (; iter!=iter_end; ++iter)
		{
			if ((*iter).handle == _handle)
			{
				auto pexecutable = (*iter).pexecutable;

				// 3) remove handle node
				bucket.vector_register.erase(iter);

				// 4) lock leave
				scoped_lock_leave(cs);

				// 5) release
				pexecutable->release();

				// return) 
				return;
			}
		}
	}
}

void* api::io_context_epoll::_process_get_executable(CGHANDLE _handle) const noexcept
{
	// 1) bucket을 얻는다.t
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	scoped_shared_lock(bucket.cs_register)
	{
		auto iter = bucket.vector_register.begin();
		auto iter_end = bucket.vector_register.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter).handle == _handle)
			{
				// - addref
				(*iter).pexecutable->add_ref();

				// break
				return (*iter).pexecutable;
			}
		}
	}

	// return)
	return nullptr;
}

bool api::io_context_epoll::_process_update_handle(CGHANDLE _handle, Iexecutable* _pexecutable)
{
	// 1) bucket을 얻는다.t
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	scoped_unique_lock_named(bucket.cs_register, cs)
	{
		// 2) find node
		auto iter = bucket.vector_register.begin();
		auto iter_end = bucket.vector_register.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter).handle == _handle)
			{
				// 3)  get old executablee
				auto pexecutable_old = (*iter).pexecutable;

				// 4) set new executable
				(*iter).pexecutable = _pexecutable;

				// 5) add ref
				_pexecutable->add_ref();

				// - scoped_lock leave
				scoped_lock_leave(cs);

				// 6) release old executable
				pexecutable_old->release();

				// return)
				return true;
			}
		}
	}

	// return)
	return false;
}

void api::io_context_epoll::pop_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
{
	conditional_lock(this->m_lock_complete, lock_wait)
	{
		// 1) 갯수가 0개면 wait...
		if (this->m_queue_executable.size() == 0 && _ticks_wait != chrono::tick::duration::zero())
		{
			// - 
			++m_count_wait;

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

void api::io_context_epoll::_fn_process_popup_epoll_event(api::io_context_epoll* _pobject)
{
	// declare)
	object_ptr<api::io_context_epoll> hold(_pobject);

	// 1) thread run
	_pobject->_process_popup_epoll_event();
}

void api::io_context_epoll::_process_popup_epoll_event()
{
	// declare) 
	const int MAX_EVENTS = 1024;

	// declare) 
	epoll_event list_epoll_vent[MAX_EVENTS];

	for (;;)
	{
		// 1) get epoll handle
		CGHANDLE handle_epoll = this->m_handle_epoll;

		// check) 
		BREAK_IF(handle_epoll == INVALID_CGHANDLE_VALUE);

		// 2) get epoll events
		auto count = ::epoll_wait(handle_epoll, list_epoll_vent, MAX_EVENTS, -1);

		// check) 
		CONTINUE_IF(count == 0);

		// check) 
		if (count < 0)
		{
			// check) 
			BREAK_IF(errno == EINVAL);

			// continue)
			continue; 
		}

		// declar)
		int count_signal = 0;

		// 3) post eexecutable
		scoped_lock(this->m_lock_complete)
		{
			// - reserve
			this->m_queue_executable.reserve(this->m_queue_executable.size() + count);

			// - push
			auto iter = list_epoll_vent;
			auto iter_end = list_epoll_vent + count;

			for (; iter < iter_end; ++iter)
			{
				iter->data.ptr = _process_get_executable(iter->data.fd);

				this->m_queue_executable.push_back(*reinterpret_cast<RESULT*>(iter));
			}

			// - get signal count
			count_signal = __select_min(m_count_wait, count);
		}

		// 4) signal
		for (; count_signal != 0; --count_signal)
		{
			this->m_cv_complete.notify_one();
		}
	}
}

}
#endif