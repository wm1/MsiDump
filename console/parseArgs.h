
#pragma once

void parseArgs(int argc, LPCWSTR argv[]);
void usage(LPCWSTR exe);
void listHeader();
void listRecord(int num, MsiDumpFileDetail* detail);

enum _cmds
{
        cmd_list,
        cmd_extract,
        cmd_help,
        cmd_invalid
};

struct _args
{
        // command
        //
        _cmds cmd;

        // options for cmd_list
        //
        LPCWSTR list_format;

        // options for cmd_extract
        //
        bool extract_full_path;

        // archive file name
        //
        LPCWSTR filename;

        // path_to_extract
        //
        LPCWSTR path_to_extract;
};

extern _args args;
