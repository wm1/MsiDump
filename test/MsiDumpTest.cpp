//
// Description:
//  1. Extract input_file to output_folder
//  2. Compare source_folder with output_folder
//

#include "precomp.h"

#define DATA_FOLDER L"data\\"
PCWSTR input_file    = DATA_FOLDER L"test.msi";
PCWSTR output_folder = DATA_FOLDER L"out";
PCWSTR source_folder = DATA_FOLDER L"in";

#define trace wcout
#define trace_error trace << L"[" << __FUNCTION__ << L" @line " << __LINE__ << L"] error: "

bool CompareFolder(PCWSTR path1, PCWSTR path2);
bool CompareFile(PCWSTR name1, PCWSTR name2, size_t length);
bool RemoveFolderRecursively(PCWSTR path);

int __cdecl wmain()
{
        trace << L"Extract " << input_file << L" to " << output_folder << endl;

        int          result = ERROR_INSTALL_FAILURE;
        IMsiDumpCab* msi    = MsiDumpCreateObject();
        if (msi == NULL)
        {
                trace_error << L"internal error" << endl;
                return result;
        }
        else if (!msi->Open(input_file))
        {
                trace_error << L"Open msi package: " << input_file << endl;
                msi->Release();
                return result;
        }
        else if (!RemoveFolderRecursively(output_folder))
        {
                ;
        }
        else if (!msi->ExtractTo(output_folder, SELECT_ALL_ITEMS, EXTRACT_TO_FLAT_FOLDER))
        {
                trace_error << L"Extract msi file" << endl;
        }
        else if (CompareFolder(source_folder, output_folder))
        {
                trace << L"Test passes" << endl;
                result = ERROR_SUCCESS;
        }

        msi->Close();
        msi->Release();
        return result;
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
        bool         valid;

public:
        Folder(wstring Path)
        {
                DWORD           attr;
                DWORD           error;
                HANDLE          find_handle;
                WIN32_FIND_DATA find_data;

                valid = false;
                attr  = GetFileAttributes(Path.c_str());
                if (attr == INVALID_FILE_ATTRIBUTES || !TEST_FLAG(attr, FILE_ATTRIBUTE_DIRECTORY))
                {
                        trace_error << Path << " is not a directory" << endl;
                        return;
                }

                path        = Path;
                find_handle = FindFirstFile(Path.append(L"\\*").c_str(), &find_data);
                if (find_handle == INVALID_HANDLE_VALUE)
                {
                        error = GetLastError();
                        trace_error << "FindFirstFile(" << path << L"\\*) failed with " << error << endl;
                        return;
                }
                valid = true;

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

                } while (FindNextFile(find_handle, &find_data) != 0);

                FindClose(find_handle);
        }

        bool Equals(Folder& that)
        {
                if (!valid || !that.valid)
                {
                        return false;
                }

                // Verify the two folders have the same number of child entries
                //
                if (files.size() != that.files.size())
                {
                        trace_error << L"File counts in two folders are different:" << endl
                                    << path << L" : " << files.size() << endl
                                    << that.path << L" : " << that.files.size() << endl;
                        return false;
                }

                for (auto p = files.begin(); p != files.end(); p++)
                {
                        // Verify the same file name exists in both folders
                        //
                        wstring name = p->name;
                        auto    q    = that.files.begin();
                        for (; q != that.files.end() && name != q->name; q++)
                                ;
                        if (q == that.files.end())
                        {
                                trace_error << name << L" exits in " << path << L" but not in " << that.path << endl;
                                return false;
                        }

                        wstring this_child = path + L"\\" + name;
                        wstring that_child = that.path + L"\\" + name;

                        if (p->is_folder != q->is_folder)
                        {
                                trace_error << L"One is a folder, but the other is not: " << this_child << ", " << that_child << endl;
                                return false;
                        }

                        if (p->is_folder)
                        {
                                // recursively compare sub-folders
                                //
                                Folder this_child_folder(this_child);
                                Folder that_child_folder(that_child);
                                if (!this_child_folder.Equals(that_child_folder))
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
                                                    << this_child << L" : " << p->size.QuadPart << endl
                                                    << that_child << L" : " << q->size.QuadPart << endl;
                                        return false;
                                }
                                if (!CompareFile(this_child.c_str(), that_child.c_str(), (size_t)p->size.QuadPart))
                                {
                                        return false;
                                }
                        }
                }
                return true;
        }

        bool Remove()
        {
                DWORD error;

                if (!valid)
                {
                        return false;
                }

                for (auto p = files.begin(); p != files.end(); p++)
                {
                        wstring child_path = path + L"\\" + p->name;

                        if (p->is_folder)
                        {
                                Folder childFolder(child_path);
                                if (!childFolder.Remove())
                                {
                                        return false;
                                }
                        }
                        else
                        {
                                if (!DeleteFile(child_path.c_str()))
                                {
                                        error = GetLastError();
                                        trace_error << error << L": DeleteFile: " << child_path << endl;
                                        return false;
                                }
                        }
                }

                if (!RemoveDirectory(path.c_str()))
                {
                        error = GetLastError();
                        trace_error << error << L": RemoveDirectory: " << path << endl;
                        return false;
                }
                return true;
        }
};

bool CompareFolder(PCWSTR path1, PCWSTR path2)
{
        trace << L"Compare " << path1 << L" with " << path2 << endl;
        Folder folder1(path1);
        Folder folder2(path2);
        return folder1.Equals(folder2);
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
        DWORD attributes = GetFileAttributes(path);
        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
                DWORD error = GetLastError();
                if (error == ERROR_FILE_NOT_FOUND)
                {
                        // the folder does not exist. consider remove folder finishes successfully
                        return true;
                }
                trace_error << error << L": GetFileAttributes: " << path << endl;
                return false;
        }

        Folder folder(path);
        return folder.Remove();
}
