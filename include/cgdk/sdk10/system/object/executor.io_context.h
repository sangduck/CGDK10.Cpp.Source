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

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

   executor::io_context

*/
//-----------------------------------------------------------------------------
class executor::io_context :
	virtual public				executor::Iio_context,
	public						object::Ninitializable_startable,
	public						Nexecutor,
	public						factory::_traits_system
{
public:
			io_context() noexcept;
	virtual	~io_context() noexcept;

public:
#if defined(__ANDROID__) || defined(__linux__)
	virtual	bool				attach(CGHANDLE _executable_handle, Iexecutable* _pexecutable, uint32_t _option = 0) override { return this->m_io_context.attach(_executable_handle, _pexecutable, _option);}
#else
	virtual	bool				attach(CGHANDLE _executable_handle) override { return this->m_io_context.attach(_executable_handle);}
#endif
	virtual	bool				detach(CGHANDLE _executable_handle) noexcept override { return this->m_io_context.detach(_executable_handle);}
	[[nodiscard]] virtual auto	handle() const noexcept -> CGHANDLE override { return this->m_io_context.handle();}
	[[nodiscard]] virtual auto	pended_count() const noexcept -> size_t override { return this->m_io_context.queued_count(); }

#if defined(__ANDROID__) || defined(__linux__)
	virtual	bool				set_event(CGHANDLE _handle, uint32_t _flag_event, Iexecutable* _pexecutable = nullptr) noexcept override { return this->m_io_context.modify(_handle, _flag_event, _pexecutable); }
	[[nodiscard]] virtual executor::io_backend backend() const noexcept override { return this->m_io_context.backend(); }
	[[nodiscard]] virtual size_t attached_count() const noexcept override { return this->m_io_context.attached_count(); }
	virtual	bool				set_busy_poll(uint32_t _usecs, uint16_t _budget = 0, bool _prefer = false) noexcept override { return this->m_io_context.set_busy_poll(_usecs, _budget, _prefer); }
	virtual	bool				get_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) const noexcept override { return this->m_io_context.get_busy_poll(_usecs, _budget, _prefer); }

	// - the op API of the io_uring_completion backend (false on the others)
	virtual	bool				submit_recv(CGHANDLE _fd, void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_recv(_fd, _buf, _len, _msg_flags, _pexecutable); }
	virtual	bool				submit_recvmsg(CGHANDLE _fd, msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_recvmsg(_fd, _msg, _msg_flags, _pexecutable); }
	virtual	bool				submit_send(CGHANDLE _fd, const void* _buf, size_t _len, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept override { return this->m_io_context.submit_send(_fd, _buf, _len, _msg_flags, _pexecutable, _zerocopy); }
	virtual	bool				submit_sendmsg(CGHANDLE _fd, const msghdr* _msg, int _msg_flags, Iexecutable* _pexecutable, bool _zerocopy) noexcept override { return this->m_io_context.submit_sendmsg(_fd, _msg, _msg_flags, _pexecutable, _zerocopy); }
	virtual	bool				submit_accept(CGHANDLE _listen_fd, sockaddr* _addr, socklen_t* _addrlen, int _flags, bool _multishot, Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_accept(_listen_fd, _addr, _addrlen, _flags, _multishot, _pexecutable); }
	virtual	bool				submit_connect(CGHANDLE _fd, const sockaddr* _addr, socklen_t _addrlen, Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_connect(_fd, _addr, _addrlen, _pexecutable); }
	virtual	bool				submit_recv_multishot(CGHANDLE _fd, Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_recv_multishot(_fd, _pexecutable); }
	[[nodiscard]] virtual const void* get_provided_buffer(size_t _param) const noexcept override { return this->m_io_context.get_provided_buffer(_param); }
	virtual	bool				return_buffer(size_t _param) noexcept override { return this->m_io_context.return_buffer(_param); }
	[[nodiscard]] virtual size_t provided_buffer_size() const noexcept override { return this->m_io_context.provided_buffer_size(); }
	[[nodiscard]] virtual bool	is_ring_owner(CGHANDLE _fd) const noexcept override { return this->m_io_context.is_ring_owner(_fd); }
	virtual	bool				submit_cancel(Iexecutable* _pexecutable) noexcept override { return this->m_io_context.submit_cancel(_pexecutable); }
	virtual	bool				submit_cancel_fd(CGHANDLE _fd) noexcept override { return this->m_io_context.submit_cancel_fd(_fd); }
	virtual	bool				wait_cancelled_fd(CGHANDLE _fd, chrono::tick::duration _ticks_wait, bool _is_wait_delivery = true) noexcept override { return this->m_io_context.wait_cancelled_fd(_fd, _ticks_wait, _is_wait_delivery); }
#endif
	virtual	void				execute_all() noexcept;

protected:
	virtual result_code			_process_initializing(context& _context) override;
	virtual result_code			_process_initialize(context& _context) override;
	virtual result_code			_process_destroying() noexcept override;
	virtual result_code			_process_destroy() noexcept override;

	virtual result_code			_process_starting(context& _context) override;
	virtual result_code			_process_start(context& _context) override;
	virtual result_code			_process_stopping() noexcept override;
	virtual result_code			_process_stop() noexcept override;

	virtual	bool				process_post(object_ptr<Iexecutable>&& _pexecutable) override;
	virtual	bool				process_execute(chrono::tick::duration _ticks_wait, intptr_t _option) override;

			// - a worker thread of this executor (linux io_uring_completion : binds a ring at its start, unbinds it at its end)
			void				process_bind_worker(bool _is_transient = false) noexcept;
			void				process_unbind_worker() noexcept;

protected:
			OBJ<api::io_context> m_io_context;
			lock_object			m_lock_wait;
};


}