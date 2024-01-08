#include "pch.h"

using namespace CGDK;


void api::threads::process_begin_thread_object(thread* /*_pthread*/) noexcept
{
	// 1) Thread수를 증가시킨다.
	++this->m_count_thread_exist;
}

void api::threads::process_end_thread_object(thread* _pthread) noexcept
{
	scoped_lock(this->m_vector_thread)
	{
		// 1) 찾는다.
		auto iter_find = std::find(this->m_vector_thread.begin(), this->m_vector_thread.end(), _pthread);

		// check) 찾지 못했으면 끝낸다.
		RETURN_IF(iter_find == this->m_vector_thread.end());

		// 2) 찾은 객체를 제거한다.
		this->m_vector_thread.erase(iter_find);
	}

	// 3) Thead 수를 줄인다.
	auto count = --this->m_count_thread_exist;

	// check)
	CGASSERT_ERROR(count >= 0);

	// check)
	RETURN_IF(count < 0, );

	// 4) Release
	this->release();
}

//! @brief 쓰레드의 종료 설정 후 모든 쓰레드가 종료될 때까지 대기한다.
bool api::threads::wait()
{
	// 1) 모든 Thread가 완전히 닫힐 때까지 기다린다.
#ifdef NDEBUG
	// declare) 최대 executor 완료 대기시간 (10초)
	const int MAX_LOOP = 1000;

	for(auto iLoop=MAX_LOOP; this->get_existing_thread_count(); --iLoop)
	{
		// check) 
		RETURN_IF(iLoop <= 0, false);
		
		// - 1mss만큼 sleep한다.
		api::sleep_for(1ms);
	}
#else
	while(this->exist())
	{
		// - 1ms만큼 sleep한다.
		api::sleep_for(1ms);
	}
#endif

	// return)
	return true;
}

void api::threads::terminate_all()
{
	// 1) 모두 지운다.
	scoped_lock(this->m_vector_thread)
	{
		for(auto& iter:this->m_vector_thread)
		{
			iter->terminate();
		}

		this->m_vector_thread.clear();
	}

	// 2) 존재하는 Thread수를 0개로 설정한다.
	this->m_count_thread_exist = 0;
}


