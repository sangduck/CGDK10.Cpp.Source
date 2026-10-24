#include "pch.h"

using namespace CGDK;


logger_hub::logger_hub() noexcept
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
	SCOPED_LOCK(this->m_lockable_log)
	{
		return this->_process_detach_all();
	}
}

void logger_hub::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check) a null record : nothing to do
	if (_plog_record.empty())
		return;

	// check)
	if(this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING)
		return;

	// 1) filtering, statistics and passing to the children
	this->_process_forward(std::move(_plog_record));
}

void logger_hub::_process_forward(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check)
	if (_plog_record.empty())
		return;

	// 1) filtering
	//  - read without lock or reference (Ilogger keeps every filter set until this logger is destroyed)
	if (auto pfilter = this->_get_filter(); pfilter != nullptr)
	{
		// check)
		if(_plog_record->buf_message.empty())
			return;

		// - filtering
		if(pfilter->process_filtering(_plog_record) == false)
			return;
	}

	// statistics)
	this->process_count_log(_plog_record->type);

	// 2) children (a snapshot : lock-free raw pointer load, no refcount)
	const auto* psnapshot = this->m_psnapshot.load(std::memory_order_acquire);

	// check)
	if(psnapshot == nullptr || psnapshot->empty())
		return;

	// 3) trace (the last one gets the record by move)
	const auto& list_logger = *psnapshot;
	const auto last = list_logger.size() - 1;
	for (size_t i = 0; i < last; ++i)
		list_logger[i]->trace(object_ptr<LOG_RECORD>(_plog_record));
	list_logger[last]->trace(std::move(_plog_record));
}
void logger_hub::_update_snapshot() noexcept
{
	// - called only under m_lockable_log (attach/detach : not a hot path)
	try
	{
		// - build a new snapshot, keep it alive (readers may still hold the previous raw pointer),
		//   then publish its raw pointer for the lock-free readers
		auto pnew = std::make_shared<const snapshot_t>(this->m_container);
		this->m_snapshot_keepalive.push_back(pnew);
		this->m_psnapshot.store(pnew.get(), std::memory_order_release);
	}
	catch (...)
	{
		// - out of memory : the old snapshot is kept
	}
}
result_code logger_hub::_process_initializing(context& _context)
{
	// declare)
	result_code result;

	try
	{
		// 1) ...
		result = this->Ninitializable::_process_initializing(_context);

		// 2) the children created by the previous setting (initialized again) : unregistered and destroyed
		//    (they were added again : every log was written twice)
		{
			std::vector<own_ptr<Nlogger>> vector_child_old;
			vector_child_old.swap(this->m_vector_child_owned);

			for (auto& iter : vector_child_old)
				this->unregister_log_targetable(iter.get());
		}

		// 3) get params
		auto& context_child = _context["child"sv];;

		if(context_child.is_valid())
		{
			// - create the children by "type" (create_logger : "file", "console", "slack"(server.log.slack.h), ...)
			//   (owned by this hub : destroyed with it)
			for (auto& iter : context_child)
			{
				std::string str_log_type = iter.second.find("type"sv);

				auto plog = create_logger(str_log_type);

				// check) unknown type
				if (plog.empty())
				{
					LOG_ERROR << "@ (err) logger_hub : unknown logger type '"sv << str_log_type << "' (child '"sv << iter.first << "')"sv;
					continue;
				}

				// - set name and initialize
				plog->name(iter.first);
				plog->initialize(iter.second);

				// - register
				this->register_log_targetable(object_ptr<Ilogger>(plog.get()));
				this->m_vector_child_owned.push_back(std::move(plog));
			}
		}

		// - reset log count
		this->reset_log_count();
	}
	catch(...)
	{
		// Reraise)
		throw;
	}

	// return)
	return result;
}

result_code logger_hub::_process_destroy() noexcept
{
	// 1) Default Tracer로 설정되어 있다면 제거한다.
	auto pholder = reset_default_logger(this);

	SCOPED_LOCK(this->m_lockable_log)
	{
		// 2) detach from parent logger
		this->detach_from_parent();

		// 3) detach all child logger
		this->_process_detach_all();
	}

	// - the children created by the setting ("child") are destroyed
	std::vector<own_ptr<Nlogger>> vector_child_owned;
	SCOPED_LOCK(this->m_lockable_log)
	{
		vector_child_owned.swap(this->m_vector_child_owned);

		// - release the snapshots (no logger is forwarding any more : destroy is single-threaded)
		this->m_psnapshot.store(nullptr, std::memory_order_release);
		this->m_snapshot_keepalive.clear();
	}
	vector_child_owned.clear();

	// 4) Call Parent's _process_destroy function
	auto result = this->Ninitializable::_process_destroy();

	// return)
	return result;
}

bool logger_hub::_process_attach(object_ptr<Ilogger>&& _child)
{
	// check) nullptr이면 안 된다.
	CGASSERT_ERROR(_child.exist());

	// check) _child가 nullptr이면 안 된다.
	if(_child.empty())
		return false;

	// check) No Self!
	CGASSERT_ERROR(_child != dynamic_cast<Ilogger*>(this));

	// check)
	if(_child == dynamic_cast<Ilogger*>(this))
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// 1) 이미 존재하는지 찾는다.
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) 이미 존재하면 그냥 ASSERT 발생!
	CGASSERT_ERROR(iter_find == this->m_container.end());

	// check) 이미 존재하면 그냥 false를 리턴한다.
	if(iter_find != this->m_container.end())
		return false;

	// 2) set parent
	_child->m_pparent = this;

	// 3) 추가하기
	try
	{
		this->m_container.emplace_back(std::move(_child));
		this->_update_snapshot();
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
		// - with m_lockable_log locked (clear() locks it again : a dead lock)
		return this->_process_detach_all() != 0;
	}

	// check) 
	if(_child->m_pparent != this)
		return false;

	// 1) Check already exists
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) find?
	if(iter_find == this->m_container.end())
		return false;

	// 2) Delete found item
	this->m_container.erase(iter_find);
	this->_update_snapshot();

	// 3) set parent
	_child->m_pparent = nullptr;

	// return)
	return true;
}

bool logger_hub::process_attach(object_ptr<Ilogger>&& _child)
{
	SCOPED_LOCK(this->m_lockable_log)
	{
		return this->_process_attach(std::move(_child));
	}
}

bool logger_hub::process_detach(Ilogger* _child) noexcept
{
	SCOPED_LOCK(this->m_lockable_log)
	{
		return this->_process_detach(_child);
	}
}

int logger_hub::clear() noexcept
{
	SCOPED_UNIQUE_LOCK(this->m_lockable_log)
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
		// check) 
		CONTINUE_IF(iter.empty());

		// - detach
		this->_process_detach(iter);
	}

	// return)
	return count;
}