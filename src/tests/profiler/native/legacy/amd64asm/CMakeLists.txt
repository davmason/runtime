
set(ASM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/amd64helpers.asm
    ${CMAKE_CURRENT_SOURCE_DIR}/asmconstants.inc
    ${CMAKE_CURRENT_SOURCE_DIR}/AsmMacros.inc)

set_source_files_properties(ASM_SOURCES PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
