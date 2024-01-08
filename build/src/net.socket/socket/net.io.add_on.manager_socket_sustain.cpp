#include "pch.h"
#include "net.io.add_on.manager_socket_sustain.h"

using namespace CGDK;


result_code net::io::add_on::manager_socket_sustain::destroy() noexcept
{
	scoped_lock(m_lockable_create)
	{
	}

	// return)
	return eRESULT::SUCCESS;
}

result_code net::io::add_on::manager_socket_sustain::process_message(sMESSAGE& /*_msg*/)
{
	// return) 
	return eRESULT::BYPASS;
}

bool net::io::add_on::manager_socket_sustain::add_socket( Nsocket_sustain* _pSocket)
{
	scoped_lock(m_list_socket)
	{
		// check)
		RETURN_IF(_pSocket->m_iter_list_socket_sustain._Ptr!=nullptr, false);

		// 1) socket을 List에 추가한다.
		m_list_socket.push_front(_pSocket);
		_pSocket->m_iter_list_socket_sustain = m_list_socket.begin();

		// 2) 첫번째 socket이 붙을 때 executor에 붙인다.
		if(m_list_socket.size() == 1)
		{
			// - executor에 붙인다.
			system_executor::register_schedulable(this);

			// Reference count를 증가시킨다.
			add_ref();
		}
	}

	// return) 
	return true;
}

bool net::io::add_on::manager_socket_sustain::remove_socket( Nsocket_sustain* _pSocket) noexcept
{
	scoped_lock(m_list_socket)
	{
		// check)
		RETURN_IF(_pSocket->m_iter_list_socket_sustain._Ptr==nullptr, false);

		// 1) socket을 리스트에서 제거한다.
		m_list_socket.erase(_pSocket->m_iter_list_socket_sustain);
		_pSocket->m_iter_list_socket_sustain._Ptr	 = nullptr;

		// 2) 모든 socket이 제거된다면 executor에서 떼낸다.
		if(m_list_socket.size()==0)
		{
			// - executor에 붙인다.
			system_executor::unregister_schedulable(this);

			// Reference count를 증가시킨다.
			release();
		}
	}

	// return) 
	return true;
}

intptr_t net::io::add_on::manager_socket_sustain::process_execute( intptr_t /*_result*/, size_t /*_param*/)
{
	scoped_lock(m_list_socket)
	{
		for(auto& iter:m_list_socket)
		{
			iter->process_socket_sustain();
		}
	}

	// return) Success! 
	return	intptr_t(0);
}

void net::io::add_on::manager_socket_sustain::initialize_instance()
{
	scoped_lock(m_lockable_create)
	{
		RETURN_IF(m_pinstance.exist());
		
		m_pinstance = _make_own_raw<manager_socket_sustain>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
	}
}

void net::io::add_on::manager_socket_sustain::set_instance(own_ptr<manager_socket_sustain>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_socket_sustain> pinstance_pre;

	scoped_lock(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance = std::move(_pinstance);
	}

	// 3) destroy instance
	if (pinstance_pre.exist())
	{
		// - unregister schedulable
		system_executor::unregister_schedulable(pinstance_pre);

		// - reset old instance
		pinstance_pre.reset();
	}
}

void net::io::add_on::manager_socket_sustain::reset_instance() noexcept
{
	// declare)
	own_ptr<manager_socket_sustain> pinstance;

	scoped_lock(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}
