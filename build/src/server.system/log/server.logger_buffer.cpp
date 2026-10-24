#include "pch.h"

using namespace CGDK;


logger_buffer::logger_buffer()
{
}

logger_buffer::~logger_buffer() noexcept
{
	this->destroy();
}

void logger_buffer::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_plog_record.empty());

	try
	{
		// declare) 
		size_t required_buffer_size = 0;

		// 1) log_records들의 크기를 얻는다.
		if (auto& iter = _plog_record; true)
		{
			// check)
			RETURN_IF(iter.empty());

			required_buffer_size += get_size_of(*iter);
		}

		// 2) 버퍼 남은 크기가 모자라면 늘린다.
		SCOPED_LOCK(this->m_lockable_buffer_log)
		{
			if (this->m_buffer_log.get_remained_size() < required_buffer_size)
			{
				// - 새로운 buffer의 크기를 얻는다.
				auto size_new = this->m_buffer_log.size() + required_buffer_size;

				// - 새로운 buffer를 할당받는다.
				auto buffer_new = alloc_shared_buffer(size_new);

				// check)
				CGASSERT_ERROR(this->m_buffer_log.size() >= 12) // 기본 12byte가 있으므로 무조건 12byte 이상이다.

					// - copy
					if (this->m_buffer_log.size() != 0)
					{
						buffer_new.append(this->m_buffer_log.data(), this->m_buffer_log.size());
					}

				// - buffer 복사
				this->m_buffer_log = std::move(buffer_new);
			}

			// 3) copy
			buffer buffer_log = this->m_buffer_log;

			// declare)
			int count = buffer_log.front<COUNT_T>(8);

			// 5) append LOG_RECORD
			if (auto& iter = _plog_record; true)
			{
				// check)
				RETURN_IF(iter.empty());

				// check) message must be not empty
				RETURN_IF(iter->buf_message.empty());

				// - filtering
				if (auto pfilter = this->_get_filter(); pfilter != nullptr)
				{
					RETURN_IF(pfilter->process_filtering(iter) == false);
				}

				// statistics) 
				this->process_count_log(iter->type);

				// - append log_record
				buffer_log.append(iter);

				// - increase count
				++count;
			}

			// 6) apply
			this->m_buffer_log = buffer_log;
			this->m_buffer_log.front<COUNT_T>(8) = count;
			this->m_buffer_log.front<uint32_t>() = this->m_buffer_log.size<uint32_t>();
		}
	}
	catch (std::exception& _e)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing log to buffer("sv << _e.what() << "), logger: '"sv << this->name() << "'";
	}
	catch (...)
	{
		// log) 
		LOG_ERROR << "@ (excp) fail to writing by unknowned exception on writing log to buffer, logger: '"sv << this->name() << "'";
	}
}

result_code logger_buffer::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_BUFFER["sv << name() << "]"sv;

	// 1) log 정보를 reset한다.
	reset_log_count();

	// 2) ...
	// 1) on_starting 함수를 호출한다.
	on_initializing(_context);

	// declare)
	size_t reserve_buffer_size = 65536;

	// 2) connect를 시도한다.
	try
	{
		// declare)
		INITIALIZE_PARAMETER parameter;

		if (_context.type() == context::value_type_t::buffer_)
		{
			parameter = _context.v.value_buffer.front<INITIALIZE_PARAMETER>();
		}
		else
		{
			// - 'reconnection' value
			auto context_find = _context["reserve_buffer_size"sv];

			if (context_find.is_invalid())
			{
				reserve_buffer_size = (size_t)context_find;

				// trace)
				LOG_INFO << "  + parameter['reserve_buffer_size']: "sv << reserve_buffer_size << "bytes"sv;
			}
		}

		if (reserve_buffer_size < 64)
		{
			reserve_buffer_size = 64;
		}

		// 3) ....
		auto buffer_log = alloc_shared_buffer(reserve_buffer_size);

		buffer_log.append<uint32_t>();
		buffer_log.append<uint32_t>(eMESSAGE::SYSTEM::LOG_);
		buffer_log.append<COUNT_T>(0);
		buffer_log.front<uint32_t>() = buffer_log.size<uint32_t>();

		// 4) set
		this->m_buffer_log = std::move(buffer_log);
	}
	catch (std::exception& e)
	{
		// log) prepare receive에 실패했다!
		LOG_FATAL << "@ (excp) logger_buffer. fatal complete '"sv << name() << __CGFUNCTION__ ""sv;

		// - call 'exception handler'
		if (throwable::execute_exception_handler(this, e, __FUNCTION__) == false)
		{
			// Reraise)
			throw;
		}
	}

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_BUFFER["sv << name() << "]"sv;

	// return)
	return eRESULT::SUCCESS;
}

result_code logger_buffer::_process_destroying() noexcept
{
	// 1) default tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	// 2) detach from parent logger
	detach_from_parent();

	// 3) on_destroying 함수를 호출한다.
	try
	{
		on_destroying();
	}
	catch(...)
	{
		// Log) 
		LOG_ERROR << "@ (excp) exception on 'on_destroying' function call ['"sv << name() << "']"sv;
	}

	// 4) buffer를 clear한다.
	SCOPED_LOCK(this->m_lockable_buffer_log)
	{
		this->m_buffer_log.clear();
	}

	// return)
	return eRESULT::SUCCESS;
}
