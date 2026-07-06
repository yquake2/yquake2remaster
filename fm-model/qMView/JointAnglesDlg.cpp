// JointAnglesDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"

#include "Matrix.h"
#include "Model.h"
#include "JointAnglesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJointAnglesDlg dialog


CJointAnglesDlg::CJointAnglesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJointAnglesDlg::IDD, pParent)
{
	m_model = NULL;
	//{{AFX_DATA_INIT(CJointAnglesDlg)
	m_nXVal = 0.0f;
	m_nYVal = 0.0f;
	m_nZVal = 0.0f;
	//}}AFX_DATA_INIT
}


void CJointAnglesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CJointAnglesDlg)
	DDX_Control(pDX, IDC_ZVAL, m_nZCtrl);
	DDX_Control(pDX, IDC_YVAL, m_nYCtrl);
	DDX_Control(pDX, IDC_XVAL, m_nXCtrl);
	DDX_Text(pDX, IDC_XVAL, m_nXVal);
	DDX_Text(pDX, IDC_YVAL, m_nYVal);
	DDX_Text(pDX, IDC_ZVAL, m_nZVal);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CJointAnglesDlg, CDialog)
	//{{AFX_MSG_MAP(CJointAnglesDlg)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_APPLY, OnApplyClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJointAnglesDlg message handlers

void CJointAnglesDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

BOOL CJointAnglesDlg::OnInitDialog()
{
	if (m_model != NULL)
	{
		m_model->GetModelAngles(&m_nXVal, &m_nYVal, &m_nZVal);
	}
	CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CJointAnglesDlg::OnApplyClicked()
{
	UpdateData(true);

	if (m_model != NULL)
	{
		m_model->SetModelAngles(m_nXVal, m_nYVal, m_nZVal);

//		m_model->ShowFrame();
	}
}

void CJointAnglesDlg::UpdateVisuals(void)
{
	if (m_model != NULL)
	{
		m_model->GetModelAngles(&m_nXVal, &m_nYVal, &m_nZVal);

		UpdateData(false);
	}
}

void CJointAnglesDlg::SetModel(CModel* model)
{
	model = m_model;
}
