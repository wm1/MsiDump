# built in Windows 2003 SP1 DDK environment
#
# external dependency:
#
#  . visual studio 2003
#  . wtl 7.5
#

!include ..\version.inc
RCOPTIONS = /dPROJECT_VERSION=$(PROJECT_VERSION) /dPROJ=msidump

SOURCES =                   \
	..\MsiUtils.cpp     \
	..\MsiTable.cpp     \
	..\version.rc

# C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\PlatformSDK
# required for <msiquery.h>
#
PSDK=C:\PROGRA~1\MICROS~2.NET\vc7\PLATFO~1

INCLUDES = ..;$(PSDK)\include

# the local copy of WTL v7.5 is saved at c:\my\wtl75
#
USE_STL=1
USE_WTL=1
WTL_INC_PATH=c:\my
WTL_VER=75

C_DEFINES = $(C_DEFINES) /DUNICODE /D_UNICODE

# comment out the following line to disable tracing
C_DEFINES = $(C_DEFINES) /DENABLE_TRACE=1

# add /debugtype for office profiler
LINKER_FLAGS = -debugtype:cv,fixup

USE_NATIVE_EH = 1
USE_MSVCRT    = 1
MFC_INCLUDES  =
