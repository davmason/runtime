
include_directories(../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/EnumObjects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GCAttach.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GCCallbacks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GCCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GCStatics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeGC.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProfilerStructs.cpp
    PARENT_SCOPE)
