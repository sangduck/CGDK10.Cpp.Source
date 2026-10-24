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
// cryptBase
//
// 1. cryptBase란!
//    1) 암복호화에 관련된...
//
//
//-----------------------------------------------------------------------------
enum class eCRYPTION_TYPE : int32_t
{
	NONE,
	PNCODE,
	DES,
	RSA,
	CSP,
};

class Icryptable;

class Idecryptor : virtual public Ireferenceable
{
public:
	[[nodiscard]] virtual shared_buffer		decrypt(const_buffer_view _source) PURE;
};


class Iencryptor : virtual public Ireferenceable
{
public:
	[[nodiscard]] virtual shared_buffer		encrypt(const_buffer_view _source) PURE;
};

class Icryptor : virtual public Ireferenceable
{
public:
	[[nodiscard]] virtual shared_buffer		encrypt(const_buffer_view _source) PURE;
	[[nodiscard]] virtual shared_buffer		decrypt(const_buffer_view _source) PURE;

	virtual	shared_buffer		initialize() { return {}; }
	virtual	void				initialize(buffer_view /*_initialize_info*/) {}
	virtual	bool				encode(buffer_view&, const const_buffer_view*, int) { return false; }
	virtual	bool				decode(buffer_view&, const_buffer_view*, int) { return false; }
};


}