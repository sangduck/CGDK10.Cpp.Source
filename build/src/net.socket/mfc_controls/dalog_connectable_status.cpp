#include "pch.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialog_connectable_status, CDialog)

CDialog_connectable_status::CDialog_connectable_status(net::io::Iconnectable* _pconnectable, CWnd* pParent)
	: CDialog(IDD_DIALOG_CONNECTABLE_STATUS, pParent)
{
	set_connectable(_pconnectable);
}

CDialog_connectable_status::~CDialog_connectable_status()
{
}

void CDialog_connectable_status::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_UPDATE_INTERVAL,		m_comboUpdateInterval);
	DDX_Control(pDX, IDC_CUSTOM_MESSAGE_TRAFFIC,	m_graphMessage_Traffic);
	DDX_Control(pDX, IDC_SLIDER_MESSAGE_TRAFFIC,	m_sliderScaleMessage_Traffic);
	DDX_Control(pDX, IDC_CUSTOM_BYTES_TRAFFIC,		m_graphBytes_Traffic);
	DDX_Control(pDX, IDC_SLIDER_BYTES_TRAFFIC,		m_sliderScaleBytes_Traffic);
	DDX_Control(pDX, IDC_CUSTOM_IO,					m_graphIO);
	DDX_Control(pDX, IDC_SLIDER_IO,					m_sliderScaleIO);
	DDX_Control(pDX, IDC_CUSTOM_IO_EFF,				m_graphIO_Eff);
	DDX_Control(pDX, IDC_SLIDER_IO_EFF,				m_sliderScaleIO_Eff);
}


BEGIN_MESSAGE_MAP(CDialog_connectable_status, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE,				&CDialog_connectable_status::OnBnClickedButtonUpdate)
	ON_BN_CLICKED(IDOK,								&CDialog_connectable_status::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL,							&CDialog_connectable_status::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT,			&CDialog_connectable_status::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE,					&CDialog_connectable_status::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_REFERENCE_LIST,		&CDialog_connectable_status::OnBnClickedButtonReferenceList)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_MESSAGE_SEND,		&CDialog_connectable_status::OnBnClickedCheckMessageSend)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_MESSAGE_RECEIVE,	&CDialog_connectable_status::OnBnClickedCheckMessageReceived)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_BYTES_SEND,		&CDialog_connectable_status::OnBnClickedCheckBytesSend)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_BYTES_RECEIVE,	&CDialog_connectable_status::OnBnClickedCheckBytesReceive)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_IOSEND,			&CDialog_connectable_status::OnBnClickedCheckIOSend)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_IORECEIVE,		&CDialog_connectable_status::OnBnClickedCheckIOReceive)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_IOSEND_EFF,		&CDialog_connectable_status::OnBnClickedCheckIOSendEff)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_IORECEIVE_EFF,	&CDialog_connectable_status::OnBnClickedCheckIOReceiveEff)
	ON_CBN_SELCHANGE(IDC_COMBO_UPDATE_INTERVAL,		&CDialog_connectable_status::OnCbnSelchangeComboUpdateInterval)
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY,			&CDialog_connectable_status::OnCGMessageNotify)
	ON_WM_VSCROLL()
	ON_WM_TIMER()
	ON_WM_ACTIVATE()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// CDialog_connectable_status message handlers
int CDialog_connectable_status::m_s_iUpdateInverval	 = 2;

BOOL CDialog_connectable_status::CreateModaless(net::io::Iconnectable* _pconnectable, CWnd* p_pParent)
{
	set_connectable(_pconnectable);

	return Create(IDD_DIALOG_CONNECTABLE_STATUS, p_pParent);
}

BOOL CDialog_connectable_status::OnInitDialog()
{
	CDialog::OnInitDialog();

	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) 
	m_comboUpdateInterval.SetCurSel(m_s_iUpdateInverval);


	// View) Graph
	m_graphMessage_Traffic.AddGraph(1024, RGB(0xff, 0x00, 0x00));
	m_graphMessage_Traffic.AddGraph(1024, RGB(0x00, 0xff, 0x00));

	// View) Slider
	m_sliderScaleMessage_Traffic.SetRange(0, 14);
	m_sliderScaleMessage_Traffic.SetPos(0);			// 1024/32=32해서 얻은 값

	// View) Graph
	m_graphBytes_Traffic.AddGraph(1024, RGB(0xff, 0x00, 0x00));
	m_graphBytes_Traffic.AddGraph(1024, RGB(0x00, 0xff, 0x00));

	// View) Slider
	m_sliderScaleBytes_Traffic.SetRange(0, 14);
	m_sliderScaleBytes_Traffic.SetPos(0);			// 1024/32=32해서 얻은 값


	// View) Graph
	m_graphIO.AddGraph(4, RGB(0xff, 0x00, 0x00));
	m_graphIO.AddGraph(4, RGB(0x00, 0xff, 0x00));

	// View) Slider
	m_sliderScaleIO.SetRange(0, 14);
	m_sliderScaleIO.SetPos(0);			// 1024/32=32해서 얻은 값

	// View) Graph
	m_graphIO_Eff.AddGraph(4, RGB(0xff, 0x00, 0x00));
	m_graphIO_Eff.AddGraph(4, RGB(0x00, 0xff, 0x00));

	// View) Slider
	m_sliderScaleIO_Eff.SetRange(0, 14);
	m_sliderScaleIO_Eff.SetPos(0);			// 1024/32=32해서 얻은 값

	// 3) Button 설정
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_MESSAGE_SEND))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_MESSAGE_RECEIVE))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_BYTES_SEND))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_BYTES_RECEIVE))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IOSEND))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IORECEIVE))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IOSEND_EFF))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IORECEIVE_EFF))->SetCheck(BST_CHECKED);


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDialog_connectable_status::OnBnClickedOk()
{
	KillTimer(0);

	OnOK();
}

void CDialog_connectable_status::OnBnClickedCancel()
{
	KillTimer(0);

	OnCancel();
}

void CDialog_connectable_status::OnDestroy()
{
	// 1) Connectable을 Reset한다.
	scoped_lock(m_csConnectable)
	{
		m_pConnectable.reset();
	}

	// 2) 부모 Class의 on_destroy
	__super::OnDestroy();
}

void CDialog_connectable_status::OnCbnSelchangeComboUpdateInterval()
{
	// 1) 
	int	selected = m_comboUpdateInterval.GetCurSel();

	// 2) 
	SetUpdateInterval(selected);
}

void CDialog_connectable_status::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pscroll_bar)
{
	switch(pscroll_bar->GetDlgCtrlID())
	{
	case	IDC_SLIDER_MESSAGE_TRAFFIC:
			{
				int	value	 = m_sliderScaleMessage_Traffic.GetPos();

				int	iScale	 = 8;
				for(; value>0; --value)
				{
					iScale	*=2;
				}
				
				m_graphMessage_Traffic.SetVerticalScale(iScale);
			}
			break;

	case	IDC_SLIDER_BYTES_TRAFFIC:
			{
				int	value	 = m_sliderScaleBytes_Traffic.GetPos();

				int	iScale	 = 8;
				for(; value>0; --value)
				{
					iScale	*=2;
				}
				
				m_graphBytes_Traffic.SetVerticalScale(iScale);
			}
			break;

	case	IDC_SLIDER_IO:
			{
				int	value	 = m_sliderScaleIO.GetPos();

				int	iScale	 = 2;
				for(; value>0; --value)
				{
					iScale	*=2;
				}

				m_graphIO.SetVerticalScale(iScale);
			}
			break;

	case	IDC_SLIDER_IO_EFF:
			{
				int	value	 = m_sliderScaleIO_Eff.GetPos();

				int	iScale	 = 2;
				for(; value>0; --value)
				{
					iScale	*=2;
				}

				m_graphIO_Eff.SetVerticalScale(iScale);
			}
			break;
	};

	CDialog::OnVScroll(nSBCode, nPos, pscroll_bar);
}

void CDialog_connectable_status::OnBnClickedButtonUpdate()
{
	UpdateConnectableStatus();
}

void CDialog_connectable_status::OnBnClickedButtonDisconnect()
{
	// declare) 
	object_ptr<net::io::Iconnectable> pconnectable;

	scoped_lock(m_csConnectable)
	{
		pconnectable = m_pConnectable;
	}

	// check) Connectable이 nullptr이면 그냥 끝낸다.
	RETURN_IF(pconnectable.empty());

	// 1) Socket을 disconnect한다.
	pconnectable->disconnect();
}

void CDialog_connectable_status::OnBnClickedButtonClose()
{
	// declare) 
	object_ptr<net::io::Iconnectable> pconnectable;

	scoped_lock(m_csConnectable)
	{
		pconnectable = m_pConnectable;
	}

	// check) Connectable이 nullptr이면 그냥 끝낸다.
	RETURN_IF(pconnectable.empty());

	// 1) Socket을 closesocket한다.
	pconnectable->closesocket();
}

void CDialog_connectable_status::OnBnClickedButtonReferenceList()
{
	// declare) 
	object_ptr<net::io::Iconnectable> pconnectable;

	scoped_lock(m_csConnectable)
	{
		pconnectable = m_pConnectable;
	}

	// check) Connectable이 nullptr이면 그냥 끝낸다.
	RETURN_IF(pconnectable.empty());

	// 1) Dialog
	CDialog_reference_status dlg(pconnectable);

	// 2) DoModal
	dlg.DoModal();
}


void CDialog_connectable_status::OnBnClickedCheckMessageSend()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_MESSAGE_SEND))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphMessage_Traffic.EnableGraph(0, status == BST_CHECKED);
	m_graphMessage_Traffic.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckMessageReceived()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_MESSAGE_RECEIVE))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphMessage_Traffic.EnableGraph(1, status == BST_CHECKED);
	m_graphMessage_Traffic.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckBytesSend()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_BYTES_SEND))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphBytes_Traffic.EnableGraph(0, status == BST_CHECKED);
	m_graphBytes_Traffic.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckBytesReceive()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_BYTES_RECEIVE))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphBytes_Traffic.EnableGraph(1, status == BST_CHECKED);
	m_graphBytes_Traffic.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckIOSend()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IOSEND))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphIO.EnableGraph(0, status == BST_CHECKED);
	m_graphIO.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckIOReceive()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IORECEIVE))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphIO.EnableGraph(1, status == BST_CHECKED);
	m_graphIO.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckIOSendEff()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IOSEND_EFF))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphIO_Eff.EnableGraph(0, status == BST_CHECKED);
	m_graphIO_Eff.Invalidate(FALSE);
}

void CDialog_connectable_status::OnBnClickedCheckIOReceiveEff()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_IORECEIVE_EFF))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	m_graphIO_Eff.EnableGraph(1, status == BST_CHECKED);
	m_graphIO_Eff.Invalidate(FALSE);
}



void CDialog_connectable_status::SetUpdateInterval(int p_iInterval)
{
	// 1) 
	m_s_iUpdateInverval = p_iInterval;

	// 2) 
	switch(m_s_iUpdateInverval)
	{
	// No Update
	case	0:
			KillTimer(0);
			break;

	// 1sec
	case	1:
			SetTimer(0, 1000, nullptr);
			break;

	// 2sec
	case	2:
			SetTimer(0, 2000, nullptr);
			break;

	// 5sec
	case	3:
			SetTimer(0, 5000, nullptr);
			break;

	// 10sec
	case	4:
			SetTimer(0, 10000, nullptr);
			break;

	// 30sec
	case	5:
			SetTimer(0, 30000, nullptr);
			break;

	// 1min
	case	6:
			SetTimer(0, 60000, nullptr);
			break;

	};
}

void CDialog_connectable_status::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		// - 상태를 Update한다.
		UpdateConnectableStatus();
	}

	CDialog::OnTimer(nIDEvent);
}


void CDialog_connectable_status::set_connectable(net::io::Iconnectable* _pconnectable)
{
	scoped_lock(m_csConnectable)
	{
		m_pConnectable		 = _pconnectable;
	}

	m_countMessageSended		 = 0;
	m_countMessageReceived		 = 0;
	m_countMessageSendedByte	 = 0;
	m_countMessageReceivedByte	 = 0;
	m_countIOSended				 = 0;
	m_countIOReceived			 = 0;
	m_tick_last					 = chrono::tick::time_point();

	UpdateConnectableStatus();
}


void CDialog_connectable_status::UpdateConnectableStatus()
{
	RETURN_IF(m_hWnd == nullptr)

	// declare) 
	object_ptr<net::io::Iconnectable> pconnectable;

	scoped_lock(m_csConnectable)
	{
		pconnectable = m_pConnectable;
	}

	if(pconnectable.exist())
	{
		// - 현재 Tick과 Tick Gap을 구한다.
		chrono::tick::time_point tick_now	 = chrono::tick::now();
		chrono::milliseconds	 tick_differ = std::chrono::duration_cast<chrono::milliseconds>(tick_now - m_tick_last);

		// - 최종 시간을 last에 넣는다.
		m_tick_last = tick_now;


		//-----------------------------------------------------------------
		// 1. 기본정보
		//-----------------------------------------------------------------
		// declare) 
		TCHAR tempString[256]={};

		// 1) Socket Address
		SetDlgItemText(IDC_STATIC_ADDRESS_SOCKET, to_string<TCHAR>(pconnectable->remote_endpoint()).c_str());
		SetDlgItemText(IDC_STATIC_ADDRESS_PEER, to_string<TCHAR>(pconnectable->remote_endpoint()).c_str());

		auto strSocketStatus = to_string<TCHAR>(pconnectable->socket_state());

		SetDlgItemText(IDC_STATIC_SOCKET_STATUS, strSocketStatus.data());

		if(pconnectable->socket_state() >= eSOCKET_STATE::ESTABLISHED)
		{
			GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(TRUE);
			SetDlgItemText(IDC_STATIC_TIME_CONNECT, to_string<TCHAR>(chrono::time::local::time_point(pconnectable->statistics_get_connect_time())).c_str());
		}
		else
		{
			GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(FALSE);

			SetDlgItemText(IDC_STATIC_TIME_CONNECT, TEXT(""));
		}

		chrono::tick::time_point tick_last_send	 = pconnectable->statistics_get_tick_last_send();	__to_string(tempString, (tick_now-tick_last_send).count());	SetDlgItemText(IDC_STATIC_TIME_SEND,	tempString);
		chrono::tick::time_point tickLastReceive = pconnectable->statistics_get_tick_last_receive(); __to_string(tempString, (tick_now - tickLastReceive).count()); SetDlgItemText(IDC_STATIC_TIME_RECEIVE, tempString);
		SetDlgItemInt(IDC_STATIC_REFERENCE_COUNT, pconnectable->reference_count());


		//-----------------------------------------------------------------
		// 3. Traffice Info 계산하기.
		//-----------------------------------------------------------------
		// 1) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
		LONGLONG countMessageSended			 = pconnectable->statistics_get_sended_message();
		LONGLONG countMessageSendedByte		 = pconnectable->statistics_get_sended_bytes();
		LONGLONG countMessageReceived		 = pconnectable->statistics_get_received_message();
		LONGLONG countMessageReceivedByte	 = pconnectable->statistics_get_received_bytes();
		LONGLONG countIOSended				 = pconnectable->statistics_get_async_send();
		LONGLONG countIOReceived			 = pconnectable->statistics_get_async_receive();
		LONGLONG bytes_io_send_pending		 = 0;
		LONGLONG bytes_io_receive_pending	 = 0;

		// 2) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
		LONGLONG	tempGap[6];
		tempGap[0]					 = countMessageSended-m_countMessageSended;
		tempGap[1]					 = countMessageSendedByte-m_countMessageSendedByte;
		tempGap[2]					 = countMessageReceived-m_countMessageReceived;
		tempGap[3]					 = countMessageReceivedByte-m_countMessageReceivedByte;
		tempGap[4]					 = countIOSended-m_countIOSended;
		tempGap[5]					 = countIOReceived-m_countIOReceived;

		// 3) 이전 값을 현재 값으로 Update한다.(다음 Frame에서의 계산을 위해)
		m_countMessageSended		 = countMessageSended;
		m_countMessageSendedByte	 = countMessageSendedByte;
		m_countMessageReceived		 = countMessageReceived;
		m_countMessageReceivedByte	 = countMessageReceivedByte;
		m_countIOSended				 = countIOSended;
		m_countIOReceived			 = countIOReceived;

		// 4) receive Pending Info를 읽는다.
		auto preceivable = dynamic_cast<net::io::receivable::Nstream*>(pconnectable.get());

		if (preceivable != nullptr)
		{
			bytes_io_receive_pending = preceivable->queued_receive_bytes();
		}

		// 4) send Pending Info
		auto psendable = dynamic_cast<net::io::sendable::Itcp_gather*>(pconnectable.get());

		if (psendable != nullptr)
		{
			bytes_io_send_pending = psendable->sending_queued_bytes_now();
		}

		float totalSended_Message_PerSec;
		float totalSended_Byte_PerSec;
		float totalReceive_Message_PerSec;
		float totalReceive_Byte_PerSec;
		float totalSend_IO_PerSec;
		float totalReceive_IO_PerSec;
			  
		float totalSend_IO_MPIO;
		float totalReceive_IO_MPIO;
		float totalSend_IO_BPIO;
		float totalReceive_IO_BPIO;

		if(tick_differ != chrono::tick::duration(0))
		{
			totalSended_Message_PerSec	 = static_cast<float>(tempGap[0])*1000/tick_differ.count();
			totalSended_Byte_PerSec		 = static_cast<float>(tempGap[1])*1000/tick_differ.count();
			totalReceive_Message_PerSec	 = static_cast<float>(tempGap[2])*1000/tick_differ.count();
			totalReceive_Byte_PerSec	 = static_cast<float>(tempGap[3])*1000/tick_differ.count();
			totalSend_IO_PerSec			 = static_cast<float>(tempGap[4])*1000/tick_differ.count();
			totalReceive_IO_PerSec		 = static_cast<float>(tempGap[5])*1000/tick_differ.count();

			totalSend_IO_MPIO			 = tempGap[0]/totalSend_IO_PerSec;
			totalSend_IO_BPIO			 = tempGap[1]/totalSend_IO_PerSec;
			totalReceive_IO_MPIO		 = tempGap[2]/totalReceive_IO_PerSec;
			totalReceive_IO_BPIO		 = tempGap[3]/totalReceive_IO_PerSec;
		}
		else
		{
			totalSended_Message_PerSec	 = 0.0f;
			totalSended_Byte_PerSec		 = 0.0f;
			totalReceive_Message_PerSec	 = 0.0f;
			totalReceive_Byte_PerSec	 = 0.0f;
			totalSend_IO_PerSec			 = 0.0f;
			totalReceive_IO_PerSec		 = 0.0f;

			totalSend_IO_MPIO			 = 0.0f;
			totalReceive_IO_MPIO		 = 0.0f;
			totalSend_IO_BPIO			 = 0.0f;
			totalReceive_IO_BPIO		 = 0.0f;
		}


		//-----------------------------------------------------------------
		// 4. control에 표시하기.
		//-----------------------------------------------------------------
		// 1) send/receive Traffic 정보.
		__to_string(tempString, m_countMessageSended);					SetDlgItemText(IDC_STATIC_MESSAGE_SEND,			tempString);
		__to_string(tempString, m_countMessageReceived);				SetDlgItemText(IDC_STATIC_MESSAGE_RECEIVE,		tempString);
		__sprintf(tempString, TEXT("%.2f"), totalSended_Message_PerSec);SetDlgItemText(IDC_STATIC_MESSAGE_SEND_PERSEC,	tempString);
		__sprintf(tempString, TEXT("%.2f"), totalReceive_Message_PerSec);SetDlgItemText(IDC_STATIC_MESSAGE_RECEIVE_PERSEC,tempString);

		to_string_scaled_byte(tempString, 256, m_countMessageSendedByte);	SetDlgItemText(IDC_STATIC_BYTES_SEND,			tempString);
		to_string_scaled_byte(tempString, 256, m_countMessageReceivedByte);SetDlgItemText(IDC_STATIC_BYTES_RECEIVE,		tempString);
		to_string_scaled_byte(tempString, 256, totalSended_Byte_PerSec);	SetDlgItemText(IDC_STATIC_BYTES_SEND_PERSEC,	tempString);
		to_string_scaled_byte(tempString, 256, totalReceive_Byte_PerSec);	SetDlgItemText(IDC_STATIC_BYTES_RECEIVE_PERSEC,	tempString);

		__sprintf(tempString, 256, TEXT("%.2f"), totalSended_Message_PerSec+totalReceive_Message_PerSec);	SetDlgItemText(IDC_STATIC_MESSAGE_PERSEC,tempString);
		to_string_scaled_byte(tempString, 256, totalSended_Byte_PerSec+totalReceive_Byte_PerSec);	SetDlgItemText(IDC_STATIC_BYTES_PERSEC,	tempString);

		m_graphMessage_Traffic.AppendData(0, static_cast<int>(totalSended_Message_PerSec));
		m_graphMessage_Traffic.AppendData(1, static_cast<int>(totalReceive_Message_PerSec));
		m_graphMessage_Traffic.Scroll(1);

		m_graphBytes_Traffic.AppendData(0, static_cast<int>(totalSended_Byte_PerSec));
		m_graphBytes_Traffic.AppendData(1, static_cast<int>(totalReceive_Byte_PerSec));
		m_graphBytes_Traffic.Scroll(1);


		// 2) send/receive IO 정보.
		__to_string(tempString, m_countIOSended);						SetDlgItemText(IDC_STATIC_IO_SEND,			tempString);
		__to_string(tempString, m_countIOReceived);						SetDlgItemText(IDC_STATIC_IO_RECEIVE,		tempString);
		__sprintf(tempString, TEXT("%.2f"), totalSend_IO_PerSec);		SetDlgItemText(IDC_STATIC_IO_SEND_PERSEC,	tempString);
		__sprintf(tempString, TEXT("%.2f"), totalReceive_IO_PerSec);	SetDlgItemText(IDC_STATIC_IO_RECEIVE_PERSEC,tempString);

		__sprintf(tempString, TEXT("%.2f"), totalSend_IO_MPIO);			SetDlgItemText(IDC_STATIC_SEND_MPIO,		tempString);
		to_string_scaled_byte(tempString, 256, totalSend_IO_BPIO);			SetDlgItemText(IDC_STATIC_SEND_BPIO,		tempString);
		__sprintf(tempString, TEXT("%.2f"), totalReceive_IO_MPIO);		SetDlgItemText(IDC_STATIC_RECEIVE_MPIO,		tempString);
		to_string_scaled_byte(tempString, 256, totalReceive_IO_BPIO);		SetDlgItemText(IDC_STATIC_RECEIVE_BPIO,		tempString);

		SetDlgItemInt(IDC_STATIC_SEND_IO_NOW,	pconnectable->statistics_get_async_sending());
		SetDlgItemInt(IDC_STATIC_RECEIVE_IO_NOW,pconnectable->statistics_get_async_receiving());

		m_graphIO.AppendData(0, static_cast<int>(totalSend_IO_PerSec));
		m_graphIO.AppendData(1, static_cast<int>(totalReceive_IO_PerSec));
		m_graphIO.Scroll(1);

		m_graphIO_Eff.AppendData(0, static_cast<int>(totalSend_IO_MPIO));
		m_graphIO_Eff.AppendData(1, static_cast<int>(totalReceive_IO_MPIO));
		m_graphIO_Eff.Scroll(1);


		// 3) send Pending/ Received
		__to_string(tempString, bytes_io_send_pending);				SetDlgItemText(IDC_STATIC_PENDED_BYTES_SEND, tempString);
		__to_string(tempString, bytes_io_receive_pending);			SetDlgItemText(IDC_STATIC_PENDED_BYTES_RECEIVE, tempString);
	}
	else
	{
		// 1) Socket Address
		SetDlgItemText(IDC_STATIC_ADDRESS_SOCKET,		TEXT(""));
		SetDlgItemText(IDC_STATIC_ADDRESS_PEER,			TEXT(""));
		GetDlgItem(IDC_BUTTON_DISCONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_CLOSE)->EnableWindow(FALSE);

		// 2) Socket 기본 정보.
		SetDlgItemText(IDC_STATIC_SOCKET_STATUS,		TEXT(""));
		SetDlgItemText(IDC_STATIC_TIME_CONNECT,			TEXT(""));
		SetDlgItemText(IDC_STATIC_TIME_SEND,			TEXT(""));
		SetDlgItemText(IDC_STATIC_TIME_RECEIVE,			TEXT(""));

		// 3) send/receive Traffic 정보.
		SetDlgItemText(IDC_STATIC_MESSAGE_SEND,			TEXT(""));
		SetDlgItemText(IDC_STATIC_MESSAGE_RECEIVE,		TEXT(""));
		SetDlgItemText(IDC_STATIC_MESSAGE_SEND_PERSEC,	TEXT(""));
		SetDlgItemText(IDC_STATIC_MESSAGE_RECEIVE_PERSEC,TEXT(""));

		SetDlgItemText(IDC_STATIC_BYTES_SEND,			TEXT(""));
		SetDlgItemText(IDC_STATIC_BYTES_RECEIVE,		TEXT(""));
		SetDlgItemText(IDC_STATIC_BYTES_SEND_PERSEC,	TEXT(""));
		SetDlgItemText(IDC_STATIC_BYTES_RECEIVE_PERSEC,	TEXT(""));

		SetDlgItemText(IDC_STATIC_MESSAGE_PERSEC,		TEXT(""));
		SetDlgItemText(IDC_STATIC_BYTES_PERSEC,			TEXT(""));

		// 4) send/receive IO 정보.
		SetDlgItemText(IDC_STATIC_IO_SEND,				TEXT(""));
		SetDlgItemText(IDC_STATIC_IO_RECEIVE,			TEXT(""));
		SetDlgItemText(IDC_STATIC_IO_SEND_PERSEC,		TEXT(""));
		SetDlgItemText(IDC_STATIC_IO_RECEIVE_PERSEC,	TEXT(""));

		SetDlgItemText(IDC_STATIC_SEND_MPIO,			TEXT(""));
		SetDlgItemText(IDC_STATIC_SEND_BPIO,			TEXT(""));
		SetDlgItemText(IDC_STATIC_RECEIVE_MPIO,			TEXT(""));
		SetDlgItemText(IDC_STATIC_RECEIVE_BPIO,			TEXT(""));

		SetDlgItemText(IDC_STATIC_SEND_IO_NOW,			TEXT(""));
		SetDlgItemText(IDC_STATIC_RECEIVE_IO_NOW,		TEXT(""));

		SetDlgItemText(IDC_STATIC_PENDED_BYTES_SEND,	TEXT(""));
		SetDlgItemText(IDC_STATIC_PENDED_BYTES_RECEIVE,	TEXT(""));
	}
}


LRESULT CDialog_connectable_status::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
{
	switch(_wparam)
	{
	case	CGACCEPTOR_SET_CONNECTIVE:
			{
			}
			break;

	case	CGACCEPTOR_READY:
			{
			}
			break;

	case	CGACCEPTOR_CLOSE:
			{
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
				UpdateConnectableStatus();
			}
			break;
	};

	return	0;

}

void CDialog_connectable_status::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	// 1) Timer를 설정한다.
	SetUpdateInterval(m_s_iUpdateInverval);

	// 2) 상태를 Update한다.
	UpdateConnectableStatus();
}


}