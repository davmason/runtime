
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeStressProfilers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Prf1_AllCallbacks_SOK.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Prf2_CodeProfiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Prf3_SamplingProfiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SimpleRWLock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/StressProfilers.def
    PARENT_SCOPE)
