
You can use either SDK / VStudio / DDK / Razzle to build the source code

1. Download Windows Template Library (WTL) 7.1 from:
   http://www.microsoft.com/downloads/details.aspx?FamilyId=1BE1EB52-AA96-4685-99A5-4256737781C5&displaylang=en

2. For SDK: Install WTL to $(MSVCDIR)\atlmfc\wtl71, and type "nmake.exe" to build

3. For Vistual Studio .NET 2003: Install WTL to $(MSVCDIR)\atlmfc\wtl71, open *.vcproj to build

4. For DDK: Install WTL to \ddk\3790\inc\wtl71, and type "build.exe"

5. For Razzle: Install WTL to \nt\public\sdk\inc\wtl71, and type "build.exe"

   Note:      DDK/Razzle only build the command line version of the tool (MsiDump.cab).
   Copy sources.gui over sources if you want to build the gui version (MsiDumpCab.exe).
