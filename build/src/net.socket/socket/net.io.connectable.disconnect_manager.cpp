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
	this->name("socket disconnecting manager");
}

net::io::connectable::manager_disconnect::~manager_disconnect () noexcept
{
}

void net::io::connectable::manager_disconnect::initialize_instance()
{
	SCOPED_LOCK(m_lockable_create)
	{
		// check) 
		RETURN_IF(m_pinstance.exist());
		
		// 1) create instance
		m_pinstance.exchange_release(_make_own_raw<manager_disconnect>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}

void net::io::connectable::manager_disconnect::set_instance(own_ptr<manager_disconnect>&& _pinstance)
{
	// declare)
	own_ptr<manager_disconnect> pentity_manager_pre;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move old instance
		pentity_manager_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pentity_manager_pre.reset();
}

void net::io::connectable::manager_disconnect::reset_instance() noexcept
{
	// declare)
	own_ptr<manager_disconnect> pinstance;

	SCOPED_LOCK(m_lockable_create)
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
	// declare)
	std::vector<net::io::Iconnectable*> vector_connectable;

	// 1) 살아 있는 connectable만 참조를 잡아 모은다. (참조가 0인 것은 해제 중이다)
	SCOPED_LOCK(m_lockable_list_connectable)
	{
		vector_connectable.reserve(m_list_connectable.size());

		for(auto& iter: m_list_connectable)
		{
			if (iter->add_ref_if_alive())
				vector_connectable.push_back(iter);
		}
	}

	// 2) close all sockets (lock 밖 : close가 목록에서 떼어낼(leave) 수 있다)
	for (auto& iter : vector_connectable)
	{
		iter->closesocket();
		iter->release();
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

	SCOPED_LOCK(this->m_lockable_list_connectable)
	{
		// 2) is already existing?
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
	SCOPED_LOCK(this->m_lockable_list_connectable)
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
	// - 닫을 것을 lock 안에서 참조를 잡아 모으고, lock 밖에서 닫는다.
	//   (이전 : lock을 잡은 채 raw pointer를 돌며 closesocket()을 불렀다. 같은 thread에서 마지막 참조가 풀리면
	//    leave()가 순회 중인 목록에서 노드를 지웠다(recursive lock). 참조가 0인 connectable은 해제 중이므로 건너뛴다)

	// 1) get now tick
	auto tick_now = chrono::tick::now();

	// declare)
	std::vector<net::io::Iconnectable*> vector_connectable;

	SCOPED_LOCK(this->m_lockable_list_connectable)
	{
		for(auto& iter: this->m_list_connectable)
		{
			// - get disconnect time
			auto tick_disconnect = iter->statistics_get_tick_disconnect();

			// check) skip if duration < 10s
			BREAK_IF((tick_now < tick_disconnect) || (tick_now - tick_disconnect) < chrono::seconds(10));

			// - 닫을 것 (참조를 잡는다)
			if (iter->add_ref_if_alive())
				vector_connectable.push_back(iter);
		}
	}

	// 2) closesocket (lock 밖)
	for (auto& iter : vector_connectable)
	{
		iter->closesocket();
		iter->release();
	}

	// return) success
	return intptr_t(0);
}
#endif