#include "pch.h"
#include "net.io.connectable.disconnect_manager.h"

using namespace CGDK;

#if defined(_WINSOCK2API_)
net::io::connectable::manager_disconnect::manager_disconnect () 
{
	// 1) set interval as 2sec
	this->execute_interval (chrono::seconds(2));

	// 2) set schedulable type
	this->m_statistics_schedulable.type = eSCHEDULABLE_TYPE::SYSTEM;

	// 3) set name
	this->set_name("socket disconnecting manager");
}

net::io::connectable::manager_disconnect::~manager_disconnect () noexcept
{
}

void net::io::connectable::manager_disconnect::initialize_instance()
{
	scoped_lock(m_lockable_create)
	{
		// check) 
		RETURN_IF(m_pinstance.exist());
		
		// 1) create instance
		m_pinstance = _make_own_raw<manager_disconnect>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}

void net::io::connectable::manager_disconnect::set_instance(own_ptr<manager_disconnect>&& _pinstance)
{
	// declare)
	own_ptr<manager_disconnect> pentity_manager_pre;

	scoped_lock(m_lockable_create)
	{
		// 1) move old instance
		pentity_manager_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance = std::move(_pinstance);
	}

	// 3) reset old instance
	pentity_manager_pre.reset();
}

void net::io::connectable::manager_disconnect::reset_instance() noexcept
{
	// declare)
	own_ptr<manager_disconnect> pinstance;

	scoped_lock(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);

		// check) 
		RETURN_IF(pinstance.empty());
	}

	// 2) get scheduler
	auto pscheduler = system_executor::schedulable_manager();

	// check)
	if (pscheduler.exist())
	{
		// 3) unregister schedulable
		pscheduler->unregister_schedulable(pinstance);
	}

	// 4) reset old instance
	pinstance.reset();
}

result_code net::io::connectable::manager_disconnect::destroy () noexcept
{
	scoped_lock(m_list_connectable)
	{
		// - close all sockets
		for(auto& iter: m_list_connectable)
		{
			iter->closesocket();
		}
	}

	// return)
	return eRESULT::SUCCESS;
}

void net::io::connectable::manager_disconnect::join (net::io::Iconnectable* _pconnectable)
{
	// check)
	CGASSERT_ERROR(_pconnectable->socket_state() == eSOCKET_STATE::CLOSED);

	// 1) add reference
	add_ref();

	scoped_lock(this->m_list_connectable)
	{
		// 2) is aleady existing?
		auto iter = std::find(this->m_list_connectable.begin(), this->m_list_connectable.end(), _pconnectable);

		// check)
		RETURN_IF(iter != this->m_list_connectable.end());

		// 3) add to connectable list
		this->m_list_connectable.push_back(_pconnectable);

		// 4) set iterator
		_pconnectable->m_iter_connectable = --this->m_list_connectable.end();

		// 5) start schedulable
		if (this->m_list_connectable.size() == 1)
		{
			// - get scheduler
			auto pscheduler = system_executor::schedulable_manager();

			// check)
			THROW_IF(pscheduler.empty(), throwable::failure(eRESULT::NOT_READY));

			// - register schedulable
			pscheduler->register_schedulable(m_pinstance);
		}
	}
}

void net::io::connectable::manager_disconnect::leave (net::io::Iconnectable* _pconnectable) noexcept
{
	scoped_lock(this->m_list_connectable)
	{
		// 1) remove from list
		this->m_list_connectable.erase(_pconnectable->m_iter_connectable);

		// 2) stop schedulable if  empty
		if (this->m_list_connectable.empty())
		{
			// - get scheduler
			auto pscheduler = system_executor::schedulable_manager();

			// check)
			if (pscheduler.exist())
			{
				pscheduler->unregister_schedulable(m_pinstance);
			}
		}
	}

	// 3) release
	release();
}

intptr_t net::io::connectable::manager_disconnect::process_execute (intptr_t /*_result*/, std::size_t /*_param*/)
{
	// 1) get now tick
	auto tick_now = chrono::tick::now();

	scoped_lock(this->m_list_connectable)
	{
		for(auto& iter: this->m_list_connectable)
		{
			// - get disconnect time
			auto tick_disconnect = iter->statistics_get_tick_disconnect();

			// check) skip if duration < 10s
			BREAK_IF((tick_now < tick_disconnect) || (tick_now - tick_disconnect) < chrono::seconds(10));

			// - closesocket
			iter->closesocket();
		}
	}

	// return) success
	return intptr_t(0);
}
#endif