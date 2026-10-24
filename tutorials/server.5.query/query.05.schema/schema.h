//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                      tutorials query - oledb.simple                       *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@cgcii.co.kr         *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                (c) 2003. Cho Sanghyun. All right reserved.                *
//*                          http://www.CGCII.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once

using namespace CGDK;

struct db_schema_table1 : rdb::table_definition
{
	DEFINE_TABLE(account);

	DEFINE_COLUMN(int64_t,					uid,			0)
	DEFINE_COLUMN(std::string,				nickname,		0)
	DEFINE_COLUMN(chrono::time::time_point,	date_create,	0)
	DEFINE_COLUMN(chrono::time::time_point,	date_delete,	0)

	DEFINE_PK(uid)
	DEFINE_INDEX(uid)
};