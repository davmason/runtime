
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)

if(WIN32)
    add_compile_options(-DTHREAD_HIJACK)
endif(WIN32)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/AllCallbacks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AllCallbacksWithDrilldown.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Asynchronous.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CorPrfProfileFlags.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DoStackSnapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ILToNativeMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeProfileHub.cpp
    PARENT_SCOPE)
