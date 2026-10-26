# Project-reference preflight only: no Roslyn, CLR or .NET SDK dependency.
# Explicit MSBuild imports/expressions are rejected until an evaluated checker exists.
function(epidemic_validate_managed_architecture)
    file(GLOB_RECURSE _managed_projects CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/EngineBase/*.csproj"
        "${PROJECT_SOURCE_DIR}/EngineRuntime/*.csproj"
        "${PROJECT_SOURCE_DIR}/EngineFramework/*.csproj"
        "${PROJECT_SOURCE_DIR}/Editor/*.csproj"
        "${PROJECT_SOURCE_DIR}/Game/*.csproj")
    if(BUILD_TESTING OR _managed_projects)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        set(_checker "${PROJECT_SOURCE_DIR}/tools/check_managed_architecture.py")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_checker}")
        execute_process(COMMAND "${Python3_EXECUTABLE}" -B "${_checker}" "${PROJECT_SOURCE_DIR}"
            RESULT_VARIABLE _result OUTPUT_VARIABLE _out ERROR_VARIABLE _error)
        if(NOT _result EQUAL 0)
            message(FATAL_ERROR "Managed architecture: ${_out}${_error}")
        endif()
        if(BUILD_TESTING)
            add_test(NAME EpidemicArchitectureFreezeSelfTests
                COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/cmake/ArchitectureFreezeSelfTest.cmake")
            add_test(NAME EpidemicManagedArchitectureTests
                COMMAND "${Python3_EXECUTABLE}" -B "${_checker}" "${PROJECT_SOURCE_DIR}"
                    --self-test "${CMAKE_BINARY_DIR}")
        endif()
    endif()
endfunction()
