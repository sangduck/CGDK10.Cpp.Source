#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_connective_connector_control, CDialogEx)

CChild_connective_connector_control::CChild_connective_connector_control(net::io::connective::connector::Ntcp* _pconnective_connector, CWnd* pParent /*=nullptr*/)
	: CChildDialog(IDD_DIALOG_CONNECTIVE_CONNECTOR_CONTROL, pParent),
	m_pconnectiveConnector(_pconnective_connector)
{
	m_str_address = TEXT("localhost");
	m_port		 = 20000;
}

CChild_connective_connector_control::~CChild_connective_connector_control()
{
}

void CChild_connective_connector_control::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CChild_connective_connector_control::CreateChild(net::io::connective::connector::Ntcp* _pconnective_connector, CWnd* _pwnd)
{
	// declare) 
	BOOL	result;

	// 1) Acceptor를 설정한다.
	SetConnectiveConnector(_pconnective_connector);

	// 2) 윈도우를 생성한다.
	result	 = Create(IDD_DIALOG_CONNECTIVE_CONNECTOR_CONTROL, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 
	return	TRUE;
}


BEGIN_MESSAGE_MAP(CChild_connective_connector_control, CDialogEx)
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_connective_connector_control::OnCGMessageNotify)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_CONNECT1, &CChild_connective_connector_control::OnBnClickedButtonConnect1)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT10, &CChild_connective_connector_control::OnBnClickedButtonConnect10)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT100, &CChild_connective_connector_control::OnBnClickedButtonConnect100)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT_MASSIVE, &CChild_connective_connector_control::OnBnClickedButtonConnectMassive)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CChild_connective_connector_control::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT_ALL, &CChild_connective_connector_control::OnBnClickedButtonDisconnectAll)
END_MESSAGE_MAP()

void CChild_connective_connector_control::OnEnable(BOOL _is_Enable)
{
	// 1) Dialog::OnEnable()
	CChildDialog::OnEnable(_is_Enable);

	// declare) 
	object_ptr<net::io::connective::connector::Ntcp> pconnective_connector;

	// 2) Acceptor를 얻어놓는다.
	SCOPED_LOCK(m_lockable_pconnectiveConnector)
	{
		pconnective_connector = m_pconnectiveConnector;
	}

	// 3) enable 여부 결정
	bool bEnableControl = pconnective_connector.exist() && _is_Enable;

	// 4) Ready/start Button
	GetDlgItem(IDC_STATIC_ADDRESS)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_EDIT_ADDRESS)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(bEnableControl);

	GetDlgItem(IDC_STATIC_SEPARATOR_1)->EnableWindow(bEnableControl);

	GetDlgItem(IDC_BUTTON_CONNECT1)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_BUTTON_CONNECT10)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_BUTTON_CONNECT100)->EnableWindow(bEnableControl);

	GetDlgItem(IDC_STATIC_SEPARATOR_2)->EnableWindow(bEnableControl);
	
	GetDlgItem(IDC_EDIT_CONNECT_MASSIVE)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_BUTTON_CONNECT_MASSIVE)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(bEnableControl);
	GetDlgItem(IDC_BUTTON_DISCONNECT_ALL)->EnableWindow(bEnableControl);
}

void CChild_connective_connector_control::OnDestroy()
{
	// 1) Acceptor를 Reset한다.
	SCOPED_LOCK(m_lockable_pconnectiveConnector)
	{
		m_pconnectiveConnector.reset();
	}

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

BOOL CChild_connective_connector_control::OnInitDialog()
{
	// 1) 
	CChildDialog::OnInitDialog();

	// 1) Destination IP Address
	SetDlgItemText(IDC_EDIT_ADDRESS, m_str_address.c_str());

	// 2) Destination Port
	SetDlgItemInt(IDC_EDIT_PORT, m_port);

	// 3) Massive Connect 수 설정
	SetDlgItemInt(IDC_EDIT_CONNECT_MASSIVE, 2000);

	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}


void CChild_connective_connector_control::SetConnectiveConnector(net::io::connective::connector::Ntcp* _pconnective_connector)
{
	SCOPED_LOCK(m_lockable_pconnectiveConnector)
	{
		// check) 
		RETURN_IF(m_pconnectiveConnector==_pconnective_connector);

		// 1) 설정한다.
		m_pconnectiveConnector = _pconnective_connector;
	}

	// 2) 정보 Update
	//OnEnable();
}

void CChild_connective_connector_control::SetAddress(const std::string_view _address, int _port)
{
	// 1) 값 설정
#ifdef _UNICODE
	TCHAR	text[256];
	encode_string(text, _address.data(), 256);
	m_str_address = text;
#else
	m_str_address = _address;
#endif
	m_port			 = _port;

	// 2) control에 반영
	if(m_hWnd!=NULL)
	{
		// - Destination IP Address
		SetDlgItemText(IDC_EDIT_ADDRESS, m_str_address.c_str());

		// - Destination Port
		SetDlgItemInt(IDC_EDIT_PORT, m_port);
	}
}

void CChild_connective_connector_control::SetAddress( std::wstring_view _address, int _port)
{
	// 1) 값 설정
#ifdef _UNICODE
	m_str_address = _address;
#else
	TCHAR	text[256];
	encode_string(text, _address.data(), 256);
	m_str_address = text;
#endif
	m_port			 = _port;

	// 2) control에 반영
	if(m_hWnd!=NULL)
	{
		// - Destination IP Address
		SetDlgItemText(IDC_EDIT_ADDRESS, m_str_address.c_str());

		// - Destination Port
		SetDlgItemInt(IDC_EDIT_PORT, m_port);
	}
}


LRESULT CChild_connective_connector_control::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			break;

	case	CGACCEPTOR_READY:
			break;

	case	CGACCEPTOR_CLOSE:
			break;

	case	CGACCEPTOR_START:
			break;

	case	CGACCEPTOR_STOP:
			break;

	case	CGUPDATE:
			break;
	};

	return	0;
}

void CChild_connective_connector_control::OnBnClickedButtonConnect1()
{
	ConnectN(1);
}

void CChild_connective_connector_control::OnBnClickedButtonConnect10()
{
	ConnectN(10);
}

void CChild_connective_connector_control::OnBnClickedButtonConnect100()
{
	ConnectN(100);
}

void CChild_connective_connector_control::OnBnClickedButtonConnectMassive()
{
	// 1) Connect 개수를 얻는다.
	int	iCount	 = GetDlgItemInt(IDC_EDIT_CONNECT_MASSIVE);

	// check) 최소 1개 최대5만개
	SATURATE(iCount, 1, 50000);

	// 2) 얻은 수만큼의 접속을 시도한다.
	ConnectN(iCount);
}

void CChild_connective_connector_control::OnBnClickedButtonDisconnect()
{
	disconnect();
}

void CChild_connective_connector_control::OnBnClickedButtonDisconnectAll()
{
	DisconnectAll();
}

void CChild_connective_connector_control::ConnectN(int p_n)
{
	// declare) 
	object_ptr<net::io::connective::connector::Ntcp>	pconnective_connector;

	// 1) Acceptor를 얻어놓는다.
	SCOPED_LOCK(m_lockable_pconnectiveConnector)
	{
		pconnective_connector = m_pconnectiveConnector;
	}

	// check) 
	RETURN_IF(pconnective_connector.empty());

	// declare) 
	net::ip::tcp::endpoint endpoint_server;
	TCHAR strAddress[256];

	// 2) Address String을 얻는다.
	GetDlgItemText(IDC_EDIT_ADDRESS, strAddress, 256);

	// 3) Address로 변환한다.
	endpoint_server = from_string(strAddress);

	// check) Address가 잘못되었으면 Exception을 던진다.
	THROW_IF(endpoint_server.empty(), throwable::failure(eRESULT::FAIL));

	// 4) Port설정.
	endpoint_server.port((uint16_t)GetDlgItemInt(IDC_EDIT_PORT));	// - Port를 설정한다.

	// 5) 접속한다.
	for(int i=0; i<p_n; ++i)
	{
		// - Connect를 요청한다.
		pconnective_connector->request_connect(endpoint_server);
	}
}

void CChild_connective_connector_control::disconnect(disconnect_reason_t _disconnect_reason) noexcept
{
	// declare) 
	object_ptr<net::io::Iconnectable> pconnectable;

	SCOPED_LOCK(this->m_lockable_pconnectiveConnector)
	{
		// 1) Connectable Manager를 얻는다.
		auto pconnectablemanager = dynamic_pointer_cast<net::io::Iconnectable_manager>(this->m_pconnectiveConnector);

		// check) Connectable Manager가 없으면 끝낸다.
		RETURN_IF(pconnectablemanager.empty());

		// 2) Iterator를 얻는다.
		auto iter = pconnectablemanager->alloc_iterator();

		// 3) 첫 번째 Connectable을 얻는다.
		pconnectable = iter->get_next();
	}

	// check) Connectable이 nullptr이면 그냥 여기서 끝냄.
	RETURN_IF(pconnectable.empty());

	// 4) Socket을 닫음.
	pconnectable->closesocket(_disconnect_reason);
}

void CChild_connective_connector_control::DisconnectAll()
{
	// declare) 
	object_ptr<net::io::connective::connector::Ntcp> pconnective_connector;

	// 2) Acceptor를 얻어놓는다.
	SCOPED_LOCK(m_lockable_pconnectiveConnector)
	{
		pconnective_connector = m_pconnectiveConnector;
	}

	// check) 
	RETURN_IF(pconnective_connector.empty());

	pconnective_connector->disconnect_connectable_all();
}


}