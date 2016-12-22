include_directories(..)
include_directories(../deps/wtl)

add_definitions(-DPROJ=MsiDumpCab)
add_definitions(-DPROJECT_VERSION=2.2016.12.10)
add_definitions(-DPROJECT_VERSION_COMMA=2,2016,12,10)

file(GLOB SOURCES "*.cpp")
set(RESOURCE "MainFrame.rc")
set(MANIFEST "MsiDumpCab.exe.manifest")

add_executable(MsiDumpCab WIN32 ${SOURCES} ${RESOURCE} ${MANIFEST})

target_link_libraries(MsiDumpCab CommonLib)
target_link_libraries(MsiDumpCab msi)
target_link_libraries(MsiDumpCab setupapi)
