//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                      Ver 10.0 / Release 2019.12.01                        *
//*                                                                           *
//*                     tutorials common - object_pointer                     *
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

int main()
{
	crypt::rsa::public_key_t public_key;
	public_key.key = "-----BEGIN PUBLIC KEY-----\n\
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuoY8v90U7gjhpePD/MA8\n\
7ygcO5EeZA1yUMU88diHQo6tv8otsV3oVABLOQEBSKbk1tq/mRyoww7WOPWEhccd\n\
OoHadiU1hatMVwzz9tPtowAAAqnqpPjluh5on0WpshFufp+F09fT0KXnayl8hMxk\n\
R5I6FGqQfRs7c4nCuhkh29V207mv3OlNHof77Ag9PySJyWhAA7IFZ8ttC0xrlTTf\n\
mQBTbd8ShMAwezZ5zl3tVKCZvVlRNt158THjGrImADxBGUa5iA8lTjF3lhRxU/4+\n\
QNdZDQHGnXKxSP/II7qQoHu1g9MpEXfOVfzh1yN1j30gV03XDHVkLfuicAjvITQZ\n\
LQIDAQAB\n\
-----END PUBLIC KEY-----";

	auto buf_source = alloc_shared_buffer(1000);
	buf_source.append_text("xxxyyyyzzzzzz\0");


	auto bu_result1 = crypt::rsa::encrypt(public_key, buf_source);


	//const unsigned char* temp_buffer = nullptr;

	//crypt::encoder(temp_buffer, buf_source);



	// Return)
	return	0;
}
