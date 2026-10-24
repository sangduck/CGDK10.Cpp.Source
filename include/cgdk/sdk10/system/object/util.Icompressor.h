//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                              Utility Classes                              *
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
// Icompressor
//
// 1. Icompressor란!
//    1) 압축기. 압축과 압축해제를 수행하는 interface 클래스
//
//  
//
//-----------------------------------------------------------------------------
class Icompressor : virtual public Ireferenceable
{
public:
	[[nodiscard]] virtual	shared_buffer		compress(const_buffer_view _buf_source) PURE;
	[[nodiscard]] virtual	shared_buffer		decompress(const_buffer_view _buf_source) PURE;
};

}