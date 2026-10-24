//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                        network socket http classes                        *
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
//
// net::acceptor<TSOCKET> - for http socket
//
//-----------------------------------------------------------------------------
template <class TSOCKET>
class net::acceptor<TSOCKET, typename std::enable_if<std::is_base_of<net::io::Nhttp_server, TSOCKET>::value>::type> :
	public						net::io::connective::acceptor::Ntcp_ssl,
	public						Nmessage_transmitter,
	public						net::io::connectable_manager::list<TSOCKET>
{
public:
			acceptor() 
			{
				// check) 
				static_assert(std::is_base_of_v<net::io::Nsocket_ssl, TSOCKET>, "secured socket must have 'net::io::Nsocket_ssl' interface (derive 'net::io::Nsocket_ssl')");

				std::string temp_name("acceptor.http<");
				temp_name += typeid(TSOCKET).name();
				temp_name += ">"sv;

				this->name(temp_name);
			}
	virtual	~acceptor() noexcept {}

protected:
	virtual	object_ptr<net::io::Iconnectable> process_alloc_connectable() override { return alloc_object<TSOCKET>(); }
	virtual result_code			_process_destroy() noexcept override { auto result = net::io::connective::acceptor::Ntcp_ssl::_process_destroy(); reset_message_transmitter(); return result; }
	virtual	uint16_t			process_get_default_port() noexcept override { return this->get_ssl_context().exist() ? 443 : 80; }
};


}
