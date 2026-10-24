#include "pch.h"


area::entity_player::entity_player()
{
}

area::entity_player::~entity_player() noexcept
{
}

void area::entity_player::process_entity_serialize_out(shared_buffer& _buf)
{
	// 1) append entity info
	SCOPED_LOCK(this->cs_area_entity)
	{
		_buf.append<sAREA_ENTITY_INFO>(this->area_entity_info);
		_buf.append<sAREA_ENTITY_STATE>(this->area_entity_status);
		_buf.append<buffer_view>(this->area_entity_last);
	}
	// 2) append entity_tag info
	SCOPED_LOCK(this->paccount->cs_account_info)
	{
		_buf.append<sENTITY_TAG>(this->paccount->account_info.tag);
	}
}
