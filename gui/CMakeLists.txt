set(TARGET MsiDumpCab)

add_definitions(-DPROJ=${TARGET})

include_directories(..)

set(SOURCES
    DragDrop.cpp
    MainFrame.cpp)

set(RESOURCE "MainFrame.rc")

set(MANIFEST "MsiDumpCab.exe.manifest")

add_executable(
    ${TARGET}
    WIN32
    ${SOURCES}
    ${RESOURCE}
    ${MANIFEST})

target_link_libraries(${TARGET} MsiDumpCommon)
target_link_libraries(${TARGET} msi)
target_link_libraries(${TARGET} setupapi)
