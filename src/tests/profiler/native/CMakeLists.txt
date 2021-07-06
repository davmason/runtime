cmake_minimum_required(VERSION 3.14.5)

project(Profiler)

set(SOURCES
    ${SOURCES}
    profiler.def
    profiler.cpp
    profilerstring.cpp
    classfactory.cpp
    dllmain.cpp
    guids.cpp
    ilrewriter.cpp
    sigparse.cpp
    CACHE INTERNAL "")

add_subdirectory(eltprofiler)
add_subdirectory(eventpipeprofiler)
add_subdirectory(gcbasicprofiler)
add_subdirectory(gcprofiler)
add_subdirectory(getappdomainstaticaddress)
add_subdirectory(metadatagetdispenser)
add_subdirectory(nullprofiler)
add_subdirectory(rejitprofiler)
add_subdirectory(releaseondetach)
add_subdirectory(transitions)
add_subdirectory(legacy)

include_directories(../../../coreclr/pal/prebuilt/inc)

if(WIN32)
    add_compile_options(/std:c++17 /GR /wd4828)
else(WIN32)
    add_compile_options(-std=c++17)
endif(WIN32)

if(NOT WIN32)
    include_directories(../../../coreclr/pal/inc/rt ../../../coreclr/pal/inc ../../../coreclr/inc)
    add_compile_options(-DPAL_STDCPP_COMPAT)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-null-arithmetic)
    else(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wno-conversion-null -Wno-pointer-arith)
    endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
endif(NOT WIN32)

# add the executable
add_library(Profiler SHARED ${SOURCES})

if(WIN32)
	set(DEPENDENCIES Advapi32.lib oleaut32.lib Ole32.lib)
else(WIN32)
    set(DEPENDENCIES pthread dl)
endif(WIN32)

target_link_libraries(Profiler PUBLIC ${DEPENDENCIES})

# add the install targets
install(TARGETS Profiler DESTINATION bin)

if(WIN32)
	install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION bin OPTIONAL)
endif(WIN32)
