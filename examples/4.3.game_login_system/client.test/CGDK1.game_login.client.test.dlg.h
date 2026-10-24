//*****************************************************************************
//*                                                                           *
//*                      Cho sanghyun's Game Classes II                       *
//*                       Ver 6.1 / Release 2012.05.28                        *
//*                                                                           *
//*                 Sample CGNetwork SocketClasses Client (3)                 *
//*                      TCP Echo Client (MFC Controls)                       *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*                                                                           *
//*  This Program is programmed by Cho SangHyun. sangducks@gmail.com          *
//*  Best for Game Developement and Optimized for Game Developement.          *
//*                                                                           *
//*                   (C) 2008 CGLabs All right reserved.                     *
//*                           http://www.CGDK.co.kr                           *
//*                                                                           *
//*****************************************************************************

#pragma once
//-----------------------------------------------------------------------------
//
// CSampleSocketClientTCPEchoDlg
//
// 1. CSampleSocketClientTCPEchoDlg은...
//    1) TCP Client Echo Server용 Dialog.
//    2) 모든 Socket의 Message가 이 Dialog에서 처리되도록 하기 위해 Message
//       Mediator를 가짐.
//    3) 이 Client에서는 크게 2가지 Test를 수행함.
// 
//       * 접속Test
//         - 기본적인 Connection/Disconnect처리를 수행함.
//         - Connect Test를 시작하면 정해진 범위에서 접속과 접속 종료를 계속
//           반복함.
//
//       * 전송 Test
//         - 전송 Test를 수행하면 일정시간마다 설정한 packet을 접속된 Socket에게
//           계속 전송함.
//
//-----------------------------------------------------------------------------
#include "afxwin.h"
#include "afxcmn.h"

// CSampleSocketClientTCPEchoDlg 대화 상자
class CSampleSocketClientTCPEchoDlg : 
	public						CDialog,
	protected					ICGExecutable,
	public						ICGReferenceable::NoAct					// (@) Releaser (No Act)
{
// 1) Constructor/Destructor)
public:
			CSampleSocketClientTCPEchoDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
			~CSampleSocketClientTCPEchoDlg();


// 2) MFC Dialog
	enum { IDD = IDD_SAMPLECGNETTCPECHOCLIENT_DIALOG };

// 구현입니다.
protected:
			HICON				m_hIcon;

// 3) MFC Virtual Functions
protected:
	virtual void				DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL				OnInitDialog();

// 4) MFC Message map
	DECLARE_MESSAGE_MAP()

private:
	afx_msg HCURSOR				OnQueryDragIcon();
	afx_msg void				OnPaint();
	afx_msg void				OnTimer(UINT_PTR nIDEvent);

	afx_msg void				OnBnClickedOk();
	afx_msg void				OnBnClickedCancel();
	afx_msg void				OnBnClickedButtonConnect1();
	afx_msg void				OnBnClickedButtonConnect10();
	afx_msg void				OnBnClickedButtonConnect100();
	afx_msg void				OnBnClickedButtonConnect();
	afx_msg void				OnBnClickedButtonDisconnectAll();
	afx_msg void				OnBnClickedButtonDisconnect();
	afx_msg void				OnBnClickedCheckConnectTestStartstop();
	afx_msg void				OnBnClickedButtonPool();
	afx_msg void				OnBnClickedButtonThread();
	afx_msg void				OnBnClickedButtonGameserverDisconnect();
	afx_msg void				OnBnClickedButtonGameserverDisconnectAll();


// 5) MFC Controls
private:


// 6) CGCII 
public:
	// - Contorls
			CCGChildConnectiveInfo		m_childConnectiveLogin;
			CCGChildConnectiveInfo		m_childConnectiveGame;
			CCGChildTrafficInfo			m_childTrafficInfo;
			CCGDialogObjectPoolStatus	m_dlgObjectPool;
			CCGDialogThreadStatus		m_dlgThreadStatus;
			
			CCGChildLogView				m_childLogView;

	// - Send Test용.
			volatile DWORD		m_countErrorCRC;

	// - Connect Test용.
			DWORD				m_tickLast_Connect;
			int					m_iTestConnectPerInterval;
			int					m_iTestMinConnect;
			int					m_iTestMaxConnect;
			volatile DWORD		m_dwCounterTotalConnected;
			float				m_fTimeTotalConnected;

			bool				m_IsConClick;	// con 을 클릭했는가?
			DWORD				m_tickConStart;	// 마지막으로 con 을 누른시간.
			DWORD				m_tickLoding;	// 로딩하는대 걸린시간.

	// - Sockets.
			SOCKADDR_IN			m_sockaddrServer;
	static	CGPTR<CGNet::CConnector<ServerLogin::CSocket>>	m_pconnectorServerLogin;
	static	CGPTR<CGNet::CConnector<ServerLobby::CSocket>>	m_pconnectorServerLobby;

	// - Executor
			CGPTR<CGExecutor::CCompletionPort>	m_pExecutor;
			void				(*pfSendTest_Packet[10])(int nSend);
			
public:
	// - 조작함수.
			void				ConnectN(int p_n);
			void				ReadServerAddress();
			void				ConnectTest();

	// - On Functions.
			void				OnConnect(ServerLogin::CSocket* _pSocket);
			void				OnFailConnect(ServerLogin::CSocket* _pSocket);
			void				OnDisconnect(ServerLogin::CSocket* _pSocket);

protected:
	// - Send Test용
	virtual	intptr_t			ProcessExecute(_In_ intptr_t _Return, _In_ size_t _Param) override;
};
