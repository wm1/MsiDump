cmake_minimum_required(VERSION 3.0)

project(MsiDump)

include_directories(inc)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

file(STRINGS "version.inc" VersionInc)
string(REGEX MATCH "PROJECT_VERSION=(.*)" VersionInc ${VersionInc})
set(PROJECT_VERSION ${CMAKE_MATCH_1})
string(REGEX REPLACE "\\." "," PROJECT_VERSION_COMMA ${PROJECT_VERSION})
message("Version : ${PROJECT_VERSION}")

add_definitions(-DPROJECT_VERSION=${PROJECT_VERSION})
add_definitions(-DPROJECT_VERSION_COMMA=${PROJECT_VERSION_COMMA})

add_subdirectory(lib)
add_subdirectory(console)
add_subdirectory(gui)
