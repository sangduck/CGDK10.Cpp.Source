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

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		io_context_Nepoll
 

*///-----------------------------------------------------------------------------
class api::io_context_Nepoll:
	virtual public				object::Idestroyable,
	private						non_copyable
{
// constructor/destructor)
public:
			io_context_Nepoll();
	virtual	~io_context_Nepoll() noexcept;

// public)
public:
	class RESULT : public epoll_event
	{
	public:
		RESULT() noexcept { data.ptr = nullptr; }

	public:
		[[nodiscard]] bool		is_valid() const noexcept { return data.ptr != nullptr;}
		[[nodiscard]] intptr_t	result() const noexcept { return events;}
		Iexecutable*			executable() const noexcept { return reinterpret_cast<Iexecutable*>(data.ptr); }
		[[nodiscard]] size_t	param() const noexcept { return 0; }
	};

	class executable_terminator : virtual public Iexecutable
	{
		virtual	intptr_t		process_execute(intptr_t, size_t) override { return 0; }
	};

public:
			size_t				attached_count() const noexcept { return this->m_count_attached;}
			[[nodiscard]] CGHANDLE handle() const noexcept { return this->m_handle_epoll;}
			
			bool				create(int _hint_size = 4096) noexcept;
			CGHANDLE			dispose() noexcept;
	virtual	result_code			destroy() noexcept override;

			// - busy poll of epoll_wait() itself (EPIOCSPARAMS of the kernel 6.9+. per epoll instance, no sysctl net.core.busy_poll needed)
			//   . _usecs  : epoll_wait() spins on the NIC queue of the sockets this long before sleeping (0 : off). the sockets of the epoll
			//               must share a NAPI ID (the same NIC queue) : the kernel does nothing for sockets without one (loopback, UDS)
			//   . _budget : packets per poll (0 : the default of the kernel 8. over 64 needs CAP_NET_ADMIN)
			//   . _prefer : prefer busy poll over the NIC interrupt (the NAPI stays in polling mode while epoll_wait() is called in time)
			//   returns false when the kernel has no EPIOCSPARAMS (before 6.9 : sysctl net.core.busy_poll applies instead) or on EPERM
			bool				set_busy_poll(uint32_t _usecs, uint16_t _budget = 0, bool _prefer = false) noexcept;
			bool				get_busy_poll(uint32_t& _usecs, uint16_t& _budget, bool& _prefer) const noexcept;
	[[nodiscard]] static bool	is_busy_poll_available() noexcept;

			bool				attach(CGHANDLE _handle, Iexecutable* _pexecutable, uint32_t _event) noexcept;
			bool				detach(CGHANDLE _handle) noexcept;
			bool				modify(CGHANDLE _handle, uint32_t _event, Iexecutable* _pexecutable)  noexcept;

// implementation)
protected:
			std::atomic<CGHANDLE>	m_handle_epoll;
			std::atomic<size_t>		m_count_attached;

	static	const size_t			BUCKET_COUNT = 4096;
	static	const CGHANDLE			BUCKET_MASK = 0x00000fff;

			// - epoll_event.data.u64 = (generation << 32) | handle
			//   generation is given for each registration : an event of a closed handle is not executed by the new registration of the same handle number
			//   (an event can stay in a batch of epoll_wait() while the handle is closed and the number is reused)
	static	constexpr uint64_t		_make_event_data(CGHANDLE _handle, uint32_t _generation) noexcept { return (static_cast<uint64_t>(_generation) << 32) | static_cast<uint32_t>(_handle); }
	static	constexpr CGHANDLE		_get_event_handle(uint64_t _event_data) noexcept { return static_cast<CGHANDLE>(static_cast<uint32_t>(_event_data)); }
	static	constexpr uint32_t		_get_event_generation(uint64_t _event_data) noexcept { return static_cast<uint32_t>(_event_data >> 32); }

			struct NODE
			{
				CGHANDLE			handle;
				Iexecutable*		pexecutable;
				uint32_t			generation;
				uint32_t			events;			// - the event mask of the registration (io_uring : re-arms a registration without EPOLLONESHOT by it)
			};

			struct BUCKET_REGISTER
			{
				std::shared_mutex	cs_register;
				std::vector<NODE>	vector_register;
			};

			BUCKET_REGISTER		m_manager_attached[BUCKET_COUNT];
			std::atomic<uint32_t>	m_generation{ 0 };
			CGHANDLE			_process_prepare_terminate(CGHANDLE _handle_epoll, executable_terminator* _pexecutable);
			void				_process_complete_terminate(CGHANDLE _handle_epoll, CGHANDLE _execution_handle) noexcept;
			uint64_t			_process_register_handle(CGHANDLE _handle, Iexecutable* _pexecutable);
			void				_process_unregister_handle(CGHANDLE _handle) noexcept;
			void				_process_remove_all_handle(CGHANDLE _handle_epoll) noexcept;
			Iexecutable*		_process_get_executable(uint64_t _event_data) const noexcept;
			bool				_process_get_event_data(CGHANDLE _handle, uint64_t& _event_data) const noexcept;
			bool				_process_update_handle(CGHANDLE _handle, Iexecutable* _pexecutable, uint64_t& _event_data) noexcept;
			bool				_process_set_event_mask(CGHANDLE _handle, uint32_t _events) noexcept;
			bool				_process_get_event_mask(CGHANDLE _handle, uint64_t& _event_data, uint32_t& _events) const noexcept;
};

}

#endif