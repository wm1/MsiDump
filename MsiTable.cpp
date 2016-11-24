
#include "MsiUtils.h"

////////////////////////////////////////////////////////////////////////

/*

Here is the original implementation of MsiQuery:

void
FOREACH(
        LPCTSTR sql,
        foreach pFunc,
        void   *param
        )
{
        MSIHANDLE view, record;
        MsiDatabaseOpenView(database, sql, &view);
        MsiViewExecute(view, 0);
        while(MsiViewFetch(view, &record) != ERROR_NO_MORE_ITEMS)
        {
                (*pFunc)(record, param);
                MsiCloseHandle(record);
        }
        MsiCloseHandle(view);
}
*/

MsiQuery::MsiQuery(
        MsiUtils *msiUtils,
        string    sql
        )
{
        viewCreated = false;
        ended       = true;
        record      = NULL;

        if(!msiUtils->IsOpened())
                return;

        UINT r;
        r = MsiDatabaseOpenView(msiUtils->database, sql.c_str(), &view);
        if(r != ERROR_SUCCESS)
                return;

        r = MsiViewExecute(view, 0);
        if(r != ERROR_SUCCESS)
        {
                MsiCloseHandle(view);
                return;
        }

        ended       = false;
        viewCreated = true;
}

MsiQuery::~MsiQuery()
{
        if(viewCreated)
                MsiCloseHandle(view);
}

MSIHANDLE
MsiQuery::Next()
{
        if(ended)
                return NULL;

        MsiCloseHandle(record);

        UINT r;
        r = MsiViewFetch(view, &record);
        if(r == ERROR_SUCCESS)
                return record;

        ended = true;
        return NULL;
}

////////////////////////////////////////////////////////////////////////

MsiTable::MsiTable(
        MsiUtils *theMsiUtils,
        string    tableName
        )
{
        msiUtils = theMsiUtils;
        name     = tableName;
        count    = CountRows();
}

MsiTable::~MsiTable()
{
}

string
MsiTable::getPrimaryKey()
{
        if(name == TEXT("_Tables"))
                return TEXT("Name");

        if(name == TEXT("_Columns"))
                return TEXT("Table");

        MSIHANDLE record;
        UINT r;
        string s;
        r = MsiDatabaseGetPrimaryKeys(msiUtils->database, name.c_str(), &record);
        if(r != ERROR_SUCCESS)
                return s;  // it is a empty string

        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        MsiRecordGetString(record, 1, buffer, &size);
        MsiCloseHandle(record);
        s = buffer;

        return s;
}

int
MsiTable::CountRows()
{
        string primaryKey = getPrimaryKey();
        if(primaryKey.empty())
                primaryKey = TEXT("*");

        string sql = TEXT("SELECT ") + primaryKey + TEXT(" FROM ") + name;

        MsiQuery q(msiUtils, sql);
        int count = 0;
        while(q.Next() != NULL)
                count++;

        return count;
}

////////////////////////////////////////////////////////////////////////

MsiFile::MsiFile(
        MsiUtils *msiUtils
        )
        : MsiTable(msiUtils, TEXT("File"))
{
        if(count == 0) return;

        MSIHANDLE record;
        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        array = new tagFile[count];
        MsiQuery q(msiUtils, TEXT("SELECT File, Component_, FileName, FileSize, Attributes, Sequence, Version, Language FROM File ORDER BY Sequence"));

        for(tagFile* p = array; (record = q.Next()) != NULL; p++)
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
                LPCTSTR verticalBar = _tcschr(buffer, TEXT('|'));
                p->filename = (verticalBar ? verticalBar + 1 : buffer);

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

MsiFile::~MsiFile()
{
        if(count != 0)
                delete[] array;
}

////////////////////////////////////////////////////////////////////////

MsiSimpleFile::MsiSimpleFile(
        MsiUtils *msiUtils
        )
        : MsiTable(msiUtils, TEXT("File"))
{
        if(count == 0) return;

        MSIHANDLE record;
        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        array = new tagFile[count];
        MsiQuery q(msiUtils, TEXT("SELECT FileName, FileSize FROM File ORDER BY Sequence"));

        for(tagFile* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                // the format here is: "shortname | longname"
                LPCTSTR verticalBar = _tcschr(buffer, TEXT('|'));
                p->filename = (verticalBar ? verticalBar + 1 : buffer);

                p->filesize   = MsiRecordGetInteger(record, 2);
        }
}

MsiSimpleFile::~MsiSimpleFile()
{
        if(count != 0)
                delete[] array;
}

////////////////////////////////////////////////////////////////////////

MsiComponent::MsiComponent(
        MsiUtils *msiUtils
        )
        : MsiTable(msiUtils, TEXT("Component"))
{
        if(count == 0) return;

        MSIHANDLE record;
        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        array = new tagComponent[count];
        MsiQuery q(msiUtils, TEXT("SELECT Component, Directory_, Condition FROM Component"));

        for(tagComponent* p = array; (record = q.Next()) != NULL; p++)
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

MsiComponent::~MsiComponent()
{
        if(count != 0)
                delete[] array;
}

////////////////////////////////////////////////////////////////////////

MsiDirectory::MsiDirectory(
        MsiUtils *msiUtils
        )
        : MsiTable(msiUtils, TEXT("Directory"))
{
        if(count == 0) return;

        MSIHANDLE record;
        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        array = new tagDirectory[count];
        MsiQuery q(msiUtils, TEXT("SELECT Directory FROM Directory"));

        for(tagDirectory* p = array; (record = q.Next()) != NULL; p++)
        {
                size = MAX_PATH;
                MsiRecordGetString(record, 1, buffer, &size);
                p->directory = buffer;
        }
}

MsiDirectory::~MsiDirectory()
{
        if(count != 0)
                delete[] array;
}

////////////////////////////////////////////////////////////////////////

MsiCabinet::MsiCabinet(
        MsiUtils *msiUtils
        )
        : MsiTable(msiUtils, TEXT("Media"))
{
        if(msiUtils->db_type == MsiUtils::merge_module)
        {
                count = 1;
                array = new tagCabinet[count];
                tagCabinet* p = array;
                p->diskId = 0;
                p->lastSequence = 0;
                p->embedded = true;
                p->cabinet = TEXT("MergeModule.CABinet");
                p->tempName.erase();
                return;
        }

        if(count == 0) return;

        MSIHANDLE record;
        DWORD size = MAX_PATH;
        TCHAR buffer[MAX_PATH];
        array = new tagCabinet[count];
        MsiQuery q(msiUtils, TEXT("SELECT DiskId, LastSequence, Cabinet FROM Media"));

        for(tagCabinet* p = array; (record = q.Next()) != NULL; p++)
        {
                p->diskId       = MsiRecordGetInteger(record, 1);
                p->lastSequence = MsiRecordGetInteger(record, 2);

                size = MAX_PATH;
                MsiRecordGetString(record, 3, buffer, &size);
                p->embedded = (buffer[0] == TEXT('#'));
                p->cabinet = (p->embedded ? buffer+1 : buffer);

                // w2k3 ddk stl does not support basic_string::clear(). use erase instead
                p->tempName.erase();
        }
}

MsiCabinet::~MsiCabinet()
{
        if(count == 0)
                return;

        for(int i=0; i<count; i++)
        {
                if(!array[i].tempName.empty())
                        DeleteFile(array[i].tempName.c_str());
        }
        delete[] array;
}

void
MsiCabinet::Extract(
        int index
        )
{
        tagCabinet *p = &array[index];
        if(!p->tempName.empty())
                return;

        TCHAR tempPath[MAX_PATH], tempFile[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        GetTempFileName(tempPath, TEXT("cab"), 0, tempFile);
        p->tempName = tempFile;

        string sql = TEXT("SELECT Data FROM _Streams WHERE Name=\'");;
        sql = sql + p->cabinet + TEXT('\'');
        MsiQuery  q(msiUtils, sql);
        MSIHANDLE record = q.Next();
        DWORD     size   = MsiRecordDataSize(record, 1);
        if(size == 0) return;

        BYTE     *buffer = new BYTE[size];
        MsiRecordReadStream(record, 1, (char*)buffer, &size);

        FILE* file  = _tfopen(tempFile, TEXT("wb"));
        fwrite(buffer, sizeof(BYTE), size, file);
        fclose(file);
        delete[] buffer;

}

////////////////////////////////////////////////////////////////////////
