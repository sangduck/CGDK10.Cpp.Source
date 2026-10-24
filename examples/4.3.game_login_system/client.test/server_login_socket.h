//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                        Ver 5.0 / Bulid 2010.02.28                         *
//*                                                                           *
//*                        Client Classes Tutorial (4)                        *
//*                                 Room Client                               *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangduck@paran.com           *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                   (C) 2008 CGLabs All right reserved.                     *
//*                           http://www.CGCII.co.kr                          *
//*                           http://www.CGLabs.co.kr                         *
//*                                                                           *
//*****************************************************************************

#pragma once
// ----------------------------------------------------------------------------
//
// ServerLogin::CSocket
//
// ----------------------------------------------------------------------------
namespace ServerLogin
{

class CSocket : 
	public						CGNet::IO::Connector::NTCP,
	public						CGNet::Socket::CTCP<>,
	public						ICGServerGroupable<CSocket>,
	public						NCGDispatchableSingle
{
// 1) Constructor/Destructor
public:
			CSocket()			{}
	virtual ~CSocket()			{}

// 2) Override functions
protected:
	// - OnRequestConnect/Connected/Disconnected/OnFailConnect (CGNet::Socket::CTCP<uint32_t>)
	virtual void				OnRequestConnect(LPCSOCKADDR_IN _SockAddr) override;
	virtual void				OnConnect(CGNet::IO::IConnective* _pConnective) override;
	virtual void				OnDisconnect(uint32_t _Reason) override;
	virtual	void				OnFailConnect(CGNet::IO::IConnective* _pConnective, uint32_t _Reason) override;

// 3) Messages
protected:
	// - Message를 받았을 때 불려지는 함수.
	virtual int					OnMessage(CGMSG& _msg) override;

	// - Message별 처리 함수
			int					OnMessage_LoginServerInfo(CGNETMSG& _msg);
			int					OnMessage_ResultLogin(CGNETMSG& _msg);
			int					OnMessage_RequestEnterUser(CGNETMSG& _msg);
			int					OnMessage_ResultCreateUserInfo(CGNETMSG& _msg);

// 4) 
public:
	// - Address/Port
			TCHAR				m_strAddress[256];
			WORD				m_wPort;

	// - ID & Password
			//CHECKIN_ID			m_cid;
			//SERVER_INFO			m_ServerInfo;

	// - Performance
			CCGQueryPerformance	m_performanceConnect;
			lockable<>			m_csGuildTestLock;

			void				GenerateLoginID();
	static	void				GetRandomNickname(char* p_str, int strLength);
};


}