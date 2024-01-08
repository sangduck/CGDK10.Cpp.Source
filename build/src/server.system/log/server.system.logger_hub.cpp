#include "pch.h"

using namespace CGDK;


logger_hub::logger_hub() noexcept
{
}

logger_hub::logger_hub(const std::string_view _name) :
	Nlogger(_name)
{
}

logger_hub::~logger_hub() noexcept
{
}

bool logger_hub::register_log_targetable(object_ptr<Ilogger>&& _plog_target)
{
	return this->process_attach(std::move(_plog_target));
}

bool logger_hub::unregister_log_targetable(Ilogger* _plog_target) noexcept
{
	return this->process_detach(_plog_target) != 0;
}
int logger_hub::reset_log_taretable() noexcept
{
	scoped_unique_lock(this->m_lockable_log)
	{
		return this->_process_detach_all();
	}
}

void logger_hub::process_log(const std::vector<object_ptr<LOG_RECORD>>& _vector_log_record)
{
	// check) _plog_record must be not null
	RETURN_IF(_vector_log_record.empty(),);

	// declare) 
	const std::vector<object_ptr<LOG_RECORD>>* pvector_log_record = nullptr;
	std::vector<object_ptr<LOG_RECORD>> temp_vector_log_record;

	// declare) 
	object_ptr<Ilog_filter>plog_filter;

	scoped_lock(this->m_lockable_log)
	{
		plog_filter = this->m_plog_filter;
	}

	// check) filtering
	//        - filter가 존재할 경우 filtering을 수행하고 그 결과가 false가
	//          나올 경우 Log를 남기지 않는다.
	//        - filter가 설정되어 있지 않을 경우 그냥 통과한다.
	if (plog_filter.empty())
	{
		for (auto& iter : _vector_log_record)
		{
			// check)
			CONTINUE_IF(iter.empty());

			// statistics) 
			this->process_count_log(iter->type);
		}

		pvector_log_record = &_vector_log_record;
	}
	else
	{
		// - reserve 
		temp_vector_log_record.reserve(_vector_log_record.size());

		// 1) insert LOG_RECORD
		for (auto& iter : _vector_log_record)
		{
			// check)
			CONTINUE_IF(iter.empty());

			// check) 
			CONTINUE_IF(iter->buf_message.empty());

			// check)
			CONTINUE_IF(plog_filter->process_filtering(iter) == false);

			// statistics) 
			this->process_count_log(iter->type);

			// - queuing 
			temp_vector_log_record.push_back(iter);
		}

		// check)
		RETURN_IF(temp_vector_log_record.empty());

		// - select temp
		pvector_log_record = &temp_vector_log_record;
	}

	// declare) 
	static_vector<object_ptr<Ilogger>, MAX_MEDIATOR> list_logger;

	// 4) copy logger list
	scoped_lock(this->m_lockable_log)
	{
		// check) 
		RETURN_IF(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING);

		for (auto& iter : this->m_container)
		{
			list_logger.push_back(iter);
		}
	}

	// check)
	CGASSERT_ERROR(pvector_log_record != nullptr)

	// 5) trace
	for (auto& iter : list_logger)
	{
		iter->trace(*pvector_log_record);
	}
}

result_code logger_hub::_process_initializing(context& _context)
{
	// trace) 
	LOG_PROGRESS << "@ <BEGIN> initializing LOGGER_HUB["sv << name() << "]"sv;

	// declare)
	result_code result;

	try
	{
		// 1) ...
		result = this->Ninitializable::_process_initializing(_context);

		// 2) get params
		auto pcontext = _context["child"sv];;

		if(pcontext.is_valid())
		{
			// declare) 
			std::vector<own_ptr<Nlogger>> vector_log_targetable;

			// - create log
			for(auto& iter: pcontext)
			{
				std::string str_log_type = iter.second.find("type"sv);

				if (str_log_type == "file")
				{
					// - create log
					auto plog = make_own<logger_file>(iter.first);

					// - initialize
					plog->initialize(iter.second);
				}
				else if (str_log_type == "socket"sv)
				{
					// - create log
					auto plog = make_own<logger_file>(iter.first);

					// - initialize
					plog->initialize(iter.second);
				}
				else if (str_log_type == "slack"sv)
				{
					// - create log
					auto plog = make_own<logger_file>(iter.first);

					// - initialize
					plog->initialize(iter.second);
				}
			}

			// - register log objects
			for (auto& iter : vector_log_targetable)
			{
				this->register_log_targetable(iter);
			}
		}

		// - reset log count
		this->reset_log_count();
	}
	catch(...)
	{
		// log) 
		LOG_FATAL << "@ <TERMINATE> exception complete logger_hub['"sv << name() << "']"sv;

		// Reraise)
		throw;
	}

	// trace) Event Manager를 생성한다.
	LOG_PROGRESS << "@ <END> initializing LOGGER_HUB["sv << name() << "]"sv;

	// return)
	return result;
}

result_code logger_hub::_process_destroy() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	reset_default_logger(this);

	scoped_lock(this->m_lockable_log)
	{
		// 2) detach from parent logger
		this->detach_from_parent();

		// 3) detach all child logger
		this->_process_detach_all();
	}

	// 4) Call Parent's _process_destroy function
	auto result = this->Ninitializable::_process_destroy();

	// return)
	return result;
}

bool logger_hub::_process_attach(object_ptr<Ilogger>&& _child)
{
	// check) nullptr이면 안됀다.
	CGASSERT_ERROR(_child.exist());

	// check) _child가 nullptr이면 안됀다.
	RETURN_IF(_child.empty(), false);

	// check) No Self!
	CGASSERT_ERROR(_child != dynamic_cast<Ilogger*>(this));

	// check)
	THROW_IF(_child == dynamic_cast<Ilogger*>(this), throwable::failure(eRESULT::INVALID_ARGUMENT));

	// 1) 이미 존재하는지 찾는다.
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) 이미 존재하면 그냥 ASSERT 발생!
	CGASSERT_ERROR(iter_find == this->m_container.end());

	// check) 이미 존재하면 그냥 false를 리턴한다.
	RETURN_IF(iter_find != this->m_container.end(), false);

	// 2) set parent
	_child->m_pparent = this;

	// 3) 추가하기
	try
	{
		this->m_container.emplace_back(std::move(_child));
	}
	catch (...)
	{
		// - rollback
		_child->m_pparent = nullptr;

		// return) 
		throw;
	}

	// return) 
	return true;
}

bool logger_hub::_process_detach(Ilogger* _child) noexcept
{
	// check) nullptr일 경우 모든 child를 떼낸다.
	if (_child == nullptr)
	{
		// - detach all!
		return this->clear() != 0;
	}

	// check) 
	RETURN_IF(_child->m_pparent != this, false);

	// 1) Check aleady exists
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) find?
	RETURN_IF(iter_find == this->m_container.end(), false);

	// 2) Delete found item
	this->m_container.erase(iter_find);

	// 3) set parent
	_child->m_pparent = nullptr;

	// return)
	return true;
}

bool logger_hub::process_attach(object_ptr<Ilogger>&& _child)
{
	scoped_lock(this->m_lockable_log)
	{
		return this->_process_attach(std::move(_child));
	}
}

bool logger_hub::process_detach(Ilogger* _child) noexcept
{
	scoped_lock(this->m_lockable_log)
	{
		return this->_process_detach(_child);
	}
}

int logger_hub::clear() noexcept
{
	scoped_unique_lock(this->m_lockable_log)
	{
		return this->_process_detach_all();
	}
}

int logger_hub::_process_detach_all() noexcept
{
	// 1) copy
	auto vector_temp = this->m_container;

	// 2) count
	int count = static_cast<int>(vector_temp.size());

	// 2) detach all
	for (auto& iter : vector_temp)
	{
		// chcek) 
		CONTINUE_IF(iter.empty());

		// - detach
		this->_process_detach(iter);
	}

	// return)
	return count;
}