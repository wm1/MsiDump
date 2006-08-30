
#pragma once

void parseArgs(int argc, LPCTSTR argv[]);
void usage(LPCTSTR exe);
void listHeader();
void listRecord(int num, MsiDumpFileDetail* detail);

enum _cmds
{
	cmd_list,
	cmd_extract,
	cmd_help,
	cmd_invalid
};

struct _args {
	
	// command
	//
	_cmds cmd;
	
	// options for cmd_list
	//
	LPCTSTR list_format;

	// options for cmd_extract
	//
	bool extract_full_path;
	
	// archive file name
	//
	LPCTSTR filename;
	
	// path_to_extract
	//
	LPCTSTR path_to_extract;
};

extern _args args;

