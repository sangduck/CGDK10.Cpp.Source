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
//  암호화/복호화 - RSA
// 
//    - 일반적으로 공개키 알고리즘 암호화방법으로 유명하다.
//      암호를 하는 키와 복호화를 하는 키가 서로 달라 복호화 키가 공개되더라도
//      원암호문을 변조하지 못하는 특성이 있어 다양한 보안 프로토콜에 사용된다.
//
//    - 하지만 비교적 AES와 같은 대칭기에 비해 많게는 부하가 1000배 가량
//      암복호화 처리 비용이 많이 들어 대량의 데이터나 실시간 데이터의 처리에는
//      적합하지 않으며 인증키나 패스워드와 비교적 짧은 데이터의 전달이나 
//      저장용으로 많이 사용된다.
//      또 각종 인증 및 인증서에 사용되는 핵심 알고리즘 중에 하나이다.
//
//-----------------------------------------------------------------------------

// - 사용되는 키는 console command로 다음과 같이 openssl을 사용해 생성했다.
//  openssl genrsa -out decryption_key.pem 1024
//  openssl rsa -in decryption_key.pem -out encryption_key.pem -pubout

// * 암호화 키 (RSA public key) - "decryption_key.pem"
const char* pem_encryption_key =
"-----BEGIN PUBLIC KEY-----\n"
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDTPh5DvH6YmeLEU1xKg0tk5Zv1\n"
"M/3yBK2mwUX6cTdBS1A5OeMBZTT8ZZq+pPDSX5WhBPgGVDojJ9DI1W0QT7ELPvi7\n"
"GVxzT+Fev4hlzIWPRo8YTVBDZ+s06eNrCo8Tj5rG/EZxowA+UZ1Iyc1inAt79wCD\n"
"7HbVVFHz6PEo3CyC+wIDAQAB\n"
"-----END PUBLIC KEY-----\n";

// * 복호화 키 (RSA private key) - "encryption_key.pem"
const char* pem_decryption_key =
"-----BEGIN RSA PRIVATE KEY-----\n"
"MIICXQIBAAKBgQDTPh5DvH6YmeLEU1xKg0tk5Zv1M/3yBK2mwUX6cTdBS1A5OeMB\n"
"ZTT8ZZq+pPDSX5WhBPgGVDojJ9DI1W0QT7ELPvi7GVxzT+Fev4hlzIWPRo8YTVBD\n"
"Z+s06eNrCo8Tj5rG/EZxowA+UZ1Iyc1inAt79wCD7HbVVFHz6PEo3CyC+wIDAQAB\n"
"AoGBAJE8nUxPFe9PpmT7N3IXsUeOwlqlQ40ZgFctfTKCks32vRKLZTM5ZWO/a9R1\n"
"zQmnJFZP90NKt6oiHt9mNhVH5V2pGTzOGu0Miz1vPGOBywLkgv4QZUojRNCOeLhj\n"
"XMFPw8NN6JxI10aqw3BOzTYNFZ5iYkn0PS8oYPLhcfWADkghAkEA8KJhWpBN+JRw\n"
"IFkQ7YxDwca7cfKv3vHwEo2zGHecq2Hyd6/hZ2Q3WgxAz6HPNZeezESVrV0fqPze\n"
"qhd16RYz8QJBAOC7RrXlaoETEoxs/9SFz2YjO+zRIMoeC721wLHSiIpwEgV42qPd\n"
"zGE8KsZRheBvWxt+Hs71mhJBuBuAWl7B4asCQQDWDBr9LA+vZpq2q4NbSsGaxORH\n"
"39CBSER8vH7q7Lr32BCOVmzyQzrZZrzf9NcUnOIsYXWi2IspDoYiVuwTgiSBAkAC\n"
"8E//Zu2anH48mdxKkilhI4rQEPgFzJjItUy8xo0Eimyh56+sPuVCTEAe8cf3ZF1j\n"
"oaWTfUCzhmp66ZXE8XIbAkBcMStOjqtSf4C3VHHmUBUTGi7UnpagIv53QRENIOY5\n"
"2vcXE73fa++0yiWFSmOWJNuBm5ZbDOirJbmiWG5uXxDZ\n"
"-----END RSA PRIVATE KEY-----\n";

void tutorial_common_rsa_function()
{
	//-----------------------------------------------------------------------------
	// 1. RSA private key/public key Generate하기
	//    - 2048 bits(256 bytes) 길이의 RSA private/public key를 생성한다.
	//    - 권장 길이는 2048 bits(256 bytes) 이상이다.
	//-----------------------------------------------------------------------------
	auto rsa_key_pair = crypt::rsa::generate_key_pair(2048);


	//-----------------------------------------------------------------------------
	// 2. RSA public key를 사용해 암호화하기
	//-----------------------------------------------------------------------------
	// declare) 원문
	std::string_view original_data = "Temp test data.";

	// 1) 암호화를 한다.
	auto buf_encrypted = crypt::rsa::encrypt(crypt::rsa::public_key_t{ rsa_key_pair.public_key }, to_buffer_view(original_data));

	// trace) 
	std::cout << "RSA encrypted code : " << buf_encrypted.to_string<char>() << '\n';

	//-----------------------------------------------------------------------------
	// 3. RSA private key를 사용해 복호화하기
	//-----------------------------------------------------------------------------
	// 1) 복호화를 한다.
	auto buf_decrypted = crypt::rsa::decrypt(crypt::rsa::private_key_t{ rsa_key_pair.private_key }, buf_encrypted);

	// trace) 
	std::cout << "RSA decrypted code : " << buf_decrypted.to_string<char>() << '\n';
}

void tutorial_common_rsa_encryptor_decryptor()
{
	//-----------------------------------------------------------------------------
	// 1. RSA private key/public key Generate하기
	// 
	//-----------------------------------------------------------------------------
	// 1) rsa public/private key를 발급한다.
	auto rsa_key_pair = crypt::rsa::generate_key_pair(2048);

	// 2) encryptor를 생성한다. (RSA public key를 사용)
	auto pencryptor = crypt::rsa::make_encryptor(rsa_key_pair.public_key);

	// 3) decryptor를 생성한다. (RSA private key를 사용)
	auto pdecryptor = crypt::rsa::make_decryptor(rsa_key_pair.private_key);

	//-----------------------------------------------------------------------------
	// 2. 암복호화하기
	//-----------------------------------------------------------------------------
	// declare) 원문
	std::string_view original_data = "Temp test data.";

	// 1) 암호화를 한다.
	auto buf_encrypted = pencryptor->encrypt(original_data);

	// trace) 
	std::cout << "RSA encrypted code : " << buf_encrypted.to_string<char>() << '\n';

	// 2) 복호화를 한다.
	auto buf_decrypted = pdecryptor->decrypt(buf_encrypted);

	// trace) 
	std::cout << "RSA decrypted code : " << buf_decrypted.to_string<char>() << '\n';
}

void tutorial_common_rsa_cryptor()
{
	//-----------------------------------------------------------------------------
	// 1. RSA private key/public key Generate하기
	// 
	//-----------------------------------------------------------------------------
	// 1) rsa public/private key를 발급한다.
	auto rsa_key_pair = crypt::rsa::generate_key_pair(2048);

	// 2) cryptor를 생성한다. (RSA public key를 사용)
	auto pcryptor = crypt::rsa::make_cryptor(rsa_key_pair);

	//-----------------------------------------------------------------------------
	// 2. 암복호화하기
	//-----------------------------------------------------------------------------
	// declare) 원문
	std::string_view original_data = "Temp test data."sv;

	// 1) 암호화를 한다.
	auto buf_encrypted = pcryptor->encrypt(original_data);

	// trace) 
	std::cout << "RSA encrypted code : " << buf_encrypted.to_string<char>() << '\n';

	// 2) 복호화를 한다.
	auto buf_decrypted = pcryptor->decrypt(buf_encrypted);

	// trace) 
	std::cout << "RSA decrypted code : " << buf_decrypted.to_string<char>() << '\n';
}

void tutorial_common_rsa()
{
	// 1) rsa fucntion
	tutorial_common_rsa_function();

	// 2) rsa encrytor와 decryptor
	tutorial_common_rsa_encryptor_decryptor();

	// 3) rsa cryptor
	tutorial_common_rsa_cryptor();
}
