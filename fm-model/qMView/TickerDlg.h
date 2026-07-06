#if !defined(AFX_TICKERDLG_H__4514B041_C559_11D1_82DF_0080C82BD965__INCLUDED_)
#define AFX_TICKERDLG_H__4514B041_C559_11D1_82DF_0080C82BD965__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TickerDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTickerDlg dialog

class CTickerDlg : public CDialog
{
// Construction
public:
	CTickerDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTickerDlg)
	enum { IDD = IDD_TICKER };
	CSpinButtonCtrl	m_nSpinCtrl;
	BOOL	m_nUseTicker;
	long	m_nTickerDelay;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTickerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTickerDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TICKERDLG_H__4514B041_C559_11D1_82DF_0080C82BD965__INCLUDED_)
