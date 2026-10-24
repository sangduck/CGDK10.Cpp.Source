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
	#include <sys/socket.h>
#endif

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

   executor::Iio_context
 

*///-----------------------------------------------------------------------------
namespace executor
{
	// - the event backend of an io_context (linux : epoll, io_uring (readiness) or io_uring_completion (a ring per worker thread,
	//   the op API below), see default_io_backend() below. windows : iocp, reported as epoll)
	enum class io_backend { epoll = 0, io_uring = 1, io_uring_completion = 2 };
}

class executor::Iio_context :
	virtual public				executor::Iqueue
{
public:
	virtual	~Iio_context() noexcept	{}

public:
#if defined(_WIN32)
	virtual	bool				attach(CGHANDLE _handle) PURE;
	virtual	bool				detach(CGHANDLE _handle) noexcept PURE;
#elif defined(__ANDROID__) || defined(__linux__)
	virtual	bool				attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _option = 0) PURE;
	virtual	bool				detach(CGHANDLE _handle) noexcept PURE;
	virtual	bool				set_event(CGHANDLE _handle, uint32_t _flag_event, Iexecutable* _pexecutable = nullptr) noexcept PURE;
#endif
	[[nodiscard]] virtual CGHANDLE handle() const noexcept PURE;
	virtual void				execute_all() PURE;

	//! @brief the backend of this io_context (linux : epoll / io_uring. the sockets ask it : MSG_ZEROCOPY is used on io_uring only)
	[[nodiscard]] virtual io_backend backend() const noexcept { return io_backend::epoll; }

	//! @brief the readiness registrations of this io_context (linux : attach/detach. a completion socket has none : its I/O is the ops)
	[[nodiscard]] virtual size_t attached_count() const noexcept { return 0; }

	//! @brief busy poll of epoll_wait() itself (linux epoll, kernel 6.9+ EPIOCSPARAMS. see executor::set_default_busy_poll() below)
	//!        _usecs : spin on the NIC queue this long before sleeping (0 : off), _budget : packets per poll (0 : kernel default),
	//!        _prefer : prefer busy poll over the NIC interrupt. false : not supported (windows, io_uring, a kernel before 6.9) or EPERM
	virtual	bool				set_busy_poll(uint32_t _usecs, uint16_t _budget = 0, bool _prefer = false) noexcept { static_cast<void>(_usecs); static_cast<void>(_budget); static_cast<void>(_prefer); return false; }
	virtual	bool				get_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) const noexcept { _usecs = 0; _budget = 0; _prefer = false; return false; }

#if defined(__ANDROID__) || defined(__linux__)
	//! @brief the op API of the io_uring_completion backend (false on the others). the completion of an op is delivered as
	//!        _pexecutable->process_execute(res, flags) : res = the bytes (>= 0) or -errno (-ECANCELED : cancelled), flags = the CQE flags
	//!        (IORING_CQE_F_MORE : more CQEs follow (multishot accept, the first CQE of a zero copy send), IORING_CQE_F_NOTIF : the
	//!        zero copy notification (the buffer may be reused)). the submit takes one add_ref() of _pexecutable, released by the
	//!        executor after the delivery of the final CQE. the buffers (msghdr, iov, sockaddr) must live till the completion
	virtual	bool				submit_recv(CGHANDLE _fd, void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable) noexcept { static_cast<void>(_fd); static_cast<void>(_buf); static_cast<void>(_len); static_cast<void>(_msg_flags); static_cast<void>(_pexecutable); return false; }
	virtual	bool				submit_recvmsg(CGHANDLE _fd, msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable) noexcept { static_cast<void>(_fd); static_cast<void>(_msg); static_cast<void>(_msg_flags); static_cast<void>(_pexecutable); return false; }
	virtual	bool				submit_send(CGHANDLE _fd, const void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept { static_cast<void>(_fd); static_cast<void>(_buf); static_cast<void>(_len); static_cast<void>(_msg_flags); static_cast<void>(_pexecutable); static_cast<void>(_zerocopy); return false; }
	virtual	bool				submit_sendmsg(CGHANDLE _fd, const msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept { static_cast<void>(_fd); static_cast<void>(_msg); static_cast<void>(_msg_flags); static_cast<void>(_pexecutable); static_cast<void>(_zerocopy); return false; }
	virtual	bool				submit_accept(CGHANDLE _listen_fd, sockaddr* _addr, socklen_t* _addrlen, int _flags, bool _multishot, Iexecutable* _pexecutable) noexcept { static_cast<void>(_listen_fd); static_cast<void>(_addr); static_cast<void>(_addrlen); static_cast<void>(_flags); static_cast<void>(_multishot); static_cast<void>(_pexecutable); return false; }
	virtual	bool				submit_connect(CGHANDLE _fd, const sockaddr* _addr, socklen_t _addrlen, Iexecutable* _pexecutable) noexcept { static_cast<void>(_fd); static_cast<void>(_addr); static_cast<void>(_addrlen); static_cast<void>(_pexecutable); return false; }
	//! @brief a multishot RECV into the provided buffers of the io_context (one submit, a completion per arrival). a completion with
	//!        IORING_CQE_F_BUFFER in its flags (_param) has its data in get_provided_buffer(_param) : give it back with return_buffer(_param)
	//!        (once, any thread). IORING_CQE_F_MORE : more follow. without it the recv ended (-ENOBUFS : the buffers ran out, -ECANCELED,
	//!        0 : the peer closed, an error, or data) : submit again. false : not supported (submit_recv() then)
	virtual	bool				submit_recv_multishot(CGHANDLE _fd, Iexecutable* _pexecutable) noexcept { static_cast<void>(_fd); static_cast<void>(_pexecutable); return false; }
	[[nodiscard]] virtual const void* get_provided_buffer(size_t _param) const noexcept { static_cast<void>(_param); return nullptr; }
	virtual	bool				return_buffer(size_t _param) noexcept { static_cast<void>(_param); return false; }
	//! @brief the size of a provided buffer of submit_recv_multishot() (0 : no multishot recv). the stream socket leaves the multishot for a
	//!        RECV into its own growing buffer when the arrivals outgrow it (net.io.receivable.stream.uring.cpp)
	[[nodiscard]] virtual size_t provided_buffer_size() const noexcept { return 0; }
	//! @brief this thread owns the ring the fd is bound to (io_uring completion : an op submitted by it goes with its next enter, no wake)
	[[nodiscard]] virtual bool	is_ring_owner(CGHANDLE _fd) const noexcept { static_cast<void>(_fd); return false; }
	//! @brief cancels every op of the executable / of the fd (IORING_OP_ASYNC_CANCEL : they complete with -ECANCELED and are delivered)
	virtual	bool				submit_cancel(Iexecutable* _pexecutable) noexcept { static_cast<void>(_pexecutable); return false; }
	virtual	bool				submit_cancel_fd(CGHANDLE _fd) noexcept { static_cast<void>(_fd); return false; }
	//! @brief cancels every op of the fd and waits till the kernel is done with them (the fd may be closed after : its number is reused at once)
	virtual	bool				wait_cancelled_fd(CGHANDLE _fd, chrono::tick::duration _ticks_wait, bool _is_wait_delivery = true) noexcept { static_cast<void>(_fd); static_cast<void>(_ticks_wait); static_cast<void>(_is_wait_delivery); return false; }
#endif
};


//-----------------------------------------------------------------------------
/**

   executor::Iio_context_threads

   an io_context executor with threads of its own (an iocp/epoll and threads apart
   from the system_executor). created by executor::make_io_context_threads().
   (the executor of the system_executor is one of these too)

   - stop() (Istartable) : stops and joins the threads.
   - request_stop()      : stops without waiting. the io_context is disposed and
                           the threads end by themselves. (stop() joins them later)

*///-----------------------------------------------------------------------------
class executor::Iio_context_threads :
	virtual public				executor::Iio_context
{
public:
	virtual	~Iio_context_threads() noexcept {}

public:
	virtual	void				request_stop() noexcept PURE;
	[[nodiscard]] virtual std::size_t existing_thread_count() const noexcept PURE;
};

namespace executor
{
	//-------------------------------------------------------------------------
	// the event backend of the io_context executors (linux) : epoll (default), io_uring (readiness) or io_uring_completion.
	//  - chosen when an io_context is created : set it before the executors start (the system_executor is created at its first use).
	//    env CGDK_IO_URING=1 (true/on/uring) selects io_uring, CGDK_IO_URING=completion (cq/2) io_uring_completion.
	//  - io_uring that is not available (a kernel before 5.13, io_uring disabled) falls back to epoll (logged).
	//    io_uring_completion needs 6.0+ (SEND_ZC, multishot accept, sync cancel) : falls back to epoll (logged).
	//  - windows : iocp (the option is ignored)
	//-------------------------------------------------------------------------
	void						set_default_io_backend(io_backend _backend) noexcept;
	[[nodiscard]] io_backend	default_io_backend() noexcept;
	[[nodiscard]] bool			is_io_uring_available() noexcept;
	[[nodiscard]] bool			is_io_uring_completion_available() noexcept;

	//-------------------------------------------------------------------------
	// busy poll of epoll_wait() itself (linux epoll, kernel 6.9+ : EPIOCSPARAMS per epoll instance. no sysctl net.core.busy_poll needed)
	//  - epoll_wait() spins on the NIC queue of the sockets for 'usecs' before it sleeps : the packets arriving in that time are
	//    handled without an interrupt and a wake up (latency), at the cost of a busy CPU. the sockets of the epoll must share
	//    a NAPI ID (one NIC queue) : for sockets without one (loopback, UDS) the kernel does nothing.
	//    with SO_INCOMING_CPU per CPU io_contexts (acceptor "cpu_affinity") the sockets of a queue land on one epoll.
	//  - the default is applied to every epoll io_context created afterwards (the system_executor is created at its first use :
	//    set it before). an io_context of its own : io_context_threads::START_PARAMETER::busy_poll_usecs / context "busy_poll"
	//    ("busy_poll_budget", "prefer_busy_poll"), or Iio_context::set_busy_poll() after the start.
	//  - budget : packets per poll (0 : the kernel default 8. over 64 needs CAP_NET_ADMIN), prefer : prefer busy poll over the NIC interrupt.
	//  - a kernel before 6.9 has no EPIOCSPARAMS : sysctl net.core.busy_poll (global) applies instead. windows, io_uring : ignored (warned)
	//-------------------------------------------------------------------------
	void						set_default_busy_poll(uint32_t _usecs, uint16_t _budget = 0, bool _prefer = false) noexcept;
	void						get_default_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) noexcept;
	[[nodiscard]] bool			is_busy_poll_available() noexcept;

	// - creates and starts an io_context executor with '_thread_count' threads of its own.
	//   (throws when it can not be created or started)
	[[nodiscard]] object_ptr<Iio_context_threads> make_io_context_threads(std::string_view _name, int _thread_count = 1);
	// - the threads pinned to a CPU (_cpu_affinity : -1 not pinned). ex. an io_context per CPU (acceptor::Ntcp::cpu_affinity_io_context)
	//   _busy_poll_usecs : the busy poll of epoll_wait() of this io_context (0 : the default. set_default_busy_poll())
	[[nodiscard]] object_ptr<Iio_context_threads> make_io_context_threads(std::string_view _name, int _thread_count, int _cpu_affinity, uint32_t _busy_poll_usecs = 0);
	// - by a context : "thread_count", "cpu_affinity", "busy_poll" (microseconds), "busy_poll_budget", "prefer_busy_poll"
	[[nodiscard]] object_ptr<Iio_context_threads> make_io_context_threads(std::string_view _name, context& _context);
}


}