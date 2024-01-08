#include "pch.h"

namespace CGDK
{

bool Nmessage_transmitter_static::process_attach(object_ptr<Imessageable>&& _pmessageable)
{
	return register_messageable_static(std::move(_pmessageable));
}

bool Nmessage_transmitter_static::process_detach(Imessageable* _pmessageable) noexcept
{
	return unregister_messageable_static(_pmessageable)!=0;
}

int Nmessage_transmitter_static::process_reset_message_transmitter() noexcept
{
	return reset_dispatchable_static().exist() ? 1:0;
}

object_ptr<Imessageable> Nmessage_transmitter_static::reset_dispatchable_static() noexcept
{
	// declare) 
	object_ptr<Imessageable> pmessageable;

	// 1) Static Messageable를 얻는다.
	lockable<object_ptr<Imessageable>>&	messageable	 = get_messageable_static();

	scoped_lock(messageable)
	{
		// check) 
		RETURN_IF(messageable.empty(), pmessageable); 

		// 1) 값을 임시로 복사
		pmessageable = std::move(messageable);
	}

	// return)
	return pmessageable;
}

bool Nmessage_transmitter_static::register_messageable_static(object_ptr<Imessageable>&& _pmessageable)	
{
	// check) _pmessageable이 nullptr이면 안됀다!
	RETURN_IF(_pmessageable.empty(), false);

	// 1) 일단 unregister
	unregister_messageable_static(get_messageable_static());

	// 2) Messagable을...
	auto& messageable = get_messageable_static();

	scoped_lock(messageable)
	{
		// check) 
		RETURN_IF(messageable == _pmessageable, false); 

		// 4) Messageable한다~
		messageable = std::move(_pmessageable);
	}

	// return) 
	return true;
}

int Nmessage_transmitter_static::unregister_messageable_static(Imessageable* _pmessageable) noexcept
{
	// check) 
	if(_pmessageable == nullptr)
	{
		// - 모두 다 제거한다.
		return reset_dispatchable_static().exist() ? 1:0;
	}

	// declare) 
	object_ptr<Imessageable> pmessageable;

	// 1) Messageable Static을 얻는다.
	lockable<object_ptr<Imessageable>>&	messageable = get_messageable_static();

	scoped_lock(messageable)
	{
		// check) 제거하려는 Messageable이 아니면 그냥 끝낸다.
		RETURN_IF(messageable!=_pmessageable, 0);
	
		// 1) reset한다.
		pmessageable = std::move(messageable);
	}

	// return) 
	return 1;
}

result_code Nmessage_transmitter_static::process_transmit_message(sMESSAGE& _msg)
{
	// 1) Messageable Static을 얻는다.
	auto& messageable = get_messageable_static();

	// declare) 
	object_ptr<Imessageable> pmessageable;

	// 2) 복사
	scoped_lock(messageable)
	{
		// check) messageable이 nullptr이면 끝낸다.
		RETURN_IF(messageable.empty(), 0);

		// - 값을 복사.
		pmessageable = messageable;
	}

	// 3) Message를 transmit_message한다.
	return pmessageable->request_process_message(_msg);
}


}