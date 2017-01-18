set(TARGET MsiDumpTest)

add_definitions(-DPROJ=${TARGET})

set(SOURCES
    MsiDumpTest.cpp)

set(RESOURCE "../version.rc")

add_executable(
    ${TARGET}
    ${SOURCES}
    ${RESOURCE})

target_link_libraries(${TARGET} MsiDumpCommon)
target_link_libraries(${TARGET} msi)
target_link_libraries(${TARGET} setupapi)
