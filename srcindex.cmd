@echo off
setlocal
set sdport=weimao1:2222
set perl_root=c:\winddk\perl
set ssindex_root=c:\debuggers\sdk\srcsrv

path %perl_root%\bin;%path%
%ssindex_root%\sdindex.cmd

endlocal