
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "MsiDumpPublic.h"

int __cdecl
_tmain(int argc, LPCTSTR argv[])
{
	if(argc != 2 && argc != 3 && argc != 4)
	{
		_tprintf(TEXT("Usage:\n")
			TEXT("  %s msiFile                  - list files in the installer package\n")
			TEXT("  %s msiFile extractPath      - extract files to specified folder\n")
			TEXT("  %s msiFile extractPath -f   - extract files flatly\n"),
			argv[0], argv[0], argv[0]
			);
		return E_INVALIDARG;
	}

	IMsiDumpCab *msi = MsiDumpCreateObject();
	if(!msi->Open(argv[1]))
	{
		msi->Release();
		return ERROR_INSTALL_PACKAGE_OPEN_FAILED;
	}

	int retVal = ERROR_SUCCESS;
	if(argc == 2)
	{
		_tprintf(TEXT(" Num %15s %9s %s\n"), TEXT("filename"), TEXT("filesize"), TEXT("path"));
		int count = msi->getCount();
		for(int i=0; i<count; i++)
		{
			MsiDumpFileDetail detail;
			msi->GetFileDetail(i, &detail);
			_tprintf(TEXT("%4d %15s %9d %s\n"), i, 
				detail.filename, detail.filesize, detail.path);
		}

	} else if(argc == 3 || argc == 4)
	{
		bool flatFolder = false;
		LPCTSTR arg3 = argv[3];
		if(argc == 4)
			flatFolder = ((arg3[0] == TEXT('-') || arg3[0] == TEXT('/')) &&
				(arg3[1] == TEXT('f') || arg3[1] == TEXT('F')));
		TCHAR filename[MAX_PATH];
		GetFullPathName(argv[2], MAX_PATH, filename, NULL);
		bool b = msi->ExtractTo(filename, true, flatFolder);
		if(!b)
		{
			_tprintf(TEXT("Fail to extract msi file. check out trace.txt for details\n"));
			retVal = ERROR_INSTALL_FAILURE;
		}
	}

	msi->Close();
	msi->Release();
	return retVal;
}
