#include "precomp.h"
#include <shlobj.h>
#include "CUnknown.h"
#include "dragdrop.h"

//////////////////////////////////////////////////////////////////////////

CDropSource::CDropSource()
        : INIT_IUNKNOWN(IID_IDropSource)
{
}

STDMETHODIMP
CDropSource::QueryContinueDrag(
        BOOL  fEscapePressed,
        DWORD grfKeyState)
{
        if (fEscapePressed)
                return DRAGDROP_S_CANCEL;
        if (grfKeyState & (MK_LBUTTON | MK_RBUTTON))
                return S_OK;
        return DRAGDROP_S_DROP;
}

STDMETHODIMP
CDropSource::GiveFeedback(
        DWORD /*dwEffect*/
        )
{
        return DRAGDROP_S_USEDEFAULTCURSORS;
}

//////////////////////////////////////////////////////////////////////////

CEnumFormatetc::CEnumFormatetc(
        FORMATETC* _array,
        int        _count)
        : INIT_IUNKNOWN(IID_IEnumFORMATETC)
{
        array   = _array;
        count   = _count;
        current = 0;
}

STDMETHODIMP
CEnumFormatetc::Next(
        ULONG /*celt*/,
        FORMATETC* rgelt,
        ULONG*     pceltFetched)
{
        if (current == count)
                return S_FALSE;

        *rgelt = array[current++];
        if (pceltFetched)
                *pceltFetched = 1;
        return S_OK;
}

STDMETHODIMP
CEnumFormatetc::Reset()
{
        current = 0;
        return S_OK;
}

//////////////////////////////////////////////////////////////////////////

FORMATETC
CDataObject::formats[2] = {
        {0 /*FILEDESCRIPTOR*/, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {0 /*FILECONTENTS  */, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL}};

CLIPFORMAT CDataObject::CF_FILEDESCRIPTOR = 0;
CLIPFORMAT CDataObject::CF_FILECONTENTS   = 0;

CDataObject::CDataObject(
        IMsiDumpCab* _msi,
        int          _selectedCount)
        : INIT_IUNKNOWN(IID_IDataObject)
{
        msi         = _msi;
        count       = _selectedCount;
        extracted   = false;
        array       = new int[count];
        int current = 0;
        for (int i = 0; i < msi->getCount(); i++)
        {
                MsiDumpFileDetail detail;
                msi->GetFileDetail(i, &detail);
                if (detail.selected)
                        array[current++] = i;
                if (current == count)
                        break;
        }

        if (CF_FILEDESCRIPTOR != 0)
                return;

        formats[0].cfFormat = CF_FILEDESCRIPTOR = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR);
        formats[1].cfFormat = CF_FILECONTENTS = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
}

CDataObject::~CDataObject()
{
        delete[] array;

        // RemoveDirectory(tempFolder) recursively.
        tempFolder[wcslen(tempFolder) + 1] = L'\0'; // SHFileOp requires double zero ending
        SHFILEOPSTRUCT op;
        ZeroMemory(&op, sizeof(op));
        op.hwnd   = NULL;
        op.wFunc  = FO_DELETE;
        op.pFrom  = tempFolder;
        op.pTo    = NULL;
        op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        SHFileOperation(&op);
}

STDMETHODIMP
CDataObject::EnumFormatEtc(
        DWORD            dwDirection,
        IEnumFORMATETC** ppenumFormatetc)
{
        if (dwDirection == DATADIR_SET)
                return E_NOTIMPL;

        *ppenumFormatetc = (IEnumFORMATETC*)new CEnumFormatetc(formats, sizeof(formats) / sizeof(formats[0]));
        return S_OK;
}

STDMETHODIMP
CDataObject::GetData(
        FORMATETC* format,
        STGMEDIUM* medium)
{
        if (format == NULL || medium == NULL)
                return E_INVALIDARG;

        if (format->cfFormat == CF_FILEDESCRIPTOR)
        {
                if (format->ptd != NULL)
                        return E_INVALIDARG;
                if ((format->dwAspect & DVASPECT_CONTENT) == 0)
                        return DV_E_DVASPECT;
                if (format->lindex != -1)
                        return DV_E_LINDEX;
                if (format->tymed != TYMED_HGLOBAL)
                        return DV_E_TYMED;

                FILEGROUPDESCRIPTOR* group = (FILEGROUPDESCRIPTOR*)GlobalAlloc(GMEM_FIXED,
                                                                               sizeof(FILEGROUPDESCRIPTOR) + (count - 1) * sizeof(FILEDESCRIPTOR));
                group->cItems = count;
                for (int i = 0; i < count; i++)
                {
                        FILEDESCRIPTOR* desc = &group->fgd[i];
                        desc->dwFlags        = FD_FILESIZE;
                        desc->nFileSizeHigh  = 0;

                        MsiDumpFileDetail detail;
                        msi->GetFileDetail(array[i], &detail);
                        desc->nFileSizeLow = detail.filesize;
                        wcscpy_s(desc->cFileName, MAX_PATH, detail.filename);
                }
                medium->tymed          = TYMED_HGLOBAL;
                medium->hGlobal        = (HGLOBAL)group;
                medium->pUnkForRelease = NULL;
        }
        else if (format->cfFormat == CF_FILECONTENTS)
        {
                if (format->ptd != NULL)
                        return E_INVALIDARG;
                if ((format->dwAspect & DVASPECT_CONTENT) == 0)
                        return DV_E_DVASPECT;
                if ((format->tymed & TYMED_HGLOBAL) == 0)
                        return DV_E_TYMED;

                medium->tymed          = TYMED_HGLOBAL;
                medium->hGlobal        = ReadFile(array[format->lindex]);
                medium->pUnkForRelease = NULL;
        }
        else
                return DV_E_FORMATETC;
        return S_OK;
}

HGLOBAL
CDataObject::ReadFile(
        int index)
{
        if (!extracted)
        {
                extracted = true;
                enumSelectAll selectAll;
                if (count == msi->getCount())
                        selectAll = ALL_SELECTED;
                else
                        selectAll = INDIVIDUAL_SELECTED;

                WCHAR temp[MAX_PATH];
                GetTempPath(MAX_PATH, temp);
                GetTempFileName(temp, L"cac", 0, tempFolder);
                DeleteFile(tempFolder);
                CreateDirectory(tempFolder, NULL);

                msi->ExtractTo(tempFolder, selectAll, EXTRACT_TO_TREE);
        }
        WCHAR tempPath[MAX_PATH], tempFile[MAX_PATH];
        GetTempPath(MAX_PATH, tempPath);
        GetTempFileName(tempPath, L"cab", 0, tempFile);

        MsiDumpFileDetail detail;
        msi->GetFileDetail(index, &detail);
        WCHAR filename[MAX_PATH];
        wcscpy_s(filename, MAX_PATH, tempFolder);
        wcscat_s(filename, MAX_PATH, detail.path);
        wcscat_s(filename, MAX_PATH, detail.filename);
        BYTE* buffer = (BYTE*)GlobalAlloc(GMEM_FIXED, detail.filesize);

        FILE* f;
        if (_wfopen_s(&f, filename, L"rb") != 0)
        {
                fread(buffer, detail.filesize, 1, f);
                fclose(f);
        }
        return (HGLOBAL)buffer;
}

//////////////////////////////////////////////////////////////////////////
//
// this is the exposed function to do drag & drop

void Drag(
        IMsiDumpCab* msi,
        int          selectedCount)
{
        CDataObject* dataObject = new CDataObject(msi, selectedCount);
        CDropSource* dropSource = new CDropSource;
        DWORD        effect     = DROPEFFECT_COPY;
        DoDragDrop(dataObject, dropSource, effect, &effect);
        dropSource->Release();
        dataObject->Release();
}

//////////////////////////////////////////////////////////////////////////
