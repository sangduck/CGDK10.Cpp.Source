#pragma once


// CDialogAmountShrink dialog
namespace CGDK
{

class CDialogAmountShrink : public CGDIALOG
{
	DECLARE_DYNAMIC(CDialogAmountShrink)

public:
			CDialogAmountShrink(CWnd* pnode_parent = NULL) noexcept;   // standard constructor
	virtual ~CDialogAmountShrink() noexcept;

// Dialog Data
	enum { IDD = IDD_DIALOG_AMOUNT_SHRINK };

public:
		int		m_amount;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonExcessAll();
	afx_msg void OnBnClickedStackedAll();
};


}