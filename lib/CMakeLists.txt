file(GLOB SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp")

add_library(CommonLib STATIC ${SOURCES})

add_precompiled_header(CommonLib precomp.h SOURCE_CXX precomp.cpp)