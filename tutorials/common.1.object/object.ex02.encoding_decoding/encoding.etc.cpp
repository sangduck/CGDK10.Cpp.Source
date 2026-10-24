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
// 기타 코드 변환 (code encoding)
//
//-----------------------------------------------------------------------------
void tutorial_common_etc()
{
	// 1) base64
	//    - base64코드는 binary(이진) 데이터를 ASCII 문자 영역의 문자만으로 인코딩하는 것이다.
	//      binary(이진) 데이터를 문자열로 전송할 때 많이 사용된다.
	{
		// - 임시 버퍼 100 byte를 할당 받는다.
		shared_buffer buf_temp = alloc_shared_buffer(100);

		// - 버퍼에 문자열을 넣는다.
		buf_temp.append_text("Temp test data."sv);

		// - base64로 encoding한다.
		auto str_encoded = crypt::encode_base64(buf_temp);

		// trace)
		std::cout << "base64 encoeded is '"sv << str_encoded << "'\n";

		// - 다시 decode한다.
		auto buf_decoded = crypt::decode_base64(str_encoded);

		// trace)
		std::cout << "base64 encoeded is '"sv << buf_decoded.to_string<char>() << "'\n";
	}
}