cmake_minimum_required(VERSION 3.0)

project(MsiDump)

include(PrecompiledHeader.cmake)

include_directories(inc)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

# Parse "version.inc" file. Save the content to ${PROJECT_VERSION}
#
file(STRINGS "version.inc" VersionInc)
string(REGEX MATCH "PROJECT_VERSION=(.*)" VersionInc ${VersionInc})
set(PROJECT_VERSION ${CMAKE_MATCH_1})
string(REGEX REPLACE "\\." "," PROJECT_VERSION_COMMA ${PROJECT_VERSION})
message("MsiDump Version : ${PROJECT_VERSION}")

add_definitions(-DPROJECT_VERSION=${PROJECT_VERSION})
add_definitions(-DPROJECT_VERSION_COMMA=${PROJECT_VERSION_COMMA})

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(console)
add_subdirectory(gui)
