
#pragma once

class MsiUtils : public IMsiDumpCab
{
private:
        wstring         msi_file_name;
        PMSIHANDLE      database;
        bool            is_compressed;
        EnumSelectItems select_items;
        EnumExtractTo   extract_to;
        wstring         target_root_directory;
        wstring         source_root_directory;
        int             files_extracted;
        bool            is_delay_loading;
        HANDLE          delay_event;
        enum
        {
                installer_database, // msi
                merge_module,       // msm
                transform_database, // mst
                patch_package       // msp
        } db_type;

        MsiFiles*       files;
        MsiComponents*  components;
        MsiDirectories* directories;
        MsiCabinets*    cabinets;
        MsiSimpleFiles* simple_files;

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
        template <class T>
        friend class MsiTable;
        friend class MsiCabinets;
        friend IMsiDumpCab* MsiDumpCreateObject();
        static void __cdecl ThreadLoadDatabase(void* parameter);
        bool DoOpen(PCWSTR file_name);

public:
        void Release();
        bool Open(PCWSTR file_name)
        {
                is_delay_loading = false;
                delay_event      = NULL;
                return DoOpen(file_name);
        }
        bool DelayOpen(PCWSTR file_name, HANDLE event)
        {
                is_delay_loading = true;
                delay_event      = event;
                return DoOpen(file_name);
        }
        void Close();
        bool ExtractTo(PCWSTR directory_name, EnumSelectItems, EnumExtractTo);

        int  GetFileCount();
        void SelectFile(int index, bool select);
        bool GetFileDetail(int index, MsiDumpFileDetail* detail);
};
