
include(functions.cmake)

if(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL x64)
	enable_language(ASM_MASM)
	set(SOURCES
        ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/amd64helpers1.asm
        ${CMAKE_CURRENT_SOURCE_DIR}/amd64helpers3.asm)
    
	set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/amd64helpers3.asm PROPERTY LANGUAGE ASM_MASM)
	set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/amd64helpers1.asm PROPERTY LANGUAGE ASM_MASM)
endif(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL x64)

if(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL arm)
    message ("Processing for arm32")
    # ARM assembly needs to be manually preprocessed and built. The crazy script below will do this.
    enable_language(ASM)
	
	# Explicitly specify the assembler to be used for Arm32 compile
    if($ENV{__VSVersion} STREQUAL "vs2015")
      file(TO_CMAKE_PATH "$ENV{VCINSTALLDIR}\\bin\\x86_arm\\armasm.exe" CMAKE_ASM_COMPILER)
    else()
      file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}\\bin\\HostX86\\arm\\armasm.exe" CMAKE_ASM_COMPILER)
    endif()

    set(CMAKE_ASM_MASM_COMPILER ${CMAKE_ASM_COMPILER})
    message("CMAKE_ASM_MASM_COMPILER explicitly set to: ${CMAKE_ASM_MASM_COMPILER}")

	set(ASM_FILE armhelpers3.asm) 
    # Inserts a custom command in CMake build to preprocess each asm source file
    get_filename_component(name ${ASM_FILE} NAME_WE)
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${name}.asm" ASM_PREPROCESSED_FILE)
    preprocess_def_file(${CMAKE_CURRENT_SOURCE_DIR}/armhelpers3.asm ${ASM_PREPROCESSED_FILE})

    # On Arm32, compile the preprocessed binary to .obj
    # We do not pass any defines since we have already done pre-processing above
    set (ASM_CMDLINE "-o ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj ${ASM_PREPROCESSED_FILE}")

    # Generate the batch file that will invoke the assembler
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/runasm_${name}_${CMAKE_BUILD_TYPE}.cmd" ASM_SCRIPT_FILE)

    file(GENERATE OUTPUT "${ASM_SCRIPT_FILE}"
         CONTENT "\"${CMAKE_ASM_MASM_COMPILER}\" -g ${ASM_INCLUDE_DIRECTORIES} ${ASM_DEFINITIONS} ${ASM_CMDLINE}")

    message("Generated  - ${ASM_SCRIPT_FILE}")

    # Need to compile asm file using custom command as include directories are not provided to asm compiler
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj
                      COMMAND ${ASM_SCRIPT_FILE}
                      DEPENDS ${ASM_PREPROCESSED_FILE}
                      COMMENT "Assembling ${ASM_PREPROCESSED_FILE} - ${ASM_SCRIPT_FILE}")

    # mark obj as source that does not require compile
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${name}.obj PROPERTIES EXTERNAL_OBJECT TRUE)

    # Add the generated OBJ in the dependency list so that it gets consumed during linkage
	set(SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
endif(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL arm)


if(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL arm64)
    message ("Processing for arm64")
    # ARM64 assembly needs to be manually preprocessed and built. The crazy script below will do this.
    enable_language(ASM)

    # Explicitly specify the assembler to be used for Arm64 compile
    file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}\\bin\\HostX86\\arm64\\armasm64.exe" CMAKE_ASM_COMPILER)

    set(CMAKE_ASM_MASM_COMPILER ${CMAKE_ASM_COMPILER})
    message("CMAKE_ASM_MASM_COMPILER explicitly set to: ${CMAKE_ASM_MASM_COMPILER}")

    set(ASM_FILE arm64helpers3.asm) 
    # Inserts a custom command in CMake build to preprocess each asm source file
    get_filename_component(name ${ASM_FILE} NAME_WE)
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${name}.asm" ASM_PREPROCESSED_FILE)
    preprocess_def_file(${CMAKE_CURRENT_SOURCE_DIR}/${ASM_FILE} ${ASM_PREPROCESSED_FILE})

    # On Arm64, compile the preprocessed binary to .obj
    # We do not pass any defines since we have already done pre-processing above
    set (ASM_CMDLINE "-o ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj ${ASM_FILE}")

    # Generate the batch file that will invoke the assembler
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/runasm_${name}_${CMAKE_BUILD_TYPE}.cmd" ASM_SCRIPT_FILE)

    file(GENERATE OUTPUT "${ASM_SCRIPT_FILE}"
         CONTENT "\"${CMAKE_ASM_MASM_COMPILER}\" -g ${ASM_INCLUDE_DIRECTORIES} ${ASM_DEFINITIONS} ${ASM_CMDLINE}")

    message("Generated  - ${ASM_SCRIPT_FILE}")

    # Need to compile asm file using custom command as include directories are not provided to asm compiler
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj
                      COMMAND ${ASM_SCRIPT_FILE}
                      DEPENDS ${ASM_PREPROCESSED_FILE}
                      COMMENT "Assembling ${ASM_PREPROCESSED_FILE} - ${ASM_SCRIPT_FILE}")

    # mark obj as source that does not require compile
    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${name}.obj PROPERTIES EXTERNAL_OBJECT TRUE)

    # Add the generated OBJ in the dependency list so that it gets consumed during linkage
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${name}.obj)
endif(WIN32 AND CLR_CMAKE_TARGET_ARCH STREQUAL arm64)

if(CLR_CMAKE_PLATFORM_UNIX AND CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)
    enable_language(ASM)
    set(SOURCES ${SOURCES} amd64helpers1.S amd64helpers3.S)
endif(CLR_CMAKE_PLATFORM_UNIX AND CLR_CMAKE_PLATFORM_UNIX_TARGET_AMD64)

if(CLR_CMAKE_PLATFORM_LINUX AND CLR_CMAKE_PLATFORM_UNIX_TARGET_ARM)
    enable_language(ASM)
    set(SOURCES ${SOURCES} armhelpers3.S)
endif(CLR_CMAKE_PLATFORM_LINUX AND CLR_CMAKE_PLATFORM_UNIX_TARGET_ARM)

if (CLR_CMAKE_PLATFORM_UNIX_TARGET_ARM64)
    enable_language(ASM)
    set(SOURCES ${SOURCES} arm64helpers3.S)
endif(CLR_CMAKE_PLATFORM_UNIX_TARGET_ARM64)

add_subdirectory(CEE)
add_subdirectory(DST)
add_subdirectory(JIT)
add_subdirectory(GC)
add_subdirectory(LTS)
add_subdirectory(TieredCompile)

set(SOURCES
    ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/TestProfiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestProfiler.def
    ${CMAKE_CURRENT_SOURCE_DIR}/ilrewriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ProfilerCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/LegacyCompat.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SigParse.cpp
    PARENT_SCOPE)

