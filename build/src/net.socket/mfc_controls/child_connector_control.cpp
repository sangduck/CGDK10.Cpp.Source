#include "pch.h"
#include <Mmsystem.h>

extern	HINSTANCE	g_hInstance;

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connector_control, CChildDialog)

CChild_connector_control::CChild_connector_control(net::io::Iconnect_requestable* _pConnector, CWnd* _pParent)
	: CChildDialog(IDD_DIALOG_CONNECTOR_CONTROL, _pParent),
	m_port_bind(0),
	m_pconnector(_pConnector),
	m_state_pre(eSOCKET_STATE::MAX),
	m_bEnableControl(TRUE)
{
}

CChild_connector_control::~CChild_connector_control()
{
}

void CChild_connector_control::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CChild_connector_control, CChildDialog)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connector_control::OnCGMessageNotify)
	ON_BN_CLICKED(IDC_CHECK_CONNECT, &CChild_connector_control::OnBnClickedCheckConnect)
END_MESSAGE_MAP()

BOOL CChild_connector_control::CreateChild(net::io::Iconnect_requestable* _pConnector, uint16_t _port, CWnd* _pwnd)
{
	// 1) set default port를 설정한다.
	m_port_default = _port;

	// 2) Connective Manager를 설정한다.
	SetConnector(_pConnector);

	// declare) 
	BOOL result;

	// 3) 윈도우를 생성해서 결과를 되돌린다.
	result = Create(IDD_DIALOG_CONNECTOR_CONTROL, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 4) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공~~
	return TRUE;
}

void CChild_connector_control::EnableControl(BOOL _flag_enable)
{
	m_bEnableControl = _flag_enable;

	RETURN_IF(GetSafeHwnd() == nullptr);

	GetDlgItem(IDC_CHECK_CONNECT)->EnableWindow(GetDlgItem(IDC_CHECK_CONNECT)->IsWindowEnabled() && _flag_enable);
}

BOOL CChild_connector_control::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. Graph설정.
	//-----------------------------------------------------------------
	// 1) peer AddressSetting
	SetDlgItemText(IDC_EDIT_PEER_ADDRESS, TEXT("localhost"));
	SetDlgItemInt(IDC_EDIT_PEER_PORT, m_port_default);

	// 2) Connector가 nullptr이면 Disable한다.
	SCOPED_LOCK(m_csConnector)
	{
		OnEnable(m_pconnector.exist());
	}


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	//-----------------------------------------------------------------
	// Timer설정) 
	//-----------------------------------------------------------------
	// 1) 1초마다 상태 체크
	SetTimer(0, 1000, nullptr);
	

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_connector_control::OnEnable(BOOL _flag_enable)
{
	//__super::OnEnable(is_enable);

	RETURN_IF(GetSafeHwnd() == nullptr);

	GetDlgItem(IDC_EDIT_PEER_ADDRESS)->EnableWindow(_flag_enable);
	GetDlgItem(IDC_EDIT_PEER_PORT)->EnableWindow(_flag_enable);
	GetDlgItem(IDC_CHECK_CONNECT)->EnableWindow(m_bEnableControl && _flag_enable);
}

void CChild_connector_control::OnDestroy()
{
	// 1) Connector 리셋~
	SCOPED_LOCK(m_csConnector)
	{
		m_pconnector.reset();
	}

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CChild_connector_control::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더 이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// 1) Update한다.
		UpdateConnectorStatus();
	}

	CDialog::OnTimer(nIDEvent);
}


LRESULT CChild_connector_control::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
				//auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);
				//set_connection_info(pconnective);
			}
			break;

	case	CGACCEPTOR_READY:
			{
				//auto pconnective = reinterpret_cast<net::io::Iconnective*>(_lparam);
				//set_connection_info(pconnective);

				//EnableWindow(TRUE);
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
				EnableWindow(FALSE);
			}
			break;

	case	CGACCEPTOR_START:
			{
			}
			break;

	case	CGACCEPTOR_STOP:
			{
			}
			break;

	case	CGUPDATE:
			{
				UpdateConnectorStatus();
			}
			break;
	};

	return	0;

}

void CChild_connector_control::SetConnector(net::io::Iconnect_requestable* _pConnector)
{
	// 1) Connector를 설정한다.
	SCOPED_LOCK(m_csConnector)
	{
		m_pconnector	 = _pConnector;
	}

	// 2) OnEnable...
	if(_pConnector!=nullptr)
	{
		// - enable한다.
		OnEnable(TRUE);
	}

	// 3) Update함.
	if(m_hWnd!=nullptr)
	{
		PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);
	}
}

void CChild_connector_control::UpdateConnectorStatus()
{
	// check) WIndow가 불능상태라면 더 이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. 먼저 Connector를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Iconnect_requestable>	pconnect_requestable;

	// 1) Connective Manager를 설정한다.
	SCOPED_LOCK(m_csConnector)
	{
		pconnect_requestable = m_pconnector;
	}

	// 2) Socket을 얻는다.
	auto psocket = dynamic_cast<net::io::Isocket*>(pconnect_requestable.get());

	// check) psocket이 nullptr이면 그냥 끝낸다.
	RETURN_IF(psocket == nullptr);


	//-----------------------------------------------------------------
	// 2. Connector를 얻는다.
	//-----------------------------------------------------------------
	// 1) Socket의 상태를 얻는다.
	eSOCKET_STATE eStatus = psocket->socket_state();

	if(m_state_pre!=eStatus)
	{
		switch(eStatus)
		{
		case	eSOCKET_STATE::CLOSED:
				{
					((CButton*)GetDlgItem(IDC_CHECK_CONNECT))->SetCheck(BST_UNCHECKED);
					GetDlgItem(IDC_CHECK_CONNECT)->SetWindowText(TEXT("Connect"));

					GetDlgItem(IDC_EDIT_PEER_ADDRESS)->EnableWindow(TRUE);
					GetDlgItem(IDC_EDIT_PEER_PORT)->EnableWindow(TRUE);
				}
				break;

		case	eSOCKET_STATE::ESTABLISHED:
				{
					((CButton*)GetDlgItem(IDC_CHECK_CONNECT))->SetCheck(BST_CHECKED);
					GetDlgItem(IDC_CHECK_CONNECT)->SetWindowText(TEXT("disconnect"));

					GetDlgItem(IDC_EDIT_PEER_ADDRESS)->EnableWindow(FALSE);
					GetDlgItem(IDC_EDIT_PEER_PORT)->EnableWindow(FALSE);

					SetDlgItemTextA(this->m_hWnd, IDC_EDIT_PEER_ADDRESS, to_string(psocket->remote_endpoint()).c_str());
					SetDlgItemInt(IDC_EDIT_PEER_PORT, psocket->remote_endpoint().port());
				}
				break;

		default:
				{
					((CButton*)GetDlgItem(IDC_CHECK_CONNECT))->SetCheck(BST_CHECKED);
					GetDlgItem(IDC_CHECK_CONNECT)->SetWindowText(TEXT("disconnect"));

					GetDlgItem(IDC_EDIT_PEER_ADDRESS)->EnableWindow(FALSE);
					GetDlgItem(IDC_EDIT_PEER_PORT)->EnableWindow(FALSE);
				}
				break;
		};

		m_state_pre	 = eStatus;
	}
}


void CChild_connector_control::OnBnClickedCheckConnect()
{
	//-----------------------------------------------------------------
	// 1. 먼저 Connector를 얻는다.
	//-----------------------------------------------------------------
	// declare) 
	object_ptr<net::io::Iconnect_requestable>	pconnect_requestable;

	// 1) Connective Manager를 설정한다.
	SCOPED_LOCK(m_csConnector)
	{
		pconnect_requestable = m_pconnector;
	}

	// check) 
	RETURN_IF(pconnect_requestable.empty());


	//-----------------------------------------------------------------
	// 2. Connector를 얻는다.
	//-----------------------------------------------------------------
	CButton* pbutton = (CButton*)GetDlgItem(IDC_CHECK_CONNECT);

	if(pbutton->GetCheck() & BST_CHECKED)
	{
		// 1) peer Address와 Port를 얻는다.
		char strAddress[256];
		GetDlgItemTextA(m_hWnd, IDC_EDIT_PEER_ADDRESS, strAddress, 256);
		uint16_t port = (uint16_t)GetDlgItemInt(IDC_EDIT_PEER_PORT);

		// 2) address
		net::sockaddr sockaddr_peer{ net::ip::make_address_v4(strAddress), port };

		// 2) Connect한다.
		auto result = pconnect_requestable->process_request_connecting(sockaddr_peer);

		// check)
		ERROR_RETURN_IF(result == false, , AfxMessageBox(TEXT("접속에 실패했습니다."), MB_OK));

		// 3) 상태를 설정한다.
		GetDlgItem(IDC_CHECK_CONNECT)->SetWindowText(TEXT("disconnect"));
		GetDlgItem(IDC_EDIT_PEER_ADDRESS)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_PEER_PORT)->EnableWindow(FALSE);
	}
	else
	{
		// 1) Socket을 얻는다.
		auto pconnectable = dynamic_cast<net::io::Iconnectable*>(pconnect_requestable.get());

		if(pconnectable!=nullptr)
		{
			// - Socket을 닫는다.
			pconnectable->closesocket();
		}
	}
}


}