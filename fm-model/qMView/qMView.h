// qMView.h : main header file for the QMVIEW application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CQMViewApp:
// See qMView.cpp for the implementation of this class
//

class CQMViewApp : public CWinApp
{
public:
	CQMViewApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQMViewApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CQMViewApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

enum
{
	QM_NEW_MODEL = 1,
	QM_DELETE_MODEL,
	QM_CHANGE_VISUAL,
	QM_ALLNODES_VISIBLE,
	QM_ADD_VISUAL,
	QM_DELETE_VISUAL,
	QM_IDLE,
	QM_STOP_ANIMATION,
	QM_RUN_ANIMATION,
	QM_NEW_SKIN,
	QM_CHANGE_SKIN,
	QM_UPDATE_MODEL,
};

#define ID_SKINVIEW		AFX_IDW_PANE_FIRST + 1
#define ID_MODELVIEW	AFX_IDW_PANE_FIRST + 2

/////////////////////////////////////////////////////////////////////////////
