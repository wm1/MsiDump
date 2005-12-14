
#pragma once

struct MsiDumpFileDetail
{
	LPCTSTR filename;
	int     filesize;
	LPCTSTR path;
	LPCTSTR version;
	bool    win9x;    // Should the file be installed on Windows 95/98/Me?
	bool    winNT;    // Should be installed on Windows NT/2000/XP/2003?
	bool    selected;
};

class IMsiDumpCab
{
public:
	virtual void Release() = 0;
	virtual bool Open(LPCTSTR filename)  = 0;
	virtual void Close() = 0;
	virtual int  getCount() = 0;
	virtual bool GetFileDetail(int index, MsiDumpFileDetail *detail) = 0;
	virtual void setSelected(int index, bool select) = 0;
	virtual bool ExtractTo(LPCTSTR directory, bool selectAll, bool flatFolder) = 0;
};

IMsiDumpCab *MsiDumpCreateObject();
