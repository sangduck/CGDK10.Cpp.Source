#include "pch.h"

using namespace CGDK;

namespace
{
	// - 'prepare_on_start' / 'must_prepare' defaults : every prepared socket costs an executor (a thread)
	constexpr std::size_t DEFAULT_PREPARE_ON_START = 2;
	constexpr std::size_t DEFAULT_MUST_PREPARE = 1;

	// - a socket younger than this is never treated as finished without a notice (between its allocation and its accept request)
	constexpr auto GRACE_ALIVE = std::chrono::seconds(5);

	// - stop()/destroy() : the time to wait for the threads of all the executors
	constexpr auto WAIT_STOP_ALL = std::chrono::seconds(10);

	// - a retired executor whose thread does not end is warned once after this
	constexpr auto WARN_STUCK = std::chrono::seconds(10);

	// - the executors whose thread never ended (stuck in a handler) : kept forever instead of joining them forever (leaked on purpose)
	std::mutex& _leaked_lockable() noexcept
	{
		static std::mutex s_lockable;
		return s_lockable;
	}

	std::vector<object_ptr<executor::Iio_context_threads>>& _leaked_executors() noexcept
	{
		static auto* s_pvector = new std::vector<object_ptr<executor::Iio_context_threads>>();
		return *s_pvector;
	}

	void _leak_executor(object_ptr<executor::Iio_context_threads>&& _pexecutor) noexcept
	{
		std::lock_guard lock(_leaked_lockable());
		_leaked_executors().push_back(std::move(_pexecutor));
	}
}


net::emergency::Nacceptor::Nacceptor()
{
	// 1) name
	this->name("emergency acceptor"sv);
}

net::emergency::Nacceptor::~Nacceptor() noexcept
{
	// 1) the executors of the sockets (an acceptor destroyed without stop())
	this->_reap_socket_executors(true);

	// 2) own executor
	this->_stop_own_executor();
}

result_code net::emergency::Nacceptor::start(const START_PARAMETER& _parameter)
{
	// 1) defaults (few prepared sockets : each one costs an executor)
	START_PARAMETER parameter = _parameter;

	if (parameter.prepare_on_start == 0)
		parameter.prepare_on_start = DEFAULT_PREPARE_ON_START;

	if (parameter.must_prepare == 0)
		parameter.must_prepare = DEFAULT_MUST_PREPARE;

	// 2) start
	return this->object::Nstartable::start<START_PARAMETER>(parameter);
}

result_code net::emergency::Nacceptor::start(context& _context)
{
	// 1) defaults (context parameters)
	if (_context.type() != context::value_type_t::buffer_)
	{
		if (_context.find("prepare_on_start", "prepare on start").is_valid() == false)
			_context["prepare_on_start"] = static_cast<int64_t>(DEFAULT_PREPARE_ON_START);

		if (_context.find("must_prepare", "must prepare").is_valid() == false)
			_context["must_prepare"] = static_cast<int64_t>(DEFAULT_MUST_PREPARE);
	}

	// 2) start
	return this->net::io::connective::acceptor::Ntcp::start(_context);
}

std::size_t net::emergency::Nacceptor::socket_executor_count() const noexcept
{
	std::lock_guard lock(this->m_lockable_executor);

	return this->m_map_socket_executor.size() + this->m_vector_retired.size();
}

void net::emergency::Nacceptor::supervise() noexcept
{
	// 1) the executors of the disconnected sockets
	this->_reap_socket_executors(false);

	// 2) the accepts lost by an exception (the retry of acceptor::Ntcp runs on the system scheduler : not here)
	this->_supervise_accept();
}

result_code net::emergency::Nacceptor::_process_starting(context& _context)
{
	// 1) own executor (an iocp/epoll and threads of its own : apart from the system_executor)
	//    (kept across a failed start of the acceptor : created once, stopped by _process_stop()/_process_destroy())
	SCOPED_LOCK(this->m_lockable_executor)
	{
		// - create & start (throws on a failure : the start of the acceptor fails)
		if (this->m_pexecutor_own.empty())
		{
			this->m_pexecutor_own = this->_make_executor(CGDK::stream_string(this->name(), " [executor]"), this->m_count_thread_executor);
		}

		// 2) the listen socket runs on it (process_create_socket() attaches the socket to it)
		this->io_context(this->m_pexecutor_own.get());
	}

	// 3) start the acceptor
	return this->net::io::connective::acceptor::Ntcp::_process_starting(_context);
}

result_code net::emergency::Nacceptor::_process_stop() noexcept
{
	// 1) the acceptor (on_stop)
	//    (_process_stopping() closed the listen socket and waited for the sockets to close already)
	auto result = this->net::io::connective::acceptor::Ntcp::_process_stop();

	// 2) the executors of the sockets
	this->_reap_socket_executors(true);

	// 3) own executor
	this->_stop_own_executor();

	// return)
	return result;
}

result_code net::emergency::Nacceptor::_process_destroy() noexcept
{
	// 1) the acceptor (on_destroy)
	auto result = this->net::io::connective::acceptor::Ntcp::_process_destroy();

	// 2) the executors (an acceptor that was never started may have its own executor)
	this->_reap_socket_executors(true);
	this->_stop_own_executor();

	// return)
	return result;
}

object_ptr<executor::Iio_context> net::emergency::Nacceptor::process_get_connectable_io_context(net::io::Iconnectable* _pconnectable)
{
	// check)
	RETURN_IF(_pconnectable == nullptr, object_ptr<executor::Iio_context>());

	// 1) the socket has its executor already?
	SCOPED_LOCK(this->m_lockable_executor)
	{
		auto iter = this->m_map_socket_executor.find(_pconnectable);

		if (iter != this->m_map_socket_executor.end())
			return object_ptr<executor::Iio_context>(iter->second.pexecutor.get());
	}

	// 2) create the executor of the socket (1 thread. throws when it can not be started : the accept request fails)
	auto pexecutor = this->_alloc_socket_executor();

	// 3) register (the socket is held : never returned to its pool while its executor is alive)
	SCOPED_LOCK(this->m_lockable_executor)
	{
		SOCKET_EXECUTOR entry;

		entry.psocket = _pconnectable;
		entry.pexecutor = pexecutor;
		entry.tick = chrono::tick::now();

		this->m_map_socket_executor.emplace(_pconnectable, std::move(entry));
	}

	// return)
	return object_ptr<executor::Iio_context>(pexecutor.get());
}

void net::emergency::Nacceptor::process_on_fail_accept(net::io::Iconnectable* _pconnectable) noexcept
{
	// 1) on_fail_accept
	this->net::io::connective::acceptor::Ntcp::process_on_fail_accept(_pconnectable);

	// 2) the executor of the socket is not needed any more
	this->_retire_socket_executor(_pconnectable);
}

void net::emergency::Nacceptor::process_connective_disconnect(net::io::Iconnectable* _pconnectable)
{
	// 1) remove from the list, request a new accept ...
	this->net::io::connective::acceptor::Ntcp::process_connective_disconnect(_pconnectable);

	// 2) retire the executor of the socket (stopped by supervise() : this may be the thread of that executor)
	this->_retire_socket_executor(_pconnectable);
}

object_ptr<net::emergency::Nacceptor::executor_t> net::emergency::Nacceptor::_alloc_socket_executor()
{
	// 1) create & start (1 thread)
	return this->_make_executor(CGDK::stream_string(this->name(), " [socket executor:", ++this->m_serial_socket_executor, "]"), 1);
}

object_ptr<net::emergency::Nacceptor::executor_t> net::emergency::Nacceptor::_make_executor(std::string_view _name, int _thread_count)
{
	// 1) parameters : the thread count, the priority of the threads (each thread sets it before its first execution)
	context context_executor;

	context_executor["thread_count"] = static_cast<int64_t>((_thread_count > 0) ? _thread_count : 1);
	context_executor["thread_priority"] = to_string(this->m_thread_priority);

	// 2) create & start (throws on a failure)
	return executor::make_io_context_threads(_name, context_executor);
}

void net::emergency::Nacceptor::_retire_socket_executor(net::io::Iconnectable* _pconnectable) noexcept
{
	// lock)
	std::lock_guard lock(this->m_lockable_executor);

	// 1) find
	auto iter = this->m_map_socket_executor.find(_pconnectable);

	// check)
	RETURN_IF(iter == this->m_map_socket_executor.end());

	// 2) move to the retired
	auto& entry = iter->second;

	entry.tick = chrono::tick::now();
	entry.is_stop_requested = false;

	this->m_vector_retired.push_back(std::move(entry));
	this->m_map_socket_executor.erase(iter);
}

void net::emergency::Nacceptor::_reap_socket_executors(bool _wait_all) noexcept
{
	// declare)
	const auto tick_now = chrono::tick::now();
	std::vector<SOCKET_EXECUTOR> vector_retired;

	// 1) retire the sockets finished without process_connective_disconnect() (a failed accept ...), take the retired
	SCOPED_LOCK(this->m_lockable_executor)
	{
		for (auto iter = this->m_map_socket_executor.begin(); iter != this->m_map_socket_executor.end(); )
		{
			auto& entry = iter->second;

			if (_wait_all == false)
			{
				// - too young (between the allocation and the accept request)
				if ((tick_now - entry.tick) < GRACE_ALIVE)
				{
					++iter;
					continue;
				}

				// - alive? (connected, or waiting for the accept)
				auto* psocket = entry.psocket.get();
				const bool is_finished = (psocket->m_pconnectable_notify == nullptr)
									  && (psocket->statistics_get_now_async_io() == 0)
									  && (psocket->socket_state() == eSOCKET_STATE::CLOSED || psocket->native_handle() == INVALID_SOCKET);
				if (is_finished == false)
				{
					++iter;
					continue;
				}

				// log)
				LOG_INFO << "  - ACCEPTOR['"sv << name() << "']: a socket finished without a notice. its executor is retired ['"sv << entry.pexecutor->name() << "']"sv;
			}

			// - retire
			entry.tick = tick_now;
			entry.is_stop_requested = false;

			this->m_vector_retired.push_back(std::move(entry));
			iter = this->m_map_socket_executor.erase(iter);
		}

		// - take
		vector_retired.swap(this->m_vector_retired);
	}

	// check)
	RETURN_IF(vector_retired.empty());

	// 2) stop the retired executors (outside the lock : the remaining executables run and the thread is joined here)
	const auto tick_deadline = tick_now + WAIT_STOP_ALL;
	std::vector<SOCKET_EXECUTOR> vector_remain;

	for (auto& entry : vector_retired)
	{
		if (entry.is_stop_requested == false)
		{
			// - close the socket : its handle is bound to the io_context of this executor, it is never reused with another one.
			//   (the pooled socket object gets a new handle on its next accept)
			if (entry.psocket.exist())
			{
				entry.psocket->closesocket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
				entry.psocket->reset_io_context();
				entry.psocket.reset();
			}

			// - stop the executor without waiting (its thread ends by itself)
			entry.pexecutor->request_stop();
			entry.is_stop_requested = true;
			entry.tick = tick_now;
		}

		// - the thread ended? => join and release
		const auto tick_wait_now = chrono::tick::now();
		const auto wait = (_wait_all && tick_deadline > tick_wait_now) ? (tick_deadline - tick_wait_now) : chrono::tick::duration::zero();

		if (_stop_executor(entry.pexecutor, wait))
			continue;

		// - still running (stuck in a handler?)
		if (entry.is_warned == false && (tick_wait_now - entry.tick) >= WARN_STUCK)
		{
			// log)
			LOG_WARNING << "(warn) ACCEPTOR['"sv << name() << "']: the thread of a socket executor does not end ['"sv << entry.pexecutor->name() << "'] (stuck in a handler?)"sv;

			entry.is_warned = true;
		}

		if (_wait_all)
		{
			// log)
			LOG_ERROR << log_level_t::important() << "(err ) ACCEPTOR['"sv << name() << "']: a socket executor is given up, its thread never ended ['"sv << entry.pexecutor->name() << "']"sv;

			// - kept forever (joining it would block forever)
			_leak_executor(std::move(entry.pexecutor));
			continue;
		}

		// - next time
		vector_remain.push_back(std::move(entry));
	}

	// 3) keep the remaining for the next supervise()
	if (vector_remain.empty() == false)
	{
		std::lock_guard lock(this->m_lockable_executor);

		for (auto& entry : vector_remain)
			this->m_vector_retired.push_back(std::move(entry));
	}
}

void net::emergency::Nacceptor::_supervise_accept() noexcept
{
#if defined(_WINSOCK2API_)
	// check) listening?
	RETURN_IF(this->get_object_state() != eOBJECT_STATE::RUNNING);
	RETURN_IF(this->is_disable());
	RETURN_IF(this->socket_state() != eSOCKET_STATE::LISTEN);

	// 1) request the accepts up to 'must_prepare' (a few at a time)
	const auto must_prepare = this->m_accept_count_must_prepare;

	for (int i = 0; i < 8 && this->m_socket_count_prepared_to_accept < must_prepare; ++i)
	{
		try
		{
			this->request_accept(this->m_address_family);
		}
		catch (...)
		{
			// - again on the next supervise()
			break;
		}
	}
#endif
}

void net::emergency::Nacceptor::_stop_own_executor() noexcept
{
	// 1) take
	object_ptr<executor_t> pexecutor;

	SCOPED_LOCK(this->m_lockable_executor)
	{
		pexecutor = std::move(this->m_pexecutor_own);
		this->m_pexecutor_own.reset();
	}

	// check)
	RETURN_IF(pexecutor.empty());

	// 2) stop
	pexecutor->request_stop();

	if (_stop_executor(pexecutor, WAIT_STOP_ALL) == false)
	{
		// log)
		LOG_ERROR << log_level_t::important() << "(err ) ACCEPTOR['"sv << name() << "']: the executor of the acceptor is given up, its thread never ended ['"sv << pexecutor->name() << "']"sv;

		// - kept forever
		_leak_executor(std::move(pexecutor));
	}
}

bool net::emergency::Nacceptor::_stop_executor(object_ptr<executor_t>& _pexecutor, chrono::tick::duration _wait) noexcept
{
	// check)
	RETURN_IF(_pexecutor.empty(), true);

	// 1) wait until the threads end (the io_context is disposed : they end by themselves)
	const auto tick_end = chrono::tick::now() + _wait;

	while (_pexecutor->existing_thread_count() != 0)
	{
		// check) time out
		RETURN_IF(chrono::tick::now() >= tick_end, false);

		// - wait
		api::sleep_for(std::chrono::milliseconds(1));
	}

	// 2) join (returns at once : the threads ended) and release
	_pexecutor->stop();
	_pexecutor.reset();

	// return)
	return true;
}
