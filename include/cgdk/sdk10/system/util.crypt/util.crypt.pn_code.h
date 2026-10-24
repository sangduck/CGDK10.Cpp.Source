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
// crypt::pn_code
//
//
//-----------------------------------------------------------------------------
namespace crypt
{
namespace pn_code
{
	constexpr int MIN_BYTES_KEY_LENGTH = 128;
	constexpr int MAX_BYTES_KEY_LENGTH = 256;
	using key_t = shared_buffer;

	[[nodiscard]] key_t		generate_key(int _min_bytes_key_length = MIN_BYTES_KEY_LENGTH, int _max_bytes_key_length = MAX_BYTES_KEY_LENGTH);
	[[nodiscard]] object_ptr<Iencryptor> make_encryptor(const_buffer_view _encrypt_key);
	[[nodiscard]] object_ptr<Idecryptor> make_decryptor(const_buffer_view _decrypt_key);
	[[nodiscard]] object_ptr<Icryptor> make_cryptor(const key_t& _keys_encrypt, const key_t& _keys_decrypt);
}

}
}