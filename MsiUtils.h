
#pragma once

#include <windows.h>
#include <tchar.h>
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
#include <SetupApi.h>

#include "MsiDumpPublic.h"
#include "MsiTable.h"

extern ofstream trace;

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

	MsiFile      *file;
	MsiComponent *component;
	MsiDirectory *directory;
	MsiCabinet   *cabinet;

	MsiUtils();
	~MsiUtils();
	bool IsOpened() { return database != NULL; }
	void LoadDatabase();
	void LoadSummary();
	void ExtractFile(int index);
	void CopyFile(int index);
	bool LocateFile(string, int *pIndex);
	static bool VerifyDirectory(string);
	static UINT CALLBACK CabinetCallback(PVOID, UINT, UINT_PTR, UINT_PTR);
	friend class MsiQuery;
	friend class MsiTable;
	friend IMsiDumpCab* MsiDumpCreateObject();

public:
	void Release();
	bool Open(LPCTSTR filename);
	void Close();
	void ExtractTo(LPCTSTR theDirectory, bool selectAll, bool flatFolder);

	int  getCount();
	void setSelected(int index, bool select);
	bool GetFileDetail(int index, MsiDumpFileDetail *detail);
};
