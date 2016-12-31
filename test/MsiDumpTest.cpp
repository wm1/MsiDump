//
// Description:
//  1. Extract input_msi to output_folder
//  2. Compare source_folder with output_folder
//

#include "precomp.h"

#define DATA_FOLDER L"data\\"
PCWSTR input_msi     = DATA_FOLDER L"test.msi";
PCWSTR output_folder = DATA_FOLDER L"out";
PCWSTR source_folder = DATA_FOLDER L"in";

bool CompareFolder(PCWSTR x, PCWSTR y)
{
        wprintf(L"Compare %s to %s\n", x, y);
        if (x == y)
        {
                return false;
        }
        return true;
}

int __cdecl wmain()
{
        class InitCOM
        {
        public:
                InitCOM() { CoInitialize(NULL); }
                ~InitCOM() { CoUninitialize(); }
        } initCOM;

        RemoveDirectory(output_folder);

        int          main_result = ERROR_SUCCESS;
        IMsiDumpCab* msi         = MsiDumpCreateObject();

        wprintf(L"Extract %s to %s\n", input_msi, output_folder);
        if (!msi->Open(input_msi))
        {
                fwprintf(stderr, L"error: fail to open msi package: %s\n", input_msi);
                main_result = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        }
        else if (!msi->ExtractTo(output_folder, ALL_SELECTED, EXTRACT_TO_FLAT_FOLDER))
        {
                fwprintf(stderr, L"error: fail to extract msi file. check out trace.txt for details\n");
                main_result = ERROR_INSTALL_FAILURE;
        }
        else if (!CompareFolder(source_folder, output_folder))
        {
                main_result = ERROR_INSTALL_FAILURE;
        }

        msi->Close();
        msi->Release();
        return main_result;
}
