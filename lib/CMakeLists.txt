set(TARGET MsiDumpCommon)

set(SOURCES
    MsiTable.cpp
    MsiUtils.cpp
    precomp.cpp)

add_library(
    ${TARGET}
    STATIC
    ${SOURCES})

add_precompiled_header(
    ${TARGET}
    precomp.h
    SOURCE_CXX precomp.cpp)
