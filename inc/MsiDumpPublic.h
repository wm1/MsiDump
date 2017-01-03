
#pragma once

//
// Platform SDK: Windows Installer: Installer Database Reference
//
// File Table
// =======================================
// Column     Type           Key  Nullable
// =======================================
// File       Identifier     Y    N
// Component_ Identifier          N
// FileName   Filename            N
// FileSize   DoubleInteger       N
// Version    Version             Y
// Language   Language            Y
// Attributes Integer             Y
// Sequence   Integer             N
// =======================================
//

struct MsiDumpFileDetail
{
        PCWSTR filename;
        int    filesize;
        PCWSTR path;
        PCWSTR version;
        PCWSTR language;
        bool   win9x; // Should the file be installed on Windows 95/98/Me?
        bool   winNT; // Should be installed on Windows NT/2000/XP/2003?
        bool   winX64;
        bool   selected;
};

enum enumSelectAll
{
        ALL_SELECTED,
        INDIVIDUAL_SELECTED
};

enum enumFlatFolder
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
        virtual bool Open(PCWSTR filename) = 0;

        // Close the package that was previously opened
        //
        virtual void Close() = 0;

        // Get the number of files in the installer package
        //
        virtual int getCount() = 0;

        // Get the i-th (0 <= i <= count) file's information.
        // Note: refer to DelayedOpen() on which fields of the result is valid.
        //
        virtual bool GetFileDetail(int index, MsiDumpFileDetail* detail) = 0;

        // Mark each file to be extracted later if calling ExtractTo() with INDIVIDUAL_SELECTED
        //
        virtual void setSelected(int index, bool select) = 0;

        // Extract files out
        //
        virtual bool ExtractTo(PCWSTR directory, enumSelectAll selectAll, enumFlatFolder flatFolder) = 0;

        // By default Open() retrieves a list of file names in the installer package, as well as each file's
        // more detailed information, before returning control to the caller.
        //
        // This causes problem when the installer package is really big e.g. contains thousands of files. It
        // simply takes too long to finish Open() and the user-interface cannot be refreshed during that time.
        //
        // The suggested approach for more responsive UI is to do it in two steps:
        //   1. Call DelayedOpen(event). When it returns, the file names are ready to be displayed on UI
        //   2. WaitForSingleObject(event). When it returns, more detailed file information can be displayed
        //
        // During each step, the caller calls the same GetFileDetail() but:
        //   1. before the said event is triggered, only file name & file size fields are valid
        //   2. after the event is set, all fields become valid
        //
        virtual bool DelayedOpen(PCWSTR filename, HANDLE event) = 0;
};

IMsiDumpCab* MsiDumpCreateObject();
