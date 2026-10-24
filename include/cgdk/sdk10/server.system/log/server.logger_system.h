//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                            Server Log Classes                             *
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
// logger_system
//
//  - writes logs to the system log.
//    . windows : Event Log ('Application', event source 'source')
//                event message file is '%SystemRoot%\System32\EventCreate.exe' (event id 1 shows the text as it is)
//                register_event_source()/unregister_event_source() register/unregister the event source. (administrator)
//                (Xservice registers it on '--install' and unregisters it on '--uninstall')
//    . linux   : syslog (openlog(source, LOG_PID, LOG_DAEMON))
//  - log types
//    . default    : fatal, error, warning
//    . all_types  : all log types (info/progress/debug/user/system are written as information)
//  - context ('logger' setting)
//    { "type": "system", "source": "<name>", "all_types": false }
//
//-----------------------------------------------------------------------------
class logger_system :
	public						Nlogger_async		// - written on the execution of this logger (the callers never wait for the system log)
{
public:
			struct INITIALIZE_PARAMETER
			{
				std::string_view	source;					// - event source / syslog ident (default: execute file name)
				bool				all_types = false;		// - true: writes all log types, false: fatal/error/warning only
			};

public:
			logger_system();
	virtual ~logger_system() noexcept;

public:
			result_code			initialize() { return this->object::Ninitializable::initialize(); }
			result_code			initialize(const INITIALIZE_PARAMETER& _parameter);
	virtual	result_code			initialize(context& _context) override { return this->object::Ninitializable::initialize(_context); }

	[[nodiscard]] const auto&	source() const noexcept { return this->m_source; }

#if defined(_WIN32)
	// - event source (administrator)
	static	bool				register_event_source(std::string_view _source) noexcept;
	static	bool				unregister_event_source(std::string_view _source) noexcept;
#endif

protected:
	virtual	result_code			_process_initializing(context& _context) override;
	virtual	result_code			_process_destroying() noexcept override;
	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) override;

			void				process_write(log_type_t _log_type, std::string_view _text) noexcept;

protected:
			std::string			m_source;
			bool				m_all_types = false;

		#if defined(_WIN32)
			HANDLE				m_handle_event_source = nullptr;
		#endif
};


}
