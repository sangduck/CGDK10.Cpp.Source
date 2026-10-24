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

class server_system::service::instance_local :
	public						instance
{
private:
	virtual result_code			process_message(CGDK::sMESSAGE&) override;
	virtual	void				process_serialize_in(buffer_view& _buffer) override;
	virtual	void				process_serialize_out(buffer& _buffer) override;
	virtual std::size_t			process_size_of() const override;
};

template <class TSERVICE_INSTANCE = server_system::service::instance_local>
own_ptr<TSERVICE_INSTANCE> make_service_instance(std::string_view _json_file)
{
	// declare) context
	context	context_service;

	// 1) read json file
	context_service.from_json(_json_file);

	// 2) create service instance
	auto pservice_instance = make_own<TSERVICE_INSTANCE>();

	// 4) load json file
	pservice_instance->load_from(context_service);

	// return)
	return pservice_instance;
}

template <class TSERVICE_INSTANCE = server_system::service::instance_local>
own_ptr<TSERVICE_INSTANCE> make_service_instance(server_system::sRANK _rank)
{
	// declare) context
	context	context_service;

	// 1) create service instance
	auto pservice_instance = make_own<TSERVICE_INSTANCE>();

	// 1) load json file
	pservice_instance->info.setting.rank = _rank;

	// return)
	return pservice_instance;
}

}