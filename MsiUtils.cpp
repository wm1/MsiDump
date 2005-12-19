
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
TCHAR pathSeperator = TEXT('\\');

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
	LPCTSTR filename,
	bool    delay,
	HANDLE  event
	)
{
	Close();
	LPTSTR pFilePart;
	TCHAR  buffer[MAX_PATH];
	GetFullPathName(filename, MAX_PATH, buffer, &pFilePart);
	if(!pFilePart)
	{
		return false;
	}
	DWORD fileAttr = GetFileAttributes(buffer);
	if((fileAttr == INVALID_FILE_ATTRIBUTES)
		|| TEST_FLAG(fileAttr, FILE_ATTRIBUTE_DEVICE   )
		|| TEST_FLAG(fileAttr, FILE_ATTRIBUTE_DIRECTORY)
		)
	{
		return false;
	}

	msiFilename = buffer;

	if(buffer + 3 == pFilePart) // C:\sample.msi
		*pFilePart = TEXT('\0');
	else
		*(pFilePart-1) = TEXT('\0');

	sourceRootDirectory = buffer;
	trace << TEXT("******************************") << endl
		<< msiFilename << endl 
		<< sourceRootDirectory << endl
		<< endl;

	UINT r = MsiOpenDatabase(msiFilename.c_str(), MSIDBOPEN_READONLY, &database);
	if(r != ERROR_SUCCESS) 
	{
		database = NULL;
		return false;
	}

	delayLoading = delay;
	delayEvent   = event;
	if(delay == false)
	{
		simpleFile = NULL;
		LoadSummary();
		bool b = LoadDatabase();
		if(!b)
		{
			Close();
			return false;
		}
	}
	else 
	{
		cabinet   = NULL;
		directory = NULL;
		component = NULL;
		file      = NULL;
		DelayLoadDatabase();
	}
	return true;
}

void
MsiUtils::Close()
{
	if(!IsOpened())
		return;

	MsiCloseHandle(database);
	database = NULL;

	if(cabinet)   { delete cabinet;   cabinet   = NULL; }
	if(directory) { delete directory; directory = NULL; }
	if(component) { delete component; component = NULL; }
	if(file)      { delete file;      file      = NULL; }
	if(simpleFile){ delete simpleFile;simpleFile= NULL; }
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

	if(r == ERROR_SUCCESS && datatype == VT_I4)
		compressed = TEST_FLAG(data, MSISOURCE_COMPRESSED);

	trace << TEXT("compressed: ") << compressed << endl << endl;
}

bool
MsiUtils::LoadDatabase()
{
	cabinet   = new MsiCabinet  (this);
	directory = new MsiDirectory(this);
	component = new MsiComponent(this);
	file      = new MsiFile     (this);
	UINT r;

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
	r = MsiOpenPackageEx(packageName, MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE, &product);
	if(r != ERROR_SUCCESS)
		return FALSE;

	r = MsiDoAction(product, TEXT("CostInitialize"));
	if(r != ERROR_SUCCESS)
	{
		MsiCloseHandle(product);
		return FALSE;
	}
	r = MsiDoAction(product, TEXT("CostFinalize"));
	if(r != ERROR_SUCCESS)
	{
		MsiCloseHandle(product);
		return FALSE;
	}
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
	return true;
}

bool
MsiUtils::ExtractTo(
	LPCTSTR theDirectory,
	bool    selectAll,
	bool    flatFolder
	)
{
	if(delayLoading) return false;

	TCHAR buffer[MAX_PATH];
	GetFullPathName(theDirectory, MAX_PATH, buffer, NULL);
	theDirectory = buffer;
	trace << TEXT("Extract to: ") << theDirectory << endl << endl;
	if(!VerifyDirectory(theDirectory))
		return false;

	targetRootDirectory = theDirectory;
	allSelected   = selectAll;
	folderFlatten = flatFolder;
	if(folderFlatten &&
		!VerifyDirectory(targetRootDirectory))
		return false;

	int i;

	for(i=0; i<cabinet->count; i++)
		cabinet->array[i].iterated = false;

	for(i=0; i<directory->count; i++)
		directory->array[i].targetDirectoryVerified = false;

	int countTodo = 0;
	this->countDone = 0;
	for(i=0; i<file->count; i++)
	{
		MsiFile::tagFile *p = &file->array[i];
		if(!allSelected && !p->selected) continue;
		countTodo++;
		
		if(p->compressed)
			ExtractFile(i);
		else
			CopyFile(i);
	}
	if(countTodo != countDone)
	{
		trace << endl 
			<< TEXT("Error: ") << (countTodo - countDone)
			<< TEXT(" files are not extracted") << endl;
		return false;
	}
	return true;
}

void
MsiUtils::CopyFile(
	int index
	)
{
	MsiFile::tagFile           *p          = &file->array[index];
	MsiDirectory::tagDirectory *pDirectory = &directory->array[ p->keyDirectory ];

	string source = pDirectory->sourceDirectory + pathSeperator + p->filename;

	//
	// targetRootDirectory = c:\temp
	// pDirectory->targetDirectory = \Program Files\Orca\
	// p->filename = orca.exe
	//
	// target = c:\temp\Program Files\Orca\orca.exe
	//
	string target = (folderFlatten)
		? (targetRootDirectory + pathSeperator + p->filename)
		: (targetRootDirectory + pathSeperator + pDirectory->targetDirectory 
		                       + pathSeperator + p->filename);

	trace << source << endl << TEXT("=> ") << target << endl << endl;
	BOOL b = ::CopyFile(source.c_str(), target.c_str(), FALSE);
	if(b)
		countDone ++;
	else
		trace << "Error copy file" << endl;
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
		sourceCabinet = sourceRootDirectory + pathSeperator + pCabinet->cabinet;
		trace << TEXT("cabinet: ") << sourceCabinet << endl;
	}
	
	DWORD attributes = GetFileAttributes(sourceCabinet.c_str());
	if(attributes == INVALID_FILE_ATTRIBUTES)
	{
		trace << TEXT("Error: cabinet not found") << endl;
		return;
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
		&& TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY))
		return true;

	if(s[s.length()-1] != pathSeperator)
		s.append(1, pathSeperator);

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
		{
			CreateDirectory(buffer, NULL);
			GetFileAttributes(buffer);
		}

		if(!TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY))
			return false;
		buffer[index] = pathSeperator;

		index = s.find(pathSeperator, index + 1);
	}

	attributes = GetFileAttributes(s.c_str());
	return ((attributes != INVALID_FILE_ATTRIBUTES)
		&& TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY));
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
		
		string targetFilename;
		if(msiUtils->folderFlatten)
		{
			targetFilename = msiUtils->targetRootDirectory + pathSeperator + p->filename;
		}
		else
		{
			MsiDirectory::tagDirectory *d = &msiUtils->directory->array[p->keyDirectory];
			if(!d->targetDirectoryVerified)
			{
				d->targetDirectoryVerified = true;
				d->targetDirectoryExists   = VerifyDirectory(
					msiUtils->targetRootDirectory + pathSeperator + d->targetDirectory);
			}
			if(!d->targetDirectoryExists)
				return FILEOP_SKIP;
			targetFilename = msiUtils->targetRootDirectory + pathSeperator + d->targetDirectory + pathSeperator + p->filename;
		}

		_tcscpy(cabinetInfo->FullTargetName, targetFilename.c_str());
		trace << TEXT("... ") << p->filename 
			<< TEXT("\t") << targetFilename << endl;

		msiUtils->countDone ++;
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
		if(_tcsicmp(file->array[i].file.c_str(), filename.c_str()) == 0)
		{
			*pIndex = i;
			return true;
		}
	trace << "Error: file not found: " << filename << endl;
	return false;
}

bool
MsiUtils::GetFileDetail(
	int                index,
	MsiDumpFileDetail* detail
	)
{
	if(delayLoading)
	{
		if(index < 0 || index > simpleFile->count)
			return false;
		
		MsiSimpleFile::tagFile* p = &simpleFile->array[index];
		ZeroMemory(detail, sizeof(MsiDumpFileDetail));

		detail->filename = p->filename.c_str();
		detail->filesize = p->filesize;
		return true;
	}
	
	if(index < 0 || index > file->count)
		return false;

	MsiFile::tagFile *p = &file->array[index];
	detail->filename = p->filename.c_str();
	detail->filesize = p->filesize;
	detail->path     = directory->array[ p->keyDirectory ].targetDirectory.c_str();
	detail->win9x    = component->array[ p->keyComponent ].win9x;
	detail->winNT    = component->array[ p->keyComponent ].winNT;
	detail->selected = p->selected;
	detail->version  = p->version.c_str();
	return true;
}

void
MsiUtils::setSelected(
	int  index,
	bool select
	)
{
	if(delayLoading) return;

	if(index < 0 || index > file->count)
		return;

	file->array[index].selected = select;
}

int
MsiUtils::getCount()
{
	return (IsOpened()
		? (delayLoading ? simpleFile->count : file->count)
		: 0);
}

IMsiDumpCab*
MsiDumpCreateObject()
{
	MsiUtils *msiUtils = new MsiUtils();
	return (IMsiDumpCab*)msiUtils;
}

extern "C" void __cdecl 
threadLoadDatabase(void* parameter)
{
	MsiUtils* _this = (MsiUtils*)parameter;
	_this->LoadSummary();
	_this->LoadDatabase();
	_this->delayLoading = false;
	SetEvent(_this->delayEvent);
}

void
MsiUtils::DelayLoadDatabase()
{
	simpleFile = new MsiSimpleFile(this);
	_beginthread(threadLoadDatabase, 0, this);
}
