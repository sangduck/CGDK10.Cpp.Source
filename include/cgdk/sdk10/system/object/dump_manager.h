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

#if defined(__linux__)
	#include <signal.h>
	#include <ucontext.h>
#endif

namespace CGDK
{

//-----------------------------------------------------------------------------
/**
  @class	dump_manager
  @brief	
 
*///-----------------------------------------------------------------------------
class dump_manager:
	public						object::Nnameable,
	public						Nmessage_transmitter,
	virtual public				object::Idestroyable,
	private						non_copyable
{
public:
		using ITERATOR = std::map<void*, DUMP_INFO>::const_iterator;

protected:
			dump_manager(eDUMP_CONTEXT _type_dump=eDUMP_CONTEXT::DEFAULT, size_t _max_dump_count=DEFAULT_MAX_DUMP_LIMIT) noexcept;
	virtual ~dump_manager() noexcept;

// public)
public:
			void				enable_exception_handling(bool _enable=true) noexcept;
			void				disable_exception_handling() noexcept;
			bool				is_exception_handling() const noexcept;
	[[deprecated("use enable_exception_handling")]]
			void				enable_exception_handleing(bool _enable=true) noexcept { this->enable_exception_handling(_enable); }
	[[deprecated("use disable_exception_handling")]]
			void				disable_exception_handleing() noexcept { this->disable_exception_handling(); }
	[[deprecated("use is_exception_handling")]]
			bool				is_excpetion_handling() const noexcept { return this->is_exception_handling(); }

			void				set_dump_type(eDUMP_CONTEXT _type_dump) noexcept { m_type_dump=_type_dump;}
			eDUMP_CONTEXT		get_dump_type() const noexcept { return m_type_dump;}
			void				set_dump_count_limit(size_t _limit) noexcept { m_count_dump_limit=_limit;}
			size_t				get_dump_count_limit() const noexcept { return m_count_dump_limit;}
			void				set_dump_path(std::string_view _path) noexcept { m_str_dump_path = _path; }
	[[nodiscard]] std::string_view get_dump_path() const noexcept { return m_str_dump_path; }
	[[nodiscard]] size_t		dump_count() const noexcept { return m_count_dump_now;}
																						  
	[[nodiscard]] auto&			get_lockable() noexcept { return m_lockable_map_dump; }
	[[nodiscard]] const auto&	get_lockable() const noexcept { return m_lockable_map_dump; }
	[[nodiscard]] 				operator std::mutex&() noexcept { return m_lockable_map_dump;}
	[[nodiscard]] 				operator const std::mutex&() const noexcept { return m_lockable_map_dump;}
	[[nodiscard]] ITERATOR		begin() const noexcept { return m_map_dump.begin();}
	[[nodiscard]] ITERATOR		end() const noexcept { return m_map_dump.end();}
	[[nodiscard]] std::vector<DUMP_INFO> status_snap() const noexcept;
																						  
	static 	auto				instance() { auto p = m_s_pinstance.get_acquire(); if (p == nullptr) { initialize_instance(); p = m_s_pinstance.get_acquire(); } return object_ptr<dump_manager>(p); }
	static	void				initialize_instance();
	static	void				initialize_instance(context& _context);
	static	void				set_instance(own_ptr<dump_manager>&& _instance) noexcept;
	static	void				reset_instance() noexcept;
	static	void				detach_instance() noexcept { m_s_pinstance.detach(); }

	static	void				print_statistics() noexcept;

			void				dispatch_exception_message_all();

public:
#if defined(_WIN32)
	virtual	void				process_exception(LPEXCEPTION_POINTERS _pexception);
	virtual	void				process_invalid_parameter(std::wstring_view _expression, std::wstring_view _function, std::wstring_view _file, unsigned int _line, uintptr_t _reserved);
#elif defined(__linux__)
	virtual	void				process_exception(int _signal);
	virtual	void				process_exception_detailed(int _signal, siginfo_t* _info, ucontext_t* _context);
#else
	#error "[CGDK10] Not supported Platform"
#endif

protected:
			bool				m_is_exception_handling;

			size_t				m_count_dump_limit;
			size_t				m_count_dump_now;
			eDUMP_CONTEXT		m_type_dump;
	mutable	std::mutex			m_lockable_map_dump;
			std::map<void*, DUMP_INFO> m_map_dump;
			circular_list<void*> m_list_exception_event;
			std::string			m_str_dump_path;

	static std::mutex			m_s_lockable_create;
	static own_ptr<dump_manager> m_s_pinstance;

private:
	virtual	result_code			destroy() noexcept override;

protected:
#if defined(_WIN32)
			bool				create_dump(const std::string& _str_dump_file, LPEXCEPTION_POINTERS _pexception);
			void				set_seh_to_thread() noexcept;
			void				set_invalid_parameter_handle() noexcept;
	static	void				translator_function(uint32_t, LPEXCEPTION_POINTERS _pexception);
	static	void				invalid_parameter_handler(wchar_t const* _expression, wchar_t const* _function, wchar_t const* _file, unsigned int _line, uintptr_t _reserved);
	friend class api::Nthread;
#elif defined(__linux__)
			bool				create_dump(const std::string& _str_dump_file);
	static	void				set_signal_handler() noexcept;
	static	void				set_signal_handler_crash() noexcept;
	static	void				translator_function(int _signal);
	static	void				translator_function_sa(int _signal, siginfo_t* _info, void* _context);
	friend class system_executor;
	friend class api::Nthread;
#else
	#error "[CGDK10] Not supported Platform"
#endif
};


// ----------------------------------------------------------------------------
//
//  Functions
//
// ----------------------------------------------------------------------------
void enable_exception_handling(bool _is_enable=true);
void disable_exception_handling();
bool is_exception_handling();
[[deprecated("use enable_exception_handling")]] inline void enable_exception_handleing(bool _is_enable=true) { enable_exception_handling(_is_enable); }
[[deprecated("use disable_exception_handling")]] inline void disable_exception_handleing() { disable_exception_handling(); }
[[deprecated("use is_exception_handling")]] inline bool is_excpetion_handling() { return is_exception_handling(); }

bool snap_dump(std::string_view _str_dump_file = std::string_view(), eDUMP_CONTEXT _type_dump = eDUMP_CONTEXT::DEFAULT);
}