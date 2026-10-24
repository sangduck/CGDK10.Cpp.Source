#include "pch.h"
#include "net.io.add_on.manager_socket_sustain.h"

using namespace CGDK;


result_code net::io::add_on::manager_socket_sustain::destroy() noexcept
{
	SCOPED_LOCK(m_lockable_create)
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
	SCOPED_LOCK(this->m_lockable_list_socket)
	{
		// check)
		RETURN_IF(_pSocket->m_iter_list_socket_sustain._Ptr!=nullptr, false);

		// 1) socket을 List에 추가한다.
		this->m_list_socket.push_front(_pSocket);
		_pSocket->m_iter_list_socket_sustain = this->m_list_socket.begin();

		// 2) 첫 번째 socket이 붙을 때 executor에 붙인다.
		if(this->m_list_socket.size() == 1)
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
	SCOPED_LOCK(this->m_lockable_list_socket)
	{
		// check)
		RETURN_IF(_pSocket->m_iter_list_socket_sustain._Ptr==nullptr, false);

		// 1) socket을 리스트에서 제거한다.
		this->m_list_socket.erase(_pSocket->m_iter_list_socket_sustain);
		_pSocket->m_iter_list_socket_sustain._Ptr	 = nullptr;

		// 2) 모든 socket이 제거된다면 executor에서 떼낸다.
		if(this->m_list_socket.size()==0)
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
	// - socket을 lock 안에서 참조를 잡아 복사하고, lock 밖에서 sustain한다.
	//   (이전 : lock을 잡은 채 raw pointer 목록을 돌며 호출했다. timeout 처리가 socket을 닫으면 마지막 참조가 풀려
	//    그 자리에서 해제되었고(해제된 socket을 계속 읽음), 소멸자가 순회 중인 목록에서 자기를 지웠다(반복자 무효).
	//    다른 thread에서 해제될 때는 소멸 중인 socket을 호출할 수 있었다)
	//   - 참조가 0인 socket(해제 중이거나 pool에 돌아간 것)은 건너뛴다.
	//   - 지금 쓰는 시간 제한이 없는 socket도 참조를 잡지 않고 건너뛴다. (_is_sustain_target : socket이 많아도 대상만 참조 count를 건드린다)

	// declare)
	std::vector<Nsocket_sustain*> vector_socket;

	// 1) 살아 있는 socket만 참조를 잡아 복사한다.
	SCOPED_LOCK(this->m_lockable_list_socket)
	{
		vector_socket.reserve(this->m_list_socket.size());

		for(auto& iter:this->m_list_socket)
		{
			if (iter->_is_sustain_target() && iter->add_ref_if_alive())
				vector_socket.push_back(iter);
		}
	}

	// 2) sustain (lock 밖 : 닫혀도 참조를 잡고 있으므로 해제되지 않는다)
	for (auto psocket : vector_socket)
	{
		try
		{
			psocket->process_socket_sustain();
		}
		catch (...)
		{
		}

		// - release (마지막 참조면 여기서 해제된다 : 소멸자는 lock을 잡고 목록에서 자기를 지운다)
		psocket->release();
	}

	// return) Success! 
	return	intptr_t(0);
}

void net::io::add_on::manager_socket_sustain::initialize_instance()
{
	SCOPED_LOCK(m_lockable_create)
	{
		RETURN_IF(m_pinstance.exist());
		
		m_pinstance.exchange_release(_make_own_raw<manager_socket_sustain>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}

void net::io::add_on::manager_socket_sustain::set_instance(own_ptr<manager_socket_sustain>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager_socket_sustain> pinstance_pre;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance.exchange_release(std::move(_pinstance));
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

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}
