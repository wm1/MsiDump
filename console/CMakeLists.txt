set(TARGET MsiDump)

add_definitions(-DPROJ=${TARGET})

set(SOURCES
    MsiDump.cpp
    ParseArgs.cpp
    precomp.cpp)

set(RESOURCE "../version.rc")

add_executable(
    ${TARGET}
    ${SOURCES}
    ${RESOURCE})

add_precompiled_header(
    ${TARGET}
    precomp.h
    SOURCE_CXX precomp.cpp)

target_link_libraries(${TARGET} MsiDumpCommon)
target_link_libraries(${TARGET} msi)
target_link_libraries(${TARGET} setupapi)
