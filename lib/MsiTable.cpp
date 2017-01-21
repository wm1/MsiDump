#include "precomp.h"

MsiQuery::MsiQuery(
        MsiUtils* msi,
        wstring   sql)
{
        is_end = true;
        record = NULL;

        if (!msi->IsOpened())
                return;

        UINT r = MsiDatabaseOpenView(msi->database, sql.c_str(), &view);
        if (r != ERROR_SUCCESS)
                return;

        r = MsiViewExecute(view, 0);
        if (r != ERROR_SUCCESS)
                return;

        is_end = false;
}

MSIHANDLE
MsiQuery::Next()
{
        if (is_end)
                return NULL;

        UINT r = MsiViewFetch(view, &record);
        if (r == ERROR_SUCCESS)
                return record;

        is_end = true;
        return NULL;
}

template <class T>
MsiTable<T>::MsiTable(
        MsiUtils* _msi)
{
        msi   = _msi;
        array = NULL;
        count = 0;

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
MsiTable<T>::GetPrimaryKey()
{
        // MSDN says MsiDatabaseGetPrimaryKeys is not valid for the following two tables,
        // therefore we provide the hard-coded answer for them
        //
        if (table_name == L"_Tables")
                return L"Name";

        if (table_name == L"_Columns")
                return L"Table";

        PMSIHANDLE record;
        wstring    s;
        UINT       r = MsiDatabaseGetPrimaryKeys(msi->database, table_name.c_str(), &record);
        if (r != ERROR_SUCCESS)
                return s; // it is a empty string

        DWORD size = MAX_PATH;
        WCHAR buffer[MAX_PATH];
        r = MsiRecordGetString(record, 1, buffer, &size);
        if (r != ERROR_SUCCESS)
                return s;

        s = buffer;
        return s;
}

template <class T>
int MsiTable<T>::GetRowCount()
{
        wstring primary_key = GetPrimaryKey();
        if (primary_key.empty())
                primary_key = L"*";

        wstring sql = L"SELECT " + primary_key + L" FROM " + table_name;

        MsiQuery query(msi, sql);
        int      row_count = 0;
        while (query.Next() != NULL)
                row_count++;

        return row_count;
}

template <>
void MsiFiles::Init()
{
        table_name = L"File";
        count      = GetRowCount();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new TagFile[count];
        MsiQuery query(msi, L"SELECT File, Component_, FileName, FileSize, Attributes, Sequence, Version, Language FROM File ORDER BY Sequence");

        for (TagFile* file = array; (record = query.Next()) != NULL; file++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                file->file = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 2, buffer, &size);
                file->component = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                // the format here is: "shortname | longname"
                PCWSTR verticalBar = wcschr(buffer, L'|');
                file->file_name    = (verticalBar ? verticalBar + 1 : buffer);

                file->file_size  = MsiRecordGetInteger(record, 4);
                file->attributes = MsiRecordGetInteger(record, 5);
                file->sequence   = MsiRecordGetInteger(record, 6);

                size = MAX_PATH;
                MsiRecordGetString(record, 7, buffer, &size);
                file->version = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 8, buffer, &size);
                file->language = buffer;
        }
}

template <>
void MsiSimpleFiles::Init()
{
        table_name = L"File";
        count      = GetRowCount();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new TagSimpleFile[count];
        MsiQuery query(msi, L"SELECT FileName, FileSize FROM File ORDER BY Sequence");

        for (TagSimpleFile* simple_file = array; (record = query.Next()) != NULL; simple_file++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                // the format here is: "shortname | longname"
                PCWSTR verticalBar     = wcschr(buffer, L'|');
                simple_file->file_name = (verticalBar ? verticalBar + 1 : buffer);
                simple_file->file_size = MsiRecordGetInteger(record, 2);
        }
}

template <>
void MsiComponents::Init()
{
        table_name = L"Component";
        count      = GetRowCount();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new TagComponent[count];
        MsiQuery query(msi, L"SELECT Component, Directory_, Condition FROM Component");

        for (TagComponent* component = array; (record = query.Next()) != NULL; component++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                component->component = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 2, buffer, &size);
                component->directory = buffer;

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                component->condition = buffer;
        }
}

template <>
void MsiDirectories::Init()
{
        table_name = L"Directory";
        count      = GetRowCount();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new TagDirectory[count];
        MsiQuery query(msi, L"SELECT Directory FROM Directory");

        for (TagDirectory* directory = array; (record = query.Next()) != NULL; directory++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                directory->directory = buffer;
        }
}

template <>
void MsiTable<TagCabinet>::Init()
{
        table_name = L"Media";
        if (msi->db_type == MsiUtils::merge_module)
        {
                count                  = 1;
                array                  = new TagCabinet[count];
                TagCabinet* cabinet    = array;
                cabinet->last_sequence = 0;
                cabinet->is_embedded   = true;
                cabinet->cabinet       = L"MergeModule.CABinet";
                cabinet->extracted_name.clear();
                return;
        }

        count = GetRowCount();
        if (count == 0)
                return;

        MSIHANDLE record;
        DWORD     size = MAX_PATH;
        WCHAR     buffer[MAX_PATH];
        array = new TagCabinet[count];
        MsiQuery query(msi, L"SELECT LastSequence, Cabinet FROM Media");

        for (TagCabinet* cabinet = array; (record = query.Next()) != NULL; cabinet++)
        {
                cabinet->last_sequence = MsiRecordGetInteger(record, 1);

                size = MAX_PATH;
                MsiRecordGetString(record, 2, buffer, &size);
                cabinet->is_embedded = (buffer[0] == L'#');
                cabinet->cabinet     = (cabinet->is_embedded ? buffer + 1 : buffer);
                cabinet->extracted_name.clear();
        }
}

MsiCabinets::~MsiCabinets()
{
        for (int i = 0; i < count; i++)
        {
                if (!array[i].extracted_name.empty())
                        DeleteFile(array[i].extracted_name.c_str());
        }
}

bool MsiCabinets::Extract(
        int index)
{
        TagCabinet* cabinet = &array[index];
        if (!cabinet->extracted_name.empty())
        {
                // already extracted
                return true;
        }

        WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
        GetTempPath(MAX_PATH, temp_path);
        GetTempFileName(temp_path, L"cab", 0, temp_file);
        cabinet->extracted_name = temp_file;

        wstring sql = L"SELECT Data FROM _Streams WHERE Name=\'";
        sql         = sql + cabinet->cabinet + L'\'';
        MsiQuery  query(msi, sql);
        MSIHANDLE record = query.Next();
        DWORD     size   = MsiRecordDataSize(record, 1);
        if (size == 0)
        {
                trace_error << L"Cabinet " << cabinet->cabinet << L" size is zero" << endl;
                return false;
        }

        BYTE* buffer = new BYTE[size];
        MsiRecordReadStream(record, 1, (char*)buffer, &size);

        FILE*   file;
        errno_t e = _wfopen_s(&file, temp_file, L"wb");
        if (e != 0)
        {
                trace_error << "Trying to create temp file " << temp_file << L" failed with " << e << endl;
                return false;
        }
        fwrite(buffer, sizeof(BYTE), size, file);
        fclose(file);
        delete[] buffer;
        return true;
}

// Explicit Instantiation, or else uninstantiated template definitions are not put into .obj/.lib and link will fail
//
template class MsiTable<TagFile>;
template class MsiTable<TagSimpleFile>;
template class MsiTable<TagComponent>;
template class MsiTable<TagDirectory>;
template class MsiTable<TagCabinet>;
