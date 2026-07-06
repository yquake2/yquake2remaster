// qMView.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "DDUtil.h"
#include "Matrix.h"

#include "Model.h"
#include "qMView.h"

#include "treectrlex.h"
#include "frametreectrl.h"
#include "skintreectrl.h"

#include "JointConstraintDlg.h"
#include "JointAnglesDlg.h"
#include "ManagerTree.h"
#include "FrameManager2.h"
#include "MainFrm.h"
#include "qMViewDoc.h"
#include "SkinPageFrm.h"
#include "qMViewView.h"

#include "Splash.h"
#include "pal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQMViewApp

BEGIN_MESSAGE_MAP(CQMViewApp, CWinApp)
	//{{AFX_MSG_MAP(CQMViewApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQMViewApp construction

CQMViewApp::CQMViewApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CQMViewApp object

CQMViewApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CQMViewApp initialization

BOOL CQMViewApp::InitInstance()
{
//	AfxEnableControlContainer();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	LoadStdProfileSettings(16);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_QMVIEWTYPE,
		RUNTIME_CLASS(CQMViewDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CQMViewView));
	AddDocTemplate(pDocTemplate);

/*	pDocTemplate = new CSingleDocTemplate(
		IDR_QUAKE,
		RUNTIME_CLASS(CQMViewDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CQMViewView));
	AddDocTemplate(pDocTemplate);

	pDocTemplate = new CSingleDocTemplate(
		IDR_QUAKE2,
		RUNTIME_CLASS(CQMViewDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CQMViewView));
	AddDocTemplate(pDocTemplate);
*/

	HRESULT ddrval = DirectDrawCreate(NULL, &CDDHelper::lpDD, NULL );
    if( ddrval != DD_OK )
    {
        return FALSE;
    }

	EnableShellOpen();
	RegisterShellFileTypes(TRUE);
	CSkinPageFrm::Register();

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileNew)
	{
		CCommandLineInfo newcmdInfo;

		if (!ProcessShellCommand(newcmdInfo))
		{
			return FALSE;
		}
	}

	if (!ProcessShellCommand(cmdInfo))
	{
		return FALSE;
	}

	m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
	m_pMainWnd->UpdateWindow();

	m_pMainWnd->DragAcceptFiles();

	if (cmdInfo.m_nShellCommand != CCommandLineInfo::FileOpen)
	{
		CSplashWnd* splash = new CSplashWnd(m_pMainWnd);
		splash->CenterWindow(m_pMainWnd);
		splash->ShowWindow(SW_SHOW);
		splash->UpdateWindow();
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CQMViewApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CQMViewApp commands
