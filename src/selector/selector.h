//
// Project: MakeBat
// Date:    2014-02-17
// Author:  Ruslan Zaporojets | ruzzzua[]gmail.com
//

#pragma once

//
// Defs
//

const TCHAR HELP_STR[] =
{
    _T("F2 - Copy selected template to current folder.\r\n\r\n")
    _T("List of params in source file:\r\n")
    _T("  #pragma comment(lib, \"_.lib\")\r\n")
    _T("  // INCLUDE_PATH: \"_\"\r\n")
    _T("  // LIB_PATH: \"_\"\r\n")
    _T("  // ADD_RESOURCE: \"_.rc\"\r\n")
    _T("  // CL_PARAMS: \"_\"\r\n")
    _T("  // LINK_PARAMS: \"_\"\r\n\r\n")
    _T("List of vars in template file:\r\n")
    _T("  {CL_PARAMS}\r\n")
    _T("  {LINK_PARAMS}\r\n")
    _T("  {INCLUDE_PATHS}\r\n")
    _T("  {LIB_PATHS}\r\n")
    _T("  {LIBS}\r\n")
    _T("  {SOURCES}\r\n")
    _T("  {RESOURCES}\r\n")
    _T("  {OUT}")
};

enum
{
    RESULT_OK = 0,
    RESULT_CANCEL,
    ERROR_UNKNOWN,
    ERROR_NOT_SPECIFIED_TEMPLATE_STORE,
    ERROR_CANT_CREATE_TEMPLATE_STORE
};

const TCHAR *RESULT_MESSAGE[] =
{
    _T(""),
    _T(""),
    _T("Internal error #1001"),
    _T("Specify path to file that stores name of template"),
    _T("Cannot create file that stores name of template")
};

const TCHAR TEMPLATES_SUB_FOLDER[] =  _T("templates\\");
const TCHAR TEMPLATE_EXT[] =          _T(".bat-template");
const TCHAR MAKEBAT_EXT[] =           _T("-make.bat");
const TCHAR INI_FILENAME[] =          _T("selector.ini");
const TCHAR REGEX_TEMPLATE_NAME[] =   _T("MakeBat-Template[ \\t]*:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*");
const TCHAR TEMPLATE_IN_STORE[] =     _T("@rem MakeBat-Template: \"%s\"\n");
const TCHAR USER_DEFINED_PREFIX[] =   _T("* ");
const TCHAR COPIED_TEMPLATE_NAME[] =  _T("New - %s");

#define ShowError(code) ::MessageBox(NULL, RESULT_MESSAGE[code], _T("Error"), MB_ICONERROR | MB_OK)

// #define _LOG
#ifdef _LOG
#define LOG(msg) ::OutputDebugString(msg)
#else
#define LOG(msg) void()
#endif

//
// CMainDlg
//

class CMainDlg :
    public CDialogImpl<CMainDlg>,
    public CDialogResize<CMainDlg>,
    public CMessageFilter
{
public:
    enum { IDD = IDD_MAINDLG };

    BEGIN_DLGRESIZE_MAP(CMainDlg)
        DLGRESIZE_CONTROL(IDC_TEMPLATES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDC_HELP_MSG, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP(CMainDlg)
        CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_TEMPLATES, OnFilesListNotify)
    END_MSG_MAP()

    // Handler prototypes (uncomment arguments if needed):
    //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    virtual BOOL PreTranslateMessage(MSG* pMsg);

private:
    CString iniFileName;
    CListBox lbTemplates;
    CEdit edHelp;
    CFont helpMsgFont;

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFilesListNotify(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    void CloseDialog(int code);
    void SaveIni();
    void LoadIni();

    // Main
    CString& TrimTemplateExt(CString &templateName);
    CString& TrimMakeBatExt(CString &makeBatFileName);
    void ReadTemplatesList(bool updateSelected = true);
    CString ReadMakeBatContent(CString &path);
    CString FindTemplateNameInMakeBatContent(CString &content);
    bool TrySelectTemplateInList(CString &name);
    CString GetSelectedFull();
    CString GetSelected(bool *isUserDefined = NULL);
    void ProcessSelectedTemplate();
    CString GetTemplatePath(const CString &name, bool isUserDefined);
    bool TryCopyStandartTemplate();
};
