// TickerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "qMView.h"
#include "TickerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTickerDlg dialog


CTickerDlg::CTickerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTickerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTickerDlg)
	m_nUseTicker = FALSE;
	m_nTickerDelay = 0;
	//}}AFX_DATA_INIT
}


void CTickerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTickerDlg)
	DDX_Control(pDX, IDC_SPIN1, m_nSpinCtrl);
	DDX_Check(pDX, IDC_TICKTOG, m_nUseTicker);
	DDX_Text(pDX, IDC_EDIT1, m_nTickerDelay);
	DDV_MinMaxLong(pDX, m_nTickerDelay, 0, 10000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTickerDlg, CDialog)
	//{{AFX_MSG_MAP(CTickerDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTickerDlg message handlers

BOOL CTickerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	UDACCEL acc[3];

	acc[0].nSec = 1;
	acc[0].nInc = 2;

	acc[1].nSec = 2;
	acc[1].nInc = 10;

	acc[2].nSec = 3;
	acc[2].nInc = 50;

	m_nSpinCtrl.SetAccel(3, acc);
	m_nSpinCtrl.SetRange(0, 1000);

	return TRUE;
}
