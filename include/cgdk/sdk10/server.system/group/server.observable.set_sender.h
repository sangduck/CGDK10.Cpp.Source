//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                         Socket template Classes                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		set_sender
 @brief		
 @todo		
 @ref		
 @details	\n
 List형 Observer Class 객체
 Group과 다른 점은 독점적이지 않다는 것이다.
 여러 개의 Observer에도 붙어서 동작할 수 있다.

*///-----------------------------------------------------------------------------
template <class _TOBSERVER, class _TOBSERVER_DATA=int64_t, class _observer_base_t=_TOBSERVER>
class observable::set_sender :
// inherited cClasses)
	public						observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>,
	virtual public				net::io::Isender
{
// definition)
public:
	using observable_bast_t = observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>;
	using observer_t = _TOBSERVER;

// constructor/destructor)
public:
			set_sender(size_t _max_observer = SIZE_MAX) noexcept : observable::set<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>(_max_observer) {}
	virtual	~set_sender() noexcept {}

// publics) 
public:
			bool				send_except(observer_t* _pobserver_except, const shared_buffer& _buffer, uint64_t _option = 0);
			bool				send_conditional(const shared_buffer& _buffer, const std::function<bool (const observer_t*)>& _pred, uint64_t _option = 0);

// implementations)
protected:
	//! @brief 전체 멤버에게 메모리 버퍼의 내용을 전송한다. @param _buffer 전송할 메모리 버퍼 @return true 성공 @return false 실패
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option);
};

	template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set_sender<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option)
{
	// check)
	RETURN_IF(_buffer.empty(), false)

	// declare) observer snapshot (copy-on-write. no lock while the published one is valid)
	auto snapshot_send = this->get_observer_snapshot();


	// 1) send to all observer
	for (auto* iter : snapshot_send)
	{
		iter->send({ shared_buffer(_buffer), _count_message }, _option);
	}

	// return)
	return true;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set_sender<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::send_except(observer_t* _pobserver_except, const shared_buffer& _buffer, uint64_t _option)
{
	// declare) observer snapshot (copy-on-write. no lock while the published one is valid)
	auto snapshot_send = this->get_observer_snapshot();


	for (auto* iter : snapshot_send)
	{
		// check) _pobserver_except
		CONTINUE_IF(iter == _pobserver_except);

		// - send to observer
		iter->send(shared_buffer(_buffer), _option);
	}

	// return)
	return true;
}

template <class _TOBSERVER, class _TOBSERVER_DATA, class _observer_base_t>
bool observable::set_sender<_TOBSERVER, _TOBSERVER_DATA, _observer_base_t>::send_conditional(const shared_buffer& _buffer, const std::function<bool (const observer_t*)>& _pred, uint64_t _option)
{
	// declare)
	std::vector<object_ptr<observer_t>> vector_send;

	SCOPED_LOCK(this->m_lockable_observable)
	{
		// - reserve
		vector_send.reserve(this->m_container_observer.size());

		for(const auto& iter: this->m_container_observer)
		{
			// check)
			CONTINUE_IF(_pred(iter.pobserver) == false);

			// - copy observer
			vector_send.push_back(iter.pobserver);
		}
	}

	for(auto& iter: vector_send)
	{
		// - send to observer
		iter->send(shared_buffer(_buffer), _option);
	}

	// return)
	return true;
}


}