# External consumers get only public include directories. A positive control
# prevents a missing compiler/header from making every negative case pass.
if(BUILD_TESTING)
    set(_save_public_includes
        "${CMAKE_SOURCE_DIR}/EngineBase/Foundation/include"
        "${CMAKE_SOURCE_DIR}/EngineFramework/BaseInfrastructure/Foundation/include"
        "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/include")
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    foreach(_fixture IN ITEMS public_control internal_mechanics_control restore_access generic_participant codec fault workspace private_include workspace_copy)
        set(_consumer_includes ${_save_public_includes})
        if(_fixture STREQUAL "internal_mechanics_control")
            list(APPEND _consumer_includes "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
        endif()
        try_compile(_save_consumer_compiles
            SOURCES "${CMAKE_SOURCE_DIR}/Tests/Fixtures/SaveGameConsumers/${_fixture}.cpp"
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${_consumer_includes}"
            CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON
            OUTPUT_VARIABLE _save_consumer_output)
        if(_fixture STREQUAL "public_control" OR _fixture STREQUAL "internal_mechanics_control")
            if(NOT _save_consumer_compiles)
                message(FATAL_ERROR "SaveGame public consumer control failed: ${_save_consumer_output}")
            endif()
        elseif(_save_consumer_compiles)
            message(FATAL_ERROR "Privileged SaveGame mechanics leaked to external consumer: ${_fixture}")
        endif()
    endforeach()
    unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
    message(STATUS "SaveGame public/internal positive controls and seven private-surface rejection fixtures passed")
endif()
