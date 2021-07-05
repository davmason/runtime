
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectMetaData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InitializeInjectMetaData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectNewUserDefinedString.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AssemblyInjector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MetadataSignatureBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectGenericTypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectModuleRef.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectAssemblyRef.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectMethodSpec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectTypeSpec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/InjectExternalCall.cpp
    PARENT_SCOPE)
