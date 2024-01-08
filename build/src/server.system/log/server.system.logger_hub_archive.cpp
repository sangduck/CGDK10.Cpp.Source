#include "pch.h"

using namespace CGDK;


logger_hub_archive::logger_hub_archive() :
	logger_hub(),
	log_archive(),
	m_is_processing(false)
{
}

logger_hub_archive::logger_hub_archive(const std::string_view _name) :
	logger_hub(_name),
	log_archive(),
	m_is_processing(false)
{
}

logger_hub_archive::~logger_hub_archive() noexcept
{
}

void logger_hub_archive::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_vector_log_record.empty());

	object_ptr<logger_hub_archive> hold(this);

	// check) nullptr이 들어오면 종료 처리를 한다.
	if (_vector_log_record[0] == nullptr)
	{
		// - wait completes
		this->_process_wait_complete();

		// return) 
		return;
	}

	// declare)
	object_ptr<Iexecutor> pexecutor;

	scoped_lock(this->m_lockable_log)
	{
		// check) 
		RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

		// check)
		RETURN_IF(this->m_pexecutor.empty(), );

		// 1) get executor
		pexecutor = this->m_pexecutor;

		// check) 
		if (this->m_is_processing == true)
		{
			// - queuing
			this->m_array_log_queue.insert(this->m_array_log_queue.end(), _vector_log_record.begin(), _vector_log_record.end());

			// check)
			CGASSERT_ERROR(this->m_array_log_queue[0].exist());

			// return)
			return;
		}

		// check)
		CGASSERT_ERROR(this->m_array_log_process.empty());

		// 2) queuing
		this->m_array_log_process.insert(this->m_array_log_process.end(), _vector_log_record.begin(), _vector_log_record.end());

		// 3) processing
		this->m_is_processing = true;
	}

	// check)
	RETURN_IF(pexecutor.empty(), );

	// check)
	CGASSERT_ERROR(this->m_array_log_process[0].exist());

	// 4) async request log
	auto result = pexecutor->post(this);

	// check)
	if (result == false)
	{
		scoped_lock(this->m_lockable_log)
		{
			// - clear process
			this->m_array_log_process.clear();

			// - reslet processing flag
			this->m_is_processing = false;
		}
	}
}

intptr_t logger_hub_archive::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check)
	CGASSERT_ERROR(this->m_is_processing == true);

	// check) 
	CGASSERT_ERROR(this->m_array_log_process.empty() == false);

	object_ptr<logger_hub_archive> hold(this);

	// 1) call 'process_log' of logger_hub
	logger_hub::process_log(this->m_array_log_process);

	// 2) call 'process_log' of log_archive
	log_archive::process_store_log(std::move(this->m_array_log_process));

	// 3) clear
	this->m_array_log_process.clear();

	scoped_lock(this->m_lockable_log)
	{
		// check)
		CGASSERT_ERROR(this->m_is_processing == true);

		// - seap buffer
		this->m_array_log_process.swap(this->m_array_log_queue);

		// check)
		if (this->m_array_log_process.empty())
		{
			// - reset processing flag
			this->m_is_processing = false;

			// return)
			return 0;
		}

		// check)
		CGASSERT_ERROR(this->m_array_log_process.empty() == false);

		// check)
		CGASSERT_ERROR(this->m_array_log_process[0].exist());

		// check)
		RETURN_IF(this->m_pexecutor.empty(), 0);

		// - async request log
		auto result = this->m_pexecutor->post(this);

		// check)
		if (result == false)
		{
			// - clear process
			this->m_array_log_process.clear();

			// - reslet processing flag
			this->m_is_processing = false;
		}
	}

	// return)
	return 0;
}

void logger_hub_archive::_process_wait_complete()
{
	for (;;)
	{
		// check) break if completed
		scoped_lock(this->m_lockable_log)
		{
			if (this->m_is_processing == false)
				break;
		}

		// - sleep
		std::this_thread::sleep_for(1ms);
	}
}

result_code logger_hub_archive::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_HUB_ARCHIVE["sv << name() << "]"sv;

	// declare)
	result_code result;

	// 1) set executor
	scoped_lock(this->m_lockable_log)
	{
		this->m_pexecutor = system_executor::executor();
	}

	try
	{
		// 2) 
		result = this->logger_hub::_process_initializing(_context);
	}
	catch (...)
	{
		// - reset executor
		scoped_lock(this->m_lockable_log)
		{
			this->m_pexecutor.reset();
		}

		// reraise) 
		throw;
	}

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_HUB_ARCHIVE["sv << name() << "]"sv;

	// return)
	return result;
}

result_code logger_hub_archive::_process_destroying() noexcept
{
	// 1) 종료를 기다린다.
	this->_process_wait_complete();

	// 2) call 'logger_hub::_process_destroing'
	return this->Ninitializable::_process_destroying();
}

result_code logger_hub_archive::_process_destroy() noexcept
{
	// 1) call ''
	auto result = this->logger_hub::_process_destroy();

	// 2) reset executor
	scoped_lock(this->m_lockable_log)
	{
		// check)
		CGASSERT_ERROR(this->m_array_log_process.empty());
		CGASSERT_ERROR(this->m_array_log_queue.empty());

		// - reset
		this->m_pexecutor.reset();
	}

	return result;
}
