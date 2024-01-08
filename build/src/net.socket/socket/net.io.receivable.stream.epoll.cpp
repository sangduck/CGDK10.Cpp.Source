#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


net::io::receivable::Nstream::Nstream() :
	m_minimum_mesage_buffer_size(g_bytes_message_buffer_default_min_size),
	m_maximum_message_buffer_size(g_bytes_message_buffer_default_max_size)
{
	// check) 최소 MINIMUM_REMAINED_RECEIVE_BUFFER보다는 크거나 같아야 한다.
	CGASSERT_ERROR(this->m_minimum_mesage_buffer_size >=MINIMUM_REMAINED_RECEIVE_BUFFER);

	// check) MAX_TCP_MESSAGE_SIZE보다는 같거나 작아야 한다.
	CGASSERT_ERROR(this->m_maximum_message_buffer_size <= MAX_TCP_MESSAGE_SIZE);

	// check) m_minimum_mesage_buffer_size보다 m_maximum_message_buffer_size가 커야한다.
	CGASSERT_ERROR(this->m_minimum_mesage_buffer_size <= this->m_maximum_message_buffer_size);

	// check) m_maximum_message_buffer_size가  풀의 최고 할당 가능 메모리(MAX_TCP_MESSAGE_SIZE)보다 크면 경고!
	EXECUTE_IF(this->m_maximum_message_buffer_size > MAX_TCP_MESSAGE_SIZE, LOG_WARNING << "(!) max receive shared_buffer size is too BIG (" << this->m_maximum_message_buffer_size << ")");
}

net::io::receivable::Nstream::~Nstream() noexcept
{
}

void net::io::receivable::Nstream::process_reset_receivable()
{
	// 1) shared_buffer를 setting한다.
	this->m_buffer_received.clear();

	// 2) 최소 receive_buffer 크기로 설정한다.
	this->m_bytes_buffer_receive = net::io::socket::Ntcp::default_maximum_receive_buffer_size();
}

bool net::io::receivable::Nstream::prepare_receive()
{
	// 1) get io_context
	auto pio_context = io_context();

	// check) io_context가 없으면 잘못된 것이다.
	RETURN_IF(pio_context.empty(), false);

	// 2) 현 버퍼 크기를 Default Min크기로 한다.
	this->m_bytes_buffer_receive = net::io::socket::Ntcp::default_maximum_receive_buffer_size();

	// 3) 버퍼 크기를 설정한다.
	api::socket::set_receive_buffer_size(this->m_epoll_handle, this->m_bytes_buffer_receive);

	// 설명) prepare_receive()에 EPOLLOUT이 설정될 수 있다. on_connect() 함수 내에서 send를 호출했을 경우 가능하다.

	// 4) set epoll event 
	this->set_epoll_event_flag(this->m_epoll_event_flag | (EPOLLONESHOT | EPOLLWAKEUP | EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR));

	// return)
	return true;
}

intptr_t net::io::receivable::Nstream::process_complete_receivable( intptr_t /*_result*/, std::size_t /*_transferred*/)
{
	// check) 
	CGASSERT_ERROR(this->socket_state() != eSOCKET_STATE::SYN);

	// declare)
	intptr_t result_return = EPOLLIN;

	try
	{
		// 1) get socket handle
		auto handle_socket = this->native_handle();

		// 2) get pended receive bytes
		auto bytes_receive_pended = api::socket::get_pending_read_buffer(handle_socket);

		// check) return if there are no pended receive data.
		RETURN_IF(bytes_receive_pended == 0, result_return);

		// declare) 
		shared_buffer buffer_receive;

		// 3) if bytes_receive_peneded size is less than min_size, set as min_size
		auto buffer_size = __select_max(static_cast<std::size_t&>(g_bytes_message_buffer_default_min_size), bytes_receive_pended);

		// 4) alloc buffer 
		{
			// 설명) 256과 4*n승 크기로 saturation한다. 그렇지 않으면 너무 다양한 크기의 pool에서 메모리를 할당 받아 pool의 효율이 떨어진다.

			// - if bytes_receive_peneded size is less than min_size, set as min_size
			auto required_bytes = this->m_buffer_received.size() + buffer_size;

			// - saturate size
			std::size_t temp_size = 256; while (temp_size < required_bytes) (temp_size <<= 2);

			// - alloc shared buffer
			buffer_receive = alloc_shared_buffer(temp_size);
		}

		// 5) receive pended  data (offset for stored data)
		const int bytes_transfered = ::recv(handle_socket, buffer_receive.data() + this->m_buffer_received.size(), static_cast<int>(buffer_size), MSG_DONTWAIT);

		// check) 
		RETURN_IF(bytes_transfered == 0, result_return);

		// check) 
		if (bytes_transfered < 0 && api::get_last_error() != EAGAIN)
		{
			// - set last error
			this->m_last_socket_error = api::get_last_error();

			// return) 
			return EPOLLHUP;
		}

		// statistics)
		this->statistics_on_receive_bytes(bytes_transfered);

		// 6) copy stored data to front of new buffer
		if (this->m_buffer_received.size() != 0)
		{
			buffer_receive.append(this->m_buffer_received.size(), this->m_buffer_received.data());
		}

		// 7) add size 
		buffer_receive.add_size(bytes_transfered);

		// debug)
	//  debug)
	#if !defined(NDEBUG) || defined(_DEBUG)
		if(this->is_socket_debug_logging_enabled())
			net::io::_debug_log_socket_received(handle_socket, bytes_transfered, buffer_receive.size());
	#endif

		// declare)
		std::size_t required_bytes;
		net::sockaddr address_peer;

		// 10) call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
		this->process_on_receive(buffer_receive ^ bytes_transfered, address_peer);

		// 11) process message
		//    - 여기서 Return해 오는 값은 다음 message를 받기위해 대음에 필요로 하는 메모리 용량이다.
		//    - 주어지는 &buffer_receive에 진행된 Memory과 남은 Size등이 되돌아 온다.
		//      따라서 buffer_receive은 받은 데이터 그대로 사용하면 된다.
		//    - Exception이 발생가능하다.
		required_bytes = this->process_packet(buffer_receive, address_peer);

		// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
		THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string<char>());

		// 12) 처리하고 남은 Bytes들을 저장해 놓는다.
		if (buffer_receive.size() != 0)
		{
			this->m_buffer_received = buffer_receive;
		}
		else
		{
			this->m_buffer_received.clear();
		}
	}
	catch (throwable::failure& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Definine함수이므로 이 함수 수행중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed (throwable) " << __CGFUNCTION__;
		LOG_DEBUG << "       - reason: " << NFMT::format("0x{:x}", _e.reason<uint64_t>()) << " [" << to_string<char>(_e.reason<eRESULT>()) << "]";
		if (_e.description().empty() == false) LOG_DEBUG << "       - description: " << _e.description();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - set result
		result_return = EPOLLHUP;
	}
	catch (std::exception& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Definine함수이므로 이 함수 수행중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed (std::exception) " << __CGFUNCTION__;
		LOG_DEBUG << "       - " << _e.what();

		// - call 'exception handler'
		throwable::execute_exception_handler(this, _e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - set result
		result_return = EPOLLHUP;
	}
	catch (...)
	{
		// exception) 위의 exception과 동일한 설명

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occure and socket closed " __CGFUNCTION__;

		// declare)
		throwable::failure	e(eRESULT::EXCEPTION);

		// - call 'exception handler'
		throwable::execute_exception_handler(this, e, __FUNCTION__);

		// statistics) Error로 인한 disconnect임을 표시한다.
		this->statistics_set_error_disconnect();

		// - disconnect를 수행한다.
		this->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);

		// - set result
		result_return = EPOLLHUP;
	}

	// return)
	return result_return;
}

#endif
