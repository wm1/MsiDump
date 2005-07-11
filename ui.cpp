
#include "ui.h"

CAppModule _Module;

void Drag(IMsiDumpCab*, int selectedCount);

class CWaitCursor
{
private:
	HCURSOR hCursorRestore;

public:
	CWaitCursor() {
		hCursorRestore = SetCursor(LoadCursor(NULL, IDC_WAIT));
	}
	~CWaitCursor() {
		SetCursor(hCursorRestore);
	}
};

int
Run(
	LPTSTR lpstrCmdLine = NULL,
	int    nCmdShow     = SW_SHOWDEFAULT
	)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain(lpstrCmdLine);

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
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
	LPTSTR    lpstrCmdLine,
	int       nCmdShow
	)
{
	//HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to
// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	
	// I am using Drag and Drop, therefore I must use OleInitialize
	HRESULT hRes = OleInitialize(NULL);
	
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	//::CoUninitialize();
	OleUninitialize();

	return nRet;
}

static LPCTSTR
LoadString(
	UINT nTextID
	)
{
	const static int size = 256;
	static TCHAR szText[size];
	szText[0] = 0;
	::LoadString(_Module.GetResourceInstance(), nTextID, szText, size);
	return szText;
}

LRESULT
CMainFrame::OnCreate(
	UINT   /*uMsg*/,
	WPARAM /*wParam*/,
	LPARAM /*lParam*/,
	BOOL&  /*bHandled*/
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
	sortColumn = -1;

	UpdateLayout();

	m_msi = MsiDumpCreateObject();
	Cleanup();

	if(CmdLine && *CmdLine)
	{
		LPWSTR *szArglist;
		int nArgs;
		szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
		if(szArglist)
		{
			LoadMsiFiles(szArglist[1]);
			LocalFree(szArglist);
		}
	}

	return 0;
}

BOOL
CMainFrame::OnIdle()
{
	BOOL enable = (m_msi->getCount() != 0);
	UIEnable(ID_EDIT_SELECT_ALL,  enable);
	UIEnable(IDM_EXTRACT_FILES,   enable);
	UIEnable(IDM_EXPORT_FILELIST, enable);

	if(selectionChanged)
	{
		selectionChanged = false;
		UpdateStatusbar(ID_STATUSBAR_SELECTED);
	}
	
	if(totalFileSize == 0)
	{
		int count = m_msi->getCount();
		for(int i = 0; i < count; i++)
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
CMainFrame::OnDropFiles(
	UINT   /*uMsg*/,
	WPARAM wParam,
	LPARAM /*lParam*/,
	BOOL&  /*bHandled*/
	)
{
	HDROP hDrop = (HDROP)wParam;
	int   count = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	if(count != 1) return 0;

	TCHAR filename[MAX_PATH];
	int r = DragQueryFile(hDrop, 0, filename, MAX_PATH);
	DragFinish(hDrop);
	size_t len = _tcslen(filename);
	if(len >= 5  /* eg. "a.msi" */
		&& _tcsicmp(&filename[len-4], TEXT(".msi")) == 0)
	{
		LoadMsiFiles(filename);
	}

	return 0;
}

LRESULT
CMainFrame::OnContextMenu(
	UINT   /*uMsg*/,
	WPARAM /*wParam*/,
	LPARAM lParam,
	BOOL&  /*bHandled*/
	)
{
	HMENU hMenu = GetSubMenu(GetMenu(), 1);
	TrackPopupMenu(hMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0, m_hWnd, NULL);
	return 0;
}

LRESULT
CMainFrame::OnFileOpen(
	WORD  /*wNotifyCode*/,
	WORD  /*wID*/,
	HWND  /*hWndCtl*/,
	BOOL& /*bHandled*/
	)
{
	CFileDialog dlg(TRUE, // TRUE for FileOpen, FALSE for FileSaveAs
		TEXT("msi"),
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LoadString(IDS_OPEN_FILE_FILTER)
		);
	if(dlg.DoModal() == IDOK)
	{
		CWaitCursor wait;
		LoadMsiFiles(dlg.m_szFileName);
	}
	return 0;
}

LRESULT
CMainFrame::OnExportFileList(
	WORD  /*wNotifyCode*/,
	WORD  /*wID*/,
	HWND  /*hWndCtl*/,
	BOOL& /*bHandled*/
	)
{
	if(m_msi->getCount() == 0) return 0;
	CFileDialog dlg(FALSE, // TRUE for FileOpen, FALSE for FileSaveAs
		TEXT("txt"),
		NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LoadString(IDS_EXPORT_FILELIST_FILTER)
		);
	if(dlg.DoModal() != IDOK) return 0;

	FILE* f = _tfopen(dlg.m_szFileName, TEXT("wt"));
	if(!f) return 0;

	_ftprintf(f, TEXT(" Num %15s %9s %s\n"), TEXT("filename"), TEXT("filesize"), TEXT("path"));
	int count = m_msi->getCount();
	for(int i = 0; i < count; i++)
	{
		MsiDumpFileDetail detail;
		m_msi->GetFileDetail(i, &detail);
		_ftprintf(f, TEXT("%4d %15s %9d %s\n"), i, 
			detail.filename, detail.filesize, detail.path);
	}
	fclose(f);
	return 0;
}

LRESULT CMainFrame::OnExtractFiles(
	WORD  /*wNotifyCode*/,
	WORD  wID,
	HWND  /*hWndCtl*/,
	BOOL& /*bHandled*/
	)
{
	if(m_msi->getCount() == 0)
		return 0;

	bool selectAll;
	int selectedCount = m_list.GetSelectedCount();
	selectAll = (selectedCount == 0 || selectedCount == m_msi->getCount());

	LPCTSTR title = LoadString(IDS_INFO_SELECT_DEST_FOLDER);
	CFolderDialog dlg(m_hWnd, title, BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE);
	if(dlg.DoModal() != IDOK) return 0;

	CWaitCursor wait;
	m_msi->ExtractTo(dlg.m_szFolderPath, selectAll, false);
	return 0;
}

LRESULT
CMainFrame::OnColumnClick(
	int     /*idCtrl*/,
	LPNMHDR pnmh,
	BOOL&   /*bHandled*/
	)
{
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
	sortAscending = (sortColumn != pnmv->iSubItem) ? true : (!sortAscending);
	sortColumn = pnmv->iSubItem;
	sort();
	m_list.SetSelectedColumn(sortColumn);
	return 0;
}

#define TEST_FLAG(field, flag) \
	((field & flag) == flag)

LRESULT
CMainFrame::OnItemChanged(
	int     /*idCtrl*/,
	LPNMHDR pnmh,
	BOOL&   /*bHandled*/
	)
{
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW)pnmh;
	bool bOldState = TEST_FLAG(pnmv->uOldState, LVIS_SELECTED);
	bool bNewState = TEST_FLAG(pnmv->uNewState, LVIS_SELECTED);
	if(TEST_FLAG(pnmv->uChanged, LVIF_STATE)
		&& (bOldState != bNewState))
	{
		MsiDumpFileDetail detail;
		int iItem = pnmv->iItem;
		selectionChanged = true;

		// If the iItem member of the structure pointed to by pnmv is -1, 
		//the change has been applied to all items in the list view. 
		//
		if(iItem == -1)
		{
			int count = m_msi->getCount();
			for(iItem=0; iItem<count; iItem++)
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
		if(bNewState == false)
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
	int     /*idCtrl*/,
	LPNMHDR pnmh,
	BOOL&   /*bHandled*/
	)
{
	LPNMLVODSTATECHANGE pnmv = (LPNMLVODSTATECHANGE)pnmh;
	bool bOldState = TEST_FLAG(pnmv->uOldState, LVIS_SELECTED);
	bool bNewState = TEST_FLAG(pnmv->uNewState, LVIS_SELECTED);
	if(bOldState == bNewState)
		return 0;

	selectionChanged = true;
	for(int iItem=pnmv->iFrom; iItem<=pnmv->iTo; iItem++)
	{
		MsiDumpFileDetail detail;
		int index = LVindex[iItem];
		m_msi->GetFileDetail(index, &detail);
		if(detail.selected == bNewState)
			continue;

		m_msi->setSelected(index, bNewState);
		if(bNewState == false)
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
	int     /*idCtrl*/,
	LPNMHDR /*pnmh*/,
	BOOL&   /*bHandled*/
	)
{
	Drag(m_msi, m_list.GetSelectedCount());
	return 0;
}

LRESULT
CMainFrame::OnGetDispInfo(
	int     /*idCtrl*/,
	LPNMHDR pnmh,
	BOOL&   /*bHandled*/
	)
{
	NMLVDISPINFO *pDispInfo = (NMLVDISPINFO*)pnmh;
	LPLVITEM      pItem     = &pDispInfo->item;
	
	if(!TEST_FLAG(pItem->mask, LVIF_TEXT))
	{
		return 0;
	}

	MsiDumpFileDetail detail;
	int index = LVindex[pItem->iItem];
	m_msi->GetFileDetail(index, &detail);
	
	switch(pItem->iSubItem)
	{

	case COLUME_NAME:
		pItem->pszText = (LPTSTR)detail.filename;
		break;

	case COLUMN_TYPE:
	{
		LPCTSTR extension = _tcsrchr(detail.filename, TEXT('.'));
		if(extension)
			extension++;
		else
			extension = TEXT("");
		pItem->pszText = (LPTSTR)extension;
		break;
	}

	case COLUMN_SIZE:
	{
		if(filesizes[index] == NULL)
		{
			TCHAR filesizeBuffer[20];
			_stprintf(filesizeBuffer, TEXT("%d"), detail.filesize);
			filesizes[index] = _tcsdup(filesizeBuffer);
		}
		pItem->pszText = (LPTSTR)filesizes[index];
		break;
	}

	case COLUMN_PATH:
		pItem->pszText = (LPTSTR)detail.path;
		break;

	case COLUMN_PLATFORM:
	{
		LPCTSTR win9x   = TEXT("Win9x");
		LPCTSTR winNT   = TEXT("WinNT");
		LPCTSTR Win9xNT = TEXT("");
		LPCTSTR platform = (detail.win9x
				? (detail.winNT ? Win9xNT : win9x)
				: (detail.winNT ? winNT : Win9xNT)
				);
		pItem->pszText = (LPTSTR)platform;
		break;
	}

	default:
		pItem->pszText = TEXT("???");
		break;

	} // end switch(pItem->iSubItem)

	return 0;
}

LRESULT
CMainFrame::OnRightClick(
	int     idCtrl,
	LPNMHDR /*pnmh*/,
	BOOL&   /*bHandled*/
	)
{
	if(idCtrl == IDC_LIST_VIEW)
		return 0;
	return S_FALSE;
}

void
CMainFrame::LoadMsiFiles(
	LPCTSTR filename
	)
{
	Cleanup();
	if(!m_msi->Open(filename))
		return;

	totalFileSize = 0;
	int count = m_msi->getCount();
	m_list.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
	LVindex = new int[count];
	for(int i=0; i<count; i++)
		LVindex[i] = i;
	filesizes = new LPCTSTR[count];
	for(i=0; i<count; i++)
		filesizes[i] = NULL;

	SetCaption(filename);
	if(sortColumn != -1)
		sort();
}

void
CMainFrame::Cleanup()
{
	m_msi->Close();
	m_list.DeleteAllItems();

	if(LVindex)
	{
		delete []LVindex;
		LVindex = NULL;
	}

	if(filesizes)
	{
		int count = m_msi->getCount();
		for(int i=0; i<count; i++)
		{
			if(filesizes[i])
				free((void*)filesizes[i]);
		}
		delete []filesizes;
		filesizes = NULL;
	}

	totalFileSize    = 0;
	selectedFileSize = 0;
	SetCaption(NULL);
}

void
CMainFrame::SetCaption(
	LPCTSTR caption
	)
{
	LPCTSTR title = LoadString(IDR_MAINFRAME);

	TCHAR buffer[MAX_PATH];
	if(caption)
	{
		_stprintf(buffer, TEXT("%s - %s"), title, PathFindFileName(caption));
		SetWindowText(buffer);
	} else
		SetWindowText(title);
	UpdateStatusbar(ID_STATUSBAR_SELECTED);
	UpdateStatusbar(ID_STATUSBAR_TOTAL);
}

void
CMainFrame::UpdateStatusbar(int part)
{
	TCHAR buffer[MAX_PATH];
	switch(part)
	{
	case ID_STATUSBAR_SELECTED:
		_stprintf(buffer, LoadString(IDS_STATUSBAR_SELECTED),
			m_list.GetSelectedCount(), selectedFileSize/1024);
		break;
	case ID_STATUSBAR_TOTAL:
		_stprintf(buffer, LoadString(IDS_STATUSBAR_TOTAL),
			m_msi->getCount(), totalFileSize/1024);
		break;
	}
	m_statusbar.SetText(part, buffer, 0);
}

static
CMainFrame* _this_qsort;

void
CMainFrame::sort()
{
	int count = m_msi->getCount();
	if(count == 0) return;
	
	_this_qsort = this;
	qsort(LVindex, count, sizeof(*LVindex), sortCallback);
	m_list.DeleteAllItems();
	m_list.SetItemCountEx(count, LVSICF_NOINVALIDATEALL);
}

int __cdecl
CMainFrame::sortCallback(
	const void *elem1,
	const void *elem2
	)
{
	CMainFrame* _this = _this_qsort;
	
	int i1 = *(int*)elem1;
	int i2 = *(int*)elem2;
	
	MsiDumpFileDetail detail1, detail2;
	_this->m_msi->GetFileDetail(i1, &detail1);
	_this->m_msi->GetFileDetail(i2, &detail2);

	int retval = 0;
	Columns sortColumn = (Columns)_this->sortColumn;
	switch(sortColumn)
	{

	case COLUME_NAME: 
	{
		retval = _tcsicmp(detail1.filename, detail2.filename);
		break;
	}

	case COLUMN_TYPE:
	{
		LPCTSTR ext1 = _tcsrchr(detail1.filename, TEXT('.'));
		if(ext1)
			ext1++;
		else
			ext1 = TEXT("");

		LPCTSTR ext2 = _tcsrchr(detail2.filename, TEXT('.'));
		if(ext2)
			ext2++;
		else
			ext2 = TEXT("");

		retval = _tcsicmp(ext1, ext2);
		break;
	}

	case COLUMN_SIZE:
	{
		int size1 = detail1.filesize;
		int size2 = detail2.filesize;

		if     (size1  < size2) retval = -1;
		else if(size1 == size2) retval = 0;
		else                    retval = 1;
		break;
	}

	case COLUMN_PATH:
	{
		retval = _tcsicmp(detail1.path, detail2.path);
		break;
	}

	case COLUMN_PLATFORM:
	{
		int plat1, plat2;

		if     ( detail1.win9x && !detail1.winNT) plat1 = 1;
		else if(!detail1.win9x &&  detail1.winNT) plat1 = 2;
		else                                      plat1 = 0;

		if     ( detail2.win9x && !detail2.winNT) plat2 = 1;
		else if(!detail2.win9x &&  detail2.winNT) plat2 = 2;
		else                                      plat2 = 0;

		if     (plat1  < plat2) retval = -1;
		else if(plat1 == plat2) retval = 0;
		else                    retval = 1;

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
