# built in Windows Vista DDK environment
#
# external dependency:
#
#  . Vista SDK
#  . WTL 8.0
#

!include ..\version.inc
RCOPTIONS = /dPROJECT_VERSION=$(PROJECT_VERSION) 

SOURCES =                   \
	..\MsiUtils.cpp     \
	..\MsiTable.cpp

# required for <msiquery.h>
#
#PSDK="C:\Program Files\Microsoft SDKs\Windows\v6.0"
PSDK=C:\PROGRA~1\MI2578~1\Windows\v6.0

INCLUDES = ..;$(PSDK)\include

# the local copy of WTL is saved at c:\my\wtl
#
USE_STL=1
USE_WTL=1
WTL_INC_PATH=c:\my

C_DEFINES = $(C_DEFINES) /DUNICODE /D_UNICODE

# comment out the following line to disable tracing
C_DEFINES = $(C_DEFINES) /DENABLE_TRACE=1

# add /debugtype for office profiler
LINKER_FLAGS = -debugtype:cv,fixup

USE_NATIVE_EH = 1
USE_MSVCRT    = 1
MFC_INCLUDES  =
