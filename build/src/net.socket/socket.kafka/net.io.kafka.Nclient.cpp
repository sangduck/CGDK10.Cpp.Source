#include "pch.h"

using namespace CGDK;


net::io::kafka::Nclient::Nclient() noexcept
{
}

net::io::kafka::Nclient::~Nclient() noexcept
{
}

result_code net::io::kafka::Nclient::process_start(context& _context)
{
	// 1) 'enable progress_log'
	if (_context.type() != context::value_type_t::ptr_)
	{
		if (const auto& context_find = static_cast<const context&>(_context).find("progress_log", "progress log", "enable_progress_log", "enable progress log"); context_find.is_valid())
		{
			// - get string value
			std::string str_find = context_find;

			if (CGDK::__stricmp(str_find, "true") == 0
			 || CGDK::__stricmp(str_find, "enable") == 0)
			{
				this->m_enable_progress_log = true;
			}
			else if (CGDK::__stricmp(str_find, "false") == 0
			      || CGDK::__stricmp(str_find, "disable") == 0)
			{
				this->m_enable_progress_log = false;
			}
		}
	}

	// declare)
	const bool enable_progress_log = this->m_enable_progress_log;

	// log)
	EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <BEGIN> start "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// 2) call '_process_starting' (on_starting)
	auto result = this->_process_starting(_context);

	// check)
	RETURN_IF(result != eRESULT::SUCCESS, result);

	try
	{
		// 3) creates the native handle
		try
		{
			this->process_open(_context);
		}
		catch (...)
		{
			// - on_stopping() (the pair of on_starting())
			this->_process_stopping();

			// reraise)
			throw;
		}

		// 4) begins the poll thread
		this->_begin_poll_thread();

		// 5) childs and on_start()
		result = this->_process_starting_child(_context);
	}
	catch (std::exception& _e)
	{
		// - ends the poll thread and closes the native handle
		if (this->_end_poll_thread())
			this->process_close_handle();

		// log)
		LOG_ERROR << "@ (excp) fail to start "sv << this->process_get_type_name() << "['"sv << this->name() << "'] ("sv << std::string_view(_e.what()) << ")"sv;

		// reraise)
		throw;
	}
	catch (...)
	{
		// - ends the poll thread and closes the native handle
		if (this->_end_poll_thread())
			this->process_close_handle();

		// log)
		LOG_ERROR << "@ (excp) fail to start "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;

		// reraise)
		throw;
	}

	// log)
	EXECUTE_IF(enable_progress_log, LOG_PROGRESS << "@ <END> start "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// return)
	return result;
}

result_code net::io::kafka::Nclient::process_stop() noexcept
{
	// log)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <BEGIN> stop "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// 1) call '_process_stopping' (on_stopping)
	this->_process_stopping();

	// 2) ends the poll thread and closes the native handle
	//    - called on the poll thread : closed by the poll thread when it ends
	if (this->_end_poll_thread())
		this->process_close_handle();

	// 3) childs and on_stop()
	auto result = this->_process_stopping_child();

	// log)
	EXECUTE_IF(this->m_enable_progress_log, LOG_PROGRESS << "@ <END> stop "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// return)
	return result;
}

std::unique_ptr<RdKafka::Conf> net::io::kafka::Nclient::process_create_configure(const sKAFKA_CLIENT_PARAMETER& _parameter)
{
	// 1) create configure
	std::unique_ptr<RdKafka::Conf> pconfigure(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));

	// check)
	THROW_IF(pconfigure == nullptr, throwable::failure(eRESULT::FAIL_TO_CREATE, 0, "fail to create kafka configure"sv));

	// 2) "bootstrap.servers"
	this->m_bootstrap_servers = _parameter.bootstrap_servers.empty() ? "localhost:9092"s : _parameter.bootstrap_servers;

	// log)
	EXECUTE_IF(_parameter.bootstrap_servers.empty() && _parameter.enable_progress_log, LOG_INFO << "  ! 'bootstrap.servers' set as default (localhost:9092)"sv);

	this->process_set_configure(pconfigure.get(), "bootstrap.servers"s, this->m_bootstrap_servers, false);

	// 3) "client.id"
	if (_parameter.client_id.empty() == false)
		this->process_set_configure(pconfigure.get(), "client.id"s, _parameter.client_id, false);

	// 4) properties
	for (const auto& iter : _parameter.properties)
		this->process_set_configure(pconfigure.get(), iter.first, iter.second, false);

	// 5) event callback
	{
		std::string errstr;
		auto result = pconfigure->set("event_cb", &this->m_event_callback, errstr);

		// check)
		THROW_IF(result != RdKafka::Conf::CONF_OK, throwable::failure(eRESULT::FAIL, 0, CGDK::stream_string("fail to set kafka event callback (", errstr, ")")));
	}

	// 6) poll interval
	this->m_poll_interval = (_parameter.poll_interval > chrono::tick::duration::zero()) ? _parameter.poll_interval : std::chrono::milliseconds(100);

	// return)
	return pconfigure;
}

void net::io::kafka::Nclient::process_set_configure(RdKafka::Conf* _pconfigure, const std::string& _name, const std::string& _value, bool _enable_progress_log)
{
	// declare)
	std::string errstr;

	// 1) set
	auto result = _pconfigure->set(_name, _value, errstr);

	// check)
	THROW_IF(result != RdKafka::Conf::CONF_OK, throwable::failure(eRESULT::INVALID_ARGUMENT, 0, CGDK::stream_string("invalid kafka configure '", _name, "' (", errstr, ")")), LOG_ERROR << "  @ (err ) invalid kafka configure '"sv << _name << "' ("sv << errstr << ") "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv);

	// log)
	EXECUTE_IF(_enable_progress_log, LOG_INFO << "  + configure['"sv << _name << "']"sv);
}

void net::io::kafka::Nclient::process_event(RdKafka::Event& _event)
{
	switch (_event.type())
	{
	// case) error
	case	RdKafka::Event::EVENT_ERROR:
			{
				// log)
				if (_event.fatal())
					LOG_FATAL << "(err ) kafka "sv << this->process_get_type_name() << "['"sv << this->name() << "'] FATAL "sv << RdKafka::err2str(_event.err()) << " ("sv << _event.str() << ")"sv;
				else
					LOG_ERROR << "(err ) kafka "sv << this->process_get_type_name() << "['"sv << this->name() << "'] "sv << RdKafka::err2str(_event.err()) << " ("sv << _event.str() << ")"sv;

				// - on_error
				try
				{
					this->on_error(static_cast<int>(_event.err()), _event.str());
				}
				catch (...)
				{
					LOG_ERROR << "(excp) exception on 'on_error' "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;
				}
			}
			break;

	// case) log
	case	RdKafka::Event::EVENT_LOG:
			{
				if (_event.severity() <= RdKafka::Event::EVENT_SEVERITY_ERROR)
					LOG_ERROR << "(kafka) "sv << _event.fac() << ": "sv << _event.str();
				else if (_event.severity() == RdKafka::Event::EVENT_SEVERITY_WARNING)
					LOG_WARNING << "(kafka) "sv << _event.fac() << ": "sv << _event.str();
				else
					LOG_DEBUG << "(kafka) "sv << _event.fac() << ": "sv << _event.str();
			}
			break;

	// case) others (stats, throttle)
	default:
			break;
	}
}

void net::io::kafka::Nclient::_begin_poll_thread()
{
	// 1) alloc thread
	auto pthread = make_object<api::thread>();

	// 2) set name
	pthread->name(CGDK::stream_string("kafka poll ", this->name()));

	// 3) reset
	this->m_is_close_deferred = false;

	// 4) begin thread (the thread holds the reference of this object while it is running)
	object_ptr<Nclient> pthis(this);
	pthread->begin_thread([pthis](std::stop_token _stop_token) { pthis->_process_poll_thread(std::move(_stop_token)); });

	// 5) set
	this->m_pthread_poll = std::move(pthread);
}

bool net::io::kafka::Nclient::_end_poll_thread() noexcept
{
	// 1) get thread
	auto pthread = std::move(this->m_pthread_poll);

	// check)
	RETURN_IF(pthread.empty(), true);

	// check) called on the poll thread itself (in the callbacks) => the poll thread closes the handle when it ends
	if (this->is_poll_thread())
	{
		this->m_is_close_deferred = true;
		pthread->request_stop();
		return false;
	}

	// 2) request stop and wait
	pthread->stop();

	// return)
	return true;
}

void net::io::kafka::Nclient::_process_poll_thread(std::stop_token _stop_token)
{
	// 1) set thread id
	this->m_id_thread_poll = std::this_thread::get_id();

	// declare)
	auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(this->m_poll_interval);

	// check)
	if (timeout < std::chrono::milliseconds(1))
		timeout = std::chrono::milliseconds(1);

	// 2) poll
	while (_stop_token.stop_requested() == false)
	{
		try
		{
			this->process_poll(timeout);
		}
		catch (std::exception& _e)
		{
			LOG_ERROR << "(excp) exception on poll "sv << this->process_get_type_name() << "['"sv << this->name() << "'] ("sv << std::string_view(_e.what()) << ")"sv;
		}
		catch (...)
		{
			LOG_ERROR << "(excp) exception on poll "sv << this->process_get_type_name() << "['"sv << this->name() << "']"sv;
		}
	}

	// 3) closes the handle (stop() was called on this thread)
	if (this->m_is_close_deferred.exchange(false))
		this->process_close_handle();

	// 4) reset thread id
	this->m_id_thread_poll = std::thread::id();
}
