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
// step 2) json 파일과 context의 연계
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
void tutorial_context_step_2_json_file()
{
	// Declare) 
	context	temp;

	// 1) JSON파일 읽기
	temp.from_json(api::file::read_file("test_setting.json"));

	// 2) 읽은 JSON 파일 활용하기.
	temp["Text"] = "Save this";

	// 3) JSON을 스트링으로 저장한다.
	shared_buffer buf = temp.to_json();

	// 4) File로 저장한다.
	api::file::write_file(temp.to_json(), "test_setting_updated.json");

	// trace) 화면에 출력한다.
	std::cout << buf.data() << std::endl;
}
