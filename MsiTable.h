
#pragma once

class MsiUtils;

class MsiQuery
{
private:
	MSIHANDLE view;
	MSIHANDLE record;
	bool      ended;
	bool      viewCreated;

public:
	MsiQuery(MsiUtils* msiUtils, string sql);
	~MsiQuery();
	MSIHANDLE Next();
};

class MsiTable
{
private:
	int       CountRows();
	string    getPrimaryKey();

protected:
	int       count;
	string    name;
	MsiUtils* msiUtils;

public:
	MsiTable(MsiUtils* theMsiUtils, string tableName);
	virtual ~MsiTable();
	friend class MsiUtils;
};

class MsiFile : public MsiTable
{
private:
	struct tagFile {
		string file;
		string component;
		string filename;
		string version;
		int    filesize;
		int    attributes;
		int    sequence;

		bool   compressed;
		int    keyCabinet;      // if(compressed), which .cab file it residents?
		int    keyDirectory;
		int    keyComponent;
		
		bool   selected;
	} *array;

public:
	MsiFile(MsiUtils* msiUtils);
	virtual ~MsiFile();
	friend class MsiUtils;
};

//
// note: ui.cpp (CMainFrame::OnGetDispInfo, case COLUMN_SIZE) caches filesize locally,
// therefore both MsiSimpleFile and MsiFile must return the same filesize.
// 
class MsiSimpleFile : public MsiTable
{
private:
	struct tagFile {
		string filename;
		int    filesize;
	} *array;

public:
	MsiSimpleFile(MsiUtils* msiUtils);
	virtual ~MsiSimpleFile();
	friend class MsiUtils;
};

class MsiComponent : public MsiTable
{
private:
	struct tagComponent {
		string component;
		string directory;
		string condition;

		int    keyDirectory;
		bool   win9x;
		bool   winNT;
	} *array;

public:
	MsiComponent(MsiUtils* msiUtils);
	virtual ~MsiComponent();
	friend class MsiUtils;
};

class MsiDirectory : public MsiTable
{
private:
	struct tagDirectory {
		string directory;

		string sourceDirectory;
		string targetDirectory;
		
		bool   targetDirectoryVerified;
		bool   targetDirectoryExists;
	} *array;

public:
	MsiDirectory(MsiUtils* msiUtils);
	virtual ~MsiDirectory();
	friend class MsiUtils;
};

class MsiCabinet : public MsiTable
{
private:
	struct tagCabinet {
		int    diskId;
		int    lastSequence;
		string cabinet;

		bool   embedded; // is it stored within .msi file as a separate stream?
		string tempName; // if(embedded), already extracted to a temporary location?
		
		bool   iterated;
	} *array;
	void Extract(int index);

public:
	MsiCabinet(MsiUtils* msiUtils);
	virtual ~MsiCabinet();
	friend class MsiUtils;
};

