add_definitions(-DPROJ=MsiDump)

file(GLOB SOURCES "*.cpp")
set(RESOURCE "../version.rc")

add_executable(MsiDump ${SOURCES} ${RESOURCE})

target_link_libraries(MsiDump CommonLib)
target_link_libraries(MsiDump msi)
target_link_libraries(MsiDump setupapi)
