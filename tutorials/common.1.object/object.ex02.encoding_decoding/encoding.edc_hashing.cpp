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
// 1. EDC(Error Detection Code, 에러 검출 코드)
//    
//    - 원본 데이터의 오류나 변조 여부를 확인할 수 있는 코드
//      송수신 중 변조가 있는지만 확인하는 코드이다.
//    - 해시 기반 알고리즘은 원래의 데이터를 직접 전달하지 않고 해시 코드만 전달해서 인증이 가능하므로
//      암호문이 전송 중 채널에서 유출될 가능성이 전혀 없는 특성이 있어 인증만을 목적으로 할 경우 많이 사용된다.
//    - 에러검출 및 해시코드는 원본 데이터를 변조하지는 않는다.
//    - checksum, CRC, MD5, SHA 등이 있다.
// 
//	  1) CRC (Cyclic Redundancy check)
//	     - 데이터의 변조 여부를 확인하는 용도로 많이 사용된다.
//	     - 비교적 추가 데이터량이 적고 빠르기 때문에 실시간 데이터 처리에 많이 사용 된다.
// 
//	  2) MD5 (Message Diegest-5)
//	     - 대규모 원본 데이터 대한 지문(Finger print)값을 만들어 내는 알고리즘으로
//	       크기는 128bit(16byte) 크기의 지문 코드를 만들어 낸다.
//	       원본 데이터의 작은 변화만으로도 MD5는 큰 변화가 일어나 큰 데이트의 동일여부를
//	       판단하는 용도로 많이 사용된다.
//	       MD5는 알고리즘의 결함으로 인해 보안이 필요한 인증 용도로는 많이 사용되지
//	       않으며 파일을 전송하거나 패치서버에서 많이 사용된다.
//	       (인증 용도로는 SHA 알고리즘을 사용하길 권장한다.)
//	  
//	  3) SHA-256 (Secure Hash Algorithm-256)
//	     - 기능이나 역할은 MD5와 같으나 보다 보안성이 뛰어난 해시 알고리즘으로 TLS, SSL 등 
//	       많은 보안 프로토콜에 많이 사용된다.
//	     - SHA1은 취약점이 발견되어 이후 SHA2(SHA-224, SHA-256, SHA-386, SHA-512)로 업그레이드 되었다.
// 
//    4) HMAC (Hash based Message Authentication Code)
//       - 해시 기반 메시지 인증 코드은 해시 알고리즘(MD5, SHA..)를 기반으로 인증을 하기 위한 형식이다.
//         데이터 처리가 비싸고 복잡한 면이 있어 제한적으로 사용할 필요가 있다.
// 
//    5) bcrypt
//       - 인증처리에 사용되는 암호의 저장 및 인증 처리에 많이 사용되는 해싱 기반 형식이다.
// 
//-----------------------------------------------------------------------------
void tutorial_common_edc_hashing()
{
	// ---------------------------------------------------------------------
	// 1. EDC(Error Detection Code, 에러 검출 코드)
	// ---------------------------------------------------------------------
	// 1) CRC (Cyclic Redundancy check)
	{
		// - 임시 버퍼를 할당받는다.
		shared_buffer buf_temp = alloc_shared_buffer(100);

		// - 버퍼에 문자열을 넣는다.
		buf_temp.append_text("Temp test data."sv);

		// - 이 버퍼의 crc값을 얻는다.
		auto crc_value = edc::default_crc.get_crc(buf_temp);

		// trace) 
		std::cout << "crc value is "sv << crc_value << '\n';

		// - 이 얻은 crc값을 데이터의 끝에 붙인다.
		buf_temp.append<uint32_t>(crc_value);

		// - 얻은 crc를 끝에 붙인 후 다시 crc값을 얻으면 0이 되어야 한다.
		crc_value = edc::default_crc.get_crc(buf_temp);

		// trace) 
		std::cout << "crc value is "sv << crc_value << '\n';

		// check)
		CGASSERT_ERROR(crc_value == 0);
	}

	// ---------------------------------------------------------------------
	// 2. 해시 코드 (Hashing code)
	// ---------------------------------------------------------------------
	// 1) MD5 (Message Diegest-5)
	{
	}

	// 2) SHA-256 (Secure Hash Algorithm-256)
	{
		// - 임시 버퍼를 할당받는다.
		shared_buffer buf_temp = alloc_shared_buffer(100);

		// - 버퍼에 문자열을 넣는다.
		buf_temp.append_text("Temp test data."sv);

		auto value_sha256 = crypt::sha256(buf_temp);

		// trace) 
		std::cout << "sha256 value is " << value_sha256.to_string<char>() << '\n';
	}

	// 3) HMAC (Hash based Message Authentication Code)
	{
		// - 원문 데이터와 키
		buffer_view	buf_source = to_buffer_view("Temp test data."sv);
		buffer_view buf_key	= to_buffer_view("xxfdafdsafs"sv);

		// - HMAC로 해쉬 값을 구한다.
		auto value_sha256 = crypt::hmac_sha256(buf_source, buf_key);

		// - sha256값을 출력한다.
		std::cout << "HMAC-SHA256 value is "sv << value_sha256.to_string<char>() << '\n';
	}

	// 4) bcrypt
	{
		// - 암호문
		std::string password = "top_secret"s;

		// - 암호를 해싱한다.
		std::string hashed_password = crypt::bcrypt::generate_hash(password);

		// trace) 
		std::cout << "hash: "sv << hashed_password << std::endl;

		// - 맞는 암호와 비교했을 때
		std::cout << "\"" << password << "\" : "sv << crypt::bcrypt::check_password(password, hashed_password) << std::endl;

		// - 틀린 암호와 비교했을 때
		std::cout << "\"wrong\" : "sv << crypt::bcrypt::check_password("wrong"sv, hashed_password) << std::endl;
	}
}