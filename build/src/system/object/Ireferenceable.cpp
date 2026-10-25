#include "pch.h"


int CGDK::Ireferenceable::release() noexcept
{
	// check) 
	DEBUG_ON_RELEASE_REFERENCECOUNT;

	// 1) decrement counter
	//    (acq_rel : 마지막 release가 다른 소유자들이 해제 전에 쓴 내용을 모두 본 뒤 on_final_release/dispose한다)
	const int result = m_counter.fetch_sub(1, std::memory_order_acq_rel) - 1;

	// check)
	CGASSERT_ERROR(result >= 0);

	if (result == 0)
		_process_on_final_release();

	// return) 
	return result;
}

int CGDK::Ireferenceable::release(int _count) noexcept
{
	// check) 
	CGASSERT_ERROR(_count >= 0);

	// 1) subtract count
	const int result = m_counter.fetch_sub(_count, std::memory_order_acq_rel) - _count;

	// check)
	CGASSERT_ERROR(result >= 0);

	// - release(0) releases nothing : it does not dispose the object (at the count 0, in its final release, it disposed it a second time.
	//   Nmessage_transmitter::process_reset_message_transmitter() releases the count of the messageables, 0 if none)
	if (result == 0 && _count != 0)
		_process_on_final_release();

	// return) 
	return result;
}

void CGDK::Ireferenceable::_process_on_final_release() noexcept
{
	on_final_release();

	CGNEW_DEBUG_INFO_RESET(this);

	process_dispose();
}

