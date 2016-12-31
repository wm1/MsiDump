
#pragma once

class MsiUtils : public IMsiDumpCab
{
private:
        wstring   msiFilename;
        MSIHANDLE database;
        bool      compressed;
        bool      allSelected;
        bool      folderFlatten;
        wstring   targetRootDirectory;
        wstring   sourceRootDirectory;
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
        bool LoadSummary();
        bool ExtractFile(int index);
        bool CopyFile(int index);
        bool LocateFile(wstring, int* pIndex);
        static bool VerifyDirectory(wstring);
        static UINT CALLBACK CabinetCallback(PVOID, UINT, UINT_PTR, UINT_PTR);
        friend class MsiQuery;
        friend class MsiTable;
        friend class MsiCabinet;
        friend IMsiDumpCab* MsiDumpCreateObject();
        static void __cdecl threadLoadDatabase(void* parameter);
        bool DoOpen(PCWSTR filename);

public:
        void Release();
        bool Open(PCWSTR filename)
        {
                delayLoading = false;
                delayEvent   = NULL;
                return DoOpen(filename);
        }
        bool DelayedOpen(PCWSTR filename, HANDLE event)
        {
                delayLoading = true;
                delayEvent   = event;
                return DoOpen(filename);
        }
        void Close();
        bool ExtractTo(PCWSTR theDirectory, enumSelectAll selectAll, enumFlatFolder flatFolder);

        int  getCount();
        void setSelected(int index, bool select);
        bool GetFileDetail(int index, MsiDumpFileDetail* detail);
};
