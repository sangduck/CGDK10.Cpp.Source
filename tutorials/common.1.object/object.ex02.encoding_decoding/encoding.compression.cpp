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
// 압축/복원(Compress/Decompress)
//
//-----------------------------------------------------------------------------
void tutorial_common_compression()
{
	// ---------------------------------------------------------------------
	// zip
	//    - lzw 알고리즘을 사용한 가장 대표적인 비손실 압축알고리즘이다.
	//    - zlib를 사용해 압축과 복원을 한다.
	// ---------------------------------------------------------------------

	// declare) 
	std::string_view data = "Temp test data."sv;

	// trace) 
	std::cout << "original size is "sv << data.size() << '\n';

	// 1) 압축하기.
	auto buf_compressed = compress::zip(to_buffer_view(data));

	// trace) 
	std::cout << "compressed size is "sv << buf_compressed.size() << '\n';

	// 2) 압축해제하기.
	auto buf_uncompressed = compress::unzip(buf_compressed);

	// trace) 
	std::cout << "uncompressed size is "sv << buf_uncompressed.size() << '\n';
}