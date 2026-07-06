// ManagerTree.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"
#include "Matrix.h"
#include "Model.h"

#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "ManagerTree.h"
#include "NodeTreeCtrl.h"
#include "JointTreeCtrl.h"
#include "FrameManager2.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CManagerTree

CManagerTree::CManagerTree()
{
}

CManagerTree::~CManagerTree()
{
}

BOOL CManagerTree::Create(CWnd* pParentWnd, UINT nStyle)
{
	if (!CDialogBar::Create (pParentWnd, CG_IDD_TREEMANAGER, nStyle, CG_IDD_TREEMANAGER))
		return FALSE;

	m_nFrameTree.SubclassDlgItem(IDC_FRAMETREE,this);
	m_nSkinTree.SubclassDlgItem(IDC_SKINTREE,this);
	CModel* model = GetModel();
	m_nFrameTree.SetModel(model);
	m_nSkinTree.SetModel(model);
	return TRUE;
}

CMenu* CManagerTree::GetSkinMenu()
{
	return m_nSkinTree.GetSkinMenu();
}

CTreeCtrl* CManagerTree::GetSkinTreeCtrl()
{
	return &m_nSkinTree;
}

void CManagerTree::DeleteContents()
{
	m_nFrameTree.DeleteContents();
	m_nSkinTree.DeleteContents();
}

BEGIN_MESSAGE_MAP(CManagerTree, CDialogBar)
	//{{AFX_MSG_MAP(CManagerTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CManagerTree message handlers

CModel* CManagerTree::GetModel()
{
	return ((CMainFrame*)GetParent())->GetModel();
}

void CManagerTree::SetModel(CModel* model)
{
	m_nFrameTree.SetModel(model);
	m_nSkinTree.SetModel(model);
}
