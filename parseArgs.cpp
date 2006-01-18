#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "MsiDumpPublic.h"
#include "parseArgs.h"

bool parseOption(LPCTSTR option);
bool findOptionWithValue(LPCTSTR option, LPCTSTR name, LPCTSTR *value);
bool isSeparator(TCHAR c);

_args args;

//
// Usage: MsiDump.exe command [options] msiFile [path_to_extract]
//
void parseArgs(int argc, LPCTSTR argv[])
{
	// default command and options
	//
	args.cmd = cmd_invalid;
	args.list_format = TEXT("nfsp");
	args.extract_full_path = true;

	if(argc == 1)
	{
		args.cmd = cmd_help;
		return;
	}
	
	LPCTSTR cmd = argv[1];
	
	//
	// check whether the first argument start with '-'. 
	// if so, it is new syntax, else it is legacy syntax
	//
	if(!isSeparator(cmd[0]))
	{
		// legacy syntax
		//
		if(argc == 2)
		{
			args.cmd = cmd_list;
			args.filename = argv[1];
		} 
		else if(argc == 3 || argc == 4)
		{
			args.cmd = cmd_extract;
			args.filename = argv[1];
			args.path_to_extract = argv[2];
			if(argc == 4)
			{
				LPCTSTR arg3 = argv[3];
				if(isSeparator(arg3[0]) && arg3[1] == TEXT('f'))
					args.extract_full_path = false;
			}
		}
		return;
	}
	
	//
	// new syntax
	//
	if(_tcscmp(&cmd[1], TEXT("list")) == 0)
	{
		args.cmd = cmd_list;
	}
	else if(_tcscmp(&cmd[1], TEXT("extract")) == 0)
	{
		args.cmd = cmd_extract;
	}
	else if(_tcscmp(&cmd[1], TEXT("help")) == 0
		|| _tcscmp(&cmd[1], TEXT("h")) == 0
		|| _tcscmp(&cmd[1], TEXT("?")) == 0)
	{
		args.cmd = cmd_help;
		return;
	}
	else
	{
		_ftprintf(stderr, TEXT("error: unrecognized command %s\n"), cmd);
		return;
	}
	
	int current_arg = 2;
	while(current_arg < argc && isSeparator(argv[current_arg][0]))
	{
		if(!parseOption(argv[current_arg]))
		{
			_ftprintf(stderr, TEXT("error: unrecognized parameter %s\n"), argv[current_arg]);
			args.cmd = cmd_invalid;
			return;
		}
		current_arg++;
	}
	
	if(current_arg >= argc)
	{
		_ftprintf(stderr, TEXT("error: msi file is not supplied\n"));
		args.cmd = cmd_invalid;
		return;
	}
	args.filename = argv[current_arg];
	current_arg++;
	
	if(args.cmd == cmd_extract)
	{
		if(current_arg >= argc)
		{
			_ftprintf(stderr, TEXT("error: path to extract is not supplied\n"));
			args.cmd = cmd_invalid;
			return;
		}
		args.path_to_extract = argv[current_arg];
		current_arg++;
	}
}

bool isSeparator(TCHAR c)
{
	return (c == TEXT('-') || c == TEXT('/'));
}

//
// option ::= separator name [colon value]
//
// return the 'value' string after the colon mark if 'name' matches
//
bool findOptionWithValue(LPCTSTR option, LPCTSTR name, LPCTSTR *value)
{
	option ++; // skip leading separator
	
	int cName = _tcslen(name);
	if(_tcsncmp(option, name, cName) == 0)
	{
		int cOption = _tcslen(option);
		if(cOption > cName && option[cName] == TEXT(':'))
		{
			*value = &option[cName+1];
			return true;
		}
	}
	return false;
}

bool parseOption(LPCTSTR option)
{
	if(args.cmd == cmd_list)
	{
		LPCTSTR list_format;
		if(findOptionWithValue(option, TEXT("format"), &list_format))
		{
			args.list_format = list_format;
			int len = _tcslen(list_format);
			for(int i=0; i<len; i++)
			{
				TCHAR c = list_format[i];
				if(_tcschr(TEXT("nfspv"), c) == NULL)
				{
					_ftprintf(stderr, TEXT("error: unrecognized char \"%c\" in %s\n"), c, option);
					return false;
				}
			}
		} else
			return false;
	}
	else if(args.cmd == cmd_extract)
	{
		LPCTSTR extract_full_path;
		if(findOptionWithValue(option, TEXT("full_path"), &extract_full_path))
		{
			if(_tcscmp(extract_full_path, TEXT("no")) == 0)
				args.extract_full_path = false;
			else if(_tcscmp(extract_full_path, TEXT("yes")) == 0)
				args.extract_full_path = true;
			else
			{
				_ftprintf(stderr, TEXT("error: unrecognized specifier \"%s\" in %s\n"), extract_full_path, option);
				return false;
			}
		} else
			return false;
	}
	return true;
}

void usage(LPCTSTR exe)
{
	_tprintf(TEXT("Usage:\n")
		TEXT("  %s command [options] msiFile [path_to_extract]\n")
		TEXT("\n")
		TEXT("  commands (use one of them):\n")
		TEXT("    -list                 list msiFile\n")
		TEXT("    -extract              extract files\n")
		TEXT("    -help                 this help secreen\n")
		TEXT("\n")
		TEXT("  options for -list:\n")
		TEXT("    -format:nfspv         list num, file, size, path, version (DEFAULT:nfsp)\n")
		TEXT("\n")
		TEXT("  options for -extract:\n")
		TEXT("    -full_path:yes|no     extract files with full path (DEFAULT:yes)\n")
		TEXT("\n")
		,
		exe);
	
	_tprintf(TEXT("Legacy Usage:\n")
			TEXT("  %s msiFile                  - list\n")
			TEXT("  %s msiFile extractPath      - extract\n")
			TEXT("  %s msiFile extractPath -f   - extract -full_path:no\n"),
			exe, exe, exe
			);
}

void listHeader()
{
	LPCTSTR format = args.list_format;
	while(*format != 0)
	{
		switch(*format)
		{
		case TEXT('n'): _tprintf(TEXT(  "%4s"), TEXT("num"     )); break;
		case TEXT('f'): _tprintf(TEXT( "%15s"), TEXT("filename")); break;
		case TEXT('s'): _tprintf(TEXT(  "%9s"), TEXT("filesize")); break;
		case TEXT('p'): _tprintf(TEXT("%-45s"), TEXT("path"    )); break;
		case TEXT('v'): _tprintf(TEXT( "%15s"), TEXT("version" )); break;
		}
		_tprintf(TEXT(" "));
		format++;
	}
	_tprintf(TEXT("\n"));
}

void listRecord(int num, MsiDumpFileDetail* detail)
{
	LPCTSTR format = args.list_format;
	while(*format != 0)
	{
		switch(*format)
		{
		case TEXT('n'): _tprintf(TEXT(  "%4d"), num             ); break;
		case TEXT('f'): _tprintf(TEXT( "%15s"), detail->filename); break;
		case TEXT('s'): _tprintf(TEXT(  "%9d"), detail->filesize); break;
		case TEXT('p'): _tprintf(TEXT("%-45s"), detail->path    ); break;
		case TEXT('v'): _tprintf(TEXT( "%15s"), detail->version ); break;
		}
		_tprintf(TEXT(" "));
		format++;
	}
	_tprintf(TEXT("\n"));
}

