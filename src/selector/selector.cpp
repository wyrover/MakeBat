// INCLUDE_PATH: "3rd\wtl"
// ADD_RESOURCE: "selector.rc"

#include <regex>

#define WINVER        0x0500
#define _WIN32_WINNT  0x0501
#define _WIN32_IE     0x0501
#define _RICHEDIT_VER 0x0200

#if     defined _M_IX86
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif   defined _M_IA64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif   defined _M_X64
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#ifndef STRICT
#define STRICT
#endif

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlmisc.h>
#include <atldlgs.h>
#include <atlconv.h>
#include <ATLWFILE.h>
#include <ATLWINMISC.h>

#include "selector_rc.h"

//
// Defs
//

#define Trace(msg) OutputDebugString(msg)

enum
{
    RESULT_OK = 0,
    RESULT_CANCEL,
    RESULT_ERROR_EMPTY_PARAM_TEMPLATE_NAME_STORE,
    RESULT_ERROR_CANT_CREATE_TEMPLATE_NAME_STORE
};

const TCHAR *RESULT_MESSAGE[] =
{
    _T(""),
    _T(""),
    _T("Set command line param for template name store file"),
    _T("Cannot create template name store file")
};

const TCHAR TEMPLATES_SUB_FOLDER[] = _T("templates\\");
const TCHAR TEMPLATE_EXT[] =         _T(".bat-template");
const TCHAR MAKEBAT_EXT[] =          _T("-make.bat");
const TCHAR INI_FILENAME[] =         _T("selector.ini");

//
// App Core
//

const TCHAR REGEX_TEMPLATE_NAME[] = _T("MakeBat-Template[ \\t]*:[ \\t]*\"([^\\t:*?\"<>|\\r\\n]+)\"[ \\t]*");

TCHAR *templateNameStoreFileName;
CAppModule _Module;

class CMainDlg :
    public CDialogImpl<CMainDlg>,
    public CDialogResize<CMainDlg>,
    public CMessageFilter
{
public:
    enum { IDD = IDD_MAINDLG };

    virtual BOOL PreTranslateMessage(MSG* pMsg)
    {
        if (pMsg->message == WM_KEYDOWN)
        {
            if (pMsg->wParam == VK_RETURN)
                ProcessSelectedTemplate();
            if (pMsg->wParam == VK_F2)
            {
                if (TryCopyStandartTemplate())
                    InitFilesList();
            }
        }
        return CWindow::IsDialogMessage(pMsg);
    }

    BEGIN_DLGRESIZE_MAP(CMainDlg)
        DLGRESIZE_CONTROL(IDC_FILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP(CMainDlg)
        CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_FILES, OnFilesListNotify)
    END_MSG_MAP()

    // Handler prototypes (uncomment arguments if needed):
    //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
    CString iniFileName;

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        pLoop->AddMessageFilter(this);
        //pLoop->AddIdleHandler(this);
        ATLASSERT(pLoop != NULL);
        DlgResize_Init();

        CModulePath path;
        iniFileName = path;
        iniFileName += INI_FILENAME;
        LoadIni();  // And set size of window
        CenterWindow();

        InitFilesList();
        return TRUE;
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        ATLASSERT(pLoop != NULL);
        pLoop->RemoveMessageFilter(this);
        return 0;
    }

    LRESULT OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (wParam == WA_INACTIVE)
            CloseDialog(RESULT_CANCEL);
        return 0;
    }

    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        CloseDialog(RESULT_CANCEL);
        return 0;
    }

    LRESULT OnFilesListNotify(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    {
        if (wNotifyCode == LBN_DBLCLK)
            ProcessSelectedTemplate();
        return 0;
    }

    void CloseDialog(int nVal)
    {
        SaveIni();
        DestroyWindow();
        ::PostQuitMessage(nVal);
    }

    void SaveIni()
    {
        CWindowPlacement pd;
        pd.GetPosData(m_hWnd);
        RECT rc = pd.rcNormalPosition;
        CIniFile ini;
        ini.SetFilename(iniFileName);
        ini.PutInt(_T("Main"), _T("Width"), rc.right - rc.left);
        ini.PutInt(_T("Main"), _T("Height"), rc.bottom - rc.top);
    }

    void LoadIni()
    {
        CIniFile ini;
        ini.SetFilename(iniFileName);
        int cx, cy;
        ini.GetInt(_T("Main"), _T("Width"), cx, 300);
        ini.GetInt(_T("Main"), _T("Height"), cy, 400);
        SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE);
    }

    // Main

    CString& TrimTemplateExt(CString &templateName)
    {
        int newSize = templateName.GetLength() - _countof(TEMPLATE_EXT) + 1;
        if (newSize >= 0)
        {
            templateName.GetBufferSetLength(newSize);
            templateName.ReleaseBuffer();
        }
        return templateName;
    }

    CString& TrimMakeBatExt(CString &makeBatFileName)
    {
        int newSize = makeBatFileName.GetLength() - _countof(MAKEBAT_EXT) + 1;
        if (newSize >= 0)
        {
            makeBatFileName.GetBufferSetLength(newSize);
            makeBatFileName.ReleaseBuffer();
        }
        return makeBatFileName;
    }

    void InitFilesList()
    {
        CListBox lb(GetDlgItem(IDC_FILES));
        lb.SetFocus();

        CString mask = _T(".\\*");
        mask += TEMPLATE_EXT;
        CFindFile finder;
        if (finder.FindFile(mask))
        {
            do
            {
                if (!(finder.IsDots() || finder.IsDirectory()))
                {
                    CString s(TrimTemplateExt(finder.GetFileName()));
                    s = _T("* ") + s;
                    if (lb.FindStringExact(0, s) == LB_ERR)
                        lb.AddString(s);
                }
            } while (finder.FindNextFile());
        }

        CModulePath path;
        mask = path + TEMPLATES_SUB_FOLDER + _T('*') + TEMPLATE_EXT;
        if (finder.FindFile(mask))
        {
            do
            {
                if (!(finder.IsDots() || finder.IsDirectory()))
                {
                    CString s(TrimTemplateExt(finder.GetFileName()));
                    // Проверяем на дубликат, так как в папке исходника может быть шаблон с именем,
                    // с которым есть и в папке шаблонов. Использоваться все равно будет тот что в
                    // папке исходников, так как не используется полный путь к шаблону а только название
                    if (lb.FindStringExact(0, s) == LB_ERR)
                        lb.AddString(s);
                }
            } while (finder.FindNextFile());
        }

        if (lb.GetCount() > 0)
        {
            bool selected;
            CString name(FindTemplateNameInMakeBatContent(ReadMakeBatContent(CString(templateNameStoreFileName))));
            if (!name.IsEmpty())
                selected = TrySelectTemplateInList(_T("* ") + name) || TrySelectTemplateInList(name);
        }
    }

    CString ReadMakeBatContent(CString &path)
    {
        if (!path.IsEmpty())
        {
            CFile file;
            if (file.Open(path))
            {
                DWORD size = file.GetSize();
                if (size > 1024)
                    size = 1024;  // TODO Magic number
                char buff[1024];

                if (file.Read(buff, size))
                {
                    #ifdef _UNICODE
                        ATL::CA2W wbuff(buff);
                        return CString(wbuff);
                    #else
                        return CString(buff);
                    #endif
                }
            }
        }
        return CString();
    }

    CString FindTemplateNameInMakeBatContent(CString &content)
    {
        if (!content.IsEmpty())
        {
            // TODO Dont use std::string and std::regex
            #ifdef _UNICODE
                std::wstring s_(content);
                std::wsmatch m;
                std::wregex r(REGEX_TEMPLATE_NAME);
            #else
                std::string s_(content);
                std::smatch m;
                std::regex r(REGEX_TEMPLATE_NAME);
            #endif
            if (std::regex_search(s_, m, r) && m.size() == 2)
            {
                s_ = m[1];
                return CString(s_.c_str());
            }
        }
        return CString();
    }

    bool TrySelectTemplateInList(CString &name)
    {
        CListBox lb(GetDlgItem(IDC_FILES));
        if (lb.GetCount() > 0)
        {
            int index = lb.FindStringExact(0, name);
            if (index != LB_ERR)
            {
                lb.SetCurSel(index);
                return true;
            }
        }
        return false;
    }

    CString GetSelected(bool *isStandart = NULL)
    {
        if (isStandart)
            *isStandart = false;
        CListBox lb(GetDlgItem(IDC_FILES));
        int index = lb.GetCurSel();
        if (index >= 0)
        {
            CString s;
            lb.GetText(index, s.GetBufferSetLength(lb.GetTextLen(index)));
            s.ReleaseBuffer();
            if (!s.IsEmpty())
            {
                bool b = s[0] == _T('*');
                if (b)
                {
                    s.SetAt(0, _T(' '));
                    s.TrimLeft();
                }
                if (isStandart)
                    *isStandart = !b;
                return s;
            }
        }
        return CString();
    }

    void ProcessSelectedTemplate()
    {
        CListBox lb(GetDlgItem(IDC_FILES));
        int index = lb.GetCurSel();
        if (index >= 0)
        {
            CString s(GetSelected());
            if (!s.IsEmpty())
            {
                s = _T("rem MakeBat-Template: \"") + s + _T("\"\n");

                CFile file;
                #ifdef _UNICODE
                    ATL::CW2A asci(s);
                    if (file.Create(templateNameStoreFileName) && file.Write(asci, strlen(asci)))
                #else
                    if (file.Create(templateNameStoreFileName) && file.Write(s, s.GetLength()))
                #endif
                {
                    CloseDialog(RESULT_OK);
                }
                else
                {
                    ::MessageBox(NULL, RESULT_MESSAGE[RESULT_ERROR_CANT_CREATE_TEMPLATE_NAME_STORE], _T("Error"), MB_ICONERROR | MB_OK);
                    CloseDialog(RESULT_ERROR_CANT_CREATE_TEMPLATE_NAME_STORE);
                }
            }
        }
    }

    CString TemplateNameToPath(CString &name, bool isStandart)
    {
        if (!name.IsEmpty())
        {
            if (isStandart)
            {
                CModulePath path;
                return CString(path + TEMPLATES_SUB_FOLDER + name + TEMPLATE_EXT);
            }
            else
                return CString(name + TEMPLATE_EXT);
        }
        return CString();
    }

    bool TryCopyStandartTemplate()
    {
        bool isStandart;
        CString stdName(GetSelected(&isStandart));
        if (!stdName.IsEmpty() && isStandart)
        {
            CString stdPath = TemplateNameToPath(stdName, true);
            if (!stdPath.IsEmpty())
            {
                CString pathTo(TrimMakeBatExt(CString(templateNameStoreFileName)) + _T(" - ") + stdName);
                pathTo = TemplateNameToPath(pathTo, false);
                return !pathTo.IsEmpty() && CopyFile(stdPath, pathTo, FALSE);  // TODO Rewrite MsgBox!
            }
        }
        return false;
    }
};

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);
    CMainDlg dlgMain;
    if (dlgMain.Create(NULL) == NULL)
        return 0;
    dlgMain.ShowWindow(nCmdShow);
    int nRet = theLoop.Run();
    _Module.RemoveMessageLoop();
    return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    LPTSTR lpstrCmdLine,
    int nCmdShow)
{
    if (__argc != 2)
    {
        ::MessageBox(NULL, RESULT_MESSAGE[RESULT_ERROR_EMPTY_PARAM_TEMPLATE_NAME_STORE], _T("Error"), MB_ICONERROR | MB_OK);
        return RESULT_ERROR_EMPTY_PARAM_TEMPLATE_NAME_STORE;
    }

#ifdef _UNICODE
    templateNameStoreFileName = __wargv[1];
#else
    templateNameStoreFileName = __argv[1];
#endif

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);
    AtlInitCommonControls(ICC_BAR_CLASSES);
    HRESULT hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));
    int nRet = Run(lpstrCmdLine, nCmdShow);
    _Module.Term();
    return nRet;
}
