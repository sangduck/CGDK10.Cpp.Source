#include "pch.h"

#if defined(_SYS_EPOLL_H)

using namespace CGDK;

extern std::size_t	g_bytes_message_buffer_default_min_size;
extern std::size_t	g_bytes_message_buffer_default_max_size;


net::io::receivable::Nstream::Nstream() :
	m_minimum_message_buffer_size(g_bytes_message_buffer_default_min_size),
	m_maximum_message_buffer_size(g_bytes_message_buffer_default_max_size)
{
	// check) 최소 MINIMUM_REMAINED_RECEIVE_BUFFER보다는 크거나 같아야 한다.
	CGASSERT_FATAL(this->m_minimum_message_buffer_size >=MINIMUM_REMAINED_RECEIVE_BUFFER);

	// check) MAX_TCP_MESSAGE_SIZE보다는 같거나 작아야 한다.
	CGASSERT_FATAL(this->m_maximum_message_buffer_size <= MAX_TCP_MESSAGE_SIZE);

	// check) m_minimum_message_buffer_size보다 m_maximum_message_buffer_size가 커야한다.
	CGASSERT_FATAL(this->m_minimum_message_buffer_size <= this->m_maximum_message_buffer_size);

	// check) m_maximum_message_buffer_size가  풀의 최고 할당 가능 메모리(MAX_TCP_MESSAGE_SIZE)보다 크면 경고!
	if(this->m_maximum_message_buffer_size > MAX_TCP_MESSAGE_SIZE)
		LOG_WARNING << "(!) max receive shared_buffer size is too BIG (" << this->m_maximum_message_buffer_size << ")";
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

	// 3) reset the shrink hysteresis
	this->m_count_receive_small = 0;
}

bool net::io::receivable::Nstream::prepare_receive()
{
	// 1) get io_context
	auto pio_context = io_context();

	// check) io_context가 없으면 잘못된 것이다.
	if(pio_context.empty())
		return false;

	// 2) 현 버퍼 크기를 Default Min크기로 한다.
	this->m_bytes_buffer_receive = net::io::socket::Ntcp::default_maximum_receive_buffer_size();

	// 3) 버퍼 크기를 설정한다.
	//    (linux : not set by default. an explicit SO_RCVBUF turns the autotuning of the kernel off. net::io::socket::Ntcp::enable_explicit_socket_buffer_size())
	if (net::io::socket::Ntcp::is_explicit_socket_buffer_size_enabled())
		api::socket::set_receive_buffer_size(this->get_epoll_handle(), this->m_bytes_buffer_receive);

	// 설명) prepare_receive()에 EPOLLOUT이 설정될 수 있다. on_connect() 함수 내에서 send를 호출했을 경우 가능하다.

	// 4) set epoll event 
	//    (EPOLLET : small receives. socket::Ntcp::process_execute_epoll switches it to EPOLLONESHOT while large receives come : Isocket.h)
	this->set_epoll_event_flag(this->m_epoll_event_flag | (EPOLLET | EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR));

	// return)
	return true;
}

intptr_t net::io::receivable::Nstream::process_complete_receivable( intptr_t _event, std::size_t /*_transferred*/)
{
	// check) 
	CGASSERT_FATAL(this->socket_state() != eSOCKET_STATE::SYN);

	// declare)
	intptr_t result_return = EPOLLIN;

	try
	{
		//-----------------------------------------------------------------
		// 설명) 수신 버퍼 (iocp와 같은 방식)
		//
		//  - m_buffer_received는 처리되지 않은 데이터를 가리킨다. 그 뒤의 빈 공간(tail)에 바로 recv한다.
		//    (event마다 FIONREAD로 크기를 묻고 새 버퍼를 할당해 남은 데이터를 복사하지 않는다)
		//  - 빈 공간이 모자라면(다음 message가 들어가지 않거나, 받던 message의 나머지가 아닌데 MINIMUM_REMAINED_RECEIVE_BUFFER 미만이면)
		//    1) 버퍼를 이 socket만 참조하고 있고 크기가 충분하면 남은 데이터를 앞으로 옮긴다. (할당하지 않는다)
		//    2) 아니면 새로 할당해 남은 데이터만 복사한다. (전달된 message를 다른 곳에서 참조하고 있을 수 있으므로 덮어쓰지 않는다)
		//  - 창을 다 채워 받았으면 버퍼를 8배로 늘린다. (다시 쓰면 버퍼가 커지지 않아 recv마다 지금 크기만큼만 받게 된다)
		//    MAX_BYTES_RECEIVE_WINDOW_GROWN까지만 늘린다. (더 큰 창은 cache 밖으로 나가 message당 CPU가 는다 : 64KB message 기준 +20%)
		//    받는 양이 크게 줄면 되감을 때 작은 버퍼로 바꾼다. (크기 규칙은 iocp와 같다)
		//  - 창을 다 채워 받았으면 남은 데이터가 더 있을 수 있으므로 다시 받는다. (덜 채웠으면 끝낸다 : 남은 것은 재등록 후 event로 다시 온다)
		//-----------------------------------------------------------------
		// declare)
		net::sockaddr address_peer;
		std::size_t bytes_received_event = 0; // 이번 event에서 받은 전체 크기 (버퍼 크기를 정할 때 쓴다)

		// 1) 수신 버퍼가 없으면 할당한다. (처음 받을 때, reset된 뒤)
		if (this->m_buffer_received.get_upper_bound() == nullptr)
			this->m_buffer_received = alloc_shared_buffer(this->m_minimum_message_buffer_size);

		// - 빈 공간이 없으면 준비한다. (창이 0이면 recv가 0을 돌려 접속 종료로 잘못 보게 된다)
		//   (지난 수신이 받던 message의 나머지만큼만 남겨 두었을 수 있다 : MINIMUM_REMAINED_RECEIVE_BUFFER 미만이어도 그대로 받는다)
		else if (this->m_buffer_received.remained_size() == 0)
			this->_process_prepare_receive_buffer(0, false, 0);

		for (int count_receive = 1; ; ++count_receive)
		{
			// 2) get socket handle (처리 중 접속이 끊겼으면 더 받지 않는다)
			const auto handle_socket = this->native_handle();

			// check)
			if (handle_socket == INVALID_SOCKET)
				break;

			// 3) receive to tail of the data not processed
			const auto pbuf_receive = this->m_buffer_received.data() + this->m_buffer_received.size();
			const auto bytes_window = this->m_buffer_received.remained_size();

			// check)
			CGASSERT_ERROR(bytes_window != 0);

			const auto bytes_transfered = ::recv(handle_socket, pbuf_receive, bytes_window, MSG_DONTWAIT);

			// check) 0 : 상대가 접속을 끊었다. (EPOLLRDHUP/EPOLLHUP event로 종료 처리된다)
			if (bytes_transfered == 0)
				break;

			// check)
			if (bytes_transfered < 0)
			{
				// - get error
				const auto error_code = api::get_last_error();

				// check) 받을 데이터가 없다.
				if(error_code == EAGAIN || error_code == EWOULDBLOCK || error_code == EINTR)
					break;

				// - set last error
				this->m_last_socket_error = error_code;

				// return)
				return EPOLLHUP;
			}

			// statistics)
			this->statistics_on_receive_bytes(bytes_transfered);

			// - TCP_QUICKACK is not permanent (the kernel returns to the delayed ACK) : set it again after every recv (socket_option().tcp_quickack)
			if (this->socket_option().tcp_quickack)
				api::socket::set_tcp_quickack(handle_socket, true);

			// 4) add size
			this->m_buffer_received.add_size(static_cast<std::size_t>(bytes_transfered));
			bytes_received_event += static_cast<std::size_t>(bytes_transfered);

			// debug)
		#if !defined(NDEBUG) || defined(_DEBUG)
			if(this->is_socket_debug_logging_enabled())
				net::io::_debug_log_socket_received(handle_socket, bytes_transfered, this->m_buffer_received.size());
		#endif

			// 5) call process_on_receive (일반적으로 여기서 하는 역할은 거의 없다.)
			this->process_on_receive(buffer_view{ pbuf_receive, static_cast<std::size_t>(bytes_transfered) }, address_peer);

			// 6) process message
			//    - 여기서 Return해 오는 값은 다음 message를 받기 위해 다음에 필요로 하는 메모리 용량이다.
			//    - 주어지는 m_buffer_received에 진행된 Memory과 남은 Size등이 되돌아 온다.
			//    - Exception이 발생가능하다.
			//    - send cork : 이번에 받은 message들을 처리하며 이 thread가 이 socket에 보내는 것은 모아 두었다가
			//      처리가 끝나면 한번에 보낸다. (recv 한번마다 보낸다 : 응답이 다음 recv까지 기다리지 않는다)
			//      exception이 나도 그때까지 모은 것은 보낸 뒤 접속을 끊는다.
			std::size_t required_bytes;
			{
				net::io::send_cork cork(this);

				required_bytes = this->process_packet(this->m_buffer_received, address_peer);
			}

			// check) 필요 message size가 최대 message buffer size보다 크면 exception을 던진다.
			THROW_IF(required_bytes > this->m_maximum_message_buffer_size, throwable::failure(eRESULT::INVALID_MESSAGE_TOO_LONG_LENGTH), LOG_ERROR << log_level_t::important() << "(excp) CGDK.net.socket: message size of to BIG. required size:" << required_bytes << "bytes/max_buffer_size:" << this->m_maximum_message_buffer_size << "bytes, ip:" << remote_endpoint().to_string());

			// 7) 다음 수신을 위한 빈 공간을 준비한다. (창을 다 채워 받았으면 버퍼를 늘린다)
			const bool is_window_filled = static_cast<std::size_t>(bytes_transfered) >= bytes_window;

			this->_process_prepare_receive_buffer(bytes_received_event, is_window_filled, required_bytes);

			// check) 창을 다 채우지 못했으면 지금 받을 데이터는 다 받았다. (너무 오래 붙잡지 않도록 횟수도 제한한다)
			//   the peer closed (EPOLLRDHUP / EPOLLHUP) : no limit. the event closes the socket after this function, so what is left in the
			//   socket buffer would never be received (the messages sent just before a shutdown / close were lost, and closing with unread
			//   data sends a RST). nothing arrives after the FIN : it ends at the end of the data.
			//   a large receive (a filled window of 4KB+, 16KB+ in this event) : the socket goes EPOLLONESHOT (EPOLL_RECEIVE_LARGE : Isocket.h).
			//   cut by the limit : EPOLL_RECEIVE_MORE (EPOLLET gives no new edge for the data left : the registration is re-armed)
			if ((is_window_filled && static_cast<std::size_t>(bytes_transfered) >= 4096) || bytes_received_event >= 16384)
				result_return |= EPOLL_RECEIVE_LARGE;

			if (is_window_filled == false)
				break;

			if (count_receive >= MAX_RECEIVE_PER_EVENT && (_event & (EPOLLRDHUP | EPOLLHUP)) == 0)
			{
				result_return |= EPOLL_RECEIVE_MORE;
				break;
			}
		}
	}
	catch (throwable::failure& _e)
	{
		// exception) 보통 여기까지 진행되면 막장이다. 해당 socket을 끊고 끝낸다.
		//
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (throwable) " << __CGFUNCTION__;
		LOG_DEBUG << "       - reason: " << CGDK::stream_hex(_e.reason<uint64_t>(), 0, false, true) << " [" << to_string(_e.reason<eRESULT>()) << "]";
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
		// * on_receive()는 User Define함수이므로 이 함수 수행 중 Exception발생가능성이 있다.
		// * process_packet() 역시 수행 중 Exception이 발생가능하다.

		// trace) 
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed (std::exception) " << __CGFUNCTION__;
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
		LOG_DEBUG << "(excp) CGDK.net.socket: exception occurred and socket closed " __CGFUNCTION__;

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

void net::io::receivable::Nstream::_process_prepare_receive_buffer(std::size_t _bytes_received, bool _is_window_filled, std::size_t _required_bytes)
{
	// declare)
	const auto bytes_stored = this->m_buffer_received.size();
	const auto plower = static_cast<char*>(const_cast<void*>(this->m_buffer_received.get_lower_bound()));
	const auto bytes_capacity = static_cast<std::size_t>(static_cast<const char*>(this->m_buffer_received.get_upper_bound()) - plower);
	const auto is_unique = this->m_buffer_received.is_source_unique();

	// 1) 알맞은 버퍼 크기를 정한다. (크기 규칙은 iocp와 같다. 받은 크기는 이번 event에서 받은 전체 크기로 본다)
	std::size_t size_prepare;

	// - 창을 다 채워 받았다 => 8배로 늘린다. (MAX_BYTES_RECEIVE_WINDOW_GROWN까지 : 더 큰 버퍼는 message가 그만큼 클 때만 쓴다)
	if (_is_window_filled)
	{
		size_prepare = bytes_capacity * 8;
		SATURATE_MAX(size_prepare, MAX_BYTES_RECEIVE_WINDOW_GROWN);
		size_prepare = __select_max(size_prepare, bytes_capacity);
	}
	// - 버퍼 크기의 절반 이상을 받았다 => 크기를 유지한다.
	else if (_bytes_received >= (bytes_capacity >> 1))
	{
		size_prepare = bytes_capacity;
	}
	// - 절반 미만 => 받은 크기보다 크게만 한다.
	else
	{
		size_prepare = this->m_minimum_message_buffer_size;
		while (_bytes_received > size_prepare) size_prepare *= 8;
	}

	// - 최대 크기 제한
	SATURATE_MAX(size_prepare, this->m_maximum_message_buffer_size);

	// - 최소 [요구_버퍼_크기]+[이미_수신한_데이터_크기]보다는 커야 한다. (빈 공간도 MINIMUM_REMAINED_RECEIVE_BUFFER 이상)
	const auto size_need = __select_max(_required_bytes, bytes_stored + MINIMUM_REMAINED_RECEIVE_BUFFER);	// - _required_bytes : the whole next message (its bytes stored already included : adding them made a buffer up to twice the message)
	size_prepare = __select_max(size_prepare, size_need);

	// 2) 이 socket만 참조하는 버퍼는 받는 양이 크게 줄었을 때만 줄인다. (버스트 뒤 큰 버퍼를 계속 붙잡지 않는다)
	//    - 줄이는 크기 : 이번 event에서 받은 크기의 8배보다 크고 다음 message가 여유 있게 들어가는 규칙 크기(최소 크기 x 8^n)
	//      (조금 줄였다가 다시 늘리며 남은 데이터를 복사하지 않도록)
	std::size_t size_shrink = this->m_minimum_message_buffer_size;
	while (size_shrink <= _bytes_received * 8 || size_shrink < size_need + MINIMUM_REMAINED_RECEIVE_BUFFER) size_shrink *= 8;
	SATURATE_MAX(size_shrink, this->m_maximum_message_buffer_size);
	size_shrink = __select_max(size_shrink, size_need);

	//    - hysteresis : shrink only after COUNT_SMALL_RECEIVES_TO_SHRINK consecutive small receives. (a filled window or a receive of the shrink size resets the count)
	//      (shrinking at the first small receive made a stream with variable arrivals reallocate at every event : shrink to the minimum, grow 8x at the next burst)
	const bool is_small = _is_window_filled == false && size_shrink < bytes_capacity;

	if (is_small == false)
		this->m_count_receive_small = 0;
	else if (this->m_count_receive_small < COUNT_SMALL_RECEIVES_TO_SHRINK)
		++this->m_count_receive_small;

	const bool is_shrink = is_unique && is_small && this->m_count_receive_small >= COUNT_SMALL_RECEIVES_TO_SHRINK;

	if (is_shrink)
	{
		size_prepare = size_shrink;
		this->m_count_receive_small = 0;
	}

	// 3) 지금 버퍼를 다시 쓸 수 있으면 다시 쓴다. (할당하지 않는다)
	//    - 창을 다 채워 받았는데 더 늘릴 수 있으면 다시 쓰지 않는다. (다시 쓰면 버퍼가 커지지 않아 recv마다 지금 크기만큼만 받게 된다)
	//    - 줄일 때는 되감을 때(남은 데이터가 없을 때 / 빈 공간이 모자랄 때) 작은 버퍼로 바꾼다.
	if (_is_window_filled == false || size_prepare <= bytes_capacity)
	{
		// - 남은 데이터가 없고 이 socket만 버퍼를 참조하고 있으면 처음부터 다시 쓴다. (복사 없음)
		if (bytes_stored == 0 && is_unique && is_shrink == false && this->m_buffer_received.data() != plower)
			this->m_buffer_received.set_data(plower);

		// check) 빈 공간이 충분하면 그대로 쓴다. (줄일 때 남은 데이터가 없으면 지금 줄인다)
		//        받던 message의 나머지가 들어가면 빈 공간이 MINIMUM_REMAINED_RECEIVE_BUFFER보다 작아도 그대로 받는다. (옮기거나 새로 할당하면 받던 message를 통째로 복사한다)
		const auto bytes_remained = this->m_buffer_received.remained_size();
		const bool is_fit = (bytes_stored + bytes_remained) >= _required_bytes && (bytes_remained >= MINIMUM_REMAINED_RECEIVE_BUFFER || (bytes_remained != 0 && bytes_stored != 0 && _required_bytes > bytes_stored));
		RETURN_IF(is_fit && (is_shrink == false || bytes_stored != 0));

		// - 이 socket만 참조하고 있고 크기가 충분하면 남은 데이터를 앞으로 옮긴다.
		if (is_unique && is_shrink == false && bytes_capacity >= _required_bytes && (bytes_capacity - bytes_stored) >= MINIMUM_REMAINED_RECEIVE_BUFFER)
		{
			::memmove(plower, this->m_buffer_received.data(), bytes_stored);
			this->m_buffer_received.set_data(plower);
			return;
		}
	}

	// 4) 새로 할당한다. (실패하면 exception : 호출한 쪽에서 접속을 끊는다)
	//    - message 크기와 꼭 같은 버퍼는 피한다. (message 하나를 받고 몇 byte가 남을 때마다 복사하게 된다 : 64KB message echo에서 message마다 64KB memmove)
	if (size_prepare < size_need + MINIMUM_REMAINED_RECEIVE_BUFFER && (size_need + MINIMUM_REMAINED_RECEIVE_BUFFER) <= this->m_maximum_message_buffer_size)
		size_prepare = size_need + MINIMUM_REMAINED_RECEIVE_BUFFER;

	auto buf_new = alloc_shared_buffer(size_prepare);

	// - 남은 데이터만 앞쪽에 복사한다.
	if (bytes_stored != 0)
		::memcpy(buf_new.data(), this->m_buffer_received.data(), bytes_stored);

	// - set
	buf_new ^= bytes_stored;
	this->m_buffer_received = std::move(buf_new);
}

void net::io::receivable::Nstream::process_clear_buffer() noexcept
{
	// 1) 버퍼를 할당 해제한다.
	this->m_buffer_received.clear();
}

#endif
