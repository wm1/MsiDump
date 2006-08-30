// MainFrame.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

extern "C" void __cdecl 
threadWaitDelayLoad(void* parameter);

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>,
		public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	virtual BOOL PreTranslateMessage(MSG* pMsg)
	{
		return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
	}

	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_EDIT_SELECT_ALL,  UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(IDM_EXTRACT_FILES,   UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
		UPDATE_ELEMENT(IDM_EXPORT_FILELIST, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE,                OnCreate)
		MESSAGE_HANDLER(WM_DROPFILES,             OnDropFiles)
		MESSAGE_HANDLER(WM_CONTEXTMENU,           OnContextMenu)
		MESSAGE_HANDLER(WM_SETCURSOR,             OnSetCursor)
		COMMAND_ID_HANDLER(ID_FILE_OPEN,          OnFileOpen)
		COMMAND_ID_HANDLER(ID_APP_EXIT,           OnAppExit)
		COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL,    OnEditSelectAll)
		COMMAND_ID_HANDLER(IDM_EXTRACT_FILES,     OnExtractFiles)
		COMMAND_ID_HANDLER(IDM_EXPORT_FILELIST,   OnExportFileList)
		COMMAND_ID_HANDLER(ID_APP_ABOUT,          OnAppAbout)
		NOTIFY_CODE_HANDLER(LVN_COLUMNCLICK,      OnColumnClick)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED,      OnItemChanged)
		NOTIFY_CODE_HANDLER(LVN_BEGINDRAG,        OnBeginDrag)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO,      OnGetDispInfo)
		NOTIFY_CODE_HANDLER(LVN_ODSTATECHANGED,   OnODStateChanged)
		NOTIFY_CODE_HANDLER(NM_RCLICK,            OnRightClick)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate         (UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnDropFiles      (UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnContextMenu    (UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnSetCursor      (UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnFileOpen       (WORD, WORD, HWND, BOOL&);
	LRESULT OnAppExit        (WORD, WORD, HWND, BOOL&)
	{
		SendMessage(WM_CLOSE);
		return 0;
	}
	LRESULT OnEditSelectAll  (WORD, WORD, HWND, BOOL&)
	{
		m_list.SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
		return 0;
	}
	LRESULT OnExtractFiles   (WORD, WORD, HWND, BOOL&);
	LRESULT OnExportFileList (WORD, WORD, HWND, BOOL&);
	LRESULT OnColumnClick    (int, LPNMHDR, BOOL&);
	LRESULT OnItemChanged    (int, LPNMHDR, BOOL&);
	LRESULT OnBeginDrag      (int, LPNMHDR, BOOL&);
	LRESULT OnGetDispInfo    (int, LPNMHDR, BOOL&);
	LRESULT OnODStateChanged (int, LPNMHDR, BOOL&);
	LRESULT OnRightClick     (int, LPNMHDR, BOOL&);
	LRESULT OnAppAbout       (WORD, WORD, HWND, BOOL&)
	{
		CAboutDlg dlg;
		dlg.DoModal();
		return 0;
	}

	CMainFrame(LPCTSTR cmdLine) { CmdLine = cmdLine; }
	~CMainFrame() { Cleanup(); m_msi->Release(); }

private:
	void SetCaption(LPCTSTR caption);
	void Cleanup();
	void LoadMsiFiles(LPCTSTR filename);
	void UpdateStatusbar(int part);

	CListViewCtrl   m_list;
	int            *LVindex;   // map ListView[iItem] to getfiledetail(index)
	LPCTSTR        *filesizes; // store listview.iItem[COLUMN_SIZE]
	CStatusBarCtrl  m_statusbar;
	IMsiDumpCab    *m_msi;

	LPCTSTR CmdLine;
	ULONGLONG totalFileSize;
	ULONGLONG selectedFileSize;
	bool    selectionChanged;
	bool    waitCursor;
	HCURSOR hWaitCursor;

	HANDLE  delayEvent;
	bool    delayLoading;
	friend void __cdecl threadWaitDelayLoad(void* parameter);

	// sorting stuff
	int  sortColumn;
	BOOL sortAscending;
	void sort();
	static int __cdecl sortCallback(const void*, const void*);
};

enum Columns
{
	// the first column must be numbered as iSubItem = 0
	//
	COLUME_NAME = 0,

	COLUMN_TYPE,
	COLUMN_SIZE,
	COLUMN_PATH,
	COLUMN_PLATFORM,
	COLUMN_VERSION,
	COLUMN_LANGUAGE
};

