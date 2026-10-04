add_library(EpidemicGameFrameworkGameplayIntegration STATIC
    src/gameplay_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkGameplayIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkGameplayIntegration
    PUBLIC
        EpidemicGameFrameworkProgression
        EpidemicGameFrameworkCombat
        EpidemicGameFrameworkAbilities
        EpidemicGameFrameworkLoot
        EpidemicGameFrameworkEffects
        EpidemicGameFrameworkConditions
        EpidemicGameplayTime
    PRIVATE
        EpidemicFoundation
        EpidemicGameFrameworkSupportRandom
)

epidemic_configure_target(EpidemicGameFrameworkGameplayIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkGameplayIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkGameplayIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkGameplayIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
