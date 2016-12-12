
#include "CUnknown.h"

class CDropSource
        : public IDropSource
{
public:
        DECLARE_IUNKNOWN;

        // IDropSource
        STDMETHOD (QueryContinueDrag) (BOOL, DWORD);
        STDMETHOD (GiveFeedback)      (DWORD);

        CDropSource();
};


class CEnumFormatetc
        : public IEnumFORMATETC
{
public:
        DECLARE_IUNKNOWN;

        // IEnumFORMATETC
        STDMETHOD (Next)  (ULONG, FORMATETC*, ULONG*);
        STDMETHOD (Reset) ();
        STDMETHOD (Skip)  (ULONG)                                 { return E_NOTIMPL; }
        STDMETHOD (Clone) (IEnumFORMATETC**)                      { return E_NOTIMPL; }

        CEnumFormatetc(FORMATETC*, int);

private:
        FORMATETC *array;
        int        count, current;
};


class CDataObject
        : public IDataObject
{
public:
        DECLARE_IUNKNOWN;

        // IDataObject
        STDMETHOD (GetData)      (FORMATETC*, STGMEDIUM*);
        STDMETHOD (EnumFormatEtc)(DWORD, IEnumFORMATETC**);
        STDMETHOD (GetDataHere)  (FORMATETC*, STGMEDIUM*)         { return E_NOTIMPL; }
        STDMETHOD (QueryGetData) (FORMATETC*)                     { return E_NOTIMPL; }
        STDMETHOD (GetCanonicalFormatEtc) (FORMATETC*, FORMATETC*){ return E_NOTIMPL; }
        STDMETHOD (SetData)      (FORMATETC*, STGMEDIUM*, BOOL)   { return E_NOTIMPL; }
        STDMETHOD (DUnadvise)    (DWORD)                          { return E_NOTIMPL; }
        STDMETHOD (EnumDAdvise)  (IEnumSTATDATA**)                { return E_NOTIMPL; }
        STDMETHOD (DAdvise)      (FORMATETC*, DWORD, IAdviseSink*, DWORD*){ return E_NOTIMPL; }

        CDataObject(IMsiDumpCab*, int);
        virtual ~CDataObject();

private:
        static FORMATETC  formats[2];
        static CLIPFORMAT CF_FILEDESCRIPTOR;
        static CLIPFORMAT CF_FILECONTENTS;

        IMsiDumpCab *msi;
        int          count;
        int         *array;
        bool         extracted;
        WCHAR        tempFolder[MAX_PATH];
        HGLOBAL      ReadFile(int index);
};

