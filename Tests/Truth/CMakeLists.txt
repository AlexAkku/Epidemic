function(epidemic_add_truth_test target source)
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    epidemic_configure_target(${target})
    epidemic_add_test(${target})
    set_tests_properties(${target} PROPERTIES LABELS "truth")
endfunction()

epidemic_add_truth_test(EpidemicFrameworkPublicApiTruthTests framework_public_api_tests.cpp)
target_link_libraries(EpidemicFrameworkPublicApiTruthTests PRIVATE
    EpidemicGameFrameworkFoundation
    EpidemicGameFrameworkSupportRandom
    EpidemicGameplayQueries
    EpidemicGameplayFacts
    EpidemicGameplayTime
    EpidemicGameFrameworkRuntimeBridge
    EpidemicGameFrameworkEntities
    EpidemicGameFrameworkWorld
    EpidemicGameFrameworkEnvironment
    EpidemicGameFrameworkMaterials
    EpidemicGameFrameworkConditions
    EpidemicGameFrameworkEffects
    EpidemicGameFrameworkInteraction
    EpidemicGameFrameworkItemsInventory
    EpidemicGameFrameworkEquipment
    EpidemicGameFrameworkDialogue
    EpidemicGameFrameworkEconomy
    EpidemicGameFrameworkCombat
    EpidemicGameFrameworkAbilities
    EpidemicGameFrameworkProgression
    EpidemicGameFrameworkLoot
    EpidemicGameFrameworkTraversal
    EpidemicGameFrameworkNavigationSemantics
    EpidemicGameFrameworkConstruction
    EpidemicGameFrameworkPerception
    EpidemicGameFrameworkKnowledge
    EpidemicGameFrameworkAI
)

epidemic_add_truth_test(EpidemicFrameworkContractTruthTests framework_contract_tests.cpp)
target_link_libraries(EpidemicFrameworkContractTruthTests PRIVATE
    EpidemicGameFrameworkSupportRandom
    EpidemicGameplayTime
    EpidemicGameFrameworkEntities
    EpidemicGameFrameworkItemsInventory
    EpidemicGameFrameworkEconomy
    EpidemicGameFrameworkProgression
)

epidemic_add_truth_test(EpidemicFrameworkIntegrationTruthTests framework_integration_tests.cpp)
target_link_libraries(EpidemicFrameworkIntegrationTruthTests PRIVATE
    EpidemicGameFrameworkPerceptionKnowledgeAIIntegration
)

epidemic_add_truth_test(EpidemicEngineLoadTruthTests engine_load_tests.cpp)
target_link_libraries(EpidemicEngineLoadTruthTests PRIVATE
    EpidemicGameFrameworkEntities
    EpidemicGameplayTime
    EpidemicGameFrameworkPerception
    EpidemicGameFrameworkAI
    EpidemicRuntimeSupport
)
set_tests_properties(EpidemicEngineLoadTruthTests PROPERTIES LABELS "truth;load" TIMEOUT 240)

epidemic_add_truth_test(EpidemicEngineSmokeTruthTests engine_smoke_tests.cpp)
target_link_libraries(EpidemicEngineSmokeTruthTests PRIVATE
    EpidemicEngineBaseSupport
    EpidemicRuntimeSupport
    EpidemicGameplayTime
    EpidemicGameFrameworkEntities
    EpidemicGameFrameworkProgression
)
set_tests_properties(EpidemicEngineSmokeTruthTests PROPERTIES LABELS "truth;smoke" TIMEOUT 120)

add_executable(EpidemicPersistenceWireTruthTests persistence_wire_tests.cpp)
target_link_libraries(EpidemicPersistenceWireTruthTests PRIVATE EpidemicGameFrameworkCoreIntegration EpidemicPlatform)
epidemic_configure_target(EpidemicPersistenceWireTruthTests)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME EpidemicPersistenceWireTruthTests COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/ci/test_save_process.py" "$<TARGET_FILE:EpidemicPersistenceWireTruthTests>" --local)
set_tests_properties(EpidemicPersistenceWireTruthTests PROPERTIES LABELS "truth;persistence-wire;persistence-storage" TIMEOUT 120)
add_test(NAME EpidemicPersistenceProcessTruthTests COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tools/ci/test_save_process.py" "$<TARGET_FILE:EpidemicPersistenceWireTruthTests>" --runtime-executable "$<TARGET_FILE:EpidemicRuntimeSupportTests>")
set_tests_properties(EpidemicPersistenceProcessTruthTests PROPERTIES LABELS "truth;persistence-wire;persistence-storage" TIMEOUT 240)

epidemic_add_truth_test(EpidemicWholeEngineRestoreTruthTests whole_engine_restore_tests.cpp)
target_include_directories(EpidemicWholeEngineRestoreTruthTests PRIVATE
    "${PROJECT_SOURCE_DIR}/EngineFramework/DevelopmentInfrastructure/Tests"
    "${PROJECT_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal"
    "${PROJECT_SOURCE_DIR}/EngineFramework/RuntimeBoundary/RuntimeBridge/src")
target_link_libraries(EpidemicWholeEngineRestoreTruthTests PRIVATE
    EpidemicEngineBaseSupport EpidemicRuntimeSupport
    EpidemicGameFrameworkRuntimeBridge
    EpidemicGameFrameworkCoreIntegration EpidemicGameFrameworkStateIntegration
    EpidemicGameFrameworkGameplayIntegration EpidemicGameFrameworkExtendedGameplayIntegration
    EpidemicGameFrameworkWorldIntegration EpidemicGameFrameworkTraversalNavigationConstructionIntegration
    EpidemicGameFrameworkPerceptionKnowledgeAIIntegration EpidemicGameFrameworkPopulationSimulationIntegration
    EpidemicGameFrameworkProcessResourceSimulationIntegration EpidemicGameFrameworkSocialLegalIntegration
    EpidemicGameFrameworkNarrativeIntegration EpidemicGameFrameworkInteractionEffectsIntegration)
if(MSVC)
    target_compile_options(EpidemicWholeEngineRestoreTruthTests PRIVATE /bigobj)
endif()
set_tests_properties(EpidemicWholeEngineRestoreTruthTests PROPERTIES LABELS "truth;persistence-whole-engine;goal5-regression" TIMEOUT 180)
add_test(NAME EpidemicWholeEngineRestoreProcessTruthTests COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tools/ci/test_whole_restore_process.py" "$<TARGET_FILE:EpidemicWholeEngineRestoreTruthTests>")
set_tests_properties(EpidemicWholeEngineRestoreProcessTruthTests PROPERTIES LABELS "truth;persistence-whole-engine;persistence-storage;goal5-regression" TIMEOUT 360)

epidemic_add_truth_test(EpidemicDeterminismTruthTests determinism_tests.cpp)
target_include_directories(EpidemicDeterminismTruthTests PRIVATE
    "${PROJECT_SOURCE_DIR}/EngineFramework/DevelopmentInfrastructure/Tests"
    "${PROJECT_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal"
    "${PROJECT_SOURCE_DIR}/EngineFramework/RuntimeBoundary/RuntimeBridge/src")
get_target_property(goal5_whole_libraries EpidemicWholeEngineRestoreTruthTests LINK_LIBRARIES)
target_link_libraries(EpidemicDeterminismTruthTests PRIVATE ${goal5_whole_libraries})
if(MSVC)
    target_compile_options(EpidemicDeterminismTruthTests PRIVATE /bigobj)
endif()
set_tests_properties(EpidemicDeterminismTruthTests PROPERTIES LABELS "truth;determinism;goal5-regression" TIMEOUT 240)
add_test(NAME EpidemicDeterminismProcessTruthTests COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tools/ci/test_determinism_process.py" "$<TARGET_FILE:EpidemicDeterminismTruthTests>")
set_tests_properties(EpidemicDeterminismProcessTruthTests PROPERTIES LABELS "truth;determinism;goal5-regression" TIMEOUT 900)
