//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              object classes                               *
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

namespace CGDK
{
//-----------------------------------------------------------------------------
/**

 @class		TLOG_RECORD


*/
//-----------------------------------------------------------------------------
class TLOG_RECORD : 
	public						LOG_RECORD,
	public						Ipoolable<TLOG_RECORD>
{
public:
	static object_ptr<TLOG_RECORD> _alloc();

	virtual	void				process_dispose() noexcept;

	// 2) Object
	object_ptr<default_factory_pool_t<TLOG_RECORD>> m_ppool;
};

extern own_ptr<default_factory_pool_t<TLOG_RECORD>> g_factory_log_record;

inline object_ptr<TLOG_RECORD> TLOG_RECORD::_alloc()
{
	return g_factory_log_record->alloc(CGNEW_DEBUG_INFO_PARAMETERS_INITIAL);
}

inline void TLOG_RECORD::process_dispose() noexcept
{
	if (m_ppool.exist())
		m_ppool->dealloc(this);
	 else
		delete this;
}


}