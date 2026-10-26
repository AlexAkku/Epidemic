set(EPIDEMIC_DOTNET_HOST_PACK "" CACHE PATH "Installed .NET 10 win-x64 host pack native headers")
foreach(_header hostfxr.h coreclr_delegates.h)
    if(NOT EXISTS "${EPIDEMIC_DOTNET_HOST_PACK}/${_header}")
        message(FATAL_ERROR "Managed bridge requires existing .NET host headers: EPIDEMIC_DOTNET_HOST_PACK/${_header}")
    endif()
endforeach()
find_program(EPIDEMIC_DOTNET_EXECUTABLE dotnet REQUIRED)
set(_managed_project "${CMAKE_CURRENT_SOURCE_DIR}/Epidemic.ManagedBridge/Epidemic.ManagedBridge.csproj")
if(BUILD_TESTING)
    set(_managed_project "${CMAKE_CURRENT_SOURCE_DIR}/tests/Epidemic.ManagedBridge.Tests/Epidemic.ManagedBridge.Tests.csproj")
endif()
set(_managed_output "${CMAKE_BINARY_DIR}/managed/$<CONFIG>")
add_custom_target(EpidemicManagedAssembly
    COMMAND "${EPIDEMIC_DOTNET_EXECUTABLE}" build "${_managed_project}" --configuration "$<CONFIG>"
        --output "${_managed_output}" --artifacts-path "${CMAKE_BINARY_DIR}/managed/artifacts"
        --nologo --verbosity quiet
    VERBATIM)

add_library(EpidemicRuntimeManagedBridge STATIC native/managed_bridge.cpp)
target_include_directories(EpidemicRuntimeManagedBridge PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    PRIVATE "${EPIDEMIC_DOTNET_HOST_PACK}")
target_link_libraries(EpidemicRuntimeManagedBridge PUBLIC EpidemicRuntimeGraph)
target_compile_definitions(EpidemicRuntimeManagedBridge PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
epidemic_configure_target(EpidemicRuntimeManagedBridge)
add_dependencies(EpidemicRuntimeManagedBridge EpidemicManagedAssembly)

if(BUILD_TESTING)
    set(EPIDEMIC_HOSTFXR_LIBRARY "" CACHE FILEPATH "Installed/deployed hostfxr.dll for managed integration tests")
    if(NOT EXISTS "${EPIDEMIC_HOSTFXR_LIBRARY}")
        message(FATAL_ERROR "Set EPIDEMIC_HOSTFXR_LIBRARY to existing hostfxr.dll for tests")
    endif()
    add_executable(EpidemicManagedBridgeTests tests/managed_bridge_tests.cpp)
    target_link_libraries(EpidemicManagedBridgeTests PRIVATE EpidemicRuntimeManagedBridge EpidemicRuntimeSchema)
    epidemic_configure_target(EpidemicManagedBridgeTests)
    add_test(NAME EpidemicManagedBridgeTests COMMAND EpidemicManagedBridgeTests
        "${EPIDEMIC_HOSTFXR_LIBRARY}" "${_managed_output}/Epidemic.ManagedBridge.Tests.runtimeconfig.json"
        "${_managed_output}/Epidemic.ManagedBridge.Tests.dll")
    add_test(NAME EpidemicManagedContractTests COMMAND "${EPIDEMIC_DOTNET_EXECUTABLE}"
        "${_managed_output}/Epidemic.ManagedBridge.Tests.dll")
    set_tests_properties(EpidemicManagedBridgeTests EpidemicManagedContractTests PROPERTIES TIMEOUT 300)
endif()
