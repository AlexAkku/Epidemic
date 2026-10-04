add_executable(EpidemicRuntimeSupportTests
    runtime_support_tests.cpp
    runtime_package_tests.cpp
)

target_link_libraries(EpidemicRuntimeSupportTests
    PRIVATE EpidemicRuntimeSupport
)

epidemic_configure_target(EpidemicRuntimeSupportTests)
epidemic_add_test(EpidemicRuntimeSupportTests)
target_compile_definitions(EpidemicRuntimeSupportTests PRIVATE EPIDEMIC_RUNTIME_FIXTURES="${PROJECT_SOURCE_DIR}/Tests/Fixtures/RuntimePersistence")
set_tests_properties(EpidemicRuntimeSupportTests PROPERTIES LABELS "persistence-schema;persistence-wire;persistence-storage")
