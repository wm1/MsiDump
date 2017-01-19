#include "precomp.h"

MsiQuery::MsiQuery(
        MsiUtils* msiUtils,
        wstring   sql)
{
        ended  = true;
        record = NULL;

        if (!msiUtils->IsOpened())
                return;

        UINT r;
        r = MsiDatabaseOpenView(msiUtils->database, sql.c_str(), &view);
        if (r != ERROR_SUCCESS)
                return;

        r = MsiViewExecute(view, 0);
        if (r != ERROR_SUCCESS)
                return;

        ended = false;
}

MSIHANDLE
MsiQuery::Next()
{
        if (ended)
                return NULL;

        UINT r;
        r = MsiViewFetch(view, &record);
        if (r == ERROR_SUCCESS)
                return record;

        ended = true;
        return NULL;
}

template <class T>
MsiTable<T>::MsiTable(
        MsiUtils* theMsiUtils)
{
        msiUtils = theMsiUtils;
        array    = NULL;
        count    = 0;

        Init();
}

template <class T>
MsiTable<T>::~MsiTable()
{
        if (count != 0)
                delete[] array;
}

template <class T>
wstring
MsiTable<T>::getPrimaryKey()
{
        if (name == L"_Tables")
                return L"Name";

        if (name == L"_Columns")
                return L"Table";

        PMSIHANDLE record;
        UINT       r;
        wstring    s;
        r = MsiDatabaseGetPrimaryKeys(msiUtils->database, name.c_str(), &record);
        if (r != ERROR_SUCCESS)
                return s; // it is a empty string

        DWORD size = MAX_PATH;
        WCHAR buffer[MAX_PATH];
        MsiRecordGetString(record, 1, buffer, &size);
        s = buffer;

        return s;
}

template <class T>
int MsiTable<T>::CountRows()
{
        wstring primaryKey = getPrimaryKey();
        if (primaryKey.empty())
                primaryKey = L"*";

        wstring sql = L"SELECT " + primaryKey + L" FROM " + name;

        MsiQuery q(msiUtils, sql);
        int      row_count = 0;
        while (q.Next() != NULL)
                row_count++;

        return row_count;
}

template <>
void MsiFile::Init()
{
        name  = L"File";
        count = CountRows();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new tagFile[count];
        MsiQuery q(msiUtils, L"SELECT File, Component_, FileName, FileSize, Attributes, Sequence, Version, Language FROM File ORDER BY Sequence");

        for (tagFile* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                p->file = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 2, buffer, &size);
                p->component = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                // the format here is: "shortname | longname"
                PCWSTR verticalBar = wcschr(buffer, L'|');
                p->filename        = (verticalBar ? verticalBar + 1 : buffer);

                p->filesize   = MsiRecordGetInteger(record, 4);
                p->attributes = MsiRecordGetInteger(record, 5);
                p->sequence   = MsiRecordGetInteger(record, 6);

                size = MAX_PATH;
                MsiRecordGetString(record, 7, buffer, &size);
                p->version = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 8, buffer, &size);
                p->language = buffer;
        }
}

template <>
void MsiSimpleFile::Init()
{
        name  = L"File";
        count = CountRows();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new tagSimpleFile[count];
        MsiQuery q(msiUtils, L"SELECT FileName, FileSize FROM File ORDER BY Sequence");

        for (tagSimpleFile* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                // the format here is: "shortname | longname"
                PCWSTR verticalBar = wcschr(buffer, L'|');
                p->filename        = (verticalBar ? verticalBar + 1 : buffer);

                p->filesize = MsiRecordGetInteger(record, 2);
        }
}

template <>
void MsiComponent::Init()
{
        name  = L"Component";
        count = CountRows();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new tagComponent[count];
        MsiQuery q(msiUtils, L"SELECT Component, Directory_, Condition FROM Component");

        for (tagComponent* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                p->component = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 2, buffer, &size);
                p->directory = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                p->condition = buffer;
        }
}

template <>
void MsiDirectory::Init()
{
        name  = L"Directory";
        count = CountRows();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new tagDirectory[count];
        MsiQuery q(msiUtils, L"SELECT Directory FROM Directory");

        for (tagDirectory* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                p->directory = buffer;
        }
}

template <>
void MsiTable<tagCabinet>::Init()
{
        name  = L"Media";
        count = CountRows();

        if (msiUtils->db_type == MsiUtils::merge_module)
        {
                count           = 1;
                array           = new tagCabinet[count];
                tagCabinet* p   = array;
                p->diskId       = 0;
                p->lastSequence = 0;
                p->embedded     = true;
                p->cabinet      = L"MergeModule.CABinet";
                p->tempName.clear();
                return;
        }

        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new tagCabinet[count];
        MsiQuery q(msiUtils, L"SELECT DiskId, LastSequence, Cabinet FROM Media");

        for (tagCabinet* p = array; (record = q.Next()) != NULL; p++)
        {
                p->diskId       = MsiRecordGetInteger(record, 1);
                p->lastSequence = MsiRecordGetInteger(record, 2);

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                p->embedded = (buffer[0] == L'#');
                p->cabinet  = (p->embedded ? buffer + 1 : buffer);
                p->tempName.clear();
        }
}

MsiCabinet::~MsiCabinet()
{
        for (int i = 0; i < count; i++)
        {
                if (!array[i].tempName.empty())
                        DeleteFile(array[i].tempName.c_str());
        }
}

bool MsiCabinet::Extract(
        int index)
{
        tagCabinet* p = &array[index];
        if (!p->tempName.empty())
        {
                // already extracted
                return true;
        }

        WCHAR tempPath[MAX_PATH], tempFile[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        GetTempFileName(tempPath, L"cab", 0, tempFile);
        p->tempName = tempFile;

        wstring sql = L"SELECT Data FROM _Streams WHERE Name=\'";
        sql         = sql + p->cabinet + L'\'';
        MsiQuery  q(msiUtils, sql);
        MSIHANDLE record = q.Next();
        DWORD     size   = MsiRecordDataSize(record, 1);
        if (size == 0)
        {
                trace_error << L"Cabinet " << p->cabinet << L" size is zero" << endl;
                return false;
        }

        BYTE* buffer = new BYTE[size];
        MsiRecordReadStream(record, 1, (char*)buffer, &size);

        FILE*   file;
        errno_t e = _wfopen_s(&file, tempFile, L"wb");
        if (e != 0)
        {
                trace_error << "Trying to create temp file " << tempFile << L" failed with " << e << endl;
                return false;
        }
        fwrite(buffer, sizeof(BYTE), size, file);
        fclose(file);
        delete[] buffer;
        return true;
}

// Explicit Instantiation, or else uninstantiated template definitions are not put into .obj/.lib and link will fail
//
template class MsiTable<tagFile>;
template class MsiTable<tagSimpleFile>;
template class MsiTable<tagComponent>;
template class MsiTable<tagDirectory>;
template class MsiTable<tagCabinet>;
