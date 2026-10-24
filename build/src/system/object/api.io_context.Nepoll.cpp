#include "pch.h"
#include "platform.h"

#if defined(__ANDROID__) || defined(__linux__)
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <iostream>

namespace CGDK
{

namespace
{
	// - EPIOCSPARAMS / EPIOCGPARAMS of the kernel 6.9+ (defined here : the headers of an older kernel/glibc have none. the kernel answers ENOTTY before 6.9)
	struct sEPOLL_PARAMS
	{
		uint32_t	busy_poll_usecs;
		uint16_t	busy_poll_budget;
		uint8_t		prefer_busy_poll;
		uint8_t		pad;
	};
	static_assert(sizeof(sEPOLL_PARAMS) == 8);

	constexpr unsigned long CGDK_EPIOCSPARAMS = _IOW(0x8A, 0x01, sEPOLL_PARAMS);
	constexpr unsigned long CGDK_EPIOCGPARAMS = _IOR(0x8A, 0x02, sEPOLL_PARAMS);
}

bool api::io_context_Nepoll::set_busy_poll(uint32_t _usecs, uint16_t _budget, bool _prefer) noexcept
{
	// check) created?
	const CGHANDLE h_epoll = this->m_handle_epoll;
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, false);

	// 1) set (the kernel checks : usecs <= S32_MAX, budget > 64 needs CAP_NET_ADMIN)
	sEPOLL_PARAMS params{ _usecs, _budget, static_cast<uint8_t>(_prefer ? 1 : 0), 0 };

	if (::ioctl(h_epoll, CGDK_EPIOCSPARAMS, &params) != 0)
	{
		const int error_code = errno;

		if (error_code == ENOTTY || error_code == EINVAL)
			LOG_WARNING << "(warn) io_context: busy poll of epoll_wait() is not supported by this kernel (6.9+ : EPIOCSPARAMS). sysctl net.core.busy_poll applies instead"sv;
		else
			LOG_WARNING << "(warn) io_context: fail to set the busy poll of epoll_wait() (usecs:"sv << _usecs << ", budget:"sv << _budget << ", prefer:"sv << _prefer << ") error_code:"sv << error_code << (error_code == EPERM ? " (a budget over 64 needs CAP_NET_ADMIN)"sv : ""sv);

		return false;
	}

	// return)
	return true;
}

bool api::io_context_Nepoll::get_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) const noexcept
{
	// 1) default
	_usecs = 0;
	_budget = 0;
	_prefer = false;

	// check) created?
	const CGHANDLE h_epoll = this->m_handle_epoll;
	RETURN_IF(h_epoll == INVALID_CGHANDLE_VALUE, false);

	// 2) get
	sEPOLL_PARAMS params{};
	RETURN_IF(::ioctl(h_epoll, CGDK_EPIOCGPARAMS, &params) != 0, false);

	_usecs = params.busy_poll_usecs;
	_budget = params.busy_poll_budget;
	_prefer = (params.prefer_busy_poll != 0);

	// return)
	return true;
}

bool api::io_context_Nepoll::is_busy_poll_available() noexcept
{
	// - once : EPIOCGPARAMS on a temporary epoll (ENOTTY before 6.9)
	static const bool s_is_available = []()
	{
		const int h_epoll = ::epoll_create1(EPOLL_CLOEXEC);
		if (h_epoll < 0)
			return false;

		sEPOLL_PARAMS params{};
		const bool result = (::ioctl(h_epoll, CGDK_EPIOCGPARAMS, &params) == 0);
		::close(h_epoll);
		return result;
	}();

	return s_is_available;
}

api::io_context_Nepoll::io_context_Nepoll() :
	m_handle_epoll(INVALID_CGHANDLE_VALUE),
	m_count_attached(0)
{
}

api::io_context_Nepoll::~io_context_Nepoll() noexcept
{
	this->destroy();
}

bool api::io_context_Nepoll::create(int _hint_size) noexcept
{
	// check) 
	if(this->m_count_attached == true)
		return false;

	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이 아니면 이미 만들어졌다는 뜻이다.
	if(this->m_handle_epoll != INVALID_CGHANDLE_VALUE)
		return false;

	// 1) epoll객체를 생성한다.
	static_cast<void>(_hint_size);
	auto h_epoll = epoll_create1(EPOLL_CLOEXEC);		// - EPOLL_CLOEXEC : not inherited by fork/exec (size hint is ignored by linux 2.6.8+)

	// check) epoll를 적절하게 생성했는가?
	if(h_epoll == INVALID_CGHANDLE_VALUE)
		return false;

	// 3) 값을 설정한다.
	this->m_handle_epoll = h_epoll;
	this->m_count_attached = 0;

	// return)
	return true; 
}

CGHANDLE api::io_context_Nepoll::dispose() noexcept
{
	// 1) iocp handle
	auto h_epoll = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// check) return if iocp handle is not exist
	if(h_epoll == INVALID_CGHANDLE_VALUE)
		return h_epoll;

	// return)
	return h_epoll;
}

result_code api::io_context_Nepoll::destroy() noexcept
{
	// 1) m_handle_epoll을 NULL로 바꾼다.
	auto h_epoll = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// check) m_handle_epoll이 INVALID_CGHANDLE_VALUE이면 생성되지 않았다는 의미이다.
	if(h_epoll == INVALID_CGHANDLE_VALUE)
		return result_code(eRESULT::FAIL);

	// 2) release all executable
	this->_process_remove_all_handle(h_epoll);

	// 3) make executable for terminating
	auto pexecutable_terminator = make_object<executable_terminator>();

	// 4) create post_event_fd
	auto terminate_handle = this->_process_prepare_terminate(h_epoll, pexecutable_terminator);

	// 5) unregister post_event_fd
	this->_process_complete_terminate(h_epoll, terminate_handle);

	// return)
	return result_code(eRESULT::SUCCESS);
}

CGHANDLE api::io_context_Nepoll::_process_prepare_terminate(CGHANDLE _handle_epoll, executable_terminator* _pexecutable)
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE 일 수는 없다.
	CGASSERT(_handle_epoll != INVALID_CGHANDLE_VALUE, INVALID_CGHANDLE_VALUE);

	// 1) create terminate event_fd
	CGHANDLE execution_id = eventfd(0, EFD_SEMAPHORE | EFD_NONBLOCK | EFD_CLOEXEC);

	// 2) attach terminate event_fd
	{
		// - register executable
		const auto event_data = this->_process_register_handle(execution_id, _pexecutable);

		// declare)
		epoll_event evt;
		evt.events = EPOLLIN;
		evt.data.u64 = event_data;

		// - EPoll과 Socket을 연결한다.
		auto result = ::epoll_ctl(_handle_epoll, EPOLL_CTL_ADD, execution_id, &evt);

		// check) 제대로 연결되었는지 확인한다. "Could not add device fd to epoll instance. errno=%d ", errno);
		if (result < 0)
		{
			// - rollback
			this->_process_unregister_handle(execution_id);

			// return) 
			return INVALID_CGHANDLE_VALUE;
		}
	}

	// 3) post executable
	{
		auto result = ::eventfd_write(execution_id, 1);

		if (result < 0)
		{
			this->_process_unregister_handle(execution_id);
			epoll_event evt;
			::epoll_ctl(_handle_epoll, EPOLL_CTL_DEL, execution_id, &evt);
			execution_id = INVALID_CGHANDLE_VALUE;
		}
	}

	return execution_id;
}

void api::io_context_Nepoll::_process_complete_terminate(CGHANDLE _handle_epoll, CGHANDLE _execution_handle) noexcept
{
	// - unregister
	this->_process_unregister_handle(_execution_handle);

	// declare) 
	epoll_event evt;

	// 2) EPoll과 Socket을 연결한다.
	::epoll_ctl(_handle_epoll, EPOLL_CTL_DEL, _execution_handle, &evt);

	// 3) destroy epoll handle
	::close(_handle_epoll);
}

bool api::io_context_Nepoll::attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	if(this->m_handle_epoll == INVALID_CGHANDLE_VALUE)
		return false;

	// 1) register executable
	const auto event_data = this->_process_register_handle(_handle, _pexecutable);

	// declare)
	epoll_event evt;
	evt.events = _event;
	evt.data.u64 = event_data;

	// 2) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_ADD, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다. "Could not add device fd to epoll instance. errno=%d ", errno);
	if (result < 0)
	{
		// - unregister
		this->_process_unregister_handle(_handle);

		// return) 
		return false;
	}

	// 3) attach개수를 증가시킴.
	++this->m_count_attached;

	// return)
	return true;
}

bool api::io_context_Nepoll::detach(CGHANDLE _handle) noexcept
{
	// check) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	if(this->m_handle_epoll == INVALID_CGHANDLE_VALUE)
		return false;

	// 1) unregister executable
	this->_process_unregister_handle(_handle);

	// declare) 
	epoll_event evt;

	// 2) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_DEL, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다.
	if(result != 0)
		return false;

	// 3) attach개수를 감소시킴.
	--this->m_count_attached;

	// return)
	return true;
}

bool api::io_context_Nepoll::modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable)  noexcept
{
	// check) 
	// 1) m_handle_epoll가 INVALID_CGHANDLE_VALUE이어서는 안된다.
	if(this->m_handle_epoll == INVALID_CGHANDLE_VALUE)
		return false;

	// declare)
	uint64_t event_data;

	// 2) _pxecutable을 update한다.
	//    - nullptr이면 등록된 executable을 그대로 쓴다. (재등록마다 lock/add_ref/release를 하지 않는다)
	//    - 등록된 것과 같은 executable이면 참조 count를 바꾸지 않는다.
	const auto result_update = (_pexecutable != nullptr) ? this->_process_update_handle(_handle, _pexecutable, event_data) : this->_process_get_event_data(_handle, event_data);

	// check) 등록되지 않은 handle
	if(result_update == false)
		return false;

	// declare)
	epoll_event evt;
	evt.events = _event;
	evt.data.u64 = event_data;

	// 3) EPoll과 Socket을 연결한다.
	auto result = ::epoll_ctl(this->m_handle_epoll, EPOLL_CTL_MOD, _handle, &evt);

	// check) 제대로 연결되었는지 확인한다.
	if(result != 0)
		return false;

	// return)
	return true;
}

uint64_t api::io_context_Nepoll::_process_register_handle(CGHANDLE _handle, Iexecutable* _pexecutable)
{
	// 1) bucket을 얻는다.t
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	// 2) generation (0은 쓰지 않는다. bit 31은 쓰지 않는다 : io_uring_completion이 user_data의 bit 63으로 poll과 op를 구분한다)
	auto generation = (this->m_generation.fetch_add(1, std::memory_order_relaxed) + 1) & 0x7fffffffu;
	if (generation == 0)
		generation = (this->m_generation.fetch_add(1, std::memory_order_relaxed) + 1) & 0x7fffffffu;

	// 3) push_back
	SCOPED_UNIQUE_LOCK(bucket.cs_register)
	{
		// - ...
		bucket.vector_register.push_back(NODE{ _handle, _pexecutable, generation, 0 });

		// - add reef
		_pexecutable->add_ref();
	}

	// return) epoll_event.data.u64
	return _make_event_data(_handle, generation);
}

void api::io_context_Nepoll::_process_unregister_handle(CGHANDLE _handle) noexcept
{
	// 1) bucket을 얻는다.
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	SCOPED_UNIQUE_LOCK_NAMED(bucket.cs_register, cs)
	{
		// 2) _handle을 제거한다.
		auto iter = bucket.vector_register.begin();
		auto iter_end = bucket.vector_register.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter).handle == _handle)
			{
				auto pexecutable = (*iter).pexecutable;

				// - erase
				bucket.vector_register.erase(iter);

				// - lock leave
				SCOPED_LOCK_LEAVE(cs);

				// - add reef
				pexecutable->release();

				// return) 
				return;
			}
		}
	}
}

void api::io_context_Nepoll::_process_remove_all_handle(CGHANDLE _handle_epoll) noexcept
{
	auto* iter = this->m_manager_attached;
	auto* iter_end = this->m_manager_attached + BUCKET_COUNT;

	// declare)
	std::vector<NODE> vector_register;

	for (; iter != iter_end; ++iter)
	{
		SCOPED_UNIQUE_LOCK(iter->cs_register)
		{
			// check)
			CGASSERT_ERROR(vector_register.empty());

			// - swap
			vector_register.swap(iter->vector_register);
		}

		// check)
		if(vector_register.empty())
			continue;

		// - _handle을 제거한다.
		for (auto& iter_nested : vector_register)
		{
			// declare) 
			::epoll_event evt;

			// - detach handle from epoll
			::epoll_ctl(_handle_epoll, EPOLL_CTL_DEL, iter_nested.handle, &evt);

			// - releasee executable
			(iter_nested.pexecutable)->release();
		}
		vector_register.clear();
	}

	this->m_count_attached = 0;

}

Iexecutable* api::io_context_Nepoll::_process_get_executable(uint64_t _event_data) const noexcept
{
	// 1) handle과 generation
	const auto handle = _get_event_handle(_event_data);
	const auto generation = _get_event_generation(_event_data);

	// 2) bucket을 얻는다.
	auto& bucket = this->m_manager_attached[handle & BUCKET_MASK];

	SCOPED_SHARED_LOCK(bucket.cs_register)
	{
		for (const auto& iter : bucket.vector_register)
		{
			if (iter.handle == handle)
			{
				// check) 닫힌 handle의 event다. (같은 번호로 새로 등록된 것에 전달하지 않는다)
				if(iter.generation != generation)
					return nullptr;

				// - addref (실행한 뒤 release된다. 실행 중 detach되어도 객체는 남아 있다)
				iter.pexecutable->add_ref();

				// return)
				return iter.pexecutable;
			}
		}
	}

	// return)
	return nullptr;
}

bool api::io_context_Nepoll::_process_get_event_data(CGHANDLE _handle, uint64_t& _event_data) const noexcept
{
	// 1) bucket을 얻는다.
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	SCOPED_SHARED_LOCK(bucket.cs_register)
	{
		for (const auto& iter : bucket.vector_register)
		{
			if (iter.handle == _handle)
			{
				// - epoll_event.data.u64
				_event_data = _make_event_data(_handle, iter.generation);

				// return)
				return true;
			}
		}
	}

	// return)
	return false;
}

bool api::io_context_Nepoll::_process_set_event_mask(CGHANDLE _handle, uint32_t _events) noexcept
{
	// 1) bucket
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	SCOPED_UNIQUE_LOCK(bucket.cs_register)
	{
		for (auto& iter : bucket.vector_register)
		{
			if (iter.handle == _handle)
			{
				iter.events = _events;
				return true;
			}
		}
	}

	// return)
	return false;
}

bool api::io_context_Nepoll::_process_get_event_mask(CGHANDLE _handle, uint64_t& _event_data, uint32_t& _events) const noexcept
{
	// 1) bucket
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	SCOPED_SHARED_LOCK(bucket.cs_register)
	{
		for (const auto& iter : bucket.vector_register)
		{
			if (iter.handle == _handle)
			{
				_event_data = _make_event_data(_handle, iter.generation);
				_events = iter.events;
				return true;
			}
		}
	}

	// return)
	return false;
}

bool api::io_context_Nepoll::_process_update_handle(CGHANDLE _handle, Iexecutable* _pexecutable, uint64_t& _event_data) noexcept
{
	// 1) bucket을 얻는다.t
	auto& bucket = this->m_manager_attached[_handle & BUCKET_MASK];

	SCOPED_UNIQUE_LOCK_NAMED(bucket.cs_register, cs)
	{
		auto iter = bucket.vector_register.begin();
		auto iter_end = bucket.vector_register.end();

		for (; iter != iter_end; ++iter)
		{
			if ((*iter).handle == _handle)
			{
				// - epoll_event.data.u64 (generation은 등록 단위다 : executable을 바꿔도 그대로)
				_event_data = _make_event_data(_handle, (*iter).generation);

				// check) 같은 executable이면 바꿀 것이 없다.
				if((*iter).pexecutable == _pexecutable)
					return true;

				// - get old executable
				auto pexecutable_old = (*iter).pexecutable;

				// - set new executable
				(*iter).pexecutable = _pexecutable;

				// - add ref
				_pexecutable->add_ref();

				// - SCOPED_LOCK leave
				SCOPED_LOCK_LEAVE(cs);

				// - release old executable
				pexecutable_old->release();

				// return)
				return true;
			}
		}
	}

	// return)
	return false;
}


}
#endif