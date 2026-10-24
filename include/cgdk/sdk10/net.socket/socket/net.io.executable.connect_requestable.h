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

//-----------------------------------------------------------------------------
//
// 
//
//-----------------------------------------------------------------------------
class CGDK::net::io::executable_connect_requestable :
	virtual public				Iexecutable,
	public						Npoolable<executable_connect_requestable>,
	public						factory::_traits_system
{
public:
			executable_connect_requestable() {}
	virtual ~executable_connect_requestable() {}

public:
	virtual	intptr_t			process_execute(intptr_t _result, size_t) override;
			void				set_connectable( net::io::connect_requestable::Ntcp* _pConnector) { this->m_pconnect_requestable = _pConnector; }
	virtual void				on_final_release() noexcept override { this->m_pconnect_requestable.reset(); this->m_is_op = false; this->m_is_connected = false; this->m_is_submit = false; this->m_seq = 0; this->m_fd = INVALID_SOCKET; }

public:
			object_ptr<net::io::connect_requestable::Ntcp>	m_pconnect_requestable;
			// - io_uring completion : posted to submit the CONNECT op by a worker (m_is_submit : the handle and the address), the executable
			//   of the op (m_is_op : the result is 0 / -errno), or posted for a socket connected at once (m_is_connected : a UDS). m_seq : the request
			bool				m_is_op = false;
			bool				m_is_connected = false;
			bool				m_is_submit = false;
			uint32_t			m_seq = 0;
			CGSOCKET			m_fd = INVALID_SOCKET;
			net::sockaddr		m_address;
};

inline intptr_t CGDK::net::io::executable_connect_requestable::process_execute(intptr_t _result, size_t)
{
#if defined(_SYS_EPOLL_H)
	// 1) move connect_requestable
	object_ptr<net::io::connect_requestable::Ntcp> pconnect_requestable = std::move(this->m_pconnect_requestable);

	// check)
	RETURN_IF(pconnect_requestable.empty(), 0);

	// - io_uring completion : posted by process_request_connecting : this worker thread submits the CONNECT op (the op belongs to it)
	if (this->m_is_submit)
	{
		this->m_is_submit = false;
		pconnect_requestable->process_submit_connect_op(this);
		return intptr_t(0);
	}

	// - io_uring completion : the result of the CONNECT op (0 / -errno) to the event of the poll, with the sequence of the request
	//   (EISCONN, m_is_connected : connected at once (a UDS) : the completion was posted. ECANCELED : closed meanwhile : a close event)
	if (this->m_is_op || this->m_is_connected)
	{
		const bool is_connected = this->m_is_connected || _result == 0 || _result == -EISCONN;

		if (is_connected == false && _result != -ECANCELED)
			pconnect_requestable->m_last_socket_error = static_cast<int>(-_result);

		_result = (is_connected ? static_cast<intptr_t>(EPOLLOUT) : static_cast<intptr_t>(EPOLLERR | EPOLLHUP)) | static_cast<intptr_t>(static_cast<uintptr_t>(this->m_seq) << net::io::connect_requestable::Ntcp::CONNECT_SEQ_SHIFT);
		this->m_is_op = false;
		this->m_is_connected = false;
		this->m_seq = 0;
	}

	// 2) call 'process_complete_connection'
	pconnect_requestable->process_complete_connecting(_result);
#else
	// 1) move connect_requestable
	object_ptr<net::io::connect_requestable::Ntcp> pconnect_requestable = std::move(this->m_pconnect_requestable);

	// check) 
	CGASSERT_ERROR(pconnect_requestable.exist());

	// 2) call 'process_complete_connection'
	pconnect_requestable->process_complete_connecting(_result);
#endif

	// return) 
	return intptr_t(0);
}
