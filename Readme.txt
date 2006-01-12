
MsiDumpCab
Extract files out of .msi package 

Sometimes you want the content of a Windows Installer (.msi) package without actually installing it. Here comes MsiDumpCab.exe with friendly UI like WinZip; or use the command line version (MsiDump.exe)

Usage 1:
  MsiDumpCab.exe [msiFile]

Usage 2:
  MsiDump.exe command [options] msiFile [path_to_extract]

  commands:
    -list                 list msiFile
    -extract              extract files

  options for -list:
    -format:nfspv         list num, file, size, path, version (DEFAULT:nfsp)

  options for -extract:
    -full_path:yes|no     extract files with full path (DEFAULT:yes)

Usage 3 (legacy):
  MsiDump.exe msiFile [extractPath [-f]]

Note 1: Not all files are necessarily embedded inside msi file; instead, a pointer may be embedded and actual files are stored in normal file system. With -extract command, these companioning files must be present at the same location as msi file is. This is not a requirement for -list command though. The same applies to GUI version "Extract files" and "Export file list" commands

Note 2: [GUI version only] For very large msi file (e.g. Office 2003) you'll notice that the list view is filled in two stages, i.e. the file name column shows almost immediately; however it takes quite some time for the path column to be filled. It is normal

Note 3: Would you meet any problem, first please look at "trace.txt" at the current folder for any obvious error; next you can get the symbol / source files at \\TKFilToolBox\Tools\20864 and debug it yourself; if it does not work for you, mailto:msidump with a link to the .msi file and trace.txt
