//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          server.query.rdb.odbc                            *
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


#define DEFINE_TABLE(name)	public: using enable_struct_db_schema = std::true_type;
#define DEFINE_COLUMN(type, name, attribute)	static constexpr const std::string_view column_##name = "##name";
#define DEFINE_PK(index)
#define DEFINE_INDEX(index)
#define DEFINE_CONSTRAINT_FK(index)

namespace CGDK
{
//-----------------------------------------------------------------------------
//
// query::rdb::schema
//
// 
//-----------------------------------------------------------------------------
	namespace rdb
	{
		class table_definition
		{
		public:
			std::string			create() { return {}; }
			std::string			drop() { return {}; }
			std::string			insert() { return {}; }

		private:
			std::vector<int>	vector_definition_column;
		};
	}
}