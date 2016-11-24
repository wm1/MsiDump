!include ..\version.inc

SOURCES =                   \
        ..\MsiUtils.cpp     \
        ..\MsiTable.cpp

INCLUDES = ..

C_DEFINES = $(C_DEFINES) /DUNICODE /D_UNICODE

# comment out the following line to disable tracing
C_DEFINES = $(C_DEFINES) /DENABLE_TRACE=1

RCOPTIONS = /dPROJ=$(PROJ) /dPROJECT_VERSION=$(PROJECT_VERSION) /dPROJECT_VERSION_COMMA=$(PROJECT_VERSION:.=,)

USE_NATIVE_EH = 1
USE_MSVCRT    = 1
MFC_INCLUDES  =
USE_STL       = 1

MUI           = 0
MUI_COMMENT   = PRIVATE_TOOL

_NT_TARGET_VERSION = $(_NT_TARGET_VERSION_WINXP)
TARGET_DESTINATION = idw
