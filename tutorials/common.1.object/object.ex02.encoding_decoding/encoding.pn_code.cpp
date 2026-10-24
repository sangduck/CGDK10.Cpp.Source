//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                 tutorials common.ex02 - encoding decoding                 *
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
// 1) CGDK header file
#include "cgdk/sdk10/system.object.h"
#include "cgdk/sdk10/system.util.crypt.h"

// 2) header files
#include <iostream>

// 3) namespace
using namespace CGDK;

//-----------------------------------------------------------------------------
//
//  암호화/복호화 - pn-code
// 
//    - pn-code는 stream data에 적용할 수 있는 강력하면서도 저렴한 암호화 알고리즘.
//    - key(seed)값이 동일하면 동일한 순차적인 값을 generate하는 Psuedo Noise를
//      이용한 암호화방법.
//    - 암호화/복호화는 동일한 key로 초기화를 해야 한다.
//-----------------------------------------------------------------------------

void tutorial_common_pncode_encryptor_decryptor()
{
	//-----------------------------------------------------------------------------
	// 1. pn-code private key/public key Generate하기
	// 
	//-----------------------------------------------------------------------------
	// 1) pncode public/private key를 발급한다.
	auto pncode_key_pair = crypt::pn_code::generate_key();

	// 2) encryptor를 생성한다. (pn-code public key를 사용)
	auto pencryptor = crypt::pn_code::make_encryptor(pncode_key_pair);

	// 3) decryptor를 생성한다. (pn-code private key를 사용)
	auto pdecryptor = crypt::pn_code::make_decryptor(pncode_key_pair);

	//-----------------------------------------------------------------------------
	// 2. 암복호화하기
	//-----------------------------------------------------------------------------
	// declare) 원문
	std::string_view original_data = "Temp test data.";

	// 1) 암호화를 한다.
	auto buf_encrypted = pencryptor->encrypt(original_data);

	// trace) 
	std::cout << "pn-code encrypted code : " << buf_encrypted.to_string<char>() << '\n';

	// 2) 복호화를 한다.
	auto buf_decrypted = pdecryptor->decrypt(buf_encrypted);

	// trace) 
	std::cout << "pn-code decrypted code : " << buf_decrypted.to_string<char>() << '\n';
}

void tutorial_common_pncode_cryptor()
{
	//-----------------------------------------------------------------------------
	// 1. pn-code private key/public key Generate하기
	// 
	//-----------------------------------------------------------------------------
	// 1) pncode public/private key를 발급한다.
	auto pncode_key= crypt::pn_code::generate_key();

	// 2) cryptor를 생성한다. (pn-code public key를 사용)
	auto pcryptor = crypt::pn_code::make_cryptor(pncode_key, pncode_key);

	//-----------------------------------------------------------------------------
	// 2. 암복호화하기
	//-----------------------------------------------------------------------------
	// declare) 원문
	std::string_view original_data = "Temp test data."sv;

	// 1) 암호화를 한다.
	auto buf_encrypted = pcryptor->encrypt(original_data);

	// trace) 
	std::cout << "pn-code encrypted code : " << buf_encrypted.to_string<char>() << '\n';

	// 2) 복호화를 한다.
	auto buf_decrypted = pcryptor->decrypt(buf_encrypted);

	// trace) 
	std::cout << "pn-code decrypted code : " << buf_decrypted.to_string<char>() << '\n';
}

void tutorial_common_pncode()
{
	// 1) pncode encrytor와 decryptor
	tutorial_common_pncode_encryptor_decryptor();

	// 2) pncode cryptor
	tutorial_common_pncode_cryptor();
}
