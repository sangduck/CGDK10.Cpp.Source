//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          network socket classes                           *
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
// net::connector<TSOCKET> - for general socket
//
//-----------------------------------------------------------------------------
template <class TSOCKET>
class net::connector<TSOCKET, typename std::enable_if<!std::is_base_of<net::io::Nhttp_client, TSOCKET>::value>::type> :
	public						net::io::connective::connector::Ntcp,
	public						net::io::connectable_manager::list<TSOCKET>
{
public:
			connector() 
			{
				std::string temp_name("connector<");
				temp_name += typeid(TSOCKET).name();
				temp_name += ">"sv;

				this->name(temp_name);
			}
	virtual	~connector() noexcept {}

protected:
	[[nodiscard]] virtual object_ptr<net::io::Iconnectable> process_alloc_connectable() override
	{
		return this->_process_alloc_connectable<TSOCKET>();
	}

	template <class NSOCKET>
	[[nodiscard]] static std::enable_if_t<std::is_base_of<net::io::Iconnect_requestable, NSOCKET>::value, object_ptr<TSOCKET>>
	_process_alloc_connectable()
	{
		return alloc_object<NSOCKET>();
	}

	template <class NSOCKET>
	[[nodiscard]] static std::enable_if_t<!std::is_base_of<net::io::Iconnect_requestable, NSOCKET>::value, object_ptr<TSOCKET>>
	_process_alloc_connectable()
	{
		return alloc_object<CONNECTABLE_SOCKET_TCP<NSOCKET>>();
	}
};


}