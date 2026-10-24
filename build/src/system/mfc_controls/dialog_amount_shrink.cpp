#include "pch.h"
#include "Dialog_amount_shrink.h"
#include "afxdialogex.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

IMPLEMENT_DYNAMIC(CDialogAmountShrink, CGDIALOG)

CDialogAmountShrink::CDialogAmountShrink(CWnd* pnode_parent /*=NULL*/) noexcept
	: CGDIALOG(CDialogAmountShrink::IDD, pnode_parent)
{

}

CDialogAmountShrink::~CDialogAmountShrink() noexcept
{

}

void CDialogAmountShrink::DoDataExchange(CDataExchange* pDX)
{
	CGDIALOG::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDialogAmountShrink, CGDIALOG)
	ON_BN_CLICKED(IDOK, &CDialogAmountShrink::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CDialogAmountShrink::OnBnClickedCancel)
	ON_BN_CLICKED(ID_BUTTON_EXCESS_ALL, &CDialogAmountShrink::OnBnClickedButtonExcessAll)
	ON_BN_CLICKED(ID_STACKED_ALL, &CDialogAmountShrink::OnBnClickedStackedAll)
END_MESSAGE_MAP()

void CDialogAmountShrink::OnBnClickedOk()
{
	m_amount = GetDlgItemInt(IDC_EDIT_AMOUNT);

	CGDIALOG::OnOK();
}

void CDialogAmountShrink::OnBnClickedCancel()
{
	CGDIALOG::OnCancel();
}

void CDialogAmountShrink::OnBnClickedButtonExcessAll()
{
	m_amount = -1;

	CGDIALOG::OnOK();
}

void CDialogAmountShrink::OnBnClickedStackedAll()
{
	m_amount = -2;

	CGDIALOG::OnOK();
}


}