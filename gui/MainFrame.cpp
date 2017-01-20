
#include "precomp.h"

CAppModule Module;

void Drag(IMsiDumpCab*, int selectedCount);

int Run(
        PWSTR command_line = NULL,
        int   show_option  = SW_SHOWDEFAULT)
{
        CMessageLoop message_loop;
        ::Module.AddMessageLoop(&message_loop);

        CMainFrame main_frame(command_line);

        if (main_frame.CreateEx() == NULL)
        {
                ATLTRACE(L"Main window creation failed!\n");
                return 0;
        }

        main_frame.ShowWindow(show_option);

        int result = message_loop.Run();

        ::Module.RemoveMessageLoop();
        return result;
}

int WINAPI
wWinMain(
        HINSTANCE instance_handle,
        HINSTANCE /*hPrevInstance*/,
        PWSTR command_line,
        int   show_option)
{
        // I am using Drag and Drop, therefore I must use OleInitialize
        HRESULT hresult = OleInitialize(NULL);
        ATLASSERT(SUCCEEDED(hresult));
        if (FAILED(hresult))
                return 0;

        AtlInitCommonControls(ICC_BAR_CLASSES); // add flags to support other controls

        hresult = ::Module.Init(NULL, instance_handle);
        ATLASSERT(SUCCEEDED(hresult));
        if (FAILED(hresult))
        {
                OleUninitialize();
                return 0;
        }

        int result = Run(command_line, show_option);
        ::Module.Term();
        OleUninitialize();

        return result;
}

static PCWSTR
LoadString(
        UINT string_resource_id)
{
        const static int size = 256;
        static WCHAR     buffer[size];
        buffer[0] = 0;
        ::LoadString(::Module.GetResourceInstance(), string_resource_id, buffer, size);
        return buffer;
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
        status_bar  = m_hWndStatusBar;
        int parts[] = {200, -1};
        status_bar.SetParts(2, parts);

        // register object for message filtering and idle updates
        CMessageLoop* message_loop = ::Module.GetMessageLoop();
        ATLASSERT(message_loop != NULL);
        message_loop->AddMessageFilter(this);
        message_loop->AddIdleHandler(this);

        // register for accepting drag-and-drop files
        ::DragAcceptFiles(m_hWnd, TRUE);

        m_hWndClient = list_view.Create(m_hWnd, rcDefault,
                                        NULL, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA,
                                        WS_EX_CLIENTEDGE,
                                        IDC_LIST_VIEW);

        list_view.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
        list_view.InsertColumn(COLUME_NAME, LoadString(IDS_LISTVIEW_COLUMN_NAME), 0, 140, -1);
        list_view.InsertColumn(COLUMN_TYPE, LoadString(IDS_LISTVIEW_COLUMN_TYPE), 0, 40, -1);
        list_view.InsertColumn(COLUMN_SIZE, LoadString(IDS_LISTVIEW_COLUMN_SIZE), LVCFMT_RIGHT, 90, -1);
        list_view.InsertColumn(COLUMN_PATH, LoadString(IDS_LISTVIEW_COLUMN_PATH), 0, 400, -1);
        list_view.InsertColumn(COLUMN_PLATFORM, LoadString(IDS_LISTVIEW_COLUMN_PLATFORM), 0, 60, -1);
        list_view.InsertColumn(COLUMN_VERSION, LoadString(IDS_LISTVIEW_COLUMN_VERSION), 0, 100, -1);
        list_view.InsertColumn(COLUMN_LANGUAGE, LoadString(IDS_LISTVIEW_COLUMN_LANGUAGE), 0, 80, -1);

        id_of_column_being_sorted = -1;
        wait_cursor_handle        = LoadCursor(NULL, IDC_WAIT);
        is_wait_cursor_in_use     = false;

        delay_event      = CreateEvent(NULL, FALSE, FALSE, NULL);
        is_delay_loading = false;

        UpdateLayout();

        msi             = MsiDumpCreateObject();
        list_view_index = NULL;
        file_sizes      = NULL;

        Cleanup();

        if (command_line)
        {
                LoadMsiFiles(command_line);
        }

        return 0;
}

BOOL CMainFrame::OnIdle()
{
        BOOL is_enabled = (msi->GetFileCount() != 0);
        UIEnable(ID_EDIT_SELECT_ALL, is_enabled);
        UIEnable(IDM_EXTRACT_FILES, is_enabled);
        UIEnable(IDM_EXPORT_FILELIST, is_enabled);

        if (is_selection_changed)
        {
                is_selection_changed = false;
                UpdateStatusbar(ID_STATUSBAR_SELECTED);
        }

        if (total_file_size == 0)
        {
                int count = msi->GetFileCount();
                for (int i = 0; i < count; i++)
                {
                        MsiDumpFileDetail detail;
                        msi->GetFileDetail(i, &detail);
                        total_file_size += detail.file_size;
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
        if (is_wait_cursor_in_use)
        {
                SetCursor(wait_cursor_handle);
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
        HDROP drop_handle = (HDROP)wParam;
        int   count       = DragQueryFile(drop_handle, 0xFFFFFFFF, NULL, 0);
        if (count != 1)
                return 0;

        WCHAR file_name[MAX_PATH];
        DragQueryFile(drop_handle, 0, file_name, MAX_PATH);
        DragFinish(drop_handle);

        // check whether the file extension is ".ms?"
        //
        size_t len        = wcslen(file_name);
        PCWSTR extPartial = L".ms";
        if (len >= 5 /* eg. "a.msi" */
            && _wcsnicmp(&file_name[len - 4], extPartial, wcslen(extPartial)) == 0)
        {
                LoadMsiFiles(file_name);
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
        HMENU menu_handle = GetSubMenu(GetMenu(), 1);
        TrackPopupMenu(menu_handle, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, m_hWnd, NULL);
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
        CFileDialog dialog(TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
                           L"msi",
                           NULL,
                           OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                           LoadString(IDS_OPEN_FILE_FILTER));
        if (dialog.DoModal() == IDOK)
        {
                is_wait_cursor_in_use = true;
                LoadMsiFiles(dialog.m_szFileName);
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
        if (msi->GetFileCount() == 0)
                return 0;
        CFileDialog dlg(FALSE, // TRUE for FileOpen, FALSE for FileSaveAs
                        L"txt",
                        NULL,
                        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                        LoadString(IDS_EXPORT_FILELIST_FILTER));
        if (dlg.DoModal() != IDOK)
                return 0;

        FILE* files;
        if (_wfopen_s(&files, dlg.m_szFileName, L"wt") == 0)
                return 0;

        fwprintf(files, L"%4s %15s %9s %-45s %15s %9s\n",
                 L"num", L"filename", L"filesize", L"path", L"version", L"language");
        int count = msi->GetFileCount();
        for (int i = 0; i < count; i++)
        {
                MsiDumpFileDetail detail;
                msi->GetFileDetail(i, &detail);
                fwprintf(files, L"%4d %15s %9d %-45s %15s %9s\n", i,
                         detail.file_name, detail.file_size, detail.path, detail.version, detail.language);
        }
        fclose(files);
        return 0;
}

LRESULT CMainFrame::OnExtractFiles(
        WORD /*wNotifyCode*/,
        WORD /*wID*/,
        HWND /*hWndCtl*/,
        BOOL& /*bHandled*/
        )
{
        if (msi->GetFileCount() == 0)
                return 0;

        int             selected_count = list_view.GetSelectedCount();
        EnumSelectItems select_items;
        if (selected_count == 0 || selected_count == msi->GetFileCount())
                select_items = SELECT_ALL_ITEMS;
        else
                select_items = SELECT_INDIVIDUAL_ITEMS;

        PCWSTR        title = LoadString(IDS_INFO_SELECT_DEST_FOLDER);
        CFolderDialog dialog(m_hWnd, title, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE);
        if (dialog.DoModal() != IDOK)
                return 0;

        is_wait_cursor_in_use = true;
        msi->ExtractTo(dialog.m_szFolderPath, select_items, EXTRACT_TO_TREE);
        is_wait_cursor_in_use = false;
        return 0;
}

LRESULT
CMainFrame::OnColumnClick(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLISTVIEW notify_message = (LPNMLISTVIEW)pnmh;

        if (id_of_column_being_sorted != notify_message->iSubItem)
                is_sort_ascending = true;
        else
                is_sort_ascending = !is_sort_ascending;

        id_of_column_being_sorted = notify_message->iSubItem;

        Sort();

        list_view.SetSelectedColumn(id_of_column_being_sorted);
        return 0;
}

LRESULT
CMainFrame::OnItemChanged(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLISTVIEW notify_message = (LPNMLISTVIEW)pnmh;
        bool         old_state      = TEST_FLAG(notify_message->uOldState, LVIS_SELECTED);
        bool         new_state      = TEST_FLAG(notify_message->uNewState, LVIS_SELECTED);
        if (TEST_FLAG(notify_message->uChanged, LVIF_STATE) && (old_state != new_state))
        {
                MsiDumpFileDetail detail;
                int               item_index = notify_message->iItem;
                is_selection_changed         = true;

                // -1 means the change has been applied to all items in the list view.
                //
                if (item_index == -1)
                {
                        int count = msi->GetFileCount();
                        for (item_index = 0; item_index < count; item_index++)
                                msi->SelectFile(item_index, new_state);

                        if (new_state == false)
                                selected_file_size = 0;
                        else
                                selected_file_size = total_file_size;

                        return 0;
                }

                int index = list_view_index[item_index];
                msi->GetFileDetail(index, &detail);
                msi->SelectFile(index, new_state);
                if (new_state == false)
                {
                        // the item is de-selected
                        detail.file_size = -detail.file_size;
                }

                selected_file_size += detail.file_size;
        }
        return 0;
}

// OD = Owner Data = virtual list view.
// The notification tells the state of all items in the specified range has changed
//
LRESULT
CMainFrame::OnODStateChanged(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        LPNMLVODSTATECHANGE notify_message = (LPNMLVODSTATECHANGE)pnmh;
        bool                old_state      = TEST_FLAG(notify_message->uOldState, LVIS_SELECTED);
        bool                new_state      = TEST_FLAG(notify_message->uNewState, LVIS_SELECTED);
        if (old_state == new_state)
                return 0;

        is_selection_changed = true;
        for (int iItem = notify_message->iFrom; iItem <= notify_message->iTo; iItem++)
        {
                MsiDumpFileDetail detail;
                int               item_index = list_view_index[iItem];
                msi->GetFileDetail(item_index, &detail);
                if (detail.is_selected == new_state)
                        continue;

                msi->SelectFile(item_index, new_state);
                if (new_state == false)
                {
                        // the item is de-selected
                        detail.file_size = -detail.file_size;
                }
                selected_file_size += detail.file_size;
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
        Drag(msi, list_view.GetSelectedCount());
        return 0;
}

LRESULT
CMainFrame::OnGetDispInfo(
        int /*idCtrl*/,
        LPNMHDR pnmh,
        BOOL& /*bHandled*/
        )
{
        NMLVDISPINFO* notify_message = (NMLVDISPINFO*)pnmh;
        LPLVITEM      item           = &notify_message->item;

        if (!TEST_FLAG(item->mask, LVIF_TEXT))
        {
                return 0;
        }

        MsiDumpFileDetail detail;
        int               item_index = list_view_index[item->iItem];
        msi->GetFileDetail(item_index, &detail);

        switch (item->iSubItem)
        {

        case COLUME_NAME:
                item->pszText = (PWSTR)detail.file_name;
                break;

        case COLUMN_TYPE:
        {
                PCWSTR extension = wcsrchr(detail.file_name, L'.');
                if (extension)
                        extension++;
                else
                        extension = L"";
                item->pszText     = (PWSTR)extension;
                break;
        }

        case COLUMN_SIZE:
        {
                if (file_sizes[item_index] == NULL)
                {
                        WCHAR filesizeBuffer[20];
                        swprintf_s(filesizeBuffer, 20, L"%d", detail.file_size);
                        file_sizes[item_index] = _wcsdup(filesizeBuffer);
                }
                item->pszText = (PWSTR)file_sizes[item_index];
                break;
        }

        case COLUMN_PATH:
                item->pszText = (PWSTR)detail.path;
                break;

        case COLUMN_PLATFORM:
        {
                PCWSTR win_9x  = L"win9x";
                PCWSTR win_nt  = L"winNT";
                PCWSTR win_x64 = L"winX64";
                PCWSTR winAll  = L"";

                PCWSTR platform;
                if (detail.win_x64)
                        platform = win_x64;
                else if (detail.win_nt && detail.win_9x)
                        platform = winAll;
                else if (detail.win_nt)
                        platform = win_nt;
                else if (detail.win_9x)
                        platform = win_9x;
                else
                        platform = winAll;

                item->pszText = (PWSTR)platform;
                break;
        }

        case COLUMN_VERSION:
                item->pszText = (PWSTR)detail.version;
                break;

        case COLUMN_LANGUAGE:
                item->pszText = (PWSTR)detail.language;
                break;

        default:
                item->pszText = L"???";
                break;

        } // end switch(pItem->iSubItem)

        return 0;
}

LRESULT
CMainFrame::OnRightClick(
        int control_id,
        LPNMHDR /*pnmh*/,
        BOOL& /*bHandled*/
        )
{
        if (control_id == IDC_LIST_VIEW)
                return 0;
        return S_FALSE;
}

void __cdecl CMainFrame::ThreadWaitDelayLoad(void* parameter)
{
        CMainFrame* _this = (CMainFrame*)parameter;

        WaitForSingleObject(_this->delay_event, INFINITE);

        _this->is_delay_loading      = false;
        _this->is_wait_cursor_in_use = false;

        SetCursor(NULL);

        ::InvalidateRect(_this->m_hWndClient, NULL, TRUE);
}

void CMainFrame::LoadMsiFiles(
        PCWSTR file_name)
{
        Cleanup();

        is_delay_loading = true;

        _beginthread(ThreadWaitDelayLoad, 0, this);

        if (!msi->DelayOpen(file_name, delay_event))
                return;

        total_file_size = 0;
        int count       = msi->GetFileCount();

        list_view.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
        list_view_index = new int[count];
        file_sizes      = new PCWSTR[count];

        for (int i = 0; i < count; i++)
        {
                list_view_index[i] = i;
                file_sizes[i]      = NULL;
        }

        SetCaption(file_name);

        if (id_of_column_being_sorted != -1)
                Sort();
}

void CMainFrame::Cleanup()
{
        msi->Close();
        list_view.DeleteAllItems();

        if (list_view_index)
        {
                delete[] list_view_index;
                list_view_index = NULL;
        }

        if (file_sizes)
        {
                int count = msi->GetFileCount();
                for (int i = 0; i < count; i++)
                {
                        if (file_sizes[i])
                                free((void*)file_sizes[i]);
                }
                delete[] file_sizes;
                file_sizes = NULL;
        }

        total_file_size    = 0;
        selected_file_size = 0;
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
                           list_view.GetSelectedCount(), selected_file_size / 1024);
                break;

        case ID_STATUSBAR_TOTAL:
#pragma warning(suppress : 4774) // warning C4774: 'swprintf_s' : format string expected in argument 3 is not a string literal
                swprintf_s(buffer, MAX_PATH, LoadString(IDS_STATUSBAR_TOTAL),
                           msi->GetFileCount(), total_file_size / 1024);
                break;
        }
        status_bar.SetText(part, buffer, 0);
}

static CMainFrame* _this_qsort;

void CMainFrame::Sort()
{
        int count = msi->GetFileCount();
        if (count == 0)
                return;

        _this_qsort = this;
        qsort(list_view_index, count, sizeof(*list_view_index), SortCallback);
        list_view.DeleteAllItems();
        list_view.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
}

int __cdecl CMainFrame::SortCallback(
        const void* elem1,
        const void* elem2)
{
        CMainFrame* _this = _this_qsort;

        int i1 = *(int*)elem1;
        int i2 = *(int*)elem2;

        MsiDumpFileDetail detail1, detail2;
        _this->msi->GetFileDetail(i1, &detail1);
        _this->msi->GetFileDetail(i2, &detail2);

        int     retval                    = 0;
        Columns id_of_column_being_sorted = (Columns)_this->id_of_column_being_sorted;
        switch (id_of_column_being_sorted)
        {

        case COLUME_NAME:
        {
                retval = _wcsicmp(detail1.file_name, detail2.file_name);
                break;
        }

        case COLUMN_TYPE:
        {
                PCWSTR ext1 = wcsrchr(detail1.file_name, L'.');
                if (ext1)
                        ext1++;
                else
                        ext1 = L"";

                PCWSTR ext2 = wcsrchr(detail2.file_name, L'.');
                if (ext2)
                        ext2++;
                else
                        ext2 = L"";

                retval = _wcsicmp(ext1, ext2);
                break;
        }

        case COLUMN_SIZE:
        {
                int size1 = detail1.file_size;
                int size2 = detail2.file_size;

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

                if (detail1.win_9x)
                        plat1 |= 1;
                if (detail1.win_nt)
                        plat1 |= 2;
                if (detail1.win_x64)
                        plat1 |= 4;

                if (detail2.win_9x)
                        plat2 |= 1;
                if (detail2.win_nt)
                        plat2 |= 2;
                if (detail2.win_x64)
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

        } // end switch(id_of_column_being_sorted)

        return (_this->is_sort_ascending) ? retval : -retval;
}
