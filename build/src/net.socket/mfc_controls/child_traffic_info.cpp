#include "pch.h"
#include <Mmsystem.h>

extern	HINSTANCE	g_hInstance;

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_traffic_info, CChildDialog)

CChild_traffic_info::CChild_traffic_info(const statistics::unit_traffic* _pinfo, CWnd* pParent)
	: CChildDialog(IDD_DIALOG_TRAFFIC_INFO, pParent)
{
	this->m_pstatistics				 = _pinfo;

	this->m_count_sended			 = 0;
	this->m_count_received			 = 0;
	this->m_count_sendedByte		 = 0;
	this->m_count_receivedByte		 = 0;
	this->m_count_send_io_call		 = 0;
	this->m_count_receive_io_call	 = 0;
}

CChild_traffic_info::~CChild_traffic_info()
{
}

void CChild_traffic_info::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CUSTOM_TRAFFIC, m_graph_traffic);
	DDX_Control(pDX, IDC_SLIDER_TRAFFIC, m_slider_scale_traffic);
}


BEGIN_MESSAGE_MAP(CChild_traffic_info, CChildDialog)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_SENDED, &CChild_traffic_info::OnBnClickedCheckSend)
	ON_BN_CLICKED(IDC_CHECK_GRAPH_RECEIVED, &CChild_traffic_info::OnBnClickedCheckReceive)
	ON_WM_TIMER()
	ON_WM_VSCROLL()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_traffic_info::OnCGMessageNotify)
END_MESSAGE_MAP()

BOOL CChild_traffic_info::CreateChild(const statistics::unit_traffic* _pinfo, CWnd* _pwnd)
{
	// 1) Statistics의 포인터를 저장~
	this->m_pstatistics = _pinfo;

	// 2) 윈도우를 생성한다.
	auto result = this->Create(IDD_DIALOG_TRAFFIC_INFO, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result == FALSE, FALSE);

	// 3) Show Window를 한다.
	this->ShowWindow(SW_SHOW);

	// return) 성공!!!
	return result;
}

BOOL CChild_traffic_info::OnInitDialog()
{
	this->CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Graph
	this->m_graph_traffic.AddGraph(1024, RGB(0xff, 0x00, 0x00));
	this->m_graph_traffic.AddGraph(1024, RGB(0x00, 0xff, 0x00));

	// 2) Slider
	this->m_slider_scale_traffic.SetRange(0, 20);
	this->m_slider_scale_traffic.SetPos(0);			// 1024/32=32해서 얻은 값

	// 3) Check를 Checked로...
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_SENDED))->SetCheck(BST_CHECKED);
	((CButton*)GetDlgItem(IDC_CHECK_GRAPH_RECEIVED))->SetCheck(BST_CHECKED);

	// 4) Tick 초기화.
	this->m_tick_last = chrono::tick::now();


	//-----------------------------------------------------------------
	// 2. Update한다.
	//-----------------------------------------------------------------
	// 1) 
	this->PostMessage(eMESSAGE::WINDOWS::NOTIFY, CGUPDATE);


	// return)  
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_traffic_info::OnBnClickedCheckSend()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_SENDED))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	this->m_graph_traffic.EnableGraph(0, status == BST_CHECKED);
	this->m_graph_traffic.Invalidate(FALSE);
}

void CChild_traffic_info::OnBnClickedCheckReceive()
{
	// 1) 현재의 버튼 상태를 얻는다.
	int	status = ((CButton*)GetDlgItem(IDC_CHECK_GRAPH_RECEIVED))->GetCheck();

	 // check) 결정되지 않은 상태면 그냥 끝낸다.
	RETURN_IF(status == BST_INDETERMINATE);

	// 2) Graph의 상태를 설정한다.
	this->m_graph_traffic.EnableGraph(1, status == BST_CHECKED);
	this->m_graph_traffic.Invalidate(FALSE);
}


void CChild_traffic_info::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pscroll_bar)
{
	switch(pscroll_bar->GetDlgCtrlID())
	{
	case	IDC_SLIDER_TRAFFIC:
			{
				int	value = m_slider_scale_traffic.GetPos();

				int	iScale = 8;
				for(; value>0; --value)
				{
					iScale *= 2;
				}
				
				this->m_graph_traffic.SetVerticalScale(iScale);
			}
			break;
	};

	CChildDialog::OnVScroll(nSBCode, nPos, pscroll_bar);
}


void CChild_traffic_info::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent==0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(this->IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(this->IsWindowVisible() == FALSE)

		// 1) Update한다.
			this->UpdateTrafficInfo();
	}


	//-----------------------------------------------------------------
	// 2. MFC Default
	//-----------------------------------------------------------------
	this->CChildDialog::OnTimer(nIDEvent);
}

void CChild_traffic_info::UpdateTrafficInfo()
{
	// check) 
	RETURN_IF(this->m_pstatistics == nullptr);

	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(this->IsWindowEnabled() == FALSE)


	//-----------------------------------------------------------------
	// 1. Tick Gap을 구한다.
	//-----------------------------------------------------------------
	// 1) 현재 Tick과 Tick Gap을 구한다.
	chrono::tick::time_point	tick_now = chrono::tick::now();
	chrono::tick::milliseconds	tick_differ = std::chrono::duration_cast<chrono::milliseconds>(tick_now-m_tick_last);

	// check) tickdiffer가 10ms이하면 그냥 끝낸다.
	RETURN_IF(tick_differ<chrono::milliseconds(10));

	// 2) 최종 시간을 Update한다.
	this->m_tick_last = tick_now;


	//-----------------------------------------------------------------
	// 2. Traffice Info 계산하기.
	//-----------------------------------------------------------------
	// 1) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
	uint64_t	countSended				 = this->m_pstatistics->count_sended_message;
	uint64_t	countSendedByte			 = this->m_pstatistics->count_sended_bytes;
	uint64_t	countSendIOCall			 = this->m_pstatistics->count_send_io;
	uint64_t	countReceived			 = this->m_pstatistics->count_received_message;
	uint64_t	countReceivedByte		 = this->m_pstatistics->count_received_bytes;
	uint64_t	countReceiveIOCall		 = this->m_pstatistics->count_receive_io;

	// 2) 초당 전송률을 구한다.
	float	totalSended_Message_PerSec	 = ((float)(countSended - this->m_count_sended))*1000.0f/tick_differ.count();
	float	totalSended_Byte_PerSec		 = ((float)(countSendedByte - this->m_count_sendedByte))*1000.0f/tick_differ.count();
	float	totalSend_IOCall_PerSec		 = ((float)(countSendIOCall - this->m_count_send_io_call))*1000.0f/tick_differ.count();
	float	totalReceive_Message_PerSec	 = ((float)(countReceived - this->m_count_received))*1000.0f/tick_differ.count();
	float	totalReceive_Byte_PerSec	 = ((float)(countReceivedByte - this->m_count_receivedByte))*1000.0f/tick_differ.count();
	float	totalReceive_IOCall_PerSec	 = ((float)(countReceiveIOCall - this->m_count_receive_io_call))*1000.0f/tick_differ.count();

	// 3) 이전 값을 현재 값으로 Update한다.(다음 Frame에서의 계산을 위해)
	this->m_count_sended				 = countSended;
	this->m_count_sendedByte			 = countSendedByte;
	this->m_count_send_io_call			 = countSendIOCall;
	this->m_count_received				 = countReceived;
	this->m_count_receivedByte			 = countReceivedByte;
	this->m_count_receive_io_call		 = countReceiveIOCall;


	//-----------------------------------------------------------------
	// 3. control에 표시하기.
	//-----------------------------------------------------------------
	// declare) 
	TCHAR tempString[256] = { NULL, };

	// 1) Total send/receive 정보.
	SetDlgItemText(IDC_STATIC_SENDEDPACKET,		to_string_scaled_byte(tempString, 256, countSended).data());
	SetDlgItemText(IDC_STATIC_RECEIVEDPACKET,	to_string_scaled_byte(tempString, 256, countReceived).data());

	// 2) 초당 전송률 표시.
	__sprintf(tempString, TEXT("%.2f"), totalSended_Message_PerSec);	SetDlgItemText(IDC_STATIC_SENDEDPACKET_PERSEC,	tempString);
	SetDlgItemText(IDC_STATIC_SENDEDDATA_PERSEC,to_string_scaled_byte(tempString, 256, totalSended_Byte_PerSec).data());
	SetDlgItemText(IDC_STATIC_SEND_IO_CALL_PERSEC,to_string_scaled_byte(tempString, 256, totalSend_IOCall_PerSec).data());
	__sprintf(tempString, TEXT("%.2f"), totalReceive_Message_PerSec);SetDlgItemText(IDC_STATIC_RECEIVEDPACKET_PERSEC,tempString);
	SetDlgItemText(IDC_STATIC_RECEIVEDDATA_PERSEC, to_string_scaled_byte(tempString, 256, totalReceive_Byte_PerSec).data());
	SetDlgItemText(IDC_STATIC_RECEIVE_IO_CALL_PERSEC, to_string_scaled_byte(tempString, 256, totalReceive_IOCall_PerSec).data());

	// 3) 초당 send/receive햡 표시.(send+Receie)
	__sprintf(tempString, TEXT("%.2f"),	totalSended_Message_PerSec+totalReceive_Message_PerSec);	SetDlgItemText(IDC_STATIC_IO_PACKET_PERSEC,tempString);
	SetDlgItemText(IDC_STATIC_IO_BYTE_PERSEC,	to_string_scaled_byte(tempString, 256, totalSended_Byte_PerSec+totalReceive_Byte_PerSec).data());
	SetDlgItemText(IDC_STATIC_IO_CALL_PERSEC,	to_string_scaled_byte(tempString, 256, totalSend_IOCall_PerSec+totalReceive_IOCall_PerSec).data());


	//-----------------------------------------------------------------
	// 4. Graph처리
	//-----------------------------------------------------------------
	// 1) Traffic Data
	m_graph_traffic.AppendData(0, static_cast<int>(totalSended_Message_PerSec));
	m_graph_traffic.AppendData(1, static_cast<int>(totalReceive_Message_PerSec));
	m_graph_traffic.Scroll(1);
}


void CChild_traffic_info::OnEnable(BOOL is_enable)
{
	CChildDialog::OnEnable(is_enable);

	GetDlgItem(IDC_CUSTOM_TRAFFIC)->EnableWindow(is_enable);
	GetDlgItem(IDC_SLIDER_TRAFFIC)->EnableWindow(is_enable);

	GetDlgItem(IDC_CHECK_GRAPH_SENDED)->EnableWindow(is_enable);
	GetDlgItem(IDC_CHECK_GRAPH_RECEIVED)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_SENDEDPACKET)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVEDPACKET)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SENDEDPACKET_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVEDPACKET_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_IO_PACKET_PERSEC)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_SENDEDDATA_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_RECEIVEDDATA_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_IO_BYTE_PERSEC)->EnableWindow(is_enable);

	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T2)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T3)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T4)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_T5)->EnableWindow(is_enable);

	// Graph를 다시 그린다.
	m_graph_traffic.InvalidateRect(nullptr);
}

void CChild_traffic_info::OnDestroy()
{
	// 1) 부모 Class의 destroy
	__super::OnDestroy();
}

LRESULT CChild_traffic_info::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
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
				EnableWindow(TRUE);
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
				UpdateTrafficInfo();
			}
			break;
	};

	return	0;

}


}