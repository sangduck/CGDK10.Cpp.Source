//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                           Server Admin Classes                            *
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


class server_system::host::instance_local:
	public						instance
{
public:
	virtual	bool				process_send(shared_buffer&& _buffer, std::size_t _count_message, uint64_t _option) override;

private:
	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;
};

} 
