set(SOURCES MsiTable.cpp MsiUtils.cpp precomp.cpp)

add_library(CommonLib STATIC ${SOURCES})

add_precompiled_header(CommonLib precomp.h SOURCE_CXX precomp.cpp)