add_definitions(-DPROJ=MsiDump)

set(SOURCES MsiDump.cpp ParseArgs.cpp precomp.cpp)
set(RESOURCE "../version.rc")

add_executable(MsiDump ${SOURCES} ${RESOURCE})

add_precompiled_header(MsiDump precomp.h SOURCE_CXX precomp.cpp)

target_link_libraries(MsiDump CommonLib)
target_link_libraries(MsiDump msi)
target_link_libraries(MsiDump setupapi)
