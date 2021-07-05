cmake_minimum_required (VERSION 3.5.2)

add_subdirectory(ProfileHub)
add_subdirectory(InjectMetaData)
add_subdirectory(ReJIT)
add_subdirectory(Regressions)
add_subdirectory(Unit)
add_subdirectory(Instrumentation)
add_subdirectory(ELT)

set(SOURCES
    ${SOURCES}
    PARENT_SCOPE)