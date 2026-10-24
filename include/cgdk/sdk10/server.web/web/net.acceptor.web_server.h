//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                             CGDK10.server.web                             *
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
// CGDK::net::acceptor_web_server<TSOCKET>
//
//  1) acceptor for general secure(ssl) tcp socket
//
//-----------------------------------------------------------------------------
template <class TSOCKET>
class net::acceptor_web_server :
// inherited classes) 
	public						net::acceptor<TSOCKET>,
	virtual public				server::web::Ientitable
{
// constructor/destructor) 
public:
			acceptor_web_server()
			{
				// check) 
				static_assert(std::is_base_of_v<net::io::Nhttp_server, TSOCKET>, "TSOCKET must inherit 'net::io::Nhttp_server' interface");

				std::string temp_name("acceptor.http<");
				temp_name += typeid(TSOCKET).name();
				temp_name += ">"sv;

				this->name(temp_name);
			}
	virtual	~acceptor_web_server() noexcept {}
};


}
