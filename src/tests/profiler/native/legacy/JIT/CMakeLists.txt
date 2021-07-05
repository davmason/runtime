include_directories(../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/CollectMpgoProfilerData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FunctionIDMapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FunctionReJIT.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GetFunctionInfo2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeJIT.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InlineVerify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/JITCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NoInline_Verify.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ShadowStackTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/VerifyInlinerLocalsLimit.cpp
    PARENT_SCOPE)
