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
