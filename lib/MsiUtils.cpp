#include "precomp.h"

wofstream trace;

char* trace_file =

#if ENABLE_TRACE == 1
        "trace.txt"
#else
        "nul"
#endif
        ;

////////////////////////////////////////////////////////////////////////
WCHAR pathSeperator = L'\\';

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

void MsiUtils::Release()
{
        delete this;
}

bool MsiUtils::DoOpen(
        PCWSTR filename)
{
        Close();
        PWSTR pFilePart;
        WCHAR buffer[MAX_PATH];
        DWORD result = GetFullPathName(filename, MAX_PATH, buffer, &pFilePart);
        if (result == 0 || !pFilePart)
        {
                DWORD error =  GetLastError();
                trace_error << L"GetFullPathName failed with: " << error << endl;
                return false;
        }
        DWORD fileAttr = GetFileAttributes(buffer);
        if ((fileAttr == INVALID_FILE_ATTRIBUTES) || TEST_FLAG(fileAttr, FILE_ATTRIBUTE_DEVICE) || TEST_FLAG(fileAttr, FILE_ATTRIBUTE_DIRECTORY))
        {
                trace_error << buffer << L" is not a normal file" << endl;
                return false;
        }

        msiFilename = buffer;

        PCWSTR fileExt = wcsrchr(buffer, L'.');
        if (!fileExt)
        {
                trace_error << buffer << L" does not have any file extension" << endl;
                return false;
        }

        if (_wcsicmp(fileExt, L".msi") == 0)
                db_type = installer_database;
        else if (_wcsicmp(fileExt, L".msm") == 0)
                db_type = merge_module;
        else
        {
                trace_error << buffer << L" does not have a recognized file extension" << endl;
                return false;
        }

        // sourceRootDirectory now contains the trailing path separator
        //
        *pFilePart          = L'\0';
        sourceRootDirectory = buffer;
        trace << L"******************************" << endl
              << msiFilename << endl
              << sourceRootDirectory << endl
              << endl;

        UINT r = MsiOpenDatabase(msiFilename.c_str(), MSIDBOPEN_READONLY, &database);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"failed to open msi file, err = " << r << endl;
                database = NULL;
                return false;
        }

        if (delayLoading == false)
        {
                simpleFile = NULL;
                bool b = LoadSummary();
                if (!b)
                {
                        Close();
                        return false;
                }
                b = LoadDatabase();
                if (!b)
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

void MsiUtils::Close()
{
        if (!IsOpened())
                return;

        MsiCloseHandle(database);
        database = NULL;

        if (cabinet)
        {
                delete cabinet;
                cabinet = NULL;
        }
        if (directory)
        {
                delete directory;
                directory = NULL;
        }
        if (component)
        {
                delete component;
                component = NULL;
        }
        if (file)
        {
                delete file;
                file = NULL;
        }
        if (simpleFile)
        {
                delete simpleFile;
                simpleFile = NULL;
        }
}

bool MsiUtils::LoadSummary()
{
        MSIHANDLE summaryInformation;
        UINT      datatype;
        INT       data;
        FILETIME  filetime;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        UINT      r;

        compressed = false;
        r          = MsiGetSummaryInformation(database, 0, 0, &summaryInformation);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"MsiGetSummaryInformation failed with " << r << endl;
                return false;
        }

        r = MsiSummaryInfoGetProperty(summaryInformation, PID_MSISOURCE,
                                      &datatype, &data, &filetime, buffer, &size);

        if (r == ERROR_SUCCESS && datatype == VT_I4)
                compressed = TEST_FLAG(data, MSISOURCE_COMPRESSED);

        if (db_type == merge_module)
                compressed = true;

        trace << L"compressed: " << compressed << endl
              << endl;

        return true;
}

bool MsiUtils::LoadDatabase()
{
        cabinet   = new MsiCabinet(this);
        directory = new MsiDirectory(this);
        component = new MsiComponent(this);
        file      = new MsiFile(this);
        UINT r;

        int i, j;
        trace << L"Cabinet count = " << cabinet->count << endl
              << endl;
        for (i = 0; i < cabinet->count; i++)
        {
                MsiCabinet::tagCabinet* p = &cabinet->array[i];
                trace << i << L"[last seq = " << p->lastSequence << L"]: "
                      << p->cabinet << endl;
        }
        trace << endl;

        MSIHANDLE product;
        WCHAR     packageName[30];
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        swprintf_s(packageName, 30, L"#%d", (int)database);
        r = MsiOpenPackageEx(packageName, MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE, &product);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"MsiOpenPackageEx(" << packageName << L") failed with " << r << endl;
                return false;
        }

        // Calculate all files' sizes.
        //
        // https://msdn.microsoft.com/en-us/library/windows/desktop/aa368593.aspx
        // Costing is the process of determining the total disk space requirements for an installation.
        // (After CostInitialize, FileCost and CostFinalize ..) the costing data (is) available through the Component table.
        //
        r = MsiDoAction(product, L"CostInitialize");
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"CostInitialize failed with " << r << endl;
                MsiCloseHandle(product);
                return false;
        }
        r = MsiDoAction(product, L"FileCost");
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"FileCost failed with " << r << endl;
                MsiCloseHandle(product);
                return false;
        }
        r = MsiDoAction(product, L"CostFinalize");
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"FileCost failed with " << r << endl;
                MsiCloseHandle(product);
                return false;
        }

        trace << L"Directory count = " << directory->count << endl
              << endl;
        for (i = 0; i < directory->count; i++)
        {
                MsiDirectory::tagDirectory* p = &directory->array[i];

                size = MAX_PATH;
                r = MsiGetSourcePath(product, p->directory.c_str(), buffer, &size);
                if (r != ERROR_SUCCESS)
                {
                        trace_error << L"MsiGetSourcePath(" << p->directory.c_str() << ") failed with " << r << endl;
                        return false;
                }
                p->sourceDirectory = buffer;

                size = MAX_PATH;
                r = MsiGetTargetPath(product, p->directory.c_str(), buffer, &size);
                if (r != ERROR_SUCCESS)
                {
                        trace_error << L"MsiGetTargetPath(" << p->directory.c_str() << ") failed with " << r << endl;
                        return false;
                }
                p->targetDirectory = &buffer[2]; // skip drive letter part of C:\xxxx

                trace << i << L": " << p->directory << endl
                      << L"    " << p->sourceDirectory << endl
                      << L"    " << p->targetDirectory << endl;
        }
        trace << endl;

        trace << L"Component count = " << component->count << endl
              << endl;
        for (i = 0; i < component->count; i++)
        {
                MsiComponent::tagComponent* p = &component->array[i];
                for (j = 0; j < directory->count; j++)
                {
                        MsiDirectory::tagDirectory* q = &directory->array[j];
                        if (p->directory == q->directory)
                        {
                                p->keyDirectory = j;
                                break;
                        }
                }

                p->win9x  = true;
                p->winNT  = true;
                p->winX64 = false;
                if (!p->condition.empty())
                {
                        // NOTE: there is no common rule to define which architecture the msi
                        // package or individual files are built for. Therefore what we're
                        // doing here is guess at our best effort
                        //
                        PCWSTR condition = p->condition.c_str();

                        MsiSetProperty(product, L"Version9X", L"490");
                        MsiSetProperty(product, L"VersionNT", L"");
                        MsiSetProperty(product, L"VersionNT64", L"");
                        p->win9x = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);

                        MsiSetProperty(product, L"Version9X", L"");
                        MsiSetProperty(product, L"VersionNT", L"502");
                        MsiSetProperty(product, L"VersionNT64", L"");
                        p->winNT = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);

                        MsiSetProperty(product, L"Version9X", L"");
                        MsiSetProperty(product, L"VersionNT", L"");
                        MsiSetProperty(product, L"VersionNT64", L"1");
                        bool x64Pos, x64Neg;
                        x64Pos = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
                        MsiSetProperty(product, L"VersionNT64", L"");
                        x64Neg    = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
                        p->winX64 = (x64Pos == true && x64Neg == false);
                }

                trace << i << L"[dir = " << p->keyDirectory;
                if (!p->condition.empty())
                        trace << L", condition = " << p->condition;
                trace << L"]: " << p->component << endl;
        }
        trace << endl;
        MsiCloseHandle(product);

        trace << L"File count = " << file->count << endl
              << endl;
        for (i = 0; i < file->count; i++)
        {
                MsiFile::tagFile* p = &file->array[i];
                for (j = 0; j < component->count; j++)
                {
                        MsiComponent::tagComponent* q = &component->array[j];
                        if (p->component == q->component)
                        {
                                p->keyDirectory = q->keyDirectory;
                                p->keyComponent = j;
                                break;
                        }
                }

                if (db_type == merge_module)
                        p->keyCabinet = 0;
                else
                {
                        for (j = 0; j < cabinet->count; j++)
                        {
                                MsiCabinet::tagCabinet* q = &cabinet->array[j];
                                if (p->sequence <= q->lastSequence)
                                {
                                        p->keyCabinet = j;
                                        break;
                                }
                        }
                }

                p->selected   = false;
                p->compressed = compressed;
                if (p->attributes & msidbFileAttributesCompressed)
                        p->compressed = true;
                if (p->attributes & msidbFileAttributesNoncompressed)
                        p->compressed = false;

                trace << p->sequence << L"[comp = " << p->keyComponent
                      << L", dir = " << p->keyDirectory;
                if (p->compressed)
                        trace << L", cab = " << p->keyCabinet;
                trace << L"]: " << p->filename << endl;
        }
        trace << endl;
        return true;
}

bool MsiUtils::ExtractTo(
        PCWSTR         theDirectory,
        enumSelectAll  selectAll,
        enumFlatFolder flatFolder)
{
        if (delayLoading)
                return false;

        WCHAR buffer[MAX_PATH];
        DWORD result;
        result = GetFullPathName(theDirectory, MAX_PATH, buffer, NULL);
        if (result == 0)
        {
                result = GetLastError();
                trace_error << L"GetFullPathName(" << theDirectory << L") fails with " << result << endl;
                return false;
        }
        theDirectory = buffer;
        trace << L"Extract to: " << theDirectory << endl
              << endl;
        if (!VerifyDirectory(theDirectory))
                return false;

        targetRootDirectory = theDirectory;
        allSelected         = (selectAll == ALL_SELECTED);
        folderFlatten       = (flatFolder == EXTRACT_TO_FLAT_FOLDER);
        if (folderFlatten &&
            !VerifyDirectory(targetRootDirectory))
        {
                return false;
        }

        int i;
        for (i = 0; i < cabinet->count; i++)
        {
                cabinet->array[i].iterated = false;
        }

        for (i = 0; i < directory->count; i++)
        {
                directory->array[i].targetDirectoryVerified = false;
        }

        int countTodo   = 0;
        this->countDone = 0;
        for (i = 0; i < file->count; i++)
        {
                MsiFile::tagFile* p = &file->array[i];
                if (!allSelected && !p->selected)
                        continue;
                countTodo++;

                bool b = (p->compressed)
                        ? ExtractFile(i)
                        : CopyFile(i);
                if (!b)
                {
                        break;
                }
        }

        if (countTodo != countDone)
        {
                trace_error << (countTodo - countDone) << L" files are not extracted" << endl;
                return false;
        }
        return true;
}

bool MsiUtils::CopyFile(
        int index)
{
        MsiFile::tagFile*           p          = &file->array[index];
        MsiDirectory::tagDirectory* pDirectory = &directory->array[p->keyDirectory];

        wstring source = pDirectory->sourceDirectory + pathSeperator + p->filename;

        //
        // targetRootDirectory = c:\temp
        // pDirectory->targetDirectory = \Program Files\Orca\
        // p->filename = orca.exe
        //
        // target = c:\temp\Program Files\Orca\orca.exe
        //
        wstring target = (folderFlatten)
                                 ? (targetRootDirectory + pathSeperator + p->filename)
                                 : (targetRootDirectory + pathSeperator + pDirectory->targetDirectory + pathSeperator + p->filename);

        trace << source << endl
              << L"=> " << target << endl
              << endl;
        BOOL b = ::CopyFile(source.c_str(), target.c_str(), FALSE);
        if (b)
        {
                countDone++;
                return true;
        }
        else
        {
                DWORD result = GetLastError();
                trace_error << L"Copy file: " << source << L" -> " << target << L", failed with " << result << endl;
                return false;
        }
}

bool MsiUtils::ExtractFile(
        int index)
{
        MsiFile::tagFile*       pFile    = &file->array[index];
        MsiCabinet::tagCabinet* pCabinet = &cabinet->array[pFile->keyCabinet];

        if (pCabinet->iterated)
        {
                // already extracted
                return true;
        }
        pCabinet->iterated = true;

        wstring sourceCabinet;
        if (pCabinet->embedded)
        {
                if (!cabinet->Extract(pFile->keyCabinet))
                {
                        return false;
                }
                sourceCabinet = pCabinet->tempName;
                trace << L"Extract " << pCabinet->cabinet
                      << L" to " << pCabinet->tempName << endl;
        }
        else
        {
                sourceCabinet = sourceRootDirectory + pCabinet->cabinet;
                trace << L"cabinet: " << sourceCabinet << endl;
        }

        DWORD attributes = GetFileAttributes(sourceCabinet.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
                trace_error << L"GetFileAttributes(" << sourceCabinet << L") failed" << endl;
                return false;
        }

        SetupIterateCabinet(sourceCabinet.c_str(), 0, CabinetCallback, this);
        return true;
}

//
// the input format must be an full path (e.g. C:\path or \\server\share)
//
bool MsiUtils::VerifyDirectory(
        wstring s)
{
        CreateDirectory(s.c_str(), NULL);
        DWORD attributes = GetFileAttributes(s.c_str());
        if ((attributes != INVALID_FILE_ATTRIBUTES) && TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY))
                return true;

        if (s[s.length() - 1] != pathSeperator)
                s.append(1, pathSeperator);

        WCHAR buffer[MAX_PATH];
        wcscpy_s(buffer, MAX_PATH, s.c_str());

        wstring::size_type index = wstring::npos;
        if (s[1] == L':' && s[2] == pathSeperator)
        {
                // it is "C:\path\"
                index = s.find(pathSeperator, 3);
        }
        else if (s[0] == pathSeperator && s[1] == pathSeperator)
        {
                // it is a UNC path, "\\server\share\path\"
                index = s.find(pathSeperator, 2);
                index = s.find(pathSeperator, index + 1);
        }

        while (index != wstring::npos && index < MAX_PATH)
        {
                buffer[index] = L'\0';
                attributes    = GetFileAttributes(buffer);
                if (attributes == INVALID_FILE_ATTRIBUTES)
                {
                        CreateDirectory(buffer, NULL);
                        GetFileAttributes(buffer);
                }

                if (!TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY))
                        return false;
                buffer[index] = pathSeperator;

                index = s.find(pathSeperator, index + 1);
        }

        attributes = GetFileAttributes(s.c_str());
        return ((attributes != INVALID_FILE_ATTRIBUTES) && TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY));
}

UINT CALLBACK
MsiUtils::CabinetCallback(
        PVOID    context,
        UINT     notification,
        UINT_PTR param1,
        UINT_PTR /*param2*/
        )
{
        MsiUtils*             msiUtils    = (MsiUtils*)context;
        FILE_IN_CABINET_INFO* cabinetInfo = (FILE_IN_CABINET_INFO*)param1;

        switch (notification)
        {
        case SPFILENOTIFY_NEEDNEWCABINET:
                return ERROR_FILE_NOT_FOUND;

        case SPFILENOTIFY_FILEINCABINET:
                break;

        case SPFILENOTIFY_FILEEXTRACTED:
        // fall through
        default:
                return NO_ERROR;
        }

        int  index;
        bool located = msiUtils->LocateFile(cabinetInfo->NameInCabinet, &index);

        if (located &&
            (msiUtils->allSelected || msiUtils->file->array[index].selected))
        {
                MsiFile::tagFile* p = &msiUtils->file->array[index];

                wstring targetFilename;
                if (msiUtils->folderFlatten)
                {
                        targetFilename = msiUtils->targetRootDirectory + pathSeperator + p->filename;
                }
                else
                {
                        MsiDirectory::tagDirectory* d = &msiUtils->directory->array[p->keyDirectory];
                        if (!d->targetDirectoryVerified)
                        {
                                d->targetDirectoryVerified = true;
                                d->targetDirectoryExists   = VerifyDirectory(
                                        msiUtils->targetRootDirectory + pathSeperator + d->targetDirectory);
                        }
                        if (!d->targetDirectoryExists)
                                return FILEOP_SKIP;
                        targetFilename = msiUtils->targetRootDirectory + pathSeperator + d->targetDirectory + pathSeperator + p->filename;
                }

                wcscpy_s(cabinetInfo->FullTargetName, MAX_PATH, targetFilename.c_str());
                trace << L"... " << p->filename
                      << L"\t" << targetFilename << endl;

                msiUtils->countDone++;
                return FILEOP_DOIT;
        }

        return FILEOP_SKIP;
}

bool MsiUtils::LocateFile(
        wstring filename,
        int*    pIndex)
{
        for (int i = 0; i < file->count; i++)
                if (_wcsicmp(file->array[i].file.c_str(), filename.c_str()) == 0)
                {
                        *pIndex = i;
                        return true;
                }
        trace_error << L"File not found: " << filename << endl;
        return false;
}

bool MsiUtils::GetFileDetail(
        int                index,
        MsiDumpFileDetail* detail)
{
        if (delayLoading)
        {
                if (index < 0 || index > simpleFile->count)
                        return false;

                MsiSimpleFile::tagFile* p = &simpleFile->array[index];
                ZeroMemory(detail, sizeof(MsiDumpFileDetail));

                detail->filename = p->filename.c_str();
                detail->filesize = p->filesize;
                return true;
        }

        if (index < 0 || index > file->count)
        {
                trace_error << L"File index [" << index << L"] out of range: 0 - " << file->count << endl;
                return false;
        }

        MsiFile::tagFile* p = &file->array[index];
        detail->filename    = p->filename.c_str();
        detail->filesize    = p->filesize;
        detail->path        = directory->array[p->keyDirectory].targetDirectory.c_str();
        detail->win9x       = component->array[p->keyComponent].win9x;
        detail->winNT       = component->array[p->keyComponent].winNT;
        detail->winX64      = component->array[p->keyComponent].winX64;
        detail->selected    = p->selected;
        detail->version     = p->version.c_str();
        detail->language    = p->language.c_str();
        return true;
}

void MsiUtils::setSelected(
        int  index,
        bool select)
{
        if (delayLoading)
                return;

        if (index < 0 || index > file->count)
                return;

        file->array[index].selected = select;
}

int MsiUtils::getCount()
{
        return (IsOpened()
                        ? (delayLoading ? simpleFile->count : file->count)
                        : 0);
}

IMsiDumpCab*
MsiDumpCreateObject()
{
        MsiUtils* msiUtils = new MsiUtils();
        return (IMsiDumpCab*)msiUtils;
}

void __cdecl MsiUtils::threadLoadDatabase(void* parameter)
{
        MsiUtils* _this = (MsiUtils*)parameter;
        bool b;
        b = _this->LoadSummary();
        if (!b)
        {
                return;
        }
        b = _this->LoadDatabase();
        if (!b)
        {
                return;
        }
        _this->delayLoading = false;
        SetEvent(_this->delayEvent);
}

void MsiUtils::DelayLoadDatabase()
{
        simpleFile = new MsiSimpleFile(this);
        _beginthread(threadLoadDatabase, 0, this);
}
