#include <windows.h>
#include <stdio.h>

#include "MsiDumpPublic.h"
#include "parseArgs.h"

bool parseOption(PCWSTR option);
bool findOptionWithValue(PCWSTR option, PCWSTR name, PCWSTR* value);
bool isSeparator(WCHAR c);

_args args;

//
// Usage: MsiDump.exe command [options] msiFile [path_to_extract]
//
void parseArgs(int argc, PCWSTR argv[])
{
        // default command and options
        //
        args.cmd               = cmd_invalid;
        args.list_format       = L"nfsp";
        args.extract_full_path = true;

        if (argc == 1)
        {
                args.cmd = cmd_help;
                return;
        }

        PCWSTR cmd = argv[1];

        //
        // check whether the first argument start with '-'.
        // if so, it is new syntax, else it is legacy syntax
        //
        if (!isSeparator(cmd[0]))
        {
                // legacy syntax
                //
                if (argc == 2)
                {
                        args.cmd      = cmd_list;
                        args.filename = argv[1];
                }
                else if (argc == 3 || argc == 4)
                {
                        args.cmd             = cmd_extract;
                        args.filename        = argv[1];
                        args.path_to_extract = argv[2];
                        if (argc == 4)
                        {
                                PCWSTR arg3 = argv[3];
                                if (isSeparator(arg3[0]) && arg3[1] == L'f')
                                        args.extract_full_path = false;
                        }
                }
                return;
        }

        //
        // new syntax
        //
        if (wcscmp(&cmd[1], L"list") == 0)
        {
                args.cmd = cmd_list;
        }
        else if (wcscmp(&cmd[1], L"extract") == 0)
        {
                args.cmd = cmd_extract;
        }
        else if (wcscmp(&cmd[1], L"help") == 0 || wcscmp(&cmd[1], L"h") == 0 || wcscmp(&cmd[1], L"?") == 0)
        {
                args.cmd = cmd_help;
                return;
        }
        else
        {
                fwprintf(stderr, L"error: unrecognized command %s\n", cmd);
                return;
        }

        int current_arg = 2;
        while (current_arg < argc && isSeparator(argv[current_arg][0]))
        {
                if (!parseOption(argv[current_arg]))
                {
                        fwprintf(stderr, L"error: unrecognized parameter %s\n", argv[current_arg]);
                        args.cmd = cmd_invalid;
                        return;
                }
                current_arg++;
        }

        if (current_arg >= argc)
        {
                fwprintf(stderr, L"error: msi file is not supplied\n");
                args.cmd = cmd_invalid;
                return;
        }
        args.filename = argv[current_arg];
        current_arg++;

        if (args.cmd == cmd_extract)
        {
                if (current_arg >= argc)
                {
                        fwprintf(stderr, L"error: path to extract is not supplied\n");
                        args.cmd = cmd_invalid;
                        return;
                }
                args.path_to_extract = argv[current_arg];
                current_arg++;
        }
}

bool isSeparator(WCHAR c)
{
        return (c == L'-' || c == L'/');
}

//
// option ::= separator name [colon value]
//
// return the 'value' string after the colon mark if 'name' matches
//
bool findOptionWithValue(PCWSTR option, PCWSTR name, PCWSTR* value)
{
        option++; // skip leading separator

        size_t cName = wcslen(name);
        if (wcsncmp(option, name, cName) == 0)
        {
                size_t cOption = wcslen(option);
                if (cOption > cName && option[cName] == L':')
                {
                        *value = &option[cName + 1];
                        return true;
                }
        }
        return false;
}

bool parseOption(PCWSTR option)
{
        if (args.cmd == cmd_list)
        {
                PCWSTR list_format;
                if (findOptionWithValue(option, L"format", &list_format))
                {
                        args.list_format = list_format;
                        size_t len       = wcslen(list_format);
                        for (int i = 0; i < len; i++)
                        {
                                WCHAR c = list_format[i];
                                if (wcschr(L"nfspvl", c) == NULL)
                                {
                                        fwprintf(stderr, L"error: unrecognized char \"%c\" in %s\n", c, option);
                                        return false;
                                }
                        }
                }
                else
                        return false;
        }
        else if (args.cmd == cmd_extract)
        {
                PCWSTR extract_full_path;
                if (findOptionWithValue(option, L"full_path", &extract_full_path))
                {
                        if (wcscmp(extract_full_path, L"no") == 0)
                                args.extract_full_path = false;
                        else if (wcscmp(extract_full_path, L"yes") == 0)
                                args.extract_full_path = true;
                        else
                        {
                                fwprintf(stderr, L"error: unrecognized specifier \"%s\" in %s\n", extract_full_path, option);
                                return false;
                        }
                }
                else
                        return false;
        }
        return true;
}

void usage(PCWSTR exe)
{
        wprintf(L"Usage:\n"
                L"  %s command [options] msiFile [path_to_extract]\n"
                L"\n"
                L"  commands (use one of them):\n"
                L"    -list                 list msiFile\n"
                L"    -extract              extract files\n"
                L"    -help                 this help secreen\n"
                L"\n"
                L"  options for -list:\n"
                L"    -format:nfspvl        list num, file, size, path, version, lang (DEFAULT:nfsp)\n"
                L"\n"
                L"  options for -extract:\n"
                L"    -full_path:yes|no     extract files with full path (DEFAULT:yes)\n"
                L"\n",
                exe);

        wprintf(L"Legacy Usage:\n"
                L"  %s msiFile                  - list\n"
                L"  %s msiFile extractPath      - extract\n"
                L"  %s msiFile extractPath -f   - extract -full_path:no\n",
                exe, exe, exe);
}

void listHeader()
{
        PCWSTR format = args.list_format;
        while (*format != 0)
        {
                switch (*format)
                {
                case L'n':
                        wprintf(L"%4s", L"num");
                        break;
                case L'f':
                        wprintf(L"%15s", L"filename");
                        break;
                case L's':
                        wprintf(L"%9s", L"filesize");
                        break;
                case L'p':
                        wprintf(L"%-45s", L"path");
                        break;
                case L'v':
                        wprintf(L"%15s", L"version");
                        break;
                case L'l':
                        wprintf(L"%9s", L"language");
                        break;
                }
                wprintf(L" ");
                format++;
        }
        wprintf(L"\n");
}

void listRecord(int num, MsiDumpFileDetail* detail)
{
        PCWSTR format = args.list_format;
        while (*format != 0)
        {
                switch (*format)
                {
                case L'n':
                        wprintf(L"%4d", num);
                        break;
                case L'f':
                        wprintf(L"%15s", detail->filename);
                        break;
                case L's':
                        wprintf(L"%9d", detail->filesize);
                        break;
                case L'p':
                        wprintf(L"%-45s", detail->path);
                        break;
                case L'v':
                        wprintf(L"%15s", detail->version);
                        break;
                case L'l':
                        wprintf(L"%9s", detail->language);
                        break;
                }
                wprintf(L" ");
                format++;
        }
        wprintf(L"\n");
}
