
#pragma once

//
// Platform SDK: Windows Installer: Installer Database Reference
//
// File Table
// =======================================
// Column     Type           Key  Nullable 
// =======================================
// File       Identifier     Y    N 
// Component_ Identifier          N 
// FileName   Filename            N 
// FileSize   DoubleInteger       N 
// Version    Version             Y 
// Language   Language            Y 
// Attributes Integer             Y 
// Sequence   Integer             N 
// =======================================
//

struct MsiDumpFileDetail
{
	LPCTSTR filename;
	int     filesize;
	LPCTSTR path;
	LPCTSTR version;
	LPCTSTR language;
	bool    win9x;    // Should the file be installed on Windows 95/98/Me?
	bool    winNT;    // Should be installed on Windows NT/2000/XP/2003?
	bool    selected;
};

class IMsiDumpCab
{
public:
	virtual void Release() = 0;
	virtual bool Open(LPCTSTR filename) = 0;
	virtual void Close() = 0;
	virtual int  getCount() = 0;
	virtual bool GetFileDetail(int index, MsiDumpFileDetail *detail) = 0;
	virtual void setSelected(int index, bool select) = 0;
	virtual bool ExtractTo(LPCTSTR directory, bool selectAll, bool flatFolder) = 0;

	//
	// delayed open:
	//   stage 1. open the msi file and read out some quick info, e.g. list of filenames
	//   stage 2. read full info e.g. path, and signal 'event' handle when finish
	//
	// this is usefull if you want a fast responsive UI. however keep in mind that
	// GetFileDetail will return different data during stage 1 and stage 2
	//
	virtual bool DelayedOpen(LPCTSTR filename, HANDLE event) = 0;
	
};

IMsiDumpCab *MsiDumpCreateObject();
