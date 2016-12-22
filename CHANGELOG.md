## 2016-11-23
 - Import the code to git
 - Add msbuild project files
 - Move util/table cpp to a static lib project
 - Use wstring and L"" string literals explicitly

## Update: 2008/3/26
* UI: Add "X64" to "platform" column
* ISSUE: the exe size issue (increased from 36K to 70K for console version) is due to STL 7.  No workaround.
  // background: STL 6 depends on msvcp60.dll, while STL 7 is completely inline and has no associated DLL)

## Update: 2007/8/28
* ISSUE: Built with VC 8.0. The exe size is increased by 200%

## Update: 2006/8/30
* UI: Add "language" column
* Change: port to DDK build environment.

## Update: 2005/12/19
* Change: Rewrite argument parsing portion in msidump.exe
* Change: Remove other build methods except for Makefile approach
* UI: Add "version" column

## Update: 2005/7/12
* UI: use virtual listview to improve startup time

## Update: 2005/3/23
* Fix: handle path separator more consistently
* Fix: mstsmmc.dll issue on 2003/11/08 below: case insensitive compare in MsiUtils::LocateFile
* Change: IMsiDumpCab::ExtractTo() now returns bool
* Improve: more diagnositic output in trace.txt file

## Update: 2004/10/5
* Fix: fail to open UNC file

## Update: 2004/3/30
* Add sources file to build in razzle window

## Update: 2004/3/16
* Add -f option for msidump.exe to extract files to just one directory
* Add .vcproj so that it can be built within VS.NET GUI. See Readme.build.txt

## Update: 2003/11/08
* Extract mstsmmc.dll from win2k3 adminpak.msi fails.  the resulting file is corrupted

## Update: 2003/09/05
* UI: Add "Type" column
* Performance: Use table's primary key when couting rows
* Performance: Use "SELECT .. ORDER BY Sequence" instead of doing qsort myself

## Update: 2003/08/24
* Port basic command line version to to .NET (MsiUtils.cs MsiTable.cs PInvoke.cs)

## Update: 2003/08/22
* Add IDropSource & IDataObject support: now you can drag files to explorer

## Update: 2003/08/20
* Fix delay when (de)select all
* Add a "Platform" column to distiguish two files with identical name & target path

## Update: 2003/08/15
* Add statusbar, UI update. Polish UI
* Check input file / dest path more strictly

## Update: 2003/08/8
* Add file drag-and-drop, accelerator, and context menu support
* Add Readme.txt file

## Update: 2003/07/28
* Rewrite the backend.
* Support multiple .cab files
* Command line version added (test.exe)
* Process log can be found in trace.txt

## Update: 2002/06/03
* Add "export filelist"
* Add support to seperated cab files (e.g. not stored within .msi as a stream)

## Update: 2002/05/22
* Handle column sorting

## Update: 2002/05/20
* Rewrite UI to use WTL/ATL

## Update: 2002/04/03
* Feature completed.  Now it has a WinZip-like main window, two menu items
(Open .msi file and Extract to specified folder)

## Update: 2002/03/26
* `e:\temp> i2.exe test.msi`
* Show a WinZip-like window with filename, size and path information.

## Update: 2002/02/05
* Sometimes you want the content of a Windows Installer (.msi) package without
actually installing it...
```
e:\temp> MsiDumpCab.exe test.msi
Open test.msi ...
Read Instal01.cab, 249408 bytes ...
Write to disk ...
Write filelist.txt ...
Done.
```
* Now you can open Instal01.cab, filelist.txt and play with it.