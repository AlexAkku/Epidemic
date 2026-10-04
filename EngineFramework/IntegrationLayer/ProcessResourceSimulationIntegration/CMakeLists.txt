add_library(EpidemicGameFrameworkProcessResourceSimulationIntegration STATIC
    src/process_resource_simulation_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkProcessResourceSimulationIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkProcessResourceSimulationIntegration
    PUBLIC
        EpidemicGameFrameworkProcesses
        EpidemicGameFrameworkResourcesProduction
        EpidemicGameFrameworkSimulation
    PRIVATE EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkProcessResourceSimulationIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkProcessResourceSimulationIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkProcessResourceSimulationIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkProcessResourceSimulationIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
