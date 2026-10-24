#include "pch.h"

using namespace CGDK;


net::io::binder::manager::manager()
{
	this->name("binder manager"sv);
}

net::io::binder::manager::~manager() noexcept
{
	// check) manager가 파괴될땐 분명 Pool이 하나도 없어야 한다.
	CGASSERT_ERROR(this->m_list_binder.empty());
}

result_code net::io::binder::manager::destroy() noexcept
{
	// declare)
	std::vector<net::io::Isocket*> vector_socket;

	// 1) 등록된 binder 리스트를 얻는다. (살아 있는 것만 참조를 잡는다 : lock 밖에서 닫는 동안 해제되지 않도록. 참조가 0인 것은 해제 중이다)
	SCOPED_LOCK(this->m_lockable_list_binder)
	{
		// - reserve
		vector_socket.reserve(this->m_list_binder.size());

		// - copy binder
		for (auto& iter : this->m_list_binder)
		{
			net::io::Isocket* psocket = iter;

			if (psocket->add_ref_if_alive())
				vector_socket.push_back(psocket);
		}
	}

	// 2) 모두 closesocket 처리한다.
	for(auto& iter: vector_socket)
	{
		iter->process_close_socket(DISCONNECT_REASON_ACTIVE | DISCONNECT_REASON_ABORTIVE);
		iter->release();
	}

	// return)
	return eRESULT::SUCCESS;
}

void net::io::binder::manager::join( net::io::Ibinder* _pbinder, net::io::statistics::Nsocket* _pstatistics_socket)
{
	SCOPED_LOCK(this->m_lockable_list_binder)
	{
		// 1) 같은 것이 있는지 먼저 검사한다.
		auto iter_binder = this->m_list_binder.find(_pbinder);

		// check) 이미 같은 것이 존재하면 여기서 끝낸다.
		RETURN_IF(iter_binder != this->m_list_binder.end());

		// 2) 추가한다.
		this->m_list_binder.emplace(_pbinder);

		// 3) Add Ref한다.
		this->add_ref();
	}

	// notify) message 날리기..
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::NETWORK::SOCKET::BINDER_REGISTER, 0, reinterpret_cast<uintptr_t>(_pbinder), reinterpret_cast<uintptr_t>(_pstatistics_socket)));
}

void net::io::binder::manager::leave( net::io::Ibinder* _pbinder) noexcept
{
	SCOPED_LOCK(this->m_lockable_list_binder)
	{
		// 1) Pool 관리 List에서 떼낸다.
		auto iter_binder = this->m_list_binder.find(_pbinder);

		// check) 해당 Connective가 없으면 제거한다.
		RETURN_IF(iter_binder == this->m_list_binder.end());

		// 2) 리스트에서 제거한다.
		this->m_list_binder.erase(iter_binder);
	}

	// 주의) 여기에서 _pbinder는 생성자에서 호출되는 것이므로 dynamic_cast를 하면 오류가 발생할 수 있다는 점을 유의해야 한다.

	// notify) message 날리기..
	this->transmit_message(sMESSAGE_NOTIFY(eMESSAGE::NETWORK::SOCKET::BINDER_UNREGISTER, 0, reinterpret_cast<uintptr_t>(_pbinder)));

	// 4) Release한다.
	this->release();
}

void net::io::binder::manager::initialize_instance()
{
	SCOPED_LOCK(m_lockable_create)
	{
		RETURN_IF(m_pinstance.exist());
		
		m_pinstance.exchange_release(_make_own_raw<manager>(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL));
	}
}

void net::io::binder::manager::set_instance(own_ptr<manager>&& _pinstance) noexcept
{
	// declare)
	own_ptr<manager> pinstance_pre;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move old instance
		pinstance_pre = std::move(m_pinstance);

		// 2) set new instance
		m_pinstance.exchange_release(std::move(_pinstance));
	}

	// 3) reset old instance
	pinstance_pre.reset();
}

void net::io::binder::manager::reset_instance() noexcept
{
	// declare)
	own_ptr<manager> pinstance;

	SCOPED_LOCK(m_lockable_create)
	{
		// 1) move instance
		pinstance = std::move(m_pinstance);
	}

	// 2) reset old instance
	pinstance.reset();
}