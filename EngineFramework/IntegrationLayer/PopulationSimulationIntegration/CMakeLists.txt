add_library(EpidemicGameFrameworkPopulationSimulationIntegration STATIC
    src/population_simulation_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkPopulationSimulationIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkPopulationSimulationIntegration
    PUBLIC
        EpidemicGameFrameworkPopulation
        EpidemicGameFrameworkEncounters
        EpidemicGameFrameworkRolesJobs
        EpidemicGameFrameworkNeedsLife
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkPopulationSimulationIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkPopulationSimulationIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkPopulationSimulationIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkPopulationSimulationIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
