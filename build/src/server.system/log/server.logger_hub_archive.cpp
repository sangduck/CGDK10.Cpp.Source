#include "pch.h"

using namespace CGDK;


logger_hub_archive::logger_hub_archive()
{
}

logger_hub_archive::~logger_hub_archive() noexcept
{
	// - the records left in the queue
	try
	{
		this->_process_drop_queued();
	}
	catch (...)
	{
	}
}

void logger_hub_archive::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// - the caller keeps this logger alive while trace() (no reference count for every log)
	//   (a posted execution keeps it alive while records are queued)

	// check) a null record : nothing to do (the logs are just queued, they are waited for only on destroy)
	if (_plog_record.empty())
		return;

	// check)
	RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

	// 1) count first (the execution never takes more records than counted)
	const bool is_first = (this->m_count_queued.fetch_add(1, std::memory_order_acq_rel) == 0);

	// 2) push to the stack (lock-free)
	//    - the reference of the record is moved to the queue
	auto precord = _plog_record.get();
	_plog_record.detach();

	precord->_pnext_queued = this->m_phead_queued.load(std::memory_order_relaxed);
	while (!this->m_phead_queued.compare_exchange_weak(precord->_pnext_queued, precord, std::memory_order_release, std::memory_order_relaxed))
	{
	}

	// 3) the first record after the queue was empty posts the execution
	if (is_first)
		this->_process_post();
}

void logger_hub_archive::_process_post()
{
	// - called by the only one which owns the queue (m_count_queued isn't 0)

	try
	{
		// declare)
		object_ptr<Iexecutor> pexecutor;

		// 1) get executor (the default executor if not set)
		SCOPED_LOCK(this->m_lockable_log)
		{
			if (this->m_pexecutor.empty())
				this->m_pexecutor = system_executor::executor();

			pexecutor = this->m_pexecutor;
		}

		// 2) async request log
		if (pexecutor.exist() && pexecutor->post(this) == true)
			return;
	}
	catch (std::exception& _e)
	{
		// log) (queued : dropped below)
		LOG_ERROR << "@ (excp) fail to async request log to executor("sv << _e.what() << "), logger: '"sv << this->name() << "'"sv;
	}
	catch (...)
	{
		// log) (queued : dropped below)
		LOG_ERROR << "@ (excp) fail to async request log to executor by unknowned exception, logger: '"sv << this->name() << "'"sv;
	}

	// 3) fail to post : drop the queued records (or no execution takes them)
	this->_process_drop_queued();
}

size_t logger_hub_archive::_process_take_queued()
{
	// - called by the only one which owns the queue (m_count_queued isn't 0)

	// 1) take the whole stack
	//    - a producer counts before it pushes : waits for the record counted
	auto phead = this->m_phead_queued.exchange(nullptr, std::memory_order_acquire);
	while (phead == nullptr)
	{
		std::this_thread::yield();
		phead = this->m_phead_queued.exchange(nullptr, std::memory_order_acquire);
	}

	// 2) count
	size_t count = 0;
	for (auto p = phead; p != nullptr; p = p->_pnext_queued)
		++count;

	// 3) the stack is LIFO : stored in the reversed order (FIFO)
	this->m_array_log_process.resize(count);

	auto index = count;
	for (auto p = phead; p != nullptr;)
	{
		auto pnext = p->_pnext_queued;
		p->_pnext_queued = nullptr;

		// - the reference of the queue is moved to m_array_log_process
		this->m_array_log_process[--index] = p;
		p->release();

		p = pnext;
	}

	// return)
	return count;
}

void logger_hub_archive::_process_drop_queued()
{
	// - called by the only one which owns the queue (or on destruction)
	while (this->m_count_queued.load(std::memory_order_acquire) != 0)
	{
		// 1) take and release
		const auto count = this->_process_take_queued();
		this->m_array_log_process.clear();

		// check) done
		if (this->m_count_queued.fetch_sub(count, std::memory_order_acq_rel) == count)
			break;
	}
}

intptr_t logger_hub_archive::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	// check)
	CGASSERT_ERROR(this->m_count_queued.load() != 0);

	object_ptr<logger_hub_archive> hold(this);

	// 1) take the queued records (FIFO)
	const auto count = this->_process_take_queued();

	// 2) pass them to the children (no state check : the records queued before destroy are passed while destroying)
	//    - the asynchronous children (file, console) just queue them and write them at once on their execution
	for (auto& iter : this->m_array_log_process)
		logger_hub::_process_forward(object_ptr<LOG_RECORD>(iter));

	// 3) call 'process_store_log' of log_archive
	log_archive::process_store_log(std::move(this->m_array_log_process));

	// 4) clear
	this->m_array_log_process.clear();

	// check) done (no record was queued meanwhile)
	if (this->m_count_queued.fetch_sub(count, std::memory_order_acq_rel) == count)
		return 0;

	// 5) more records : the next execution
	//    - only this execution posts it (the count isn't 0 : no producer posts)
	this->_process_post();

	// return)
	return 0;
}

void logger_hub_archive::_process_wait_complete()
{
	// - waits until every queued record is processed
	while (this->m_count_queued.load(std::memory_order_acquire) != 0)
	{
		std::this_thread::sleep_for(1ms);
	}
}

result_code logger_hub_archive::_process_initializing(context& _context)
{
	// return)
	return this->logger_hub::_process_initializing(_context);
}

result_code logger_hub_archive::_process_destroying() noexcept
{
	// 1) wait for the completion of the queued records
	this->_process_wait_complete();

	// 2) call 'Ninitializable::_process_destroying'
	return this->Ninitializable::_process_destroying();
}

result_code logger_hub_archive::_process_destroy() noexcept
{
	// 1) call 'logger_hub::_process_destroy'
	auto result = this->logger_hub::_process_destroy();

	// 2) reset executor
	SCOPED_LOCK(this->m_lockable_log)
	{
		// - a log may be queued while destroying (after the state check of process_log) :
		//   its execution passes it to no child (the children are detached) and releases it
		// - reset
		this->m_pexecutor.reset();
	}

	return result;
}
