//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                         tutorials common - context                        *
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

// 2) header files
#include <iostream>

// 3) namespace
using namespace CGDK;


//-----------------------------------------------------------------------------
//
// step 1) context의 기본 사용 법
//
// 
// 
// 
//
// 
// 
// 
// 
//
//-----------------------------------------------------------------------------
void tutorial_context_step_1_basic()
{
	// Declare) 
	context	temp;

	// 1) Context 작성
	temp["z"][0]		  = 1;
	temp["z"][1]		  = 2;
	temp["z"][2]		  = 3;

	temp["x"][0]["Test"]  = "\uD14C\uC2A4\uD2B8 string";	// "테스트 string"
	temp["x"][0]["value"] = 2;
	temp["x"][0]["exit"]  = 3;

	temp["x"][1]["a"]	  = 5;
	temp["x"][1]["b"]	  = 6;
	temp["x"][1]["c"]	  = 7;
	temp["x"][1]["d"]	  = chrono::time::utc::now();

	// 2) JSON string으로 써넣기
	auto buf = temp.to_json();

	// 3) 화면에 출력한다.
	std::cout << buf.data() << std::endl;
}
