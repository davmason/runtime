
include_directories(../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeTieredCompilation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TieredCompilation.cpp
    PARENT_SCOPE)
