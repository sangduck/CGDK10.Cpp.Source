#include "pch.h"


account::account()
{
}

account::~account() noexcept
{
}

result_code account::destroy() noexcept
{
	SCOPED_LOCK(this->cs_account_info)
	{
		account_info.account_index = 0;
		account_info.tag.name.clear();
		area_index_now = 0;

		parea_entity.reset();
		psocket.reset();
	}

	// return) 
	return eRESULT::SUCCESS;
}
