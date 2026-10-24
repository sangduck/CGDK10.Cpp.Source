//*****************************************************************************
//*                                                                           *
//*                          CGCII mmog field system                          *
//*                                                                           *
//*                               field server                                *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                 ¨Ï 2016 Cho Sanghyun All right reserved.                  *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************
#pragma once


class area::Ientity : virtual public Ireferenceable
{
public:
			lock_object			cs_area_entity;
			sAREA_ENTITY_INFO	area_entity_info;
			sAREA_ENTITY_STATE	area_entity_status;
			shared_buffer		area_entity_last;
			object_ptr<account> paccount;

	virtual void process_entity_serialize_out(shared_buffer& _buf) PURE;
};
