#if !defined(AFX_JOINTTREECTRL_H__358B2D86_C279_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_JOINTTREECTRL_H__358B2D86_C279_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// JointTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJointTreeCtrl window

class CJointTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CJointTreeCtrl();
	void SetModel(CModel* model);
	void DeleteContents();
	CModel* GetModel();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJointTreeCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CJointTreeCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CJointTreeCtrl)
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	HTREEITEM		m_rootJoint;
	CImageList		m_image;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOINTTREECTRL_H__358B2D86_C279_11D1_82DF_0080C82BD965__INCLUDED_)
