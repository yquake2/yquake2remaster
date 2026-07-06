#if !defined(AFX_JOINTANGLESDLG_H__4203DC61_D052_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_JOINTANGLESDLG_H__4203DC61_D052_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// JointAnglesDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJointAnglesDlg dialog

class CJointAnglesDlg : public CDialog
{
// Construction
public:
	void SetModel(CModel* model);
	CJointAnglesDlg(CWnd* pParent = NULL);   // standard constructor

	void UpdateVisuals(void);

	// Dialog Data
	//{{AFX_DATA(CJointAnglesDlg)
	enum { IDD = IDD_JOINT_ANGLES };
	CEdit	m_nZCtrl;
	CEdit	m_nYCtrl;
	CEdit	m_nXCtrl;
	float	m_nXVal;
	float	m_nYVal;
	float	m_nZVal;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJointAnglesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJointAnglesDlg)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
	afx_msg void OnApplyClicked();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CModel*			m_model;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOINTANGLESDLG_H__4203DC61_D052_11D1_82DF_0080C82BD965__INCLUDED_)
