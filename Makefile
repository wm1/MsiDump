SRC_RELEASE_POINT=\\weimao1\public\msi
TOOLBOX_RELEASE_POINT=\\TKFilToolBox\Tools\20864

!INCLUDE version.inc

PROJ=MsiDumpCab
PROJ_CMDLINE=MsiDump

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
	$(O)\parseArgs.obj       \
	$(O)\ui.res

WTL=$(MSVCDIR)\atlmfc\wtl75\include
INCLUDE=$(INCLUDE);$(WTL)

LIBS = user32.lib ole32.lib comctl32.lib comdlg32.lib shell32.lib shlwapi.lib \
       atl.lib setupapi.lib msi.lib

CFLAGS=/nologo /Zi /c /EHsc /MT /D_WIN32_WINNT=0x0501 /D_UNICODE /DUNICODE 

# comment out the following line to disable tracing
CFLAGS=$(CFLAGS) /DENABLE_TRACE=1

# add /debugtype for office profiler
# add /release to avoid symbol matching warning in windbg
LFLAGS=/nologo /debug /debugtype:cv,fixup /release

RFLAGS=/dPROJECT_VERSION=$(PROJECT_VERSION) /dPROJ=$(PROJ)

.cpp{$(O)}.obj::
	cl $(CFLAGS) /Fo"$(O)/" /Fd"$(O)/" $<

.rc{$(O)}.res:
	rc $(RFLAGS) /Fo"$*.res" /r $<

all: $(O)\$(PROJ).exe $(O)\$(PROJ_CMDLINE).exe

$(O)\$(PROJ).exe: $(OBJECTS)
	link $(LFLAGS) $(OBJECTS) $(LIBS) -out:$@

$(O)\MsiUtils.obj: MsiUtils.cpp MsiUtils.h

$(O)\MsiTable.obj: MsiTable.cpp MsiTable.h

$(O)\ui.obj: ui.cpp ui.h

$(O)\DragDrop.obj: DragDrop.cpp DragDrop.h CUnknown.h

$(O)\ui.res: ui.rc version.inc

ui.h: Resource.h MainFrame.h AboutDlg.h

$(O)\$(PROJ_CMDLINE).obj: $(PROJ_CMDLINE).cpp parseArgs.h

$(O)\parseArgs.obj: parseArgs.cpp parseArgs.h

$(O)\$(PROJ_CMDLINE).exe: $(PROJ_CMDLINE_OBJECTS)
	link $(LFLAGS) $** setupapi.lib msi.lib -out:$@

release:
	nmake /nologo clean 2>nul
	-robocopy . $(SRC_RELEASE_POINT)
	nmake /nologo
	call srcindex.cmd
	-robocopy $(O) $(SRC_RELEASE_POINT)\bin *.exe *.pdb /xf vc*.pdb
	-robocopy $(O) $(TOOLBOX_RELEASE_POINT) *.exe *.pdb /xf vc*.pdb
	-robocopy .    $(TOOLBOX_RELEASE_POINT) Readme.txt History.txt
	-robocopy .    $(TOOLBOX_RELEASE_POINT)\src

clean:
	-@del /q trace.txt 2>nul
	-@del /q $(O)      2>nul

