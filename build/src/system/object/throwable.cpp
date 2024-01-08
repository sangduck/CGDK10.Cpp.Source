#include "pch.h"


namespace CGDK
{

object_ptr<LOG_RECORD> Nthrowable::get_log_record() const noexcept
{
	// 1) log record를 생성한다.
	auto plog_record = make_object<LOG_RECORD>();

	// 2) set log record
	plog_record->type = log_type_t::fatal;
	plog_record->level = log_level_t::higher();
	plog_record->result = reason();
	plog_record->origin = log_origin_t{ static_cast<uint64_t>(m_param)};
	plog_record->time_occure = chrono::time::utc::now();

	// 3) log message
	plog_record->set_message(m_description);

	// return)
	return	plog_record;
}

object_ptr<LOG_RECORD> _throwable_ex(const Nthrowable& _throwable, const Ndescription_ex& _pdescription) noexcept
{
	// 1) log record를 생성한다.
	auto plog_record = make_object<LOG_RECORD>();

	// 2) set log record
	plog_record->type = log_type_t::fatal;
	plog_record->level = log_level_t::higher();
	plog_record->result = _throwable.reason();
	plog_record->origin = log_origin_t{static_cast<uint64_t>(_throwable.m_param)};
	plog_record->time_occure = chrono::time::utc::now();

	// 3) log message
	if (_throwable.m_description.empty() == false)
	{
		plog_record->set_message(_throwable.m_description);
	}

	// 4) description ex를 추가한다.
	for (auto& iter : _pdescription.m_description_ex)
	{
		plog_record->add_message(iter);
	}

	// return)
	return plog_record;
}

void Ndescription_ex::add_description(std::string_view _description)
{
	this->m_description_ex.emplace_back(_description);
}

void Ndescription_ex::add_description(std::wstring_view _description)
{
	// 1) alloc temporary shared_buffer
	shared_buffer temp_buffer = alloc_shared_buffer(_description.size() + 1);

	// 2) convert string을 한다.
	auto temp_str = encode_string(temp_buffer.data(), _description);

	// 3) push_back~
	this->m_description_ex.emplace_back(temp_str);
}

object_ptr<LOG_RECORD> Nthrowable_ex::get_log_record() const noexcept
{
	return _throwable_ex(*this, *this);
}

object_ptr<LOG_RECORD> throwable::failure_ex::get_log_record() const noexcept
{
	return _throwable_ex(*this, *this);
}

object_ptr<LOG_RECORD> throwable::critical_ex::get_log_record() const noexcept
{
	return _throwable_ex(*this, *this);
}


}