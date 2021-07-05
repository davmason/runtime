
include_directories(../../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeUnitTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit_CCW.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit_EnumJittedFunctions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit_EnumModules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Unit_EnumThreads.cpp
    PARENT_SCOPE)
