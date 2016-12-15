
#pragma once

#include <windows.h>
#include <wtypes.h>
#include <process.h>
#include <string>
#include <fstream>
using namespace std;

#ifdef _UNICODE
#undef string
#define string wstring
#undef ofstream
#define ofstream wofstream
#endif

#include <msiquery.h>
#include <msidefs.h>
#include <setupapi.h>

#include "MsiDumpPublic.h"
#include "MsiTable.h"

#define MSISOURCE_COMPRESSED 0x00000002

#define TEST_FLAG(field, flag) (((field) & (flag)) != 0)
#define SET_FLAG(field, flag) ((field) |= (flag))
#define CLEAR_FLAG(field, flag) ((field) &= ~(flag))

extern ofstream trace;

extern "C" void __cdecl threadLoadDatabase(void* parameter);

class MsiUtils : public IMsiDumpCab
{
private:
        string    msiFilename;
        MSIHANDLE database;
        bool      compressed;
        bool      allSelected;
        bool      folderFlatten;
        string    targetRootDirectory;
        string    sourceRootDirectory;
        int       countDone;
        bool      delayLoading;
        HANDLE    delayEvent;
        enum
        {
                installer_database, // msi
                merge_module,       // msm
                transform_database, // mst
                patch_package       // msp
        } db_type;

        MsiFile*       file;
        MsiComponent*  component;
        MsiDirectory*  directory;
        MsiCabinet*    cabinet;
        MsiSimpleFile* simpleFile;

        MsiUtils();
        virtual ~MsiUtils();
        bool IsOpened() { return database != NULL; }
        bool LoadDatabase();
        void DelayLoadDatabase();
        void LoadSummary();
        void ExtractFile(int index);
        void CopyFile(int index);
        bool LocateFile(string, int* pIndex);
        static bool VerifyDirectory(string);
        static UINT CALLBACK CabinetCallback(PVOID, UINT, UINT_PTR, UINT_PTR);
        friend class MsiQuery;
        friend class MsiTable;
        friend class MsiCabinet;
        friend IMsiDumpCab* MsiDumpCreateObject();
        friend void __cdecl threadLoadDatabase(void* parameter);
        bool DoOpen(LPCWSTR filename);

public:
        void Release();
        bool Open(LPCWSTR filename)
        {
                delayLoading = false;
                delayEvent   = NULL;
                return DoOpen(filename);
        }
        bool DelayedOpen(LPCWSTR filename, HANDLE event)
        {
                delayLoading = true;
                delayEvent   = event;
                return DoOpen(filename);
        }
        void Close();
        bool ExtractTo(LPCWSTR theDirectory, enumSelectAll selectAll, enumFlatFolder flatFolder);

        int  getCount();
        void setSelected(int index, bool select);
        bool GetFileDetail(int index, MsiDumpFileDetail* detail);
};
