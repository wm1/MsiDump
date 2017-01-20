#include "precomp.h"

bool ParseOption(PCWSTR option);
bool FindOptionWithValue(PCWSTR option, PCWSTR name, PCWSTR* value);
bool IsSeparator(WCHAR c);

CommandArgs Args;

//
// Usage: MsiDump.exe command [options] msiFile [path_to_extract]
//
void ParseArgs(int argc, PCWSTR argv[])
{
        // default command and options
        //
        Args.command = CMD_INVALID;

        if (argc == 1)
        {
                Args.command = CMD_HELP;
                return;
        }

        PCWSTR command = argv[1];

        //
        // check whether the first argument start with '-'.
        // if so, it is new syntax, else it is legacy syntax
        //
        if (!IsSeparator(command[0]))
        {
                // legacy syntax
                //
                if (argc == 2)
                {
                        Args.command            = CMD_LIST;
                        Args.file_name          = argv[1];
                        Args.u.list.list_format = L"nfsp";
                }
                else if (argc == 3 || argc == 4)
                {
                        Args.command                        = CMD_EXTRACT;
                        Args.file_name                      = argv[1];
                        Args.u.extract.path_to_extract      = argv[2];
                        Args.u.extract.is_extract_full_path = true;
                        if (argc == 4)
                        {
                                PCWSTR arg3 = argv[3];
                                if (IsSeparator(arg3[0]) && arg3[1] == L'f')
                                        Args.u.extract.is_extract_full_path = false;
                        }
                }
                return;
        }

        //
        // new syntax
        //
        if (wcscmp(&command[1], L"list") == 0)
        {
                Args.command            = CMD_LIST;
                Args.u.list.list_format = L"nfsp";
        }
        else if (wcscmp(&command[1], L"extract") == 0)
        {
                Args.command                        = CMD_EXTRACT;
                Args.u.extract.is_extract_full_path = true;
        }
        else if (wcscmp(&command[1], L"help") == 0 || wcscmp(&command[1], L"h") == 0 || wcscmp(&command[1], L"?") == 0)
        {
                Args.command = CMD_HELP;
                return;
        }
        else
        {
                fwprintf(stderr, L"error: unrecognized command %s\n", command);
                return;
        }

        int current_arg = 2;
        while (current_arg < argc && IsSeparator(argv[current_arg][0]))
        {
                if (!ParseOption(argv[current_arg]))
                {
                        fwprintf(stderr, L"error: unrecognized parameter %s\n", argv[current_arg]);
                        Args.command = CMD_INVALID;
                        return;
                }
                current_arg++;
        }

        if (current_arg >= argc)
        {
                fwprintf(stderr, L"error: msi file is not supplied\n");
                Args.command = CMD_INVALID;
                return;
        }
        Args.file_name = argv[current_arg];
        current_arg++;

        if (Args.command == CMD_EXTRACT)
        {
                if (current_arg >= argc)
                {
                        fwprintf(stderr, L"error: path to extract is not supplied\n");
                        Args.command = CMD_INVALID;
                        return;
                }
                Args.u.extract.path_to_extract = argv[current_arg];
                current_arg++;
        }
}

bool IsSeparator(WCHAR c)
{
        return (c == L'-' || c == L'/');
}

//
// option ::= separator name [colon value]
//
// return the 'value' string after the colon mark if 'name' matches
//
bool FindOptionWithValue(PCWSTR option, PCWSTR name, PCWSTR* value)
{
        option++; // skip leading separator

        size_t name_count = wcslen(name);
        if (wcsncmp(option, name, name_count) == 0)
        {
                size_t option_count = wcslen(option);
                if (option_count > name_count && option[name_count] == L':')
                {
                        *value = &option[name_count + 1];
                        return true;
                }
        }
        return false;
}

bool ParseOption(PCWSTR option)
{
        if (Args.command == CMD_LIST)
        {
                PCWSTR list_format;
                if (FindOptionWithValue(option, L"format", &list_format))
                {
                        Args.u.list.list_format = list_format;
                        size_t len              = wcslen(list_format);
                        for (size_t i = 0; i < len; i++)
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
        else if (Args.command == CMD_EXTRACT)
        {
                PCWSTR is_extract_full_path;
                if (FindOptionWithValue(option, L"full_path", &is_extract_full_path))
                {
                        if (wcscmp(is_extract_full_path, L"no") == 0)
                                Args.u.extract.is_extract_full_path = false;
                        else if (wcscmp(is_extract_full_path, L"yes") == 0)
                                Args.u.extract.is_extract_full_path = true;
                        else
                        {
                                fwprintf(stderr, L"error: unrecognized specifier \"%s\" in %s\n", is_extract_full_path, option);
                                return false;
                        }
                }
                else
                        return false;
        }
        return true;
}

void Usage(PCWSTR exe)
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

void ListHeader()
{
        PCWSTR format = Args.u.list.list_format;
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

void ListRow(int num, MsiDumpFileDetail* detail)
{
        PCWSTR format = Args.u.list.list_format;
        while (*format != 0)
        {
                switch (*format)
                {
                case L'n':
                        wprintf(L"%4d", num);
                        break;
                case L'f':
                        wprintf(L"%15s", detail->file_name);
                        break;
                case L's':
                        wprintf(L"%9d", detail->file_size);
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
