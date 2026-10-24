#include "pch.h"
#include "net.io.connect_requestable.tcp_reconnect.manager_reconnector.h"

using namespace CGDK;


net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::manager_reconnector()
{
	this->name("tcp reconnect manager"sv);
}

net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::~manager_reconnector()
{
}

bool net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::register_reconnector(net::io::connect_requestable::Ntcp_reconnect* _preconnector)
{
	// check)
	RETURN_IF(_preconnector == nullptr, false);

	SCOPED_LOCK(this->m_lockable_set_reconnect)
	{
		// 1) 이미 존재하는지 확인한다.
		auto iter_find = this->m_set_reconnect.find(_preconnector);

		// check) 이미 존재하면 그냥 끝낸다.
		RETURN_IF(iter_find != this->m_set_reconnect.end(), false);

		// 2) 추가한다.
		this->m_set_reconnect.emplace(_preconnector);

		// 3) start schedulable
		if (this->m_set_reconnect.size() == 1)
		{
			system_executor::register_schedulable(this);
		}
	}

	// return) 
	return true;
}

bool net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::unregister_reconnector(net::io::connect_requestable::Ntcp_reconnect* _preconnector) noexcept
{
	SCOPED_LOCK(this->m_lockable_set_reconnect)
	{
		// 1) remove _preconnector
		//    (found by the pointer : an object_ptr made of '_preconnector' for erase() would add a reference to it while it is
		//     being destroyed. called from its final release -> stop() -> process_cancel_reconnector() -> released twice)
		auto iter_find = std::find_if(this->m_set_reconnect.begin(), this->m_set_reconnect.end(), [=](const object_ptr<net::io::connect_requestable::Ntcp_reconnect>& _preconnector_set) { return _preconnector_set.get() == _preconnector; });

		if (iter_find != this->m_set_reconnect.end())
			this->m_set_reconnect.erase(iter_find);

		// 2) sop schedulable
		if (this->m_set_reconnect.empty())
		{
			system_executor::unregister_schedulable(this);
		}
	}

	// return) 
	return true;
}

intptr_t net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::process_execute( intptr_t /*_result*/, size_t /*_param*/)
{
	// declare) 
	std::vector<object_ptr<net::io::connect_requestable::Ntcp_reconnect>> vector_temp;

	// 1) 먼저 복사한다.
	SCOPED_LOCK(this->m_lockable_set_reconnect)
	{
		// - Resize
		vector_temp.reserve(this->m_set_reconnect.size());

		// - 복사
		vector_temp.assign(this->m_set_reconnect.begin(), this->m_set_reconnect.end());
	}

	// 2) wait한다...
	for(auto& iter: vector_temp)
	{
		// - ...
		auto result = iter->process_wait_reconnect();

		// check) true면 제거한다.
		if(result ==true)
		{
			SCOPED_LOCK(this->m_lockable_set_reconnect)
			{
				this->m_set_reconnect.erase(iter);
			}
		}
	}

	// return)
	return	0;
}

void net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::initialize_instance()
{
	SCOPED_LOCK(m_lockable_create)
	{
		// check) 
		RETURN_IF(m_pinstance.exist());
		
		// 1) create instance
		auto pinstance = _make_own_raw<manager_reconnector>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
		
		// 2) set instance
		m_pinstance.exchange_release(std::move(pinstance)); 
	} 
}

void net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::set_instance(own_ptr<manager_reconnector>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_reconnector> pentity_pre;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move old instance
		pentity_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pentity_pre.reset();
}

void net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::reset_instance() noexcept
{
	// declare) 
	own_ptr<manager_reconnector> pinstance;
	
	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);

		// check)
		RETURN_IF(pinstance.empty());
	}

	// 2) unregister
	system_executor::unregister_schedulable(pinstance);

	// 3) reset 
	pinstance.reset();
}

result_code net::io::connect_requestable::Ntcp_reconnect::manager_reconnector::destroy() noexcept
{
	// 1) 먼저 unregister한다.
	system_executor::unregister_schedulable(this);

	// declare) 
	std::set<object_ptr<net::io::connect_requestable::Ntcp_reconnect>> set_reconnect;

	// 2) swap
	SCOPED_LOCK(this->m_lockable_set_reconnect)
	{
		// - swap
		set_reconnect.swap(this->m_set_reconnect);

		// -  모든 Reconnectable을 제거한다.
		for (auto& iter : set_reconnect)
		{
			iter->disable_reconnect();
			iter->m_pmanager_reconnector.reset();
		}
	}

	// return)
	return eRESULT::SUCCESS;
}
