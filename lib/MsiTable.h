
#pragma once

class MsiUtils;

class MsiQuery
{
private:
        MSIHANDLE view;
        MSIHANDLE record;
        bool      ended;
        bool      viewCreated;

public:
        MsiQuery(MsiUtils* msiUtils, wstring sql);
        ~MsiQuery();
        MSIHANDLE Next();
};

template <class T>
class MsiTable
{
protected:
        int     CountRows();
        wstring getPrimaryKey();
        void    Init();

        int       count;
        wstring   name;
        MsiUtils* msiUtils;
        T*        array;

public:
        MsiTable(MsiUtils*);
        virtual ~MsiTable();
        friend class MsiUtils;
};

struct tagFile
{
        wstring file;
        wstring component;
        wstring filename;
        wstring version;
        wstring language;
        int     filesize; // issue: should be DoubleInteger
        int     attributes;
        int     sequence;

        bool compressed;
        int  keyCabinet; // if(compressed), which .cab file it residents?
        int  keyDirectory;
        int  keyComponent;

        bool selected;
};
typedef class MsiTable<tagFile> MsiFile;

//
// note: ui.cpp (CMainFrame::OnGetDispInfo, case COLUMN_SIZE) caches filesize locally,
// therefore both MsiSimpleFile and MsiFile must return the same filesize.
//
struct tagSimpleFile
{
        wstring filename;
        int     filesize;
};
typedef MsiTable<tagSimpleFile> MsiSimpleFile;

struct tagComponent
{
        wstring component;
        wstring directory;
        wstring condition;

        int  keyDirectory;
        bool win9x;
        bool winNT;
        bool winX64;
};
typedef MsiTable<tagComponent> MsiComponent;

struct tagDirectory
{
        wstring directory;

        wstring sourceDirectory;
        wstring targetDirectory;

        bool targetDirectoryVerified;
        bool targetDirectoryExists;
};
typedef MsiTable<tagDirectory> MsiDirectory;

struct tagCabinet
{
        int     diskId;
        int     lastSequence;
        wstring cabinet;

        bool    embedded; // is it stored within .msi file as a separate stream?
        wstring tempName; // if(embedded), already extracted to a temporary location?

        bool iterated;
};

class MsiCabinet
        : public MsiTable<tagCabinet>
{
public:
        MsiCabinet(MsiUtils* utils)
                : MsiTable<tagCabinet>(utils)
        {
        }
        virtual ~MsiCabinet();

        bool Extract(int index);
};
