
#pragma once

void parseArgs(int argc, PCWSTR argv[]);
void usage(PCWSTR exe);
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
        PCWSTR list_format;

        // options for cmd_extract
        //
        bool extract_full_path;

        // archive file name
        //
        PCWSTR filename;

        // path_to_extract
        //
        PCWSTR path_to_extract;
};

extern _args args;
