SRC_RELEASE_POINT=\\weimao2\public\msi
TOOLBOX_RELEASE_POINT=\\TKFilToolBox\Tools\20864

PROJECT_VERSION=2,0,0,14

PROJ=MsiDumpCab
PROJ_CMDLINE=MsiDump

!ifdef NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!else

O        = obj

OBJECTS =                 \
	$(O)\MsiUtils.obj \
	$(O)\MsiTable.obj \
	$(O)\DragDrop.obj \
	$(O)\ui.obj       \
	$(O)\ui.res

PROJ_CMDLINE_OBJECTS =           \
	$(O)\MsiUtils.obj        \
	$(O)\MsiTable.obj        \
	$(O)\$(PROJ_CMDLINE).obj \
	$(O)\ui.res

WTL=$(MSVCDIR)\atlmfc\wtl71\include
INCLUDE=$(INCLUDE);$(WTL)

LIBS = user32.lib ole32.lib comctl32.lib comdlg32.lib shell32.lib shlwapi.lib \
       atl.lib setupapi.lib msi.lib

CFLAGS=/nologo /Zi /c /EHsc /D_WIN32_WINNT=0x0501 /D_UNICODE /DUNICODE 

# comment out the following line to disable tracing
CFLAGS=$(CFLAGS) /DENABLE_TRACE=1

LFLAGS=/nologo /debug

RFLAGS=/dPROJECT_VERSION=$(PROJECT_VERSION) /dPROJ=$(PROJ)

.cpp{$(O)}.obj::
	cl $(CFLAGS) /Fo"$(O)/" /Fd"$(O)/" $<

.rc{$(O)}.res:
	rc $(RFLAGS) /Fo"$*.res" /r $<

all: $(O)\$(PROJ).exe $(O)\$(PROJ_CMDLINE).exe

$(O)\$(PROJ).exe: $(OBJECTS)
	link $(LFLAGS) $(OBJECTS) $(LIBS) -out:$@
	rem -copy /y $@ C:\WINDOWS\System32

$(O)\MsiUtils.obj: MsiUtils.cpp MsiUtils.h

$(O)\MsiTable.obj: MsiTable.cpp MsiTable.h

$(O)\ui.obj: ui.cpp ui.h

$(O)\DragDrop.obj: DragDrop.cpp DragDrop.h CUnknown.h

$(O)\ui.res: ui.rc

ui.h: Resource.h MainFrame.h AboutDlg.h

$(O)\$(PROJ_CMDLINE).exe: $(PROJ_CMDLINE_OBJECTS)
	link $(LFLAGS) $** setupapi.lib msi.lib -out:$@

release:
	del /q /s $(SRC_RELEASE_POINT) >nul
	del /q $(TOOLBOX_RELEASE_POINT)
	nmake /nologo clean 2>nul
	-robocopy . $(SRC_RELEASE_POINT)
	-robocopy managed $(SRC_RELEASE_POINT)\managed
	nmake /nologo
	-robocopy $(O) $(SRC_RELEASE_POINT)\bin *.exe *.pdb /xf vc*.pdb
	-robocopy .    $(TOOLBOX_RELEASE_POINT) Readme.txt History.txt
	-robocopy .    $(TOOLBOX_RELEASE_POINT)\src
	-robocopy $(O) $(TOOLBOX_RELEASE_POINT) *.exe

clean:
	-@del /q trace.txt 2>nul
	-@del /q $(O)      2>nul

!endif
