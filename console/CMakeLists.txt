file(GLOB SOURCES "*.cpp")

add_executable(MsiDump ${SOURCES})

target_link_libraries(MsiDump CommonLib)
target_link_libraries(MsiDump msi)
target_link_libraries(MsiDump setupapi)
