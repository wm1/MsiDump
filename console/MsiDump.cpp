#include "precomp.h"

int __cdecl wmain(int argc, PCWSTR argv[])
{
        ParseArgs(argc, argv);
        if (Args.command == CMD_HELP)
        {
                PCWSTR exe = argv[0];
                Usage(exe);
                return ERROR_SUCCESS;
        }
        else if (Args.command == CMD_INVALID)
                return ERROR_INVALID_PARAMETER;

        IMsiDumpCab* msi = MsiDumpCreateObject();
        if (msi == NULL)
        {
                fwprintf(stderr, L"internal error\n");
                return ERROR_NOT_ENOUGH_MEMORY;
        }

        DWORD attributes = GetFileAttributes(Args.file_name);
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
                fwprintf(stderr, L"error: file not found: %s\n", Args.file_name);
                return ERROR_FILE_NOT_FOUND;
        }

        if (!msi->Open(Args.file_name))
        {
                fwprintf(stderr, L"error: fail to open msi package: %s\n", Args.file_name);
                msi->Release();
                return ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        }

        if (Args.command == CMD_LIST)
        {
                ListHeader();
                int count = msi->GetFileCount();
                for (int i = 0; i < count; i++)
                {
                        MsiDumpFileDetail detail;
                        msi->GetFileDetail(i, &detail);
                        ListRow(i, &detail);
                }
        }
        else if (Args.command == CMD_EXTRACT)
        {
                EnumExtractTo extract_to = (Args.u.extract.is_extract_full_path ? EXTRACT_TO_TREE : EXTRACT_TO_FLAT_FOLDER);

                WCHAR file_name[MAX_PATH];
                GetFullPathName(Args.u.extract.path_to_extract, MAX_PATH, file_name, NULL);
                bool b = msi->ExtractTo(file_name, SELECT_ALL_ITEMS, extract_to);
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
