//*****************************************************************************
//*                                                                           *
//*                             CGCII Trade System                            *
//*                                                                           *
//*                               Trading Server                              *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                  ¨Ï2017. Cho SangHyun. All rights reserved                *
//*                          http://www.CGCIII.co.kr                          *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
/**

@class		time_event::sSERVER_OPERATION

@brief
@todo
@ref
@details

*/
//-----------------------------------------------------------------------------
class time_event_server::server_operation :
	public						time_event::Ientity,
	public						Ibuffer_serializable
{
public:
			server_operation() noexcept;
	virtual	~server_operation() noexcept;

public:
	virtual result_code			process_event(chrono::time::utc::time_point _time_now);
	virtual	intptr_t			process_execute(intptr_t /*_return*/, size_t /*_param*/) override { return 0; }

	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(basic_buffer& _buffer) override;
	virtual size_t				process_size_of() const override;

public:
			//sSERVER_OPERATION	m_operation;
};
