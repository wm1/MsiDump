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

#define trace wcout
#define trace_error trace << L"[" << __FUNCTION__ << L" @line " << __LINE__ << L"] error: "

bool CompareFolder(PCWSTR path1, PCWSTR path2);
bool CompareFile(PCWSTR name1, PCWSTR name2, size_t length);
bool RemoveFolderRecursively(PCWSTR path);

int __cdecl wmain()
{
        class InitCOM
        {
        public:
                InitCOM() { CoInitialize(NULL); }
                ~InitCOM() { CoUninitialize(); }
        } initCOM;

        trace << L"Extract " << input_msi << L" to " << output_folder << endl;

        int          main_result = ERROR_SUCCESS;
        IMsiDumpCab* msi         = MsiDumpCreateObject();
        if (msi == NULL)
        {
                trace_error << L"internal error" << endl;
                main_result = ERROR_INSTALL_FAILURE;
                return main_result;
        }
        else if (!RemoveFolderRecursively(output_folder))
        {
                main_result = ERROR_INSTALL_FAILURE;
        }
        else if (!msi->Open(input_msi))
        {
                trace_error << L"Open msi package:" << input_msi << endl;
                main_result = ERROR_INSTALL_PACKAGE_OPEN_FAILED;
        }
        else if (!msi->ExtractTo(output_folder, ALL_SELECTED, EXTRACT_TO_FLAT_FOLDER))
        {
                trace_error << L"Extract msi file" << endl;
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

class Folder
{
        struct File
        {
                bool          is_folder;
                wstring       name;
                LARGE_INTEGER size;
        };

        wstring      path;
        vector<File> files;

public:
        Folder(wstring Path)
        {
                DWORD           error;
                HANDLE          hFind;
                WIN32_FIND_DATA find_data;

                path  = Path;
                hFind = FindFirstFile(Path.append(L"\\*").c_str(), &find_data);
                if (hFind == INVALID_HANDLE_VALUE)
                {
                        error = GetLastError();
                        trace_error << "FindFirstFile(" << path << L"\\*) failed with " << error << endl;
                        return;
                }

                do
                {
                        File file;
                        file.is_folder     = TEST_FLAG(find_data.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
                        file.name          = find_data.cFileName;
                        file.size.LowPart  = find_data.nFileSizeLow;
                        file.size.HighPart = find_data.nFileSizeHigh;

                        bool skip = (file.is_folder) && ((file.name == L"." || file.name == L".."));
                        if (!skip)
                        {
                                files.push_back(file);
                        }

                } while (FindNextFile(hFind, &find_data) != 0);

                FindClose(hFind);
        }

        bool Equals(Folder& That)
        {
                // Verify the two folders have the same number of child entries
                //
                if (files.size() != That.files.size())
                {
                        trace_error << L"File counts in two folders are different:" << endl
                                    << path << L" : " << files.size() << endl
                                    << That.path << L" : " << That.files.size() << endl;
                        return false;
                }

                for (auto p = files.begin(); p != files.end(); p++)
                {
                        // Verify the same file name exists in both folders
                        //
                        wstring name = p->name;
                        auto    q    = That.files.begin();
                        for (; q != That.files.end() && name != q->name; q++)
                                ;
                        if (q == That.files.end())
                        {
                                trace_error << name << L" exits in " << path << L" but not in " << That.path << endl;
                                return false;
                        }

                        wstring thisChild = path + L"\\" + name;
                        wstring thatChild = That.path + L"\\" + name;

                        if (p->is_folder != q->is_folder)
                        {
                                trace_error << L"One is a folder, but the other is not: " << thisChild << ", " << thatChild << endl;
                                return false;
                        }

                        if (p->is_folder)
                        {
                                // recursively compare sub-folders
                                //
                                Folder thisChildFolder(thisChild);
                                Folder thatChildFolder(thatChild);
                                if (!thisChildFolder.Equals(thatChildFolder))
                                {
                                        return false;
                                }
                        }
                        else
                        {
                                // compare file size and content
                                //
                                if (p->size.QuadPart != q->size.QuadPart)
                                {
                                        trace_error << L"File sizes are different" << endl
                                                    << thisChild << L" : " << p->size.QuadPart << endl
                                                    << thatChild << L" : " << q->size.QuadPart << endl;
                                        return false;
                                }
                                if (!CompareFile(thisChild.c_str(), thatChild.c_str(), (size_t)p->size.QuadPart))
                                {
                                        return false;
                                }
                        }
                }
                return true;
        }
};

bool CompareFolder(PCWSTR path1, PCWSTR path2)
{
        trace << L"Compare " << path1 << L" with " << path2 << endl;
        Folder thisFolder(path1);
        Folder thatFolder(path2);
        return thisFolder.Equals(thatFolder);
}

bool CompareFile(PCWSTR name1, PCWSTR name2, size_t length)
{
        if (length == 0)
        {
                return true;
        }

        bool     result = false;
        ifstream file1(name1, ios::in | ios::binary);
        ifstream file2(name2, ios::in | ios::binary);
        if (file1.is_open() && file2.is_open())
        {
                char* buf1 = new char[length];
                char* buf2 = new char[length];
                buf1[0]    = 'A';
                buf2[0]    = 'B';

                file1.read(buf1, length);
                file2.read(buf2, length);
                file1.close();
                file2.close();

                if (memcmp(buf1, buf2, length) == 0)
                {
                        result = true;
                }

                delete[] buf1;
                delete[] buf2;
        }

        if (!result)
        {
                trace_error << L"File contents are different: " << name1 << L", " << name2 << endl;
        }
        return result;
}

bool RemoveFolderRecursively(PCWSTR path)
{
        DWORD attr = GetFileAttributes(path);
        if (attr == INVALID_FILE_ATTRIBUTES)
        {
                DWORD error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND)
                {
                        // the folder does not exist. consider remove folder finishes successfully
                        return true;
                }
                trace_error << error << L". " << path << endl;
                return false;
        }
        if (!TEST_FLAG(attr, FILE_ATTRIBUTE_DIRECTORY))
        {
                trace_error << path << " is not a directory" << endl;
                return false;
        }
        size_t len   = wcslen(path);
        PWSTR  zzBuf = new WCHAR[len + 2]; // This string must be double-null terminated
        wcscpy_s(zzBuf, len + 2, path);
        zzBuf[len + 1] = L'\0';

        SHFILEOPSTRUCT file_op = {
                NULL,
                FO_DELETE,
                zzBuf,
                NULL,
                FOF_NO_UI,
                FALSE,
                NULL,
                NULL};

        int ret = SHFileOperation(&file_op);
        delete[] zzBuf;

        if (ret != 0)
        {
                trace_error << L"RemoveFolder(" << path << L") failed with " << ret << endl;
                return false;
        }
        return true;
}
