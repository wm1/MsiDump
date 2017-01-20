
#pragma once

void ParseArgs(int argc, PCWSTR argv[]);
void Usage(PCWSTR exe);
void ListHeader();
void ListRow(int num, MsiDumpFileDetail* detail);

enum Commands
{
        CMD_LIST,
        CMD_EXTRACT,
        CMD_HELP,
        CMD_INVALID
};

struct CommandArgs
{
        Commands command;

        PCWSTR file_name;

        union {
                struct
                {
                        PCWSTR list_format;
                } list;

                struct
                {
                        bool   is_extract_full_path;
                        PCWSTR path_to_extract;
                } extract;
        } u;
};

extern CommandArgs Args;
