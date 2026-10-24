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
void tutorial_common_compression();
void tutorial_common_rsa();
void tutorial_common_aes();
void tutorial_common_pncode();
void tutorial_common_edc_hashing();
void tutorial_common_etc();

//-----------------------------------------------------------------------------
//
// tutorial common.ex02 - encoding / decoding
//
//-----------------------------------------------------------------------------
int main()
{
	// 1) 압축 예제
	tutorial_common_compression();

	// 2) 암호화 - RSA 예제
	tutorial_common_rsa();

	// 3) 암호화 - AES 예제
	tutorial_common_aes();

	// 4) 암호화 - pn-code
	tutorial_common_pncode();

	// 5) EDC - CRC, Hasing 알고리즘들 예제
	tutorial_common_edc_hashing();

	// 6) 기타 - Base64 등 예제
	tutorial_common_etc();

	// return) 
	return	0;
}