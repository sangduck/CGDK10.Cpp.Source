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
// C_logEvent
//
// 1. C_logEvent은~
//     Log처리를 위한 것이다.
//
// 2. Log의 내용
//		0) Date/Time	Log가 남은 시간이다.(자동기록)
//		1) Logger		Log를 남기는 자의 이름이다.(자동 등록)
//		2) type			Log의 종류이다. Error, Progress, Warning, Info, ...
//	    3) Level		Log의 중요도이다.
//		4) Source		Log에 남기는 Event의 주체가 되는 것을 의미한다.
//		5) Dest			Log에 남기는 Event의 대상이 되는 것을 의미한다.
//		6) Where		Log에 남기는 Event가 발생하는 객체를 의미한다.
//		7) Function		Log가 남는 Event를 호출한 함수를 의미한다.
//		8) Message		남길 Log Message를 의미한다.
//
//-----------------------------------------------------------------------------
class logger_console : 
	public						Nlogger_async
{
public:
			struct INITIALIZE_PARAMETER
			{
			};

			logger_console();
	virtual ~logger_console() noexcept;

			result_code			initialize() { return this->object::Ninitializable::initialize(); }
			result_code			initialize(const INITIALIZE_PARAMETER&) { return initialize(); }
	virtual	result_code			initialize(context& _context) override { return this->object::Ninitializable::initialize(_context); }

	[[nodiscard]] auto			character_encoding() const noexcept -> eCHARACTER_SET { return this->m_char_encoding; }
			void				character_encoding(eCHARACTER_SET _char_code) noexcept { this->m_char_encoding = _char_code; }

protected:
	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) override;
			void				_process_append_log(const object_ptr<LOG_RECORD>& _plog_record);
			void				_process_write_output();

	// - output of the batch (used on the execution only : reused, the console is written synchronously)
			shared_buffer		m_buf_output;

protected:
	virtual	result_code			_process_initializing(context& _context) override;
	virtual	result_code			_process_destroying() noexcept;

			eCHARACTER_SET		m_char_encoding{ DEFAULT_CONSOLE_CHARACTER_SET };

		#if defined(_WIN32)
			HANDLE				m_console_handle = INVALID_HANDLE_VALUE;
			bool				m_is_console{ true };	// - false : redirected to a pipe or a file (WriteFile)
		#endif

};


}