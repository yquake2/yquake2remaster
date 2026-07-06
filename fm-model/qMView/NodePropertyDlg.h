#if !defined(AFX_NODEPROPERTYDLG_H__4BAEFDE1_C304_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_NODEPROPERTYDLG_H__4BAEFDE1_C304_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NodePropertyDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodePropertyDlg dialog

class CNodePropertyDlg : public CDialog
{
// Construction
public:
	CNodePropertyDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNodePropertyDlg)
	enum { IDD = IDD_NODE_PROP };
	CString	m_tris;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodePropertyDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNodePropertyDlg)
	afx_msg void OnApply();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODEPROPERTYDLG_H__4BAEFDE1_C304_11D1_82DF_0080C82BD965__INCLUDED_)
