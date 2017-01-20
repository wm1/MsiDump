#include "precomp.h"

//////////////////////////////////////////////////////////////////////////

CDropSource::CDropSource()
        : INIT_IUNKNOWN(IID_IDropSource)
{
}

STDMETHODIMP
CDropSource::QueryContinueDrag(
        BOOL  is_escape_pressed,
        DWORD key_state)
{
        if (is_escape_pressed)
                return DRAGDROP_S_CANCEL;
        if (key_state & (MK_LBUTTON | MK_RBUTTON))
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
        FORMATETC* item,
        ULONG*     fetched)
{
        if (current == count)
                return S_FALSE;

        *item = array[current++];
        if (fetched)
                *fetched = 1;
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
        msi          = _msi;
        count        = _selectedCount;
        is_extracted = false;
        array        = new int[count];
        int current  = 0;
        for (int i = 0; i < msi->GetFileCount(); i++)
        {
                MsiDumpFileDetail detail;
                msi->GetFileDetail(i, &detail);
                if (detail.is_selected)
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

        // RemoveDirectory(temporary_folder) recursively.
        temporary_folder[wcslen(temporary_folder) + 1] = L'\0'; // SHFileOp requires double zero ending

        SHFILEOPSTRUCT op;
        ZeroMemory(&op, sizeof(op));
        op.hwnd   = NULL;
        op.wFunc  = FO_DELETE;
        op.pFrom  = temporary_folder;
        op.pTo    = NULL;
        op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;
        SHFileOperation(&op);
}

STDMETHODIMP
CDataObject::EnumFormatEtc(
        DWORD            direction,
        IEnumFORMATETC** enum_formatetc)
{
        if (direction != DATADIR_GET)
                return E_NOTIMPL;

        *enum_formatetc = (IEnumFORMATETC*)new CEnumFormatetc(formats, sizeof(formats) / sizeof(formats[0]));
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
                        desc->nFileSizeLow = detail.file_size;
                        wcscpy_s(desc->cFileName, MAX_PATH, detail.file_name);
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
        if (!is_extracted)
        {
                is_extracted = true;
                EnumSelectItems select_items;
                if (count == msi->GetFileCount())
                        select_items = SELECT_ALL_ITEMS;
                else
                        select_items = SELECT_INDIVIDUAL_ITEMS;

                WCHAR temp[MAX_PATH];
                GetTempPath(MAX_PATH, temp);
                GetTempFileName(temp, L"cac", 0, temporary_folder);
                DeleteFile(temporary_folder);
                CreateDirectory(temporary_folder, NULL);

                msi->ExtractTo(temporary_folder, select_items, EXTRACT_TO_TREE);
        }

        WCHAR temp_path[MAX_PATH], temp_file[MAX_PATH];
        GetTempPath(MAX_PATH, temp_path);
        GetTempFileName(temp_path, L"cab", 0, temp_file);

        MsiDumpFileDetail detail;
        msi->GetFileDetail(index, &detail);
        WCHAR file_name[MAX_PATH];
        wcscpy_s(file_name, MAX_PATH, temporary_folder);
        wcscat_s(file_name, MAX_PATH, detail.path);
        wcscat_s(file_name, MAX_PATH, detail.file_name);
        BYTE* buffer = (BYTE*)GlobalAlloc(GMEM_FIXED, detail.file_size);

        FILE* file;
        if (_wfopen_s(&file, file_name, L"rb") != 0)
        {
                fread(buffer, detail.file_size, 1, file);
                fclose(file);
        }
        return (HGLOBAL)buffer;
}

//////////////////////////////////////////////////////////////////////////
//
// this is the exposed function to do drag & drop

void Drag(
        IMsiDumpCab* msi,
        int          selected_count)
{
        CDataObject* data_object = new CDataObject(msi, selected_count);
        CDropSource* drop_source = new CDropSource;
        DWORD        effect      = DROPEFFECT_COPY;
        DoDragDrop(data_object, drop_source, effect, &effect);
        drop_source->Release();
        data_object->Release();
}

//////////////////////////////////////////////////////////////////////////
