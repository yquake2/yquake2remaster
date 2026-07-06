// FrameManager2.cpp : implementation file
//

#include "stdafx.h"
#include "DDutil.h"
#include "qMView.h"
#include "Matrix.h"
#include "Model.h"
#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "FrameManager2.h"

#include "NodeTreeCtrl.h"
#include "JointTreeCtrl.h"
#include "SkinTreeCtrl.h"
#include "FrameTreeCtrl.h"
#include "ManagerTree.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// FrameManager2

FrameManager2::FrameManager2()
{
}

FrameManager2::~FrameManager2()
{
}

void FrameManager2::DeleteContents()
{
	m_nNodeTree->DeleteContents();
	m_nJointTree->DeleteContents();
}

void FrameManager2::PickNode(HTREEITEM item)
{
	m_nNodeTree->Select(item,TVGN_CARET);
}

CTreeCtrl* FrameManager2::GetNodeTreeCtrl()
{
	return m_nNodeTree;
}

CModel* FrameManager2::GetModel()
{
	return ((CMainFrame*)GetParent())->GetModel();
}

void FrameManager2::SetModel(CModel* model)
{
	m_nNodeTree->SetModel(model);
	m_nJointTree->SetModel(model);
}

BOOL FrameManager2::Create(CWnd* pParentWnd, UINT nStyle)
{
	if (!CDialogBar::Create (pParentWnd, CG_IDD_FRAMEMANAGER2, nStyle, CG_IDD_FRAMEMANAGER2))
		return FALSE;

	m_nNodeTree = new CNodeTreeCtrl();
	m_nJointTree = new CJointTreeCtrl();
	m_nNodeTree->SubclassDlgItem(IDC_NODETREE,this);
	m_nJointTree->SubclassDlgItem(IDC_JOINTTREE,this);
	m_nNodeTree->SetModel(NULL);
	m_nJointTree->SetModel(NULL);
	return TRUE;
}

BEGIN_MESSAGE_MAP(FrameManager2, CDialogBar)
	//{{AFX_MSG_MAP(FrameManager2)
	ON_WM_RBUTTONDOWN()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// FrameManager2 message handlers

void FrameManager2::OnRButtonDown(UINT nFlags, CPoint point)
{
	CDialogBar::OnRButtonDown(nFlags, point);
}

void FrameManager2::OnDestroy()
{
	CDialogBar::OnDestroy();

	// TODO: Add your message handler code here

	if (m_nNodeTree != NULL)
	{
		m_nNodeTree->DestroyWindow();
		delete m_nNodeTree;
		m_nNodeTree = NULL;
	}

	if (m_nJointTree != NULL)
	{
		m_nJointTree->DestroyWindow();
		delete m_nJointTree;
		m_nJointTree = NULL;
	}
}
