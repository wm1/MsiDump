
MsiDumpCab
Extract files out of .msi package 

Sometimes you want the content of a Windows Installer (.msi) package without actually installing it...

Here comes MsiDumpCab.exe with friendly UI like WinZip; or use the command line version (MsiDump.exe).

Writing your own UI is easy: the interface between UI and backend is published in MsiDumpPublic.h (in the source tree), and you can take MsiDump.cpp as a starting point.

Usage:
  MsiDump.exe command [options] msiFile [path_to_extract]

  commands:
    -list                 list msiFile
    -extract              extract files

  options for -list:
    -format:nfspv         list num, file, size, path, version (DEFAULE:nfsp)

  options for -extract:
    -full_path:yes|no     extract files with full path (DEFAULT:yes)

Note 1: this is a work-in-progress.  It should work with most msi file you might meet; however there is known issue with Office msi package on \\products server.

Note 2: it may take quite some time for the listview window to be completely filled in if the msi file is big.  It is normal.

Note 3: a trace.txt is created in the current directory.  Souce code is published at \\weimao1\public\msi for diagnostic. Symbols in \\weimao1\public\msi\bin

Note 4: to build the source code, download Windows Template Library (WTL) 7.5 at:
    http://www.microsoft.com/downloads/details.aspx?FamilyID=48CB01D7-112E-46C2-BB6E-5BB2FE20E626&displaylang=en
Install WTL to $(MSVCDIR)\atlmfc\wtl75, and type "nmake.exe" to build.

mailto:weimao for feedback / suggstion. Please give me a link to the .msi file as well as trace.txt file (in the current folder)
