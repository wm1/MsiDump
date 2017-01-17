set(TARGET MsiDumpCab)

include_directories(..)

find_path(WTL_INCLUDE NAMES wtl/atlapp.h)
set(WTL_INCLUDE "${WTL_INCLUDE}/wtl")
include_directories(${WTL_INCLUDE})

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
