// NodePropertyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"

#include "Matrix.h"
#include "Model.h"

#include "NodePropertyDlg.h"
#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "ManagerTree.h"
#include "FrameManager2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNodePropertyDlg dialog


CNodePropertyDlg::CNodePropertyDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNodePropertyDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNodePropertyDlg)
	m_tris = _T("");
	//}}AFX_DATA_INIT
}


void CNodePropertyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNodePropertyDlg)
	DDX_Text(pDX, IDC_EDIT1, m_tris);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNodePropertyDlg, CDialog)
	//{{AFX_MSG_MAP(CNodePropertyDlg)
	ON_BN_CLICKED(IDAPPLY, OnApply)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNodePropertyDlg message handlers

void CNodePropertyDlg::OnApply()
{

}

BOOL CNodePropertyDlg::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Add your specialized code here and/or call the base class

	return CDialog::PreCreateWindow(cs);
}

BOOL CNodePropertyDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CNodePropertyDlg::OnOK()
{
	CDialog::OnOK();
}
