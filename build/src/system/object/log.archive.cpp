#include "pch.h"

namespace CGDK
{

log_archive::log_archive() : m_capacity(8192), m_index_stored_log(0)
{
}

log_archive::~log_archive() noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_index_stored_log);

	// 1) clear log list
	this->m_archive_log.clear();
}

void log_archive::capacity(size_t _capacity) noexcept
{
	// check)
	if(_capacity < 32)
		return;

	// 1) saturate
	SATURATE(_capacity, 32, 65536);

	// 2) set capacity
	this->m_capacity = _capacity;
}

std::vector<object_ptr<LOG_RECORD>> log_archive::get_stored_log(size_t _position) const
{
	// declare)
	std::vector<object_ptr<LOG_RECORD>>	vector_log_record;

	{
		// lock)
		std::lock_guard lock(this->m_lockable_index_stored_log);

		// check) no log after _position
		if(_position >= this->m_index_stored_log)
			return vector_log_record;

		// 1) serial index of the oldest log kept
		//    (a _position older than it gets every log kept : the older logs were removed)
		const auto index_oldest = this->m_index_stored_log - this->m_archive_log.size();
		if(_position < index_oldest)
			_position = index_oldest;

		// 2) count of logs from _position to the newest
		auto count = this->m_index_stored_log - _position;

		// 3) reserve
		vector_log_record.reserve(count);

		// 4) skip the logs before _position
		auto iter = this->m_archive_log.begin();
		for (auto skip = _position - index_oldest; skip > 0; --skip)
			++iter;

		// 5) copy log list
		for (; count > 0; --count, ++iter)
			vector_log_record.push_back(*iter);
	}

	// return)
	return vector_log_record;
}

void log_archive::process_store_log(std::vector<object_ptr<LOG_RECORD>>&& _vector_plog_record)
{
	// lock)
	std::lock_guard lock(this->m_lockable_index_stored_log);

	// 1) push log
	for (auto& iter:_vector_plog_record)
	{
		// - push_back
		this->m_archive_log.push_back(std::move(iter));

		// - increase the serial index
		++m_index_stored_log;
	}

	// 2) pop the oldest logs exceeding the capacity
	while (this->m_archive_log.size() > m_capacity)
		this->m_archive_log.pop_front();
}


}
