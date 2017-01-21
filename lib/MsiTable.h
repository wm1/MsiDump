
#pragma once

// Overview of Windows Installer
//
//  - From user's point of view, a installation package contains multiple
//    invididually-selectable-installable *features*
//
//  - Each feature may contain multiple independent *components*.
//
//  - Each component contains multiple *files*. A file cannot belong to more
//    than one components.
//
//  - Each component must be installed to a single folder.
//    https://msdn.microsoft.com/en-us/library/windows/desktop/aa372830.aspx
//

class MsiUtils;

class MsiQuery
{
private:
        PMSIHANDLE view;
        PMSIHANDLE record;
        bool       is_end;

public:
        MsiQuery(MsiUtils* msi, wstring sql);
        MSIHANDLE Next();
};

template <class T>
class MsiTable
{
protected:
        void    Init();
        int     GetRowCount();
        wstring GetPrimaryKey();

        wstring   table_name;
        int       count;
        T*        array;
        MsiUtils* msi;

public:
        MsiTable(MsiUtils* msi);
        virtual ~MsiTable();
        friend class MsiUtils;
};

//
// Platform SDK: Windows Installer: Installer Database Reference
//

// File Table
//
// ========================================
// Column      Type           Key  Nullable
// ========================================
// File        Identifier     Y    N
// Component_  Identifier          N
// FileName    Filename            N
// FileSize    DoubleInteger       N
// Version     Version             Y
// Language    Language            Y
// Attributes  Integer             Y
// Sequence    Integer             N
// ========================================
//
// The File table contains a complete list of source files with their various attributes,
// ordered by a unique, non-localized identifier. Files can be stored on the source media
// as individual files or compressed within a cabinet file
//
struct TagFile
{
        wstring file; // A unique, non-localized, case-insensitive identifier

        wstring component; // External key into Component Table

        wstring file_name; // A string in the format of: short_file_name ['|' long_file_name]

        wstring version; // [Optional] e.g. aaa.bbb.ccc.ddd

        wstring language; // [Optional] e.g. 0x0409 for U.S.

        int file_size; // A 32-bit integer representing the file size in bytes. max is 2 GB

        int attributes; // [Optional] bit flags. e.g. msidbFileAttributesCompressed = 0x4000

        int sequence; // Sequence position of this file on the compressed media images.
                      // For files that are not compressed, the sequence numbers need not be unique.
                      // For a file that is compressed with a sequence number of e.g. 92, look up
                      // the Media table with the smallest Last Sequence value that is bigger than 92

        int  key_cabinet;
        int  key_directory;
        int  key_component;
        bool is_compressed;
        bool is_selected;
};
typedef class MsiTable<TagFile> MsiFiles;

// This uses the same File table above, but only exposes limited information for the files.
// The idea around this sub-table is that, the main user-interface can display this partical
// information to the user quickly, while on the background, the program continues to read
// other tables (component, directory, cabinet). when that is finished, the UI switches back
// to display more complete information for the files
//
struct TagSimpleFile
{
        wstring file_name;
        int     file_size;
};
typedef MsiTable<TagSimpleFile> MsiSimpleFiles;

// Component Table
//
// ========================================
// Column      Type           Key  Nullable
// ========================================
// Component   Identifier     Y    N
// ComponentId GUID                Y
// Directory_  Identifier          N
// Attributes  Integer             N
// Condition   Condition           Y
// KeyPath     Identifier          Y
// ========================================
//
struct TagComponent
{
        wstring component; // Identifies the component record.

        wstring directory; // External key into the Directory table.

        wstring condition; // [Optional] If the condition is null or evaluates to true, then the component is installed

        int  key_directory;
        bool win_9x;
        bool win_nt;
        bool win_x64;
};
typedef MsiTable<TagComponent> MsiComponents;

// Directory Table
//
// ========================================
// Column      Type           Key  Nullable
// ========================================
// Directory   Identifier     Y    N
// Directory_Parent Identifier     Y
// DefaultDir  DefaultDir          N
// ========================================
//
// Each record in the table represents a directory in both the source and the destination images.
// The source location is useful if the file is not compressed.
//
struct TagDirectory
{
        wstring directory; // Identifies the directory record.

        wstring source_directory; // the source/target location is calculated by call API
        wstring target_directory; // instead of reading from tables directly
        bool    is_target_verified;
        bool    is_target_existing;
};
typedef MsiTable<TagDirectory> MsiDirectories;

// Media Table
//
// ========================================
// Column      Type           Key  Nullable
// ========================================
// DiskId      Integer        Y    N
// LastSequence Integer            N
// DiskPrompt  Text                Y
// Cabinet     Cabinet             Y
// VolumeLabel Text                Y
// Source      Property            Y
// ========================================
//
struct TagCabinet
{
        int last_sequence; // File sequence number for the last file for this media, thus specifiy
                           // which files in the File table are found on a particular source disk.

        wstring cabinet; // The name of the cabinet file if some or all files are compressed.
                         // If the cabinet name is preceded by the number sign '#', the cabinet
                         // is embedded in '_Streams' table inside the package.

        wstring extracted_name;
        bool    is_embedded; // cabinet name = (embedded ? this->extracted_name : this->cabinet)
        bool    iterated;
};

class MsiCabinets
        : public MsiTable<TagCabinet>
{
public:
        MsiCabinets(MsiUtils* utils)
                : MsiTable<TagCabinet>(utils)
        {
        }
        virtual ~MsiCabinets();

        bool Extract(int index);
};
