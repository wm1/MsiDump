set(TARGET MsiDumpCab)

include_directories(..)
include_directories(../deps/wtl)

add_definitions(-DPROJ=${TARGET})

set(SOURCES
    DragDrop.cpp
    MainFrame.cpp
    precomp.cpp)

set(RESOURCE "MainFrame.rc")

set(MANIFEST "MsiDumpCab.exe.manifest")

add_executable(
    ${TARGET}
    WIN32
    ${SOURCES}
    ${RESOURCE}
    ${MANIFEST})

add_precompiled_header(
    ${TARGET}
    precomp.h
    SOURCE_CXX precomp.cpp)

target_link_libraries(${TARGET} MsiDumpCommon)
target_link_libraries(${TARGET} msi)
target_link_libraries(${TARGET} setupapi)
