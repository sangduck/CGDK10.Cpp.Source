#include "pch.h"

using namespace CGDK;


log_filter::FILTER_INFO get_parameter_filter_info(const context& _context, bool _is_enable_log = true)
{
	// declare) 
	log_filter::FILTER_INFO	filter_info;

	filter_info.is_enable = true;	// bool

	// 2) level
	if (auto& context_find = _context["range_level_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_level_min = log_level_t{ (int)context_find };
		filter_info.is_limit_level = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_level_min']: '"sv << filter_info.range_level_min << "'"sv);
	}
	if (auto& context_find = _context["range_level_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_level_max = log_level_t{ (int)context_find };
		filter_info.is_limit_level = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_level_max']: '"sv << filter_info.range_level_max << "'"sv);
	}

	// 2) location
	if (auto& context_find = _context["range_origin_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_origin_min = log_origin_t{ (uint64_t)context_find };
		filter_info.is_limit_origin = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_origin_min']: '"sv << filter_info.range_origin_min << "'"sv);
	}
	if (auto& context_find = _context["range_origin_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_origin_max = log_origin_t{ (uint64_t)context_find };
		filter_info.is_limit_origin = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_origin_max']: '"sv << filter_info.range_origin_max << "'"sv);
	}

	// 3) source
	if (auto& context_find = _context["range_source_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_source_min = log_source_t{ (uint64_t)context_find };
		filter_info.is_limit_source = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_source_min']: '"sv << filter_info.range_source_min << "'"sv);
	}
	if (auto& context_find = _context["range_source_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_source_max = log_source_t{ (uint64_t)context_find };
		filter_info.is_limit_source = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_source_max']: '"sv << filter_info.range_source_max << "'"sv);
	}

	// 4) destination
	if (auto& context_find = _context["range_destination_min"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_destination_min = log_destination_t{ (uint64_t)context_find };
		filter_info.is_limit_destination = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_destination_min']: '"sv << filter_info.range_destination_min << "'"sv);
	}
	if (auto& context_find = _context["range_destination_max"sv]; context_find.is_valid())
	{
		// - set value
		filter_info.range_destination_max = log_destination_t{ (uint64_t)context_find };
		filter_info.is_limit_destination = true;

		// log)
		EXECUTE_IF(_is_enable_log, LOG_INFO << "  + parameter['range_destination_max']: '"sv << filter_info.range_destination_max << "'"sv);
	}

	// return) 
	return filter_info;
}

result_code Nlogger::_process_initializing(context& _context)
{
	// declare)
	eRESULT result;

	try
	{
		// 1) ...
		result = Ninitializable::_process_initializing(_context);

		// 2) "filter"
		if (auto& context_find = _context.find("filter"sv); context_find.is_valid())
		{
			// 3) Filter를 새로 생성한다.
			auto pfilter = make_object<log_filter>();

			if (auto& context_filter_info = context_find.find("info"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::info, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("progress"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::progress, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("debug"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::debug, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("fatal"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::fatal, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("error"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::error, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("warning"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::warning, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("user"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::user, get_parameter_filter_info(context_filter_info));
			}
			if (auto& context_filter_info = context_find.find("system"sv); context_filter_info.is_valid())
			{
				pfilter->set_filter_info(log_type_t::system, get_parameter_filter_info(context_filter_info));
			}

			// - set filter object
			set_filter(pfilter);
		}
	}
	catch(...)
	{
		// log) 
		LOG_ERROR << "@ <TERMINATE> exception complete Nlogger['"sv << name() << "']"sv;

		// Reraise)
		throw;
	}

	// return)
	return result;
}

//-----------------------------------------------------------------------------
// Nlogger_async
//-----------------------------------------------------------------------------
namespace
{
	using sNODE = Nlogger_async::sNODE;

	// - free nodes of all loggers
	//   pushed by the executions (a list at once), taken whole by the producers (exchange : no ABA)
	std::atomic<sNODE*> g_pfree_nodes{ nullptr };

	void _free_nodes(sNODE* _pfirst, sNODE* _plast) noexcept
	{
		auto phead = g_pfree_nodes.load(std::memory_order_relaxed);
		do
		{
			_plast->pnext = phead;
		} while (!g_pfree_nodes.compare_exchange_weak(phead, _pfirst, std::memory_order_release, std::memory_order_relaxed));
	}

	// - free nodes of this thread (taken from g_pfree_nodes)
	struct sNODE_CACHE
	{
		sNODE*	phead{ nullptr };

		~sNODE_CACHE() noexcept
		{
			// - return the nodes left
			if (this->phead == nullptr)
				return;

			auto plast = this->phead;
			while (plast->pnext != nullptr)
				plast = plast->pnext;
			_free_nodes(this->phead, plast);
		}
	};
	thread_local sNODE_CACHE t_node_cache;

	sNODE* _alloc_node()
	{
		// 1) this thread
		auto& cache = t_node_cache;
		if (cache.phead == nullptr)
			cache.phead = g_pfree_nodes.exchange(nullptr, std::memory_order_acquire);

		// 2) a free node
		if (auto pnode = cache.phead; pnode != nullptr)
		{
			cache.phead = pnode->pnext;
			return pnode;
		}

		// 3) a new node
		return new sNODE;
	}

	// - the nodes are deleted at exit
	struct sNODE_RELEASER
	{
		~sNODE_RELEASER() noexcept
		{
			for (auto p = g_pfree_nodes.exchange(nullptr, std::memory_order_acquire); p != nullptr;)
			{
				auto pnext = p->pnext;
				delete p;
				p = pnext;
			}
		}
	} g_node_releaser;
}

Nlogger_async::~Nlogger_async() noexcept
{
	// - the records left (an execution which failed to run)
	while (this->m_count_queued.load(std::memory_order_acquire) != 0)
	{
		const auto count = this->_process_take_queued();
		this->m_vector_processing.clear();
		if (this->m_count_queued.fetch_sub(count, std::memory_order_acq_rel) == count)
			break;
	}
}

void Nlogger_async::process_log(object_ptr<LOG_RECORD>&& _plog_record)
{
	// check) a null record : nothing to do (the logs are just queued, they are not waited for)
	if (_plog_record.empty())
		return;

	// check)
	if (this->get_object_state() < eOBJECT_STATE::STOPPED || this->get_object_state() > eOBJECT_STATE::RUNNING)
		return;

	// 1) node (the reference of the record is moved to the node)
	auto pnode = _alloc_node();
	pnode->precord = _plog_record.get();
	_plog_record.detach();

	// 2) count first (the execution never takes more records than counted)
	const bool is_first = (this->m_count_queued.fetch_add(1, std::memory_order_acq_rel) == 0);

	// 3) push to the stack (lock-free)
	pnode->pnext = this->m_phead_queued.load(std::memory_order_relaxed);
	while (!this->m_phead_queued.compare_exchange_weak(pnode->pnext, pnode, std::memory_order_release, std::memory_order_relaxed))
	{
	}

	// 4) the first record after the queue was empty posts the execution
	if (is_first)
		this->_process_post();
}

void Nlogger_async::_process_post()
{
	try
	{
		// 1) async request
		if (system_executor::post(this) == true)
			return;
	}
	catch (...)
	{
	}

	// 2) fail to post : processed on this thread (the only one which owns the queue now)
	this->process_execute(0, 0);
}

size_t Nlogger_async::_process_take_queued()
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
	sNODE* plast = nullptr;
	for (auto p = phead; p != nullptr; p = p->pnext)
	{
		++count;
		plast = p;
	}

	// 3) the stack is LIFO : stored in the reversed order (FIFO)
	this->m_vector_processing.resize(count);

	auto index = count;
	for (auto p = phead; p != nullptr; p = p->pnext)
	{
		// - the reference of the node is moved to m_vector_processing
		this->m_vector_processing[--index] = p->precord;
		p->precord->release();
		p->precord = nullptr;
	}

	// 4) the nodes are reused
	_free_nodes(phead, plast);

	// return)
	return count;
}

intptr_t Nlogger_async::process_execute(intptr_t /*_return*/, size_t /*_param*/)
{
	object_ptr<Nlogger_async> hold(this);

	for (;;)
	{
		// 1) take every queued record
		const auto count = this->_process_take_queued();

		// 2) write them at once
		try
		{
			this->process_log_batch(this->m_vector_processing);
		}
		catch (...)
		{
		}

		// 3) clear
		this->m_vector_processing.clear();

		// check) done (no record was queued meanwhile)
		if (this->m_count_queued.fetch_sub(count, std::memory_order_acq_rel) == count)
			return 0;
	}
}

void Nlogger_async::_process_wait_complete()
{
	// - waits until every queued record is written
	while (this->m_count_queued.load(std::memory_order_acquire) != 0)
		std::this_thread::sleep_for(1ms);
}


//-----------------------------------------------------------------------------
// logger factory
//-----------------------------------------------------------------------------
namespace
{
	struct sLOGGER_CREATORS
	{
		std::mutex											lockable;
		std::vector<std::pair<std::string, logger_creator_t>> vector_creator;
	};

	sLOGGER_CREATORS& _get_logger_creators()
	{
		static sLOGGER_CREATORS creators;
		return creators;
	}
}

void CGDK::register_logger_creator(std::string_view _type, logger_creator_t _fn_creator)
{
	// check)
	if (_type.empty() || _fn_creator == nullptr)
		return;

	auto& creators = _get_logger_creators();
	std::lock_guard lock(creators.lockable);

	// - replaced if registered already
	for (auto& iter : creators.vector_creator)
	{
		if (iter.first == _type)
		{
			iter.second = _fn_creator;
			return;
		}
	}

	creators.vector_creator.emplace_back(std::string(_type), _fn_creator);
}

own_ptr<Nlogger> CGDK::create_logger(std::string_view _type)
{
	// 1) built-in
	if (_type.empty() || _type == "file"sv)
		return make_own<logger_file>();
	if (_type == "console"sv)
		return make_own<logger_console>();
	if (_type == "system"sv)
		return make_own<logger_system>();		// - windows : event log, linux : syslog
	if (_type == "socket"sv)
		return make_own<logger_socket>();
#if defined(__linux__)
	if (_type == "syslog"sv)
		return make_own<logger_syslog>();
#endif

	// 2) registered
	logger_creator_t fn_creator = nullptr;
	{
		auto& creators = _get_logger_creators();
		std::lock_guard lock(creators.lockable);
		for (auto& iter : creators.vector_creator)
		{
			if (iter.first == _type)
			{
				fn_creator = iter.second;
				break;
			}
		}
	}

	// return)
	return (fn_creator != nullptr) ? fn_creator() : own_ptr<Nlogger>();
}
