
#pragma once

#include <windows.h>
#include <wtypes.h>
#include <tchar.h>
#include <process.h>
#include <string>
#include <fstream>
using namespace std;

#ifdef _UNICODE
	#undef  string
	#define string   wstring
	#undef  ofstream
	#define ofstream wofstream
#endif

#include <msiquery.h>
#include <msidefs.h>
#include <setupapi.h>

#include "MsiDumpPublic.h"
#include "MsiTable.h"

#define MSISOURCE_COMPRESSED          0x00000002

#define TEST_FLAG(field, flag)        (((field) & (flag)) != 0)
#define SET_FLAG( field, flag)        ((field) |= (flag))
#define CLEAR_FLAG(field, flag)       ((field) &= ~(flag))

extern ofstream trace;

extern "C" void __cdecl 
threadLoadDatabase(void* parameter);

class MsiUtils : public IMsiDumpCab
{
private:
	string        msiFilename;
	MSIHANDLE     database;
	bool          compressed;
	bool          allSelected;
	bool          folderFlatten;
	string        targetRootDirectory;
	string        sourceRootDirectory;
	int           countDone;
	bool          delayLoading;
	HANDLE        delayEvent;

	MsiFile      *file;
	MsiComponent *component;
	MsiDirectory *directory;
	MsiCabinet   *cabinet;
	MsiSimpleFile*simpleFile;

	MsiUtils();
	~MsiUtils();
	bool IsOpened() { return database != NULL; }
	bool LoadDatabase();
	void DelayLoadDatabase();
	void LoadSummary();
	void ExtractFile(int index);
	void CopyFile(int index);
	bool LocateFile(string, int *pIndex);
	static bool VerifyDirectory(string);
	static UINT CALLBACK CabinetCallback(PVOID, UINT, UINT_PTR, UINT_PTR);
	friend class MsiQuery;
	friend class MsiTable;
	friend IMsiDumpCab* MsiDumpCreateObject();
	friend void __cdecl threadLoadDatabase(void* parameter);
	bool Open(LPCTSTR filename, bool delay, HANDLE event);

public:
	void Release();
	bool Open(LPCTSTR filename)
	{
		return Open(filename, false, NULL);
	}
	bool DelayedOpen(LPCTSTR filename, HANDLE event)
	{
		return Open(filename, true, event);
	}
	void Close();
	bool ExtractTo(LPCTSTR theDirectory, bool selectAll, bool flatFolder);

	int  getCount();
	void setSelected(int index, bool select);
	bool GetFileDetail(int index, MsiDumpFileDetail *detail);
};
