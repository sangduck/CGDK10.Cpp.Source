#include "pch.h"

namespace CGDK
{

log_archive::log_archive() : m_capacity(8192), m_index_stored_log(0)
{
}

log_archive::~log_archive() noexcept
{
	scoped_lock(m_lockable_index_stored_log)
	{
		m_archive_log.clear();
	}
}

void log_archive::capacity(size_t _capacity) noexcept
{
	// check)
	RETURN_IF(_capacity < 32);

	// 1) saturate
	SATURATE(_capacity, 32, 65536);

	// 2) set capacity
	m_capacity = _capacity;
}

std::vector<object_ptr<LOG_RECORD>> log_archive::get_stored_log(size_t _position) const
{
	// declare)
	std::vector<object_ptr<LOG_RECORD>>	vector_log_record;

	scoped_lock(m_lockable_index_stored_log)
	{
		// check) _position이 m_index_stored_log보다 크면 안됀다.
		RETURN_IF(_position >= m_index_stored_log, vector_log_record);

		// check) _position이 m_index_stored_log보다 크면 안됀다.
		RETURN_IF(_position < m_index_stored_log - this->m_archive_log.size(), vector_log_record);

		// 1) 되돌려 준다.
		auto count = m_index_stored_log - _position;

		// 2) reserve
		vector_log_record.reserve(count);

		// 3) copy log list
		for (auto iter = this->m_archive_log.begin(); count > 0; --count, ++iter)
		{
			vector_log_record.push_back(*iter);
		}
	}

	// 4) 결과를 복사한다.
	return vector_log_record;
}

void log_archive::process_store_log(std::vector<object_ptr<LOG_RECORD>>&& _vector_plog_record)
{
	scoped_lock(m_lockable_index_stored_log)
	{
		// 1) pop log if full
		if (this->m_archive_log.size() >= m_capacity)
		{
			this->m_archive_log.pop_front();
		}

		// 2) push log
		for (auto& iter:_vector_plog_record)
		{
			// - push_back
			this->m_archive_log.push_back(std::move(iter));

			// - m_index_stored_log증가.
			++m_index_stored_log;
		}
	}
}


}