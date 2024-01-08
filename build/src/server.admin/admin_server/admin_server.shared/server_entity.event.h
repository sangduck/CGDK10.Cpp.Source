#pragma once

namespace server_entity
{

class server_event : virtual public time_event::Ientity
{
public:
			server_event();
	virtual ~server_event();

public:

public:
	// 5) Event °´Ã¼¸¦ ´Ý´Â´Ù.
	//virtual	void				process_close_time_event() noexcept override {}
	virtual	intptr_t			process_execute(_In_ intptr_t, _In_ size_t) override { return 0; }
};


}