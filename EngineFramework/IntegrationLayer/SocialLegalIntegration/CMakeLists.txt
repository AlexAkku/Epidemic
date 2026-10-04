add_library(EpidemicGameFrameworkSocialLegalIntegration STATIC
    src/social_legal_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkSocialLegalIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkSocialLegalIntegration
    PUBLIC
        EpidemicGameFrameworkOwnership
        EpidemicGameFrameworkSociety
        EpidemicGameFrameworkCrime
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkSocialLegalIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkSocialLegalIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkSocialLegalIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkSocialLegalIntegration PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
