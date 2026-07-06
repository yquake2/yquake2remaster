#if !defined(AFX_FRAMEMANAGER2_H__358B2D84_C279_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_FRAMEMANAGER2_H__358B2D84_C279_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FrameManager2.h : header file
//

#include "NodeTreeCtrl.h"
#include "JointTreeCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// FrameManager2 window

class FrameManager2 : public CDialogBar
{
// Construction
public:

	CNodeTreeCtrl*		m_nNodeTree;
	CJointTreeCtrl*		m_nJointTree;
	FrameManager2();
	BOOL Create(CWnd* pParentWnd, UINT nStyle);
	void DeleteContents();
	void PickNode(HTREEITEM item);
	CModel* GetModel();
	CTreeCtrl* GetNodeTreeCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FrameManager2)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~FrameManager2();
	void SetModel(CModel* model);

	// Generated message map functions
protected:
	//{{AFX_MSG(FrameManager2)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FRAMEMANAGER2_H__358B2D84_C279_11D1_82DF_0080C82BD965__INCLUDED_)
