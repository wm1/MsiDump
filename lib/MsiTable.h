
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

class MsiTable
{
private:
        int     CountRows();
        wstring getPrimaryKey();

protected:
        int       count;
        wstring   name;
        MsiUtils* msiUtils;

public:
        MsiTable(MsiUtils* theMsiUtils, wstring tableName);
        virtual ~MsiTable();
        friend class MsiUtils;
};

class MsiFile : public MsiTable
{
private:
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
        } * array;

public:
        MsiFile(MsiUtils* msiUtils);
        virtual ~MsiFile();
        friend class MsiUtils;
};

//
// note: ui.cpp (CMainFrame::OnGetDispInfo, case COLUMN_SIZE) caches filesize locally,
// therefore both MsiSimpleFile and MsiFile must return the same filesize.
//
class MsiSimpleFile : public MsiTable
{
private:
        struct tagFile
        {
                wstring filename;
                int     filesize;
        } * array;

public:
        MsiSimpleFile(MsiUtils* msiUtils);
        virtual ~MsiSimpleFile();
        friend class MsiUtils;
};

class MsiComponent : public MsiTable
{
private:
        struct tagComponent
        {
                wstring component;
                wstring directory;
                wstring condition;

                int  keyDirectory;
                bool win9x;
                bool winNT;
                bool winX64;
        } * array;

public:
        MsiComponent(MsiUtils* msiUtils);
        virtual ~MsiComponent();
        friend class MsiUtils;
};

class MsiDirectory : public MsiTable
{
private:
        struct tagDirectory
        {
                wstring directory;

                wstring sourceDirectory;
                wstring targetDirectory;

                bool targetDirectoryVerified;
                bool targetDirectoryExists;
        } * array;

public:
        MsiDirectory(MsiUtils* msiUtils);
        virtual ~MsiDirectory();
        friend class MsiUtils;
};

class MsiCabinet : public MsiTable
{
private:
        struct tagCabinet
        {
                int     diskId;
                int     lastSequence;
                wstring cabinet;

                bool    embedded; // is it stored within .msi file as a separate stream?
                wstring tempName; // if(embedded), already extracted to a temporary location?

                bool iterated;
        } * array;
        bool Extract(int index);

public:
        MsiCabinet(MsiUtils* msiUtils);
        virtual ~MsiCabinet();
        friend class MsiUtils;
};
