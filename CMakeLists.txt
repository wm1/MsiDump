cmake_minimum_required(VERSION 3.6)

project(MsiDump)

include_directories(inc)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_subdirectory(lib)
add_subdirectory(test)
add_subdirectory(console)
add_subdirectory(gui)
