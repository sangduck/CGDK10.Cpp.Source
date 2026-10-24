#include "pch.h"
#include "platform.h"

//-----------------------------------------------------------------------------
// io_context_uring : the io_uring backend of the io_context (linux)
//
//  - readiness events like epoll (IORING_OP_POLL_ADD) on the registry of io_context_Nepoll : the socket code is the same.
//  - raw syscalls (io_uring_setup / io_uring_enter, the rings are mmapped here) : no liburing to link.
//  - needs linux 5.13+ (IORING_ENTER_EXT_ARG at runtime. the headers : IORING_POLL_ADD_MULTI / IORING_POLL_UPDATE_EVENTS defined).
//    older => is_available() false.
//
//  submission
//   - the SQ is written under m_lockable_sq. a worker thread in its batch (pop_events() set t_pdefer_context) defers the enter :
//     the re-arms of the events it processed are submitted together by its next pop_events() / flush_pending().
//     another worker of the executor submits at once.
//   - a request belongs to the thread that submitted it : its completion runs as the task work of that thread (TIF_NOTIFY_SIGNAL :
//     a blocking call of that thread with a timeout, a recv with SO_RCVTIMEO, returned EINTR when a poll of it fired. kernel 7.0 :
//     every time). and an io_uring_enter submits every SQE of the SQ (the deferred ones of the workers too). so a thread that is not
//     a worker of the executor (bind_worker(), after expect_workers()) never enters the ring : its SQEs wait in the SQ and it wakes a
//     worker with the post eventfd (the worker that takes the CQE submits at once). it takes no CQE either (pop_events() returns
//     nothing : the workers process the events).
//     the ring is created without a submit (the poll of the post eventfd is submitted by the first worker).
//     an io_context without workers (expect_workers() not called : pumped by its owner) is submitted by any thread.
//   - the kernel keeps every thread that created a ring or submitted to it (a nop too) and sends each one a task work when the ring
//     is released (the same EINTR : the thread that created an executor, or stopped it, when it was destroyed. kernel 7.0 : every
//     time. a wait (GETEVENTS) or a sync cancel (register) is not kept) : a ring is created by a thread that ends at once
//     (_io_uring_setup()), destroy() of a thread that is not a worker wakes by the eventfd.
//   - the SQEs not submitted yet are SQ entries no thread consumes meanwhile (every enter that submits is under lockable_sq) :
//     detach() turns the polls of the registration into nops, destroy() all of them.
//   - the SQ full => submitted at once (a thread that is not a worker : after it waited for the workers a while).
//  completion
//   - one thread waits in the kernel at a time (lockable_cq) and takes up to _max_events CQEs. the others wait on the lock,
//     no longer than their own wait (a thread pumping the executor with a short wait returns with nothing).
//   - a CQE of a cancel / update (TAG_IGNORE) is dropped. another error => EPOLLERR | EPOLLHUP.
//   - -ECANCELED : a poll cancelled by modify()/detach() (not the armed poll any more) is dropped. the armed poll of a live
//     registration cancelled by the kernel is polled again : a request belongs to the thread that submitted it and completes with
//     -ECANCELED when it fires after that thread exited (a worker that ended : an extra thread of the executor, an executor without
//     workers). (dropped, the registration got no more events : the post eventfd of an executor => its posts were never executed)
//  one shot polls
//   - every poll is one shot (a multishot poll posts a CQE per wakeup : a CQE storm on UDP, no coalescing). a registration
//     without EPOLLONESHOT (UDP, listen sockets) is armed again by flush_pending() of the thread that took its CQE, after
//     the events are processed (like EPOLLET : the processing drains the socket, the arrivals after wake one CQE, and no two
//     threads process the same socket at once).
//     the post eventfd is armed again at once by pop_events() (executable_post takes a part of its queue per CQE and signals
//     again : the rest goes to the other threads. armed after the processing, a blocking executable held every other post)
//   - m_armed[handle] : the sequence (1~127) of the armed poll of the handle (0 : none). it is in the user_data of the poll
//     (bits 24~30) : a CQE disarms the handle only when it is of the armed poll (a CQE of a poll replaced meanwhile does not).
//     the SQEs of a handle are written under lockable_sq with m_armed : modify() cancels the armed poll (IORING_OP_POLL_REMOVE)
//     and adds a new one, or just adds a new poll when the old one fired (a cancel of a poll that fired meanwhile fails with
//     ENOENT and is dropped). only one poll per registration => detach() cancels it, no poll keeps a closed socket open.
//     (detach() waits till the poll is gone : IORING_REGISTER_SYNC_CANCEL. the cancel completes in the task_work of the thread
//      that submitted the poll : a close right after an IORING_OP_POLL_REMOVE left the socket open a moment)
//   - a poll is added only while the registration is the one registered (checked under lockable_sq) : a re-arm racing with
//     detach() does not poll the handle after the cancel (the poll held the file : a closed socket stayed open, no FIN)
//-----------------------------------------------------------------------------
#if defined(__ANDROID__) || defined(__linux__)

#include <linux/io_uring.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <poll.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#if defined(IORING_FEAT_EXT_ARG) && defined(IORING_POLL_ADD_MULTI) && defined(IORING_POLL_UPDATE_EVENTS)
	#define CGDK_IO_URING_SUPPORTED 1
#else
	#define CGDK_IO_URING_SUPPORTED 0
#endif

namespace CGDK
{

//-----------------------------------------------------------------------------
// executor::io_backend : the default backend (set_default_io_backend, env CGDK_IO_URING)
//-----------------------------------------------------------------------------
namespace
{
	std::atomic<int> s_default_io_backend{ -1 };	// - -1 : not decided (the env is read at the first use)

	int _decide_default_io_backend() noexcept
	{
		int backend = s_default_io_backend.load(std::memory_order_acquire);

		if (backend >= 0)
			return backend;

		// - env CGDK_IO_URING : 1/true/on => io_uring, completion/cq/2 => io_uring_completion
		backend = static_cast<int>(executor::io_backend::epoll);

		if (const char* penv = std::getenv("CGDK_IO_URING"); penv != nullptr)
		{
			if (std::strcmp(penv, "1") == 0 || std::strcmp(penv, "true") == 0 || std::strcmp(penv, "on") == 0 || std::strcmp(penv, "uring") == 0 || std::strcmp(penv, "io_uring") == 0)
				backend = static_cast<int>(executor::io_backend::io_uring);
			else if (std::strcmp(penv, "2") == 0 || std::strcmp(penv, "completion") == 0 || std::strcmp(penv, "cq") == 0)
				backend = static_cast<int>(executor::io_backend::io_uring_completion);
		}

		// - the first decision wins (a set_default_io_backend() meanwhile)
		int expected = -1;
		s_default_io_backend.compare_exchange_strong(expected, backend, std::memory_order_acq_rel);

		return s_default_io_backend.load(std::memory_order_acquire);
	}
}

void executor::set_default_io_backend(executor::io_backend _backend) noexcept
{
	s_default_io_backend.store(static_cast<int>(_backend), std::memory_order_release);
}

executor::io_backend executor::default_io_backend() noexcept
{
	return static_cast<executor::io_backend>(_decide_default_io_backend());
}

bool executor::is_io_uring_available() noexcept
{
	return api::io_context_uring::is_available();
}

bool executor::is_io_uring_completion_available() noexcept
{
	return api::io_context_uring_cq::is_available();
}


//-----------------------------------------------------------------------------
// the ring
//-----------------------------------------------------------------------------
namespace
{
	constexpr uint64_t	TAG_IGNORE = 0x80000000ull;		// - bit 31 of user_data (the handle part) : the CQE is not an event (a cancel, an update, a nop)
	constexpr unsigned	RING_ENTRIES = 4096;			// - SQ entries (CQ : x4)

	// - the sequence of a poll : bits 24~30 of user_data (the handle part. m_armed covers the handles below 4M : < 2^24)
	constexpr unsigned	POLL_SEQ_SHIFT = 24;
	constexpr uint64_t	POLL_SEQ_MASK = 0x7Full << POLL_SEQ_SHIFT;
	constexpr unsigned	MAX_ARMED_HANDLE = 4u * 1024u * 1024u;
	constexpr unsigned	DEFAULT_ARMED_HANDLE = 1024u * 1024u;			// - the hard limit is infinite : 1M handles
	static_assert(MAX_ARMED_HANDLE <= (1u << POLL_SEQ_SHIFT));

	constexpr uint64_t	_poll_user_data(uint64_t _event_data, uint8_t _seq) noexcept { return _event_data | (static_cast<uint64_t>(_seq) << POLL_SEQ_SHIFT); }
	constexpr uint64_t	_poll_event_data(uint64_t _user_data) noexcept { return _user_data & ~POLL_SEQ_MASK; }
	constexpr uint8_t	_poll_seq(uint64_t _user_data) noexcept { return static_cast<uint8_t>((_user_data & POLL_SEQ_MASK) >> POLL_SEQ_SHIFT); }

	thread_local const void* t_pdefer_context = nullptr;	// - this thread is in a batch of this io_context_uring : its SQEs are submitted by its next pop_events()/flush_pending()
	thread_local std::vector<const void*> t_worker_context;	// - the io_context_uring this thread is a worker of (bind_worker())

	constexpr auto		WAIT_SQ_FREE = std::chrono::milliseconds(100);		// - the SQ full : a thread that is not a worker waits for the workers this long (then submits itself)

	struct REARM
	{
		const void*			pring_owner;			// - the io_context_uring (a thread may drive more than one : each takes its own entries)
		CGHANDLE			handle;
		uint64_t			event_data;
	};
	thread_local std::vector<REARM> t_rearm;			// - the registrations without EPOLLONESHOT whose CQE this thread took : armed again by its flush_pending() (after the processing)

	int _io_uring_setup(unsigned _entries, io_uring_params* _pparams) noexcept
	{
		// - on a thread that ends at once : the kernel sends a task work to the thread that created a ring when the ring is released
		//   (TIF_NOTIFY_SIGNAL : a blocking call with a timeout of that thread returned EINTR. the thread that created an executor, or
		//   that called is_available() first). a thread that ended is not kept
		int result = -1;
		int error = 0;

		try
		{
			std::thread([&]() { result = static_cast<int>(::syscall(__NR_io_uring_setup, _entries, _pparams)); error = errno; }).join();
			errno = error;
		}
		catch (...)
		{
			result = static_cast<int>(::syscall(__NR_io_uring_setup, _entries, _pparams));
		}

		return result;
	}

	int _io_uring_enter(int _fd, unsigned _to_submit, unsigned _min_complete, unsigned _flags, const void* _parg, std::size_t _size_arg) noexcept
	{
		return static_cast<int>(::syscall(__NR_io_uring_enter, _fd, _to_submit, _min_complete, _flags, _parg, _size_arg));
	}

	// - IORING_REGISTER_SYNC_CANCEL (linux 6.0+. defined here : the older headers do not have it). an older kernel : EINVAL
	constexpr unsigned	REGISTER_SYNC_CANCEL = 24;

	struct SYNC_CANCEL_REG
	{
		uint64_t			addr;
		int32_t				fd;
		uint32_t			flags;
		int64_t				timeout_sec;
		int64_t				timeout_nsec;
		uint8_t				opcode;
		uint8_t				pad[7];
		uint64_t			pad2[3];
	};
	static_assert(sizeof(SYNC_CANCEL_REG) == 64);

	std::atomic<bool>	s_is_sync_cancel_unsupported{ false };

	int _io_uring_sync_cancel(int _fd, uint64_t _user_data) noexcept
	{
		SYNC_CANCEL_REG reg{};
		reg.addr = _user_data;
		reg.fd = -1;
		reg.timeout_sec = -1;			// - no timeout (a request being completed : waited for)
		reg.timeout_nsec = -1;

		return static_cast<int>(::syscall(__NR_io_uring_register, _fd, REGISTER_SYNC_CANCEL, &reg, 1));
	}

	// - the poll mask of the epoll flags (EPOLLET/EPOLLONESHOT/EPOLLEXCLUSIVE/EPOLLWAKEUP are not poll events)
	constexpr uint32_t _poll_mask(uint32_t _event) noexcept
	{
		return _event & static_cast<uint32_t>(EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
	}
}

struct api::io_context_uring::RING
{
	int					fd = -1;

	// - SQ
	unsigned*			sq_head = nullptr;
	unsigned*			sq_tail = nullptr;
	unsigned			sq_mask = 0;
	unsigned			sq_entries = 0;
	unsigned*			sq_array = nullptr;
	io_uring_sqe*		sqes = nullptr;
	unsigned			sq_tail_local = 0;				// - the next free slot (under m_lockable_sq)
	uint8_t				seq_poll = 0;					// - the sequence of the last poll (under m_lockable_sq. 1~127)

	// - CQ
	unsigned*			cq_head = nullptr;
	unsigned*			cq_tail = nullptr;
	unsigned			cq_mask = 0;
	unsigned			cq_entries = 0;
	io_uring_cqe*		cqes = nullptr;

	// - mmap
	void*				ptr_sq = nullptr;
	std::size_t			size_sq = 0;
	void*				ptr_cq = nullptr;
	std::size_t			size_cq = 0;
	std::size_t			size_sqes = 0;

	std::mutex			lockable_sq;
	std::timed_mutex	lockable_cq;

	// - the SQEs written but not submitted (the kernel consumes sq_head on io_uring_enter)
	[[nodiscard]] unsigned pending() const noexcept { return sq_tail_local - __atomic_load_n(sq_head, __ATOMIC_ACQUIRE); }

	// - the next SQE (under lockable_sq). nullptr : the SQ is full (after a submit)
	io_uring_sqe* alloc_sqe() noexcept
	{
		if (pending() >= sq_entries)
		{
			submit(0, 0, nullptr, 0);

			if (pending() >= sq_entries)
				return nullptr;
		}

		const unsigned index = sq_tail_local & sq_mask;
		io_uring_sqe* psqe = &sqes[index];

		std::memset(psqe, 0, sizeof(*psqe));
		sq_array[index] = index;
		++sq_tail_local;

		return psqe;
	}

	// - the SQEs not submitted yet => nops (under lockable_sq : no thread enters the ring meanwhile, the kernel reads them at the enter)
	static void _neutralize(io_uring_sqe* _psqe) noexcept
	{
		std::memset(_psqe, 0, sizeof(*_psqe));
		_psqe->opcode = IORING_OP_NOP;
		_psqe->fd = -1;
		_psqe->user_data = TAG_IGNORE;
	}

	void neutralize_all() noexcept
	{
		for (unsigned i = __atomic_load_n(sq_head, __ATOMIC_ACQUIRE); i != sq_tail_local; ++i)
			_neutralize(&sqes[sq_array[i & sq_mask]]);
	}

	// - the polls of the registration _event_data and its poll removes. the polls the removes were for (submitted before : in the
	//   kernel) are given in _premoved (up to _max : the others are left to their remove). returns their count
	std::size_t neutralize_registration(uint64_t _event_data, uint64_t* _premoved, std::size_t _max) noexcept
	{
		std::size_t count = 0;

		for (unsigned i = __atomic_load_n(sq_head, __ATOMIC_ACQUIRE); i != sq_tail_local; ++i)
		{
			io_uring_sqe* psqe = &sqes[sq_array[i & sq_mask]];

			if (psqe->opcode == IORING_OP_POLL_ADD && _poll_event_data(psqe->user_data) == _event_data)
			{
				_neutralize(psqe);
			}
			else if (psqe->opcode == IORING_OP_POLL_REMOVE && psqe->user_data == (_event_data | TAG_IGNORE) && count < _max)
			{
				_premoved[count++] = psqe->addr;
				_neutralize(psqe);
			}
		}

		return count;
	}

	// - publishes the tail and enters (under lockable_sq when _min_complete == 0). returns the result of io_uring_enter (-1 : errno)
	int submit(unsigned _min_complete, unsigned _flags, const void* _parg, std::size_t _size_arg) noexcept
	{
		__atomic_store_n(sq_tail, sq_tail_local, __ATOMIC_RELEASE);

		const unsigned to_submit = pending();

		if (to_submit == 0 && _min_complete == 0 && (_flags & IORING_ENTER_GETEVENTS) == 0)
			return 0;

		for (int count_retry = 0; ; ++count_retry)
		{
			const int result = _io_uring_enter(fd, to_submit, _min_complete, _flags, _parg, _size_arg);

			if (result >= 0 || (errno != EINTR && errno != EAGAIN && errno != EBUSY))
				return result;

			// - EINTR while waiting => the caller waits again
			if (errno == EINTR && _min_complete != 0)
				return result;

			// - EAGAIN/EBUSY (the kernel is busy, the CQ is full) => again, a few times (the SQEs stay pending : the next submit tries again)
			if (count_retry >= 64)
				return result;

			std::this_thread::yield();
		}
	}

	bool create(unsigned _entries) noexcept
	{
	#if CGDK_IO_URING_SUPPORTED
		io_uring_params params{};
		params.flags = IORING_SETUP_CLAMP | IORING_SETUP_CQSIZE;
		params.cq_entries = _entries * 4;

		fd = _io_uring_setup(_entries, &params);

		if (fd < 0)
			return false;

		// check) the features this backend needs
		if ((params.features & IORING_FEAT_EXT_ARG) == 0 || (params.features & IORING_FEAT_NODROP) == 0)
		{
			::close(fd);
			fd = -1;
			return false;
		}

		size_sq = params.sq_off.array + params.sq_entries * sizeof(unsigned);
		size_cq = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);

		const bool is_single_mmap = (params.features & IORING_FEAT_SINGLE_MMAP) != 0;

		if (is_single_mmap)
			size_sq = size_cq = std::max(size_sq, size_cq);

		ptr_sq = ::mmap(nullptr, size_sq, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQ_RING);

		if (ptr_sq == MAP_FAILED)
		{
			ptr_sq = nullptr;
			destroy();
			return false;
		}

		if (is_single_mmap)
		{
			ptr_cq = ptr_sq;
		}
		else
		{
			ptr_cq = ::mmap(nullptr, size_cq, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_CQ_RING);

			if (ptr_cq == MAP_FAILED)
			{
				ptr_cq = nullptr;
				destroy();
				return false;
			}
		}

		size_sqes = params.sq_entries * sizeof(io_uring_sqe);
		sqes = static_cast<io_uring_sqe*>(::mmap(nullptr, size_sqes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQES));

		if (sqes == MAP_FAILED)
		{
			sqes = nullptr;
			destroy();
			return false;
		}

		auto* base_sq = static_cast<char*>(ptr_sq);
		auto* base_cq = static_cast<char*>(ptr_cq);

		sq_head = reinterpret_cast<unsigned*>(base_sq + params.sq_off.head);
		sq_tail = reinterpret_cast<unsigned*>(base_sq + params.sq_off.tail);
		sq_mask = *reinterpret_cast<unsigned*>(base_sq + params.sq_off.ring_mask);
		sq_entries = *reinterpret_cast<unsigned*>(base_sq + params.sq_off.ring_entries);
		sq_array = reinterpret_cast<unsigned*>(base_sq + params.sq_off.array);
		sq_tail_local = *sq_tail;

		cq_head = reinterpret_cast<unsigned*>(base_cq + params.cq_off.head);
		cq_tail = reinterpret_cast<unsigned*>(base_cq + params.cq_off.tail);
		cq_mask = *reinterpret_cast<unsigned*>(base_cq + params.cq_off.ring_mask);
		cq_entries = *reinterpret_cast<unsigned*>(base_cq + params.cq_off.ring_entries);
		cqes = reinterpret_cast<io_uring_cqe*>(base_cq + params.cq_off.cqes);

		return true;
	#else
		static_cast<void>(_entries);
		return false;
	#endif
	}

	void destroy() noexcept
	{
		if (sqes != nullptr) { ::munmap(sqes, size_sqes); sqes = nullptr; }
		if (ptr_cq != nullptr && ptr_cq != ptr_sq) { ::munmap(ptr_cq, size_cq); }
		if (ptr_sq != nullptr) { ::munmap(ptr_sq, size_sq); }
		ptr_cq = nullptr;
		ptr_sq = nullptr;
		if (fd >= 0) { ::close(fd); fd = -1; }
	}
};


//-----------------------------------------------------------------------------
// RING_GUARD : counts a thread using the ring. destroy() waits for the count after it invalidated the handle (the rings are unmapped after)
//-----------------------------------------------------------------------------
struct api::io_context_uring::RING_GUARD
{
	api::io_context_uring&	self;
	RING*					pring;

	explicit RING_GUARD(api::io_context_uring& _self) noexcept : self(_self), pring(nullptr)
	{
		self.m_count_users.fetch_add(1, std::memory_order_acq_rel);

		// - the ring is usable only while the handle is valid (destroy() invalidates it first)
		if (self.m_handle_epoll != INVALID_CGHANDLE_VALUE)
			pring = self.m_pring.get();
	}
	~RING_GUARD() noexcept
	{
		self.m_count_users.fetch_sub(1, std::memory_order_acq_rel);
	}
	RING_GUARD(const RING_GUARD&) = delete;
	RING_GUARD& operator=(const RING_GUARD&) = delete;
};


//-----------------------------------------------------------------------------
// executable_post : the executables posted to the io_context (an eventfd polled multishot)
//
//  - post() queues the executable and signals the eventfd when the queue was idle.
//  - process_execute() (the CQE of the eventfd) runs a batch. the queue not empty after it => the eventfd is signaled again
//    (a multishot poll completes on a wakeup : the eventfd staying readable does not complete again like a level triggered epoll)
//-----------------------------------------------------------------------------
class api::io_context_uring::executable_post : virtual public Iexecutable, virtual public object::Idestroyable
{
public:
			api::io_context_uring* m_pthis{ nullptr };
			CGHANDLE			m_event_fd_post{ INVALID_CGHANDLE_VALUE };
			uint64_t			m_event_data_post{ 0 };			// - the registration of the eventfd (set by initialize() before its poll : read by pop_events() without a lock)
			CGDK::circular_list<RESULT> m_queue_executable;
			bool				m_is_signaled{ false };
			std::atomic<int>	m_count_signaling{ 0 };
	mutable	std::mutex			m_lockable_complete;

	static	constexpr std::size_t MAX_EXECUTE_BATCH = 16;

public:
			bool				initialize(api::io_context_uring* _pthis) noexcept;
	virtual	result_code			destroy() noexcept override;
			size_t				get_queued_count() const noexcept { std::lock_guard lock(this->m_lockable_complete); return this->m_queue_executable.size(); }

			bool				post(uintptr_t _completion_key, Iexecutable* _pexecutable, bool _is_adopt) noexcept;
			void				signal() noexcept;
			void				wake() noexcept;
	virtual	intptr_t			process_execute(intptr_t _result, size_t _param) override;
};

bool api::io_context_uring::executable_post::initialize(api::io_context_uring* _pthis) noexcept
{
	// 1) create post event_fd (not a semaphore : read once when the queue is empty)
	const CGHANDLE event_fd_post = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

	if (event_fd_post == INVALID_CGHANDLE_VALUE)
		return false;

	// 2) register (the registry : handle, executable, generation, the mask) and poll it (armed again after each event : no EPOLLONESHOT)
	const auto event_data = _pthis->_process_register_handle(event_fd_post, this);

	_pthis->_process_set_event_mask(event_fd_post, EPOLLIN);

	this->m_event_data_post = event_data;

	if (_pthis->_submit_poll_add(event_fd_post, event_data, EPOLLIN) == false)
	{
		_pthis->_process_unregister_handle(event_fd_post);
		::close(event_fd_post);
		return false;
	}

	// 3) store
	this->m_event_fd_post = event_fd_post;
	this->m_pthis = _pthis;

	return true;
}

result_code api::io_context_uring::executable_post::destroy() noexcept
{
	// 1) take the fd under the lock (post() checks it under the lock)
	CGHANDLE event_fd_post;
	{
		std::lock_guard lock(this->m_lockable_complete);
		event_fd_post = this->m_event_fd_post;
		this->m_event_fd_post = INVALID_CGHANDLE_VALUE;
	}

	// 2) wait for the signals out of the lock
	while (this->m_count_signaling.load(std::memory_order_acquire) != 0)
		std::this_thread::yield();

	if (event_fd_post != INVALID_CGHANDLE_VALUE)
	{
		// - unregister (the poll was cancelled or dropped with the ring)
		this->m_pthis->_process_unregister_handle(event_fd_post);

		// - close eventfd
		::close(event_fd_post);
	}

	// 3) release the executables not executed
	{
		CGDK::circular_list<RESULT> queue_executable;
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

void api::io_context_uring::executable_post::signal() noexcept
{
	// - the eventfd (the count is read when the queue is empty)
	CGHANDLE event_fd_signal = INVALID_CGHANDLE_VALUE;
	{
		std::lock_guard lock(this->m_lockable_complete);

		if (this->m_is_signaled || this->m_event_fd_post == INVALID_CGHANDLE_VALUE)
			return;

		this->m_is_signaled = true;
		event_fd_signal = this->m_event_fd_post;
		this->m_count_signaling.fetch_add(1, std::memory_order_relaxed);
	}

	if (::eventfd_write(event_fd_signal, 1) < 0)
	{
		std::lock_guard lock(this->m_lockable_complete);
		this->m_is_signaled = false;
	}

	this->m_count_signaling.fetch_sub(1, std::memory_order_release);
}

void api::io_context_uring::executable_post::wake() noexcept
{
	// - a CQE of the eventfd whatever the queue (the worker that takes it submits the SQ : io_context_uring::_wake_submitter()).
	//   process_execute() takes nothing from an empty queue and reads the count
	CGHANDLE event_fd_signal = INVALID_CGHANDLE_VALUE;
	{
		std::lock_guard lock(this->m_lockable_complete);

		if (this->m_event_fd_post == INVALID_CGHANDLE_VALUE)
			return;

		event_fd_signal = this->m_event_fd_post;
		this->m_count_signaling.fetch_add(1, std::memory_order_relaxed);
	}

	::eventfd_write(event_fd_signal, 1);

	this->m_count_signaling.fetch_sub(1, std::memory_order_release);
}

bool api::io_context_uring::executable_post::post(uintptr_t _completion_key, Iexecutable* _pexecutable, bool _is_adopt) noexcept
{
	// 1) make RESULT
	RESULT result;
	result.data.ptr = reinterpret_cast<void*>(_pexecutable);
	result.events = static_cast<uint32_t>(_completion_key);

	// declare)
	CGHANDLE event_fd_signal = INVALID_CGHANDLE_VALUE;

	{
		std::lock_guard lock(this->m_lockable_complete);

		// check) destroyed
		RETURN_IF(this->m_event_fd_post == INVALID_CGHANDLE_VALUE, false);

		// 2) queue
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

		// 4) signal when idle
		if (this->m_is_signaled == false)
		{
			this->m_is_signaled = true;
			event_fd_signal = this->m_event_fd_post;
			this->m_count_signaling.fetch_add(1, std::memory_order_relaxed);
		}
	}

	// 5) notify (out of the lock)
	if (event_fd_signal != INVALID_CGHANDLE_VALUE)
	{
		if (::eventfd_write(event_fd_signal, 1) < 0)
		{
			std::lock_guard lock(this->m_lockable_complete);
			this->m_is_signaled = false;
		}

		this->m_count_signaling.fetch_sub(1, std::memory_order_release);
	}

	return true;
}

intptr_t api::io_context_uring::executable_post::process_execute(intptr_t _result, size_t _param)
{
	// declare)
	Iexecutable* array_executable[MAX_EXECUTE_BATCH];
	std::size_t count_executable = 0;
	bool is_more = false;

	{
		std::lock_guard lock(this->m_lockable_complete);

		// check) destroyed
		RETURN_IF(this->m_event_fd_post == INVALID_CGHANDLE_VALUE, 0);

		// 1) a batch (a part of the queue : the other threads take the rest)
		const auto size_queue = this->m_queue_executable.size();
		count_executable = (size_queue == 0) ? 0 : std::clamp<std::size_t>(size_queue / 8, 1, MAX_EXECUTE_BATCH);

		for (std::size_t i = 0; i < count_executable; ++i)
		{
			array_executable[i] = reinterpret_cast<Iexecutable*>(this->m_queue_executable.front().data.ptr);
			this->m_queue_executable.pop_front();
		}

		// 2) the queue is empty => read the eventfd (the count) and the next post signals again
		if (this->m_queue_executable.empty())
		{
			eventfd_t temp_eventfd;
			::eventfd_read(this->m_event_fd_post, &temp_eventfd);

			this->m_is_signaled = false;
		}
		else
		{
			is_more = true;
		}
	}

	// 3) not empty => signal again (a multishot poll completes on a wakeup only)
	if (is_more)
	{
		std::lock_guard lock(this->m_lockable_complete);

		if (this->m_event_fd_post != INVALID_CGHANDLE_VALUE)
			::eventfd_write(this->m_event_fd_post, 1);
	}

	// 4) execute (an exception : the rest is executed and released, the first one is thrown again)
	std::exception_ptr pexception;

	for (std::size_t i = 0; i < count_executable; ++i)
	{
		auto pexecutable = array_executable[i];

		CONTINUE_IF(pexecutable == nullptr);

		try
		{
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

	if (pexception)
		std::rethrow_exception(pexception);

	return 0;
}


//-----------------------------------------------------------------------------
// io_context_uring
//-----------------------------------------------------------------------------
api::io_context_uring::io_context_uring()
{
}

api::io_context_uring::~io_context_uring() noexcept
{
	this->destroy();
}

bool api::io_context_uring::is_available() noexcept
{
#if CGDK_IO_URING_SUPPORTED
	// - checked once : a ring with the features this backend needs
	static const bool s_is_available = []()
	{
		io_uring_params params{};
		params.flags = IORING_SETUP_CLAMP;

		const int fd = _io_uring_setup(8, &params);

		if (fd < 0)
			return false;

		::close(fd);

		// - 5.11 : EXT_ARG (a wait with a timeout), 5.5 : NODROP. (POLL_ADD_MULTI / POLL_UPDATE : 5.13, an older kernel fails the SQE : EINVAL)
		return (params.features & IORING_FEAT_EXT_ARG) != 0 && (params.features & IORING_FEAT_NODROP) != 0;
	}();

	return s_is_available;
#else
	return false;
#endif
}

bool api::io_context_uring::create(int /*_hint_size*/) noexcept
{
	// check) created already
	RETURN_IF(this->m_handle_epoll != INVALID_CGHANDLE_VALUE || this->m_pring, false);

	// check) available
	RETURN_IF(is_available() == false, false);

	// 1) the ring (created by a thread that ends at once : _io_uring_setup())
	auto pring = std::make_unique<RING>();

	RETURN_IF(pring->create(RING_ENTRIES) == false, false);

	// 2) the armed flags of the one shot polls (indexed by the handle : the open file limit)
	{
		// - the HARD limit of open files (the soft limit is raised after the start of a process often : an fd above it had no slot)
		rlimit limit_files{};
		unsigned count = DEFAULT_ARMED_HANDLE;

		if (::getrlimit(RLIMIT_NOFILE, &limit_files) == 0)
			count = static_cast<unsigned>(std::min<rlim_t>(std::max<rlim_t>(limit_files.rlim_cur, (limit_files.rlim_max == RLIM_INFINITY) ? DEFAULT_ARMED_HANDLE : limit_files.rlim_max), MAX_ARMED_HANDLE));

		this->m_armed = std::make_unique<std::atomic<uint8_t>[]>(count);

		for (unsigned i = 0; i < count; ++i)
			this->m_armed[i].store(0, std::memory_order_relaxed);

		this->m_count_armed = count;
	}

	// 3) store (handle() : the ring fd)
	this->m_pring = std::move(pring);
	this->m_handle_epoll = this->m_pring->fd;
	this->m_count_attached = 0;
	this->m_is_worker_expected.store(false, std::memory_order_release);
	this->m_is_wake_submit.store(false, std::memory_order_release);

	// 4) the post eventfd (its poll is left in the SQ : submitted by the first pop_events(), a worker. submitted by this thread,
	//    the poll would be its request : every post of the executor till it fired once, a task work into this thread)
	auto pexecutable_post = make_own<executable_post>();

	const auto pdefer_before = t_pdefer_context;

	t_pdefer_context = this;
	const bool is_initialized = pexecutable_post->initialize(this);
	t_pdefer_context = pdefer_before;

	if (is_initialized == false)
	{
		this->m_handle_epoll = INVALID_CGHANDLE_VALUE;
		this->m_pring->destroy();
		this->m_pring.reset();
		return false;
	}

	this->m_pexecutable_post = std::move(pexecutable_post);

	// return)
	return true;
}

CGHANDLE api::io_context_uring::dispose() noexcept
{
	// - handle() => INVALID : pop_events() returns at once. the ring stays until destroy()
	return this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);
}

result_code api::io_context_uring::destroy() noexcept
{
	// check)
	RETURN_IF(!this->m_pring, eRESULT::SUCCESS);

	// 1) handle() => INVALID
	this->m_handle_epoll = INVALID_CGHANDLE_VALUE;

	// 2) wake the waiter and wait until no thread uses the ring (pop_events, modify, flush ... : RING_GUARD). the rings are unmapped below.
	//    - the SQEs not submitted yet : nops (the executor stopped)
	//    - a worker (or any thread of an io_context without workers) : a nop CQE. another thread : the post eventfd (a thread that ever
	//      submitted to a ring gets a task work when the ring is released below : EINTR. usually the workers ended already :
	//      io_context_threads::stop() waits for them)
	const bool is_submitter = this->_is_submitter();
	bool is_woken = false;

	for (;;)
	{
		{
			std::lock_guard lock(this->m_pring->lockable_sq);

			this->m_pring->neutralize_all();

			if (is_submitter)
			{
				if (io_uring_sqe* psqe = this->m_pring->alloc_sqe(); psqe != nullptr)
				{
					psqe->opcode = IORING_OP_NOP;
					psqe->fd = -1;
					psqe->user_data = TAG_IGNORE;
				}

				this->m_pring->submit(0, 0, nullptr, 0);
			}
		}

		if (is_submitter == false && is_woken == false && this->m_pexecutable_post)
		{
			this->m_pexecutable_post->wake();
			is_woken = true;
		}

		if (this->m_count_users.load(std::memory_order_acquire) == 0)
			break;

		std::this_thread::yield();
	}

	// 3) release the registry (the polls go with the ring)
	this->_process_remove_all_handle(INVALID_CGHANDLE_VALUE);

	// 4) destroy the post eventfd (its registration was released above : the executable is kept by own_ptr)
	if (this->m_pexecutable_post)
	{
		this->m_pexecutable_post->destroy();
		this->m_pexecutable_post.reset();
	}

	// 5) the ring
	this->m_pring->destroy();
	this->m_pring.reset();
	this->m_armed.reset();
	this->m_count_armed = 0;

	return eRESULT::SUCCESS;
}

bool api::io_context_uring::_disarm(CGHANDLE _handle, uint8_t _seq) noexcept
{
	// - the CQE of the armed poll (_seq) : not armed any more. false : another poll is armed (replaced by modify() meanwhile) or none
	//   (a handle out of m_armed : not tracked, every CQE disarms)
	if (_handle < 0 || static_cast<unsigned>(_handle) >= this->m_count_armed)
		return true;

	uint8_t expected = _seq;

	return this->m_armed[static_cast<unsigned>(_handle)].compare_exchange_strong(expected, 0, std::memory_order_acq_rel);
}

bool api::io_context_uring::_is_armed(CGHANDLE _handle) const noexcept
{
	return _handle >= 0 && static_cast<unsigned>(_handle) < this->m_count_armed && this->m_armed[static_cast<unsigned>(_handle)].load(std::memory_order_acquire) != 0;
}

void api::io_context_uring::expect_workers() noexcept
{
	// - called by the executor before its threads start (they bind as they start : the SQEs written meanwhile wait for them)
	this->m_is_worker_expected.store(true, std::memory_order_release);
}

void api::io_context_uring::bind_worker() noexcept
{
	RETURN_IF(this->_is_worker());

	try
	{
		t_worker_context.push_back(this);
	}
	catch (...)
	{
	}
}

void api::io_context_uring::unbind_worker() noexcept
{
	// - the requests this thread submitted complete with -ECANCELED when it exits : the armed polls are polled again by the thread
	//   that takes the CQE (pop_events())
	auto iter = std::find(t_worker_context.begin(), t_worker_context.end(), static_cast<const void*>(this));

	if (iter != t_worker_context.end())
		t_worker_context.erase(iter);

	if (t_pdefer_context == this)
		t_pdefer_context = nullptr;
}

bool api::io_context_uring::_is_worker() const noexcept
{
	return std::find(t_worker_context.begin(), t_worker_context.end(), static_cast<const void*>(this)) != t_worker_context.end();
}

bool api::io_context_uring::_is_submitter() const noexcept
{
	// - a worker, or any thread of an io_context without workers (pumped by its owner)
	return this->m_is_worker_expected.load(std::memory_order_acquire) == false || this->_is_worker();
}

void api::io_context_uring::_wake_submitter() noexcept
{
	// - a CQE of the post eventfd : the worker that takes it submits the SQ at once (pop_events(), the re-arm of the eventfd).
	//   once till a worker submits (m_is_wake_submit : cleared by the submit of a worker, after this thread wrote its SQEs).
	//   no worker bound yet : the first one submits
	RETURN_IF(this->m_is_wake_submit.exchange(true, std::memory_order_acq_rel));

	if (auto pexecutable_post = this->m_pexecutable_post.get(); pexecutable_post != nullptr)
		pexecutable_post->wake();
}

void api::io_context_uring::_wait_sq_free(std::unique_lock<std::mutex>& _lock, unsigned _count) noexcept
{
	// - under lockable_sq (_lock). a thread that is not a worker with the SQ full : the workers submit (woken), it waits for them a
	//   while (WAIT_SQ_FREE), then alloc_sqe() submits by this thread (the last resort)
	auto* pring = this->m_pring.get();

	RETURN_IF(this->_is_submitter() || (pring->pending() + _count) <= pring->sq_entries);

	const auto tick_end = chrono::tick::now() + WAIT_SQ_FREE;

	do
	{
		_lock.unlock();
		this->_wake_submitter();
		std::this_thread::sleep_for(std::chrono::microseconds(50));
		_lock.lock();
	} while ((pring->pending() + _count) > pring->sq_entries && chrono::tick::now() < tick_end);
}

void api::io_context_uring::_flush_locked() noexcept
{
	// - under lockable_sq. a worker in its batch defers (its pop_events()/flush_pending() submits), another worker submits now.
	//   a thread that is not a worker wakes one (the SQEs wait in the SQ : entered by this thread, they would be its requests)
	if (t_pdefer_context == this)
		return;

	if (this->_is_submitter() == false)
	{
		if (this->m_pring->pending() != 0)
			this->_wake_submitter();
		return;
	}

	this->m_is_wake_submit.store(false, std::memory_order_release);
	this->m_pring->submit(0, 0, nullptr, 0);
}

bool api::io_context_uring::_submit_poll_add(CGHANDLE _handle, uint64_t _event_data, uint32_t _event) noexcept
{
	// - the caller holds a RING_GUARD (or creates the ring)
	auto* pring = this->m_pring.get();

	RETURN_IF(pring == nullptr || this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	std::unique_lock lock(pring->lockable_sq);

	// - room for the two SQEs (a thread that is not a worker with the SQ full : waits for the workers)
	this->_wait_sq_free(lock, 2);

	// check) the registration is not the one registered now (detach() meanwhile : polled after its cancel, the poll would keep
	//        the closed socket open. registered again : the poll of the new registration is its own)
	{
		uint64_t event_data_now;

		RETURN_IF(this->_process_get_event_data(_handle, event_data_now) == false || event_data_now != _event_data, false);
	}

	// - the armed poll (not fired yet, or fired and its CQE not taken yet) : cancelled. one poll per registration
	//   (the -ECANCELED CQE is dropped : not the armed poll any more. a poll that fired : ENOENT, dropped. its event is processed)
	const bool is_tracked = (_handle >= 0 && static_cast<unsigned>(_handle) < this->m_count_armed);
	const uint8_t seq_old = is_tracked ? this->m_armed[static_cast<unsigned>(_handle)].load(std::memory_order_acquire) : 0;

	if (seq_old != 0)
	{
		io_uring_sqe* psqe_remove = pring->alloc_sqe();

		RETURN_IF(psqe_remove == nullptr, false);

		psqe_remove->opcode = IORING_OP_POLL_REMOVE;
		psqe_remove->fd = -1;
		psqe_remove->addr = _poll_user_data(_event_data, seq_old);
		psqe_remove->user_data = _event_data | TAG_IGNORE;
	}

	io_uring_sqe* psqe = pring->alloc_sqe();

	if (psqe == nullptr)
	{
		if (is_tracked)
			this->m_armed[static_cast<unsigned>(_handle)].store(0, std::memory_order_release);

		this->_flush_locked();
		return false;
	}

	// - the sequence of this poll (1~127)
	uint8_t seq = 0;

	if (is_tracked)
	{
		pring->seq_poll = static_cast<uint8_t>((pring->seq_poll % 127) + 1);
		seq = pring->seq_poll;
	}

	// - every poll is one shot. a registration without EPOLLONESHOT (listen sockets, UDP) is armed again by flush_pending() of
	//   the thread that processed its event (a multishot poll posted a CQE per wakeup : a CQE storm on UDP). the post eventfd :
	//   by pop_events() when its CQE is taken
	psqe->opcode = IORING_OP_POLL_ADD;
	psqe->fd = _handle;
	psqe->poll32_events = _poll_mask(_event);
	psqe->len = 0;
	psqe->user_data = _poll_user_data(_event_data, seq);

	// - armed
	if (is_tracked)
		this->m_armed[static_cast<unsigned>(_handle)].store(seq, std::memory_order_release);

	this->_flush_locked();

	return true;
}

bool api::io_context_uring::_submit_poll_cancel(CGHANDLE _handle, uint64_t _event_data) noexcept
{
	// - the caller holds a RING_GUARD. the registration was released (no poll is added for it after this : _submit_poll_add())
	auto* pring = this->m_pring.get();

	RETURN_IF(pring == nullptr || this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	// 1) the armed poll (none : fired, its CQE is dropped. not tracked : the poll of the registration)
	const bool is_tracked = (_handle >= 0 && static_cast<unsigned>(_handle) < this->m_count_armed);
	uint64_t user_data_polls[1 + 8];
	std::size_t count_polls = 0;

	{
		std::unique_lock lock(pring->lockable_sq);

		const uint8_t seq_old = is_tracked ? this->m_armed[static_cast<unsigned>(_handle)].exchange(0, std::memory_order_acq_rel) : 0;

		if (is_tracked == false || seq_old != 0)
			user_data_polls[count_polls++] = _poll_user_data(_event_data, seq_old);

		// - the SQEs of this registration not submitted yet (written deferred by a worker, or by a thread that is not a worker) : nops.
		//   the handle is closed right after : a poll submitted after it would poll the closed handle (or the socket that got its number)
		//   and hold the file. a poll remove not submitted yet : the poll it was for (in the kernel) is cancelled below instead
		count_polls += pring->neutralize_registration(_event_data, user_data_polls + count_polls, 8);

		// - no IORING_REGISTER_SYNC_CANCEL (linux < 6.0) : cancelled by IORING_OP_POLL_REMOVE (submitted by a worker : this one, or woken).
		//   the CQE is dropped (TAG_IGNORE)
		if (count_polls != 0 && s_is_sync_cancel_unsupported.load(std::memory_order_relaxed))
		{
			this->_wait_sq_free(lock, static_cast<unsigned>(count_polls));

			for (std::size_t i = 0; i < count_polls; ++i)
			{
				if (io_uring_sqe* psqe = pring->alloc_sqe(); psqe != nullptr)
				{
					psqe->opcode = IORING_OP_POLL_REMOVE;
					psqe->fd = -1;
					psqe->addr = user_data_polls[i];
					psqe->user_data = _event_data | TAG_IGNORE;
				}
			}

			const auto pdefer_before = t_pdefer_context;
			t_pdefer_context = nullptr;
			this->_flush_locked();
			t_pdefer_context = pdefer_before;

			count_polls = 0;
		}
	}

	// 2) cancel the polls and wait till they are gone (IORING_REGISTER_SYNC_CANCEL : ENOENT once it is removed. a register call, not a
	//    request : any thread)
	//    - the cancel completes in the task_work of the thread that submitted the poll : a close right after a POLL_REMOVE left the
	//      socket open a moment (a listen socket still accepted, its UDS name stayed bound : an acceptor started again failed)
	for (std::size_t i = 0; i < count_polls; ++i)
	{
		const uint64_t user_data_poll = user_data_polls[i];

		for (int count_retry = 0; count_retry < 100000; ++count_retry)
		{
			const int result = _io_uring_sync_cancel(pring->fd, user_data_poll);

			// - gone
			BREAK_IF(result < 0 && errno == ENOENT);

			// - not supported (linux < 6.0) : IORING_OP_POLL_REMOVE from now
			if (result < 0 && errno == EINVAL)
			{
				s_is_sync_cancel_unsupported.store(true, std::memory_order_relaxed);

				std::unique_lock lock(pring->lockable_sq);

				this->_wait_sq_free(lock, 1);

				if (io_uring_sqe* psqe = pring->alloc_sqe(); psqe != nullptr)
				{
					psqe->opcode = IORING_OP_POLL_REMOVE;
					psqe->fd = -1;
					psqe->addr = user_data_poll;
					psqe->user_data = _event_data | TAG_IGNORE;
				}

				const auto pdefer_before = t_pdefer_context;
				t_pdefer_context = nullptr;
				this->_flush_locked();
				t_pdefer_context = pdefer_before;
				break;
			}

			// check) another error
			BREAK_IF(result < 0 && errno != EALREADY && errno != EINTR);

			// - cancelled (0) : removed by the task_work of the thread that submitted the poll => again till ENOENT
			std::this_thread::yield();
		}
	}

	return true;
}

bool api::io_context_uring::_submit_nop() noexcept
{
	// - the caller holds a RING_GUARD
	auto* pring = this->m_pring.get();

	RETURN_IF(pring == nullptr, false);

	std::lock_guard lock(pring->lockable_sq);

	io_uring_sqe* psqe = pring->alloc_sqe();

	RETURN_IF(psqe == nullptr, false);

	psqe->opcode = IORING_OP_NOP;
	psqe->fd = -1;
	psqe->user_data = TAG_IGNORE;

	return true;
}

void api::io_context_uring::flush_pending() noexcept
{
	// - submits the deferred SQEs of this thread (and any other pending). the thread defers no more until its next pop_events()
	RING_GUARD guard(*this);

	if (guard.pring == nullptr)
	{
		if (t_pdefer_context == this)
			t_pdefer_context = nullptr;
		return;
	}

	// 1) the registrations without EPOLLONESHOT whose events this thread processed : polled again (written deferred : the one enter below)
	this->_process_rearm_pending();

	if (t_pdefer_context == this)
		t_pdefer_context = nullptr;

	// check) a thread that is not a worker : it took no CQE (nothing of it was deferred), its SQEs woke the workers when written
	RETURN_IF(this->_is_submitter() == false);

	// 2) submit
	std::lock_guard lock(guard.pring->lockable_sq);

	this->_flush_locked();
}

void api::io_context_uring::_process_rearm_pending() noexcept
{
	// - the caller holds a RING_GUARD. the poll is written with the mask of the registration now (a modify() meanwhile changed it).
	//   skipped : armed meanwhile (a modify() during the processing), released or registered again (detach : the generation differs)
	RETURN_IF(t_rearm.empty());

	const auto pdefer_before = t_pdefer_context;

	t_pdefer_context = this;

	// - the entries of this ring (the others stay for their ring)
	auto iter_keep = t_rearm.begin();

	for (auto iter = t_rearm.begin(); iter != t_rearm.end(); ++iter)
	{
		if (iter->pring_owner != this)
		{
			*iter_keep++ = *iter;
			continue;
		}

		CONTINUE_IF(this->_is_armed(iter->handle));

		uint64_t event_data_now;
		uint32_t events_now;

		if (this->_process_get_event_mask(iter->handle, event_data_now, events_now) && event_data_now == iter->event_data)
			this->_submit_poll_add(iter->handle, event_data_now, events_now);
	}

	t_rearm.erase(iter_keep, t_rearm.end());
	t_pdefer_context = pdefer_before;
}

bool api::io_context_uring::attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.pring == nullptr, false);

	// 1) register executable (and the event mask : the re-arm of a registration without EPOLLONESHOT)
	const auto event_data = this->_process_register_handle(_handle, _pexecutable);

	this->_process_set_event_mask(_handle, _event);

	// 2) poll
	if (this->_submit_poll_add(_handle, event_data, _event) == false)
	{
		this->_process_unregister_handle(_handle);
		return false;
	}

	// 3) count
	++this->m_count_attached;

	return true;
}

bool api::io_context_uring::detach(CGHANDLE _handle) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.pring == nullptr, false);

	// 1) the registration (its user_data)
	uint64_t event_data;

	RETURN_IF(this->_process_get_event_data(_handle, event_data) == false, false);

	// 2) unregister (release the executable)
	this->_process_unregister_handle(_handle);

	// 3) cancel the poll (the handle is closed right after : the poll would stay on the file)
	this->_submit_poll_cancel(_handle, event_data);

	// 4) count
	--this->m_count_attached;

	return true;
}

bool api::io_context_uring::modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.pring == nullptr, false);

	// 1) the registration (a new executable replaces the registered one)
	uint64_t event_data;

	const auto result_update = (_pexecutable != nullptr) ? this->_process_update_handle(_handle, _pexecutable, event_data) : this->_process_get_event_data(_handle, event_data);

	RETURN_IF(result_update == false, false);

	// - the event mask (the re-arm of a registration without EPOLLONESHOT)
	this->_process_set_event_mask(_handle, _event);

	// 2) armed (a one shot poll not fired yet) => cancel it and poll again with the new events. fired => a new poll
	//    (_submit_poll_add() : under lockable_sq with the armed sequence. IORING_POLL_UPDATE_EVENTS is not used : the kernel
	//     rejects it with IORING_POLL_ADD_MULTI (EINVAL on 6.6) and a poll that fired meanwhile is not found by it anyway.
	//     the -ECANCELED CQE of the old poll is dropped)
	return this->_submit_poll_add(_handle, event_data, _event);
}

bool api::io_context_uring::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept
{
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);
	RETURN_IF(_pexecutable == nullptr, false);

	auto pexecutable_post = this->m_pexecutable_post.get();

	RETURN_IF(pexecutable_post == nullptr, false);

	return pexecutable_post->post(_completion_key, _pexecutable, false);
}

bool api::io_context_uring::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);

	auto pexecutable = _pexecutable.get();

	RETURN_IF(pexecutable == nullptr, false);

	auto pexecutable_post = this->m_pexecutable_post.get();

	RETURN_IF(pexecutable_post == nullptr, false);

	RETURN_IF(pexecutable_post->post(_completion_key, pexecutable, true) == false, false);

	_pexecutable.detach();

	return true;
}

int api::io_context_uring::pop_events(epoll_event* _events, int _max_events, chrono::tick::duration _ticks_wait) noexcept
{
	// check) (destroy() waits for the threads in here : RING_GUARD)
	RING_GUARD guard(*this);

	RETURN_IF(guard.pring == nullptr || _max_events <= 0, 0);

	auto* pring = guard.pring;

	// check) not a worker of an executor with workers : nothing is taken nor submitted (the requests would be this thread's, their
	//        completions its task work). the workers process the events (a thread pumping the executor, wait_complete_* : waits)
	if (this->_is_submitter() == false)
	{
		if (_ticks_wait > chrono::tick::duration::zero())
			std::this_thread::sleep_for(std::min(_ticks_wait, std::chrono::duration_cast<chrono::tick::duration>(std::chrono::milliseconds(2))));

		return 0;
	}

	// 1) submit the SQEs deferred by this thread (and any other pending) : one enter. (the re-arms of the last batch first, when
	//    the executor did not call flush_pending())
	this->_process_rearm_pending();

	{
		std::lock_guard lock(pring->lockable_sq);
		this->m_is_wake_submit.store(false, std::memory_order_release);
		pring->submit(0, 0, nullptr, 0);
	}

	// - this thread is in a batch : the SQEs it writes for this io_context while processing the events are submitted by its next
	//   pop_events()/flush_pending() (the SQEs of another io_context : as a thread that is not its worker)
	t_pdefer_context = this;

	// 2) wait (one thread in the kernel at a time : the others wait for the lock, no longer than their own wait)
	//    - the thread in the kernel holds the lock up to its wait (1s : the executor). a thread pumping the executor with a short
	//      wait (wait_complete_connecting / wait_complete_closed : 2ms) must not wait behind it (and behind the other workers
	//      competing for the mutex) : it returns with nothing and the thread in the kernel processes the events
	const bool is_wait_forever = (_ticks_wait == chrono::tick::duration::max());
	const auto tick_deadline = is_wait_forever ? chrono::tick::time_point::max() : chrono::tick::now() + _ticks_wait;

	std::unique_lock lock_cq(pring->lockable_cq, std::defer_lock);

	if (is_wait_forever)
		lock_cq.lock();
	else if (lock_cq.try_lock_for(_ticks_wait) == false)
		return 0;

	unsigned head = __atomic_load_n(pring->cq_head, __ATOMIC_ACQUIRE);
	unsigned tail = __atomic_load_n(pring->cq_tail, __ATOMIC_ACQUIRE);

	if (head == tail)
	{
		// check) disposed meanwhile
		RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, 0);

		// - the wait left after the lock
		chrono::tick::duration ticks_wait_left = _ticks_wait;

		if (is_wait_forever == false)
		{
			const auto tick_now = chrono::tick::now();

			RETURN_IF(tick_now >= tick_deadline, 0);

			ticks_wait_left = tick_deadline - tick_now;
		}

	#if CGDK_IO_URING_SUPPORTED
		__kernel_timespec timespec_wait{};
		io_uring_getevents_arg arg{};

		if (is_wait_forever == false)
		{
			const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(ticks_wait_left).count();
			timespec_wait.tv_sec = nanoseconds / 1000000000ll;
			timespec_wait.tv_nsec = nanoseconds % 1000000000ll;
			arg.ts = reinterpret_cast<uint64_t>(&timespec_wait);
		}

		const int result = _io_uring_enter(pring->fd, 0, 1, IORING_ENTER_GETEVENTS | IORING_ENTER_EXT_ARG, &arg, sizeof(arg));

		// - a timeout (ETIME), an interrupt (EINTR) or a closed ring => nothing
		static_cast<void>(result);
	#endif

		head = __atomic_load_n(pring->cq_head, __ATOMIC_ACQUIRE);
		tail = __atomic_load_n(pring->cq_tail, __ATOMIC_ACQUIRE);
	}

	// 3) take the CQEs
	int count = 0;

	// - the eventfd of the posts : armed again at once (below), not after the processing like a socket. executable_post takes a
	//   part of its queue per CQE under its own lock and signals the eventfd again while some are left : the next CQE goes to
	//   another thread (like the level triggered eventfd of epoll). armed after the processing, an executable that blocks kept
	//   every other post of the executor waiting (one thread took all the posts : a blocked handler stopped the executor)
	const uint64_t event_data_post = (this->m_pexecutable_post) ? this->m_pexecutable_post->m_event_data_post : 0;
	bool is_rearm_post = false;

	while (head != tail && count < _max_events)
	{
		const io_uring_cqe& cqe = pring->cqes[head & pring->cq_mask];
		++head;

		// - a cancel / update / nop : dropped
		CONTINUE_IF((cqe.user_data & TAG_IGNORE) != 0);

		// - the registration and the poll
		const auto event_data = _poll_event_data(cqe.user_data);
		const auto seq = _poll_seq(cqe.user_data);
		const auto handle = _get_event_handle(event_data);

		// - the poll completed : not armed any more when it is the armed poll (a poll replaced by modify() meanwhile : the
		//   armed one stays). a registration without EPOLLONESHOT (UDP, listen, the eventfd) is armed again by flush_pending() of
		//   this thread after the events are processed (like EPOLLET : the processing drains the socket, the arrivals after wake
		//   one CQE). the SQ is shared, so a poll written here would be submitted by any thread while this one still drains the
		//   socket => it fires at once and another thread processes the same socket
		const bool is_disarmed = this->_disarm(handle, seq);

		uint64_t event_data_now;
		uint32_t events_now;
		const bool is_registered = this->_process_get_event_mask(handle, event_data_now, events_now) && event_data_now == event_data;

		// - cancelled
		//   . by modify() / detach() : not the armed poll any more (or released) => dropped
		//   . the armed poll of a live registration : cancelled by the kernel. a request belongs to the thread that submitted it
		//     (the SQEs of the others too) and a poll that fires after that thread exited completes with -ECANCELED.
		//     => polled again by this thread (the readiness is checked when it is added : the event is not lost)
		if (cqe.res == -ECANCELED)
		{
			if (seq != 0 && is_disarmed && is_registered)
			{
				if (event_data == event_data_post)
					is_rearm_post = true;
				else
					t_rearm.push_back(REARM{ this, handle, event_data });
			}

			continue;
		}

		if (is_disarmed && is_registered && (events_now & EPOLLONESHOT) == 0)
		{
			if (event_data == event_data_post)
				is_rearm_post = true;
			else
				t_rearm.push_back(REARM{ this, handle, event_data });
		}

		// - the event (an error of the poll : EPOLLERR | EPOLLHUP closes the socket)
		_events[count].events = (cqe.res >= 0) ? static_cast<uint32_t>(cqe.res) : static_cast<uint32_t>(EPOLLERR | EPOLLHUP);
		_events[count].data.u64 = event_data;
		++count;
	}

	__atomic_store_n(pring->cq_head, head, __ATOMIC_RELEASE);

	// 4) the eventfd of the posts : polled again and submitted now (out of lockable_cq : another thread may wait in the kernel).
	//    the SQEs the threads that are not workers wrote go with it (they woke this one by the eventfd)
	if (is_rearm_post)
	{
		lock_cq.unlock();

		const auto pdefer_before = t_pdefer_context;

		t_pdefer_context = nullptr;
		this->_submit_poll_add(_get_event_handle(event_data_post), event_data_post, EPOLLIN);
		t_pdefer_context = pdefer_before;
	}

	return count;
}

void api::io_context_uring::get_result(const epoll_event& _event, RESULT& _result) const noexcept
{
	// 1) event flag
	_result.events = _event.events;

	// 2) get executable (add_ref. nullptr : the handle was closed)
	_result.data.ptr = this->_process_get_executable(_event.data.u64);
}

void api::io_context_uring::pop_executable(RESULT& _result, chrono::tick::duration _ticks_wait) noexcept
{
	epoll_event event_epoll;

	const auto count = this->pop_events(&event_epoll, 1, _ticks_wait);

	RETURN_IF(count <= 0);

	this->get_result(event_epoll, _result);
}

size_t api::io_context_uring::queued_count() const noexcept
{
	auto pexecutable_post = this->m_pexecutable_post.get();

	return (pexecutable_post != nullptr) ? pexecutable_post->get_queued_count() : 0;
}


//-----------------------------------------------------------------------------
// io_context_linux : epoll or io_uring
//-----------------------------------------------------------------------------
api::io_context_linux::io_context_linux()
{
}

api::io_context_linux::~io_context_linux() noexcept
{
	this->destroy();
}

bool api::io_context_linux::create(int _hint_size) noexcept
{
	// check) created already (a backend with a live handle)
	RETURN_IF(this->m_pepoll && this->m_pepoll->handle() != INVALID_CGHANDLE_VALUE, false);
	RETURN_IF(this->m_puring && this->m_puring->handle() != INVALID_CGHANDLE_VALUE, false);
	RETURN_IF(this->m_pcq && this->m_pcq->handle() != INVALID_CGHANDLE_VALUE, false);

	// - the backends destroyed before (a restart of the executor) are released here : no thread uses them now
	this->m_pepoll.reset();
	this->m_puring.reset();
	this->m_pcq.reset();

	// 1) io_uring_completion when asked for and available (6.0+)
	if (executor::default_io_backend() == executor::io_backend::io_uring_completion)
	{
		if (api::io_context_uring_cq::is_available())
		{
			auto pcq = std::make_unique<OBJ<io_context_uring_cq>>();

			if (pcq->create(_hint_size))
			{
				this->m_pcq = std::move(pcq);
				return true;
			}

			LOG_WARNING << "(warn) io_context: fail to create the io_uring completion backend. epoll is used"sv;
		}
		else
		{
			LOG_WARNING << "(warn) io_context: the io_uring completion backend is not available on this kernel (6.0+ needed, or io_uring disabled). epoll is used"sv;
		}
	}

	// 2) io_uring when asked for and available
	if (executor::default_io_backend() == executor::io_backend::io_uring)
	{
		if (api::io_context_uring::is_available())
		{
			auto puring = std::make_unique<OBJ<io_context_uring>>();

			if (puring->create(_hint_size))
			{
				this->m_puring = std::move(puring);
				return true;
			}

			LOG_WARNING << "(warn) io_context: fail to create the io_uring ring. epoll is used"sv;
		}
		else
		{
			LOG_WARNING << "(warn) io_context: io_uring is not available on this kernel (5.13+ needed, or disabled). epoll is used"sv;
		}
	}

	// 3) epoll
	auto pepoll = std::make_unique<OBJ<io_context_epoll_direct>>();

	RETURN_IF(pepoll->create(_hint_size) == false, false);

	this->m_pepoll = std::move(pepoll);

	return true;
}

CGHANDLE api::io_context_linux::dispose() noexcept
{
	if (this->m_pcq) return this->m_pcq->dispose();
	if (this->m_puring) return this->m_puring->dispose();
	if (this->m_pepoll) return this->m_pepoll->dispose();
	return INVALID_CGHANDLE_VALUE;
}

result_code api::io_context_linux::destroy() noexcept
{
	// - the objects are kept : a thread closing a socket while the executor stops still calls set_event()/detach() through them
	//   (they answer false once destroyed). released at the next create() or with this object
	if (this->m_pcq) this->m_pcq->destroy();
	if (this->m_puring) this->m_puring->destroy();
	if (this->m_pepoll) this->m_pepoll->destroy();
	return eRESULT::SUCCESS;
}

int api::io_context_linux::pop_results(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept
{
	// 1) io_uring_completion : the ring of this thread
	if (this->m_pcq)
		return this->m_pcq->pop_results(_results, _max, _ticks_wait);

	// 2) epoll / io_uring : the events and their executables (an event of a handle closed meanwhile : no executable, skipped)
	RETURN_IF(_results == nullptr || _max <= 0, 0);

	epoll_event array_event[MAX_EVENT_BATCH];

	const int count_event = this->pop_events(array_event, std::min(_max, MAX_EVENT_BATCH), _ticks_wait);
	int count = 0;

	for (int i = 0; i < count_event; ++i)
	{
		RESULT result_event;

		this->get_result(array_event[i], result_event);

		CONTINUE_IF(result_event.is_valid() == false);

		_results[count].pexecutable = result_event.executable();
		_results[count].result = result_event.result();
		_results[count].param = result_event.param();
		++count;
	}

	return count;
}

}

#else

namespace CGDK
{
	// - windows : iocp only
	void executor::set_default_io_backend(executor::io_backend /*_backend*/) noexcept {}
	executor::io_backend executor::default_io_backend() noexcept { return executor::io_backend::epoll; }
	bool executor::is_io_uring_available() noexcept { return false; }
	bool executor::is_io_uring_completion_available() noexcept { return false; }
}

#endif
