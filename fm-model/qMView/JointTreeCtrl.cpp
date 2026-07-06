// JointTreeCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DDUtil.h"
#include "qMView.h"

#include "Matrix.h"
#include "Model.h"
#include "JointTreeCtrl.h"
#include "NodeTreeCtrl.h"
#include "FrameManager2.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CJointTreeCtrl

CJointTreeCtrl::CJointTreeCtrl()
{
	m_rootJoint = NULL;
}

CJointTreeCtrl::~CJointTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CJointTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CJointTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJointTreeCtrl message handlers

void CJointTreeCtrl::DeleteContents()
{
	DeleteAllItems();
	m_rootJoint = InsertItem("Joints", TVI_ROOT, TVI_FIRST);
	SetItemImage(m_rootJoint, 0, 0);
}

CModel* CJointTreeCtrl::GetModel()
{
	return ((FrameManager2*)GetParent())->GetModel();
}

void CJointTreeCtrl::SetModel(CModel* model)
{
	DeleteContents();
	if (model != NULL)
	{
		model->AddJoints(this, m_rootJoint);
	}

}

void CJointTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM	item, parent;

	CModel* model = GetModel();
	if (model == NULL)
	{
		return;
	}
	if (!model->GetMesh())
		return;

	item = GetSelectedItem();
	parent = GetParentItem(item);

	if (parent == TVI_ROOT)
	{
		model->SetCurJoint(-1);
		*pResult = 0;
		return;
	}

	model->SetCurJoint(GetItemData(item));

	*pResult = 0;
}

void CJointTreeCtrl::PreSubclassWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	CTreeCtrl::PreSubclassWindow();
	m_image.Create(IDB_BITMAP4, 13, 1, RGB(255,255,255));
    SetImageList(&(m_image), TVSIL_NORMAL);
}
