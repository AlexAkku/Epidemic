add_library(EpidemicGameComposition STATIC src/graph_host.cpp)
target_include_directories(EpidemicGameComposition PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(EpidemicGameComposition PUBLIC EpidemicGameFrameworkCoreIntegration
    EpidemicGameFrameworkRuntimeBridge EpidemicRuntimeSupport)
epidemic_configure_target(EpidemicGameComposition)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
