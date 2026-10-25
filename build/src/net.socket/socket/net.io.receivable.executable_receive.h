//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// net::io::receivable::Ndatagram::executable_receive
//
//-----------------------------------------------------------------------------
#if defined(_WINSOCK2API_)
class net::io::receivable::Ndatagram::executable_receive :
	public						Iexecutable
{
public:
			void				set_receivable( net::io::receivable::Ndatagram* _preceivable) noexcept { this->m_preceivable_datagram = _preceivable; }
	[[nodiscard]] shared_buffer& receive_buffer() noexcept { return this->m_buffer;}
																		  
	[[nodiscard]] const net::sockaddr& sockaddr() const noexcept { return this->m_socket_address[0]; }
	[[nodiscard]] ::sockaddr*	sockaddr_pointer() noexcept { return reinterpret_cast<::sockaddr*>(this->m_socket_address);}
	[[nodiscard]] int*			sockaddr_length() noexcept { return &this->m_socket_address[0].length;}
			void				set_receive_buffer(shared_buffer&& _buffer) noexcept { this->m_wsabuf = WSABUF{ _buffer.size<ULONG>(), _buffer.data() }; this->m_buffer = std::move(_buffer); }
			void				set_length_of_address( int _length) noexcept { this->m_socket_address[0].length = _length; }
	[[nodiscard]] LPWSABUF		get_wsabuf() noexcept { return &m_wsabuf; }
	[[nodiscard]] LPDWORD 		reset_and_get_ptr_bytes_reiced() noexcept { this->m_bytes_received = 0; return &m_bytes_received; }
	[[nodiscard]] LPDWORD 		reset_and_get_ptr_flags() noexcept { this->m_flags = 0; return &m_flags; }

	// - the buffer of the next receive : this one again when nothing else refers to it (the message was not kept) and it is large enough,
	//   a new one otherwise (a buffer of the message size per datagram was allocated before : 64KB by default)
			void				prepare_receive_buffer(std::size_t _size)
			{
				const auto plower = static_cast<char*>(const_cast<void*>(this->m_buffer.get_lower_bound()));
				if (this->m_buffer.is_source_unique() && plower != nullptr && static_cast<std::size_t>(static_cast<const char*>(this->m_buffer.get_upper_bound()) - plower) >= _size)
				{
					this->m_buffer.set_data(plower);
					this->m_buffer.set_size(_size);
				}
				else
				{
					this->m_buffer = alloc_shared_buffer(_size) ^ _size;
				}
				this->m_wsabuf = WSABUF{ static_cast<ULONG>(_size), this->m_buffer.data() };
			}

	// - the status of the completed receive (OVERLAPPED::Internal : the NTSTATUS of the I/O). the completion result of the io_context is
	//   only 0 / 1 (GetQueuedCompletionStatusEx) and the last error of the thread is not the one of this I/O
	[[nodiscard]] LONG			io_status() const noexcept { return static_cast<LONG>(static_cast<ULONG>(this->Internal)); }

private:
	virtual	intptr_t			process_execute( intptr_t _result, std::size_t _param) override;

			net::io::receivable::Ndatagram*	m_preceivable_datagram = nullptr;
			WSABUF				m_wsabuf {0, nullptr};
			shared_buffer		m_buffer;
			DWORD				m_bytes_received = 0;
			DWORD				m_flags = 0;
			net::sockaddr		m_socket_address[2];
};
#endif
}