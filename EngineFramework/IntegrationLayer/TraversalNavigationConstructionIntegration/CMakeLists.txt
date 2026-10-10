add_library(EpidemicGameFrameworkTraversalNavigationConstructionIntegration STATIC
    src/traversal_navigation_construction_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkTraversalNavigationConstructionIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkTraversalNavigationConstructionIntegration
    PUBLIC
        EpidemicGameFrameworkTraversal
        EpidemicGameFrameworkNavigationSemantics
        EpidemicGameFrameworkConstruction
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkTraversalNavigationConstructionIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkTraversalNavigationConstructionIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkTraversalNavigationConstructionIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkTraversalNavigationConstructionIntegration PRIVATE "${EpidemicEngine_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
