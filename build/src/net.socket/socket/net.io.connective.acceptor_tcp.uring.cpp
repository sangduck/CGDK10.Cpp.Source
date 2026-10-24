#include "pch.h"

#if defined(_SYS_EPOLL_H)

namespace CGDK
{

// - the acceptor whose accept this thread processes (net.io.connective.acceptor_tcp.cpp)
extern thread_local const void* t_pacceptor_accepting;

//-----------------------------------------------------------------------------
// io_uring completion (executor::io_backend::io_uring_completion) : the accept with the multishot ACCEPT op
//
//  - the listen socket (the TCP one, the UDS one of enable_uds) posts one multishot accept op instead of the EPOLLIN
//    registration : every CQE carries an accepted socket (res = the fd, IORING_CQE_F_MORE) and goes through _process_accepted()
//    like the accept loop of the readiness path. the final CQE (no F_MORE : -ECANCELED at stop, the exit of the thread that
//    submitted it, an error of accept) ends the op : posted again while the acceptor listens.
//  - stop (process_close_socket) : the state is CLOSED, the accepts in process are waited for, the op is cancelled (cancel_fd)
//    and the listen socket is closed. an accepted socket delivered after (taken by the kernel before the cancel) is closed here.
//  - the executable holds the acceptor (object_ptr) : released when it goes back to its pool (the final CQE of the op and
//    the reset of m_pexecutable_accept(_uds) at stop)
//-----------------------------------------------------------------------------
class net::io::connective::acceptor::Ntcp::executable_accept_cq :
	public						Iexecutable,
	public						Npoolable<executable_accept_cq>,
	public						factory::_traits_system
{
public:
	virtual ~executable_accept_cq() noexcept {}

	DEFAULT_FACTORY_NAME(net::io::connective::acceptor::Ntcp::executable_accept_cq)

private:
	virtual	intptr_t			process_execute(intptr_t _result, std::size_t _param) override
	{
		if (auto pconnective = this->m_pconnective; pconnective.exist())
			pconnective->process_connective_accept_cq(_result, _param, this->m_handle_listen, this->m_address_family);

		return intptr_t(0);
	}

	virtual void				on_final_release() noexcept override { this->m_pconnective.reset(); }

public:
	object_ptr<net::io::connective::acceptor::Ntcp> m_pconnective;

	// - the listen socket (INVALID_SOCKET : the acceptor's TCP socket) and its address family (AF_UNSPEC : the acceptor's)
	CGSOCKET					m_handle_listen = INVALID_SOCKET;
	ADDRESS_FAMILY				m_address_family = AF_UNSPEC;
};

// - the CQE flag (linux/io_uring.h : IORING_CQE_F_MORE)
static constexpr std::size_t CGDK_ACCEPT_CQE_FLAG_MORE = 1u << 1;

bool net::io::connective::acceptor::Ntcp::_is_io_completion_accept() const noexcept
{
	auto pio_context = this->io_context();

	return pio_context.exist() && pio_context->backend() == executor::io_backend::io_uring_completion;
}

bool net::io::connective::acceptor::Ntcp::_process_start_accept_cq(CGSOCKET _handle_listen, ADDRESS_FAMILY _address_family)
{
	// 1) io_context
	auto pio_context = this->io_context();

	// check)
	RETURN_IF(pio_context.empty(), false);

	// 2) the executable (a new one per op, held by the op only : the submit takes a reference, released after the final CQE => back to its pool)
	auto pexecutable_accept = make_object<executable_accept_cq>();

	pexecutable_accept->m_pconnective = this;
	pexecutable_accept->m_handle_listen = (_address_family == AF_UNSPEC) ? INVALID_SOCKET : _handle_listen;
	pexecutable_accept->m_address_family = _address_family;

	// 3) the multishot accept (the accepted sockets are nonblocking : accept4 SOCK_NONBLOCK like the accept loop)
	const auto result = pio_context->submit_accept(static_cast<CGHANDLE>(_handle_listen), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC, true, pexecutable_accept.get());

	// check)
	if (result == false)
	{
		pexecutable_accept->m_pconnective.reset();
		return false;
	}

	// return)
	return true;
}

void net::io::connective::acceptor::Ntcp::process_connective_accept_cq(intptr_t _result, std::size_t _flags, CGSOCKET _handle_listen, ADDRESS_FAMILY _address_family)
{
	// - in process : process_close_socket() waits for it before it closes the listen sockets (like process_connective_accept)
	this->m_count_accepting.fetch_add(1, std::memory_order_seq_cst);

	struct ACCEPTING
	{
		Ntcp*			pthis;
		const void*		pbefore;
		~ACCEPTING() noexcept { t_pacceptor_accepting = pbefore; pthis->m_count_accepting.fetch_sub(1, std::memory_order_seq_cst); }
	} accepting{ this, t_pacceptor_accepting };

	t_pacceptor_accepting = this;

	// declare)
	const bool is_uds = (_address_family != AF_UNSPEC);
	const bool is_listening = (this->socket_state() != eSOCKET_STATE::CLOSED) && (is_uds == false || _handle_listen == this->m_handle_uds_listen.load(std::memory_order_acquire));
	const auto address_family = is_uds ? _address_family : this->m_address_family;

	// 1) an accepted socket
	if (_result >= 0)
	{
		const auto handle_socket_new = static_cast<CGSOCKET>(_result);

		// - counted like a socket of accept() (closesocket counts it down)
		api::socket::increase_socket_count();

		// statistics)
		this->statistics_on_try();

		// check) closed (stop) or disabled meanwhile : the socket is closed (the acceptor is not accepting)
		if (is_listening == false || this->is_disable())
		{
			api::socket::closesocket(handle_socket_new);
			this->statistics_on_fail_connect();
		}
		else
		{
			this->_process_accepted(handle_socket_new, address_family);
		}
	}
	// 2) an error of the accept (-ECANCELED : stop, or the thread that submitted the op exited)
	else if (_result != -ECANCELED)
	{
		this->m_last_socket_error = static_cast<int>(-_result);
		this->statistics_on_fail_connect();
	}

	// 3) the op ended (no more CQEs) : posted again while listening
	RETURN_IF((_flags & CGDK_ACCEPT_CQE_FLAG_MORE) != 0);
	RETURN_IF(is_listening == false || this->is_disable());

	const auto handle_listen = is_uds ? _handle_listen : this->native_handle();

	// check) the listen socket is closed (stop in process)
	RETURN_IF(handle_listen == INVALID_SOCKET);

	// - (EMFILE/ENFILE ... : the accept is posted again at once like the readiness event that comes again)
	const bool result = this->_process_start_accept_cq(handle_listen, _address_family);

	// check)
	if (result == false)
	{
		// log)
		LOG_ERROR << "@ (err ) ACCEPTOR['"sv << name() << "']: fail to post the accept again (error:"sv << static_cast<int>(-_result) << ")"sv;
	}
}

}

#endif
