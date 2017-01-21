#include "precomp.h"

wofstream trace;

PCSTR trace_file =

#if ENABLE_TRACE == 1
        "trace.txt"
#else
        "nul"
#endif
        ;

////////////////////////////////////////////////////////////////////////
WCHAR path_seperator = L'\\';

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
        PCWSTR file_name)
{
        Close();
        PWSTR file_part;
        WCHAR buffer[MAX_PATH];
        DWORD result = GetFullPathName(file_name, MAX_PATH, buffer, &file_part);
        if (result == 0 || !file_part)
        {
                DWORD error = GetLastError();
                trace_error << L"GetFullPathName failed with: " << error << endl;
                return false;
        }
        DWORD file_attributes = GetFileAttributes(buffer);
        if ((file_attributes == INVALID_FILE_ATTRIBUTES) || TEST_FLAG(file_attributes, FILE_ATTRIBUTE_DEVICE) || TEST_FLAG(file_attributes, FILE_ATTRIBUTE_DIRECTORY))
        {
                trace_error << buffer << L" is not a normal file" << endl;
                return false;
        }

        msi_file_name = buffer;

        PCWSTR file_extension = wcsrchr(buffer, L'.');
        if (!file_extension)
        {
                trace_error << buffer << L" does not have any file extension" << endl;
                return false;
        }

        if (_wcsicmp(file_extension, L".msi") == 0)
                db_type = installer_database;
        else if (_wcsicmp(file_extension, L".msm") == 0)
                db_type = merge_module;
        else
        {
                trace_error << buffer << L" does not have a recognized file extension" << endl;
                return false;
        }

        // source_root_directory now contains the trailing path separator
        //
        *file_part            = L'\0';
        source_root_directory = buffer;
        trace << L"******************************" << endl
              << msi_file_name << endl
              << source_root_directory << endl
              << endl;

        UINT r = MsiOpenDatabase(msi_file_name.c_str(), MSIDBOPEN_READONLY, &database);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"failed to open msi file, err = " << r << endl;
                database = NULL;
                return false;
        }

        if (is_delay_loading == false)
        {
                simple_files = NULL;
                bool b       = LoadSummary();
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
                cabinets    = NULL;
                directories = NULL;
                components  = NULL;
                files       = NULL;
                DelayLoadDatabase();
        }
        return true;
}

void MsiUtils::Close()
{
        if (!IsOpened())
                return;

        database = NULL;

        if (cabinets)
        {
                delete cabinets;
                cabinets = NULL;
        }
        if (directories)
        {
                delete directories;
                directories = NULL;
        }
        if (components)
        {
                delete components;
                components = NULL;
        }
        if (files)
        {
                delete files;
                files = NULL;
        }
        if (simple_files)
        {
                delete simple_files;
                simple_files = NULL;
        }
}

bool MsiUtils::LoadSummary()
{
        PMSIHANDLE summary;
        UINT       data_type;
        INT        data;
        FILETIME   file_time;
        DWORD      size = MAX_PATH;
        WCHAR      buffer[MAX_PATH];
        UINT       r;

        is_compressed = false;
        r             = MsiGetSummaryInformation(database, 0, 0, &summary);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"MsiGetSummaryInformation failed with " << r << endl;
                return false;
        }

        r = MsiSummaryInfoGetProperty(summary, PID_MSISOURCE,
                                      &data_type, &data, &file_time, buffer, &size);

        if (r == ERROR_SUCCESS && data_type == VT_I4)
                is_compressed = TEST_FLAG(data, MSISOURCE_COMPRESSED);

        if (db_type == merge_module)
                is_compressed = true;

        trace << L"compressed: " << is_compressed << endl
              << endl;

        return true;
}

bool MsiUtils::LoadDatabase()
{
        cabinets    = new MsiCabinets(this);
        directories = new MsiDirectories(this);
        components  = new MsiComponents(this);
        files       = new MsiFiles(this);
        UINT r;

        int i, j;
        trace << L"Cabinet count = " << cabinets->count << endl
              << endl;
        for (i = 0; i < cabinets->count; i++)
        {
                TagCabinet* cabinet = &cabinets->array[i];
                trace << i << L"[last seq = " << cabinet->last_sequence << L"]: "
                      << cabinet->cabinet << endl;
        }
        trace << endl;

        PMSIHANDLE product;
        WCHAR      package_name[30];
        DWORD      size = MAX_PATH;
        WCHAR      buffer[MAX_PATH];
        swprintf_s(package_name, 30, L"#%d", (int)database);
        r = MsiOpenPackageEx(package_name, MSIOPENPACKAGEFLAGS_IGNOREMACHINESTATE, &product);
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"MsiOpenPackageEx(" << package_name << L") failed with " << r << endl;
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
                return false;
        }
        r = MsiDoAction(product, L"FileCost");
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"FileCost failed with " << r << endl;
                return false;
        }
        r = MsiDoAction(product, L"CostFinalize");
        if (r != ERROR_SUCCESS)
        {
                trace_error << L"CostFinalize failed with " << r << endl;
                return false;
        }

        trace << L"Directory count = " << directories->count << endl
              << endl;
        for (i = 0; i < directories->count; i++)
        {
                TagDirectory* direcotry = &directories->array[i];

                size = MAX_PATH;
                r    = MsiGetSourcePath(product, direcotry->directory.c_str(), buffer, &size);
                if (r != ERROR_SUCCESS)
                {
                        trace_error << L"MsiGetSourcePath(" << direcotry->directory.c_str() << ") failed with " << r << endl;
                        return false;
                }
                direcotry->source_directory = buffer;

                size = MAX_PATH;
                r    = MsiGetTargetPath(product, direcotry->directory.c_str(), buffer, &size);
                if (r != ERROR_SUCCESS)
                {
                        trace_error << L"MsiGetTargetPath(" << direcotry->directory.c_str() << ") failed with " << r << endl;
                        return false;
                }
                direcotry->target_directory = &buffer[2]; // skip drive letter part of C:\xxxx

                trace << i << L": " << direcotry->directory << endl
                      << L"    " << direcotry->source_directory << endl
                      << L"    " << direcotry->target_directory << endl;
        }
        trace << endl;

        trace << L"Component count = " << components->count << endl
              << endl;
        for (i = 0; i < components->count; i++)
        {
                TagComponent* component = &components->array[i];
                for (j = 0; j < directories->count; j++)
                {
                        TagDirectory* directory = &directories->array[j];
                        if (component->directory == directory->directory)
                        {
                                component->key_directory = j;
                                break;
                        }
                }

                component->win_9x  = true;
                component->win_nt  = true;
                component->win_x64 = false;
                if (!component->condition.empty())
                {
                        // NOTE: there is no common rule to define which architecture the msi
                        // package or individual files are built for. Therefore what we're
                        // doing here is guess at our best effort
                        //
                        PCWSTR condition = component->condition.c_str();

                        MsiSetProperty(product, L"Version9X", L"490");
                        MsiSetProperty(product, L"VersionNT", L"");
                        MsiSetProperty(product, L"VersionNT64", L"");
                        component->win_9x = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);

                        MsiSetProperty(product, L"Version9X", L"");
                        MsiSetProperty(product, L"VersionNT", L"502");
                        MsiSetProperty(product, L"VersionNT64", L"");
                        component->win_nt = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);

                        MsiSetProperty(product, L"Version9X", L"");
                        MsiSetProperty(product, L"VersionNT", L"");
                        MsiSetProperty(product, L"VersionNT64", L"1");
                        bool x64Pos, x64Neg;
                        x64Pos = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
                        MsiSetProperty(product, L"VersionNT64", L"");
                        x64Neg             = (MsiEvaluateCondition(product, condition) == MSICONDITION_TRUE);
                        component->win_x64 = (x64Pos == true && x64Neg == false);
                }

                trace << i << L"[dir = " << component->key_directory;
                if (!component->condition.empty())
                        trace << L", condition = " << component->condition;
                trace << L"]: " << component->component << endl;
        }
        trace << endl;

        trace << L"File count = " << files->count << endl
              << endl;
        for (i = 0; i < files->count; i++)
        {
                TagFile* file = &files->array[i];
                for (j = 0; j < components->count; j++)
                {
                        TagComponent* component = &components->array[j];
                        if (file->component == component->component)
                        {
                                file->key_directory = component->key_directory;
                                file->key_component = j;
                                break;
                        }
                }

                if (db_type == merge_module)
                        file->key_cabinet = 0;
                else
                {
                        for (j = 0; j < cabinets->count; j++)
                        {
                                TagCabinet* cabinet = &cabinets->array[j];
                                if (file->sequence <= cabinet->last_sequence)
                                {
                                        file->key_cabinet = j;
                                        break;
                                }
                        }
                }

                file->is_selected   = false;
                file->is_compressed = is_compressed;
                if (file->attributes & msidbFileAttributesCompressed)
                        file->is_compressed = true;
                if (file->attributes & msidbFileAttributesNoncompressed)
                        file->is_compressed = false;

                trace << file->sequence << L"[comp = " << file->key_component
                      << L", dir = " << file->key_directory;
                if (file->is_compressed)
                        trace << L", cab = " << file->key_cabinet;
                trace << L"]: " << file->file_name << endl;
        }
        trace << endl;
        return true;
}

bool MsiUtils::ExtractTo(
        PCWSTR          directory_name,
        EnumSelectItems select_items_,
        EnumExtractTo   extract_to_)
{
        if (is_delay_loading)
                return false;

        WCHAR buffer[MAX_PATH];
        DWORD result;
        result = GetFullPathName(directory_name, MAX_PATH, buffer, NULL);
        if (result == 0)
        {
                result = GetLastError();
                trace_error << L"GetFullPathName(" << directory_name << L") fails with " << result << endl;
                return false;
        }
        directory_name = buffer;
        trace << L"Extract to: " << directory_name << endl
              << endl;
        if (!VerifyDirectory(directory_name))
                return false;

        target_root_directory = directory_name;
        select_items          = select_items_;
        extract_to            = extract_to_;

        if ((extract_to == EXTRACT_TO_FLAT_FOLDER) &&
            !VerifyDirectory(target_root_directory))
        {
                return false;
        }

        int i;
        for (i = 0; i < cabinets->count; i++)
        {
                cabinets->array[i].iterated = false;
        }

        for (i = 0; i < directories->count; i++)
        {
                directories->array[i].is_target_verified = false;
        }

        int files_to_be_extracted = 0;
        this->files_extracted     = 0;
        for (i = 0; i < files->count; i++)
        {
                TagFile* file = &files->array[i];
                if ((select_items == SELECT_INDIVIDUAL_ITEMS) && !file->is_selected)
                        continue;
                files_to_be_extracted++;

                bool b = (file->is_compressed)
                                 ? ExtractFile(i)
                                 : CopyFile(i);
                if (!b)
                {
                        break;
                }
        }

        if (files_to_be_extracted != files_extracted)
        {
                trace_error << (files_to_be_extracted - files_extracted) << L" files are not extracted" << endl;
                return false;
        }
        return true;
}

bool MsiUtils::CopyFile(
        int index)
{
        TagFile*      file      = &files->array[index];
        TagDirectory* directory = &directories->array[file->key_directory];

        wstring source = directory->source_directory + path_seperator + file->file_name;

        wstring target = (extract_to == EXTRACT_TO_FLAT_FOLDER)
                                 ? (target_root_directory + path_seperator + file->file_name)
                                 : (target_root_directory + path_seperator + directory->target_directory + path_seperator + file->file_name);

        trace << source << endl
              << L"=> " << target << endl
              << endl;
        BOOL b = ::CopyFile(source.c_str(), target.c_str(), FALSE);
        if (b)
        {
                files_extracted++;
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
        TagFile*    file    = &files->array[index];
        TagCabinet* cabinet = &cabinets->array[file->key_cabinet];

        if (cabinet->iterated)
        {
                // already extracted
                return true;
        }
        cabinet->iterated = true;

        wstring source_cabinet;
        if (cabinet->is_embedded)
        {
                if (!cabinets->Extract(file->key_cabinet))
                {
                        return false;
                }
                source_cabinet = cabinet->extracted_name;
                trace << L"Extract " << cabinet->cabinet
                      << L" to " << cabinet->extracted_name << endl;
        }
        else
        {
                source_cabinet = source_root_directory + cabinet->cabinet;
                trace << L"cabinet: " << source_cabinet << endl;
        }

        DWORD attributes = GetFileAttributes(source_cabinet.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
                trace_error << L"GetFileAttributes(" << source_cabinet << L") failed" << endl;
                return false;
        }

        SetupIterateCabinet(source_cabinet.c_str(), 0, CabinetCallback, this);
        return true;
}

//
// the input format must be an full path (e.g. C:\path or \\server\share)
//
bool MsiUtils::VerifyDirectory(
        wstring path)
{
        CreateDirectory(path.c_str(), NULL);
        DWORD attributes = GetFileAttributes(path.c_str());
        if ((attributes != INVALID_FILE_ATTRIBUTES) && TEST_FLAG(attributes, FILE_ATTRIBUTE_DIRECTORY))
                return true;

        if (path[path.length() - 1] != path_seperator)
                path.append(1, path_seperator);

        WCHAR buffer[MAX_PATH];
        wcscpy_s(buffer, MAX_PATH, path.c_str());

        size_t index = wstring::npos;
        if (path[1] == L':' && path[2] == path_seperator)
        {
                // it is "C:\path\"
                index = path.find(path_seperator, 3);
        }
        else if (path[0] == path_seperator && path[1] == path_seperator)
        {
                // it is a UNC path, "\\server\share\path\"
                index = path.find(path_seperator, 2);
                index = path.find(path_seperator, index + 1);
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
                buffer[index] = path_seperator;

                index = path.find(path_seperator, index + 1);
        }

        attributes = GetFileAttributes(path.c_str());
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
        MsiUtils*             msi  = (MsiUtils*)context;
        FILE_IN_CABINET_INFO* info = (FILE_IN_CABINET_INFO*)param1;

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
        bool located = msi->LocateFile(info->NameInCabinet, &index);

        if (located &&
            ((msi->select_items == SELECT_ALL_ITEMS) || msi->files->array[index].is_selected))
        {
                TagFile* file = &msi->files->array[index];

                wstring target_file_name;
                if (msi->extract_to == EXTRACT_TO_FLAT_FOLDER)
                {
                        target_file_name = msi->target_root_directory + path_seperator + file->file_name;
                }
                else
                {
                        TagDirectory* directory = &msi->directories->array[file->key_directory];
                        if (!directory->is_target_verified)
                        {
                                directory->is_target_verified = true;
                                directory->is_target_existing = VerifyDirectory(
                                        msi->target_root_directory + path_seperator + directory->target_directory);
                        }
                        if (!directory->is_target_existing)
                                return FILEOP_SKIP;
                        target_file_name = msi->target_root_directory + path_seperator + directory->target_directory + path_seperator + file->file_name;
                }

                wcscpy_s(info->FullTargetName, MAX_PATH, target_file_name.c_str());
                trace << L"... " << file->file_name
                      << L"\t" << target_file_name << endl;

                msi->files_extracted++;
                return FILEOP_DOIT;
        }

        return FILEOP_SKIP;
}

bool MsiUtils::LocateFile(
        wstring file_name,
        int*    index)
{
        for (int i = 0; i < files->count; i++)
                if (_wcsicmp(files->array[i].file.c_str(), file_name.c_str()) == 0)
                {
                        *index = i;
                        return true;
                }

        trace_error << L"File not found: " << file_name << endl;
        return false;
}

bool MsiUtils::GetFileDetail(
        int                index,
        MsiDumpFileDetail* detail)
{
        if (is_delay_loading)
        {
                if (index < 0 || index >= simple_files->count)
                        return false;

                TagSimpleFile* simple_file = &simple_files->array[index];
                ZeroMemory(detail, sizeof(MsiDumpFileDetail));

                detail->file_name = simple_file->file_name.c_str();
                detail->file_size = simple_file->file_size;
                return true;
        }

        if (index < 0 || index >= files->count)
        {
                trace_error << L"File index [" << index << L"] out of range: 0 - " << files->count << endl;
                return false;
        }

        TagFile*      file      = &files->array[index];
        TagDirectory* directory = &directories->array[file->key_directory];
        TagComponent* component = &components->array[file->key_component];

        detail->file_name   = file->file_name.c_str();
        detail->file_size   = file->file_size;
        detail->path        = directory->target_directory.c_str();
        detail->win_9x      = component->win_9x;
        detail->win_nt      = component->win_nt;
        detail->win_x64     = component->win_x64;
        detail->is_selected = file->is_selected;
        detail->version     = file->version.c_str();
        detail->language    = file->language.c_str();
        return true;
}

void MsiUtils::SelectFile(
        int  index,
        bool select)
{
        if (is_delay_loading)
                return;

        if (index < 0 || index > files->count)
                return;

        files->array[index].is_selected = select;
}

int MsiUtils::GetFileCount()
{
        return (IsOpened()
                        ? (is_delay_loading ? simple_files->count : files->count)
                        : 0);
}

IMsiDumpCab*
MsiDumpCreateObject()
{
        MsiUtils* msi = new MsiUtils();
        return (IMsiDumpCab*)msi;
}

void __cdecl MsiUtils::ThreadLoadDatabase(void* parameter)
{
        MsiUtils* _this = (MsiUtils*)parameter;
        bool      b;

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

        _this->is_delay_loading = false;
        SetEvent(_this->delay_event);
}

void MsiUtils::DelayLoadDatabase()
{
        simple_files = new MsiSimpleFiles(this);
        _beginthread(ThreadLoadDatabase, 0, this);
}
