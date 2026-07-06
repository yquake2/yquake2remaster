#if !defined(AFX_NODETREECTRL_H__358B2D85_C279_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_NODETREECTRL_H__358B2D85_C279_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NodeTreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodeTreeCtrl window

class CNodeTreeCtrl : public CTreeCtrl
{
// Construction
public:
	CNodeTreeCtrl();
	void SetModel(CModel* model);
	void DeleteContents();
	CModel* GetModel();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodeTreeCtrl)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNodeTreeCtrl();

	// Generated message map functions
protected:
	afx_msg void OnContextMenu(CWnd*, CPoint point);
	//{{AFX_MSG(CNodeTreeCtrl)
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNodeToggleVis();
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	HTREEITEM			m_rootNode;
	CImageList			m_image;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODETREECTRL_H__358B2D85_C279_11D1_82DF_0080C82BD965__INCLUDED_)
