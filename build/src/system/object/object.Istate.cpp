#include "pch.h"

bool _stop_child_only(CGDK::object::Istate* _object);
bool _destroy_child_only(CGDK::object::Istate* _object);

namespace CGDK
{

result_code object::initialize(object::Istate* _object, context& _context)
{
	// check)
	if(_object == nullptr)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// declare)
	result_code result;

	// 1) 
	auto* pinitializable = dynamic_cast<object::Iinitializable*>(_object);

	// 2-1) 
	if(pinitializable != nullptr)
	{
		result = pinitializable->initialize(_context);
	}
	// 2-2) Child node를 initialize한다.
	else
	{
		// - set result
		result = eRESULT::SUCCESS;

		// - eOBJECT_STATE::NONE 상태일 때만 eOBJECT_STATE::INITIALIZING_PENDING 상태로 바꾼다.
		bool is_changed = _object->set_object_state_if(eOBJECT_STATE::NONE, eOBJECT_STATE::INITIALIZING_PENDING);

		// check) 상태가 바뀌지 않았으면 false를 리턴하며 끝낸다.
		if(is_changed == false)
			return result_code(eRESULT::INVALID_STATE);

		// - 자기의 이름을 얻는다.
		auto* pnameable = dynamic_cast<object::Inameable*>(_object);
		auto* pcontext_now = &_context;

		// - 이름이 있으면 Context를 변경한다.
		if(pnameable != nullptr && pnameable->name().empty() == false)
		{
			auto& context_find = _context[pnameable->name()];

			if(context_find.is_valid())
				pcontext_now = &context_find;
		}

		// - child를 모두 initialize한다.
		auto iter = _object->m_container.begin();
		auto iter_end = _object->m_container.end();

		try
		{
			for(; iter!=iter_end; ++iter)
			{
				// - push wait info
				_object->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

				// - initialize child
				result_code result_nested = object::initialize(*iter, *pcontext_now);

				if (result_nested != eRESULT::PENDED)
				{
					if (_object->m_queue_pended.empty() == false && _object->m_queue_pended.back().pobject == *iter)
						_object->m_queue_pended.pop_back();
				}
				else
				{
					result = eRESULT::PENDED;
				}
			}
		}
		catch(...)
		{
			// roll-back한다.
			for (; iter != _object->m_container.begin();)
				object::destroy(*(--iter));

			// - clear pended queue
			_object->m_queue_pended.clear();

			// - 상태를 다시 eOBJECT_STATE::NONE으로 되돌린다.
			_object->set_object_state(eOBJECT_STATE::NONE);

			// reraise) 
			throw;
		}

		if (result != eRESULT::PENDED)
			_object->set_object_state(eOBJECT_STATE::STOPPED);
	}

	// return)
	return result;
}

result_code object::start(object::Istate* _object, context& _context)
{
	// check)
	if(_object == nullptr)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// 1) 먼저 initialize한다.
	auto is_initialize_now  = object::initialize(_object, _context);

	// declare)
	result_code result;

	// 2) 자기 자신을 start한다.
	auto* pstartable = dynamic_cast<object::Istartable*>(_object);

	// 3-A) ICGStratable을 상속받았으면 start함수를 호출한다.
	if(pstartable != nullptr)
	{
		result = pstartable->start(_context);
	}
	// 3-B) Child node를 start한다.
	else
	{
		// - set as eRESULT::SUCCESS
		result = eRESULT::SUCCESS;

		// - 만약 이전 상태가 STOPPED상태면 START_PENDING상태로 변경한다.
		bool is_changed = _object->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::START_PENDING);

		// check) 이전 상태가 STOPPED상태가 아니라면 eRESULT::INVALID_STATE를 리턴한다.
		if(is_changed == false)
			return result_code(eRESULT::INVALID_STATE);

		// - 자기의 이름을 얻는다.
		auto* pnameable = dynamic_cast<object::Inameable*>(_object);
		auto* pcontext_now = &_context;

		// - 이름이 있으면 Context를 변경한다.
		if(pnameable != nullptr && pnameable->name().empty() == false)
		{
			auto& context_find = _context[pnameable->name()];

			if(context_find.is_valid())
				pcontext_now = &context_find;
		}

		// - Child를 모두 start한다.
		auto iter = _object->m_container.begin();
		auto iter_end = _object->m_container.end();

		try
		{
			for(; iter!=iter_end; ++iter)
			{
				// - push wait info
				_object->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

				// - start child
				result_code result_nested = object::start(*iter, *pcontext_now);

				if (result_nested != eRESULT::PENDED)
				{
					if (_object->m_queue_pended.empty() == false && _object->m_queue_pended.back().pobject == *iter)
						_object->m_queue_pended.pop_back();
				}
				else
				{
					result = eRESULT::PENDED;
				}
			}
		}
		catch(...)
		{
			// Roll-back한다.
			for (; iter != _object->m_container.begin();)
				object::stop(*(--iter));

			// - 상태를 다시 eOBJECT_STATE::STOPPED으로 되돌린다.
			_object->set_object_state(eOBJECT_STATE::STOPPED);

			// - destroy한다.
			if (is_initialize_now)
				object::destroy(_object);

			// reraise) 
			throw;
		}

		if (result != eRESULT::PENDED)
			// - service state를 변경한다.(eOBJECT_STATE::RUNNING)
			_object->set_object_state(eOBJECT_STATE::RUNNING);
	}

	// return)
	return result;
}

result_code _stop_child_only(object::Istate* _object)
{
	// 1) eOBJECT_STATE::RUNNING상태일 때만 eOBJECT_STATE::STOP_PENDING상태로 바꾼다.
	bool is_changed = _object->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::STOP_PENDING);

	// check) 상태가 변경되지 않았다면 그냥 끝낸다.
	if(is_changed == false)
		return result_code(eRESULT::INVALID_STATE);

	// declare) 
	result_code result = eRESULT::SUCCESS;

	// 2) 모든 자식 객체들을 stop한다.
	auto iter = _object->m_container.rbegin();
	auto iter_end = _object->m_container.rend();

	for (; iter != iter_end; ++iter)
	{
		// - push wait info
		_object->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

		// - start child
		result_code result_nested = object::stop(*iter);

		if (result_nested != eRESULT::PENDED)
		{
			if (_object->m_queue_pended.empty() == false && _object->m_queue_pended.back().pobject == *iter)
				_object->m_queue_pended.pop_back();
		}
		else
		{
			result = eRESULT::PENDED;
		}
	}

	// 3) 상태를 eOBJECT_STATE::STOPPED로 변경한다.
	if (result != eRESULT::PENDED)
		_object->set_object_state(eOBJECT_STATE::STOPPED);

	// return) 
	return result;
}

result_code object::stop(object::Istate* _object) noexcept
{
	// check)
	if(_object == nullptr)
		return true;

	// declare)
	result_code result;

	// 1) Child node를 stop한다. (뒤에서 부터 앞으로 stop)
	auto* pstartable = dynamic_cast<object::Istartable*>(_object);

	if(pstartable != nullptr)
		result = pstartable->stop();
	else
		result = _stop_child_only(_object);

	// return)
	return result;
}

result_code _destroy_child_only(object::Istate* _object)
{
	// 1) eOBJECT_STATE::STOPPED 상태일 때만 eOBJECT_STATE::DESTROYING_PENDING로 상태를 바꾼다.
	bool is_changed = _object->set_object_state_if(eOBJECT_STATE::STOPPED, eOBJECT_STATE::DESTROYING_PENDING);

	// check) 상태가 바뀌지 않았을 경우에는 false를 리턴하며 끝낸다.
	if(is_changed == false)
		return result_code(eRESULT::INVALID_STATE);

	// declare) 
	result_code result = eRESULT::SUCCESS;

	// 2) 모든 자식 객체들을 stop한다.
	auto iter = _object->m_container.rbegin();
	auto iter_end = _object->m_container.rend();

	for (; iter != iter_end; ++iter)
	{
		// - push wait info
		_object->m_queue_pended.emplace_back(object::Istate::WAIT_INFO{ *iter, eRESULT::PENDED });

		// - start child
		result_code result_nested = object::destroy(*iter);

		if (result_nested != eRESULT::PENDED)
		{
			if (_object->m_queue_pended.empty() == false && _object->m_queue_pended.back().pobject == *iter)
				_object->m_queue_pended.pop_back();
		}
		else
		{
			result = eRESULT::PENDED;
		}
	}

	// 3) 상태를 eOBJECT_STATE::NONE로 변경한다.
	if (result != eRESULT::PENDED)
		_object->set_object_state(eOBJECT_STATE::NONE);

	// return)
	return result;
}

result_code object::destroy(object::Istate* _object) noexcept
{
	// check)
	if(_object == nullptr)
		return true;

	// 1) 먼저 stop를 한다
	object::stop(_object);

	// declare)
	result_code result;

	// 2) 자신 destroy
	auto* pdestroyable = dynamic_cast<object::Idestroyable*>(_object);

	// 3) Child node를 destroy한다. (뒤에서 부터 앞으로)
	if(pdestroyable != nullptr)
		result = pdestroyable->destroy();
	else
		result = _destroy_child_only(_object);

	// return)
	return result;
}

result_code object::pause(object::Istate* _object)
{
	// check)
	if(_object == nullptr)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// declare)
	result_code result;

	// 1) 자신 Pause
	auto* ppausable = dynamic_cast<object::Ipausable*>(_object);

	// 2) Child node를 Pause한다. (뒤에서 부터 앞으로)
	if (ppausable != nullptr)
	{
		result = ppausable->pause();
	}
	else
	{
		// - Service상태를 pause의 Pending상태로 바꾼다.
		bool is_changed = _object->set_object_state_if(eOBJECT_STATE::RUNNING, eOBJECT_STATE::PAUSE_PENDING);

		// check) 상태가 바뀌지 않았을 경우에는 false를 리턴하며 끝낸다.
		RETURN_IF(is_changed == false, result_code(eRESULT::INVALID_STATE));

		auto iter = _object->m_container.rbegin();
		auto iter_end = _object->m_container.rend();

		for (; iter != iter_end; ++iter)
		{
			object::pause(*iter);
		}

		// - 상태를 eOBJECT_STATE::NONE로 변경한다.
		_object->set_object_state(eOBJECT_STATE::PAUSED);

		// - Return값은 true로...
		result = eRESULT::SUCCESS;
	}

	// return)
	return result;
}

result_code object::resume(object::Istate* _object)
{
	// check)
	if(_object == nullptr)
		throw throwable::failure(eRESULT::INVALID_ARGUMENT);

	// declare)
	result_code result;

	// 1) 자신 resume
	auto* ppausable = dynamic_cast<object::Ipausable*>(_object);

	// 2) Child node를 resume한다. (뒤에서 부터 앞으로)
	if (ppausable != nullptr)
	{
		result = ppausable->resume();
	}
	else
	{
		// - eOBJECT_STATE::STOPPED 상태일 때만 eOBJECT_STATE::DESTROYING_PENDING로 상태를 바꾼다.
		bool is_changed = _object->set_object_state_if(eOBJECT_STATE::PAUSED, eOBJECT_STATE::CONTINUE_PENDING);

		// check) 상태가 바뀌지 않았을 경우에는 false를 리턴하며 끝낸다.
		RETURN_IF(is_changed == false, result_code(eRESULT::INVALID_STATE));

		auto iter = _object->m_container.rbegin();
		auto iter_end = _object->m_container.rend();

		for (; iter != iter_end; ++iter)
			object::resume(*iter);

		// - 상태를 eOBJECT_STATE::NONE로 변경한다.
		_object->set_object_state(eOBJECT_STATE::RUNNING);

		// - return값은 true로...
		result = eRESULT::SUCCESS;
	}

	// return)
	return result;
}

bool object::Istate::process_attach(object_ptr<Istate>&& _child)
{
	// check) nullptr이면 안 된다.
	CGASSERT_ERROR(_child.exist());

	// check) _child가 nullptr이면 안 된다.
	if(_child.empty())
		return false;

	// 1) 이미 존재하는지 찾는다.
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) 이미 존재하면 그냥 ASSERT 발생!
	CGASSERT_ERROR(iter_find == this->m_container.end());

	// check) 이미 존재하면 그냥 false를 리턴한다.
	if(iter_find != this->m_container.end())
		return false;

	// 2) 추가하기
	this->m_container.emplace_back(std::move(_child));

	// return) 
	return true;
}

bool object::Istate::process_detach(object::Istate* _child) noexcept
{
	// check) No nullptr!
	if(_child == nullptr)
		return this->clear() != 0;

	// check) No Self!
	CGASSERT_ERROR(_child != dynamic_cast<object::Istate*>(this));

	// 1) Check already exists
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) find?
	if(iter_find == this->m_container.end())
		return false;

	// 2) Delete found item
	this->m_container.erase(iter_find);

	// return) 
	return true;
}

CGDK::object_ptr<object::Istate> object::Istate::find_attachable(object::Istate* _child) noexcept
{
	// 1) 먼저 찾는다.
	auto iter_find = std::find(this->m_container.begin(), this->m_container.end(), _child);

	// check) 
	if(iter_find == this->m_container.end())
		return {};

	// return) 
	return (*iter_find);
}

int object::Istate::clear() noexcept
{
	// declare) 
	std::vector<object_ptr<object::Istate>> vec_temp;

	// 1) m_container에 복사
	vec_temp.assign(this->m_container.begin(), this->m_container.end());

	// declare)
	const int count = static_cast<int>(vec_temp.size());

	// 2) 모두 detach
	for(auto& iter:vec_temp)
		this->process_detach(iter);

	// return) 
	return count;
}


}