@echo off
setlocal

if "%1" == "-?"        goto :usage
if "%1" == "-cleanup"  goto :cleanup
if "%1" == "-build"    goto :build
if "%1" == "-release"  goto :release
if "%1" == "-srcindex" goto :srcindex
if "%1" == "-binplace" goto :binplace
if "%1" == "-robocopy" goto :robocopy
goto :usage

rem ==============================================================================
:usage
echo.
echo   -?          display this message
echo   -cleanup    remove generated files completely
echo   -build      build
echo   -release    copy files to release server
echo.
echo you need to do the following:
echo.
echo   repeat {
echo     -cleanup
echo     build all favors (x86, amd64, ia64)
echo   } until (everything is ok);
echo.
echo   sd.exe submit changes
echo.
echo   -release()
echo   {
echo      -srcindex
echo      -binplace
echo      -robocopy
echo   }
goto :end

rem ==============================================================================
:cleanup
for %%i in (wxp_x86 wnet_AMD64 wnet_IA64) DO (
  for /r %%j in (objchk) do rd /s/q %%j_%%i 2>nul
)
del /s/q build*.log trace.txt 2>nul
rd /s/q bin >nul 2>nul
goto :end

rem ==============================================================================
:build
echo.
echo please start multiple build window, e.g. x86, amd64, ia64 checked, and "bcz"
echo.
echo after that, run -release 
echo.
goto :end

rem ==============================================================================
:srcindex
echo .. srcindex
set sdport=weimao1:2222
set perl_root=c:\winddk\perl
set ssindex_root=c:\debuggers\sdk\srcsrv
path %perl_root%\bin;%path%
call %ssindex_root%\sdindex.cmd >nul
goto :end

rem ==============================================================================
:binplace
rd /s/q bin >nul 2>nul
md bin

for /r %%i in (i386) do (
  xcopy %%i\*.exe bin\x86\ >nul 2>nul
  xcopy %%i\*.pdb bin\x86\ >nul 2>nul
)

for /r %%i in (amd64) do (
  xcopy %%i\*.exe bin\amd64\ >nul 2>nul
  xcopy %%i\*.pdb bin\amd64\ >nul 2>nul
)

for /r %%i in (ia64) do (
  xcopy %%i\*.exe bin\ia64\ >nul 2>nul
  xcopy %%i\*.pdb bin\ia64\ >nul 2>nul
)
goto :end

rem ==============================================================================
:robocopy
echo .. robocopy
set SRC_RELEASE_POINT=\\weimao1\public\msi
set TOOLBOX_RELEASE_POINT=\\TKFilToolBox\Tools\20864

robocopy .   %SRC_RELEASE_POINT%     /mir /xd objchk* >nul
robocopy bin %TOOLBOX_RELEASE_POINT% /mir >nul
robocopy .   %TOOLBOX_RELEASE_POINT% *.txt >nul

copy /y bin\x86 %windir% >nul
goto :end

rem ==============================================================================
:release
call :srcindex
call :binplace
call :robocopy
goto :end

rem ==============================================================================
:end
endlocal
