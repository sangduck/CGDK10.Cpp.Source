#include "pch.h"

namespace CGDK
{
	void Ilogger::trace(object_ptr<LOG_RECORD>&& _plog_record)
	{
		// - a record isn't changed once it is traced (the loggers read it without lock)
		if (_plog_record.exist())
			_plog_record->_set_traced();

		this->process_log(std::move(_plog_record));
	}
	void Ilogger::set_filter(Ilog_filter* _plog_filter)
	{
		// lock)
		std::lock_guard lock(this->m_lockable_filter);

		// 1) the old filter is kept until this logger is destroyed
		//    (the loggers call the filter by the raw pointer without lock or reference : a logger may call
		//     the old one after it is changed, so it must not be released here (released : access violation))
		if (this->m_plog_filter.exist())
			this->m_vector_filter_old.push_back(std::move(this->m_plog_filter));

		// 2) set
		this->m_plog_filter = _plog_filter;
		this->m_pfilter_now.store(_plog_filter, std::memory_order_release);
	}

	object_ptr<Ilog_filter> Ilogger::get_filter() const
	{
		// lock)
		std::lock_guard lock(this->m_lockable_filter);

		// return)
		return this->m_plog_filter;
	}

	void Ilogger::detach_from_parent() noexcept
	{
		// 1) get parent logger
		auto pparent = this->m_pparent;

		// 2) detach from parents
		if (pparent != nullptr)
		{
			pparent->detach(this);
		}
	}

	void Ilogger::reset_log_count() noexcept
	{
		// lock) (for the readers)
		std::lock_guard lock(this->m_lockable_count_log);

		// 1) reset all m_count_log
		for (auto& iter : this->m_count_log)
			iter.store(0, std::memory_order_relaxed);

		// 2) reset total
		this->m_count_log_total.store(0, std::memory_order_relaxed);
	}

	std::array<size_t, MAX_LOG_TYPE> Ilogger::_log_count() const noexcept
	{
		std::array<size_t, MAX_LOG_TYPE> result;
		for (size_t i = 0; i < MAX_LOG_TYPE; ++i)
			result[i] = this->m_count_log[i].load(std::memory_order_relaxed);
		return result;
	}

	void Ilogger::process_count_log(log_type_t _type) noexcept
	{
		// declare)
		uint32_t type = static_cast<uint32_t>(_type) & 0xffff;

		// 1) total count
		this->m_count_log_total.fetch_add(1, std::memory_order_relaxed);

		// 2) count by log type
		if (type >= (uint32_t)log_type_t::info && type < (uint32_t)log_type_t::max_)
			this->m_count_log[type].fetch_add(1, std::memory_order_relaxed);
	}


}
