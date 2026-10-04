add_library(EpidemicGameFrameworkNarrativeIntegration STATIC
    src/narrative_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkNarrativeIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkNarrativeIntegration
    PUBLIC
        EpidemicGameFrameworkNarrative
        EpidemicGameFrameworkKnowledge
        EpidemicGameFrameworkSaveGame
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkNarrativeIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkNarrativeIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkNarrativeIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkNarrativeIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
