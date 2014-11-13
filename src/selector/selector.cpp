//
// Project: MakeBat
// Date:    2014-02-17
// Author:  Ruslan Zaporojets | ruzzzua[]gmail.com
//

// INCLUDE_PATH: "3rd\wtl"
// ADD_RESOURCE: "selector.rc"
// CL_PARAMS: "/O1"

#include <regex>

#define WINVER        0x0500
#define _WIN32_WINNT  0x0501
#define _WIN32_IE     0x0501
#define _RICHEDIT_VER 0x0200

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
#include "selector.h"

//
// App Core
//

TCHAR *templateStoreFileName;
CAppModule _Module;

//
// CMainDlg
//

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN)
    {
        if (lbTemplates.IsWindowVisible())
        {
            if (pMsg->wParam == VK_RETURN)
                ProcessSelectedTemplate();
            else if (pMsg->wParam == VK_F2)
            {
                if (TryCopyStandartTemplate())
                {
                    CString prevSelected = GetSelectedFull();
                    ReadTemplatesList(false);
                    TrySelectTemplateInList(prevSelected);
                }
            }
        }

        if (pMsg->wParam == VK_F1)
        {
            BOOL b = edHelp.IsWindowVisible();
            edHelp.ShowWindow(b ? SW_HIDE : SW_SHOW);
            lbTemplates.ShowWindow(!b ? SW_HIDE : SW_SHOW);
        }
    }
    return CWindow::IsDialogMessage(pMsg);
};

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    pLoop->AddMessageFilter(this);
    //pLoop->AddIdleHandler(this);
    ATLASSERT(pLoop != NULL);
    DlgResize_Init();

    CModulePath path;
    iniFileName = path;
    iniFileName += INI_FILENAME;

    lbTemplates.Attach(GetDlgItem(IDC_TEMPLATES));

    helpMsgFont.CreateFont(14, 0, 0, 0, FW_REGULAR, 0, 0, 0, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_MODERN, _T("Courier New"));
    edHelp.Attach(GetDlgItem(IDC_HELP_MSG));
    edHelp.ShowWindow(SW_HIDE);
    edHelp.SetFont(helpMsgFont);
    edHelp.SetWindowText(HELP_STR);

    LoadIni();  // And set size of window
    CenterWindow();

    ReadTemplatesList();
    return TRUE;
};

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    LOG(L"OnDestroy");
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    return 0;
};

LRESULT CMainDlg::OnActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    if (wParam == WA_INACTIVE)
        CloseDialog(RESULT_CANCEL);
    return 0;
};

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    LOG(L"OnCancel");
    CloseDialog(RESULT_CANCEL);
    return 0;
};

LRESULT CMainDlg::OnFilesListNotify(WORD wNotifyCode, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (wNotifyCode == LBN_DBLCLK)
        ProcessSelectedTemplate();
    return 0;
};

void CMainDlg::CloseDialog(int code)
{
    LOG(L"CloseDialog");

    static bool close = false;  // TODO
    if (!close)
    {
        SaveIni();
        close = true;
        if (!(code == RESULT_OK) || (code == RESULT_CANCEL))
            ShowError(code);
    }

    DestroyWindow();
    ::PostQuitMessage(code);
};

void CMainDlg::SaveIni()
{
    LOG(L"SaveIni");
    CWindowPlacement pd;
    pd.GetPosData(m_hWnd);
    RECT rc = pd.rcNormalPosition;
    CIniFile ini;
    ini.SetFilename(iniFileName);
    ini.PutInt(_T("Main"), _T("Width"), rc.right - rc.left);
    ini.PutInt(_T("Main"), _T("Height"), rc.bottom - rc.top);
};

void CMainDlg::LoadIni()
{
    LOG(L"LoadIni");
    CIniFile ini;
    ini.SetFilename(iniFileName);
    int cx, cy;
    ini.GetInt(_T("Main"), _T("Width"), cx, 300);
    ini.GetInt(_T("Main"), _T("Height"), cy, 400);
    SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOMOVE);
};

CString& CMainDlg::TrimTemplateExt(CString &templateName)
{
    int newSize = templateName.GetLength() - _countof(TEMPLATE_EXT) + 1;
    if (newSize >= 0)
    {
        templateName.GetBufferSetLength(newSize);
        templateName.ReleaseBuffer();
    }
    return templateName;
};

CString& CMainDlg::TrimMakeBatExt(CString &makeBatFileName)
{
    int newSize = makeBatFileName.GetLength() - _countof(MAKEBAT_EXT) + 1;
    if (newSize >= 0)
    {
        makeBatFileName.GetBufferSetLength(newSize);
        makeBatFileName.ReleaseBuffer();
    }
    return makeBatFileName;
};

void CMainDlg::ReadTemplatesList(bool updateSelected)
{
    lbTemplates.SetFocus();

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
                s = USER_DEFINED_PREFIX + s;
                if (lbTemplates.FindStringExact(0, s) == LB_ERR)
                    lbTemplates.AddString(s);
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
                if (lbTemplates.FindStringExact(0, USER_DEFINED_PREFIX + s) == LB_ERR &&
                    lbTemplates.FindStringExact(0, s) == LB_ERR)
                {
                    lbTemplates.AddString(s);
                }
            }
        } while (finder.FindNextFile());
    }

    if (updateSelected && lbTemplates.GetCount() > 0)
    {
        CString name(FindTemplateNameInMakeBatContent(ReadMakeBatContent(CString(templateStoreFileName))));
        if (!name.IsEmpty())
            TrySelectTemplateInList(USER_DEFINED_PREFIX + name) || TrySelectTemplateInList(name);
    }
};

CString CMainDlg::ReadMakeBatContent(CString &path)
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
};

CString CMainDlg::FindTemplateNameInMakeBatContent(CString &content)
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
};

bool CMainDlg::TrySelectTemplateInList(CString &name)
{
    if (lbTemplates.GetCount() > 0)
    {
        int index = lbTemplates.FindStringExact(0, name);
        if (index != LB_ERR)
        {
            lbTemplates.SetCurSel(index);
            return true;
        }
    }
    return false;
};

CString CMainDlg::GetSelectedFull()
{
    int index = lbTemplates.GetCurSel();
    if (index >= 0)
    {
        CString s;
        lbTemplates.GetText(index, s.GetBufferSetLength(lbTemplates.GetTextLen(index)));
        s.ReleaseBuffer();
        return s;
    }
    return CString();
};

CString CMainDlg::GetSelected(bool *isUserDefined)
{
    if (isUserDefined)
        *isUserDefined = false;
    int index = lbTemplates.GetCurSel();
    if (index >= 0)
    {
        CString s;
        lbTemplates.GetText(index, s.GetBufferSetLength(lbTemplates.GetTextLen(index)));
        s.ReleaseBuffer();
        if (!s.IsEmpty())
        {
            CString prefix(s.Left(_countof(USER_DEFINED_PREFIX) - 1));
            if (prefix == USER_DEFINED_PREFIX)
            {
                if (isUserDefined)
                    *isUserDefined = true;
                return s.Mid(_countof(USER_DEFINED_PREFIX) - 1);
            }
            return s;
        }
    }
    return CString();
};

void CMainDlg::ProcessSelectedTemplate()
{
    CString s(GetSelected());
    if (!s.IsEmpty())
    {
        s.Format(TEMPLATE_IN_STORE, s);
        CFile file;
    #ifdef _UNICODE
        ATL::CW2A asci(s);
        bool ok = file.Create(templateStoreFileName) && file.Write(asci, strlen(asci));
    #else
        bool ok = file.Create(templateStoreFileName) && file.Write(s, s.GetLength());
    #endif
        CloseDialog(ok ? RESULT_OK : ERROR_CANT_CREATE_TEMPLATE_STORE);
    }
};

CString CMainDlg::GetTemplatePath(const CString &name, bool isUserDefined)
{
    if (name.IsEmpty())
        return CString();
    if (isUserDefined)
        return CString(name + TEMPLATE_EXT);
    else
    {
        CModulePath path;
        return CString(path + TEMPLATES_SUB_FOLDER + name + TEMPLATE_EXT);
    }
};

bool CMainDlg::TryCopyStandartTemplate()
{
    bool isUserDefined;
    CString name(GetSelected(&isUserDefined));
    if (!(name.IsEmpty() || isUserDefined))
    {
        CString path = GetTemplatePath(name, false);
        if (!path.IsEmpty())
        {
            CString pathTo;
            pathTo.Format(COPIED_TEMPLATE_NAME, name);
            pathTo = GetTemplatePath(pathTo, true);
            return !pathTo.IsEmpty() && CopyFile(path, pathTo, FALSE);  // TODO 'Rewrite' MsgBox!
        }
    }
    return false;
};

//
//  Main
//

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*cmdLine*/, int cmdShow)
{
    if (__argc != 2)
    {
        ShowError(ERROR_NOT_SPECIFIED_TEMPLATE_STORE);
        return ERROR_NOT_SPECIFIED_TEMPLATE_STORE;
    }

#ifdef _UNICODE
    templateStoreFileName = __wargv[1];
#else
    templateStoreFileName = __argv[1];
#endif

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);
    AtlInitCommonControls(ICC_BAR_CLASSES);
    HRESULT hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));
    CMessageLoop loop;
    _Module.AddMessageLoop(&loop);
    CMainDlg dlgMain;
    if (dlgMain.Create(NULL) == NULL)
    {
        ShowError(ERROR_UNKNOWN);
        return ERROR_UNKNOWN;
    }
    dlgMain.ShowWindow(cmdShow);
    int code = loop.Run();
    _Module.RemoveMessageLoop();
    _Module.Term();
    return code;
}
