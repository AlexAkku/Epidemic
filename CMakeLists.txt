cmake_minimum_required(VERSION 3.25)

project(EpidemicEngine VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EMSCRIPTEN)
    set(_epidemic_default_platform Web)
else()
    set(_epidemic_default_platform Windows)
endif()
set(EPIDEMIC_TARGET_PLATFORM "${_epidemic_default_platform}" CACHE STRING "Target platform: Windows or Web")
set_property(CACHE EPIDEMIC_TARGET_PLATFORM PROPERTY STRINGS Windows Web)
if(EPIDEMIC_TARGET_PLATFORM STREQUAL "Web")
    if(NOT EMSCRIPTEN)
        message(FATAL_ERROR "Web requires the Emscripten toolchain; use tools/Build-Web.ps1")
    endif()
    add_compile_options(-fexceptions)
    add_link_options(-fexceptions -sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=1073741824 -sEXIT_RUNTIME=0)
elseif(NOT EPIDEMIC_TARGET_PLATFORM STREQUAL "Windows" OR NOT WIN32)
    message(FATAL_ERROR "Supported target platforms are Windows and Emscripten Web")
endif()

option(BUILD_TESTING "Build Epidemic Engine tests" ON)
if(EMSCRIPTEN AND BUILD_TESTING)
    message(FATAL_ERROR "The desktop test suite requires Windows. Use the web-release preset; it builds EpidemicWebContracts separately.")
endif()
option(EPIDEMIC_BUILD_RUNTIME "Build Epidemic EngineRuntime" ON)
option(EPIDEMIC_BUILD_FRAMEWORK "Build Epidemic GameFramework" ON)
option(EPIDEMIC_WARNINGS_AS_ERRORS "Treat warnings in Epidemic-owned targets as errors" OFF)
option(EPIDEMIC_ARCHITECTURE_FREEZE_CHECKS "Validate frozen layer boundaries and public headers" ON)
set(EP_AUDITOR_PROFILE "" CACHE STRING "Optional Auditor instrumentation profile")
if(EP_AUDITOR_PROFILE MATCHES "^audit(-deep|-memory)?$")
    if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Exact Auditor instrumentation currently requires MSVC x64 on Windows")
    endif()
    enable_language(ASM_MASM)
    add_subdirectory(.epidemic-auditor/native/tracing)
    if(EP_AUDITOR_PROFILE STREQUAL "audit-deep" OR EP_AUDITOR_PROFILE STREQUAL "audit-memory")
        # Reuse the uninstrumented runtime: observer plumbing is not engine work.
        target_sources(AuditorExactRuntime PRIVATE .epidemic-auditor/native/probes/shadow_probe.cpp)
        target_compile_definitions(AuditorExactRuntime PRIVATE EP_AUDITOR_SHADOW_PROBES=1 EP_AUDITOR_SHADOW_EXACT_CONTEXT=1)
        # Build-only shadow probes. Ordinary Debug/Release have no probe macro/include.
        add_compile_definitions(EP_AUDITOR_SHADOW_PROBES=1)
        include_directories(BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/.epidemic-auditor/native/probes")
    endif()
    if(EP_AUDITOR_PROFILE STREQUAL "audit-memory")
        # The exact tracing hook must remain free of function-entry
        # instrumentation. However, shadow_probe.cpp uses the MSVC STL,
        # so its ASan annotations must match the instrumented engine.
        #
        # Apply ASan only to this translation unit inside AuditorExactRuntime.
        set_source_files_properties(
            "${CMAKE_CURRENT_SOURCE_DIR}/.epidemic-auditor/native/probes/shadow_probe.cpp"
            TARGET_DIRECTORY AuditorExactRuntime
            PROPERTIES COMPILE_OPTIONS "/fsanitize=address"
        )

        # ASan cannot coexist with Debug /RTC checks or incremental linking.
        
        string(REGEX REPLACE "[/\\-]RTC[1su]+" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/fsanitize=address>)
        add_link_options(/INCREMENTAL:NO)
        get_filename_component(auditor_compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set(EP_AUDITOR_ASAN_RUNTIME "${auditor_compiler_directory}/clang_rt.asan_dynamic-x86_64.dll")
        if(NOT EXISTS "${EP_AUDITOR_ASAN_RUNTIME}")
            message(FATAL_ERROR "audit-memory requires the installed MSVC ASan runtime: ${EP_AUDITOR_ASAN_RUNTIME}")
        endif()
    endif()
elseif(EP_AUDITOR_PROFILE STREQUAL "audit-coverage")
    if(NOT WIN32 OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Auditor source coverage requires clang-cl x64 on Windows")
    endif()
    enable_language(ASM_MASM)
    add_subdirectory(.epidemic-auditor/native/tracing)
elseif(EP_AUDITOR_PROFILE)
    message(FATAL_ERROR "Unsupported Auditor instrumentation profile: ${EP_AUDITOR_PROFILE}")
endif()

if(BUILD_TESTING)
    enable_testing()

    function(epidemic_add_test target)
        add_test(NAME "${target}" COMMAND "${target}")
        set_tests_properties("${target}" PROPERTIES TIMEOUT 300)
    endfunction()
endif()

add_subdirectory(EngineBase)

if(EPIDEMIC_BUILD_RUNTIME)
    add_subdirectory(EngineRuntime)
endif()

if(EPIDEMIC_BUILD_FRAMEWORK)
    if(NOT EPIDEMIC_BUILD_RUNTIME)
        message(FATAL_ERROR "EngineFramework requires EPIDEMIC_BUILD_RUNTIME=ON.")
    endif()

    add_subdirectory(EngineFramework)
endif()
if(BUILD_TESTING AND EPIDEMIC_BUILD_RUNTIME AND EPIDEMIC_BUILD_FRAMEWORK)
    add_subdirectory(Tests/Truth)
endif()

if(TARGET AuditorExactRuntime)
    if(EP_AUDITOR_PROFILE STREQUAL "audit-coverage")
        auditor_apply_coverage("${CMAKE_CURRENT_SOURCE_DIR}")
    else()
        auditor_apply_exact_tracing("${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
endif()

if(EPIDEMIC_ARCHITECTURE_FREEZE_CHECKS)
    include(cmake/ArchitectureFreeze.cmake)
    epidemic_validate_architecture_freeze()
    epidemic_add_public_header_self_containment_target()
endif()

include(cmake/EpidemicSDK.cmake)
epidemic_install_sdk()

if(EMSCRIPTEN)
    add_subdirectory(Examples/Web)
endif()
