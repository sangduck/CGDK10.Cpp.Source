//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

#if defined(__ANDROID__) || defined(__linux__)

#include <memory>

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		io_context_uring

 the io_uring backend of the io_context (linux 5.13+). the same interface as io_context_epoll_direct :
 readiness events (IORING_OP_POLL_ADD) with the registry (handle, executable, generation) of io_context_Nepoll.

  - EPOLLONESHOT    : a one shot poll. set_event() (re-arm) after the event : IORING_OP_POLL_ADD again. while the poll
                      is still armed (EPOLLOUT added by a sender) : IORING_OP_POLL_REMOVE + POLL_ADD.
  - no EPOLLONESHOT : (listen sockets, UDP, the post eventfd) the poll is armed again by flush_pending() of the thread that
                      processed its event (a multishot poll posted a CQE per wakeup : no coalescing. like EPOLLET).
  - the SQEs of a worker thread (the re-arms of the events it processed) are submitted together by its next
    pop_events() / flush_pending() : one io_uring_enter for a batch (an epoll_ctl per event before).
  - only the worker threads of the executor (bind_worker(), after expect_workers()) enter the ring : a request belongs to the
    thread that submitted it and its completion runs as the task work of that thread (a blocking call with a timeout of that
    thread returned EINTR). the SQEs of the other threads wait in the SQ and they wake a worker (the post eventfd), they take no
    event either. an io_context without workers (no expect_workers()) is entered by any thread. the ring is created by a thread
    that ends at once (the kernel sends a task work to every thread that created a ring or submitted to it when it is released).
  - one waiter at a time (the CQ is read by one thread) : the others wait on a lock, not in the kernel.
  - one poll per registration (the armed sequence in the user_data). detach() cancels it and waits till it is gone
    (IORING_REGISTER_SYNC_CANCEL, 6.0+) : a poll holds the file (a closed socket stays open till its poll ends).
  - a poll cancelled by the kernel (the thread that submitted it exited : -ECANCELED when it fires) is polled again.
  - raw syscalls (no liburing) : nothing to link.

*///-----------------------------------------------------------------------------
class api::io_context_uring : public io_context_Nepoll
{
// constructor/destructor)
public:
			io_context_uring();
	virtual	~io_context_uring() noexcept;

public:
	//! @brief the kernel supports what the backend needs (5.13+ : poll update/multishot, ext_arg). cached
	[[nodiscard]] static bool	is_available() noexcept;

			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

			bool				attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept;
			bool				detach(CGHANDLE _handle) noexcept;
			bool				modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable) noexcept;

			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept;
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _executable) noexcept;

			void				pop_executable(RESULT& _result_event, chrono::tick::duration _ticks_wait) noexcept;
			[[nodiscard]] size_t queued_count() const noexcept;

	static	constexpr int		MAX_EVENT_BATCH = 32;
			int					pop_events(epoll_event* _events, int _max_events, chrono::tick::duration _ticks_wait) noexcept;
			void				get_result(const epoll_event& _event, RESULT& _result) const noexcept;

	//! @brief submits the SQEs this thread deferred (the re-arms of the events it processed). called by the executor after a batch
			void				flush_pending() noexcept;

	//! @brief the executor has worker threads (called before they start) : only they enter the ring from now
			void				expect_workers() noexcept;
	//! @brief a worker thread of the executor (at its start / its end)
			void				bind_worker() noexcept;
			void				unbind_worker() noexcept;

// implementation)
private:
			class executable_post;
			friend class executable_post;
			struct RING;

			bool				_submit_poll_add(CGHANDLE _handle, uint64_t _event_data, uint32_t _event) noexcept;
			bool				_submit_poll_cancel(CGHANDLE _handle, uint64_t _event_data) noexcept;
			bool				_submit_nop() noexcept;
			void				_flush_locked() noexcept;
	[[nodiscard]] bool			_is_worker() const noexcept;
	[[nodiscard]] bool			_is_submitter() const noexcept;
			void				_wake_submitter() noexcept;
			void				_wait_sq_free(std::unique_lock<std::mutex>& _lock, unsigned _count) noexcept;
			void				_process_rearm_pending() noexcept;
			bool				_disarm(CGHANDLE _handle, uint8_t _seq) noexcept;
	[[nodiscard]] bool			_is_armed(CGHANDLE _handle) const noexcept;

			std::unique_ptr<RING>	m_pring;
			std::unique_ptr<std::atomic<uint8_t>[]> m_armed;		// - the sequence of the armed poll (0 : none. indexed by the handle. written under lockable_sq, cleared by its CQE)
			unsigned			m_count_armed{ 0 };
			std::atomic<int>	m_count_users{ 0 };						// - threads using the ring (pop_events, modify ... : destroy() waits for them before the rings are unmapped)
			std::atomic<bool>	m_is_worker_expected{ false };			// - expect_workers() : only the workers enter the ring
			std::atomic<bool>	m_is_wake_submit{ false };				// - a thread that is not a worker woke the workers to submit its SQEs (cleared by the submit of a worker)
			struct RING_GUARD;
			own_ptr<executable_post> m_pexecutable_post;
};


//-----------------------------------------------------------------------------
/**

 @class		io_context_linux

 the io_context of linux : epoll (io_context_epoll_direct), io_uring (io_context_uring : readiness) or io_uring_completion
 (io_context_uring_cq : a ring per worker thread, the op API), chosen at create() by executor::default_io_backend()
 (set_default_io_backend(), env CGDK_IO_URING=1 / completion). a backend that is not available falls back to epoll.

  - pop_results() : the backend neutral pop of the executor (the executable add_ref'd, its result and param).
    epoll / io_uring : pop_events() + get_result(). io_uring_completion : the ring of the worker thread.
  - bind_worker() / unbind_worker() : the worker threads of the executor (io_uring_completion : their ring)

*///-----------------------------------------------------------------------------
class api::io_context_linux :
	virtual public				object::Idestroyable,
	private						non_copyable
{
public:
			using RESULT = io_context_Nepoll::RESULT;
			using POP_RESULT = io_context_uring_cq::POP_RESULT;
	static	constexpr int		MAX_EVENT_BATCH = io_context_epoll_direct::MAX_EVENT_BATCH;

public:
			io_context_linux();
	virtual	~io_context_linux() noexcept;

public:
			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

			bool				attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept { return (m_pcq) ? m_pcq->attach(_handle, _pexecutable, _event) : (m_puring) ? m_puring->attach(_handle, _pexecutable, _event) : (m_pepoll ? m_pepoll->attach(_handle, _pexecutable, _event) : false); }
			bool				detach(CGHANDLE _handle) noexcept { return (m_pcq) ? m_pcq->detach(_handle) : (m_puring) ? m_puring->detach(_handle) : (m_pepoll ? m_pepoll->detach(_handle) : false); }
			bool				modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->modify(_handle, _event, _pexecutable) : (m_puring) ? m_puring->modify(_handle, _event, _pexecutable) : (m_pepoll ? m_pepoll->modify(_handle, _event, _pexecutable) : false); }

			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->post_executable(_bytes_transferred, _completion_key, _pexecutable) : (m_puring) ? m_puring->post_executable(_bytes_transferred, _completion_key, _pexecutable) : (m_pepoll ? m_pepoll->post_executable(_bytes_transferred, _completion_key, _pexecutable) : false); }
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _executable) noexcept { return (m_pcq) ? m_pcq->post_executable(_bytes_transferred, _completion_key, std::move(_executable)) : (m_puring) ? m_puring->post_executable(_bytes_transferred, _completion_key, std::move(_executable)) : (m_pepoll ? m_pepoll->post_executable(_bytes_transferred, _completion_key, std::move(_executable)) : false); }

			void				pop_executable(RESULT& _result_event, chrono::tick::duration _ticks_wait) noexcept { if (m_puring) m_puring->pop_executable(_result_event, _ticks_wait); else if (m_pepoll) m_pepoll->pop_executable(_result_event, _ticks_wait); }
			int					pop_events(epoll_event* _events, int _max_events, chrono::tick::duration _ticks_wait) noexcept { return (m_puring) ? m_puring->pop_events(_events, _max_events, _ticks_wait) : (m_pepoll ? m_pepoll->pop_events(_events, _max_events, _ticks_wait) : 0); }
			void				get_result(const epoll_event& _event, RESULT& _result) const noexcept { if (m_puring) m_puring->get_result(_event, _result); else if (m_pepoll) m_pepoll->get_result(_event, _result); }
			void				flush_pending() noexcept { if (m_pcq) m_pcq->flush_pending(); else if (m_puring) m_puring->flush_pending(); }

			//! @brief the pop of the executor : up to _max results (the executable add_ref'd, nullptr : skip). returns the count
			int					pop_results(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept;
			//! @brief a worker thread of the executor (io_uring_completion : its ring. io_uring : only the workers enter the ring. epoll : nothing)
			void				bind_worker(bool _is_transient = false) noexcept { if (m_pcq) m_pcq->bind_worker(_is_transient); else if (m_puring) m_puring->bind_worker(); }
			void				unbind_worker() noexcept { if (m_pcq) m_pcq->unbind_worker(); else if (m_puring) m_puring->unbind_worker(); }
	//! @brief the executor has worker threads (called before they start. io_uring : the other threads do not enter the ring from now)
			void				expect_workers() noexcept { if (m_puring) m_puring->expect_workers(); }
	//! @brief this thread blocks / unblocks (io_uring_completion : the ring of a worker is executed by the other workers meanwhile. the others : nothing)
			void				thread_blocked() noexcept { if (m_pcq) m_pcq->thread_blocked(); }
			void				thread_unblocked() noexcept { if (m_pcq) m_pcq->thread_unblocked(); }
	//! @brief the backend of the batches (io_context_uring_cq::BATCH::pcontext : io_uring_completion only, nullptr on the others)
	[[nodiscard]] const void*	completion_context() const noexcept { return static_cast<const io_context_uring_cq*>(m_pcq.get()); }

			// - the op API (io_uring_completion only : false on the others)
			bool				submit_recv(CGHANDLE _fd, void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_recv(_fd, _buf, _len, _msg_flags, _pexecutable) : false; }
			bool				submit_recvmsg(CGHANDLE _fd, msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_recvmsg(_fd, _msg, _msg_flags, _pexecutable) : false; }
			bool				submit_send(CGHANDLE _fd, const void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept { return (m_pcq) ? m_pcq->submit_send(_fd, _buf, _len, _msg_flags, _pexecutable, _zerocopy) : false; }
			bool				submit_sendmsg(CGHANDLE _fd, const msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept { return (m_pcq) ? m_pcq->submit_sendmsg(_fd, _msg, _msg_flags, _pexecutable, _zerocopy) : false; }
			bool				submit_accept(CGHANDLE _listen_fd, sockaddr* _addr, socklen_t* _addrlen, int _flags, bool _multishot, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_accept(_listen_fd, _addr, _addrlen, _flags, _multishot, _pexecutable) : false; }
			bool				submit_connect(CGHANDLE _fd, const sockaddr* _addr, socklen_t _addrlen, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_connect(_fd, _addr, _addrlen, _pexecutable) : false; }
			bool				submit_recv_multishot(CGHANDLE _fd, Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_recv_multishot(_fd, _pexecutable) : false; }
	[[nodiscard]] const void*	get_provided_buffer(size_t _param) const noexcept { return (m_pcq) ? m_pcq->get_provided_buffer(_param) : nullptr; }
			bool				return_buffer(size_t _param) noexcept { return (m_pcq) ? m_pcq->return_buffer(_param) : false; }
	[[nodiscard]] size_t		provided_buffer_size() const noexcept { return (m_pcq) ? m_pcq->provided_buffer_size() : 0; }
	[[nodiscard]] bool			is_ring_owner(CGHANDLE _fd) const noexcept { return (m_pcq) ? m_pcq->is_ring_owner(_fd) : false; }
			bool				submit_cancel(Iexecutable* _pexecutable) noexcept { return (m_pcq) ? m_pcq->submit_cancel(_pexecutable) : false; }
			bool				submit_cancel_fd(CGHANDLE _fd) noexcept { return (m_pcq) ? m_pcq->submit_cancel_fd(_fd) : false; }
			bool				wait_cancelled_fd(CGHANDLE _fd, chrono::tick::duration _ticks_wait, bool _is_wait_delivery = true) noexcept { return (m_pcq) ? m_pcq->wait_cancelled_fd(_fd, _ticks_wait, _is_wait_delivery) : false; }

			// - busy poll of epoll_wait() (io_context_Nepoll::set_busy_poll). io_uring : not supported (false)
			bool				set_busy_poll(uint32_t _usecs, uint16_t _budget = 0, bool _prefer = false) noexcept { return (m_pepoll) ? m_pepoll->set_busy_poll(_usecs, _budget, _prefer) : false; }
			bool				get_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) const noexcept { _usecs = 0; _budget = 0; _prefer = false; return (m_pepoll) ? m_pepoll->get_busy_poll(_usecs, _budget, _prefer) : false; }

			[[nodiscard]] size_t queued_count() const noexcept { return (m_pcq) ? m_pcq->queued_count() : (m_puring) ? m_puring->queued_count() : (m_pepoll ? m_pepoll->queued_count() : 0); }
			[[nodiscard]] CGHANDLE handle() const noexcept { return (m_pcq) ? m_pcq->handle() : (m_puring) ? m_puring->handle() : (m_pepoll ? m_pepoll->handle() : INVALID_CGHANDLE_VALUE); }
			[[nodiscard]] size_t attached_count() const noexcept { return (m_pcq) ? m_pcq->attached_count() : (m_puring) ? m_puring->attached_count() : (m_pepoll ? m_pepoll->attached_count() : 0); }

	//! @brief the backend in use (after create()). a backend that was asked for but is not available : epoll
	[[nodiscard]] executor::io_backend backend() const noexcept { return (m_pcq) ? executor::io_backend::io_uring_completion : (m_puring) ? executor::io_backend::io_uring : executor::io_backend::epoll; }

private:
			std::unique_ptr<OBJ<io_context_epoll_direct>> m_pepoll;		// - OBJ<> : the referenceable implementation (make_object/OBJ create CGDK objects)
			std::unique_ptr<OBJ<io_context_uring>> m_puring;
			std::unique_ptr<OBJ<io_context_uring_cq>> m_pcq;
};

}

#endif
