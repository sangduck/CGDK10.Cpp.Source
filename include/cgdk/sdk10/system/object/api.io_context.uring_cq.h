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
#include <deque>
#include <condition_variable>
#include <sys/socket.h>

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		io_context_uring_cq

 the io_uring "completion" backend of the io_context (linux 6.0+) : one ring per worker thread and an op submission
 API (recv/send/sendmsg/recvmsg/accept/connect/cancel) whose completions are delivered as
 Iexecutable::process_execute(res, flags) - the IOCP model. the readiness interface of io_context_Nepoll
 (attach / modify / detach : IORING_OP_POLL_ADD on the registry) is served too : the socket code of the readiness
 model runs unchanged on it.

  - rings      : a worker thread binds a ring at its start (bind_worker(), called by the executor thread) and unbinds it
                 at its end. the CQ of a ring is consumed by its owner only (no CQ lock). a ring whose worker left
                 (orphan) is adopted by the next worker that binds, and its CQ is drained by the other workers meanwhile.
  - handles    : a handle is bound to a ring at its first attach()/submit (the ring of the submitting worker, else round
                 robin). the polls and the ops of the handle go to that ring. a handle bound to an orphan ring moves to
                 the ring of the next worker that submits for it. an accepted socket (the CQE of an accept op) is bound
                 round robin when its CQE is taken : the sockets of an acceptor spread over the workers.
  - submission : a request belongs to the thread that submitted it (it completes with -ECANCELED when that thread exits), so
                 a poll or an op is submitted by the owner of the ring only : any thread stages it under the SQ lock, the owner
                 moves the staged SQEs into the SQ and submits them with the io_uring_enter that waits (one syscall per batch).
                 only the owner enters its ring (an enter submits every SQE of the SQ : the flushed SQEs of the owner too) :
                 the poll removes and the async cancels of the other threads are staged as well, and a thread staging on a ring
                 it does not own wakes the owner once with the wake eventfd of the ring (polled multishot by the owner : a thread
                 that submitted to a ring, an IORING_OP_MSG_RING too, gets a task work when the ring is released = EINTR in its
                 blocking calls). the staged SQEs of a ring without an owner move to the ring of the worker that drains it. an op
                 cancelled while staged is delivered with -ECANCELED by the owner like a cancelled request.
  - ops        : the submit takes one add_ref() of the executable, released by the executor after the delivery of the
                 final CQE. a CQE with IORING_CQE_F_MORE (multishot accept, the first CQE of a zero copy send) takes one
                 more add_ref() before its delivery. a cancelled op completes with -ECANCELED and is delivered.
  - post       : a shared queue. a post wakes one owned ring (round robin) with a nop SQE (its CQE wakes the owner).
                 a thread that owns no ring (not a worker of this io_context) pops the post queue only.
  - ring size  : 256 SQ entries per ring (CQ x4, ~36KB) : the ring memory counts against RLIMIT_MEMLOCK of the user
                 (8MB on ubuntu : ~225 rings). ENOMEM => a smaller ring (down to 32).
  - raw syscalls (no liburing) : nothing to link.
  - multishot recv : submit_recv_multishot() receives into the provided buffer ring of the ring of the fd (registered at the
                 first one : PROVIDED_BUFFER_COUNT x PROVIDED_BUFFER_SIZE). a CQE per arrival with IORING_CQE_F_BUFFER (the param
                 carries the ring and the buffer : get_provided_buffer()), the buffer is given back by return_buffer() from any
                 thread (the tail of the buffer ring is written under a lock of its own : not a request). the CQEs of a ring run on
                 one thread at a time in CQ order (an orphan ring stays with the worker that drained it till it executed them).

*///-----------------------------------------------------------------------------
class api::io_context_uring_cq : public io_context_Nepoll
{
// constructor/destructor)
public:
			io_context_uring_cq();
	virtual	~io_context_uring_cq() noexcept;

public:
	//! @brief the result of pop_results() : an executable (add_ref'd. nullptr : skip), the result and the param of its process_execute()
	struct POP_RESULT
	{
		Iexecutable*		pexecutable;
		intptr_t			result;
		size_t				param;
	};

	static	constexpr int		MAX_EVENT_BATCH = 32;

	//! @brief the provided buffers of a ring (multishot recv) : the size of a buffer, the count per ring (a power of 2)
	static	constexpr std::size_t PROVIDED_BUFFER_SIZE = 16 * 1024;
	static	constexpr unsigned	PROVIDED_BUFFER_COUNT = 256;

public:
	//! @brief the kernel supports what the backend needs (6.0+ : SEND_ZC, multishot accept, async cancel by fd, sync cancel). cached
	[[nodiscard]] static bool	is_available() noexcept;

			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

	// - readiness (the registry of io_context_Nepoll : one poll per registration, like io_context_uring)
			bool				attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept;
			bool				detach(CGHANDLE _handle) noexcept;
			bool				modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable) noexcept;

	// - post
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, Iexecutable* _pexecutable) noexcept;
			bool				post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _executable) noexcept;
			[[nodiscard]] size_t queued_count() const noexcept;

	// - the worker threads (the executor calls them at the start/end of a thread)
	//! @brief binds the ring of this worker thread. a transient worker (exits while the io_context lives : the kernel cancels the requests of an exiting thread) binds none and pops the post queue only
			void				bind_worker(bool _is_transient = false) noexcept;
			void				unbind_worker() noexcept;
			[[nodiscard]] bool	is_worker() const noexcept;
			[[nodiscard]] size_t ring_count() const noexcept { return this->m_count_rings.load(std::memory_order_acquire); }

	//! @brief waits on the ring of this thread (a worker) and fills the results (up to _max). a thread without a ring pops the post queue only
			int					pop_results(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept;

	//! @brief the batch of results this thread executes now (the executor sets it around its loop : *pcount is the count it executes, index
	//!        the result executing now). a worker that blocks hands the rest of the batch over (thread_blocked() cuts *pcount)
	struct BATCH
	{
		POP_RESULT*			presults;
		int*				pcount;
		int					index;
		const void*			pcontext;			// - the io_context_uring_cq of the results (nullptr : another backend)
		BATCH*				pouter;				// - the batch of an outer loop (a handler pumping the executor)
		void*				pring = nullptr;	// - the ring the batch is published on (batch_begin() : the outermost batch of a worker)
	};
	[[nodiscard]] static BATCH*& current_batch() noexcept;

	//! @brief the batch is published on the ring of this worker (begin / end) and its results are claimed one by one (next : false when the
	//!        rest was taken, *pcount cut). a worker finding one executable running BUSY_LEND (CPU in user space) lends the ring like
	//!        thread_blocked() and takes the rest of the batch (_lend_busy)
	static	void				batch_begin(BATCH& _batch) noexcept;
	static	bool				batch_next(BATCH& _batch, int _index) noexcept;
	static	void				batch_end(BATCH& _batch) noexcept;

	//! @brief this thread blocks (scoped_thread_block, a wait pumping the executor) / unblocks. the ring of a worker is lent meanwhile : the other
	//!        workers execute its CQEs (the task work wakes a sleeping owner, so they are posted), the executable the owner blocked in gets its next
	//!        results after the owner unblocked (in order : the CQEs of one executable never run concurrently). nested : the outermost counts
			void				thread_blocked() noexcept;
			void				thread_unblocked() noexcept;
	//! @brief the re-arms of the events this thread processed are written (submitted by its next pop_results())
			void				flush_pending() noexcept;

	// - ops (Iio_context) : _pexecutable gets add_ref() per submitted op, released by the executor after the delivery of the final CQE
			bool				submit_recv(CGHANDLE _fd, void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable) noexcept;
			bool				submit_recvmsg(CGHANDLE _fd, msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable) noexcept;
			bool				submit_send(CGHANDLE _fd, const void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept;
			bool				submit_sendmsg(CGHANDLE _fd, const msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept;
			bool				submit_accept(CGHANDLE _listen_fd, sockaddr* _addr, socklen_t* _addrlen, int _flags, bool _multishot, Iexecutable* _pexecutable) noexcept;
			bool				submit_connect(CGHANDLE _fd, const sockaddr* _addr, socklen_t _addrlen, Iexecutable* _pexecutable) noexcept;
			bool				submit_cancel(Iexecutable* _pexecutable) noexcept;
			bool				submit_cancel_fd(CGHANDLE _fd) noexcept;
	//! @brief cancels the ops of the fd in the kernel synchronously (the file is released when it returns) and, with _is_wait_delivery, waits for the delivery of their completions
			bool				wait_cancelled_fd(CGHANDLE _fd, chrono::tick::duration _ticks_wait, bool _is_wait_delivery = true) noexcept;
			[[nodiscard]] int	inflight_count(CGHANDLE _fd) const noexcept;

	// - multishot recv + provided buffers
	//! @brief a multishot RECV into the provided buffers of the ring of the fd : one submit, a CQE per arrival (IORING_CQE_F_BUFFER : the
	//!        data in get_provided_buffer(param), given back by return_buffer(param). IORING_CQE_F_MORE : more follow). a CQE without F_MORE
	//!        ends it (-ENOBUFS : the buffers of the ring ran out, -ECANCELED, 0 : the peer closed, an error, or data : submitted again by
	//!        the caller). false : not available (CGDK_IO_URING_MULTISHOT=0, the kernel refused the buffer ring) : submit_recv()
			bool				submit_recv_multishot(CGHANDLE _fd, Iexecutable* _pexecutable) noexcept;
	//! @brief the data of the provided buffer of a completion (_param : the param of the delivery, with IORING_CQE_F_BUFFER). nullptr : none
	[[nodiscard]] const void*	get_provided_buffer(size_t _param) const noexcept;
	//! @brief gives the provided buffer of a completion back to its ring (any thread, once per completion with IORING_CQE_F_BUFFER)
			bool				return_buffer(size_t _param) noexcept;
	//! @brief the size of a provided buffer (the multishot recv switched off by CGDK_IO_URING_MULTISHOT=0 : submit_recv_multishot() fails anyway)
	[[nodiscard]] size_t		provided_buffer_size() const noexcept { return PROVIDED_BUFFER_SIZE; }
	//! @brief this thread is the owner of the ring the fd is bound to (an op it submits goes with its next enter : no wake, no syscall of its own)
	[[nodiscard]] bool			is_ring_owner(CGHANDLE _fd) const noexcept;
	//! @brief the owner of the ring of the fd submits the ops staged on it now when one of the fd is staged (flush + submit, no wait).
	//!        false : this thread is not the owner (or the ring is lent) or nothing of the fd is staged
			bool				submit_staged(CGHANDLE _fd) noexcept;
	//! @brief the provided buffers delivered and not given back yet (every ring)
	[[nodiscard]] size_t		provided_buffer_out_count() const noexcept;

// implementation)
private:
			struct RING;
			struct RING_GUARD;
			struct OP;
			struct HANDLE_ENTRY;
			friend struct RING_GUARD;

			//! @brief the entry of a handle (its chunk allocated now when none. nullptr : out of range or no memory)
			HANDLE_ENTRY*		_entry(CGHANDLE _handle) noexcept;
			//! @brief the entry of a handle when its chunk exists (nullptr : never used : nothing armed, bound or in flight)
			[[nodiscard]] HANDLE_ENTRY* _find_entry(CGHANDLE _handle) const noexcept;
			void				_free_handle_chunks() noexcept;

	static	constexpr unsigned	MAX_RINGS = 2048;
	static	constexpr unsigned	HANDLE_STRIPES = 256;

			RING*				_create_ring(bool _only_if_none) noexcept;
			RING*				_create_ring_locked() noexcept;
			RING*				_find_worker_ring() const noexcept;
			RING*				_ring_of_handle(CGHANDLE _handle, RING*& _pring_before) noexcept;
			void				_bind_round_robin(CGHANDLE _handle, std::atomic<unsigned>& _index_next) noexcept;
			RING*				_ring_round_robin() noexcept;
	[[nodiscard]] bool			_has_orphan() const noexcept;
			bool				_wake_ring() noexcept;

			bool				_submit_poll_add(CGHANDLE _handle, uint64_t _event_data, uint32_t _event) noexcept;
			bool				_submit_poll_cancel(CGHANDLE _handle, uint64_t _event_data) noexcept;
			void				_wake_staged(RING* _pring) noexcept;
			bool				_send_wake(RING* _pring) noexcept;
			bool				_purge_staged_poll(RING* _pring, uint64_t _user_data) noexcept;
			void				_adopt_staged(RING* _pring_from, RING* _pring_to) noexcept;
			void				_process_rearm_pending() noexcept;
			bool				_disarm(CGHANDLE _handle, uint8_t _seq) noexcept;
	[[nodiscard]] bool			_is_armed(CGHANDLE _handle) const noexcept;

			template <class TFILL>
			bool				_submit_op(CGHANDLE _fd, Iexecutable* _pexecutable, TFILL&& _fill) noexcept;
			template <class TPREPARE, class TFILL>
			bool				_submit_op_ex(CGHANDLE _fd, Iexecutable* _pexecutable, TPREPARE&& _prepare, TFILL&& _fill) noexcept;
			bool				_prepare_pbuf(RING* _pring) noexcept;
			void				_release_draining() noexcept;
			bool				_take_lent(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept;
			bool				_deliver_lent(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept;
			void				_release_lent(RING* _pring) noexcept;
			void				_check_busy(RING* _pring_own) noexcept;
			bool				_lend_busy(RING* _pring, uint64_t _word) noexcept;
			void				_reclaim_busy(RING* _pring) noexcept;
	[[nodiscard]] bool			_is_busy(const RING* _pring_own) const noexcept;
			void				_hand_watch(RING* _pring_own) noexcept;
			template <class TMATCH>
			bool				_cancel_ops(TMATCH&& _match) noexcept;
			template <class TMATCH>
			bool				_cancel_staged(RING* _pring, TMATCH&& _match) noexcept;
			void				_cancel_now(RING* _pring, uint64_t _user_data) noexcept;
			void				_complete_op_final(OP* _pop) noexcept;
			void				_deliver_synthetic(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept;
			OP*					_alloc_op() noexcept;
			void				_free_op(OP* _pop) noexcept;
			int					_sync_cancel(RING* _pring, uint64_t _user_data, int _fd, uint32_t _flags, chrono::tick::duration _ticks_wait) noexcept;

			void				_process_cqe(RING* _pring, const void* _pcqe, POP_RESULT* _results, int& _count, int _max) noexcept;
			void				_process_wake(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept;
			void				_process_wake_poll(RING* _pring, const void* _pcqe) noexcept;
			int					_drain_ring(RING* _pring, POP_RESULT* _results, int _count, int _max) noexcept;
			void				_drain_orphans(POP_RESULT* _results, int& _count, int _max) noexcept;
			int					_pop_posted(POP_RESULT* _results, int _count, int _max, bool _is_ring) noexcept;
			int					_pop_posted_wait(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept;

			std::unique_ptr<RING>	m_rings[MAX_RINGS];				// - the rings (created at bind_worker() or on demand. kept till the object goes : a worker unbinding touches its ring)
			std::atomic<unsigned>	m_count_rings{ 0 };
			std::mutex			m_lockable_rings;

			// - the state of a handle (HANDLE_ENTRY : the armed poll, the ring it is bound to, the ops in flight) in chunks allocated on
			//   the first use of a handle number in the chunk (the open file limit is raised after the io_context is created : the
			//   net socket initialization raises it to the hard limit. a table sized by the limit at create() refused the higher fds)
			std::unique_ptr<std::atomic<HANDLE_ENTRY*>[]> m_handle_chunks;
			unsigned			m_count_handles{ 0 };				// - the handles tracked (MAX_HANDLE while created, 0 : not created)
			std::mutex			m_lockable_handle[HANDLE_STRIPES];	// - armed / binding of a handle are written under its stripe
			std::atomic<uint32_t>	m_seq_poll{ 0 };
			std::atomic<unsigned>	m_index_ring_next{ 0 };

			std::atomic<int>	m_count_users{ 0 };					// - threads using the rings (pop_results, modify ... : destroy() waits for them before the rings are unmapped)
			int					m_event_fd_handle{ -1 };			// - handle() : an eventfd of its own (handle() != INVALID while created)

			// - the post queue (shared by the rings)
	mutable	std::mutex			m_lockable_post;
			std::condition_variable m_cv_post;
			std::deque<Iexecutable*> m_queue_post;
			std::atomic<std::size_t> m_count_post{ 0 };				// - the size of m_queue_post (written under m_lockable_post : read without it by an owner about to wait)
			std::atomic<int>	m_count_lent{ 0 };					// - the rings lent (STATE_LENT / STATE_DRAINING_LENT : their owner blocked) : the idle workers wait no longer than LENT_WAIT
			std::atomic<int64_t> m_tick_check_busy{ 0 };			// - the next check of the busy rings (_check_busy : one worker per BUSY_CHECK, ns of chrono::tick)
			std::atomic<RING*>	m_pring_watch{ nullptr };			// - the idle owner that waits no longer than BUSY_CHECK / BUSY_WATCH (the busy rings are checked while every worker idles)
			bool				m_is_signaled{ false };				// - a ring was woken and did not take the queue yet
			bool				m_is_post_closed{ false };
			int					m_count_sleepers{ 0 };				// - threads without a ring waiting on m_cv_post
			std::atomic<int>	m_count_signaling{ 0 };

			// - the pool of OP
			std::mutex			m_lockable_pool;
			std::vector<OP*>	m_pool_op;
};

}

#endif
