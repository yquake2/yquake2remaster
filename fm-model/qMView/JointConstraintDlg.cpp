// JointConstraintDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"
#include "Matrix.h"

#include "Model.h"
#include "JointConstraintDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJointConstraintDlg dialog

CJointConstraintDlg::CJointConstraintDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJointConstraintDlg::IDD, pParent)
{
	m_model = NULL;
	//{{AFX_DATA_INIT(CJointConstraintDlg)
	m_nXVal = 0.0f;
	m_nYVal = 0.0f;
	m_nZVal = 0.0f;
	m_nXVal2 = 0.0f;
	m_nYVal2 = 0.0f;
	m_nZVal2 = 0.0f;
	//}}AFX_DATA_INIT
}


void CJointConstraintDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJointConstraintDlg)
	DDX_Text(pDX, IDC_XVAL, m_nXVal);
	DDX_Text(pDX, IDC_YVAL, m_nYVal);
	DDX_Text(pDX, IDC_ZVAL, m_nZVal);
	DDX_Text(pDX, IDC_XVAL2, m_nXVal2);
	DDX_Text(pDX, IDC_YVAL2, m_nYVal2);
	DDX_Text(pDX, IDC_ZVAL2, m_nZVal2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJointConstraintDlg, CDialog)
	//{{AFX_MSG_MAP(CJointConstraintDlg)
		// NOTE: the ClassWizard will add message map macros here
	ON_BN_CLICKED(IDC_APPLY, OnApplyClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJointConstraintDlg message handlers

BOOL CJointConstraintDlg::OnInitDialog()
{
	if (m_model != NULL)
	{
		m_model->GetConstraintAngleMaxs(&m_nXVal, &m_nYVal, &m_nZVal);
		m_model->GetConstraintAngleMins(&m_nXVal2, &m_nYVal2, &m_nZVal2);
	}
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CJointConstraintDlg::OnApplyClicked()
{
	UpdateData(true);
	if (m_model != NULL)
	{
		m_model->SetConstraintAngleMaxs(m_nXVal, m_nYVal, m_nZVal);
		m_model->SetConstraintAngleMins(m_nXVal2, m_nYVal2, m_nZVal2);
//		m_model->ShowFrame();
	}
}

void CJointConstraintDlg::SetModel(CModel* model)
{
	m_model = model;
}
