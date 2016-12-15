
#include "ui.h"

CAppModule _Module;

void Drag(IMsiDumpCab*, int selectedCount);

int Run(
        PWSTR pstrCmdLine = NULL,
        int   nCmdShow    = SW_SHOWDEFAULT)
{
        CMessageLoop theLoop;
        _Module.AddMessageLoop(&theLoop);

        CMainFrame wndMain(pstrCmdLine);

        if (wndMain.CreateEx() == NULL)
        {
                ATLTRACE(L"Main window creation failed!\n");
                return 0;
        }

        wndMain.ShowWindow(nCmdShow);

        int nRet = theLoop.Run();

        _Module.RemoveMessageLoop();
        return nRet;
}

int WINAPI
_tWinMain(
        HINSTANCE hInstance,
        HINSTANCE /*hPrevInstance*/,
        PWSTR pstrCmdLine,
        int   nCmdShow)
{
        // I am using Drag and Drop, therefore I must use OleInitialize
        HRESULT hRes = OleInitialize(NULL);
        ATLASSERT(SUCCEEDED(hRes));
        if (FAILED(hRes))
                return 0;

        // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
        // ::DefWindowProc(NULL, 0, 0, 0L); // n.b. no longer a problem with WTL 7.0

        AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

        hRes = _Module.Init(NULL, hInstance);
        ATLASSERT(SUCCEEDED(hRes));
        if (FAILED(hRes))
                return 0;

        int nRet = Run(pstrCmdLine, nCmdShow);

        _Module.Term();

        OleUninitialize();

        return nRet;
}

static PCWSTR
LoadString(
        UINT nTextID)
{
        const static int size = 256;
        static WCHAR     szText[size];
        szText[0] = 0;
        ::LoadString(_Module.GetResourceInstance(), nTextID, szText, size);
        return szText;
}

LRESULT
CMainFrame::OnCreate(
        UINT /*uMsg*/,
        WPARAM /*wParam*/,
        LPARAM /*lParam*/,
        BOOL& /*bHandled*/
        )
{
        CreateSimpleStatusBar();
        m_statusbar = m_hWndStatusBar;
        int parts[] = {200, -1};
        m_statusbar.SetParts(2, parts);

        // register object for message filtering and idle updates
        CMessageLoop* pLoop = _Module.GetMessageLoop();
        ATLASSERT(pLoop != NULL);
        pLoop->AddMessageFilter(this);
        pLoop->AddIdleHandler(this);

        // register for accepting drag-and-drop files
        ::DragAcceptFiles(m_hWnd, TRUE);

        m_hWndClient = m_list.Create(m_hWnd, rcDefault,
                                     NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA,
                                     WS_EX_CLIENTEDGE,
                                     IDC_LIST_VIEW);
        m_list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
        m_list.InsertColumn(COLUME_NAME, LoadString(IDS_LISTVIEW_COLUMN_NAME), 0, 140, -1);
        m_list.InsertColumn(COLUMN_TYPE, LoadString(IDS_LISTVIEW_COLUMN_TYPE), 0, 40, -1);
        m_list.InsertColumn(COLUMN_SIZE, LoadString(IDS_LISTVIEW_COLUMN_SIZE), LVCFMT_RIGHT, 90, -1);
        m_list.InsertColumn(COLUMN_PATH, LoadString(IDS_LISTVIEW_COLUMN_PATH), 0, 400, -1);
        m_list.InsertColumn(COLUMN_PLATFORM, LoadString(IDS_LISTVIEW_COLUMN_PLATFORM), 0, 60, -1);
        m_list.InsertColumn(COLUMN_VERSION, LoadString(IDS_LISTVIEW_COLUMN_VERSION), 0, 100, -1);
        m_list.InsertColumn(COLUMN_LANGUAGE, LoadString(IDS_LISTVIEW_COLUMN_LANGUAGE), 0, 80, -1);
        sortColumn  = -1;
        hWaitCursor = LoadCursor(NULL, IDC_WAIT);
        waitCursor  = false;

        delayEvent   = CreateEvent(NULL, FALSE, FALSE, NULL);
        delayLoading = false;

        UpdateLayout();

        m_msi     = MsiDumpCreateObject();
        LVindex   = NULL;
        filesizes = NULL;
        Cleanup();

        if (CmdLine && *CmdLine)
        {
                PWSTR* szArglist;
                int    nArgs;
                szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
                if (szArglist)
                {
                        LoadMsiFiles(szArglist[1]);
                        LocalFree(szArglist);
                }
        }

        return 0;
}

BOOL CMainFrame::OnIdle()
{
        BOOL enable = (m_msi->getCount() != 0);
        UIEnable(ID_EDIT_SELECT_ALL, enable);
        UIEnable(IDM_EXTRACT_FILES, enable);
        UIEnable(IDM_EXPORT_FILELIST, enable);

        if (selectionChanged)
        {
                selectionChanged = false;
                UpdateStatusbar(ID_STATUSBAR_SELECTED);
        }

        if (totalFileSize == 0)
        {
                int count = m_msi->getCount();
                for (int i = 0; i < count; i++)
                {
                        MsiDumpFileDetail detail;
                        m_msi->GetFileDetail(i, &detail);
                        totalFileSize += detail.filesize;
                }
                UpdateStatusbar(ID_STATUSBAR_TOTAL);
        }

        return FALSE;
}

LRESULT
CMainFrame::OnSetCursor(
        UINT /*uMsg*/,
        WPARAM /*wParam*/,
        LPARAM /*lParam*/,
        BOOL& /*bHandled*/
        )
{
        if (waitCursor)
        {
                SetCursor(hWaitCursor);
                return TRUE;
        }
        else
                return FALSE;
}

LRESULT
CMainFrame::OnDropFiles(
        UINT /*uMsg*/,
        WPARAM wParam,
        LPARAM /*lParam*/,
        BOOL& /*bHandled*/
        )
{
        HDROP hDrop = (HDROP)wParam;
        int   count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
        if (count != 1)
                return 0;

        WCHAR filename[MAX_PATH];
        DragQueryFile(hDrop, 0, filename, MAX_PATH);
        DragFinish(hDrop);
        size_t len        = wcslen(filename);
        PCWSTR extPartial = L".ms";
        if (len >= 5 /* eg. "a.msi" */
            && _wcsnicmp(&filename[len - 4], extPartial, wcslen(extPartial)) == 0)
        {
                LoadMsiFiles(filename);
        }

        return 0;
}

LRESULT
CMainFrame::OnContextMenu(
        UINT /*uMsg*/,
        WPARAM /*wParam*/,
        LPARAM lParam,
        BOOL& /*bHandled*/
        )
{
        HMENU hMenu = GetSubMenu(GetMenu(), 1);
        TrackPopupMenu(hMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, m_hWnd, NULL);
        return 0;
}

LRESULT
CMainFrame::OnFileOpen(
        WORD /*wNotifyCode*/,
        WORD /*wID*/,
        HWND /*hWndCtl*/,
        BOOL& /*bHandled*/
        )
{
        CFileDialog dlg(TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
                        L"msi",
                        NULL,
                        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                        LoadString(IDS_OPEN_FILE_FILTER));
        if (dlg.DoModal() == IDOK)
        {
                waitCursor = true;
                LoadMsiFiles(dlg.m_szFileName);
        }
        return 0;
}

LRESULT
CMainFrame::OnExportFileList(
        WORD /*wNotifyCode*/,
        WORD /*wID*/,
        HWND /*hWndCtl*/,
        BOOL& /*bHandled*/
        )
{
        if (m_msi->getCount() == 0)
                return 0;
        CFileDialog dlg(FALSE, // TRUE for FileOpen, FALSE for FileSaveAs
                        L"txt",
                        NULL,
                        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                        LoadString(IDS_EXPORT_FILELIST_FILTER));
        if (dlg.DoModal() != IDOK)
                return 0;

        FILE* f;
        if (_wfopen_s(&f, dlg.m_szFileName, L"wt") == 0)
                return 0;

        fwprintf(f, L"%4s %15s %9s %-45s %15s %9s\n",
                 L"num", L"filename", L"filesize", L"path", L"version", L"language");
        int count = m_msi->getCount();
        for (int i = 0; i < count; i++)
        {
                MsiDumpFileDetail detail;
                m_msi->GetFileDetail(i, &detail);
                fwprintf(f, L"%4d %15s %9d %-45s %15s %9s\n", i,
                         detail.filename, detail.filesize, detail.path, detail.version, detail.language);
        }
        fclose(f);
        return 0;
}

LRESULT CMainFrame::OnExtractFiles(
        WORD /*wNotifyCode*/,
        WORD /*wID*/,
        HWND /*hWndCtl*/,
        BOOL& /*bHandled*/
        )
{
        if (m_msi->getCount() == 0)
                return 0;

        int           selectedCount = m_list.GetSelectedCount();
        enumSelectAll selectAll;
        if (selectedCount == 0 || selectedCount == m_msi->getCount())
                selectAll = ALL_SELECTED;
        else
                selectAll = INDIVIDUAL_SELECTED;

        PCWSTR        title = LoadString(IDS_INFO_SELECT_DEST_FOLDER);
        CFolderDialog dlg(m_hWnd, title, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE);
        if (dlg.DoModal() != IDOK)
                return 0;

        waitCursor = true;
        m_msi->ExtractTo(dlg.m_szFolderPath, selectAll, EXTRACT_TO_TREE);
        waitCursor = false;
        return 0;
}

LRESULT
CMainFrame::OnColumnClick(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
        sortAscending     = (sortColumn != pnmv->iSubItem) ? true : (!sortAscending);
        sortColumn        = pnmv->iSubItem;
        sort();
        m_list.SetSelectedColumn(sortColumn);
        return 0;
}

#define TEST_FLAG(field, flag) \
        ((field & flag) == flag)

LRESULT
CMainFrame::OnItemChanged(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLISTVIEW pnmv      = (LPNMLISTVIEW)pnmh;
        bool         bOldState = TEST_FLAG(pnmv->uOldState, LVIS_SELECTED);
        bool         bNewState = TEST_FLAG(pnmv->uNewState, LVIS_SELECTED);
        if (TEST_FLAG(pnmv->uChanged, LVIF_STATE) && (bOldState != bNewState))
        {
                MsiDumpFileDetail detail;
                int               iItem = pnmv->iItem;
                selectionChanged        = true;

                // If the iItem member of the structure pointed to by pnmv is -1,
                //the change has been applied to all items in the list view.
                //
                if (iItem == -1)
                {
                        int count = m_msi->getCount();
                        for (iItem = 0; iItem < count; iItem++)
                                m_msi->setSelected(iItem, bNewState);

                        selectedFileSize =
                                (bNewState == false)
                                        ? 0
                                        : totalFileSize;

                        return 0;
                }

                int index = LVindex[iItem];
                m_msi->GetFileDetail(index, &detail);
                m_msi->setSelected(index, bNewState);
                if (bNewState == false)
                {
                        // the item is de-selected
                        detail.filesize = -detail.filesize;
                }

                selectedFileSize += detail.filesize;
        }
        return 0;
}

LRESULT
CMainFrame::OnODStateChanged(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLVODSTATECHANGE pnmv      = (LPNMLVODSTATECHANGE)pnmh;
        bool                bOldState = TEST_FLAG(pnmv->uOldState, LVIS_SELECTED);
        bool                bNewState = TEST_FLAG(pnmv->uNewState, LVIS_SELECTED);
        if (bOldState == bNewState)
                return 0;

        selectionChanged = true;
        for (int iItem = pnmv->iFrom; iItem <= pnmv->iTo; iItem++)
        {
                MsiDumpFileDetail detail;
                int               index = LVindex[iItem];
                m_msi->GetFileDetail(index, &detail);
                if (detail.selected == bNewState)
                        continue;

                m_msi->setSelected(index, bNewState);
                if (bNewState == false)
                {
                        // the item is de-selected
                        detail.filesize = -detail.filesize;
                }
                selectedFileSize += detail.filesize;
        }
        return 0;
}

LRESULT
CMainFrame::OnBeginDrag(
        int /*idCtrl*/,
        LPNMHDR /*pnmh*/,
        BOOL& /*bHandled*/
        )
{
        Drag(m_msi, m_list.GetSelectedCount());
        return 0;
}

LRESULT
CMainFrame::OnGetDispInfo(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pnmh;
        LPLVITEM      pItem     = &pDispInfo->item;

        if (!TEST_FLAG(pItem->mask, LVIF_TEXT))
        {
                return 0;
        }

        MsiDumpFileDetail detail;
        int               index = LVindex[pItem->iItem];
        m_msi->GetFileDetail(index, &detail);

        switch (pItem->iSubItem)
        {

        case COLUME_NAME:
                pItem->pszText = (PWSTR)detail.filename;
                break;

        case COLUMN_TYPE:
        {
                PCWSTR extension = wcsrchr(detail.filename, L'.');
                if (extension)
                        extension++;
                else
                        extension = L"";
                pItem->pszText    = (PWSTR)extension;
                break;
        }

        case COLUMN_SIZE:
        {
                if (filesizes[index] == NULL)
                {
                        WCHAR filesizeBuffer[20];
                        swprintf_s(filesizeBuffer, 20, L"%d", detail.filesize);
                        filesizes[index] = _wcsdup(filesizeBuffer);
                }
                pItem->pszText = (PWSTR)filesizes[index];
                break;
        }

        case COLUMN_PATH:
                pItem->pszText = (PWSTR)detail.path;
                break;

        case COLUMN_PLATFORM:
        {
                PCWSTR win9x  = L"Win9x";
                PCWSTR winNT  = L"WinNT";
                PCWSTR winX64 = L"WinX64";
                PCWSTR winAll = L"";

                PCWSTR platform;
                if (detail.winX64)
                        platform = winX64;
                else if (detail.winNT && detail.win9x)
                        platform = winAll;
                else if (detail.winNT)
                        platform = winNT;
                else if (detail.win9x)
                        platform = win9x;
                else
                        platform = winAll;

                pItem->pszText = (PWSTR)platform;
                break;
        }

        case COLUMN_VERSION:
                pItem->pszText = (PWSTR)detail.version;
                break;

        case COLUMN_LANGUAGE:
                pItem->pszText = (PWSTR)detail.language;
                break;

        default:
                pItem->pszText = L"???";
                break;

        } // end switch(pItem->iSubItem)

        return 0;
}

LRESULT
CMainFrame::OnRightClick(
        int idCtrl,
        LPNMHDR /*pnmh*/,
        BOOL& /*bHandled*/
        )
{
        if (idCtrl == IDC_LIST_VIEW)
                return 0;
        return S_FALSE;
}

extern "C" void __cdecl threadWaitDelayLoad(void* parameter)
{
        CMainFrame* _this = (CMainFrame*)parameter;
        WaitForSingleObject(_this->delayEvent, INFINITE);
        _this->delayLoading = false;
        _this->waitCursor   = false;
        SetCursor(NULL);
        InvalidateRect(_this->m_hWndClient, NULL, TRUE);
}

void CMainFrame::LoadMsiFiles(
        PCWSTR filename)
{
        Cleanup();
        delayLoading = true;
        _beginthread(threadWaitDelayLoad, 0, this);
        if (!m_msi->DelayedOpen(filename, delayEvent))
                return;

        totalFileSize = 0;
        int count     = m_msi->getCount();
        m_list.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
        LVindex = new int[count];
        int i;
        for (i             = 0; i < count; i++)
                LVindex[i] = i;
        filesizes          = new PCWSTR[count];
        for (i               = 0; i < count; i++)
                filesizes[i] = NULL;

        SetCaption(filename);
        if (sortColumn != -1)
                sort();
}

void CMainFrame::Cleanup()
{
        m_msi->Close();
        m_list.DeleteAllItems();

        if (LVindex)
        {
                delete[] LVindex;
                LVindex = NULL;
        }

        if (filesizes)
        {
                int count = m_msi->getCount();
                for (int i = 0; i < count; i++)
                {
                        if (filesizes[i])
                                free((void*)filesizes[i]);
                }
                delete[] filesizes;
                filesizes = NULL;
        }

        totalFileSize    = 0;
        selectedFileSize = 0;
        SetCaption(NULL);
}

void CMainFrame::SetCaption(
        PCWSTR caption)
{
        PCWSTR title = LoadString(IDR_MAINFRAME);

        WCHAR buffer[MAX_PATH];
        if (caption)
        {
                swprintf_s(buffer, MAX_PATH, L"%s - %s", title, PathFindFileName(caption));
                SetWindowText(buffer);
        }
        else
                SetWindowText(title);

        UpdateStatusbar(ID_STATUSBAR_SELECTED);
        UpdateStatusbar(ID_STATUSBAR_TOTAL);
}

void CMainFrame::UpdateStatusbar(int part)
{
        WCHAR buffer[MAX_PATH];
        switch (part)
        {
        case ID_STATUSBAR_SELECTED:
#pragma warning(suppress : 4774) // warning C4774: 'swprintf_s' : format string expected in argument 3 is not a string literal
                swprintf_s(buffer, MAX_PATH, LoadString(IDS_STATUSBAR_SELECTED),
                           m_list.GetSelectedCount(), selectedFileSize / 1024);
                break;

        case ID_STATUSBAR_TOTAL:
#pragma warning(suppress : 4774) // warning C4774: 'swprintf_s' : format string expected in argument 3 is not a string literal
                swprintf_s(buffer, MAX_PATH, LoadString(IDS_STATUSBAR_TOTAL),
                           m_msi->getCount(), totalFileSize / 1024);
                break;
        }
        m_statusbar.SetText(part, buffer, 0);
}

static CMainFrame* _this_qsort;

void CMainFrame::sort()
{
        int count = m_msi->getCount();
        if (count == 0)
                return;

        _this_qsort = this;
        qsort(LVindex, count, sizeof(*LVindex), sortCallback);
        m_list.DeleteAllItems();
        m_list.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
}

int __cdecl CMainFrame::sortCallback(
        const void* elem1,
        const void* elem2)
{
        CMainFrame* _this = _this_qsort;

        int i1 = *(int*)elem1;
        int i2 = *(int*)elem2;

        MsiDumpFileDetail detail1, detail2;
        _this->m_msi->GetFileDetail(i1, &detail1);
        _this->m_msi->GetFileDetail(i2, &detail2);

        int     retval     = 0;
        Columns sortColumn = (Columns)_this->sortColumn;
        switch (sortColumn)
        {

        case COLUME_NAME:
        {
                retval = _wcsicmp(detail1.filename, detail2.filename);
                break;
        }

        case COLUMN_TYPE:
        {
                PCWSTR ext1 = wcsrchr(detail1.filename, L'.');
                if (ext1)
                        ext1++;
                else
                        ext1 = L"";

                PCWSTR ext2 = wcsrchr(detail2.filename, L'.');
                if (ext2)
                        ext2++;
                else
                        ext2 = L"";

                retval = _wcsicmp(ext1, ext2);
                break;
        }

        case COLUMN_SIZE:
        {
                int size1 = detail1.filesize;
                int size2 = detail2.filesize;

                if (size1 < size2)
                        retval = -1;
                else if (size1 == size2)
                        retval = 0;
                else
                        retval = 1;
                break;
        }

        case COLUMN_PATH:
        {
                if (detail1.path == NULL || detail2.path == 0)
                {
                        retval = 0;
                        break;
                }
                retval = _wcsicmp(detail1.path, detail2.path);
                break;
        }

        case COLUMN_PLATFORM:
        {
                int plat1 = 0, plat2 = 0;

                if (detail1.win9x)
                        plat1 |= 1;
                if (detail1.winNT)
                        plat1 |= 2;
                if (detail1.winX64)
                        plat1 |= 4;

                if (detail2.win9x)
                        plat2 |= 1;
                if (detail2.winNT)
                        plat2 |= 2;
                if (detail2.winX64)
                        plat2 |= 4;

                if (plat1 < plat2)
                        retval = -1;
                else if (plat1 == plat2)
                        retval = 0;
                else
                        retval = 1;

                break;
        }

        case COLUMN_VERSION:
        {
                retval = _wcsicmp(detail1.version, detail2.version);
                break;
        }

        case COLUMN_LANGUAGE:
        {
                retval = _wcsicmp(detail1.language, detail2.language);
                break;
        }

        default:
        {
                retval = 0;
                break;
        }

        } // end switch(sortColumn)

        return (_this->sortAscending) ? retval : -retval;
}
