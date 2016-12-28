include_directories(..)
include_directories(../deps/wtl)

add_definitions(-DPROJ=MsiDumpCab)

file(GLOB SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp")
set(RESOURCE "MainFrame.rc")
set(MANIFEST "MsiDumpCab.exe.manifest")

add_executable(MsiDumpCab WIN32 ${SOURCES} ${RESOURCE} ${MANIFEST})

add_precompiled_header(MsiDumpCab precomp.h SOURCE_CXX precomp.cpp)

target_link_libraries(MsiDumpCab CommonLib)
target_link_libraries(MsiDumpCab msi)
target_link_libraries(MsiDumpCab setupapi)
