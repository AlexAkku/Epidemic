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

epidemic_add_truth_test(EpidemicDeterminismSaveLoadTruthTests determinism_save_load_tests.cpp)
file(GLOB_RECURSE ms5_required_engine_sources CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/EngineBase/*.h" "${PROJECT_SOURCE_DIR}/EngineBase/*.hpp"
    "${PROJECT_SOURCE_DIR}/EngineBase/*.cpp" "${PROJECT_SOURCE_DIR}/EngineBase/*.cc"
    "${PROJECT_SOURCE_DIR}/EngineBase/*.cxx" "${PROJECT_SOURCE_DIR}/EngineBase/*.inl"
    "${PROJECT_SOURCE_DIR}/EngineRuntime/*.h" "${PROJECT_SOURCE_DIR}/EngineRuntime/*.hpp"
    "${PROJECT_SOURCE_DIR}/EngineRuntime/*.cpp" "${PROJECT_SOURCE_DIR}/EngineRuntime/*.cc"
    "${PROJECT_SOURCE_DIR}/EngineRuntime/*.cxx" "${PROJECT_SOURCE_DIR}/EngineRuntime/*.inl"
    "${PROJECT_SOURCE_DIR}/EngineFramework/*.h" "${PROJECT_SOURCE_DIR}/EngineFramework/*.hpp"
    "${PROJECT_SOURCE_DIR}/EngineFramework/*.cpp" "${PROJECT_SOURCE_DIR}/EngineFramework/*.cc"
    "${PROJECT_SOURCE_DIR}/EngineFramework/*.cxx" "${PROJECT_SOURCE_DIR}/EngineFramework/*.inl")
list(FILTER ms5_required_engine_sources EXCLUDE REGEX "/(build|\\.ci_cache|Deltas)/")
set(ms5_required_source_names
    Tests/Truth/goal5_required_scenarios.h Tests/Truth/goal5_required_scenario_runner.h
    Tests/Truth/determinism_save_load_tests.cpp Tests/Truth/goal5_whole_composition.h
    Tests/Truth/goal5_replay.h Tests/Truth/goal5_scenario.h Tests/Truth/goal5_digest.h
    Tests/Truth/goal5_digest_value_codecs.h
    EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h
    tools/ci/test_required_determinism_process.py Tests/Truth/CMakeLists.txt)
foreach(ms5_source IN LISTS ms5_required_engine_sources)
    file(RELATIVE_PATH ms5_relative "${PROJECT_SOURCE_DIR}" "${ms5_source}")
    list(APPEND ms5_required_source_names "${ms5_relative}")
endforeach()
list(REMOVE_DUPLICATES ms5_required_source_names)
list(SORT ms5_required_source_names)
set(ms5_required_source_material "")
foreach(ms5_relative IN LISTS ms5_required_source_names)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/${ms5_relative}")
    file(SHA256 "${PROJECT_SOURCE_DIR}/${ms5_relative}" ms5_source_hash)
    string(APPEND ms5_required_source_material "${ms5_relative}:${ms5_source_hash}\n")
endforeach()
string(SHA256 ms5_required_build_sha256 "${ms5_required_source_material}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/goal5_required_build_stamp.h"
    CONTENT "#pragma once\nnamespace goal5_test_utility { inline constexpr char RequiredBuildSourceSha256[] = \"${ms5_required_build_sha256}\"; }\n")
target_include_directories(EpidemicDeterminismSaveLoadTruthTests PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "${PROJECT_SOURCE_DIR}/EngineFramework/DevelopmentInfrastructure/Tests"
    "${PROJECT_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal"
    "${PROJECT_SOURCE_DIR}/EngineFramework/RuntimeBoundary/RuntimeBridge/src")
target_link_libraries(EpidemicDeterminismSaveLoadTruthTests PRIVATE ${goal5_whole_libraries})
if(MSVC)
    target_compile_options(EpidemicDeterminismSaveLoadTruthTests PRIVATE /bigobj)
endif()
set_tests_properties(EpidemicDeterminismSaveLoadTruthTests PROPERTIES LABELS "truth;determinism;determinism-save-load;persistence-whole-engine;goal5-regression" TIMEOUT 900)
add_test(NAME EpidemicDeterminismRequiredProcessTruthTests COMMAND "${Python3_EXECUTABLE}"
    "${PROJECT_SOURCE_DIR}/tools/ci/test_required_determinism_process.py"
    "$<TARGET_FILE:EpidemicDeterminismSaveLoadTruthTests>")
set_tests_properties(EpidemicDeterminismRequiredProcessTruthTests PROPERTIES
    LABELS "truth;determinism;determinism-save-load;persistence-whole-engine;goal5-regression" TIMEOUT 900)
