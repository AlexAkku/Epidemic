include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

function(_epidemic_sdk_targets directory output)
    get_property(_targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(_children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(_child IN LISTS _children)
        _epidemic_sdk_targets("${_child}" _nested)
        list(APPEND _targets ${_nested})
    endforeach()
    set(${output} "${_targets}" PARENT_SCOPE)
endfunction()

function(epidemic_install_sdk)
    # Instrumentation runtimes are local diagnostics, never production SDKs.
    if(EP_AUDITOR_PROFILE)
        return()
    endif()
    set(EPIDEMIC_ABI_VERSION 1)
    execute_process(COMMAND git rev-parse HEAD WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE EPIDEMIC_SOURCE_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(NOT EPIDEMIC_SOURCE_REVISION)
        set(EPIDEMIC_SOURCE_REVISION "source-archive")
    endif()
    # A commit can change revision metadata without touching any C++ source.
    foreach(_git_file HEAD logs/HEAD packed-refs)
        execute_process(COMMAND git rev-parse --path-format=absolute --git-path "${_git_file}"
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE _git_path
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if(EXISTS "${_git_path}")
            set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_git_path}")
        endif()
    endforeach()
    set(EPIDEMIC_SDK_CAPABILITIES "\"base\"")
    foreach(_pair "EpidemicRuntimeSupport|runtime" "EpidemicGameFrameworkFoundation|framework"
                  "EpidemicGameFrameworkPresentation|presentation" "EpidemicGameFrameworkSession|session")
        string(REPLACE "|" ";" _parts "${_pair}")
        list(GET _parts 0 _capability_target)
        list(GET _parts 1 _capability_name)
        if(TARGET "${_capability_target}")
            string(APPEND EPIDEMIC_SDK_CAPABILITIES ", \"${_capability_name}\"")
        endif()
    endforeach()
    # Content identity includes uncommitted/new engine sources, not timestamps.
    file(GLOB_RECURSE _inputs CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/EngineBase/*.h" "${PROJECT_SOURCE_DIR}/EngineBase/*.cpp"
        "${PROJECT_SOURCE_DIR}/EngineRuntime/*.h" "${PROJECT_SOURCE_DIR}/EngineRuntime/*.cpp"
        "${PROJECT_SOURCE_DIR}/EngineFramework/*.h" "${PROJECT_SOURCE_DIR}/EngineFramework/*.cpp"
        "${PROJECT_SOURCE_DIR}/Game/Composition/*.h" "${PROJECT_SOURCE_DIR}/Game/Composition/*.cpp"
        "${PROJECT_SOURCE_DIR}/Game/Editor/*.h" "${PROJECT_SOURCE_DIR}/Game/Editor/*.cpp"
        "${PROJECT_SOURCE_DIR}/Game/Editor/CMakeLists.txt" "${PROJECT_SOURCE_DIR}/Game/Editor/*/CMakeLists.txt"
        "${PROJECT_SOURCE_DIR}/Game/Composition/CMakeLists.txt" "${PROJECT_SOURCE_DIR}/Game/Composition/*/CMakeLists.txt"
        "${PROJECT_SOURCE_DIR}/EngineBase/CMakeLists.txt" "${PROJECT_SOURCE_DIR}/EngineBase/*/CMakeLists.txt"
        "${PROJECT_SOURCE_DIR}/EngineRuntime/*/CMakeLists.txt" "${PROJECT_SOURCE_DIR}/EngineFramework/*/CMakeLists.txt"
        "${PROJECT_SOURCE_DIR}/cmake/*.cmake" "${PROJECT_SOURCE_DIR}/cmake/*.in")
    list(APPEND _inputs "${PROJECT_SOURCE_DIR}/CMakeLists.txt")
    list(SORT _inputs)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_inputs})
    set(_identity "${PROJECT_VERSION};${EPIDEMIC_ABI_VERSION};${CMAKE_CXX_COMPILER_ID};${CMAKE_CXX_COMPILER_VERSION}")
    foreach(_input IN LISTS _inputs)
        file(SHA256 "${_input}" _hash)
        file(RELATIVE_PATH _relative "${PROJECT_SOURCE_DIR}" "${_input}")
        string(APPEND _identity ";${_relative}:${_hash}")
    endforeach()
    string(SHA256 EPIDEMIC_BUILD_ID "${_identity}")
    configure_file("${PROJECT_SOURCE_DIR}/cmake/build_info.h.in"
        "${PROJECT_BINARY_DIR}/sdk/include/Epidemic/build_info.h" @ONLY)
    _epidemic_sdk_targets("${PROJECT_SOURCE_DIR}" _targets)
    set(_libraries)
    set(_build_libraries)
    foreach(_target IN LISTS _targets)
        get_target_property(_type "${_target}" TYPE)
        if(NOT _type MATCHES "^(STATIC|INTERFACE)_LIBRARY$")
            continue()
        endif()
        get_target_property(_source "${_target}" SOURCE_DIR)
        if(_source MATCHES "/(Tests|tests|Apps)/")
            continue()
        endif()
        get_target_property(_includes "${_target}" INTERFACE_INCLUDE_DIRECTORIES)
        set(_export_includes "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
        foreach(_include IN LISTS _includes)
            if(_include MATCHES "^\\$<")
                list(APPEND _export_includes "${_include}")
            elseif(IS_DIRECTORY "${_include}")
                list(APPEND _export_includes "$<BUILD_INTERFACE:${_include}>")
                if(_target STREQUAL "EpidemicRuntimePresentation")
                    install(DIRECTORY "${_include}/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Epidemic/Runtime/Presentation"
                        FILES_MATCHING PATTERN "*.h")
                    list(APPEND _export_includes "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/Epidemic/Runtime/Presentation>")
                else()
                    install(DIRECTORY "${_include}/" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
                        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
                endif()
            endif()
        endforeach()
        set_property(TARGET "${_target}" PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_export_includes}")
        list(APPEND _libraries "${_target}")
        if(_type STREQUAL "STATIC_LIBRARY")
            list(APPEND _build_libraries "${_target}")
        endif()
    endforeach()
    add_custom_target(EpidemicSDK DEPENDS ${_build_libraries})
    install(TARGETS ${_libraries} EXPORT EpidemicTargets ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    install(EXPORT EpidemicTargets NAMESPACE Epidemic:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Epidemic")
    install(FILES "${PROJECT_BINARY_DIR}/sdk/include/Epidemic/build_info.h" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Epidemic")
    configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/EpidemicConfig.cmake.in"
        "${PROJECT_BINARY_DIR}/sdk/EpidemicConfig.cmake" INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Epidemic")
    write_basic_package_version_file("${PROJECT_BINARY_DIR}/sdk/EpidemicConfigVersion.cmake"
        VERSION "${PROJECT_VERSION}" COMPATIBILITY SameMinorVersion)
    install(FILES "${PROJECT_BINARY_DIR}/sdk/EpidemicConfig.cmake" "${PROJECT_BINARY_DIR}/sdk/EpidemicConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Epidemic")
    configure_file("${PROJECT_SOURCE_DIR}/cmake/SDKManifest.cmake.in" "${PROJECT_BINARY_DIR}/sdk/SDKManifest.cmake" @ONLY)
    install(SCRIPT "${PROJECT_BINARY_DIR}/sdk/SDKManifest.cmake")
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION "share/Epidemic")
endfunction()
