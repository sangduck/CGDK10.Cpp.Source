//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 10.0 / Release 2019.12.11                       *
//*                                                                           *
//*                          Utinity.Crypto Classes                           *
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
// definitions
//
//-----------------------------------------------------------------------------
namespace crypt
{
	// definitions) 
	const int		DIGEST_LENGTH_SHA224 = 28;
	const int		DIGEST_LENGTH_SHA256 = 32;
	const int		DIGEST_LENGTH_SHA384 = 48;
	const int		DIGEST_LENGTH_SHA512 = 64;

	// 1. functions
	bool						sha1	(buffer_view _out, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	sha1	(const_buffer_view _source);

	bool						sha224	(buffer_view _out, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	sha224	(const_buffer_view _source);

	bool						sha256	(buffer_view _out, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	sha256	(const_buffer_view _source);

	bool						sha384	(buffer_view _out, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	sha384	(const_buffer_view _source);

	bool						sha512	(buffer_view _out, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	sha512	(const_buffer_view _source);
}

}
