
MsiDumpCab
Extract files out of .msi package 

Sometimes you want the content of a Windows Installer (.msi) package without actually installing it...

Here comes MsiDumpCab.exe with friendly UI like WinZip; or use the command line version (MsiDump.exe).

Writing your own UI is easy: the interface between UI and backend is published in MsiDumpPublic.h (in the source tree), and you can take MsiDump.cpp as a starting point.

Usage:
  MsiDump.exe msiFile                  - list files in the installer package
  MsiDump.exe msiFile extractPath      - extract files to specified folder
  MsiDump.exe msiFile extractPath -f   - extract files flatly

Note 1: this is a work-in-progress.  It should work with most msi file you might meet; however there is known issue with Office msi package on \\products server.

Note 2: it may take quite some time for the listview window to be completely filled in if the msi file is big.  It is normal.

Note 3: a trace.txt is created in the current directory.  Souce code is published at \\weimao2\public\msi for diagnostic. Symbols in \\weimao2\public\msi\bin

mailto:weimao for feedback / suggstion. Please give me a link to the .msi file as well as trace.txt file (in the current folder)
