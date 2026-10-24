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
// CGDK::crypt::rsa
//
//
//-----------------------------------------------------------------------------
namespace crypt
{
	// 1. functions
	buffer_view					hmac_md5	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_md5	(const_buffer_view _source, const_buffer_view _key);

	buffer_view					hmac_sha1	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_sha1	(const_buffer_view _source, const_buffer_view _key);

	buffer_view					hmac_sha224	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_sha224 (const_buffer_view _source, const_buffer_view _key);

	buffer_view					hmac_sha256	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_sha256 (const_buffer_view _source, const_buffer_view _key);

	buffer_view					hmac_sha384	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_sha384 (const_buffer_view _source, const_buffer_view _key);

	buffer_view					hmac_sha512	(buffer_view _out, const_buffer_view _key, const_buffer_view _source) noexcept;
	[[nodiscard]] shared_buffer	hmac_sha512 (const_buffer_view _source, const_buffer_view _key);
}

}
