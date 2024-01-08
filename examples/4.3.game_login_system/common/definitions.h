//*****************************************************************************
//*                                                                           *
//*                       CGCII Sample Login System (1)                       *
//*                                                                           *
//*                                   Common                                  *
//*                                                                           *
//*  Programmed by Cho SangHyun(sangduck@cgcii.co.kr) 2016/02/01.             *
//*                                                                           *
//*  Updated : 2016/02/01                                                     *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
//  Protocols
//
//-----------------------------------------------------------------------------
namespace CGMESSAGE
{
	const uint32_t	REQUEST_CERTIFICATION		 = CGMESSAGE::NETWORK::USER::BASE + 100;
	const uint32_t	ACCEPT_CERTIFICATION		 = CGMESSAGE::NETWORK::USER::BASE + 101;
	const uint32_t	SEND_LOBBYSERVER_STATUS		 = CGMESSAGE::NETWORK::USER::BASE + 102;

	const uint32_t	REQUEST_ACCOUNT_LOGIN		 = CGMESSAGE::NETWORK::USER::BASE + 103;
	const uint32_t	RESPONSE_ACCOUNT_LOGIN		 = CGMESSAGE::NETWORK::USER::BASE + 104;
	const uint32_t	REQUEST_ACCOUNT_CHECKIN		 = CGMESSAGE::NETWORK::USER::BASE + 105;
	const uint32_t	RESPONSE_ACCOUNT_CHECKIN	 = CGMESSAGE::NETWORK::USER::BASE + 106;
	const uint32_t	REQUEST_ACCOUNT_CHECKOUT	 = CGMESSAGE::NETWORK::USER::BASE + 107;
}


//-----------------------------------------------------------------------------
//
//  Definitions
//
//-----------------------------------------------------------------------------
typedef class stServer_Info
{
public:
	SOCKADDR_IN		nServer_Address;

	uint32_t		nServer_Type;
	uint32_t		nServer_No;

	uint64_t		nServer_SessionKey;

	uint32_t		nServer_Player_Count;

public:
	stServer_Info()
	{
		nServer_Address.sin_port = 0;
		nServer_Type			 = 0;
		nServer_No				 = 0;
		nServer_SessionKey		 = 0;
		nServer_Player_Count	 = 0;
	}
} SERVER_INFO, *LPSERVER_INFO;


struct ACCOUNT_ID
{
	uint64_t	u64Account;	// 계정의 고유 값
	uint32_t	u32Login;	// Login할 때마다 할당되는 값
	uint32_t	u32Key;		// 그냥 Random 값
};