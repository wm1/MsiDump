
#include "MsiUtils.h"

ofstream trace;

char*  trace_file =

#if ENABLE_TRACE == 1
	"trace.txt"
#else
	"nul"
#endif

;

////////////////////////////////////////////////////////////////////////

MsiUtils::MsiUtils()
{
	
	trace.open(trace_file);
	database = NULL;

	MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
}

MsiUtils::~MsiUtils()
{
	Close();
	trace.close();
}

void
MsiUtils::Release()
{
	delete this;
}

bool
MsiUtils::Open(
	LPCTSTR filename
	)
{
	LPTSTR pFilePart;
	TCHAR  buffer[MAX_PATH];
	if(filename[0] == TEXT('\\') && filename[1] == TEXT('\\'))
	{
		lstrcpy(buffer, filename);
		pFilePart = _tcsrchr(buffer, TEXT('\\'));
	}
	else
	{
		GetFullPathName(filename, MAX_PATH, buffer, &pFilePart);
	}
	if(!pFilePart)
	{
		database = NULL;
		return false;
	}

	msiFilename = buffer;
	*pFilePart = TEXT('\0');
	sourceRootDirectory = buffer;
	trace << TEXT("******************************") << endl
		<< msiFilename << endl 
		<< endl;

	UINT r = MsiOpenDatabase(msiFilename.c_str(), MSIDBOPEN_READONLY, &database);
	if(r != ERROR_SUCCESS) 
	{
		database = NULL;
		return false;
	}

	LoadSummary();
	LoadDatabase();
	return true;
}

void
MsiUtils::Close()
{
	if(IsOpened())
	{
		MsiCloseHandle(database);
		database = NULL;

		delete cabinet;   cabinet   = NULL;
		delete directory; directory = NULL;
		delete component; component = NULL;
		delete file;      file      = NULL;
	}
}

void
MsiUtils::LoadSummary()
{
	MSIHANDLE summaryInformation;
	UINT      datatype;
	INT       data;
	FILETIME  filetime;
	DWORD     size = MAX_PATH;
	TCHAR     buffer[MAX_PATH];
	UINT      r;

	compressed  = false;
	r = MsiGetSummaryInformation(database, 0, 0, &summaryInformation);
	if(r != ERROR_SUCCESS)
		return;

	r = MsiSummaryInfoGetProperty(summaryInformation, PID_MSISOURCE,
		&datatype, &data, &filetime, buffer, &size);

	if(r == ERROR_SUCCESS && datatype == 3) // VT_I4 = 3, defined in wtype.h
		compressed = ((data & 2) != 0);

	trace << TEXT("compressed: ") << compressed << endl << endl;
}

void
MsiUtils::LoadDatabase()
{
	cabinet   = new MsiCabinet  (this);
	directory = new MsiDirectory(this);
	component = new MsiComponent(this);
	file      = new MsiFile     (this);

	int i, j;
	trace << TEXT("Cabinet count = ") << cabinet->count << endl << endl;
	for(i=0; i<cabinet->count; i++)
	{
		MsiCabinet::tagCabinet *p = &cabinet->array[i];
		trace << i << TEXT("[last seq = ") << p->lastSequence << TEXT("]: ")
			<< p->cabinet << endl;
	}
	trace << endl;

	MSIHANDLE product;
	TCHAR     packageName[30];
	DWORD     size = MAX_PATH;
	TCHAR     buffer[MAX_PATH];
	_stprintf(packageName, TEXT("#%d"), (DWORD)database);
	MsiOpenPackage(packageName, &product);
	MsiDoAction(product, TEXT("CostInitialize"));
	MsiDoAction(product, TEXT("CostFinalize"));
	trace << TEXT("Directory count = ") << directory->count << endl << endl;
	for(i=0; i<directory->count; i++)
	{
		MsiDirectory::tagDirectory *p = &directory->array[i];

		size = MAX_PATH;
		MsiGetSourcePath(product, p->directory.c_str(), buffer, &size);
		p->sourceDirectory = buffer;

		size = MAX_PATH;
		MsiGetTargetPath(product, p->directory.c_str(), buffer, &size);
		p->targetDirectory = &buffer[2]; // skip drive letter part of C:\xxxx

		trace << i << TEXT(": ") << p->directory << endl
			<< TEXT("    ") << p->sourceDirectory << endl
			<< TEXT("    ") << p->targetDirectory << endl;
	}
	trace << endl;

	trace << TEXT("Component count = ") << component->count << endl << endl;
	for(i=0; i<component->count; i++)
	{
		MsiComponent::tagComponent *p = &component->array[i];
		for(j=0; j<directory->count; j++)
		{
			MsiDirectory::tagDirectory *q = &directory->array[j];
			if(p->directory == q->directory)
			{
				p->keyDirectory = j;
				break;
			}
		}
		
		p->win9x = true;
		p->winNT = true;
		if(!p->condition.empty())
		{
			LPCTSTR condition = p->condition.c_str();
			MsiSetProperty(product, TEXT("Version9X"), TEXT("490"));
			MsiSetProperty(product, TEXT("VersionNT"), TEXT(""));
			p->win9x = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
			MsiSetProperty(product, TEXT("Version9X"), TEXT(""));
			MsiSetProperty(product, TEXT("VersionNT"), TEXT("502"));
			p->winNT = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
		}
		
		trace << i << TEXT("[dir = ") << p->keyDirectory;
		if(!p->condition.empty())
			trace << TEXT(", condition = ") << p->condition;
		trace << TEXT("]: ") << p->component << endl;
	}
	trace << endl;
	MsiCloseHandle(product);

	trace << TEXT("File count = ") << file->count << endl << endl;
	for(i=0; i<file->count; i++)
	{
		MsiFile::tagFile *p = &file->array[i];
		for(j=0; j<component->count; j++)
		{
			MsiComponent::tagComponent *q = &component->array[j];
			if(p->component == q->component)
			{
				p->keyDirectory = q->keyDirectory;
				p->keyComponent = j;
				break;
			}
		}

		for(j=0; j<cabinet->count; j++)
		{
			MsiCabinet::tagCabinet *q = &cabinet->array[j];
			if(p->sequence <= q->lastSequence)
			{
				p->keyCabinet = j;
				break;
			}
		}

		p->selected   = false;
		p->compressed = compressed;
		if(p->attributes & msidbFileAttributesCompressed)
			p->compressed = true;
		if(p->attributes & msidbFileAttributesNoncompressed)
			p->compressed = false;

		trace << p->sequence << TEXT("[comp = ") << p->keyComponent
			<< TEXT(", dir = ") << p->keyDirectory;
		if(p->compressed)
			trace << TEXT(", cab = ") << p->keyCabinet;
		trace << TEXT("]: ") << p->filename << endl;
	}
	trace << endl;
}

void
MsiUtils::ExtractTo(
	LPCTSTR theDirectory,
	bool    selectAll,
	bool    flatFolder
	)
{
	TCHAR buffer[MAX_PATH];
	GetFullPathName(theDirectory, MAX_PATH, buffer, NULL);
	theDirectory = buffer;
	trace << TEXT("Extract to: ") << theDirectory << endl << endl;
	if(!VerifyDirectory(theDirectory))
		return;

	targetRootDirectory = theDirectory;
	allSelected   = selectAll;
	folderFlatten = flatFolder;
	if(folderFlatten &&
		!VerifyDirectory(targetRootDirectory + TEXT('\\')))
		return;

	int i;

	for(i=0; i<cabinet->count; i++)
		cabinet->array[i].iterated = false;

	for(i=0; i<directory->count; i++)
		directory->array[i].targetDirectoryVerified = false;

	for(i=0; i<file->count; i++)
	{
		MsiFile::tagFile *p = &file->array[i];
		if(!allSelected && !p->selected) continue;
		
		if(p->compressed)
			ExtractFile(i);
		else
			CopyFile(i);
	}
}

void
MsiUtils::CopyFile(
	int index
	)
{
	MsiFile::tagFile           *p          = &file->array[index];
	MsiDirectory::tagDirectory *pDirectory = &directory->array[ p->keyDirectory ];

	string source = pDirectory->sourceDirectory + p->filename;

	//
	// targetRootDirectory = c:\temp
	// pDirectory->targetDirectory = \Program Files\Orca\
	// p->filename = orca.exe
	//
	// target = c:\temp\Program Files\Orca\orca.exe
	//
	string middle = (folderFlatten ? TEXT("\\") : pDirectory->targetDirectory);
	string target = targetRootDirectory + middle + p->filename;

	trace << source << endl << TEXT("=> ") << target << endl << endl;
	::CopyFile(source.c_str(), target.c_str(), FALSE);
}

void
MsiUtils::ExtractFile(
	int index
	)
{
	MsiFile::tagFile       *pFile    = &file->array[index];
	MsiCabinet::tagCabinet *pCabinet = &cabinet->array[ pFile->keyCabinet ];
	
	if(pCabinet->iterated)
		return;
	pCabinet->iterated = true;

	string sourceCabinet;
	if(pCabinet->embedded)
	{
		cabinet->Extract(pFile->keyCabinet);
		sourceCabinet = pCabinet->tempName;
		trace << TEXT("Extract ") << pCabinet->cabinet
			<< TEXT(" to ") << pCabinet->tempName << endl;
	}
	else
	{
		sourceCabinet = sourceRootDirectory + pCabinet->cabinet;
		trace << TEXT("cabinet: ") << sourceCabinet << endl;
	}
	
	SetupIterateCabinet(sourceCabinet.c_str(), 0, CabinetCallback, this);
}

//
// the input format must be an full path (e.g. C:\path or \\server\share)
//
bool
MsiUtils::VerifyDirectory(
	string s
	)
{
	CreateDirectory(s.c_str(), NULL);
	DWORD attributes = GetFileAttributes(s.c_str());
	if((attributes != INVALID_FILE_ATTRIBUTES)
		&& ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0))
		return true;

	TCHAR pathSeperator = TEXT('\\');
	if(s[s.length()-1] != pathSeperator)
		s.append(TEXT("\\"));

	TCHAR  buffer[MAX_PATH];
	_tcscpy(buffer, s.c_str());
	
	string::size_type index = string::npos;
	if(s[1] == TEXT(':') && s[2] == pathSeperator)
	{
		// it is "C:\path\"
		index = s.find(pathSeperator, 3);
	}
	else if(s[0] == pathSeperator && s[1] == pathSeperator)
	{
		// it is a UNC path, "\\server\share\path\"
		index = s.find(pathSeperator, 2);
		index = s.find(pathSeperator, index + 1);
	}

	while(index != string::npos)
	{
		buffer[index] = TEXT('\0');
		attributes = GetFileAttributes(buffer);
		if(attributes == INVALID_FILE_ATTRIBUTES)
			CreateDirectory(buffer, NULL);
		else if((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			return false;
		buffer[index] = pathSeperator;

		index = s.find(pathSeperator, index + 1);
	}

	attributes = GetFileAttributes(s.c_str());
	return ((attributes != INVALID_FILE_ATTRIBUTES)
		&& ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

UINT CALLBACK
MsiUtils::CabinetCallback(
	PVOID    context,
	UINT     notification,
	UINT_PTR param1,
	UINT_PTR param2
	)
{
	MsiUtils             *msiUtils    = (MsiUtils*)context;
	FILE_IN_CABINET_INFO *cabinetInfo = (FILE_IN_CABINET_INFO*)param1;
	switch(notification)
	{
	case SPFILENOTIFY_NEEDNEWCABINET: return ERROR_FILE_NOT_FOUND;
	case SPFILENOTIFY_FILEINCABINET:  break;
	case SPFILENOTIFY_FILEEXTRACTED:
	default:                          return NO_ERROR;
	}
	
	int  index;
	bool located = msiUtils->LocateFile(cabinetInfo->NameInCabinet, &index);

	if(located && 
		(msiUtils->allSelected || msiUtils->file->array[index].selected)
		)
	{
		MsiFile::tagFile *p = &msiUtils->file->array[index];
		string middle = TEXT("\\");;
		if(!msiUtils->folderFlatten)
		{
			MsiDirectory::tagDirectory *d = &msiUtils->directory->array[p->keyDirectory];
			if(!d->targetDirectoryVerified)
			{
				d->targetDirectoryVerified = true;
				d->targetDirectoryExists   = VerifyDirectory(
					msiUtils->targetRootDirectory + d->targetDirectory);
			}
			if(!d->targetDirectoryExists)
				return FILEOP_SKIP;
			middle = d->targetDirectory;
		}
		string targetFilename = msiUtils->targetRootDirectory + middle + p->filename;
		_tcscpy(cabinetInfo->FullTargetName, targetFilename.c_str());
		trace << TEXT("... ") << p->filename 
			<< TEXT("\t") << targetFilename << endl;
		return FILEOP_DOIT;
	}

	return FILEOP_SKIP;
}

bool
MsiUtils::LocateFile(
	string filename,
	int   *pIndex
	)
{
	for(int i=0; i<file->count; i++)
		if(file->array[i].file == filename)
		{
			*pIndex = i;
			return true;
		}
	return false;
}

bool
MsiUtils::GetFileDetail(
	int                index,
	MsiDumpFileDetail* detail
	)
{
	if(index < 0 || index > file->count)
		return false;

	MsiFile::tagFile *p = &file->array[index];
	detail->filename = p->filename.c_str();
	detail->filesize = p->filesize;
	detail->path     = directory->array[ p->keyDirectory ].targetDirectory.c_str();
	detail->win9x    = component->array[ p->keyComponent ].win9x;
	detail->winNT    = component->array[ p->keyComponent ].winNT;
	detail->selected = p->selected;
	return true;
}

void
MsiUtils::setSelected(
	int  index,
	bool select
	)
{
	if(index < 0 || index > file->count)
		return;

	file->array[index].selected = select;
}

int
MsiUtils::getCount()
{
	return (IsOpened() ? file->count : 0);
}

IMsiDumpCab*
MsiDumpCreateObject()
{
	MsiUtils *msiUtils = new MsiUtils();
	return (IMsiDumpCab*)msiUtils;
}
