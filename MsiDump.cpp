
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "MsiDumpPublic.h"
#include "parseArgs.h"

int __cdecl
_tmain(int argc, LPCTSTR argv[])
{
	parseArgs(argc, argv);
	if(args.cmd == cmd_invalid)
	{
		LPCTSTR exe = argv[0];
		usage(exe);
		return E_INVALIDARG;
	}

	IMsiDumpCab *msi = MsiDumpCreateObject();
	if(!msi->Open(args.filename))
	{
		msi->Release();
		return ERROR_INSTALL_PACKAGE_OPEN_FAILED;
	}

	int retVal = ERROR_SUCCESS;

	if(args.cmd == cmd_list)
	{
		listHeader();
		int count = msi->getCount();
		for(int i=0; i<count; i++)
		{
			MsiDumpFileDetail detail;
			msi->GetFileDetail(i, &detail);
			listRecord(i, &detail);
		}
	}
	else if(args.cmd == cmd_extract)
	{
		bool flatFolder = !args.extract_full_path;

		TCHAR filename[MAX_PATH];
		GetFullPathName(args.path_to_extract, MAX_PATH, filename, NULL);
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
