#if !defined(AFX_JOINTCONSTRAINTDLG_H__4203DC62_D052_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_JOINTCONSTRAINTDLG_H__4203DC62_D052_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// JointConstraintDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CJointConstraintDlg dialog

class CJointConstraintDlg : public CDialog
{
// Construction
public:
	void SetModel(CModel* model);

	CJointConstraintDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CJointConstraintDlg)
	enum { IDD = IDD_JOINT_LOCK_ANGLES };
	float	m_nXVal;
	float	m_nYVal;
	float	m_nZVal;
	float	m_nXVal2;
	float	m_nYVal2;
	float	m_nZVal2;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJointConstraintDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CJointConstraintDlg)
		// NOTE: the ClassWizard will add member functions here
	virtual BOOL OnInitDialog();
	afx_msg void OnApplyClicked();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CModel*			m_model;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_JOINTCONSTRAINTDLG_H__4203DC62_D052_11D1_82DF_0080C82BD965__INCLUDED_)
