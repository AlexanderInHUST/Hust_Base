// HustBase.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "HustBase.h"

#include "MainFrm.h"
#include "HustBaseDoc.h"
#include "HustBaseView.h"
#include "TreeList.h"

#include "IX_Manager.h"
#include "PF_Manager.h"
#include "RM_Manager.h"
#include "SYS_Manager.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp

BEGIN_MESSAGE_MAP(CHustBaseApp, CWinApp)
	//{{AFX_MSG_MAP(CHustBaseApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_CREATEDB, OnCreateDB)
	ON_COMMAND(ID_OPENDB, OnOpenDB)
	ON_COMMAND(ID_DROPDB, OnDropDb)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp construction

CHustBaseApp::CHustBaseApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CHustBaseApp object

CHustBaseApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp initialization
bool CHustBaseApp::pathvalue=false;

BOOL CHustBaseApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CHustBaseDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CHustBaseView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

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
void CHustBaseApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CHustBaseApp message handlers

void CHustBaseApp::OnCreateDB()
{
	//关联创建数据库按钮，此处应提示用户输入数据库的存储路径和名称，并调用CreateDB函数创建数据库。
	CString filename;
	CString pathname;
	CFileDialog createDialog(FALSE, NULL, _T("HustBase"), 0, NULL, NULL);
	createDialog.m_ofn.lpstrTitle = "Create Database here";

	if (createDialog.DoModal() == IDOK) {
		CString allpath = createDialog.GetPathName();
		filename = createDialog.GetFileName();
		pathname = allpath.Left(allpath.GetLength() - filename.GetLength() - 1);
		char *filename_cary = filename.GetBuffer();
		char *pathname_cary = pathname.GetBuffer();
		auto createResult = CreateDB(pathname_cary, filename_cary);
		pathname.ReleaseBuffer();
		filename.ReleaseBuffer();
		if (createResult == SUCCESS) {
			AfxMessageBox("Create successfully.");
		}
		else {
			AfxMessageBox("Fail to create database.");
		}
	}
	else {
		AfxMessageBox("Invalid path");
	}
}

void CHustBaseApp::OnOpenDB() 
{
	//关联打开数据库按钮，此处应提示用户输入数据库所在位置，并调用OpenDB函数改变当前数据库路径，并在界面左侧的控件中显示数据库中的表、列信息。
	CString filename;
	CString pathname;
	CFileDialog createDialog(TRUE, NULL, _T("HustBase"), 0, NULL, NULL);
	createDialog.m_ofn.lpstrTitle = "Open This Database";

	if (createDialog.DoModal() == IDOK) {
		CString allpath = createDialog.GetPathName();
		filename = createDialog.GetFileName();
		pathname = allpath.Left(allpath.GetLength() - filename.GetLength() - 1);
		char *filename_cary = filename.GetBuffer();
		char *pathname_cary = pathname.GetBuffer();
		auto openResult = OpenDB(pathname_cary, filename_cary);
		filename.ReleaseBuffer();
		pathname.ReleaseBuffer();
		if (openResult == SUCCESS) {
			SetCurrentDirectory(pathname_cary);
			CHustBaseDoc *pDoc;
			pDoc = CHustBaseDoc::GetDoc();
			CHustBaseApp::pathvalue = true;
			pDoc->m_pTreeView->PopulateTree();
			AfxMessageBox("Open successfully.");
		}
		else {
			AfxMessageBox("Fail to open database.");
		}
	}
	else {
		AfxMessageBox("Invalid path");
	}
}

void CHustBaseApp::OnDropDb() 
{
	//关联删除数据库按钮，此处应提示用户输入数据库所在位置，并调用DropDB函数删除数据库的内容。
	CString filename;
	CString pathname;
	CFileDialog createDialog(TRUE, NULL, _T("HustBase"), 0, NULL, NULL);
	createDialog.m_ofn.lpstrTitle = "Drop This Database";

	if (createDialog.DoModal() == IDOK) {
		CString allpath = createDialog.GetPathName();
		filename = createDialog.GetFileName();
		pathname = allpath.Left(allpath.GetLength() - filename.GetLength() - 1);
		char *filename_cary = filename.GetBuffer();
		char *pathname_cary = pathname.GetBuffer();
		SetCurrentDirectory(pathname_cary);
		auto openResult = DropDB(filename_cary);
		filename.ReleaseBuffer();
		pathname.ReleaseBuffer();
		if (openResult == SUCCESS) {
			AfxMessageBox("Drop successfully.");
		}
		else {
			AfxMessageBox("Fail to drop database.");
		}
	}
	else {
		AfxMessageBox("Invalid path");
	}
}
