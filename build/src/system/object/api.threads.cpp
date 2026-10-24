#include "pch.h"

using namespace CGDK;


void api::threads::process_begin_thread_object(thread* /*_pthread*/) noexcept
{
	// 1) Thread수를 증가시킨다.
	++this->m_count_thread_exist;
}

void api::threads::process_end_thread_object(thread* _pthread) noexcept
{
	{
		// lock)
		std::lock_guard lock(this->m_lockable_vector_thread);

		// 1) 찾는다.
		auto iter_find = std::find(this->m_vector_thread.begin(), this->m_vector_thread.end(), _pthread);

		// check) 찾지 못했으면 끝낸다.
		if(iter_find == this->m_vector_thread.end())
			return;

		// 2) 찾은 객체를 제거한다.
		this->m_vector_thread.erase(iter_find);
	}

	// 3) Thead 수를 줄인다.
	auto count = --this->m_count_thread_exist;

	// check)
	CGASSERT_ERROR(count >= 0);

	// check)
	if(count < 0)
		return;

	// 4) Release
	this->release();
}

//! @brief 스레드의 종료 설정 후 모든 스레드가 종료될 때까지 대기한다.
bool api::threads::wait()
{
	// 1) Get All threads
	std::vector<object_ptr<api::thread>> vector_threads;

	// 2) get all threads
	{
		// lock)
		std::lock_guard lock(this->m_lockable_vector_thread);

		vector_threads = this->m_vector_thread;
	}

	// 2) join all thread
	for(auto& iter: vector_threads)
		iter->wait();

	// return)
	return true;
}

void api::threads::stop()
{
	// 1) get all threads
	//    - copy and unlock : an ending thread removes itself from the list in process_end_thread_object() with the same lock
	std::vector<object_ptr<api::thread>> vector_threads;
	{
		// lock)
		std::lock_guard lock(this->m_lockable_vector_thread);

		vector_threads = this->m_vector_thread;
	}

	// 2) request stop to all threads first (they stop in parallel)
	for(auto& iter: vector_threads)
		iter->request_stop();

	// 3) wait for all threads to finish (each thread removes itself from the list and releases this object)
	for(auto& iter: vector_threads)
		iter->wait();
}
