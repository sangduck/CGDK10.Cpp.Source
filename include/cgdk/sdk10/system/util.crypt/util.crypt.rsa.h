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
//
//-----------------------------------------------------------------------------
namespace crypt
{
namespace rsa	
{
	// definitions) 
	struct public_key_t
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string			key;
	};

	struct private_key_t
	{
		ENABLE_STRUCT_SERIALIZABLE

		std::string			key;
	};

	struct key_pair_t
	{
		public_key_t	public_key;
		private_key_t	private_key;
	};

	// 1. functions
	[[nodiscard]] private_key_t generate_private_key(int _bits_key_length = 2048);
	[[nodiscard]] public_key_t	generate_public_key(const private_key_t& _private_key);
	[[nodiscard]] key_pair_t	generate_key_pair(int _bits_key_length = 2048);

	[[nodiscard]] shared_buffer encrypt(const public_key_t& _encryption_key, buffer_view _source);
	bool						encrypt(buffer& _dest, const public_key_t& _encryption_key, buffer_view _source);
	[[nodiscard]] shared_buffer encrypt(const private_key_t& _encryption_key, buffer_view _source);
	bool						encrypt(buffer& _dest, const private_key_t& _encryption_key, buffer_view _source);

	[[nodiscard]] shared_buffer decrypt(const public_key_t& _decryption_key, buffer_view _source);
	bool						decrypt(buffer& _dest, const public_key_t& _decryption_key, buffer_view _source);
	[[nodiscard]] shared_buffer decrypt(const private_key_t& _decryption_key, buffer_view _source);
	bool						decrypt(buffer& _dest, const private_key_t& _decryption_key, buffer_view _source);

	// 2.encoder/decoder
	object_ptr<Iencryptor>		make_encryptor(const public_key_t& _pem_rsa_key);
	object_ptr<Idecryptor>		make_decryptor(const private_key_t& _pem_rsa_key);

	// 3. cryptor
	object_ptr<Icryptor>		make_cryptor(const key_pair_t& _pair_rsa_key);
}

}
}