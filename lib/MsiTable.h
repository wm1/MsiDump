
#pragma once

class MsiUtils;

class MsiQuery
{
private:
        PMSIHANDLE view;
        PMSIHANDLE record;
        bool       is_end;

public:
        MsiQuery(MsiUtils* msiUtils, wstring sql);
        MSIHANDLE Next();
};

template <class T>
class MsiTable
{
protected:
        int     GetRowCount();
        wstring GetPrimaryKey();
        void    Init();

        int       count;
        wstring   table_name;
        MsiUtils* msiUtils;
        T*        array;

public:
        MsiTable(MsiUtils*);
        virtual ~MsiTable();
        friend class MsiUtils;
};

struct TagFile
{
        wstring file;
        wstring component;
        wstring file_name;
        wstring version;
        wstring language;
        int     file_size;
        int     attributes;
        int     sequence;

        bool is_compressed;
        int  key_cabinet;
        int  key_directory;
        int  key_component;

        bool is_selected;
};
typedef class MsiTable<TagFile> MsiFiles;


struct TagSimpleFile
{
        wstring file_name;
        int     file_size;
};
typedef MsiTable<TagSimpleFile> MsiSimpleFiles;

struct TagComponent
{
        wstring component;
        wstring directory;
        wstring condition;

        int  key_directory;
        bool win_9x;
        bool win_nt;
        bool win_x64;
};
typedef MsiTable<TagComponent> MsiComponents;

struct TagDirectory
{
        wstring directory;
        wstring source_directory;
        wstring target_directory;

        bool is_target_verified;
        bool is_target_existing;
};
typedef MsiTable<TagDirectory> MsiDirectories;

struct TagCabinet
{
        int     disk_id;
        int     last_sequence;
        wstring cabinet;

        bool    is_embedded;
        wstring extracted_name;

        bool iterated;
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
