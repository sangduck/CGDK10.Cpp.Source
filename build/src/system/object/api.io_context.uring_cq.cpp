#include "pch.h"
#include "platform.h"

//-----------------------------------------------------------------------------
// io_context_uring_cq : the io_uring "completion" backend of the io_context (linux 6.0+)
//
//  - one ring per worker thread (bind_worker()/unbind_worker() by the executor thread). the CQ of a ring is consumed by
//    its owner only : no CQ lock, the owner waits in the kernel (io_uring_enter GETEVENTS) and submits its pending SQEs
//    with the same syscall.
//  - a ring whose worker left (an extra thread of the executor, a stop) is an orphan : its CQ is drained by the other
//    workers after their own (the requests of the exited thread completed with -ECANCELED : a readiness poll of a live
//    registration is armed again on the ring of the draining worker, an op is delivered with -ECANCELED). the next worker
//    that binds adopts an orphan ring before it creates a new one.
//  - a handle is bound to a ring at its first attach()/submit (HANDLE_ENTRY::binding) : the ring of the submitting worker (locality),
//    else round robin over the owned rings, else ring 0 (created on demand, adopted by the first worker). the polls and
//    the ops of the handle go to its ring. a handle bound to an orphan ring moves to the ring of the next worker that
//    submits for it (the armed poll is removed from the old ring first).
//  - an accepted socket (the CQE of an accept op) is bound when its CQE is taken, before its handler runs : round robin over the
//    owned rings (RING::index_bind_next of the ring of the accept). bound by its first submit it went to the ring of the thread
//    running the accept : every socket of an acceptor ran on the owner of the listen ring while the other workers idled.
//  - submission : a request belongs to the thread that submitted it (the kernel rule : the requests of a thread that exits
//    complete with -ECANCELED). so a poll or an op is never submitted by a thread that is not the owner of the ring : any
//    thread stages it (RING::staged, under the SQ lock) and the owner moves the staged SQEs into the SQ and submits them with
//    the enter of its next pop_results() (the re-arms of a batch, the ops submitted by the executables of the batch : one
//    syscall). only the owner writes the SQ of its ring and enters it : an io_uring_enter submits EVERY SQE between the head
//    and the tail of the SQ, so a thread entering a ring between the flush of its owner (the tail published) and the enter of
//    the owner would take the SQEs of the owner (they would complete with -ECANCELED when that thread exits). so the poll
//    removes and the async cancels of the other threads are staged too, and a thread staging on a ring it does not own wakes
//    the owner with the wake eventfd of the ring (RING::wake_fd, polled multishot by the owner : a write is no request. once
//    till the owner flushes). the staged SQEs of a ring without an owner move to the ring of the worker that drains it. a staged
//    poll cancelled before its submit is just dropped ; a staged op cancelled is delivered with -ECANCELED by the owner
//    (RING::synthetic) like a cancelled request.
//  - the kernel keeps every thread that created a ring or submitted to it and sends each one a task work when the ring is
//    released (TIF_NOTIFY_SIGNAL : a blocking call with a timeout of that thread returned EINTR. kernel 7.0 : every time) : a ring
//    is created by a thread that ends at once (_io_uring_setup()), and a thread that is not the owner submits to no ring (the
//    wakes were IORING_OP_MSG_RING from a wake ring before : every thread that posted to an executor got EINTR when it was
//    destroyed)
//  - ops : an OP record per submitted op (the executable, the fd, the ring : the user_data of the SQE). the submit takes
//    one add_ref() of the executable, released by the executor after the delivery of the final CQE. a CQE with
//    IORING_CQE_F_MORE takes one more add_ref() before its delivery. the final CQE releases the OP and the in-flight count
//    of the fd.
//  - user_data : bit 63 clear => a readiness poll ((generation << 32) | seq << 24 | handle, TAG_IGNORE at bit 31 : the
//    registry generation never has bit 31 set). bit 63 set => an op (the OP pointer, < 2^48), bits 63+62 => the wake of a ring
//    (a nop of the owner, the poll of its wake eventfd : UD_WAKE_POLL | its generation << 16, its index), bits 63+62+61 =>
//    ignored (a cancel, a nop).
//  - post : a shared queue. a post wakes one owned ring (round robin) with a wake CQE (the wake eventfd of the ring : it wakes
//    the owner waiting in the kernel. the owner itself : a nop in its SQ). the woken ring takes a batch and wakes the next ring
//    when the queue is not empty. an owner about to wait takes a batch too (no owner waited when the post came : the wake went to a
//    busy ring, its owner may run a long executable). a thread without a ring
//    (a thread pumping the executor : wait_complete_* with execute(2ms)) pops the queue only and sleeps on a condition.
//  - readiness : the one shot poll / armed sequence / re-arm logic of io_context_uring on the bound ring of the handle.
//  - the setup flags COOP_TASKRUN | TASKRUN_FLAG : the task work of a request runs when its owner returns to user space
//    or wakes from a sleep (no IPI). a thread spinning in user space delays the CQEs of the requests it submitted.
//  - multishot recv + provided buffers (submit_recv_multishot) : a provided buffer ring per ring (IORING_REGISTER_PBUF_RING,
//    group PBUF_GROUP, PROVIDED_BUFFER_COUNT x PROVIDED_BUFFER_SIZE : registered at the first multishot recv on the ring). the
//    kernel takes a buffer per arrival (the CQE : IORING_CQE_F_BUFFER + the buffer id, F_MORE while the recv goes on), the param
//    of the delivery carries the ring of the buffer too (bits 32~47 : its index + 1). the consumer copies the data and gives the
//    buffer back (return_buffer) from any thread : the buffer ring is memory shared with the kernel, not a request (no owner
//    rule). its tail is written under RING::lockable_pbuf (one producer at a time : the consumer of a socket bound to another ring
//    too), the kernel consumes the head. a buffer is given back once (RING::pbuf_out). the buffers ran out => the recv ends with
//    -ENOBUFS (no F_MORE) : the consumer submits again.
//  - order : the CQEs of a ring are executed by one thread at a time, in CQ order (the owner, or the worker that drained it while
//    an orphan : STATE_DRAINING till that worker executed them, t_draining). the CQEs of one multishot op never run concurrently.
//  - lent ring (thread_blocked : the owner blocks in scoped_thread_block / a wait pumping the executor) : STATE_LENT, drained by the
//    other workers like an orphan (STATE_DRAINING_LENT while one holds it) - otherwise every socket of the ring waited for the
//    blocked handler. the task work wakes a sleeping owner, so the CQEs of its requests are posted meanwhile (a busy owner in user
//    space delays them : COOP_TASKRUN, not helped). the rest of the batch of the owner (RING::carry) runs first, the results of the
//    executable the owner blocked in (RING::pblocked) wait for the owner (RING::deferred, in order : one executable's results never
//    run concurrently). the owner takes the ring back at its unblock (at the release of the worker holding it) and executes carry
//    and deferred before its CQ. the staged SQEs move to the ring of the draining worker (the owner does not enter while it blocks)
//-----------------------------------------------------------------------------
#if defined(__ANDROID__) || defined(__linux__)

#include <linux/io_uring.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

#if defined(IORING_FEAT_EXT_ARG) && defined(IORING_SETUP_CQSIZE) && defined(IORING_POLL_ADD_MULTI)
	#define CGDK_IO_URING_CQ_SUPPORTED 1
#else
	#define CGDK_IO_URING_CQ_SUPPORTED 0
#endif

namespace CGDK
{

namespace
{
	// - the ABI values (the headers of an older kernel do not have all of them)
	constexpr uint8_t	OPCODE_NOP = 0;
	constexpr uint8_t	OPCODE_POLL_ADD = 6;
	constexpr uint8_t	OPCODE_POLL_REMOVE = 7;
	constexpr uint8_t	OPCODE_SENDMSG = 9;
	constexpr uint8_t	OPCODE_RECVMSG = 10;
	constexpr uint8_t	OPCODE_ACCEPT = 13;
	constexpr uint8_t	OPCODE_ASYNC_CANCEL = 14;
	constexpr uint8_t	OPCODE_CONNECT = 16;
	constexpr uint8_t	OPCODE_SEND = 26;
	constexpr uint8_t	OPCODE_RECV = 27;
	constexpr uint8_t	OPCODE_SEND_ZC = 47;
	constexpr uint8_t	OPCODE_SENDMSG_ZC = 48;
	constexpr unsigned	POLL_FLAG_ADD_MULTI = 1u << 0;		// - IORING_POLL_ADD_MULTI (sqe->len of a POLL_ADD) : a CQE per wakeup (IORING_CQE_F_MORE while armed)

	constexpr unsigned	SETUP_COOP_TASKRUN = 1u << 8;
	constexpr unsigned	SETUP_TASKRUN_FLAG = 1u << 9;
	constexpr unsigned	SQ_FLAG_TASKRUN = 1u << 2;
	constexpr unsigned	CANCEL_FLAG_ALL = 1u << 0;
	constexpr unsigned	CANCEL_FLAG_FD = 1u << 1;
	constexpr unsigned	CANCEL_FLAG_ANY = 1u << 2;
	constexpr unsigned	ACCEPT_FLAG_MULTISHOT = 1u << 0;
	constexpr unsigned	CQE_FLAG_MORE = 1u << 1;
	constexpr unsigned	REGISTER_PROBE_OPS = 8;
	constexpr unsigned	REGISTER_SYNC_CANCEL_OPS = 24;
	constexpr unsigned	PROBE_OP_SUPPORTED = 1u << 0;
	constexpr uint8_t	SQE_FLAG_BUFFER_SELECT = 1u << 5;	// - IOSQE_BUFFER_SELECT
	constexpr uint16_t	RECV_FLAG_MULTISHOT = 1u << 1;		// - IORING_RECV_MULTISHOT (the ioprio of a RECV, 6.0+)
	constexpr unsigned	CQE_FLAG_BUFFER = 1u << 0;			// - IORING_CQE_F_BUFFER : the buffer id in the upper 16 bits of the flags
	constexpr unsigned	CQE_BUFFER_SHIFT = 16;
	constexpr unsigned	REGISTER_PBUF_RING = 22;			// - IORING_REGISTER_PBUF_RING (5.19+)
	constexpr uint16_t	PBUF_GROUP = 0;						// - the buffer group of the provided buffer ring of a ring (the group ids are per io_uring : the same on every ring)
	constexpr unsigned	PARAM_RING_SHIFT = 32;				// - the param of an op completion with a provided buffer : the index of its ring + 1 in the bits 32~47

	constexpr unsigned	RING_ENTRIES = 256;					// - SQ entries per ring (CQ : x4). one ring per worker thread : the locked memory of the user (RLIMIT_MEMLOCK) bounds the count
	constexpr unsigned	MIN_RING_ENTRIES = 32;
	constexpr int		STATE_ORPHAN = 0;
	constexpr int		STATE_OWNED = 1;
	constexpr int		STATE_DRAINING = 2;
	constexpr int		STATE_LENT = 3;						// - the owner blocks (thread_blocked) : drained by the other workers like an orphan, taken back by the owner
	constexpr int		STATE_DRAINING_LENT = 4;			// - a worker drains a lent ring (given back to the owner at its release when the owner unblocked meanwhile)
	constexpr uint8_t	OP_STAGED = 0;						// - the SQE of the op is staged (not in the kernel)
	constexpr uint8_t	OP_SUBMITTED = 1;					// - in the kernel (its CQE comes)
	constexpr uint8_t	OP_CANCELLED = 2;					// - cancelled while staged : delivered with -ECANCELED by the owner of the ring (RING::synthetic)
	constexpr std::size_t MAX_POST_BATCH = 16;
	constexpr std::size_t MAX_POOL_OP = 4096;

	// - user_data
	constexpr uint64_t	TAG_IGNORE = 0x80000000ull;			// - bit 31 of a poll user_data : a cancel / an update of a poll (dropped)
	constexpr uint64_t	UD_OP = 1ull << 63;
	constexpr uint64_t	UD_WAKE = (1ull << 63) | (1ull << 62);
	constexpr uint64_t	UD_WAKE_POLL = 1ull << 40;				// - (with UD_WAKE) the poll of the wake eventfd of a ring : its generation in bits 16~31, the ring in bits 0~15
	constexpr unsigned	UD_WAKE_GENERATION_SHIFT = 16;
	constexpr uint64_t	UD_IGNORE = (1ull << 63) | (1ull << 62) | (1ull << 61);
	constexpr uint64_t	UD_MASK_KIND = 0xE000000000000000ull;
	constexpr uint64_t	UD_MASK_PAYLOAD = 0x0000FFFFFFFFFFFFull;

	// - the sequence of a poll : bits 24~30 of user_data (the handle part. the handles below 4M : < 2^24)
	constexpr unsigned	POLL_SEQ_SHIFT = 24;
	constexpr uint64_t	POLL_SEQ_MASK = 0x7Full << POLL_SEQ_SHIFT;
	constexpr unsigned	MAX_HANDLE = 4u * 1024u * 1024u;
	static_assert(MAX_HANDLE <= (1u << POLL_SEQ_SHIFT));

	constexpr uint64_t	_poll_user_data(uint64_t _event_data, uint8_t _seq) noexcept { return _event_data | (static_cast<uint64_t>(_seq) << POLL_SEQ_SHIFT); }
	constexpr uint64_t	_poll_event_data(uint64_t _user_data) noexcept { return _user_data & ~POLL_SEQ_MASK; }
	constexpr uint8_t	_poll_seq(uint64_t _user_data) noexcept { return static_cast<uint8_t>((_user_data & POLL_SEQ_MASK) >> POLL_SEQ_SHIFT); }

	// - the ring of this thread per io_context (a thread may be the worker of one io_context and pump another)
	struct WORKER_RING
	{
		const void*			pcontext;
		void*				pring;
		int					block_depth = 0;		// - thread_blocked() nested (the ring is lent at the first, taken back at the last)
	};
	thread_local std::vector<WORKER_RING> t_worker_ring;

	// - the registrations without EPOLLONESHOT whose CQE this thread took : armed again by its flush_pending() (after the processing)
	struct REARM
	{
		const void*			pcontext;
		CGHANDLE			handle;
		uint64_t			event_data;
	};
	thread_local std::vector<REARM> t_rearm;

	// - the orphan rings whose CQEs this thread took : STATE_DRAINING till this thread executed them (flush_pending() / its next
	//   pop_results()). the CQEs of a ring run in order on one thread at a time (the CQEs of a multishot recv are the data of a stream)
	struct DRAINING
	{
		const void*			pcontext;
		void*				pring;
		bool				is_lent = false;		// - a lent ring (STATE_DRAINING_LENT) : LENT again, or OWNED when its owner unblocked meanwhile
	};

	// - the idle workers check the lent rings this often (their CQEs are posted while the owner blocks : nobody waits on that CQ)
	constexpr auto		LENT_WAIT = std::chrono::milliseconds(1);
	thread_local std::vector<DRAINING> t_draining;

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

	int _io_uring_register(int _fd, unsigned _opcode, void* _parg, unsigned _count) noexcept
	{
		return static_cast<int>(::syscall(__NR_io_uring_register, _fd, _opcode, _parg, _count));
	}

	// - IORING_REGISTER_SYNC_CANCEL (linux 6.0+)
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

	// - IORING_REGISTER_PROBE
	struct PROBE_OP
	{
		uint8_t				op;
		uint8_t				resv;
		uint16_t			flags;
		uint32_t			resv2;
	};
	struct PROBE
	{
		uint8_t				last_op;
		uint8_t				ops_len;
		uint16_t			resv;
		uint32_t			resv2[3];
		PROBE_OP			ops[64];
	};
	static_assert(sizeof(PROBE) == 16 + 64 * 8);

	// - IORING_REGISTER_PBUF_RING : an entry of the provided buffer ring (struct io_uring_buf : the resv of the entry 0 is the tail
	//   of the ring) and the registration (struct io_uring_buf_reg)
	struct PBUF_ENTRY
	{
		uint64_t			addr;
		uint32_t			len;
		uint16_t			bid;
		uint16_t			resv;
	};
	static_assert(sizeof(PBUF_ENTRY) == 16);

	struct PBUF_REG
	{
		uint64_t			ring_addr;
		uint32_t			ring_entries;
		uint16_t			bgid;
		uint16_t			flags;
		uint64_t			resv[3];
	};
	static_assert(sizeof(PBUF_REG) == 40);

	// - the multishot recv (CGDK_IO_URING_MULTISHOT=0 or off : submit_recv_multishot() returns false, the sockets post one RECV per
	//   arrival. read once)
	bool _is_multishot_enabled() noexcept
	{
		static const bool s_is_enabled = []()
		{
			const char* pvalue = std::getenv("CGDK_IO_URING_MULTISHOT");
			return pvalue == nullptr || (std::strcmp(pvalue, "0") != 0 && std::strcmp(pvalue, "off") != 0);
		}();

		return s_is_enabled;
	}

	// - the poll mask of the epoll flags (EPOLLET/EPOLLONESHOT/EPOLLEXCLUSIVE/EPOLLWAKEUP are not poll events)
	constexpr uint32_t _poll_mask(uint32_t _event) noexcept
	{
		return _event & static_cast<uint32_t>(EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLHUP | EPOLLRDHUP);
	}
}


//-----------------------------------------------------------------------------
// OP : a submitted op (the user_data of its SQE)
//-----------------------------------------------------------------------------
struct api::io_context_uring_cq::OP
{
	Iexecutable*		pexecutable;
	CGHANDLE			fd;
	RING*				pring;
	std::size_t			index;				// - in RING::ops (under lockable_sq)
	uint8_t				state;				// - OP_STAGED / OP_SUBMITTED / OP_CANCELLED (under lockable_sq of the ring)
	uint8_t				opcode;				// - the opcode of its SQE (an accept : the accepted socket is bound at its CQE)
};


//-----------------------------------------------------------------------------
// HANDLE_ENTRY : the state of a handle number (m_handle_chunks : chunks of HANDLE_CHUNK entries, allocated at the first use)
//-----------------------------------------------------------------------------
struct api::io_context_uring_cq::HANDLE_ENTRY
{
	std::atomic<uint8_t>	armed{ 0 };			// - the sequence of the armed poll (0 : none. written under the stripe lock, cleared by its CQE)
	std::atomic<uint16_t>	binding{ 0 };		// - the ring of the handle + 1 (0 : none)
	std::atomic<int32_t>	inflight{ 0 };		// - the ops of the handle not delivered yet
};

namespace
{
	constexpr unsigned	HANDLE_CHUNK_SHIFT = 14;				// - 16384 handles per chunk (128KB)
	constexpr unsigned	HANDLE_CHUNK = 1u << HANDLE_CHUNK_SHIFT;
	constexpr unsigned	HANDLE_CHUNK_COUNT = MAX_HANDLE >> HANDLE_CHUNK_SHIFT;
	static_assert((MAX_HANDLE & (HANDLE_CHUNK - 1)) == 0);
}

api::io_context_uring_cq::HANDLE_ENTRY* api::io_context_uring_cq::_find_entry(CGHANDLE _handle) const noexcept
{
	RETURN_IF(_handle < 0 || static_cast<unsigned>(_handle) >= this->m_count_handles || this->m_handle_chunks == nullptr, nullptr);

	auto* pchunk = this->m_handle_chunks[static_cast<unsigned>(_handle) >> HANDLE_CHUNK_SHIFT].load(std::memory_order_acquire);

	RETURN_IF(pchunk == nullptr, nullptr);

	return &pchunk[static_cast<unsigned>(_handle) & (HANDLE_CHUNK - 1)];
}

api::io_context_uring_cq::HANDLE_ENTRY* api::io_context_uring_cq::_entry(CGHANDLE _handle) noexcept
{
	RETURN_IF(_handle < 0 || static_cast<unsigned>(_handle) >= this->m_count_handles || this->m_handle_chunks == nullptr, nullptr);

	auto& chunk = this->m_handle_chunks[static_cast<unsigned>(_handle) >> HANDLE_CHUNK_SHIFT];
	auto* pchunk = chunk.load(std::memory_order_acquire);

	if (pchunk == nullptr)
	{
		// - the first use of the chunk : allocated (another thread allocating it meanwhile : its chunk is used, this one freed)
		auto* pchunk_new = new (std::nothrow) HANDLE_ENTRY[HANDLE_CHUNK];

		RETURN_IF(pchunk_new == nullptr, nullptr);

		if (chunk.compare_exchange_strong(pchunk, pchunk_new, std::memory_order_acq_rel))
			pchunk = pchunk_new;
		else
			delete[] pchunk_new;
	}

	return &pchunk[static_cast<unsigned>(_handle) & (HANDLE_CHUNK - 1)];
}

void api::io_context_uring_cq::_free_handle_chunks() noexcept
{
	if (this->m_handle_chunks != nullptr)
	{
		for (unsigned i = 0; i < HANDLE_CHUNK_COUNT; ++i)
			delete[] this->m_handle_chunks[i].exchange(nullptr, std::memory_order_acq_rel);
	}

	this->m_handle_chunks.reset();
	this->m_count_handles = 0;
}


//-----------------------------------------------------------------------------
// the ring
//-----------------------------------------------------------------------------
struct api::io_context_uring_cq::RING
{
	int					fd = -1;
	unsigned			index = 0;

	// - SQ
	unsigned*			sq_head = nullptr;
	unsigned*			sq_tail = nullptr;
	unsigned*			sq_flags = nullptr;
	unsigned			sq_mask = 0;
	unsigned			sq_entries = 0;
	unsigned*			sq_array = nullptr;
	io_uring_sqe*		sqes = nullptr;
	unsigned			sq_tail_local = 0;				// - the next free slot (under lockable_sq)

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
	std::atomic<int>	state{ 0 };						// - STATE_ORPHAN (no owner : drained by the workers), STATE_OWNED (a worker is bound), STATE_DRAINING (a worker drains it)
	std::vector<OP*>	ops;							// - the ops not delivered yet : staged, in the kernel or cancelled while staged (under lockable_sq)

	// - the SQEs staged for the owner (a poll, an op : written by any thread under lockable_sq, moved into the SQ and submitted by
	//   the owner with the enter of its next pop_results(). a ring without an owner : moved to the ring of the worker that drains it)
	struct STAGED
	{
		io_uring_sqe		sqe;
		OP*					pop;						// - the op of the SQE (nullptr : a poll)
	};
	std::vector<STAGED>	staged;
	std::vector<OP*>	synthetic;						// - the ops cancelled while staged (OP_CANCELLED, still in ops) : delivered with -ECANCELED by the owner
	std::atomic<bool>	is_wake_pending{ false };		// - a wake for the staged SQEs was written and the owner did not flush them yet (one wake till then)
	std::atomic<bool>	is_waiting{ false };			// - the owner waits in the kernel for a completion (idle : a wake of the post queue is taken at once)
	std::atomic<unsigned> index_bind_next{ 0 };			// - the round robin of the sockets accepted by the accept ops of this ring (_bind_accepted)
	int					wake_fd = -1;					// - the wake eventfd : the other threads write it, the owner polls it (multishot : a CQE per write)
	uint16_t			wake_generation = 0;			// - the generation of the poll of wake_fd armed last (under lockable_sq : an ended poll of an older owner is not armed again)

	// - lent (the owner blocks : thread_blocked). under lockable_lend (the state LENT <-> DRAINING_LENT / OWNED changes under it too)
	std::mutex			lockable_lend;
	Iexecutable*		pblocked = nullptr;				// - the executable the owner blocked in : its results wait for the owner (deferred, in order)
	std::vector<POP_RESULT> carry;						// - the rest of the batch of the owner when it blocked : executed first by the worker that drains the ring
	std::vector<POP_RESULT> deferred;					// - the results of pblocked taken meanwhile : executed by the owner when it took the ring back
	bool				is_reclaim = false;				// - the owner unblocked while a worker drained the ring : OWNED at the release of that worker

	// - the provided buffer ring (submit_recv_multishot : registered at the first one on this ring, _prepare_pbuf()). the kernel takes
	//   the buffers at the head, the buffers given back are written at the tail under lockable_pbuf by any thread (return_buffer)
	std::mutex			lockable_pbuf;
	std::atomic<int>	pbuf_state{ 0 };				// - 0 : none, 1 : registered, -1 : the registration failed (not tried again : submit_recv_multishot() returns false)
	PBUF_ENTRY*			pbuf_ring = nullptr;
	std::size_t			size_pbuf_ring = 0;
	char*				pbuf_data = nullptr;
	std::size_t			size_pbuf_data = 0;
	uint16_t			pbuf_tail = 0;					// - the tail of the buffer ring (under lockable_pbuf)
	std::unique_ptr<std::atomic<uint8_t>[]> pbuf_out;	// - the buffer is out (its CQE taken, not given back yet) : a buffer given back twice is ignored
	std::atomic<int>	pbuf_count_out{ 0 };

	// - the SQEs written but not submitted (the kernel consumes sq_head on io_uring_enter)
	[[nodiscard]] unsigned pending() const noexcept { return sq_tail_local - __atomic_load_n(sq_head, __ATOMIC_ACQUIRE); }

	// - a staged SQE (under lockable_sq). nullptr : no memory. the pointer is valid till the next stage()
	io_uring_sqe* stage(OP* _pop) noexcept
	{
		try
		{
			staged.push_back(STAGED{});
		}
		catch (...)
		{
			return nullptr;
		}

		auto& entry = staged.back();

		std::memset(&entry.sqe, 0, sizeof(entry.sqe));
		entry.pop = _pop;

		return &entry.sqe;
	}

	// - the staged SQEs into the SQ (under lockable_sq, by the owner : submitted by its enter. the SQ full => submitted meanwhile.
	//   the enter failed => the rest stays staged)
	void flush_staged() noexcept
	{
		std::size_t count_flushed = 0;

		for (; count_flushed < staged.size(); ++count_flushed)
		{
			io_uring_sqe* psqe = alloc_sqe();

			BREAK_IF(psqe == nullptr);

			auto& entry = staged[count_flushed];

			std::memcpy(psqe, &entry.sqe, sizeof(*psqe));

			if (entry.pop != nullptr)
				entry.pop->state = OP_SUBMITTED;
		}

		staged.erase(staged.begin(), staged.begin() + static_cast<std::ptrdiff_t>(count_flushed));
	}

	// - the next SQE (under lockable_sq). nullptr : the SQ is full (after a submit)
	io_uring_sqe* alloc_sqe() noexcept
	{
		if (pending() >= sq_entries)
		{
			submit();

			if (pending() >= sq_entries)
				return nullptr;
		}

		const unsigned index_sqe = sq_tail_local & sq_mask;
		io_uring_sqe* psqe = &sqes[index_sqe];

		std::memset(psqe, 0, sizeof(*psqe));
		sq_array[index_sqe] = index_sqe;
		++sq_tail_local;

		return psqe;
	}

	// - publishes the tail (under lockable_sq) and returns the count to submit
	unsigned publish() noexcept
	{
		__atomic_store_n(sq_tail, sq_tail_local, __ATOMIC_RELEASE);
		return pending();
	}

	// - publishes the tail and enters (under lockable_sq). returns the result of io_uring_enter (-1 : errno)
	int submit() noexcept
	{
		const unsigned to_submit = publish();

		if (to_submit == 0)
			return 0;

		return enter(to_submit, 0, 0, nullptr, 0);
	}

	// - io_uring_enter with a few retries on EAGAIN/EBUSY (the kernel is busy, the CQ is full : the SQEs stay pending)
	int enter(unsigned _to_submit, unsigned _min_complete, unsigned _flags, const void* _parg, std::size_t _size_arg) noexcept
	{
		for (int count_retry = 0; ; ++count_retry)
		{
			const int result = _io_uring_enter(fd, _to_submit, _min_complete, _flags, _parg, _size_arg);

			if (result >= 0 || (errno != EAGAIN && errno != EBUSY))
				return result;

			if (count_retry >= 64)
				return result;

			std::this_thread::yield();
		}
	}

	bool create(unsigned _entries) noexcept
	{
	#if CGDK_IO_URING_CQ_SUPPORTED
		io_uring_params params{};

		// - 6.0+ : COOP_TASKRUN | TASKRUN_FLAG (EINVAL on an older kernel : without them)
		//   the ring memory is locked memory of the user (RLIMIT_MEMLOCK, 8MB on ubuntu : ~36KB per ring of 256 entries) : ENOMEM => a smaller ring
		for (unsigned entries = _entries; ; entries /= 2)
		{
			for (int count_try = 0; count_try < 2; ++count_try)
			{
				std::memset(&params, 0, sizeof(params));
				params.flags = IORING_SETUP_CLAMP | IORING_SETUP_CQSIZE | ((count_try == 0) ? (SETUP_COOP_TASKRUN | SETUP_TASKRUN_FLAG) : 0u);
				params.cq_entries = entries * 4;

				fd = _io_uring_setup(entries, &params);

				if (fd >= 0 || errno != EINVAL)
					break;
			}

			if (fd >= 0 || errno != ENOMEM || entries <= MIN_RING_ENTRIES)
				break;
		}

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
		sq_flags = reinterpret_cast<unsigned*>(base_sq + params.sq_off.flags);
		sq_mask = *reinterpret_cast<unsigned*>(base_sq + params.sq_off.ring_mask);
		sq_entries = *reinterpret_cast<unsigned*>(base_sq + params.sq_off.ring_entries);
		sq_array = reinterpret_cast<unsigned*>(base_sq + params.sq_off.array);
		sq_tail_local = *sq_tail;

		cq_head = reinterpret_cast<unsigned*>(base_cq + params.cq_off.head);
		cq_tail = reinterpret_cast<unsigned*>(base_cq + params.cq_off.tail);
		cq_mask = *reinterpret_cast<unsigned*>(base_cq + params.cq_off.ring_mask);
		cq_entries = *reinterpret_cast<unsigned*>(base_cq + params.cq_off.ring_entries);
		cqes = reinterpret_cast<io_uring_cqe*>(base_cq + params.cq_off.cqes);

		// - the wake eventfd (polled by the owner : write_wake_poll())
		wake_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

		if (wake_fd < 0)
		{
			destroy();
			return false;
		}

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
		cqes = nullptr;
		if (fd >= 0) { ::close(fd); fd = -1; }
		if (wake_fd >= 0) { ::close(wake_fd); wake_fd = -1; }

		// - the provided buffers (the ring is closed : no request uses them). a buffer still out (held by a message the application keeps :
		//   the zero copy receive) keeps the data mapped : it is not unmapped (a leak of the ring's buffers at the end of the process only)
		destroy_pbuf(pbuf_count_out.load(std::memory_order_acquire) > 0);
	}

	// - a wake of the owner for the post queue, written by the owner itself into its SQ (under lockable_sq. submitted by the enter
	//   of its next pop_results() : a nop completes in the enter that submits it). the other threads : _send_wake()
	bool write_wake() noexcept
	{
		io_uring_sqe* psqe = alloc_sqe();

		if (psqe == nullptr)
			return false;

		psqe->opcode = OPCODE_NOP;
		psqe->fd = -1;
		psqe->user_data = UD_WAKE | index;

		return true;
	}

	// - the poll of the wake eventfd, written by the owner into its SQ (under lockable_sq : bind_worker(), an ended poll of the owner).
	//   a new generation : the CQEs of an older poll do not arm it again
	bool write_wake_poll() noexcept
	{
		io_uring_sqe* psqe = alloc_sqe();

		if (psqe == nullptr)
			return false;

		++wake_generation;

		psqe->opcode = OPCODE_POLL_ADD;
		psqe->fd = wake_fd;
		psqe->poll32_events = POLLIN;
		psqe->len = POLL_FLAG_ADD_MULTI;
		psqe->user_data = UD_WAKE | UD_WAKE_POLL | (static_cast<uint64_t>(wake_generation) << UD_WAKE_GENERATION_SHIFT) | index;

		return true;
	}

	// - the provided buffer ring (under lockable_pbuf) : _count buffers of _size, registered as the group PBUF_GROUP and given to the
	//   kernel. false : no memory, or the kernel refused it (before 5.19 : EINVAL)
	bool create_pbuf(unsigned _count, std::size_t _size) noexcept
	{
		const auto size_page = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
		const std::size_t size_ring = ((sizeof(PBUF_ENTRY) * _count + size_page - 1) / size_page) * size_page;
		const std::size_t size_data = _size * _count;

		// - the ring : page aligned (mmap). the data : not locked, its pages come at their first use
		void* pmemory_ring = ::mmap(nullptr, size_ring, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);

		RETURN_IF(pmemory_ring == MAP_FAILED, false);

		void* pmemory_data = ::mmap(nullptr, size_data, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

		if (pmemory_data == MAP_FAILED)
		{
			::munmap(pmemory_ring, size_ring);
			return false;
		}

		try
		{
			pbuf_out = std::make_unique<std::atomic<uint8_t>[]>(_count);
		}
		catch (...)
		{
			::munmap(pmemory_data, size_data);
			::munmap(pmemory_ring, size_ring);
			return false;
		}

		pbuf_ring = static_cast<PBUF_ENTRY*>(pmemory_ring);
		size_pbuf_ring = size_ring;
		pbuf_data = static_cast<char*>(pmemory_data);
		size_pbuf_data = size_data;

		// - every buffer into the ring (the resv of the entry 0 is the tail : not written here)
		for (unsigned i = 0; i < _count; ++i)
		{
			pbuf_out[i].store(0, std::memory_order_relaxed);
			pbuf_ring[i].addr = reinterpret_cast<uint64_t>(pbuf_data + _size * i);
			pbuf_ring[i].len = static_cast<uint32_t>(_size);
			pbuf_ring[i].bid = static_cast<uint16_t>(i);
		}

		PBUF_REG reg{};
		reg.ring_addr = reinterpret_cast<uint64_t>(pbuf_ring);
		reg.ring_entries = _count;
		reg.bgid = PBUF_GROUP;

		if (_io_uring_register(fd, REGISTER_PBUF_RING, &reg, 1) < 0)
		{
			destroy_pbuf();
			return false;
		}

		// - given to the kernel (the tail : the count of the buffers)
		pbuf_tail = static_cast<uint16_t>(_count);
		__atomic_store_n(&pbuf_ring[0].resv, pbuf_tail, __ATOMIC_RELEASE);

		return true;
	}

	// - a buffer given back (under lockable_pbuf) : written at the tail, then the tail published (read by the kernel with an acquire).
	//   the ring has an entry per buffer : the slot at the tail is never one the kernel did not consume yet
	void give_back_pbuf(unsigned _bid, unsigned _count, std::size_t _size) noexcept
	{
		auto& entry = pbuf_ring[pbuf_tail & (_count - 1)];

		entry.addr = reinterpret_cast<uint64_t>(pbuf_data + _size * _bid);
		entry.len = static_cast<uint32_t>(_size);
		entry.bid = static_cast<uint16_t>(_bid);

		++pbuf_tail;
		__atomic_store_n(&pbuf_ring[0].resv, pbuf_tail, __ATOMIC_RELEASE);
	}

	// - the memory of the provided buffer ring (the ring closed or never registered : no request uses a buffer). _is_keep_data : a buffer is
	//   still out (a message of the zero copy receive held by the application) : the data stays mapped
	void destroy_pbuf(bool _is_keep_data = false) noexcept
	{
		if (pbuf_data != nullptr && _is_keep_data == false) { ::munmap(pbuf_data, size_pbuf_data); }
		pbuf_data = nullptr;
		if (pbuf_ring != nullptr) { ::munmap(pbuf_ring, size_pbuf_ring); pbuf_ring = nullptr; }
		size_pbuf_data = 0;
		size_pbuf_ring = 0;
	}
};


//-----------------------------------------------------------------------------
// RING_GUARD : counts a thread using the rings. destroy() waits for the count after it invalidated the handle (the rings are unmapped after)
//-----------------------------------------------------------------------------
struct api::io_context_uring_cq::RING_GUARD
{
	api::io_context_uring_cq&	self;
	bool					is_valid;

	explicit RING_GUARD(api::io_context_uring_cq& _self) noexcept : self(_self), is_valid(false)
	{
		self.m_count_users.fetch_add(1, std::memory_order_acq_rel);

		// - the rings are usable only while the handle is valid (destroy() invalidates it first)
		is_valid = (self.m_handle_epoll != INVALID_CGHANDLE_VALUE);
	}
	~RING_GUARD() noexcept
	{
		self.m_count_users.fetch_sub(1, std::memory_order_acq_rel);
	}
	RING_GUARD(const RING_GUARD&) = delete;
	RING_GUARD& operator=(const RING_GUARD&) = delete;
};


//-----------------------------------------------------------------------------
// io_context_uring_cq
//-----------------------------------------------------------------------------
api::io_context_uring_cq::io_context_uring_cq()
{
}

api::io_context_uring_cq::~io_context_uring_cq() noexcept
{
	this->destroy();

	// - the rings (kept by destroy() : a worker unbinding touches its ring) and the pool
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	for (unsigned i = 0; i < count_rings; ++i)
		this->m_rings[i].reset();

	this->m_count_rings.store(0, std::memory_order_release);

	for (auto* pop : this->m_pool_op)
		delete pop;

	this->m_pool_op.clear();
}

bool api::io_context_uring_cq::is_available() noexcept
{
#if CGDK_IO_URING_CQ_SUPPORTED
	// - checked once : a ring with the features and the opcodes this backend needs (6.0+ : SEND_ZC / SENDMSG_ZC, sync cancel, cancel by fd)
	static const bool s_is_available = []()
	{
		io_uring_params params{};
		params.flags = IORING_SETUP_CLAMP;

		const int fd = _io_uring_setup(8, &params);

		if (fd < 0)
			return false;

		bool is_available = (params.features & IORING_FEAT_EXT_ARG) != 0 && (params.features & IORING_FEAT_NODROP) != 0;

		if (is_available)
		{
			PROBE probe{};

			if (_io_uring_register(fd, REGISTER_PROBE_OPS, &probe, 64) < 0)
			{
				is_available = false;
			}
			else
			{
				for (const uint8_t opcode : { OPCODE_RECV, OPCODE_SEND, OPCODE_RECVMSG, OPCODE_SENDMSG, OPCODE_ACCEPT, OPCODE_CONNECT, OPCODE_ASYNC_CANCEL, OPCODE_POLL_ADD, OPCODE_SEND_ZC, OPCODE_SENDMSG_ZC })
				{
					if (opcode > probe.last_op || (probe.ops[opcode].flags & PROBE_OP_SUPPORTED) == 0)
					{
						is_available = false;
						break;
					}
				}
			}
		}

		::close(fd);

		return is_available;
	}();

	return s_is_available;
#else
	return false;
#endif
}

bool api::io_context_uring_cq::create(int /*_hint_size*/) noexcept
{
	// check) created already
	RETURN_IF(this->m_handle_epoll != INVALID_CGHANDLE_VALUE || this->m_count_rings.load(std::memory_order_acquire) != 0, false);

	// check) available
	RETURN_IF(is_available() == false, false);

	// 1) the handle table (the chunk pointers : a chunk is allocated at the first use of a handle in it. every handle below
	//    MAX_HANDLE is tracked : the open file limit may be raised after this)
	{
		auto pchunks = std::unique_ptr<std::atomic<HANDLE_ENTRY*>[]>(new (std::nothrow) std::atomic<HANDLE_ENTRY*>[HANDLE_CHUNK_COUNT]);

		RETURN_IF(pchunks == nullptr, false);

		for (unsigned i = 0; i < HANDLE_CHUNK_COUNT; ++i)
			pchunks[i].store(nullptr, std::memory_order_relaxed);

		this->m_handle_chunks = std::move(pchunks);
		this->m_count_handles = MAX_HANDLE;
	}

	// 2) the handle : an eventfd of its own (handle() != INVALID_CGHANDLE_VALUE while created)
	const int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

	if (event_fd < 0)
	{
		this->_free_handle_chunks();
		return false;
	}

	// 3) store
	{
		std::lock_guard lock(this->m_lockable_post);
		this->m_is_signaled = false;
		this->m_is_post_closed = false;
	}

	this->m_count_attached = 0;
	this->m_event_fd_handle = event_fd;
	this->m_handle_epoll = event_fd;

	// return)
	return true;
}

CGHANDLE api::io_context_uring_cq::dispose() noexcept
{
	// - handle() => INVALID : pop_results() returns at once. the rings stay until destroy()
	const CGHANDLE handle = this->m_handle_epoll.exchange(INVALID_CGHANDLE_VALUE);

	// - wake the workers waiting on their ring (the wake eventfd)
	if (handle != INVALID_CGHANDLE_VALUE)
	{
		const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

		for (unsigned i = 0; i < count_rings; ++i)
		{
			if (auto* pring = this->m_rings[i].get(); pring != nullptr)
				this->_send_wake(pring);
		}
	}

	return handle;
}

bool api::io_context_uring_cq::_send_wake(RING* _pring) noexcept
{
	// - a CQE on the ring without a request of this thread : a write of its wake eventfd (the owner polls it multishot : the poll is
	//   the owner's request, its CQE wakes the owner waiting in the kernel. without an owner (an orphan) : no poll, nothing).
	//   the caller keeps the ring alive (a RING_GUARD, m_count_signaling, or destroy() itself : wake_fd is closed by RING::destroy())
	RETURN_IF(_pring == nullptr || _pring->wake_fd < 0, false);

	return ::eventfd_write(_pring->wake_fd, 1) == 0;
}

result_code api::io_context_uring_cq::destroy() noexcept
{
	// check) created?
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE && this->m_count_handles == 0, eRESULT::SUCCESS);

	// 1) handle() => INVALID
	this->m_handle_epoll = INVALID_CGHANDLE_VALUE;

	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	// 2) wake the waiters (a wake CQE on every ring) and wait until no thread uses the rings (pop_results, modify, submit ... : RING_GUARD)
	for (;;)
	{
		for (unsigned i = 0; i < count_rings; ++i)
			this->_send_wake(this->m_rings[i].get());

		if (this->m_count_users.load(std::memory_order_acquire) == 0)
			break;

		std::this_thread::yield();
	}

	// - the post signals out of the lock
	while (this->m_count_signaling.load(std::memory_order_acquire) != 0)
		std::this_thread::yield();

	// 3) cancel every request of every ring (sync : the requests are gone from the ring when it returns ENOENT)
	for (unsigned i = 0; i < count_rings; ++i)
		this->_sync_cancel(this->m_rings[i].get(), 0, -1, CANCEL_FLAG_ANY, 1s);

	// 4) the CQEs : nothing is delivered (the executor stopped). the final CQE of an op releases the op
	for (unsigned i = 0; i < count_rings; ++i)
	{
		auto* pring = this->m_rings[i].get();

		CONTINUE_IF(pring->cqes == nullptr);

		unsigned head = __atomic_load_n(pring->cq_head, __ATOMIC_ACQUIRE);
		const unsigned tail = __atomic_load_n(pring->cq_tail, __ATOMIC_ACQUIRE);

		for (; head != tail; ++head)
		{
			const io_uring_cqe& cqe = pring->cqes[head & pring->cq_mask];

			CONTINUE_IF((cqe.user_data & UD_MASK_KIND) != UD_OP);
			CONTINUE_IF((cqe.flags & CQE_FLAG_MORE) != 0);

			auto* pop = reinterpret_cast<OP*>(cqe.user_data & UD_MASK_PAYLOAD);
			auto* pexecutable = pop->pexecutable;

			this->_complete_op_final(pop);
			pexecutable->release();
		}

		__atomic_store_n(pring->cq_head, head, __ATOMIC_RELEASE);
	}

	// 5) the ops without a final CQE yet (their CQE is posted by the task work of another thread later : never read), the ops
	//    staged or cancelled while staged (never submitted) : released here
	for (unsigned i = 0; i < count_rings; ++i)
	{
		auto* pring = this->m_rings[i].get();

		std::vector<OP*> ops;
		{
			std::lock_guard lock(pring->lockable_sq);
			ops.swap(pring->ops);
			pring->staged.clear();
			pring->synthetic.clear();
		}

		for (auto* pop : ops)
		{
			pop->pexecutable->release();
			this->_free_op(pop);
		}

		// - a lent ring : the results nobody executed (the rest of a batch of its owner, the results deferred to it)
		std::vector<POP_RESULT> carry_left;
		std::vector<POP_RESULT> deferred_left;
		{
			std::lock_guard lock(pring->lockable_lend);
			carry_left.swap(pring->carry);
			deferred_left.swap(pring->deferred);
			pring->pblocked = nullptr;
		}

		for (auto* presults : { &carry_left, &deferred_left })
		{
			for (auto& result : *presults)
			{
				if (result.pexecutable != nullptr)
					result.pexecutable->release();
			}
		}
	}

	// 6) release the registry (the polls go with the rings)
	this->_process_remove_all_handle(INVALID_CGHANDLE_VALUE);

	// 7) the post queue : release the executables not executed, wake the sleepers
	{
		std::deque<Iexecutable*> queue_post;
		{
			std::lock_guard lock(this->m_lockable_post);
			queue_post.swap(this->m_queue_post);
			this->m_count_post.store(0, std::memory_order_seq_cst);
			this->m_is_post_closed = true;
			this->m_is_signaled = false;
		}

		this->m_cv_post.notify_all();

		for (auto* pexecutable : queue_post)
		{
			if (pexecutable != nullptr)
				pexecutable->release();
		}
	}

	// 8) the rings : unmapped and closed, their wake eventfds too (the RING objects stay : a worker unbinding touches the state)
	for (unsigned i = 0; i < count_rings; ++i)
	{
		auto* pring = this->m_rings[i].get();

		std::lock_guard lock(pring->lockable_sq);
		pring->destroy();
	}

	// 9) the handle table
	this->_free_handle_chunks();
	this->m_count_attached = 0;

	// 10) the handle eventfd
	if (this->m_event_fd_handle >= 0)
	{
		::close(this->m_event_fd_handle);
		this->m_event_fd_handle = -1;
	}

	return eRESULT::SUCCESS;
}


//-----------------------------------------------------------------------------
// rings
//-----------------------------------------------------------------------------
api::io_context_uring_cq::RING* api::io_context_uring_cq::_create_ring(bool _only_if_none) noexcept
{
	std::lock_guard lock(this->m_lockable_rings);

	// - ring 0 on demand (a thread without a ring before any worker bound) : the one created meanwhile
	if (_only_if_none && this->m_count_rings.load(std::memory_order_acquire) != 0)
		return this->m_rings[0].get();

	return this->_create_ring_locked();
}

api::io_context_uring_cq::RING* api::io_context_uring_cq::_create_ring_locked() noexcept
{
	// - under m_lockable_rings
	const unsigned index = this->m_count_rings.load(std::memory_order_acquire);

	RETURN_IF(index >= MAX_RINGS, nullptr);

	std::unique_ptr<RING> pring;

	try
	{
		pring = std::make_unique<RING>();
	}
	catch (...)
	{
		return nullptr;
	}

	RETURN_IF(pring->create(RING_ENTRIES) == false, nullptr);

	pring->index = index;

	auto* pring_raw = pring.get();

	this->m_rings[index] = std::move(pring);
	this->m_count_rings.store(index + 1, std::memory_order_release);

	return pring_raw;
}

api::io_context_uring_cq::RING* api::io_context_uring_cq::_find_worker_ring() const noexcept
{
	for (const auto& iter : t_worker_ring)
	{
		if (iter.pcontext == this)
			return static_cast<RING*>(iter.pring);
	}

	return nullptr;
}

bool api::io_context_uring_cq::is_worker() const noexcept
{
	return this->_find_worker_ring() != nullptr;
}

bool api::io_context_uring_cq::_has_orphan() const noexcept
{
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	for (unsigned i = 0; i < count_rings; ++i)
	{
		if (this->m_rings[i]->state.load(std::memory_order_acquire) == STATE_ORPHAN)
			return true;
	}

	return false;
}

api::io_context_uring_cq::RING* api::io_context_uring_cq::_ring_round_robin() noexcept
{
	// - an owned ring (round robin). none : ring 0 (an orphan till a worker binds : drained by the workers meanwhile. created on demand)
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	if (count_rings != 0)
	{
		const unsigned start = this->m_index_ring_next.fetch_add(1, std::memory_order_relaxed);

		for (unsigned i = 0; i < count_rings; ++i)
		{
			auto* pring = this->m_rings[(start + i) % count_rings].get();

			if (pring->state.load(std::memory_order_acquire) == STATE_OWNED)
				return pring;
		}

		return this->m_rings[0].get();
	}

	return this->_create_ring(true);
}

api::io_context_uring_cq::RING* api::io_context_uring_cq::_ring_of_handle(CGHANDLE _handle, RING*& _pring_before) noexcept
{
	// - under the stripe lock of the handle. the ring the handle is bound to (bound here when none). a handle bound to an
	//   orphan ring moves to the ring of this thread (a worker). _pring_before : the ring it was bound to (nullptr : none)
	_pring_before = nullptr;

	HANDLE_ENTRY* pentry = this->_entry(_handle);
	RING* pring_own = this->_find_worker_ring();

	if (pentry != nullptr)
	{
		const uint16_t index = pentry->binding.load(std::memory_order_acquire);

		if (index != 0)
		{
			auto* pring = this->m_rings[index - 1].get();

			_pring_before = pring;

			if (pring == pring_own || pring->state.load(std::memory_order_acquire) == STATE_OWNED)
				return pring;

			// - orphan : moved to the ring of this worker, or (a thread without a ring) to an owned ring (none owned : stays)
			RING* pring_to = (pring_own != nullptr) ? pring_own : this->_ring_round_robin();

			if (pring_to == nullptr || pring_to == pring)
				return pring;

			pentry->binding.store(static_cast<uint16_t>(pring_to->index + 1), std::memory_order_release);
			return pring_to;
		}
	}

	auto* pring = (pring_own != nullptr) ? pring_own : this->_ring_round_robin();

	if (pring != nullptr && pentry != nullptr)
		pentry->binding.store(static_cast<uint16_t>(pring->index + 1), std::memory_order_release);

	return pring;
}

void api::io_context_uring_cq::_bind_accepted(RING* _pring_accept, CGHANDLE _handle) noexcept
{
	// - a socket accepted by an accept op of _pring_accept (its CQE taken now, before its handler runs) : bound to an owned ring of
	//   round robin, the ring of its first submit (the RECV of the socket staged for that owner). bound by that submit instead, every
	//   socket of an acceptor went to the ring of the thread running the accepts (the owner of the listen ring : one thread for all
	//   of them). the number is the new socket's (the accept gave it) : the binding left by a socket closed before is replaced (the
	//   ops of that socket carry their ring). an armed poll (a readiness registration of the number) keeps its ring. no owned ring
	//   (the workers left) : bound by its first submit
	HANDLE_ENTRY* pentry = this->_entry(_handle);

	RETURN_IF(pentry == nullptr);

	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);
	const unsigned start = _pring_accept->index_bind_next.fetch_add(1, std::memory_order_relaxed);
	RING* pring = nullptr;

	for (unsigned i = 0; i < count_rings && pring == nullptr; ++i)
	{
		auto* pring_now = this->m_rings[(start + i) % count_rings].get();

		if (pring_now->state.load(std::memory_order_acquire) == STATE_OWNED)
			pring = pring_now;
	}

	RETURN_IF(pring == nullptr);

	std::lock_guard lock_handle(this->m_lockable_handle[static_cast<unsigned>(_handle) % HANDLE_STRIPES]);

	RETURN_IF(pentry->armed.load(std::memory_order_acquire) != 0);

	pentry->binding.store(static_cast<uint16_t>(pring->index + 1), std::memory_order_release);
}

void api::io_context_uring_cq::bind_worker(bool _is_transient) noexcept
{
	// check) a transient worker (the extra threads of io_context_threads) owns no ring : it would exit with the ops in flight on it
	//        (-ECANCELED : the kernel cancels the requests of an exiting thread). it pops the post queue, its executables stage
	//        their ops on the rings of the permanent workers
	RETURN_IF(_is_transient);

	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false);

	// check) bound already
	RETURN_IF(this->_find_worker_ring() != nullptr);

	// 1) an orphan ring (a worker that left, ring 0 created on demand : its handles and the CQEs of its CQ), else a new one
	//    (under m_lockable_rings : a ring created on demand meanwhile is seen here, or sees this one owned)
	RING* pring = nullptr;

	{
		std::lock_guard lock(this->m_lockable_rings);

		const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

		for (unsigned i = 0; i < count_rings; ++i)
		{
			auto* pring_orphan = this->m_rings[i].get();

			int expected = STATE_ORPHAN;

			if (pring_orphan->state.compare_exchange_strong(expected, STATE_OWNED, std::memory_order_acq_rel))
			{
				// - its staged SQEs are submitted by the first pop_results() of this thread
				pring = pring_orphan;
				pring->is_wake_pending.store(false, std::memory_order_release);
				break;
			}
		}

		if (pring == nullptr)
		{
			pring = this->_create_ring_locked();

			RETURN_IF(pring == nullptr);

			pring->state.store(STATE_OWNED, std::memory_order_release);
		}
	}

	try
	{
		t_worker_ring.push_back(WORKER_RING{ this, pring });
	}
	catch (...)
	{
		pring->state.store(STATE_ORPHAN, std::memory_order_release);
		return;
	}

	// 2) the poll of the wake eventfd of the ring : this thread's (submitted by its first pop_results(). the poll of the owner before :
	//    cancelled when that thread exited, its CQE does not arm it again : an older generation)
	{
		std::lock_guard lock(pring->lockable_sq);
		pring->write_wake_poll();
	}

	// 3) the posts queued : this ring takes a part (the wake is found by its first pop_results()). also when the queue is signaled
	//    already : the ring signaled may be busy (its owner in an executable) while this one is free
	bool is_wake = false;
	{
		std::lock_guard lock(this->m_lockable_post);

		if (this->m_queue_post.empty() == false)
		{
			this->m_is_signaled = true;
			is_wake = true;
		}
	}

	if (is_wake)
	{
		// - this thread is the owner : submitted by its first pop_results()
		std::lock_guard lock(pring->lockable_sq);
		pring->write_wake();
	}
}

void api::io_context_uring_cq::unbind_worker() noexcept
{
	// - the orphan rings this thread drained (executed : the thread leaves after its last batch)
	this->_release_draining();

	// 1) the ring of this thread
	RING* pring = nullptr;

	for (auto iter = t_worker_ring.begin(); iter != t_worker_ring.end(); ++iter)
	{
		if (iter->pcontext == this)
		{
			pring = static_cast<RING*>(iter->pring);
			t_worker_ring.erase(iter);
			break;
		}
	}

	RETURN_IF(pring == nullptr);

	// 2) the SQEs staged on the ring stay staged : submitted by this thread now they would complete with -ECANCELED when it exits.
	//    the worker that adopts the ring, or the worker that drains it meanwhile (they move to its ring), submits them
	// 3) an orphan from now (adopted by the next worker, drained by the others meanwhile)
	pring->state.store(STATE_ORPHAN, std::memory_order_release);

	// 4) a wake sent to this ring is lost with the poll of this thread : another ring takes the queue
	bool is_wake = false;
	{
		std::lock_guard lock(this->m_lockable_post);

		if (this->m_queue_post.empty() == false && this->m_is_signaled)
			is_wake = true;
	}

	if (is_wake && this->_wake_ring() == false)
	{
		std::lock_guard lock(this->m_lockable_post);
		this->m_is_signaled = false;
	}
}

bool api::io_context_uring_cq::_wake_ring() noexcept
{
	// - a wake CQE on an owned ring (round robin) : its owner wakes and takes the post queue. false : no ring is owned
	//   (the rings stay mapped while m_count_signaling != 0 : destroy() waits for it)
	//   a ring whose owner waits in the kernel first : the queue is signaled once till a ring takes it, so a wake on a ring whose
	//   owner runs an executable that blocks kept every post waiting (the other workers idle). any other owned ring when none
	//   waits, the ring of this thread last : its wake is submitted by its next pop_results(), after the executables it is about
	//   to run (the posts it took : one of them blocking kept the wake, and every post, in its SQ)
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	RETURN_IF(count_rings == 0, false);

	this->m_count_signaling.fetch_add(1, std::memory_order_acq_rel);

	bool is_written = false;

	if (this->m_handle_epoll != INVALID_CGHANDLE_VALUE)
	{
		const unsigned start = this->m_index_ring_next.fetch_add(1, std::memory_order_relaxed);
		RING* pring_own = this->_find_worker_ring();

		for (unsigned i = 0; i < count_rings * 2 && is_written == false; ++i)
		{
			auto* pring = this->m_rings[(start + i) % count_rings].get();

			CONTINUE_IF(pring == pring_own || pring->state.load(std::memory_order_acquire) != STATE_OWNED);

			// - the first round : the idle owners only (seq_cst : against the owner that marks itself waiting, then checks m_count_post)
			CONTINUE_IF(i < count_rings && pring->is_waiting.load(std::memory_order_seq_cst) == false);

			// - another thread : its wake eventfd (never an enter of a ring this thread does not own)
			is_written = this->_send_wake(pring);
		}

		// - no other owned ring : the owner itself (it is in an executable) : a nop in its SQ, submitted with the enter of its next pop_results()
		if (is_written == false && pring_own != nullptr && pring_own->state.load(std::memory_order_acquire) == STATE_OWNED)
		{
			std::lock_guard lock(pring_own->lockable_sq);
			is_written = (pring_own->fd >= 0 && pring_own->write_wake());
		}
	}

	this->m_count_signaling.fetch_sub(1, std::memory_order_acq_rel);

	return is_written;
}


//-----------------------------------------------------------------------------
// readiness polls
//-----------------------------------------------------------------------------
bool api::io_context_uring_cq::_disarm(CGHANDLE _handle, uint8_t _seq) noexcept
{
	// - the CQE of the armed poll (_seq) : not armed any more. false : another poll is armed (replaced by modify() meanwhile) or none.
	//   a handle without an entry : not tracked (its polls have the sequence 0)
	auto* pentry = this->_find_entry(_handle);

	if (pentry == nullptr)
		return (_seq == 0);

	uint8_t expected = _seq;

	return pentry->armed.compare_exchange_strong(expected, 0, std::memory_order_acq_rel);
}

bool api::io_context_uring_cq::_is_armed(CGHANDLE _handle) const noexcept
{
	const auto* pentry = this->_find_entry(_handle);

	return pentry != nullptr && pentry->armed.load(std::memory_order_acquire) != 0;
}

void api::io_context_uring_cq::_wake_staged(RING* _pring) noexcept
{
	// - the owner of the ring submits the staged SQEs with the enter of its next pop_results() : woken when it waits in the
	//   kernel (the wake eventfd of the ring). once till it flushes (is_wake_pending : cleared by the owner before it
	//   flushes). the owner itself, a ring without an owner (its staged SQEs move to the ring of the worker that drains it) : nothing
	RETURN_IF(_pring == nullptr || _pring == this->_find_worker_ring());
	RETURN_IF(_pring->state.load(std::memory_order_acquire) != STATE_OWNED);
	RETURN_IF(_pring->is_wake_pending.exchange(true, std::memory_order_acq_rel));

	// - not written (a closed ring) : the next stage tries again
	if (this->_send_wake(_pring) == false)
		_pring->is_wake_pending.store(false, std::memory_order_release);
}

bool api::io_context_uring_cq::_purge_staged_poll(RING* _pring, uint64_t _user_data) noexcept
{
	// - under lockable_sq of the ring. the staged poll with the user_data : dropped (never submitted). false : not staged (in the kernel)
	for (auto iter = _pring->staged.begin(); iter != _pring->staged.end(); ++iter)
	{
		CONTINUE_IF(iter->pop != nullptr || iter->sqe.opcode != OPCODE_POLL_ADD || iter->sqe.user_data != _user_data);

		_pring->staged.erase(iter);
		return true;
	}

	return false;
}

template <class TMATCH>
bool api::io_context_uring_cq::_cancel_staged(RING* _pring, TMATCH&& _match) noexcept
{
	// - under lockable_sq of the ring. the staged ops _match() picks : taken out of the staging (never submitted), delivered with
	//   -ECANCELED by the owner of the ring (synthetic). true : one was found (the owner has to be woken : _wake_staged())
	bool is_found = false;

	for (auto iter = _pring->staged.begin(); iter != _pring->staged.end(); )
	{
		auto* pop = iter->pop;

		if (pop == nullptr || _match(pop) == false)
		{
			++iter;
			continue;
		}

		try
		{
			_pring->synthetic.push_back(pop);
		}
		catch (...)
		{
			// - no memory : stays staged (submitted later, cancelled by the kernel then : the fd is closed)
			++iter;
			continue;
		}

		pop->state = OP_CANCELLED;
		iter = _pring->staged.erase(iter);
		is_found = true;
	}

	return is_found;
}

void api::io_context_uring_cq::_complete_op_final(OP* _pop) noexcept
{
	// - the final completion of an op (its last CQE, or its synthetic -ECANCELED) : out of the ops of its ring, the in-flight count
	//   of its fd, the record freed. the executable is released by the caller (the executor after the delivery)
	{
		std::lock_guard lock(_pop->pring->lockable_sq);

		auto& ops = _pop->pring->ops;

		if (_pop->index < ops.size() && ops[_pop->index] == _pop)
		{
			ops[_pop->index] = ops.back();
			ops[_pop->index]->index = _pop->index;
			ops.pop_back();
		}
	}

	if (auto* pentry = this->_find_entry(_pop->fd); pentry != nullptr)
		pentry->inflight.fetch_sub(1, std::memory_order_acq_rel);

	this->_free_op(_pop);
}

void api::io_context_uring_cq::_adopt_staged(RING* _pring_from, RING* _pring_to) noexcept
{
	// - the staged SQEs and the synthetic completions of a ring without an owner : moved to the ring of this worker (_pring_to :
	//   submitted / delivered by its next pop_results()). the ops move with them (ops, index, pring). both rings locked (std::lock)
	std::scoped_lock lock(_pring_from->lockable_sq, _pring_to->lockable_sq);

	RETURN_IF(_pring_from->staged.empty() && _pring_from->synthetic.empty());

	try
	{
		_pring_to->staged.reserve(_pring_to->staged.size() + _pring_from->staged.size());
		_pring_to->synthetic.reserve(_pring_to->synthetic.size() + _pring_from->synthetic.size());
		_pring_to->ops.reserve(_pring_to->ops.size() + _pring_from->staged.size() + _pring_from->synthetic.size());
	}
	catch (...)
	{
		return;
	}

	auto move_op = [&](OP* _pop)
	{
		auto& ops_from = _pring_from->ops;

		if (_pop->index < ops_from.size() && ops_from[_pop->index] == _pop)
		{
			ops_from[_pop->index] = ops_from.back();
			ops_from[_pop->index]->index = _pop->index;
			ops_from.pop_back();
		}

		_pop->pring = _pring_to;
		_pop->index = _pring_to->ops.size();
		_pring_to->ops.push_back(_pop);
	};

	for (auto& entry : _pring_from->staged)
	{
		if (entry.pop != nullptr)
			move_op(entry.pop);

		_pring_to->staged.push_back(entry);
	}

	for (auto* pop : _pring_from->synthetic)
	{
		move_op(pop);
		_pring_to->synthetic.push_back(pop);
	}

	_pring_from->staged.clear();
	_pring_from->synthetic.clear();
}

void api::io_context_uring_cq::_deliver_synthetic(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept
{
	// - the ops cancelled while staged (this thread owns the ring) : completed and delivered with -ECANCELED like a cancelled request
	while (_count < _max)
	{
		OP* pop;
		{
			std::lock_guard lock(_pring->lockable_sq);

			BREAK_IF(_pring->synthetic.empty());

			pop = _pring->synthetic.back();
			_pring->synthetic.pop_back();
		}

		auto* pexecutable = pop->pexecutable;

		this->_complete_op_final(pop);

		_results[_count].pexecutable = pexecutable;
		_results[_count].result = static_cast<intptr_t>(-ECANCELED);
		_results[_count].param = 0;
		++_count;
	}
}

bool api::io_context_uring_cq::_submit_poll_add(CGHANDLE _handle, uint64_t _event_data, uint32_t _event) noexcept
{
	// - the caller holds a RING_GUARD
	HANDLE_ENTRY* pentry = this->_entry(_handle);
	const bool is_tracked = (pentry != nullptr);
	RING* pring_wake = nullptr;
	RING* pring_wake_old = nullptr;

	{
		std::lock_guard lock_handle(this->m_lockable_handle[static_cast<unsigned>(_handle) % HANDLE_STRIPES]);

		// check) the registration is not the one registered now (detach() meanwhile : polled after its cancel, the poll would keep
		//        the closed socket open. registered again : the poll of the new registration is its own)
		{
			uint64_t event_data_now;

			RETURN_IF(this->_process_get_event_data(_handle, event_data_now) == false || event_data_now != _event_data, false);
		}

		// 1) the ring of the handle (moved to an owned ring when the one it was bound to is an orphan)
		RING* pring_before;
		RING* pring = this->_ring_of_handle(_handle, pring_before);

		RETURN_IF(pring == nullptr, false);

		// 2) the armed poll (not fired yet, or fired and its CQE not taken yet) : cancelled. one poll per registration
		//    (still staged : dropped. in the kernel : a POLL_REMOVE staged for the owner of its ring, before the new poll when it
		//     is the same ring. the -ECANCELED CQE is dropped : not the armed poll any more. a poll that fired : ENOENT, dropped.
		//     its event is processed)
		const uint8_t seq_old = is_tracked ? pentry->armed.load(std::memory_order_acquire) : 0;

		if (seq_old != 0)
		{
			auto* pring_old = (pring_before != nullptr) ? pring_before : pring;
			const uint64_t user_data_old = _poll_user_data(_event_data, seq_old);

			std::lock_guard lock(pring_old->lockable_sq);

			if (this->_purge_staged_poll(pring_old, user_data_old) == false)
			{
				if (io_uring_sqe* psqe_remove = pring_old->stage(nullptr); psqe_remove != nullptr)
				{
					psqe_remove->opcode = OPCODE_POLL_REMOVE;
					psqe_remove->fd = -1;
					psqe_remove->addr = user_data_old;
					psqe_remove->user_data = _event_data | TAG_IGNORE;

					if (pring_old != pring)
						pring_wake_old = pring_old;
				}
			}
		}

		// 3) the poll : staged for the owner of the ring (a poll belongs to the thread that submits it)
		std::lock_guard lock(pring->lockable_sq);

		io_uring_sqe* psqe = pring->stage(nullptr);

		if (psqe == nullptr)
		{
			if (is_tracked)
				pentry->armed.store(0, std::memory_order_release);
		}
		else
		{

			// - the sequence of this poll (1~127. one counter for every ring : the polls of a handle on two rings differ)
			uint8_t seq = 0;

			if (is_tracked)
				seq = static_cast<uint8_t>((this->m_seq_poll.fetch_add(1, std::memory_order_relaxed) % 127) + 1);

			// - every poll is one shot. a registration without EPOLLONESHOT (listen sockets, UDP) is armed again by flush_pending() of
			//   the thread that processed its event
			psqe->opcode = OPCODE_POLL_ADD;
			psqe->fd = _handle;
			psqe->poll32_events = _poll_mask(_event);
			psqe->len = 0;
			psqe->user_data = _poll_user_data(_event_data, seq);

			// - armed
			if (is_tracked)
				pentry->armed.store(seq, std::memory_order_release);

			pring_wake = pring;
		}
	}

	// 4) the owners of the rings submit them (woken out of the locks. the owner itself : with the enter of its next pop_results())
	this->_wake_staged(pring_wake_old);
	this->_wake_staged(pring_wake);

	return (pring_wake != nullptr);
}

int api::io_context_uring_cq::_sync_cancel(RING* _pring, uint64_t _user_data, int _fd, uint32_t _flags, chrono::tick::duration _ticks_wait) noexcept
{
	// - IORING_REGISTER_SYNC_CANCEL till ENOENT (the cancel completes in the task work of the thread that submitted the request :
	//   a 0 means cancelled, asked again till the request is gone). returns 0 (gone), -1 (an error / a timeout : errno)
	RETURN_IF(_pring == nullptr || _pring->fd < 0, 0);

	const auto tick_deadline = chrono::tick::now() + _ticks_wait;

	for (int count_retry = 0; count_retry < 100000; ++count_retry)
	{
		SYNC_CANCEL_REG reg{};
		reg.addr = _user_data;
		reg.fd = _fd;
		reg.flags = _flags;
		reg.timeout_sec = -1;
		reg.timeout_nsec = -1;

		const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(tick_deadline - chrono::tick::now()).count();

		if (nanoseconds <= 0)
		{
			errno = ETIME;
			return -1;
		}

		reg.timeout_sec = nanoseconds / 1000000000ll;
		reg.timeout_nsec = nanoseconds % 1000000000ll;

		const int result = _io_uring_register(_pring->fd, REGISTER_SYNC_CANCEL_OPS, &reg, 1);

		// - gone (ENOENT, or 0 : nothing matched. a kernel answers 0 for ENOENT too). > 0 : cancelled (found) => asked again till gone
		if (result == 0 || (result < 0 && errno == ENOENT))
			return 0;

		// - an error (EINVAL : not supported, a closed fd ...) or the timeout
		if (result < 0 && errno != EALREADY && errno != EINTR)
			return -1;

		std::this_thread::yield();
	}

	errno = ETIME;
	return -1;
}

bool api::io_context_uring_cq::_submit_poll_cancel(CGHANDLE _handle, uint64_t _event_data) noexcept
{
	// - the caller holds a RING_GUARD. the registration was released (no poll is added for it after this : _submit_poll_add())
	HANDLE_ENTRY* pentry = this->_find_entry(_handle);
	const bool is_tracked = (pentry != nullptr);
	RING* pring = nullptr;
	bool is_armed;
	uint64_t user_data_poll;

	{
		std::lock_guard lock_handle(this->m_lockable_handle[static_cast<unsigned>(_handle) % HANDLE_STRIPES]);

		const uint8_t seq_old = is_tracked ? pentry->armed.exchange(0, std::memory_order_acq_rel) : 0;

		is_armed = (is_tracked == false || seq_old != 0);
		user_data_poll = _poll_user_data(_event_data, seq_old);

		if (is_tracked)
		{
			const uint16_t index = pentry->binding.load(std::memory_order_acquire);

			if (index != 0)
				pring = this->m_rings[index - 1].get();
		}

		// - the armed poll still staged (not in the kernel) : dropped here (the handle is closed right after : submitted later it
		//   would poll the closed number, reused by another socket)
		if (is_armed && pring != nullptr)
		{
			std::lock_guard lock(pring->lockable_sq);

			if (this->_purge_staged_poll(pring, user_data_poll))
				is_armed = false;
		}
	}

	// - cancel the armed poll in the kernel and wait till it is gone (a close right after a POLL_REMOVE left the socket open a moment)
	if (is_armed && pring != nullptr)
		this->_sync_cancel(pring, user_data_poll, -1, 0, 5s);

	// - the binding of the handle (the number is reused by the next socket) : cleared when nothing is in flight
	if (is_tracked)
	{
		std::lock_guard lock_handle(this->m_lockable_handle[static_cast<unsigned>(_handle) % HANDLE_STRIPES]);

		if (pentry->inflight.load(std::memory_order_acquire) == 0 && pentry->armed.load(std::memory_order_acquire) == 0)
			pentry->binding.store(0, std::memory_order_release);
	}

	return true;
}

void api::io_context_uring_cq::flush_pending() noexcept
{
	// - the re-arms of the events this thread processed : staged now (the owner of the ring submits them with the enter of its
	//   next pop_results(). a ring of another worker : its owner is woken)
	// - the orphan rings this thread drained : their CQEs were executed (another worker may drain them now)
	this->_release_draining();

	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false);

	this->_process_rearm_pending();
}

void api::io_context_uring_cq::_process_rearm_pending() noexcept
{
	// - the caller holds a RING_GUARD. the poll is written with the mask of the registration now (a modify() meanwhile changed it).
	//   skipped : armed meanwhile (a modify() during the processing), released or registered again (detach : the generation differs)
	RETURN_IF(t_rearm.empty());

	auto iter_keep = t_rearm.begin();

	for (auto iter = t_rearm.begin(); iter != t_rearm.end(); ++iter)
	{
		if (iter->pcontext != this)
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
}

bool api::io_context_uring_cq::attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || _pexecutable == nullptr, false);

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

bool api::io_context_uring_cq::detach(CGHANDLE _handle) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false, false);

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

bool api::io_context_uring_cq::modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable) noexcept
{
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false, false);

	// 1) the registration (a new executable replaces the registered one)
	uint64_t event_data;

	const auto result_update = (_pexecutable != nullptr) ? this->_process_update_handle(_handle, _pexecutable, event_data) : this->_process_get_event_data(_handle, event_data);

	RETURN_IF(result_update == false, false);

	// - the event mask (the re-arm of a registration without EPOLLONESHOT)
	this->_process_set_event_mask(_handle, _event);

	// 2) armed (a one shot poll not fired yet) => cancel it and poll again with the new events. fired => a new poll
	return this->_submit_poll_add(_handle, event_data, _event);
}


//-----------------------------------------------------------------------------
// ops
//-----------------------------------------------------------------------------
api::io_context_uring_cq::OP* api::io_context_uring_cq::_alloc_op() noexcept
{
	{
		std::lock_guard lock(this->m_lockable_pool);

		if (this->m_pool_op.empty() == false)
		{
			auto* pop = this->m_pool_op.back();
			this->m_pool_op.pop_back();
			return pop;
		}
	}

	try
	{
		return new OP{};
	}
	catch (...)
	{
		return nullptr;
	}
}

void api::io_context_uring_cq::_free_op(OP* _pop) noexcept
{
	std::lock_guard lock(this->m_lockable_pool);

	if (this->m_pool_op.size() < MAX_POOL_OP)
	{
		try
		{
			this->m_pool_op.push_back(_pop);
			return;
		}
		catch (...)
		{
		}
	}

	delete _pop;
}

template <class TFILL>
bool api::io_context_uring_cq::_submit_op(CGHANDLE _fd, Iexecutable* _pexecutable, TFILL&& _fill) noexcept
{
	return this->_submit_op_ex(_fd, _pexecutable, [](RING*) noexcept { return true; }, std::forward<TFILL>(_fill));
}

template <class TPREPARE, class TFILL>
bool api::io_context_uring_cq::_submit_op_ex(CGHANDLE _fd, Iexecutable* _pexecutable, TPREPARE&& _prepare, TFILL&& _fill) noexcept
{
	// - _prepare(ring) : called with the ring of the fd before the op is staged (false : not submitted. a multishot recv : its
	//   provided buffer ring). _fill(sqe) : the SQE of the op (fd and user_data are set after)
	// check)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || _pexecutable == nullptr, false);
	RETURN_IF(_fd < 0 || static_cast<unsigned>(_fd) >= this->m_count_handles, false);

	// - the entry of the fd (its in-flight count)
	HANDLE_ENTRY* pentry = this->_entry(_fd);

	RETURN_IF(pentry == nullptr, false);

	// 1) the OP
	OP* pop = this->_alloc_op();

	RETURN_IF(pop == nullptr, false);

	// 2) the ring of the fd
	RING* pring;
	{
		std::lock_guard lock_handle(this->m_lockable_handle[static_cast<unsigned>(_fd) % HANDLE_STRIPES]);

		RING* pring_before;
		pring = this->_ring_of_handle(_fd, pring_before);
	}

	if (pring == nullptr || _prepare(pring) == false)
	{
		this->_free_op(pop);
		return false;
	}

	pop->pexecutable = _pexecutable;
	pop->fd = _fd;
	pop->pring = pring;
	pop->state = OP_STAGED;

	// 3) the SQE : staged for the owner of the ring (an op belongs to the thread that submits it : a short lived thread must not).
	//    the submit takes one reference : released by the executor after the delivery of the final CQE
	{
		std::lock_guard lock(pring->lockable_sq);

		try
		{
			pop->index = pring->ops.size();
			pring->ops.push_back(pop);
		}
		catch (...)
		{
			this->_free_op(pop);
			return false;
		}

		io_uring_sqe* psqe = pring->stage(pop);

		if (psqe == nullptr)
		{
			pring->ops.pop_back();
			this->_free_op(pop);
			return false;
		}

		_fill(psqe);
		psqe->fd = _fd;
		psqe->user_data = UD_OP | reinterpret_cast<uint64_t>(pop);
		pop->opcode = psqe->opcode;

		_pexecutable->add_ref();
		pentry->inflight.fetch_add(1, std::memory_order_acq_rel);
	}

	// 4) the owner submits it (the owner itself : with the enter of its next pop_results())
	this->_wake_staged(pring);

	return true;
}

bool api::io_context_uring_cq::submit_recv(CGHANDLE _fd, void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable) noexcept
{
	return this->_submit_op(_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = OPCODE_RECV;
			_psqe->addr = reinterpret_cast<uint64_t>(_buf);
			_psqe->len = static_cast<uint32_t>(_len);
			_psqe->msg_flags = static_cast<uint32_t>(_msg_flags);
		});
}

bool api::io_context_uring_cq::_prepare_pbuf(RING* _pring) noexcept
{
	// - the provided buffer ring of the ring : registered once, by the first multishot recv on the ring (any thread : a registration
	//   is not a request). false : not available (no memory, the kernel refused it : not tried again)
	const int state = _pring->pbuf_state.load(std::memory_order_acquire);

	RETURN_IF(state != 0, state > 0);

	std::lock_guard lock(_pring->lockable_pbuf);

	const int state_now = _pring->pbuf_state.load(std::memory_order_acquire);

	RETURN_IF(state_now != 0, state_now > 0);
	RETURN_IF(_pring->fd < 0, false);

	const bool is_created = _pring->create_pbuf(PROVIDED_BUFFER_COUNT, PROVIDED_BUFFER_SIZE);

	_pring->pbuf_state.store(is_created ? 1 : -1, std::memory_order_release);

	return is_created;
}

bool api::io_context_uring_cq::submit_recv_multishot(CGHANDLE _fd, Iexecutable* _pexecutable) noexcept
{
	// - a multishot RECV into the provided buffers of the ring of the fd (IOSQE_BUFFER_SELECT : the kernel picks a buffer per arrival,
	//   the length is the buffer's). staged for the owner of the ring like every op
	RETURN_IF(_is_multishot_enabled() == false, false);

	return this->_submit_op_ex(_fd, _pexecutable, [this](RING* _pring) noexcept { return this->_prepare_pbuf(_pring); }, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = OPCODE_RECV;
			_psqe->flags = SQE_FLAG_BUFFER_SELECT;
			_psqe->ioprio = RECV_FLAG_MULTISHOT;
			_psqe->buf_group = PBUF_GROUP;
			_psqe->addr = 0;
			_psqe->len = 0;
			_psqe->msg_flags = 0;
		});
}

const void* api::io_context_uring_cq::get_provided_buffer(size_t _param) const noexcept
{
	// - the data of the provided buffer of a completion (the ring : bits 32~47 of the param, the buffer id : bits 16~31)
	RETURN_IF((_param & CQE_FLAG_BUFFER) == 0, nullptr);

	const auto index_ring = static_cast<unsigned>((_param >> PARAM_RING_SHIFT) & 0xFFFFu);
	const auto bid = static_cast<unsigned>((_param & 0xFFFFFFFFu) >> CQE_BUFFER_SHIFT);

	RETURN_IF(index_ring == 0 || index_ring > this->m_count_rings.load(std::memory_order_acquire) || bid >= PROVIDED_BUFFER_COUNT, nullptr);

	const auto* pring = this->m_rings[index_ring - 1].get();

	RETURN_IF(pring == nullptr || pring->pbuf_data == nullptr, nullptr);

	return pring->pbuf_data + PROVIDED_BUFFER_SIZE * bid;
}

bool api::io_context_uring_cq::return_buffer(size_t _param) noexcept
{
	// - the provided buffer of a completion back to the buffer ring of its ring. any thread (the consumer of a socket bound to another
	//   ring, a thread without a ring) : the tail is written under lockable_pbuf. once per buffer (pbuf_out : a second one is ignored)
	RETURN_IF((_param & CQE_FLAG_BUFFER) == 0, false);

	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false, false);

	const auto index_ring = static_cast<unsigned>((_param >> PARAM_RING_SHIFT) & 0xFFFFu);
	const auto bid = static_cast<unsigned>((_param & 0xFFFFFFFFu) >> CQE_BUFFER_SHIFT);

	RETURN_IF(index_ring == 0 || index_ring > this->m_count_rings.load(std::memory_order_acquire) || bid >= PROVIDED_BUFFER_COUNT, false);

	auto* pring = this->m_rings[index_ring - 1].get();

	RETURN_IF(pring == nullptr || pring->pbuf_state.load(std::memory_order_acquire) <= 0, false);
	RETURN_IF(pring->pbuf_out[bid].exchange(0, std::memory_order_acq_rel) == 0, false);

	pring->pbuf_count_out.fetch_sub(1, std::memory_order_relaxed);

	std::lock_guard lock(pring->lockable_pbuf);

	RETURN_IF(pring->pbuf_ring == nullptr, false);

	pring->give_back_pbuf(bid, PROVIDED_BUFFER_COUNT, PROVIDED_BUFFER_SIZE);

	return true;
}

bool api::io_context_uring_cq::is_ring_owner(CGHANDLE _fd) const noexcept
{
	// - the ring of this worker thread is the ring the fd is bound to (the binding of the handle entry : the ring index + 1)
	RETURN_IF(_fd < 0, false);

	const auto* pring = this->_find_worker_ring();

	RETURN_IF(pring == nullptr, false);

	// - lent (this thread blocks) : not the owner meanwhile (an op it posts would wait for its unblock : sent directly)
	RETURN_IF(pring->state.load(std::memory_order_acquire) != STATE_OWNED, false);

	const auto* pentry = this->_find_entry(_fd);

	return pentry != nullptr && pentry->binding.load(std::memory_order_acquire) == static_cast<uint16_t>(pring->index + 1);
}

size_t api::io_context_uring_cq::provided_buffer_out_count() const noexcept
{
	std::size_t count = 0;
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);

	for (unsigned i = 0; i < count_rings; ++i)
		count += static_cast<std::size_t>(std::max(this->m_rings[i]->pbuf_count_out.load(std::memory_order_relaxed), 0));

	return count;
}

bool api::io_context_uring_cq::submit_recvmsg(CGHANDLE _fd, msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable) noexcept
{
	return this->_submit_op(_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = OPCODE_RECVMSG;
			_psqe->addr = reinterpret_cast<uint64_t>(_msg);
			_psqe->len = 1;
			_psqe->msg_flags = static_cast<uint32_t>(_msg_flags);
		});
}

bool api::io_context_uring_cq::submit_send(CGHANDLE _fd, const void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept
{
	return this->_submit_op(_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = _zerocopy ? OPCODE_SEND_ZC : OPCODE_SEND;
			_psqe->addr = reinterpret_cast<uint64_t>(_buf);
			_psqe->len = static_cast<uint32_t>(_len);
			_psqe->msg_flags = static_cast<uint32_t>(_msg_flags);
		});
}

bool api::io_context_uring_cq::submit_sendmsg(CGHANDLE _fd, const msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept
{
	return this->_submit_op(_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = _zerocopy ? OPCODE_SENDMSG_ZC : OPCODE_SENDMSG;
			_psqe->addr = reinterpret_cast<uint64_t>(_msg);
			_psqe->len = 1;
			_psqe->msg_flags = static_cast<uint32_t>(_msg_flags);
		});
}

bool api::io_context_uring_cq::submit_accept(CGHANDLE _listen_fd, sockaddr* _addr, socklen_t* _addrlen, int _flags, bool _multishot, Iexecutable* _pexecutable) noexcept
{
	// - multishot : no address (the kernel rejects it)
	RETURN_IF(_multishot && _addr != nullptr, false);

	return this->_submit_op(_listen_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = OPCODE_ACCEPT;
			_psqe->addr = reinterpret_cast<uint64_t>(_addr);
			_psqe->off = reinterpret_cast<uint64_t>(_addrlen);
			_psqe->accept_flags = static_cast<uint32_t>(_flags);
			_psqe->ioprio = _multishot ? static_cast<uint16_t>(ACCEPT_FLAG_MULTISHOT) : 0;
		});
}

bool api::io_context_uring_cq::submit_connect(CGHANDLE _fd, const sockaddr* _addr, socklen_t _addrlen, Iexecutable* _pexecutable) noexcept
{
	return this->_submit_op(_fd, _pexecutable, [&](io_uring_sqe* _psqe)
		{
			_psqe->opcode = OPCODE_CONNECT;
			_psqe->addr = reinterpret_cast<uint64_t>(_addr);
			_psqe->off = static_cast<uint64_t>(_addrlen);
		});
}

bool api::io_context_uring_cq::submit_cancel(Iexecutable* _pexecutable) noexcept
{
	// - every op of the executable (on every ring) : still staged => taken out, delivered with -ECANCELED by the owner of its ring.
	//   in the kernel => IORING_OP_ASYNC_CANCEL by user_data (submitted now : the owner of a cancel does not matter), it completes
	//   with -ECANCELED (delivered)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || _pexecutable == nullptr, false);

	return this->_cancel_ops([&](OP* _pop) { return _pop->pexecutable == _pexecutable; });
}

bool api::io_context_uring_cq::submit_cancel_fd(CGHANDLE _fd) noexcept
{
	// - every op of the fd (the readiness poll of the fd is not touched : detach() cancels it). the ops complete with -ECANCELED (delivered)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || _fd < 0, false);

	return this->_cancel_ops([&](OP* _pop) { return _pop->fd == _fd; });
}

template <class TMATCH>
bool api::io_context_uring_cq::_cancel_ops(TMATCH&& _match) noexcept
{
	// - the caller holds a RING_GUARD. the ops _match() picks on every ring : staged => synthetic -ECANCELED (the owner is woken),
	//   in the kernel => cancelled now. the owner of the ring : an async cancel each, written and submitted. another thread :
	//   IORING_REGISTER_SYNC_CANCEL per op without a wait (the cancel runs in this thread at once and submits nothing : never an
	//   enter of a ring this thread does not own. the cancelled op completes with -ECANCELED on its ring). true : one was found
	bool is_found = false;
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);
	RING* pring_own = this->_find_worker_ring();
	std::vector<uint64_t> user_data_cancel;

	for (unsigned i = 0; i < count_rings; ++i)
	{
		auto* pring = this->m_rings[i].get();
		bool is_staged = false;

		user_data_cancel.clear();

		{
			std::lock_guard lock(pring->lockable_sq);

			// 1) the staged ops
			is_staged = this->_cancel_staged(pring, _match);

			// 2) the ops in the kernel
			const bool is_own = (pring == pring_own);
			bool is_written = false;

			for (auto* pop : pring->ops)
			{
				CONTINUE_IF(pop->state != OP_SUBMITTED || _match(pop) == false);

				const uint64_t user_data_op = UD_OP | reinterpret_cast<uint64_t>(pop);

				if (is_own)
				{
					io_uring_sqe* psqe = pring->alloc_sqe();

					BREAK_IF(psqe == nullptr);

					psqe->opcode = OPCODE_ASYNC_CANCEL;
					psqe->fd = -1;
					psqe->addr = user_data_op;
					psqe->user_data = UD_IGNORE;
					is_written = true;
				}
				else
				{
					try { user_data_cancel.push_back(user_data_op); } catch (...) { break; }
				}
			}

			if (is_written)
				pring->submit();

			// 3) another thread : cancelled now, under the lock (the op can not complete and be freed meanwhile : its OP record is
			//    reused by the next op, a cancel by its address after that would cancel another op. the owner never holds this lock
			//    while it waits in the kernel)
			for (const auto user_data_op : user_data_cancel)
				this->_cancel_now(pring, user_data_op);

			is_found = is_found || is_staged || is_written || (user_data_cancel.empty() == false);
		}

		// 4) the synthetic completions : the owner of the ring is woken
		if (is_staged)
			this->_wake_staged(pring);
	}

	return is_found;
}

void api::io_context_uring_cq::_cancel_now(RING* _pring, uint64_t _user_data) noexcept
{
	// - IORING_REGISTER_SYNC_CANCEL by user_data without a wait (timeout 0) : the request is cancelled in this thread now (a poll
	//   armed request is removed at once, its -ECANCELED CQE comes by the task work of its owner). nothing is submitted
	RETURN_IF(_pring == nullptr || _pring->fd < 0);

	SYNC_CANCEL_REG reg{};
	reg.addr = _user_data;
	reg.fd = -1;
	reg.flags = 0;
	reg.timeout_sec = 0;
	reg.timeout_nsec = 0;

	static_cast<void>(_io_uring_register(_pring->fd, REGISTER_SYNC_CANCEL_OPS, &reg, 1));
}

bool api::io_context_uring_cq::wait_cancelled_fd(CGHANDLE _fd, chrono::tick::duration _ticks_wait, bool _is_wait_delivery) noexcept
{
	// - the ops of the fd still staged : taken out (delivered with -ECANCELED by the owner). IORING_REGISTER_SYNC_CANCEL (fd, all) on
	//   every ring with an op of the fd in the kernel : the kernel is done with the file when it returns (the fd can be closed : its
	//   number is reused at once). then the delivery of their completions is waited for (the in-flight count of the fd), unless
	//   this thread is the owner of a ring (it delivers them itself after)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || _fd < 0 || static_cast<unsigned>(_fd) >= this->m_count_handles, false);

	const auto tick_deadline = chrono::tick::now() + _ticks_wait;
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);
	bool is_done = true;

	for (unsigned i = 0; i < count_rings; ++i)
	{
		auto* pring = this->m_rings[i].get();
		bool is_op = false;
		bool is_staged = false;

		{
			std::lock_guard lock(pring->lockable_sq);

			is_staged = this->_cancel_staged(pring, [&](OP* _pop) { return _pop->fd == _fd; });

			for (auto* pop : pring->ops)
			{
				if (pop->fd == _fd && pop->state == OP_SUBMITTED)
				{
					is_op = true;
					break;
				}
			}
		}

		if (is_staged)
			this->_wake_staged(pring);

		CONTINUE_IF(is_op == false);

		const auto ticks_left = tick_deadline - chrono::tick::now();

		if (this->_sync_cancel(pring, 0, _fd, CANCEL_FLAG_FD | CANCEL_FLAG_ALL, (ticks_left > chrono::tick::duration::zero()) ? ticks_left : chrono::tick::duration(1ms)) != 0)
			is_done = false;
	}

	// - the delivery (a worker of this io_context delivers them itself : not waited for. _is_wait_delivery false : the caller needs
	//   the kernel done with the file only)
	if (_is_wait_delivery && this->_find_worker_ring() == nullptr)
	{
		for (auto* pentry = this->_find_entry(_fd); pentry != nullptr && pentry->inflight.load(std::memory_order_acquire) != 0; )
		{
			if (chrono::tick::now() >= tick_deadline)
				return false;

			std::this_thread::yield();
		}
	}

	return is_done;
}

int api::io_context_uring_cq::inflight_count(CGHANDLE _fd) const noexcept
{
	RETURN_IF(_fd < 0 || static_cast<unsigned>(_fd) >= this->m_count_handles, 0);

	const auto* pentry = this->_find_entry(_fd);

	return (pentry != nullptr) ? pentry->inflight.load(std::memory_order_acquire) : 0;
}


//-----------------------------------------------------------------------------
// post
//-----------------------------------------------------------------------------
bool api::io_context_uring_cq::post_executable(uint32_t /*_bytes_transferred*/, uintptr_t /*_completion_key*/, Iexecutable* _pexecutable) noexcept
{
	RETURN_IF(this->m_handle_epoll == INVALID_CGHANDLE_VALUE, false);
	RETURN_IF(_pexecutable == nullptr, false);

	bool is_wake = false;
	bool is_notify = false;

	{
		std::lock_guard lock(this->m_lockable_post);

		// check) destroyed
		RETURN_IF(this->m_is_post_closed, false);

		// 1) queue (the count before the wake below : an owner about to wait checks it after it marked itself waiting)
		try
		{
			this->m_queue_post.push_back(_pexecutable);
		}
		catch (...)
		{
			return false;
		}

		this->m_count_post.store(this->m_queue_post.size(), std::memory_order_seq_cst);

		// 2) add_ref
		_pexecutable->add_ref();

		// 3) wake a ring when none was woken yet, a thread without a ring sleeping on the queue
		if (this->m_is_signaled == false)
		{
			this->m_is_signaled = true;
			is_wake = true;
		}

		is_notify = (this->m_count_sleepers != 0);
	}

	// 4) notify (out of the lock)
	if (is_notify)
		this->m_cv_post.notify_one();

	if (is_wake && this->_wake_ring() == false)
	{
		// - no ring is owned (the workers did not bind yet, or left) : the next bind_worker() takes the queue
		std::lock_guard lock(this->m_lockable_post);
		this->m_is_signaled = false;
	}

	return true;
}

bool api::io_context_uring_cq::post_executable(uint32_t _bytes_transferred, uintptr_t _completion_key, object_ptr<Iexecutable>&& _pexecutable) noexcept
{
	auto pexecutable = _pexecutable.get();

	RETURN_IF(pexecutable == nullptr, false);
	RETURN_IF(this->post_executable(_bytes_transferred, _completion_key, pexecutable) == false, false);

	// - the reference of the object_ptr is released (post_executable() above took its own)
	_pexecutable.reset();

	return true;
}

size_t api::io_context_uring_cq::queued_count() const noexcept
{
	std::lock_guard lock(this->m_lockable_post);

	return this->m_queue_post.size();
}

int api::io_context_uring_cq::_pop_posted(POP_RESULT* _results, int _count, int _max, bool _is_ring) noexcept
{
	// - a batch of the post queue (a part : the other rings take the rest). _is_ring : called for the wake of a ring (the next ring
	//   is woken when the queue is not empty after, the signal is cleared when it is)
	bool is_wake = false;

	{
		std::lock_guard lock(this->m_lockable_post);

		const auto size_queue = this->m_queue_post.size();
		const auto count_room = static_cast<std::size_t>(std::max(_max - _count, 1));
		const auto count_take = (size_queue == 0) ? 0 : std::clamp<std::size_t>(size_queue / 8, 1, std::min(MAX_POST_BATCH, count_room));

		for (std::size_t i = 0; i < count_take; ++i)
		{
			_results[_count].pexecutable = this->m_queue_post.front();
			_results[_count].result = EPOLLIN;
			_results[_count].param = 0;
			++_count;

			this->m_queue_post.pop_front();
		}

		this->m_count_post.store(this->m_queue_post.size(), std::memory_order_seq_cst);

		if (_is_ring)
		{
			if (this->m_queue_post.empty())
				this->m_is_signaled = false;
			else
				is_wake = true;
		}
	}

	if (is_wake && this->_wake_ring() == false)
	{
		std::lock_guard lock(this->m_lockable_post);
		this->m_is_signaled = false;
	}

	return _count;
}

int api::io_context_uring_cq::_pop_posted_wait(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept
{
	// - a thread without a ring : the post queue only (a thread pumping the executor : wait_complete_* with execute(2ms))
	const bool is_wait_forever = (_ticks_wait == chrono::tick::duration::max());
	const auto tick_deadline = is_wait_forever ? chrono::tick::time_point::max() : chrono::tick::now() + _ticks_wait;

	for (;;)
	{
		{
			std::unique_lock lock(this->m_lockable_post);

			RETURN_IF(this->m_is_post_closed || this->m_handle_epoll == INVALID_CGHANDLE_VALUE, 0);

			if (this->m_queue_post.empty() == false)
				break;

			RETURN_IF(_ticks_wait == chrono::tick::duration::zero(), 0);

			if (is_wait_forever == false && chrono::tick::now() >= tick_deadline)
				return 0;

			++this->m_count_sleepers;

			if (is_wait_forever)
				this->m_cv_post.wait(lock);
			else
				this->m_cv_post.wait_until(lock, tick_deadline);

			--this->m_count_sleepers;
		}
	}

	return this->_pop_posted(_results, 0, _max, false);
}


//-----------------------------------------------------------------------------
// completions
//-----------------------------------------------------------------------------
void api::io_context_uring_cq::_process_wake(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept
{
	// - the CQE of a wake (post : a batch of the post queue. a stage, dispose/destroy : nothing to take, the queue is checked)
	static_cast<void>(_pring);

	_count = this->_pop_posted(_results, _count, _max, true);
}

void api::io_context_uring_cq::_process_wake_poll(RING* _pring, const void* _pcqe) noexcept
{
	// - a CQE of the poll of the wake eventfd : the count read (the next writes wake again). the poll ended (no F_MORE : its owner
	//   exited, the kernel cancelled it) and it is the poll armed last on the ring of this thread => armed again (a worker draining
	//   an orphan does not : the next owner arms its own at bind_worker())
	const auto& cqe = *static_cast<const io_uring_cqe*>(_pcqe);

	if (cqe.res >= 0 && _pring->wake_fd >= 0)
	{
		eventfd_t value;
		static_cast<void>(::eventfd_read(_pring->wake_fd, &value));
	}

	RETURN_IF((cqe.flags & CQE_FLAG_MORE) != 0 || _pring != this->_find_worker_ring());

	std::lock_guard lock(_pring->lockable_sq);

	if (static_cast<uint16_t>(cqe.user_data >> UD_WAKE_GENERATION_SHIFT) == _pring->wake_generation)
		_pring->write_wake_poll();
}

void api::io_context_uring_cq::_process_cqe(RING* _pring, const void* _pcqe, POP_RESULT* _results, int& _count, int _max) noexcept
{
	const auto& cqe = *static_cast<const io_uring_cqe*>(_pcqe);
	const uint64_t user_data = cqe.user_data;

	switch (user_data & UD_MASK_KIND)
	{
	case UD_WAKE:
		// - the poll of the wake eventfd (a wake of another thread) or the nop of the owner (its own wake)
		if ((user_data & UD_WAKE_POLL) != 0)
			this->_process_wake_poll(_pring, _pcqe);

		this->_process_wake(_pring, _results, _count, _max);
		return;

	case UD_IGNORE:
		return;

	case UD_OP:
		{
			auto* pop = reinterpret_cast<OP*>(user_data & UD_MASK_PAYLOAD);
			auto* pexecutable = pop->pexecutable;

			// - an accepted socket : bound to a ring now (round robin), not by the first submit of its handler on this thread
			if (pop->opcode == OPCODE_ACCEPT && cqe.res >= 0)
				this->_bind_accepted(_pring, static_cast<CGHANDLE>(cqe.res));

			if ((cqe.flags & CQE_FLAG_MORE) != 0)
			{
				// - more CQEs follow (multishot accept, the first CQE of a zero copy send) : one more reference for this delivery
				pexecutable->add_ref();
			}
			else
			{
				// - the final CQE : the op is gone (the reference of the submit goes with this delivery)
				this->_complete_op_final(pop);
			}

			// - a provided buffer (a multishot recv) : out till given back (return_buffer). the param carries its ring (the ring of this CQ)
			size_t param = static_cast<size_t>(cqe.flags);

			if ((cqe.flags & CQE_FLAG_BUFFER) != 0)
			{
				const unsigned bid = cqe.flags >> CQE_BUFFER_SHIFT;

				param |= static_cast<size_t>(_pring->index + 1) << PARAM_RING_SHIFT;

				if (_pring->pbuf_out && bid < PROVIDED_BUFFER_COUNT)
				{
					_pring->pbuf_out[bid].store(1, std::memory_order_release);
					_pring->pbuf_count_out.fetch_add(1, std::memory_order_relaxed);
				}
			}

			_results[_count].pexecutable = pexecutable;
			_results[_count].result = static_cast<intptr_t>(cqe.res);
			_results[_count].param = param;
			++_count;
		}
		return;

	default:
		break;
	}

	// - a readiness poll
	//   . a cancel / update of a poll (TAG_IGNORE) : dropped
	RETURN_IF((user_data & TAG_IGNORE) != 0);

	const auto event_data = _poll_event_data(user_data);
	const auto seq = _poll_seq(user_data);
	const auto handle = _get_event_handle(event_data);

	// - the poll completed : not armed any more when it is the armed poll (a poll replaced by modify() meanwhile : the armed one
	//   stays). a registration without EPOLLONESHOT (UDP, listen) is armed again by flush_pending() of this thread after the
	//   events are processed (like EPOLLET : the processing drains the socket, the arrivals after wake one CQE)
	const bool is_disarmed = this->_disarm(handle, seq);

	uint64_t event_data_now;
	uint32_t events_now;
	const bool is_registered = this->_process_get_event_mask(handle, event_data_now, events_now) && event_data_now == event_data;

	// - cancelled
	//   . by modify() / detach() : not the armed poll any more (or released) => dropped
	//   . the armed poll of a live registration : cancelled by the kernel (the thread that submitted it exited) => polled again
	//     by this thread (on its ring : the readiness is checked when it is added, the event is not lost)
	if (cqe.res == -ECANCELED)
	{
		if (seq != 0 && is_disarmed && is_registered)
		{
			try { t_rearm.push_back(REARM{ this, handle, event_data }); } catch (...) {}
		}

		return;
	}

	if (is_disarmed && is_registered && (events_now & EPOLLONESHOT) == 0)
	{
		try { t_rearm.push_back(REARM{ this, handle, event_data }); } catch (...) {}
	}

	// - the executable (add_ref. nullptr : the handle was closed : skipped)
	auto* pexecutable = this->_process_get_executable(event_data);

	RETURN_IF(pexecutable == nullptr);

	_results[_count].pexecutable = pexecutable;
	_results[_count].result = (cqe.res >= 0) ? static_cast<intptr_t>(static_cast<uint32_t>(cqe.res)) : static_cast<intptr_t>(EPOLLERR | EPOLLHUP);
	_results[_count].param = 0;
	++_count;
}

int api::io_context_uring_cq::_drain_ring(RING* _pring, POP_RESULT* _results, int _count, int _max) noexcept
{
	// - takes the CQEs of the ring (this thread is its consumer : the owner, or a worker draining an orphan)
	RETURN_IF(_pring->cqes == nullptr, _count);

	unsigned head = __atomic_load_n(_pring->cq_head, __ATOMIC_ACQUIRE);
	const unsigned tail = __atomic_load_n(_pring->cq_tail, __ATOMIC_ACQUIRE);

	while (head != tail && _count < _max)
	{
		const io_uring_cqe& cqe = _pring->cqes[head & _pring->cq_mask];
		++head;

		this->_process_cqe(_pring, &cqe, _results, _count, _max);
	}

	__atomic_store_n(_pring->cq_head, head, __ATOMIC_RELEASE);

	return _count;
}

void api::io_context_uring_cq::_drain_orphans(POP_RESULT* _results, int& _count, int _max) noexcept
{
	// - the rings without an owner : their CQEs (the requests of the exited worker : -ECANCELED), their staged SQEs and synthetic
	//   completions (moved to the ring of this worker : submitted / delivered by its next pop_results()). taken by one worker at a
	//   time (STATE_DRAINING while draining : a worker binding meanwhile creates a ring)
	const unsigned count_rings = this->m_count_rings.load(std::memory_order_acquire);
	RING* pring_own = this->_find_worker_ring();

	for (unsigned i = 0; i < count_rings && _count < _max; ++i)
	{
		auto* pring = this->m_rings[i].get();

		CONTINUE_IF(pring == pring_own);

		// - a lent ring (its owner blocks) : drained like an orphan, the results of the executable the owner blocked in deferred to it
		//   (DRAINING_LENT till this thread executed them : t_draining, the room reserved first : a release before would let another
		//    worker run the next CQEs concurrently)
		if (pring->state.load(std::memory_order_acquire) == STATE_LENT)
		{
			CONTINUE_IF(pring_own == nullptr);

			try
			{
				t_draining.reserve(t_draining.size() + 1);
			}
			catch (...)
			{
				continue;
			}

			if (this->_take_lent(pring, _results, _count, _max))
				t_draining.push_back(DRAINING{ this, pring, true });

			continue;
		}

		CONTINUE_IF(pring->state.load(std::memory_order_acquire) != STATE_ORPHAN);

		int expected = STATE_ORPHAN;

		CONTINUE_IF(pring->state.compare_exchange_strong(expected, STATE_DRAINING, std::memory_order_acq_rel) == false);

		if (pring_own != nullptr)
			this->_adopt_staged(pring, pring_own);

		const int count_before = _count;

		_count = this->_drain_ring(pring, _results, _count, _max);

		// - CQEs taken : the ring stays STATE_DRAINING till this thread executed them (_release_draining() : flush_pending() after the
		//   batch, the next pop_results()). a worker draining it meanwhile would run its next CQEs concurrently with these (out of
		//   order : the CQEs of a multishot recv are the data of a stream)
		if (_count != count_before)
		{
			try
			{
				t_draining.push_back(DRAINING{ this, pring });
				continue;
			}
			catch (...)
			{
			}
		}

		pring->state.store(STATE_ORPHAN, std::memory_order_release);
	}
}

void api::io_context_uring_cq::_release_draining() noexcept
{
	// - the orphan rings this thread drained : orphans again (their CQEs were executed)
	RETURN_IF(t_draining.empty());

	auto iter_keep = t_draining.begin();

	for (auto iter = t_draining.begin(); iter != t_draining.end(); ++iter)
	{
		if (iter->pcontext != this)
		{
			*iter_keep++ = *iter;
			continue;
		}

		if (iter->is_lent)
			this->_release_lent(static_cast<RING*>(iter->pring));
		else
			static_cast<RING*>(iter->pring)->state.store(STATE_ORPHAN, std::memory_order_release);
	}

	t_draining.erase(iter_keep, t_draining.end());
}

api::io_context_uring_cq::BATCH*& api::io_context_uring_cq::current_batch() noexcept
{
	thread_local BATCH* t_pbatch = nullptr;

	return t_pbatch;
}

void api::io_context_uring_cq::thread_blocked() noexcept
{
	// - this thread blocks (scoped_thread_block : a sync query ..., a wait pumping the executor). a worker : its ring is lent to the
	//   other workers till it unblocks (the CQEs of its requests are posted meanwhile : the task work wakes a sleeping owner. a busy
	//   owner in user space delays them : COOP_TASKRUN, not helped). nested blocks : the outermost one
	WORKER_RING* pworker = nullptr;

	for (auto& iter : t_worker_ring)
	{
		if (iter.pcontext == this)
		{
			pworker = &iter;
			break;
		}
	}

	RETURN_IF(pworker == nullptr);
	RETURN_IF(pworker->block_depth++ != 0);

	auto* pring = static_cast<RING*>(pworker->pring);
	bool is_lent = false;

	{
		std::lock_guard lock(pring->lockable_lend);

		// 1) the batch this thread executes now : the rest is executed first by the worker that drains the ring (in order. the
		//    results of the executable running now - the one blocked - go to this thread after it : deferred). the room first
		auto* pbatch = current_batch();
		const bool is_batch = (pbatch != nullptr && pbatch->pcontext == this && pbatch->pcount != nullptr && pbatch->index < *pbatch->pcount);

		if (is_batch)
		{
			try
			{
				pring->carry.reserve(pring->carry.size() + static_cast<std::size_t>(*pbatch->pcount - pbatch->index));
			}
			catch (...)
			{
				return;
			}
		}

		// 2) lent (the state changes under lockable_lend : a worker taking it reads the carry under it)
		int expected = STATE_OWNED;

		if (pring->state.compare_exchange_strong(expected, STATE_LENT, std::memory_order_acq_rel))
		{
			pring->is_reclaim = false;
			pring->pblocked = is_batch ? pbatch->presults[pbatch->index].pexecutable : nullptr;

			if (is_batch)
			{
				for (int i = pbatch->index + 1; i < *pbatch->pcount; ++i)
					pring->carry.push_back(pbatch->presults[i]);

				*pbatch->pcount = pbatch->index + 1;
			}

			is_lent = true;
		}
	}

	RETURN_IF(is_lent == false);

	this->m_count_lent.fetch_add(1, std::memory_order_acq_rel);

	// 3) an idle worker drains it now (the others check the lent rings every LENT_WAIT)
	this->_wake_ring();
}

void api::io_context_uring_cq::thread_unblocked() noexcept
{
	// - the outermost unblock : the ring is taken back. a worker draining it gives it back at its release (is_reclaim). the results
	//   deferred to this thread are executed by its next pop_results() before the CQ (in order)
	WORKER_RING* pworker = nullptr;

	for (auto& iter : t_worker_ring)
	{
		if (iter.pcontext == this)
		{
			pworker = &iter;
			break;
		}
	}

	RETURN_IF(pworker == nullptr || pworker->block_depth == 0);
	RETURN_IF(--pworker->block_depth != 0);

	auto* pring = static_cast<RING*>(pworker->pring);

	std::lock_guard lock(pring->lockable_lend);

	int expected = STATE_LENT;

	if (pring->state.compare_exchange_strong(expected, STATE_OWNED, std::memory_order_acq_rel))
	{
		pring->pblocked = nullptr;
		this->m_count_lent.fetch_sub(1, std::memory_order_acq_rel);
	}
	else if (expected == STATE_DRAINING_LENT)
	{
		pring->is_reclaim = true;
	}
}

bool api::io_context_uring_cq::_take_lent(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept
{
	// - a lent ring (this thread is a worker with a ring of its own) : STATE_DRAINING_LENT, then the rest of the batch of the owner
	//   (carry) and the CQEs, the results of the executable the owner blocked in moved to deferred (they run after it, on the owner).
	//   the staged SQEs (the ops and the polls the handlers of the ring wrote) move to the ring of this worker (submitted by it : the
	//   owner does not enter its ring while it blocks). true : held (released by _release_draining() after the results ran)
	{
		std::lock_guard lock(_pring->lockable_lend);

		int expected = STATE_LENT;

		RETURN_IF(_pring->state.compare_exchange_strong(expected, STATE_DRAINING_LENT, std::memory_order_acq_rel) == false, false);
	}

	if (auto* pring_own = this->_find_worker_ring(); pring_own != nullptr)
		this->_adopt_staged(_pring, pring_own);

	bool is_cq = true;
	Iexecutable* pblocked;

	{
		std::lock_guard lock(_pring->lockable_lend);

		pblocked = _pring->pblocked;

		// 1) the rest of the batch of the owner first (older than the CQEs). the CQ waits while some of it is left (in order)
		std::size_t index = 0;

		for (; index < _pring->carry.size(); ++index)
		{
			const auto& result = _pring->carry[index];

			if (result.pexecutable != nullptr && result.pexecutable == pblocked)
			{
				try { _pring->deferred.push_back(result); } catch (...) { break; }
				continue;
			}

			BREAK_IF(_count >= _max);

			_results[_count++] = result;
		}

		_pring->carry.erase(_pring->carry.begin(), _pring->carry.begin() + static_cast<std::ptrdiff_t>(index));

		is_cq = _pring->carry.empty();
	}

	// 2) the CQEs (the results of pblocked deferred : kept in the order they came)
	if (is_cq && _count < _max)
	{
		const int count_before = _count;

		_count = this->_drain_ring(_pring, _results, _count, _max);

		if (pblocked != nullptr)
		{
			std::lock_guard lock(_pring->lockable_lend);

			int index_to = count_before;

			for (int i = count_before; i < _count; ++i)
			{
				if (_results[i].pexecutable == pblocked)
				{
					try
					{
						_pring->deferred.push_back(_results[i]);
						continue;
					}
					catch (...)
					{
						// - no memory : executed here (the owner is blocked in its handler : out of order for that executable, rare)
					}
				}

				_results[index_to++] = _results[i];
			}

			_count = index_to;
		}
	}

	// - held till the results ran (also when none : the deferred ones are the owner's)
	return true;
}

bool api::io_context_uring_cq::_deliver_lent(RING* _pring, POP_RESULT* _results, int& _count, int _max) noexcept
{
	// - the owner took its ring back : the rest of its batch nobody executed (carry), then the results deferred to it, before the CQ.
	//   false : some are left (the CQ waits : in order)
	std::lock_guard lock(_pring->lockable_lend);

	RETURN_IF(_pring->carry.empty() && _pring->deferred.empty(), true);

	auto take = [&](std::vector<POP_RESULT>& _from)
	{
		std::size_t index = 0;

		for (; index < _from.size() && _count < _max; ++index)
			_results[_count++] = _from[index];

		_from.erase(_from.begin(), _from.begin() + static_cast<std::ptrdiff_t>(index));
	};

	take(_pring->carry);

	if (_pring->carry.empty())
		take(_pring->deferred);

	return _pring->carry.empty() && _pring->deferred.empty();
}

void api::io_context_uring_cq::_release_lent(RING* _pring) noexcept
{
	// - the results of a lent ring ran : lent again, or the owner's when it unblocked meanwhile
	std::lock_guard lock(_pring->lockable_lend);

	if (_pring->is_reclaim)
	{
		_pring->is_reclaim = false;
		_pring->pblocked = nullptr;
		_pring->state.store(STATE_OWNED, std::memory_order_release);
		this->m_count_lent.fetch_sub(1, std::memory_order_acq_rel);
	}
	else
	{
		_pring->state.store(STATE_LENT, std::memory_order_release);
	}
}

int api::io_context_uring_cq::pop_results(POP_RESULT* _results, int _max, chrono::tick::duration _ticks_wait) noexcept
{
	RETURN_IF(_results == nullptr || _max <= 0, 0);

	// - the orphan rings drained by the last pop of this thread (executed by now : the executor did not call flush_pending())
	this->_release_draining();

	// - a thread without a ring : the post queue
	RING* pring = this->_find_worker_ring();

	if (pring == nullptr)
		return this->_pop_posted_wait(_results, _max, _ticks_wait);

	// - the ring of this thread is lent (this thread blocks and pumps the executor : wait_complete_* ...), or given back while a worker
	//   still runs its results : the post queue only (the other workers drain the ring. not the CQ here : out of order)
	if (pring->state.load(std::memory_order_acquire) != STATE_OWNED)
		return this->_pop_posted_wait(_results, _max, std::min(_ticks_wait, chrono::tick::duration(LENT_WAIT)));

	// check) (destroy() waits for the threads in here : RING_GUARD)
	RING_GUARD guard(*this);

	RETURN_IF(guard.is_valid == false || pring->cqes == nullptr, 0);

	// 1) the SQEs staged on this ring (the re-arms of the last batch first, when the executor did not call flush_pending()) : into
	//    the SQ, submitted with the enter below (one syscall). the wake flag is cleared before the flush : a stage after it wakes again
	this->_process_rearm_pending();

	pring->is_wake_pending.store(false, std::memory_order_release);

	//    the flush and the submit under one lock : an op marked OP_SUBMITTED is in the kernel when another thread sees it (a cancel
	//    of another thread runs in the kernel at once : an op written into the SQ and not entered yet was missed by it and submitted
	//    after it. a multishot accept submitted so after the cancel of acceptor::stop() kept the closed listen socket listening
	//    : uds.listen_name_and_release). the wait is a second enter below, out of the lock (only when nothing completed meanwhile)
	unsigned to_submit;
	{
		std::lock_guard lock(pring->lockable_sq);
		pring->flush_staged();

		if (pring->publish() != 0)
			static_cast<void>(pring->submit());

		to_submit = pring->pending();
	}

	// 2) the ring was lent (this thread blocked) : the rest of its batch nobody executed and the results deferred to it first. some left
	//    => no CQ this time (in order)
	int count = 0;

	if (this->_deliver_lent(pring, _results, count, _max) == false)
		return count;

	//    the ops cancelled while staged : delivered now (no wait below when there is one)
	this->_deliver_synthetic(pring, _results, count, _max);

	// 3) wait when the CQ is empty (the task work pending : run by the enter)
	unsigned head = __atomic_load_n(pring->cq_head, __ATOMIC_ACQUIRE);
	unsigned tail = __atomic_load_n(pring->cq_tail, __ATOMIC_ACQUIRE);
	const bool is_taskrun = (__atomic_load_n(pring->sq_flags, __ATOMIC_ACQUIRE) & SQ_FLAG_TASKRUN) != 0;
	bool is_take_post = false;

	if (head == tail || is_taskrun || to_submit != 0)
	{
	#if CGDK_IO_URING_CQ_SUPPORTED
		// - a ring without an owner (a worker that left) : its CQ is drained after this wait => a short wait while one exists.
		//   a result taken already (a synthetic completion) : no wait
		chrono::tick::duration ticks_wait = (count != 0) ? chrono::tick::duration::zero() : _ticks_wait;

		if (ticks_wait > chrono::tick::duration(10ms) && this->_has_orphan())
			ticks_wait = 10ms;

		// - a lent ring (its owner blocks) : drained after this wait => LENT_WAIT at most (its CQEs wake nobody)
		if (ticks_wait > chrono::tick::duration(LENT_WAIT) && this->m_count_lent.load(std::memory_order_acquire) != 0)
			ticks_wait = LENT_WAIT;

		bool is_wait = (head == tail && ticks_wait != chrono::tick::duration::zero());

		// - an owner about to wait takes the posts queued : a post wakes one ring (the queue stays signaled till that ring takes it), a
		//   busy one when no owner waited at that moment (its owner in a long executable : the posts waited for it while the others
		//   slept. emergency.acceptor_works_while_system_executor_is_jammed). is_waiting first, then the count (seq_cst, the post the
		//   other way round) : a post after the check finds this ring waiting and wakes it
		if (is_wait)
		{
			pring->is_waiting.store(true, std::memory_order_seq_cst);

			if (this->m_count_post.load(std::memory_order_seq_cst) != 0)
			{
				pring->is_waiting.store(false, std::memory_order_release);
				is_wait = false;
				is_take_post = true;
			}
		}

		const bool is_wait_forever = (ticks_wait == chrono::tick::duration::max());
		unsigned flags = 0;
		unsigned min_complete = 0;
		__kernel_timespec timespec_wait{};
		io_uring_getevents_arg arg{};
		const void* parg = nullptr;
		std::size_t size_arg = 0;

		if (head == tail || is_taskrun)
			flags |= IORING_ENTER_GETEVENTS;

		if (is_wait)
		{
			min_complete = 1;

			if (is_wait_forever == false)
			{
				const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(ticks_wait).count();
				timespec_wait.tv_sec = nanoseconds / 1000000000ll;
				timespec_wait.tv_nsec = nanoseconds % 1000000000ll;
				arg.ts = reinterpret_cast<uint64_t>(&timespec_wait);
				flags |= IORING_ENTER_EXT_ARG;
				parg = &arg;
				size_arg = sizeof(arg);
			}
		}

		// - a timeout (ETIME), an interrupt (EINTR) or a closed ring => nothing
		//   (is_waiting, set above : _wake_ring() picks a ring whose owner waits here)
		if (is_take_post == false || to_submit != 0 || is_taskrun)
			static_cast<void>(pring->enter(to_submit, min_complete, flags, parg, size_arg));

		if (is_wait)
			pring->is_waiting.store(false, std::memory_order_release);
	#endif

		head = __atomic_load_n(pring->cq_head, __ATOMIC_ACQUIRE);
		tail = __atomic_load_n(pring->cq_tail, __ATOMIC_ACQUIRE);
	}

	// 4) take the CQEs
	if (head != tail && count < _max)
		count = this->_drain_ring(pring, _results, count, _max);

	// 5) the posts this owner found instead of waiting (taken like a wake of its ring : the next ring woken when some are left)
	if (is_take_post && count < _max)
		count = this->_pop_posted(_results, count, _max, true);

	// 6) the rings without an owner
	if (count < _max)
		this->_drain_orphans(_results, count, _max);

	return count;
}

}

#endif
