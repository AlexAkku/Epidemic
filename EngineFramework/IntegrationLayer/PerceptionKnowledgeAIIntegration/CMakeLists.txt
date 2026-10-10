add_library(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration STATIC
    src/perception_knowledge_ai_adapters.cpp
)

target_include_directories(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration
    PUBLIC
        EpidemicGameFrameworkPerception
        EpidemicGameFrameworkKnowledge
        EpidemicGameFrameworkAI
    PUBLIC EpidemicFoundation
)

epidemic_configure_target(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration)

# M5.2 ownership-local save bindings; SaveGame never links back to owners.
target_sources(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration PRIVATE src/save_participants.cpp)
target_link_libraries(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration PUBLIC EpidemicGameFrameworkSaveGame)
target_include_directories(EpidemicGameFrameworkPerceptionKnowledgeAIIntegration PRIVATE "${EpidemicEngine_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")
