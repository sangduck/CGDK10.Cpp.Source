#pragma once


namespace CGDK
{

class CLogFilterDlg : public CGDIALOG
{
	DECLARE_DYNAMIC(CLogFilterDlg)

public:
	CLogFilterDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLogFilterDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_FILTERS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedCheckLogSystem();
	afx_msg void OnBnClickedCheckLogInformation();
	afx_msg void OnBnClickedCheckLogProgress();
	afx_msg void OnBnClickedCheckLogDebug();
	afx_msg void OnBnClickedCheckLogFatal();
	afx_msg void OnBnClickedCheckLogError();
	afx_msg void OnBnClickedCheckLogWarning();

	afx_msg void OnBnClickedCheckSystemLevel();
	afx_msg void OnBnClickedCheckInfoLevel();
	afx_msg void OnBnClickedCheckProgressLevel();
	afx_msg void OnBnClickedCheckDebugLevel();
	afx_msg void OnBnClickedCheckFatalLevel();
	afx_msg void OnBnClickedCheckErrorLevel();
	afx_msg void OnBnClickedCheckWarningLevel();

	afx_msg void OnBnClickedCheckSystemSource();
	afx_msg void OnBnClickedCheckInfoSource();
	afx_msg void OnBnClickedCheckProgressSource();
	afx_msg void OnBnClickedCheckDebugSource();
	afx_msg void OnBnClickedCheckFatalSource();
	afx_msg void OnBnClickedCheckErrorSource();
	afx_msg void OnBnClickedCheckWarningSource();

	afx_msg void OnBnClickedCheckSystemDestination();
	afx_msg void OnBnClickedCheckInfoDestination();
	afx_msg void OnBnClickedCheckProgressDestination();
	afx_msg void OnBnClickedCheckDebugDestination();
	afx_msg void OnBnClickedCheckFatalDestination();
	afx_msg void OnBnClickedCheckErrorDestination();
	afx_msg void OnBnClickedCheckWarningDestination();

	afx_msg void OnBnClickedCheckSystemLocation();
	afx_msg void OnBnClickedCheckInfoLocation();
	afx_msg void OnBnClickedCheckProgressLocation();
	afx_msg void OnBnClickedCheckDebugLocation();
	afx_msg void OnBnClickedCheckFatalLocation();
	afx_msg void OnBnClickedCheckErrorLocation();
	afx_msg void OnBnClickedCheckWarningLocation();

	void	EnableFilter_System(BOOL _bEnable);
	void	EnableFilter_Information(BOOL _bEnable);
	void	EnableFilter_Progress(BOOL _bEnable);
	void	EnableFilter_Debug(BOOL _bEnable);
	void	EnableFilter_Fatal(BOOL _bEnable);
	void	EnableFilter_Error(BOOL _bEnable);
	void	EnableFilter_Warning(BOOL _bEnable);

public:
			std::array<TCHAR, 256>	m_strName;
			std::array<TCHAR, 256>	m_strFile;

			object_ptr<log_filter>	m_pFilter;
};


}