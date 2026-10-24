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
// 1. C_logEvent��~
//     Logó���� ���� ���̴�.
//
// 2. Log�� ����
//	  0) Date/Time		Log�� ���� �ð��̴�.(�ڵ����)
//	  1) Logger			Log�� ����� ���� �̸��̴�.(�ڵ� ���)
//	  2) type			Log�� �����̴�. Error, Progress, Warning, Info, ...
//	  3) Level			Log�� �߿䵵�̴�.
//	  4) Source			Log�� ����� Event�� ��ü�� �Ǵ� ���� �ǹ��Ѵ�.
//	  5) Dest			Log�� ����� Event�� ����� �Ǵ� ���� �ǹ��Ѵ�.
//	  6) Where			Log�� ����� Event�� �߻��ϴ� ��ü�� �ǹ��Ѵ�.
//	  7) Function		Log�� ���� Event�� ȣ���� �Լ��� �ǹ��Ѵ�.
//	  8) Message		���� Log Message�� �ǹ��Ѵ�.
//
// 3. Initializing Parameters
//    1) ���� �̸�		'filename'			���ϸ�  ex) "log_file ({}{}[}-{}{}{}.log"
//    2) ���� �ð�		'recreation'		����� ���� (true, false)
//    3) ����� ����	'interval'			1�д����� ����� �ð� 
//    4) ���� �ð�		'origin_time_type'	������� ���� �ð� ���� ("now", "minute_begin", "hour_begin", "day_begin", "week_begin", "month_begin", "year_begin", "specified", "default")
//    5) ���� �ð�		'origin_time'		���� �ð� (origin_time_type�� "specified"�� ��� ���� �ð�
//    6) ���� �ð�		'create_new'		���� ���� ���� (true, false)
//    7) ���� �ð�		'char_encoding'			���� ���ڵ� ("multibyte", "utf8", "utf16", "utf32", "ucs16", "ucs32", "default")
//
//-----------------------------------------------------------------------------
class logger_file : 
	public						Nlogger_async
{
public:
			struct INITIALIZE_PARAMETER
			{
				std::string_view				filename{};
				std::string_view				path{};
				std::string_view				backup_path{};
				chrono::time::duration			recreation_interval = chrono::time::duration::zero();
				eORIGIN_TIME_TYPE				origin_time_type = eORIGIN_TIME_TYPE::DEFAULT;
				chrono::time::utc::time_point	begin_time = (chrono::time::utc::time_point::min)();
				uint64_t						limit_bytes{ 0 };
				uint64_t						limit_lines{ 0 };
				eCHARACTER_SET					char_code = _DEFAULT_CHAR_CHARACTER_SET;
				eEOL_TYPE						eol_type = _DEFAULT_FILE_EOL_TYPE;
				bool							is_instant_open{ false };		// - opens the file for the writes and closes it when no write for instant_close_delay
				std::chrono::milliseconds		instant_close_delay{ 100 };		// - 0 : closes after every write
			};

			logger_file();
	virtual ~logger_file() noexcept;

			result_code			initialize() { return this->object::Ninitializable::initialize(); }
			result_code			initialize(const INITIALIZE_PARAMETER& _initialize_param);
	virtual	result_code			initialize(context& _context) override	{ return this->object::Ninitializable::initialize(_context); }

	[[nodiscard]] auto			character_encoding() const noexcept -> eCHARACTER_SET { return this->m_char_encoding; }
			void				character_encoding(eCHARACTER_SET _char_code) noexcept { this->m_char_encoding = _char_code; }

protected:
	virtual	result_code			_process_initializing(context& _context) override;
	virtual	result_code			_process_destroying() noexcept;

	virtual void				_process_recreate_file();
	static	void				_process_append_EOL(shared_buffer& _buf_target, eEOL_TYPE _eol_type);
	static	bool				_process_get_parameter_enabled(context& _context);
	static	std::string			_process_get_parameter_path(context& _context);
	static	std::string			_process_get_parameter_path_backup(context& _context);
	static	std::string			_process_get_parameter_filename(context& _context);
	static	auto				_process_get_parameter_interval(context& _context) -> chrono::time::duration;
	static	eORIGIN_TIME_TYPE	_process_get_parameter_origin_time_type(context& _context);
	static	auto				_process_get_parameter_origin_time(context& _context, chrono::time::utc::time_point _time_now, eORIGIN_TIME_TYPE _origin_time_type, std::string _filename) -> chrono::time::utc::time_point;
	static	uint64_t			_process_get_parameter_recreation_bytes_limit(context& _context);
	static	uint64_t			_process_get_parameter_recreation_lines_limit(context& _context);
	static	bool				_process_get_parameter_create_new(context& _context);
	static	eCHARACTER_SET		_process_get_parameter_char_encoding(context& _context);
	static	int					_process_get_parameter_filter(context& _context);
	static	bool				_process_get_parameter_is_instant_open(context& _context);
	static	auto				_process_get_parameter_instant_close_delay(context& _context) -> std::chrono::milliseconds;

	virtual	void				process_log_batch(std::vector<object_ptr<LOG_RECORD>>& _vector_plog_record) override;
			bool				_process_write_text();
			void				process_log_write(shared_buffer& _buf_output, const object_ptr<LOG_RECORD>& _log);

	// - file : written by this logger itself (on its execution : synchronous, the callers never wait)
			std::mutex			m_lockable_file;
			FILE_HANDLE			m_handle_file{ INVALID_FILE_HANDLE };
			std::string			m_full_path_now;
			eFILE_OPERATION		m_file_operation{ eFILE_OPERATION::APPEND };
			std::atomic<bool>	m_is_file_enabled{ false };	// - read without lock by the batch (checked again with the lock)
			bool				m_is_write_failed{ false };	// - the error is logged only when the writing fails first
			chrono::tick::time_point m_tick_last_write{};
			bool				m_is_close_scheduled{ false };
			bool				_process_open_file();
			void				_process_close_file() noexcept;
			void				_process_post_check_close(chrono::tick::duration _delay);
			void				_process_check_close();
			eCHARACTER_SET		m_char_encoding{ _DEFAULT_CHAR_CHARACTER_SET };
			eEOL_TYPE			m_EOL_type{ _DEFAULT_FILE_EOL_TYPE };

			std::string			m_path;
			std::string			m_path_backup;
			std::string			m_filename;

			chrono::time::duration m_recreation_duration{};
			chrono::time::utc::time_point m_recreation_last{};
			std::atomic<uint64_t> m_recreation_bytes_now{ 0 };	// atomic for lock-free reads
			uint64_t			m_recreation_bytes_limit{ 0 };
			std::atomic<uint64_t> m_recreation_lines_now{ 0 };	// atomic for lock-free reads
			uint64_t			m_recreation_lines_limit{ 0 };

	// - instant open (kept for the recreation of the file)
			bool				m_is_instant_open{ false };
			std::chrono::milliseconds m_instant_close_delay{ 100 };

	// - text buffer (used on the execution only)
	//   the logs are appended until it is full, then written at once (reused : the write is synchronous)
			shared_buffer		m_buf_text;
			uint64_t			m_lines_text{ 0 };
};


}