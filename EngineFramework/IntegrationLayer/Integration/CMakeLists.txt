add_library(EpidemicGameFrameworkCoreIntegration STATIC
    src/core_adapters.cpp
    src/save_package_store.cpp
    src/save_generation.cpp
    src/save_restore.cpp
    src/graph_scope.cpp
    src/components.cpp
    src/graph_behavior.cpp
    src/prefab.cpp
)

target_include_directories(EpidemicGameFrameworkCoreIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkCoreIntegration
    PUBLIC
        EpidemicGameplayQueries
        EpidemicGameplayFacts
        EpidemicGameplayTime
        EpidemicGameFrameworkSaveGame
        EpidemicGameFrameworkEntities
        EpidemicRuntimeTime
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkCoreIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkCoreIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkCoreIntegration PRIVATE EpidemicPlatform)
target_link_libraries(EpidemicGameFrameworkCoreIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkCoreIntegration PRIVATE "${EpidemicEngine_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
