set(TARGET MsiDumpCommon)

set(SOURCES
    MsiTable.cpp
    MsiUtils.cpp)

add_library(
    ${TARGET}
    STATIC
    ${SOURCES})
