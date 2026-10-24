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
// AES (Advanced Encryption Standard)
// 
//    - AES('발전된 암호화 표준')은 취약점이 발견된 DES를 대체하기 위해 발표된 
//      대칭키 암호 알고리즘으로 강력한 보안력과 비교적 빠른 성능을 가지고 있다.
//      또 블록 암호화 알고리즘으로 여러 번 AES를 중복으로 적용해 더 강력한
//      암호화가 가능하다.
//    - AES 암호화 알고리즘은 하드웨어 암복호화를 지원해 주기도 한다.
//    - 이런 특성으로 다양한 분야에 많이 사용하는 알고리즘이긴 하지만
//      여전히 부하를 무시할 수 없으므로 신중한 사용이 필요하다.
//
//-----------------------------------------------------------------------------
void tutorial_common_aes()
{
	// - 테스트용 256 bit key 와 128 bit IV 값
	crypt::aes_key key 
	{
		to_buffer_view( "01234567890123456789012345678901"sv),
		to_buffer_view( "0123456789012345"sv)
	};

	// - 원문 & 추가 데이터
	buffer_view plain_text = to_buffer_view("this is CGCII cryption test code and tutorials"sv);

	// - 암호화된 데이터를 저장할 버퍼
	auto cipher_text = alloc_shared_buffer(256) ^ 256;

	// * 암호화한다.
	auto result_crypt = crypt::aes::encrypt(cipher_text, key, plain_text);

	// trace) 
	std::cout << "AES encrypted text : "sv << result_crypt.to_string<char>() << '\n';


	// - 복호화한 데이트를 저장할 버퍼를 할당받는다.
	auto buf_decrypted = alloc_shared_buffer(256) ^ 256;

	// * 복호화한다.
	auto result_decrypt = crypt::aes::decrypt(buf_decrypted, key, result_crypt);

	// trace) 
	std::cout << "AES decrypted text : "sv << result_decrypt.to_string<char>() << '\n';
}