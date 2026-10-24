#include "pch.h"
#include <cstdlib>

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

//-----------------------------------------------------------------------------
// io_uring completion (executor::io_backend::io_uring_completion) : the receive of the stream with a multishot RECV
//
//  - one receive op in flight per socket, posted by receive() under the receiving lock (the handle is read and the op submitted
//    under it : process_close_socket() swaps the handle under the same lock and cancels the ops after). the executable holds the
//    socket (add_ref, m_count_io_inflight) from the submit till the final completion of the op (the one without IORING_CQE_F_MORE).
//  - the op is a multishot RECV into the provided buffers of the io_context (submit_recv_multishot : one submit, a completion per
//    arrival). the data of a completion is copied after the data not processed yet (m_buffer_received : grown when it does not
//    fit, the same sizing as the epoll receive after the processing, _process_prepare_receive_buffer) and the buffer is given back
//    at once (return_buffer, to the io_context the op was submitted to : the socket releases its io_context at the disconnect).
//    the multishot not available (CGDK_IO_URING_MULTISHOT=0, no provided buffer ring) => a RECV per arrival into the free space of
//    m_buffer_received (the receive of the phase 2).
//  - the completion (process_complete_receivable_cq) : the bytes are processed (process_packet under a send cork). F_MORE : the op
//    goes on. no F_MORE : the op ended => data : submitted again. -ENOBUFS (the provided buffers of the ring ran out) : one RECV into
//    m_buffer_received, the multishot again after it. 0 bytes (the peer closed) or an error (-ECANCELED : closed by
//    process_close_socket, -ECONNRESET ...) : the disconnect. the disconnect is completed once (socket_state CAS in
//    process_complete_disconnect) by the final completion, or by process_close_socket() when no op is in flight (m_is_receiving_op
//    under the receiving lock). a socket closed while the multishot goes on : its final completion (-ECANCELED : the close cancels
//    the ops of the fd, or the data / 0 the kernel completed before) completes the disconnect.
//  - the completions of an op run in order, one at a time (the backend runs the CQEs of a ring on one thread at a time)
//  - zero copy (no data waiting in m_buffer_received, CGDK_IO_URING_ZC_RECV not 0) : the provided buffer is processed in place, like
//    IOCP processes the buffer it received into. it is wrapped in a pooled Imemory (provided_memory) that gives it back to the ring
//    when its last reference goes : the messages of process_packet are slices of it, so a message the application keeps holds the
//    buffer till it is released. only the tail of a message cut by the end of the buffer is copied into m_buffer_received (the next
//    arrival is in another buffer). a buffer held long leaves the ring short of buffers : -ENOBUFS ends the multishot and one RECV
//    into m_buffer_received bridges it (as before).
//  - the receive mode (_process_select_receive_mode, IOCP like) : the multishot while the arrivals fit in a provided buffer (small
//    messages : one submit, no buffer per socket). an arrival that filled a provided buffer, or a message larger than one waiting,
//    => the RECV mode : a RECV into m_buffer_received, whose size follows the IOCP rule (_process_prepare_receive_buffer : x8 when a
//    RECV filled its window - 1KB, 8KB, 64KB - the message size when larger, smaller after many small receives). the multishot going
//    on is cancelled, its final completion (-ECANCELED, the handle open) posts the RECV. COUNT_SMALL_RECEIVES_TO_MULTISHOT small
//    RECVs in a row => the multishot again. CGDK_IO_URING_RECV_ADAPTIVE=0 : always the multishot.
//-----------------------------------------------------------------------------
namespace CGDK
{
	// - a provided buffer as the source of a shared_buffer : given back to its ring at the final release
	class net_io_uring_provided_memory :
		public						Imemory,
		public						Npoolable<net_io_uring_provided_memory>,
		public						factory::_traits_system
	{
	public:
		virtual ~net_io_uring_provided_memory() noexcept {}
		DEFAULT_FACTORY_NAME(net_io_uring_provided_memory)

	private:
		virtual void				on_final_release() noexcept override
		{
			if (this->m_pio_context.exist() && this->m_param != 0)
				this->m_pio_context->return_buffer(this->m_param);

			this->m_pio_context.reset();
			this->m_param = 0;
			this->set_data(nullptr);
			this->set_size(0);
		}

	public:
		void						place(char* _pdata, std::size_t _size, executor::Iio_context* _pio_context, std::size_t _param) noexcept
		{
			this->set_data(_pdata);
			this->set_size(_size);
			this->m_pio_context = _pio_context;
			this->m_param = _param;
		}

		object_ptr<executor::Iio_context> m_pio_context;
		std::size_t					m_param = 0;
	};
}

namespace
{
	// - the zero copy receive (CGDK_IO_URING_ZC_RECV=0 : off, the data is copied into m_buffer_received)
	std::atomic<int> s_zero_copy_receive{ -1 };

	bool _is_zero_copy_receive() noexcept
	{
		int value = s_zero_copy_receive.load(std::memory_order_relaxed);

		if (value < 0)
		{
			const char* penv = std::getenv("CGDK_IO_URING_ZC_RECV");
			value = (penv != nullptr && (penv[0] == '0' || penv[0] == 'f' || penv[0] == 'F' || penv[0] == 'n' || penv[0] == 'N')) ? 0 : 1;
			s_zero_copy_receive.store(value, std::memory_order_relaxed);
		}

		return value != 0;
	}

	// - the switch of the receive mode (CGDK_IO_URING_RECV_ADAPTIVE=0 : off, always the multishot)
	std::atomic<int> s_receive_adaptive{ -1 };

	bool _is_receive_adaptive() noexcept
	{
		int value = s_receive_adaptive.load(std::memory_order_relaxed);

		if (value < 0)
		{
			const char* penv = std::getenv("CGDK_IO_URING_RECV_ADAPTIVE");
			value = (penv != nullptr && (penv[0] == '0' || penv[0] == 'f' || penv[0] == 'F' || penv[0] == 'n' || penv[0] == 'N')) ? 0 : 1;
			s_receive_adaptive.store(value, std::memory_order_relaxed);
		}

		return value != 0;
	}

	constexpr std::size_t	CQE_F_BUFFER = 1u << 0;		// - IORING_CQE_F_BUFFER : the data is in a provided buffer
	constexpr std::size_t	CQE_F_MORE = 1u << 1;		// - IORING_CQE_F_MORE : the op goes on

	// - room for _bytes after the data in _buffer (the data not processed yet is kept : moved to the front when the buffer is this
	//   socket's only and large enough, else copied into a larger one). an exception : no memory (the caller closes the socket)
	void _reserve_receive_buffer(shared_buffer& _buffer, std::size_t _bytes, std::size_t _bytes_minimum)
	{
		// - not allocated yet
		if (_buffer.get_upper_bound() == nullptr)
		{
			_buffer = alloc_shared_buffer(__select_max(_bytes, _bytes_minimum));
			return;
		}

		// check) room enough
		RETURN_IF(_buffer.remained_size() >= _bytes);

		const auto bytes_stored = _buffer.size();
		const auto plower = static_cast<char*>(const_cast<void*>(_buffer.get_lower_bound()));
		const auto bytes_capacity = static_cast<std::size_t>(static_cast<const char*>(_buffer.get_upper_bound()) - plower);

		// - this socket's only and large enough : the data moved to the front
		if (_buffer.is_source_unique() && (bytes_capacity - bytes_stored) >= _bytes)
		{
			::memmove(plower, _buffer.data(), bytes_stored);
			_buffer.set_data(plower);
			return;
		}

		// - a larger buffer (x2 till it fits) with the data
		std::size_t size_new = __select_max(bytes_capacity, _bytes_minimum);

		while (size_new < bytes_stored + _bytes)
			size_new *= 2;

		auto buf_new = alloc_shared_buffer(size_new);

		if (bytes_stored != 0)
			::memcpy(buf_new.data(), _buffer.data(), bytes_stored);

		buf_new ^= bytes_stored;
		_buffer = std::move(buf_new);
	}
}

intptr_t net::io::receivable::Nstream::executable_receiving::process_execute(intptr_t _result, std::size_t _param)
{
	// 1) the stream (held by receive() : released after the final completion)
	auto* pstream = this->m_preceivable_stream;

	// check)
	if (pstream == nullptr)
	{
		// - the provided buffer goes back anyway
		if ((_param & CQE_F_BUFFER) != 0 && this->m_pio_context != nullptr)
			this->m_pio_context->return_buffer(_param);

		return 0;
	}

	// 2) complete
	pstream->process_complete_receivable_cq(_result, _param);

	// 3) the final completion (no F_MORE) : the op is done (the reference of receive()). F_MORE : the multishot goes on
	if ((_param & CQE_F_MORE) == 0)
	{
		// statistics)
		pstream->statistics_on_decrease_async_receiveing();

		pstream->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
		pstream->release();
	}

	// return)
	return 0;
}

bool net::io::receivable::Nstream::process_start_receive() noexcept
{
	// 1) the executable (pooled : allocated at the first use of this socket)
	if (this->m_pexecutable_receiving.empty())
	{
		try
		{
			this->m_pexecutable_receiving = make_object<executable_receiving>();
		}
		catch (...)
		{
			return false;
		}
	}

	this->m_pexecutable_receiving->m_preceivable_stream = this;

	// - the receive mode of a new connection (a pooled socket keeps the one of its last connection)
	this->m_receive_mode = RECEIVE_MODE_MULTISHOT;
	this->m_is_receive_mode_switching = false;
	this->m_count_receive_small_recv = 0;

	// 2) the first receive
	return this->receive();
}

bool net::io::receivable::Nstream::receive(bool _is_multishot)
{
	//-----------------------------------------------------------------
	// posts the receive op (under the receiving lock : the handle is read and the op is submitted under it)
	//
	//  - the multishot RECV into the provided buffers. the RECV mode (_process_select_receive_mode), not available or _is_multishot
	//    false (after -ENOBUFS) : a RECV into the free space after the data not processed yet
	//  - false : the handle is closed (INVALID_SOCKET : process_close_socket() swapped it) or the submit failed.
	//    m_is_receiving_op is false then : the caller completes the disconnect
	//-----------------------------------------------------------------
	// check)
	RETURN_IF(this->m_pexecutable_receiving.empty(), false);

	// 1) the buffer (allocated at the first receive, prepared again when it is full : the free space of a RECV)
	try
	{
		if (this->m_buffer_received.get_upper_bound() == nullptr)
			this->m_buffer_received = alloc_shared_buffer(this->m_minimum_message_buffer_size);
		else if (this->m_buffer_received.remained_size() == 0)
			this->_process_prepare_receive_buffer(0, false, 0);
	}
	catch (...)
	{
		std::lock_guard lock(this->m_lockable_receiving);
		this->m_is_receiving_op = false;
		return false;
	}

	// 2) post (under the receiving lock)
	std::lock_guard lock(this->m_lockable_receiving);

	const auto handle_socket = this->native_handle();

	// check) closed meanwhile
	if (handle_socket == INVALID_SOCKET)
	{
		this->m_is_receiving_op = false;
		return false;
	}

	auto* pio_context = this->m_pio_context.get();

	// check)
	if (pio_context == nullptr)
	{
		this->m_is_receiving_op = false;
		return false;
	}

	// - the socket is held by the op (released by its final completion). the provided buffers go back to this io_context
	this->add_ref();
	this->m_count_io_inflight.fetch_add(1, std::memory_order_acq_rel);
	this->m_pexecutable_receiving->m_pio_context = pio_context;

	// statistics)
	this->statistics_on_increase_async_receiveing();

	// - the multishot RECV, else a RECV into the free space
	bool is_submitted = _is_multishot && this->m_receive_mode == RECEIVE_MODE_MULTISHOT && pio_context->submit_recv_multishot(static_cast<CGHANDLE>(handle_socket), this->m_pexecutable_receiving.get());

	if (is_submitted == false)
	{
		const auto pbuf_receive = this->m_buffer_received.data() + this->m_buffer_received.size();
		const auto bytes_window = this->m_buffer_received.remained_size();

		// check)
		CGASSERT_ERROR(bytes_window != 0);

		is_submitted = pio_context->submit_recv(static_cast<CGHANDLE>(handle_socket), pbuf_receive, bytes_window, 0, this->m_pexecutable_receiving.get());
	}

	if (is_submitted == false)
	{
		// - rollback
		this->m_last_socket_error = api::get_last_error();
		this->statistics_on_error_receive();
		this->statistics_on_decrease_async_receiveing();
		this->m_count_io_inflight.fetch_sub(1, std::memory_order_acq_rel);
		this->release();
		this->m_is_receiving_op = false;
		return false;
	}

	// statistics)
	this->statistics_on_receive_async();

	// - in flight (a cancel for the RECV mode is done : the op it ended was replaced)
	this->m_is_receiving_op = true;
	this->m_is_receive_mode_switching = false;

	// return)
	return true;
}

void net::io::receivable::Nstream::process_complete_receivable_cq(intptr_t _result, std::size_t _flags)
{
	// check)
	CGASSERT_FATAL(this->socket_state() != eSOCKET_STATE::SYN);

	// declare)
	const bool is_more = (_flags & CQE_F_MORE) != 0;
	const bool is_provided = (_flags & CQE_F_BUFFER) != 0;

	// 0) zero copy : data in a provided buffer and nothing waiting in m_buffer_received => processed in place (_process_receive_in_place)
	if (is_provided && _result > 0 && this->m_buffer_received.size() == 0 && _is_zero_copy_receive())
	{
		auto* pio_context = this->m_pexecutable_receiving.exist() ? this->m_pexecutable_receiving->m_pio_context : nullptr;
		const void* pdata = (pio_context != nullptr) ? pio_context->get_provided_buffer(_flags) : nullptr;

		if (pdata != nullptr)
		{
			this->_process_receive_in_place(pio_context, pdata, static_cast<std::size_t>(_result), _flags, is_more);
			return;
		}
	}

	// 1) the data in a provided buffer : copied after the data not processed yet, the buffer given back at once (whatever comes after.
	//    no memory : the socket is closed below)
	bool is_copy_failed = false;

	if (is_provided)
	{
		auto* pio_context = this->m_pexecutable_receiving.exist() ? this->m_pexecutable_receiving->m_pio_context : nullptr;

		if (_result > 0)
		{
			const void* pdata = (pio_context != nullptr) ? pio_context->get_provided_buffer(_flags) : nullptr;

			try
			{
				if (pdata == nullptr)
					throw std::bad_alloc();

				_reserve_receive_buffer(this->m_buffer_received, static_cast<std::size_t>(_result), this->m_minimum_message_buffer_size);
				::memcpy(this->m_buffer_received.data() + this->m_buffer_received.size(), pdata, static_cast<std::size_t>(_result));
			}
			catch (...)
			{
				is_copy_failed = true;
			}
		}

		if (pio_context != nullptr)
			pio_context->return_buffer(_flags);
	}

	// 2) 0 bytes (the peer closed) or an error (-ECANCELED : closed by process_close_socket) => the disconnect
	if (_result <= 0)
	{
		// check) the op goes on (a multishot ends with its error : not expected)
		RETURN_IF(is_more);

		// - -ENOBUFS : the provided buffers of the ring ran out (the multishot ended, the data waits in the socket) => one RECV into
		//   m_buffer_received, the multishot again after it
		if (_result == -ENOBUFS && this->native_handle() != INVALID_SOCKET && this->receive(false))
			return;

		// - -ECANCELED with the handle still open : the thread that submitted the op exited (the kernel cancels its requests) => posted again
		if (_result == -ECANCELED && this->native_handle() != INVALID_SOCKET && this->receive())
			return;

		{
			std::lock_guard lock(this->m_lockable_receiving);
			this->m_is_receiving_op = false;
		}

		if (_result < 0 && _result != -ECANCELED)
		{
			this->m_last_socket_error = static_cast<int>(-_result);
			this->m_disconnect_reason |= DISCONNECT_REASON_FAIL;
			this->statistics_on_error_receive();
		}

		// - close (nothing when process_close_socket() closed it already) and complete the disconnect (once : socket_state CAS)
		this->process_close_socket();
		this->process_complete_disconnect();
		this->process_clear_buffer();
		return;
	}

	// declare)
	bool is_closed = false;

	try
	{
		// check) the data of the provided buffer was not copied (no memory)
		if (is_copy_failed)
			throw std::bad_alloc();

		// declare)
		net::sockaddr address_peer;
		const auto bytes_transfered = static_cast<std::size_t>(_result);
		const auto pbuf_receive = this->m_buffer_received.data() + this->m_buffer_received.size();
		const auto handle_socket = this->native_handle();

		// - a RECV that filled the free space it was given => the buffer grows (a provided buffer : copied, m_buffer_received is not the window)
		const bool is_window_filled = (is_provided == false && bytes_transfered >= this->m_buffer_received.remained_size());

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// - TCP_QUICKACK is not permanent (the kernel returns to the delayed ACK) : set it again after every recv (socket_option().tcp_quickack)
		if (this->socket_option().tcp_quickack && handle_socket != INVALID_SOCKET)
			api::socket::set_tcp_quickack(handle_socket, true);

		// 3) add size
		this->m_buffer_received.add_size(bytes_transfered);

		// debug)
	#if !defined(NDEBUG) || defined(_DEBUG)
		if(this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(handle_socket, bytes_transfered, this->m_buffer_received.size());
	#endif

		// 4) call process_on_receive
		this->process_on_receive(buffer_view{ pbuf_receive, bytes_transfered }, address_peer);

		// 5) process message (send cork : the sends of this thread to this socket while processing are sent at once at the end)
		std::size_t required_bytes;
		{
			net::io::send_cork cork(this);

			required_bytes = this->process_packet(this->m_buffer_received, address_peer);
		}

		// check)
		THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string());

		// 6) the free space for the next data (the window filled => the buffer grows)
		this->_process_prepare_receive_buffer(bytes_transfered, is_window_filled, required_bytes);

		// 7) the receive mode (the arrivals outgrew a provided buffer => RECV, small RECVs in a row => the multishot)
		this->_process_select_receive_mode(bytes_transfered, required_bytes, is_provided, is_more);
	}
	catch (throwable::failure& _e)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (throwable) " << __CGFUNCTION__;
		LOG_DEBUG << "       - reason: " << CGDK::stream_hex(_e.reason<uint64_t>(), 0, false, true) << " [" << to_string(_e.reason<eRESULT>()) << "]";
		if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics)
		this->statistics_set_error_disconnect();

		// - disconnect
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
		is_closed = true;
	}
	catch (std::exception& _e)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (std::exception) " << __CGFUNCTION__;
		LOG_DEBUG << "       - " << _e.what();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics)
		this->statistics_set_error_disconnect();

		// - disconnect
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
		is_closed = true;
	}
	catch (...)
	{
		// trace)
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__;

		// declare)
		throwable::failure	e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics)
		this->statistics_set_error_disconnect();

		// - disconnect
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
		is_closed = true;
	}

	// 7) the multishot goes on (closed meanwhile, by this thread above or by another : its final completion completes the disconnect.
	//    process_close_socket() left it to the op in flight : m_is_receiving_op)
	RETURN_IF(is_more);

	// 8) the op ended (a RECV, a multishot that ended with data) : the next one (closed meanwhile : receive() fails on the closed
	//    handle and this completion completes the disconnect)
	if (is_closed == false && this->receive())
		return;

	// - closed by this thread (an exception above) or by another thread (process_close_socket : the receive in flight was this op) :
	//   the disconnect is completed here. a submit failure : closed now (FAIL)
	{
		std::lock_guard lock(this->m_lockable_receiving);
		this->m_is_receiving_op = false;
	}

	this->process_close_socket(DISCONNECT_REASON_FAIL);
	this->process_complete_disconnect();
	this->process_clear_buffer();
}

void net::io::receivable::Nstream::_process_receive_in_place(executor::Iio_context* _pio_context, const void* _pdata, std::size_t _bytes, std::size_t _flags, bool _is_more)
{
	//-----------------------------------------------------------------
	// the data of a provided buffer processed in place (m_buffer_received holds no data : the buffer starts with a message)
	//
	//  - the buffer is the source of a shared_buffer (net_io_uring_provided_memory : given back to the ring at its final release) :
	//    the messages are slices of it. the tail of a message cut by the end of the buffer is copied into m_buffer_received.
	//  - the rest is the same as the copy path : the exception closes the socket, the end of the op posts the next one
	//-----------------------------------------------------------------
	// declare)
	bool is_closed = false;

	// 1) the buffer as a shared_buffer (the reference of this function : given back when the messages kept by the application go too)
	shared_buffer buffer_place;

	try
	{
		auto pmemory = make_object<net_io_uring_provided_memory>();
		pmemory->place(static_cast<char*>(const_cast<void*>(_pdata)), _bytes, _pio_context, _flags);

		buffer_place = object_ptr<Imemory>(pmemory.get());
		buffer_place.add_size(_bytes);
	}
	catch (...)
	{
		// - no memory for the wrapper : the buffer goes back, the socket is closed (like the copy failure)
		if (_pio_context != nullptr)
			_pio_context->return_buffer(_flags);

		this->statistics_set_error_disconnect();
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
		is_closed = true;
	}

	if (is_closed == false)
	{
		try
		{
			// declare)
			net::sockaddr address_peer;
			const auto handle_socket = this->native_handle();

			// statistics)
			this->statistics_on_receive_bytes(_bytes);

			// - TCP_QUICKACK is not permanent : set it again after every recv (socket_option().tcp_quickack)
			if (this->socket_option().tcp_quickack && handle_socket != INVALID_SOCKET)
				api::socket::set_tcp_quickack(handle_socket, true);

			// debug)
		#if !defined(NDEBUG) || defined(_DEBUG)
			if(this->is_socket_debug_logging_enabled())
				net::io::_debug_log_socket_received(handle_socket, _bytes, _bytes);
		#endif

			// 2) call process_on_receive
			this->process_on_receive(buffer_view{ static_cast<char*>(const_cast<void*>(_pdata)), _bytes }, address_peer);

			// 3) process message (send cork : the sends of this thread to this socket while processing are sent at once at the end)
			std::size_t required_bytes;
			{
				net::io::send_cork cork(this);

				required_bytes = this->process_packet(buffer_place, address_peer);
			}

			// check)
			THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string());

			// 4) the tail of a message cut by the end of the buffer : copied into m_buffer_received (room for the whole message)
			if (const auto bytes_tail = buffer_place.size(); bytes_tail != 0 && this->is_invalid_native_handle() == false)
			{
				_reserve_receive_buffer(this->m_buffer_received, __select_max(bytes_tail, required_bytes), this->m_minimum_message_buffer_size);
				::memcpy(this->m_buffer_received.data() + this->m_buffer_received.size(), buffer_place.data(), bytes_tail);
				this->m_buffer_received.add_size(bytes_tail);
			}

			// 5) the receive mode (the arrivals outgrew a provided buffer => RECV)
			this->_process_select_receive_mode(_bytes, required_bytes, true, _is_more);
		}
		catch (throwable::failure& _e)
		{
			LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (throwable) " << __CGFUNCTION__;
			LOG_DEBUG << "       - reason: " << CGDK::stream_hex(_e.reason<uint64_t>(), 0, false, true) << " [" << to_string(_e.reason<eRESULT>()) << "]";
			if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();
			throwable::execute_exception_handler(this, _e, __FUNCTION__);
			this->statistics_set_error_disconnect();
			this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			is_closed = true;
		}
		catch (std::exception& _e)
		{
			LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (std::exception) " << __CGFUNCTION__;
			LOG_DEBUG << "       - " << _e.what();
			throwable::execute_exception_handler(this, _e, __FUNCTION__);
			this->statistics_set_error_disconnect();
			this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			is_closed = true;
		}
		catch (...)
		{
			LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__;
			throwable::failure	e(eRESULT::EXCEPTION);
			throwable::execute_exception_handler(this, e, __FUNCTION__);
			this->statistics_set_error_disconnect();
			this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
			is_closed = true;
		}
	}

	// 5) this function's reference of the buffer (given back here unless a message the application keeps holds it)
	buffer_place.clear();

	// 6) the multishot goes on (closed meanwhile : its final completion completes the disconnect)
	RETURN_IF(_is_more);

	// 7) the op ended : the next one (closed meanwhile : receive() fails and the disconnect is completed here)
	if (is_closed == false && this->receive())
		return;

	{
		std::lock_guard lock(this->m_lockable_receiving);
		this->m_is_receiving_op = false;
	}

	this->process_close_socket(DISCONNECT_REASON_FAIL);
	this->process_complete_disconnect();
	this->process_clear_buffer();
}

void net::io::receivable::Nstream::_process_select_receive_mode(std::size_t _bytes, std::size_t _required_bytes, bool _is_provided, bool _is_more)
{
	//-----------------------------------------------------------------
	// the receive mode after a completion was processed (IOCP like : the RECV buffer grows with the traffic)
	//
	//  - multishot : an arrival that filled a provided buffer (more is waiting : IOCP grows a filled buffer) or a message larger than
	//    one waiting => the RECV mode. the next receive is a RECV into m_buffer_received, sized by _process_prepare_receive_buffer
	//    (the IOCP rule : x8 when a RECV filled its window, the message size when larger). the multishot going on is cancelled once :
	//    its final completion (-ECANCELED with the handle open, or the data the kernel completed before) posts the RECV.
	//  - RECV : COUNT_SMALL_RECEIVES_TO_MULTISHOT RECVs in a row smaller than half a provided buffer => the multishot again. the
	//    completions of the multishot being cancelled do not count.
	//  - no switch : CGDK_IO_URING_RECV_ADAPTIVE=0, no provided buffers (provided_buffer_size() 0)
	//-----------------------------------------------------------------
	auto* pio_context = this->m_pexecutable_receiving.exist() ? this->m_pexecutable_receiving->m_pio_context : nullptr;
	const std::size_t size_provided = (pio_context != nullptr && _is_receive_adaptive()) ? pio_context->provided_buffer_size() : 0;

	RETURN_IF(size_provided == 0);

	const bool is_large = (_bytes >= size_provided) || (_required_bytes > size_provided);

	// 1) multishot => RECV
	if (this->m_receive_mode == RECEIVE_MODE_MULTISHOT)
	{
		RETURN_IF(is_large == false);

		this->m_receive_mode = RECEIVE_MODE_RECV;
		this->m_count_receive_small_recv = 0;

		// - the multishot going on : cancelled (once. the op ended already : the next receive() posts the RECV)
		if (_is_provided && _is_more && this->m_is_receive_mode_switching == false)
		{
			this->m_is_receive_mode_switching = true;
			pio_context->submit_cancel(this->m_pexecutable_receiving.get());
		}

		return;
	}

	// 2) RECV => multishot (the completions of the multishot being cancelled do not count)
	RETURN_IF(_is_provided);

	if (is_large == false && _bytes < (size_provided >> 1))
	{
		if (++this->m_count_receive_small_recv >= COUNT_SMALL_RECEIVES_TO_MULTISHOT)
		{
			this->m_receive_mode = RECEIVE_MODE_MULTISHOT;
			this->m_count_receive_small_recv = 0;
		}
	}
	else
	{
		this->m_count_receive_small_recv = 0;
	}
}

#endif
