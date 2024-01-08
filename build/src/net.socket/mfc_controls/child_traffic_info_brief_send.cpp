#include "pch.h"

#if _MSC_VER>=_MSC_VER_2010
#include "afxdialogex.h"
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CChild_traffic_info_brief_send, CChildDialog)

CChild_traffic_info_brief_send::CChild_traffic_info_brief_send(const statistics::unit_traffic* _pinfo/*=nullptr*/,CWnd* pParent /*=nullptr*/)
	: CChildDialog(IDD_DIALOG_TRAFFIC_INFO_BRIEF_SEND, pParent),
	m_pstatistics(_pinfo),
	m_count_sended(0),
	m_count_sendedByte(0)
{
}

CChild_traffic_info_brief_send::~CChild_traffic_info_brief_send()
{
}

void CChild_traffic_info_brief_send::DoDataExchange(CDataExchange* pDX)
{
	CChildDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CChild_traffic_info_brief_send, CChildDialog)
	ON_WM_TIMER()
	ON_WM_ENABLE()
	ON_WM_DESTROY()
	ON_MESSAGE(eMESSAGE::WINDOWS::NOTIFY, &CChild_traffic_info::OnCGMessageNotify)
END_MESSAGE_MAP()

BOOL CChild_traffic_info_brief_send::CreateChild(const statistics::unit_traffic* _pinfo, CWnd* _pwnd)
{
	// 1) Statistics의 포인터를 저장~
	scoped_lock(m_lockable_statistics)
	{
		m_pstatistics	 = _pinfo;
	}

	// 2) 윈도우를 생성한다.
	BOOL	result	 = Create(IDD_DIALOG_TRAFFIC_INFO_BRIEF_SEND, _pwnd);

	// check) 생성에 실패했으면 FALSE를 Return한다.
	RETURN_IF(result==FALSE, FALSE);

	// 3) Show Window를 한다.
	ShowWindow(SW_SHOW);

	// return) 성공!!!
	return	result;
}

BOOL CChild_traffic_info_brief_send::OnInitDialog()
{
	CChildDialog::OnInitDialog();


	//-----------------------------------------------------------------
	// 1. control 초기화.
	//-----------------------------------------------------------------
	// 1) Tick 초기화.
	m_tick_last = chrono::tick::now();


	// return) 
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CChild_traffic_info_brief_send::OnTimer(UINT_PTR nIDEvent)
{
	// 1) ...
	if(nIDEvent == 0)
	{
		// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowEnabled() == FALSE)

		// check) WIndow가 보이지 않으면 더이상 진행하지 않는다.
		RETURN_IF(IsWindowVisible() == FALSE)

		UpdateTrafficeInfoBriefSend();
	}


	//-----------------------------------------------------------------
	// 2. MFC Default
	//-----------------------------------------------------------------
	CChildDialog::OnTimer(nIDEvent);
}

void CChild_traffic_info_brief_send::UpdateTrafficeInfoBriefSend()
{
	// check) 
	RETURN_IF(m_hWnd == nullptr);
	
	// check) WIndow가 불능상태라면 더이상 진행하지 않는다.
	RETURN_IF(IsWindowEnabled() == FALSE)

	// declare) 
	const statistics::unit_traffic* pstatistics = nullptr;

	scoped_lock(m_lockable_statistics)
	{
		pstatistics = m_pstatistics;
	}

	// check) 
	RETURN_IF(pstatistics == nullptr);
	

	//-----------------------------------------------------------------
	// 1. Traffice Info 계산하기.
	//-----------------------------------------------------------------
	// 1) 현재 Tick과 Tick Gap을 구한다.
	chrono::tick::time_point	tick_now	 = chrono::tick::now();
	chrono::tick::milliseconds	tick_differ	 = std::chrono::duration_cast<chrono::milliseconds>(tick_now-m_tick_last);

	// 2) 최종 시간을 last에 넣는다.
	m_tick_last = tick_now;

	// 3) 이전과 현재의 차이를 구한다. (Overflow가 났을 경우를 대비하여 계산해야 한다.)
	LONGLONG countSended			 = pstatistics->count_sended_message;
	LONGLONG countSendedByte		 = pstatistics->count_sended_bytes;

	// 4) 초당 전송률을 구한다.
	float totalSend_Message_PerSec	 = ((float)(countSended-m_count_sended))*1000/tick_differ.count();
	float totalSend_Byte_PerSec		 = ((float)(countSendedByte-m_count_sendedByte))*1000/tick_differ.count();

	// 5) 이전 값을 현재 값으로 Update한다.(다음 Frame에서의 계산을 위해)
	m_count_sended					 = countSended;
	m_count_sendedByte				 = countSendedByte;


	//-----------------------------------------------------------------
	// 2. control에 표시하기.
	//-----------------------------------------------------------------
	// declare) 
	TCHAR	tempString[256];

	// 1) Total send 정보.
	SetDlgItemText(IDC_STATIC_SENDEDMESSAGE, to_string_scaled_byte(tempString, 256, countSended).data());

	// 2) 초당 전송률 표시.
	__sprintf(tempString, TEXT("%.2f"), totalSend_Message_PerSec);SetDlgItemText(IDC_STATIC_SENDEDMESSAGE_PERSEC,tempString);
	SetDlgItemText(IDC_STATIC_SENDEDBYTES_PERSEC, to_string_scaled_byte(tempString, 256, totalSend_Byte_PerSec).data());
}

void CChild_traffic_info_brief_send::OnEnable(BOOL is_enable)
{
	// Dialog)
	__super::OnEnable(is_enable);

	// 1) 정보를 출력할 Static
	GetDlgItem(IDC_STATIC_SENDEDMESSAGE)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SENDEDMESSAGE_PERSEC)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SENDEDBYTES_PERSEC)->EnableWindow(is_enable);

	//2) Static
	GetDlgItem(IDC_STATIC_T1)->EnableWindow(is_enable);
}

void CChild_traffic_info_brief_send::OnDestroy()
{
	// 1) 부모 Class의 destroy
	__super::OnDestroy();
}

LRESULT CChild_traffic_info_brief_send::OnCGMessageNotify(WPARAM _wparam, LPARAM /*_lparam*/)
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
				UpdateTrafficeInfoBriefSend();
			}
			break;
	};

	return	0;

}


}