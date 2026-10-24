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

 @class		Iresource
 
*/
//-----------------------------------------------------------------------------
class Iresource : virtual public Iobject, virtual public object::Idestroyable
{
public:
	virtual ~Iresource() noexcept { this->unregister();}

public:
			bool				register_();
			bool				unregister() noexcept;

private:
			lock_object			m_lockable_pmanager;
			object_ptr<resource::manager> m_pmanager;
};


}