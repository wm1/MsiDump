
#pragma once

struct MsiDumpFileDetail
{
        PCWSTR file_name;
        int    file_size;
        PCWSTR path;
        PCWSTR version;
        PCWSTR language;
        bool   win_9x; // Should the file be installed on Windows 95/98/Me?
        bool   win_nt; // Should be installed on Windows NT/2000/XP/2003?
        bool   win_x64;
        bool   is_selected;
};

enum EnumSelectItems
{
        SELECT_ALL_ITEMS,
        SELECT_INDIVIDUAL_ITEMS
};

enum EnumExtractTo
{
        EXTRACT_TO_FLAT_FOLDER,
        EXTRACT_TO_TREE
};

class IMsiDumpCab
{
public:
        // Typical usage:
        //
        //   p = MsiDumpCreateObject();
        //   ...
        //   p->Release();
        //
        virtual void Release() = 0;

        // Open the said installer package
        //
        //   p->Open();
        //   ...
        //   p->Close();
        //
        virtual bool Open(PCWSTR file_name) = 0;

        // Close the package that was previously opened
        //
        virtual void Close() = 0;

        // Get the number of files in the installer package
        //
        virtual int GetFileCount() = 0;

        // Get the i-th (0 <= i < count) file's information.
        // Note: refer to DelayOpen() on which fields of the result is valid.
        //
        virtual bool GetFileDetail(int index, MsiDumpFileDetail* detail) = 0;

        // Mark each file to be extracted later if calling ExtractTo() with SELECT_INDIVIDUAL_ITEMS
        //
        virtual void SelectFile(int index, bool select) = 0;

        // Extract files out
        //
        virtual bool ExtractTo(PCWSTR directory, EnumSelectItems select_items, EnumExtractTo extract_to) = 0;

        // By default Open() retrieves a list of file names in the installer package, as well as each file's
        // more detailed information, before returning control to the caller.
        //
        // This causes problem when the installer package is really big e.g. contains thousands of files. It
        // simply takes too long to finish Open() and the user-interface cannot be refreshed during that time.
        //
        // The suggested approach for more responsive UI is to do it in two steps:
        //   1. Call DelayOpen(event). When it returns, the file names are ready to be displayed on UI
        //   2. WaitForSingleObject(event). When it returns, more detailed file information can be displayed
        //
        // During each step, the caller calls the same GetFileDetail() but:
        //   1. before the said event is triggered, only file name & file size fields are valid
        //   2. after the event is set, all fields become valid
        //
        virtual bool DelayOpen(PCWSTR file_name, HANDLE event) = 0;
};

IMsiDumpCab* MsiDumpCreateObject();
