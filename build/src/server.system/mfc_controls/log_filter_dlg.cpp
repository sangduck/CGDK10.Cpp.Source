#include "pch.h"
#include "log_filter_dlg.h"

namespace CGDK
{

IMPLEMENT_DYNAMIC(CLogFilterDlg, CGDIALOG)

CLogFilterDlg::CLogFilterDlg(CWnd* pParent /*=NULL*/)
	: CGDIALOG(CLogFilterDlg::IDD, pParent)
{
	m_strName.at(0) = NULL;
	m_strFile.at(0) = NULL;
}

CLogFilterDlg::~CLogFilterDlg()
{
}

void CLogFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CGDIALOG::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLogFilterDlg, CGDIALOG)
	ON_BN_CLICKED(IDC_CHECK_LOG_SYSTEM, &CLogFilterDlg::OnBnClickedCheckLogSystem)
	ON_BN_CLICKED(IDC_CHECK_LOG_INFORMATION, &CLogFilterDlg::OnBnClickedCheckLogInformation)
	ON_BN_CLICKED(IDC_CHECK_LOG_PROGRESS, &CLogFilterDlg::OnBnClickedCheckLogProgress)
	ON_BN_CLICKED(IDC_CHECK_LOG_DEBUG, &CLogFilterDlg::OnBnClickedCheckLogDebug)
	ON_BN_CLICKED(IDC_CHECK_LOG_FATAL, &CLogFilterDlg::OnBnClickedCheckLogFatal)
	ON_BN_CLICKED(IDC_CHECK_LOG_ERROR, &CLogFilterDlg::OnBnClickedCheckLogError)
	//ON_BN_CLICKED(IDC_CHECK_LOG_WARNING, &CLogFilterDlg::OnBnClickedCheckLogWarning)

	ON_BN_CLICKED(IDC_CHECK_SYSTEM_LEVEL, &CLogFilterDlg::OnBnClickedCheckSystemLevel)
	ON_BN_CLICKED(IDC_CHECK_INFO_LEVEL, &CLogFilterDlg::OnBnClickedCheckInfoLevel)
	ON_BN_CLICKED(IDC_CHECK_PROGRESS_LEVEL, &CLogFilterDlg::OnBnClickedCheckProgressLevel)
	ON_BN_CLICKED(IDC_CHECK_DEBUG_LEVEL, &CLogFilterDlg::OnBnClickedCheckDebugLevel)
	ON_BN_CLICKED(IDC_CHECK_FATAL_LEVEL, &CLogFilterDlg::OnBnClickedCheckFatalLevel)
	ON_BN_CLICKED(IDC_CHECK_ERROR_LEVEL, &CLogFilterDlg::OnBnClickedCheckErrorLevel)
	//ON_BN_CLICKED(IDC_CHECK_WARNING_LEVEL, &CLogFilterDlg::OnBnClickedCheckWarningLevel)

	ON_BN_CLICKED(IDC_CHECK_SYSTEM_SOURCE, &CLogFilterDlg::OnBnClickedCheckSystemSource)
	ON_BN_CLICKED(IDC_CHECK_INFO_SOURCE, &CLogFilterDlg::OnBnClickedCheckInfoSource)
	ON_BN_CLICKED(IDC_CHECK_PROGRESS_SOURCE, &CLogFilterDlg::OnBnClickedCheckProgressSource)
	ON_BN_CLICKED(IDC_CHECK_DEBUG_SOURCE, &CLogFilterDlg::OnBnClickedCheckDebugSource)
	ON_BN_CLICKED(IDC_CHECK_FATAL_SOURCE, &CLogFilterDlg::OnBnClickedCheckFatalSource)
	ON_BN_CLICKED(IDC_CHECK_ERROR_SOURCE, &CLogFilterDlg::OnBnClickedCheckErrorSource)
	//ON_BN_CLICKED(IDC_CHECK_WARNING_SOURCE, &CLogFilterDlg::OnBnClickedCheckWarningSource)

	ON_BN_CLICKED(IDC_CHECK_SYSTEM_DESTINATION, &CLogFilterDlg::OnBnClickedCheckSystemDestination)
	ON_BN_CLICKED(IDC_CHECK_INFO_DESTINATION, &CLogFilterDlg::OnBnClickedCheckInfoDestination)
	ON_BN_CLICKED(IDC_CHECK_PROGRESS_DESTINATION, &CLogFilterDlg::OnBnClickedCheckProgressDestination)
	ON_BN_CLICKED(IDC_CHECK_DEBUG_DESTINATION, &CLogFilterDlg::OnBnClickedCheckDebugDestination)
	ON_BN_CLICKED(IDC_CHECK_FATAL_DESTINATION, &CLogFilterDlg::OnBnClickedCheckFatalDestination)
	ON_BN_CLICKED(IDC_CHECK_ERROR_DESTINATION, &CLogFilterDlg::OnBnClickedCheckErrorDestination)
	//ON_BN_CLICKED(IDC_CHECK_WARNING_DESTINATION, &CLogFilterDlg::OnBnClickedCheckWarningDestination)

	ON_BN_CLICKED(IDC_CHECK_SYSTEM_LOCATION, &CLogFilterDlg::OnBnClickedCheckSystemLocation)
	ON_BN_CLICKED(IDC_CHECK_INFO_LOCATION, &CLogFilterDlg::OnBnClickedCheckInfoLocation)
	ON_BN_CLICKED(IDC_CHECK_PROGRESS_LOCATION, &CLogFilterDlg::OnBnClickedCheckProgressLocation)
	ON_BN_CLICKED(IDC_CHECK_DEBUG_LOCATION, &CLogFilterDlg::OnBnClickedCheckDebugLocation)
	ON_BN_CLICKED(IDC_CHECK_FATAL_LOCATION, &CLogFilterDlg::OnBnClickedCheckFatalLocation)
	ON_BN_CLICKED(IDC_CHECK_ERROR_LOCATION, &CLogFilterDlg::OnBnClickedCheckErrorLocation)
	//ON_BN_CLICKED(IDC_CHECK_WARNING_LOCATION, &CLogFilterDlg::OnBnClickedCheckWarningLocation)

	ON_BN_CLICKED(IDOK, &CLogFilterDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CLogFilterDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CLogFilterDlg message handlers
void CLogFilterDlg::EnableFilter_System(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;

	GetDlgItem(IDC_CHECK_SYSTEM_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_SYSTEM_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_SYSTEM_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_SYSTEM_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_SYSTEM_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_SYSTEM_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_SYSTEM_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_4)->EnableWindow(_bEnable && is_enable);
}

// CLogFilterDlg message handlers
void CLogFilterDlg::EnableFilter_Information(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;

	GetDlgItem(IDC_CHECK_INFO_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_INFO_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_INFO_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_INFO_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_INFO_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_INFO_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_INFO_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_INFO_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_INFO_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_INFO_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_INFO_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_INFO_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::EnableFilter_Progress(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;
	
	GetDlgItem(IDC_CHECK_PROGRESS_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_PROGRESS_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_PROGRESS_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_PROGRESS_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_PROGRESS_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_PROGRESS_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_PROGRESS_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::EnableFilter_Debug(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;
	
	GetDlgItem(IDC_CHECK_DEBUG_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_DEBUG_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_DEBUG_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_DEBUG_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_DEBUG_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_DEBUG_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_DEBUG_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_DEBUG_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::EnableFilter_Fatal(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;

	GetDlgItem(IDC_CHECK_FATAL_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_FATAL_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_FATAL_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_FATAL_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_FATAL_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_FATAL_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_FATAL_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_FATAL_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_FATAL_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_FATAL_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_FATAL_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_FATAL_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_FATAL_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_FATAL_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_FATAL_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_FATAL_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::EnableFilter_Error(BOOL _bEnable)
{
	// declare) 
	BOOL is_enable;

	GetDlgItem(IDC_CHECK_ERROR_LEVEL)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LEVEL))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_ERROR_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_ERROR_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_ERROR_1)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_ERROR_SOURCE)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_SOURCE))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_ERROR_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_ERROR_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_ERROR_2)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_ERROR_DESTINATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_ERROR_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_ERROR_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_ERROR_3)->EnableWindow(_bEnable && is_enable);

	GetDlgItem(IDC_CHECK_ERROR_LOCATION)->EnableWindow(_bEnable);
	is_enable	 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LOCATION))->GetCheck()!=BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_ERROR_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_EDIT_ERROR_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	GetDlgItem(IDC_STATIC_ERROR_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::EnableFilter_Warning(BOOL /*_bEnable*/)
{
	//// declare) 
	//BOOL is_enable;

	//GetDlgItem(IDC_CHECK_WARNING_LEVEL)->EnableWindow(_bEnable);
	//is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LEVEL))->GetCheck() != BST_UNCHECKED);
	//GetDlgItem(IDC_EDIT_WARNING_LEVEL_MIN)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_LEVEL_MAX)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_1)->EnableWindow(_bEnable && is_enable);

	//GetDlgItem(IDC_CHECK_WARNING_SOURCE)->EnableWindow(_bEnable);
	//is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_SOURCE))->GetCheck() != BST_UNCHECKED);
	//GetDlgItem(IDC_EDIT_WARNING_SOURCE_MIN)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_SOURCE_MAX)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_2)->EnableWindow(_bEnable && is_enable);

	//GetDlgItem(IDC_CHECK_WARNING_DESTINATION)->EnableWindow(_bEnable);
	//is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_DESTINATION))->GetCheck() != BST_UNCHECKED);
	//GetDlgItem(IDC_EDIT_WARNING_DESTINATION_MIN)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_DESTINATION_MAX)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_3)->EnableWindow(_bEnable && is_enable);

	//GetDlgItem(IDC_CHECK_WARNING_LOCATION)->EnableWindow(_bEnable);
	//is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LOCATION))->GetCheck() != BST_UNCHECKED);
	//GetDlgItem(IDC_EDIT_WARNING_LOCATION_MIN)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_LOCATION_MAX)->EnableWindow(_bEnable && is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_4)->EnableWindow(_bEnable && is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogSystem()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_SYSTEM))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_System(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogInformation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_INFORMATION))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_Information(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogProgress()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_PROGRESS))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_Progress(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogDebug()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_DEBUG))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_Debug(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogFatal()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_FATAL))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_Fatal(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogError()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_ERROR))->GetCheck()!=BST_UNCHECKED);

	EnableFilter_Error(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckLogWarning()
{
	//BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_LOG_WARNING))->GetCheck() != BST_UNCHECKED);

	//EnableFilter_Warning(is_enable);
}



void CLogFilterDlg::OnBnClickedCheckSystemLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_SYSTEM_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckInfoLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_INFO_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_INFO_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckProgressLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_PROGRESS_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckDebugLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_DEBUG_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckFatalLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_FATAL_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_FATAL_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_FATAL_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckErrorLevel()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LEVEL))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_ERROR_LEVEL_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_ERROR_LEVEL_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_ERROR_1)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckWarningLevel()
{
	//BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LEVEL))->GetCheck() != BST_UNCHECKED);

	//GetDlgItem(IDC_EDIT_WARNING_LEVEL_MIN)->EnableWindow(is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_LEVEL_MAX)->EnableWindow(is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_1)->EnableWindow(is_enable);
}



void CLogFilterDlg::OnBnClickedCheckSystemSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_SYSTEM_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckInfoSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_INFO_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_INFO_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_INFO_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckProgressSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_PROGRESS_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckDebugSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_DEBUG_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckFatalSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_FATAL_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_FATAL_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_FATAL_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckErrorSource()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_SOURCE))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_ERROR_SOURCE_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_ERROR_SOURCE_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_ERROR_2)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckWarningSource()
{
	//BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_SOURCE))->GetCheck() != BST_UNCHECKED);

	//GetDlgItem(IDC_EDIT_WARNING_SOURCE_MIN)->EnableWindow(is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_SOURCE_MAX)->EnableWindow(is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_2)->EnableWindow(is_enable);
}



void CLogFilterDlg::OnBnClickedCheckSystemDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_SYSTEM_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_3)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckInfoDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_INFO_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_INFO_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_INFO_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_3)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckProgressDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_PROGRESS_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_3)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckDebugDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_DEBUG_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_3)->EnableWindow(is_enable);
}


void CLogFilterDlg::OnBnClickedCheckFatalDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_FATAL_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_FATAL_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_FATAL_3)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckErrorDestination()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_DESTINATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_ERROR_DESTINATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_ERROR_DESTINATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_ERROR_3)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckWarningDestination()
{
	//BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_DESTINATION))->GetCheck() != BST_UNCHECKED);

	//GetDlgItem(IDC_EDIT_WARNING_DESTINATION_MIN)->EnableWindow(is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_DESTINATION_MAX)->EnableWindow(is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_3)->EnableWindow(is_enable);
}



void CLogFilterDlg::OnBnClickedCheckSystemLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_SYSTEM_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_SYSTEM_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_SYSTEM_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckInfoLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_INFO_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_INFO_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_INFORMATION_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckProgressLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_PROGRESS_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_PROGRESS_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_PROGRESS_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckDebugLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_DEBUG_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_DEBUG_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_DEBUG_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckFatalLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_FATAL_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_FATAL_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_FATAL_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckErrorLocation()
{
	BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LOCATION))->GetCheck()!=BST_UNCHECKED);

	GetDlgItem(IDC_EDIT_ERROR_LOCATION_MIN)->EnableWindow(is_enable);
	GetDlgItem(IDC_EDIT_ERROR_LOCATION_MAX)->EnableWindow(is_enable);
	GetDlgItem(IDC_STATIC_ERROR_4)->EnableWindow(is_enable);
}

void CLogFilterDlg::OnBnClickedCheckWarningLocation()
{
	//BOOL is_enable = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LOCATION))->GetCheck() != BST_UNCHECKED);

	//GetDlgItem(IDC_EDIT_WARNING_LOCATION_MIN)->EnableWindow(is_enable);
	//GetDlgItem(IDC_EDIT_WARNING_LOCATION_MAX)->EnableWindow(is_enable);
	//GetDlgItem(IDC_STATIC_WARNING_4)->EnableWindow(is_enable);
}


BOOL CLogFilterDlg::OnInitDialog()
{
	CGDIALOG::OnInitDialog();

	// 1) Name을 설정한다.
	SetDlgItemText(IDC_EDIT_NAME, m_strName.data());

	// 2) FileName을 설정한다.
	SetDlgItemText(IDC_EDIT_FILENAME, m_strFile.data());

	// 3) Filter의 내용을 설정한다.
	if(m_pFilter.exist())
	{
		// declare) 
		log_filter::FILTER_INFO	temp_filter_info;

		// - System
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::system);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_SYSTEM))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_SYSTEM_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_SYSTEM_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_SYSTEM_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_SYSTEM_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_SYSTEM_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		SetDlgItemInt(IDC_EDIT_SYSTEM_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_SYSTEM_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_SYSTEM_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_System(temp_filter_info.is_enable);

		// - Info
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::info);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_INFORMATION))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_INFO_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_INFO_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_INFO_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_INFO_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_INFO_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_INFO_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_INFO_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_INFO_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		SetDlgItemInt(IDC_EDIT_INFO_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_INFO_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_INFO_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_INFO_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_Information(temp_filter_info.is_enable);

		// - Progress
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::progress);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_PROGRESS))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_PROGRESS_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_PROGRESS_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_PROGRESS_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_PROGRESS_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_PROGRESS_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));;
		SetDlgItemInt(IDC_EDIT_PROGRESS_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_PROGRESS_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_PROGRESS_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_Progress(temp_filter_info.is_enable);

		// - Debug
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::debug);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_DEBUG))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_DEBUG_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_DEBUG_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_DEBUG_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_DEBUG_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_DEBUG_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		SetDlgItemInt(IDC_EDIT_DEBUG_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_DEBUG_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_DEBUG_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_Debug(temp_filter_info.is_enable);

		// - Fatal
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::fatal);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_FATAL))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_FATAL_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_FATAL_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_FATAL_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_FATAL_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_FATAL_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		SetDlgItemInt(IDC_EDIT_FATAL_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_FATAL_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_FATAL_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_Fatal(temp_filter_info.is_enable);

		// - Error
		temp_filter_info = m_pFilter->get_filter_info(log_type_t::error);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_ERROR))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_ERROR_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		SetDlgItemInt(IDC_EDIT_ERROR_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_ERROR_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		SetDlgItemInt(IDC_EDIT_ERROR_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_ERROR_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		SetDlgItemInt(IDC_EDIT_ERROR_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(IDC_EDIT_ERROR_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		SetDlgItemInt(IDC_EDIT_ERROR_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		EnableFilter_Error(temp_filter_info.is_enable);

		//// - Warning
		//temp_filter_info = m_pFilter->get_filter_info(log_type_t::warning);
		//((CButton*)GetDlgItem(IDC_CHECK_LOG_WARNING))->SetCheck(temp_filter_info.is_enable ? BST_CHECKED : BST_UNCHECKED);
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_LEVEL))->SetCheck(temp_filter_info.is_limit_level ? BST_CHECKED : BST_UNCHECKED);
		//SetDlgItemInt(IDC_EDIT_WARNING_LEVEL_MIN, (UINT)temp_filter_info.range_level_min);
		//SetDlgItemInt(IDC_EDIT_WARNING_LEVEL_MAX, (UINT)temp_filter_info.range_level_max);
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_SOURCE))->SetCheck(temp_filter_info.is_limit_source ? BST_CHECKED : BST_UNCHECKED);
		//SetDlgItemInt(IDC_EDIT_WARNING_SOURCE_MIN, static_cast<UINT>(temp_filter_info.range_source_min.value));
		//SetDlgItemInt(IDC_EDIT_WARNING_SOURCE_MAX, static_cast<UINT>(temp_filter_info.range_source_max.value));
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_DESTINATION))->SetCheck(temp_filter_info.is_limit_destination ? BST_CHECKED : BST_UNCHECKED);
		//SetDlgItemInt(IDC_EDIT_WARNING_DESTINATION_MIN, static_cast<UINT>(temp_filter_info.range_destination_min.value));
		//SetDlgItemInt(IDC_EDIT_WARNING_DESTINATION_MAX, static_cast<UINT>(temp_filter_info.range_destination_max.value));
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_LOCATION))->SetCheck(temp_filter_info.is_limit_origin ? BST_CHECKED : BST_UNCHECKED);
		//SetDlgItemInt(IDC_EDIT_WARNING_LOCATION_MIN, static_cast<UINT>(temp_filter_info.range_origin_min.value));
		//SetDlgItemInt(IDC_EDIT_WARNING_LOCATION_MAX, static_cast<UINT>(temp_filter_info.range_origin_max.value));
		//EnableFilter_WARNING(temp_filter_info.is_enable);
	}
	else
	{
		// -
		((CButton*)GetDlgItem(IDC_CHECK_LOG_SYSTEM))->SetCheck(BST_CHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_INFORMATION))->SetCheck(BST_CHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_PROGRESS))->SetCheck(BST_CHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_DEBUG))->SetCheck(BST_CHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_FATAL))->SetCheck(BST_CHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_LOG_ERROR))->SetCheck(BST_CHECKED);
		//((CButton*)GetDlgItem(IDC_CHECK_LOG_WARNING))->SetCheck(BST_CHECKED);

		// - 
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LOCATION))->SetCheck(BST_UNCHECKED);

		// - 
		((CButton*)GetDlgItem(IDC_CHECK_INFO_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_INFO_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_INFO_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_INFO_LOCATION))->SetCheck(BST_UNCHECKED);

		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LOCATION))->SetCheck(BST_UNCHECKED);

		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LOCATION))->SetCheck(BST_UNCHECKED);

		((CButton*)GetDlgItem(IDC_CHECK_FATAL_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_FATAL_LOCATION))->SetCheck(BST_UNCHECKED);

		((CButton*)GetDlgItem(IDC_CHECK_ERROR_LEVEL))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_SOURCE))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_DESTINATION))->SetCheck(BST_UNCHECKED);
		((CButton*)GetDlgItem(IDC_CHECK_ERROR_LOCATION))->SetCheck(BST_UNCHECKED);

		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_LEVEL))->SetCheck(BST_UNCHECKED);
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_SOURCE))->SetCheck(BST_UNCHECKED);
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_DESTINATION))->SetCheck(BST_UNCHECKED);
		//((CButton*)GetDlgItem(IDC_CHECK_WARNING_LOCATION))->SetCheck(BST_UNCHECKED);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CLogFilterDlg::OnBnClickedOk()
{
	// 1) Name을 얻는다.
	GetDlgItemText(IDC_EDIT_NAME, m_strName.data(), static_cast<int>(m_strName.size()));

	// check) 
	ERROR_RETURN_IF(m_strName.at(0)==NULL, , AfxMessageBox(TEXT("No Name"), MB_OK));

	// 2) File Name을 얻는다.
	GetDlgItemText(IDC_EDIT_FILENAME, m_strFile.data(), static_cast<int>(m_strFile.size()));

	// declare) 
	log_filter::FILTER_INFO	temp_filter_info;

	// 3) Filter를 새로 생성한다.
	auto pFilter = make_object<log_filter>();

	// 4) Info
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_SYSTEM))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t{static_cast<int>(GetDlgItemInt(IDC_EDIT_SYSTEM_LEVEL_MIN))};
	temp_filter_info.range_level_max		 = log_level_t{ static_cast<int>(GetDlgItemInt(IDC_EDIT_SYSTEM_LEVEL_MAX))};
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{GetDlgItemInt(IDC_EDIT_SYSTEM_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_SYSTEM_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_SYSTEM_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_SYSTEM_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_SYSTEM_LOCATION))->GetCheck()!=BST_UNCHECKED);;
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_SYSTEM_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_SYSTEM_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::system, temp_filter_info);

	// 5) Info
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_INFORMATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t{ static_cast<int>(GetDlgItemInt(IDC_EDIT_INFO_LEVEL_MIN))};
	temp_filter_info.range_level_max		 = log_level_t{ static_cast<int>(GetDlgItemInt(IDC_EDIT_INFO_LEVEL_MAX))};
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{GetDlgItemInt(IDC_EDIT_INFO_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{GetDlgItemInt(IDC_EDIT_INFO_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_INFO_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_INFO_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_INFO_LOCATION))->GetCheck()!=BST_UNCHECKED);;
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_INFO_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_INFO_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::info, temp_filter_info);

	// 6) Progress
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_PROGRESS))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_PROGRESS_LEVEL_MIN)));
	temp_filter_info.range_level_max		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_PROGRESS_LEVEL_MAX)));
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_PROGRESS_LOCATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_PROGRESS_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::progress, temp_filter_info);

	// 7) Debug
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_DEBUG))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_DEBUG_LEVEL_MIN)));
	temp_filter_info.range_level_max		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_DEBUG_LEVEL_MAX)));
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{ GetDlgItemInt(IDC_EDIT_DEBUG_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_DEBUG_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_DEBUG_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_DEBUG_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_DEBUG_LOCATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_DEBUG_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_DEBUG_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::debug, temp_filter_info);

	// 8) Fatal
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_FATAL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_FATAL_LEVEL_MIN)));
	temp_filter_info.range_level_max		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_FATAL_LEVEL_MAX)));
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{ GetDlgItemInt(IDC_EDIT_FATAL_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_FATAL_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_FATAL_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_FATAL_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_FATAL_LOCATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_FATAL_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_FATAL_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::fatal, temp_filter_info);

	// 9) Error
	temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_ERROR))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LEVEL))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_level_min		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_ERROR_LEVEL_MIN)));
	temp_filter_info.range_level_max		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_ERROR_LEVEL_MAX)));
	temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_SOURCE))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_source_min		 = log_source_t{ GetDlgItemInt(IDC_EDIT_ERROR_SOURCE_MIN)};
	temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_ERROR_SOURCE_MAX)};
	temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_ERROR_DESTINATION_MIN)};
	temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_ERROR_DESTINATION_MAX)};
	temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_ERROR_LOCATION))->GetCheck()!=BST_UNCHECKED);
	temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_ERROR_LOCATION_MIN)};
	temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_ERROR_LOCATION_MAX)};
	pFilter->set_filter_info(log_type_t::error, temp_filter_info);

	//// 10) Warning
	//temp_filter_info.is_enable				 = (((CButton*)GetDlgItem(IDC_CHECK_LOG_WARNING))->GetCheck()!=BST_UNCHECKED);
	//temp_filter_info.is_limit_level			 = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LEVEL))->GetCheck()!=BST_UNCHECKED);
	//temp_filter_info.range_level_min		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_WARNING_LEVEL_MIN)));
	//temp_filter_info.range_level_max		 = log_level_t(static_cast<int>(GetDlgItemInt(IDC_EDIT_WARNING_LEVEL_MAX)));
	//temp_filter_info.is_limit_source		 = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_SOURCE))->GetCheck()!=BST_UNCHECKED);
	//temp_filter_info.range_source_min		 = log_source_t{ GetDlgItemInt(IDC_EDIT_WARNING_SOURCE_MIN)};
	//temp_filter_info.range_source_max		 = log_source_t{ GetDlgItemInt(IDC_EDIT_WARNING_SOURCE_MAX)};
	//temp_filter_info.is_limit_destination	 = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_DESTINATION))->GetCheck()!=BST_UNCHECKED);
	//temp_filter_info.range_destination_min	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_WARNING_DESTINATION_MIN)};
	//temp_filter_info.range_destination_max	 = log_destination_t{ GetDlgItemInt(IDC_EDIT_WARNING_DESTINATION_MAX)};
	//temp_filter_info.is_limit_origin		 = (((CButton*)GetDlgItem(IDC_CHECK_WARNING_LOCATION))->GetCheck()!=BST_UNCHECKED);
	//temp_filter_info.range_origin_min		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_WARNING_LOCATION_MIN)};
	//temp_filter_info.range_origin_max		 = log_origin_t{ GetDlgItemInt(IDC_EDIT_WARNING_LOCATION_MAX)};
	//pFilter->set_filter_info(log_type_t::warning, temp_filter_info);

	// 11) User
	temp_filter_info.is_enable				 = false;
	temp_filter_info.is_limit_level			 = true;
	temp_filter_info.range_level_min		 = log_level_t::normal();
	temp_filter_info.range_level_max		 = log_level_t::normal();
	temp_filter_info.is_limit_source		 = true;
	temp_filter_info.range_source_min		 = log_source_t::min_();
	temp_filter_info.range_source_max		 = log_source_t::min_();
	temp_filter_info.is_limit_destination	 = true;
	temp_filter_info.range_destination_min	 = log_destination_t::min_();
	temp_filter_info.range_destination_max	 = log_destination_t::min_();
	temp_filter_info.is_limit_origin		 = true;
	temp_filter_info.range_origin_min		 = log_origin_t::min_();
	temp_filter_info.range_origin_max		 = log_origin_t::min_();
	pFilter->set_filter_info(log_type_t::user, temp_filter_info);

	// 11) Filter를 설정한다.
	m_pFilter = std::move(pFilter);
		
	// 12) Dialog
	CGDIALOG::OnOK();
}


void CLogFilterDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CGDIALOG::OnCancel();
}


}