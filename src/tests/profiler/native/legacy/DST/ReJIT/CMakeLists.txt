include_directories(../../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ilrewriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeReJIT.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ReJITScript.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rejitter.cpp
    PARENT_SCOPE)
