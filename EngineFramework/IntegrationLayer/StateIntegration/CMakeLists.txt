add_library(EpidemicGameFrameworkStateIntegration STATIC
    src/state_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkStateIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkStateIntegration
    PUBLIC
        EpidemicFoundation
        EpidemicGameFrameworkEntities
        EpidemicGameFrameworkMaterials
        EpidemicGameFrameworkConditions
        EpidemicGameFrameworkEffects
        EpidemicGameplayQueries
        EpidemicGameplayFacts
        EpidemicGameplayTime
        EpidemicGameFrameworkCoreIntegration
)

epidemic_configure_target(EpidemicGameFrameworkStateIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkStateIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkStateIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkStateIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
