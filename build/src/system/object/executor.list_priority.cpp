#include "pch.h"

namespace CGDK
{

//executor::list_priority::list_priority(Iexecutor* _pexecutor) noexcept : m_psystem_executor(_pexecutor)
//{
//	// 1) set object type
//	this->set_object_type(eOBJECT_TYPE::EXECUTOR_LIST_PRIORITY);
//
//	// 2) prepare priority queue
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		this->m_priorityqueue_executable.clear();
//		this->m_priorityqueue_executable.reserve(8192);
//		this->m_priorityqueue_executable.emplace_back();
//	}
//
//	// 3) register to factory manager
//	this->process_register_executor();
//}
//
//executor::list_priority::list_priority(object_ptr<Iexecutor>&& _pexecutable) noexcept : m_psystem_executor(std::move(_pexecutable))
//{
//	// 1) set object type
//	this->set_object_type(eOBJECT_TYPE::EXECUTOR_LIST_PRIORITY);
//
//	// 2) prepare priority queue
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		this->m_priorityqueue_executable.clear();
//		this->m_priorityqueue_executable.reserve(8192);
//		this->m_priorityqueue_executable.emplace_back();
//	}
//
//	// 3) register to factory manager
//	this->process_register_executor();
//}
//
//executor::list_priority::~list_priority() noexcept
//{
//	// 1) stop
//	executor::list_priority::stop();
//
//	// 2) unregister to factory manager
//	this->process_unregister_executor();
//}
//
//result_code executor::list_priority::start(context& /*_context*/)
//{
//	return result_code(eRESULT::SUCCESS);
//}
//
//result_code executor::list_priority::stop() noexcept
//{
//	// 1) 모두 할당해제해준다.(이건 소멸자에서 반드시 해줘야 한다. 아니면 소멸의 순서 때문에 뻑날 수 있다.)
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		this->m_priorityqueue_executable.clear();
//
//		this->m_priorityqueue_executable.emplace_back();
//	}
//
//	// return)
//	return result_code(eRESULT::SUCCESS);
//}
//
//bool executor::list_priority::register_executable(Iexecutable* _pexecutable)
//{
//	// check) 
//	RETURN_IF(_pexecutable == nullptr, false);
//
//	// 1) List에 추가한다.
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		// 1) 위치 저장해 놓음.
//		size_t position = this->m_priorityqueue_executable.size();
//
//		// 2) 데이터를 Push한다.
//		this->m_priorityqueue_executable.emplace_back();
//
//		// 3) Heap Up한다.
//		object_ptr<Iexecutable>* pexecutable_pre = &this->m_priorityqueue_executable.back();
//
//		while(position != 1)
//		{
//			// - 나누기 2한다.
//			position >>= 1;
//
//			object_ptr<Iexecutable>& Now = this->m_priorityqueue_executable.at(position);
//
//			// check) 
//			BREAK_IF(Now->m_tick_time <=_pexecutable->m_tick_time);
//
//			// Swap한다.
//			*pexecutable_pre = std::move(Now);
//
//			// - 교체...
//			pexecutable_pre = &Now;
//		}
//
//		*pexecutable_pre = _pexecutable;
//	}
//	
//	// return) 성공~
//	return true;
//}
//
//bool executor::list_priority::register_executable(object_ptr<Iexecutable>&& _pexecutable)
//{
//	// check) 
//	RETURN_IF(_pexecutable.empty(), false);
//
//	// 1) List에 추가한다.
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		// 1) 위치 저장해 놓음.
//		size_t position = this->m_priorityqueue_executable.size();
//
//		// 2) 데이터를 Push한다.
//		this->m_priorityqueue_executable.emplace_back();
//
//		// 3) Heap Up한다.
//		object_ptr<Iexecutable>* pexecutable_pre = &this->m_priorityqueue_executable.back();
//
//		while(position != 1)
//		{
//			// - 나누기 2한다.
//			position	>>= 1;
//
//			object_ptr<Iexecutable>& Now = this->m_priorityqueue_executable.at(position);
//
//			// check) 
//			BREAK_IF(Now->m_tick_time <= _pexecutable->m_tick_time);
//
//			// Swap한다.
//			*pexecutable_pre = std::move(Now);
//
//			// - 교체...
//			pexecutable_pre = &Now;
//		}
//
//		*pexecutable_pre = std::move(_pexecutable);
//	}
//
//	// return) 성공~
//	return true;
//}
//
//bool executor::list_priority::unregister_executable(Iexecutable* _pexecutable) noexcept
//{
//	// check) 
//	RETURN_IF(_pexecutable==nullptr, false);
//
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		// 1) 해당 객체를 찾는다.
//		auto iter_end = this->m_priorityqueue_executable.end();
//		auto iter = std::find(this->m_priorityqueue_executable.begin(), iter_end, _pexecutable);
//
//		// check) 못찾았으면 끝낸다.
//		RETURN_IF(iter==iter_end, false);
//
//		// 2) 마지막 객체를 얻는다.
//		object_ptr<Iexecutable>	 ptarget = this->m_priorityqueue_executable.back();
//		auto qwCompare = ptarget->m_tick_time;
//
//		// 3) 객체의 Index를 얻는다.
//		size_t iIndex = iter-m_priorityqueue_executable.begin();
//
//		// 4) Target객체를 제거하고 제일 마지막 객체를 그 위치로 가져 온다.
//		this->m_priorityqueue_executable.pop_back();
//
//		// check) 
//		RETURN_IF(this->m_priorityqueue_executable.size()<2, true)
//
//		// CaseA) Heap Up
//		if(this->m_priorityqueue_executable.at(iIndex>>1)->m_tick_time >qwCompare)
//		{
//			// - Heap Up한다.
//			auto iter_pre = iter;
//
//			while(iIndex!=1)
//			{
//				// - 나누기 2한다.
//				iIndex >>= 1;
//
//				auto iter_now = this->m_priorityqueue_executable.begin()+iIndex;
//
//				// check) 
//				BREAK_IF((*iter_now)->m_tick_time <= qwCompare);
//
//				// Swap한다.
//				*iter_pre = std::move(*iter_now);
//
//				// - 교체...
//				iter_pre = iter_now;
//			}
//
//			*iter_pre = std::move(ptarget);
//		}
//		// CaseB) Heap Down
//		else
//		{
//			// 1) 
//			// 3) Size & Parent
//			size_t iSize = this->m_priorityqueue_executable.size()-1;
//			size_t iChild = (iIndex<<2);
//			auto iter_parent = iter;
//
//			// 4) Heap down
//			while(iChild <= iSize)
//			{
//				// - Child를 얻는다.
//				auto iter_child = this->m_priorityqueue_executable.begin()+iChild;
//
//				// - Left가 더 크면 Right 선택한다.
//				if(iChild<iSize && (*iter_child)->m_tick_time>(*(iter_child+1))->m_tick_time)
//				{
//					++iter_child;
//				}
//
//				// check) Child가 더 크면 끝냄.
//				BREAK_IF(qwCompare<=(*iter_child)->m_tick_time);
//
//				// - 복사
//				*iter_parent = std::move(*iter_child);
//
//				// - Parent를 복사...
//				iter_parent = iter_child;
//
//				// iChild = iChild x 2
//				iChild <<=1;
//			}
//
//			*iter_parent = std::move(ptarget);
//		}
//	}
//
//	// return) 성공!!!
//	return true;
//}
//
//void executor::list_priority::unregister_executor_all() noexcept
//{
//	// locked) 
//	{
//		// lock) 
//		std::lock_guard< lock_object> cs(const_cast<lock_object>(this->m_priorityqueue_executable));
//
//		// 1) clear
//		this->m_priorityqueue_executable.clear();
//	}
//}
//
//bool executor::list_priority::process_post(object_ptr<Iexecutable>&& /*_pexecutable*/)
//{
//	// check)
//	CGASSERT_ERROR(false)
//
//	// return) 종료~
//	return true;
//}
//
//bool executor::list_priority::process_execute(chrono::tick::duration /*dwMilliseconds*/, intptr_t /*_option*/)
//{
//	// 1) executor를 받아놓는다.
//	object_ptr<Iexecutor> psystem_executor = this->m_psystem_executor;
//
//	// check) executor가 없다면.. 끝냄...
//	RETURN_IF(psystem_executor.empty(), false);
//
//	// 2) 현재 tick을 구한다.
//	auto tick_now = chrono::tick::now();
//
//	SCOPED_LOCK(this->m_priorityqueue_executable)
//	{
//		while(this->m_priorityqueue_executable.size()>1)
//		{
//			// 3) 제일 앞의 Executable을 얻는다.
//			object_ptr<Iexecutable>& pexecutable = this->m_priorityqueue_executable.at(1);
//
//			// check) 시간이 지나지 않았으면 끝낸다.
//			BREAK_IF(pexecutable->m_tick_time > tick_now);
//
//			// 4) 실행한다.
//			psystem_executor->post(std::move(pexecutable));
//
//			// 5) 제일 앞의 Executable을 제거한다.
//			this->heap_down_executable();
//
//			// statistics)
//			++this->m_statistics.now.count_execute;
//		}
//	}
//
//	// return)
//	return true;
//}
//
//size_t executor::list_priority::count() const noexcept
//{
//	// locked) 
//	{
//		// lock) 
//		std::lock_guard cs(this->m_lockable_priorityqueue_executable);
//
//		return this->m_priorityqueue_executable.size();
//	}
//}
//
//void executor::list_priority::heap_down_executable() noexcept
//{
//	// locked) 
//	{
//		// lock)
//		std::lock_guard< lock_object> cs(this->m_lockable_priorityqueue_executable);
//
//		// check) 
//		RETURN_IF(this->m_priorityqueue_executable.size() < 2)
//
//			// 1) 
//			object_ptr<Iexecutable>	ptarget = this->m_priorityqueue_executable.back();
//		auto qwCompare = ptarget->m_tick_time;
//
//		// 2) size & parent
//		size_t iSize = this->m_priorityqueue_executable.size() - 1;
//		size_t iChild = 2;
//		auto  iter_parent = this->m_priorityqueue_executable.begin() + 1;
//
//		// 3) heap down
//		while (iChild <= iSize)
//		{
//			// - child를 얻는다.
//			auto iter_now = this->m_priorityqueue_executable.begin() + iChild;
//
//			// - left가 더 크면 Right 선택한다.
//			if (iChild<iSize && (*iter_now)->m_tick_time >(*(iter_now + 1))->m_tick_time)
//			{
//				++iter_now;
//			}
//
//			// check) child가 더 크면 끝냄.
//			BREAK_IF(qwCompare <= (*iter_now)->m_tick_time);
//
//			// - 복사
//			*iter_parent = std::move(*iter_now);
//
//			// - Parent를 복사...
//			iter_parent = iter_now;
//
//			// iChid = iChid x 2
//			iChild <<= 1;
//		}
//
//		*iter_parent = std::move(ptarget);
//	}
//}


}