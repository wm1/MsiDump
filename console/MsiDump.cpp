
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include "MsiDumpPublic.h"
#include "parseArgs.h"

class InitCOM
{
public:
        InitCOM()  { CoInitialize(NULL); }
        ~InitCOM() { CoUninitialize();   }
} initCOM;

int __cdecl
wmain(int argc, LPCWSTR argv[])
{
        parseArgs(argc, argv);
        if (args.cmd == cmd_help)
        {
                LPCWSTR exe = argv[0];
                usage(exe);
                return ERROR_SUCCESS;
        }
        else if (args.cmd == cmd_invalid)
                return E_INVALIDARG;

        IMsiDumpCab *msi = MsiDumpCreateObject();

        DWORD fileAttr = GetFileAttributes(args.filename);
        if (fileAttr == INVALID_FILE_ATTRIBUTES)
        {
                fwprintf(stderr, L"error: file not found: %s\n", args.filename);
                return ERROR_FILE_NOT_FOUND;
        }

        if (!msi->Open(args.filename))
        {
                fwprintf(stderr, L"error: fail to open msi package: %s\n", args.filename);
                msi->Release();
                return ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        }

        if (args.cmd == cmd_list)
        {
                listHeader();
                int count = msi->getCount();
                for (int i=0; i<count; i++)
                {
                        MsiDumpFileDetail detail;
                        msi->GetFileDetail(i, &detail);
                        listRecord(i, &detail);
                }
        }
        else if (args.cmd == cmd_extract)
        {
                enumFlatFolder flatFolder = (args.extract_full_path ? EXTRACT_TO_TREE : EXTRACT_TO_FLAT_FOLDER);

                WCHAR filename[MAX_PATH];
                GetFullPathName(args.path_to_extract, MAX_PATH, filename, NULL);
                bool b = msi->ExtractTo(filename, ALL_SELECTED, flatFolder);
                if (!b)
                {
                        fwprintf(stderr, L"error: fail to extract msi file. check out trace.txt for details\n");
                        msi->Close();
                        msi->Release();
                        return ERROR_INSTALL_FAILURE;
                }
        }

        msi->Close();
        msi->Release();
        return ERROR_SUCCESS;
}
