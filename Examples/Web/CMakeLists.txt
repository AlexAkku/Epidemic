set(EPIDEMIC_WEB_ASCENT_SOURCE "" CACHE PATH "Optional external ASCENT Game directory for a diagnostic WASM slice")
function(epidemic_web_page target source output shell)
    add_executable(${target} ${source})
    set_target_properties(${target} PROPERTIES SUFFIX ".html" OUTPUT_NAME "${output}" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")
    target_link_options(${target} PRIVATE "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/${shell}" -sASSERTIONS=1)
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${shell}")
    epidemic_configure_target(${target})
endfunction()
epidemic_web_page(EpidemicWebPlatformApp platform_app.cpp platform shell.html)
target_link_libraries(EpidemicWebPlatformApp PRIVATE EpidemicEngineBaseSupport)

add_executable(EpidemicWebContracts web_contracts.cpp)
target_link_libraries(EpidemicWebContracts PRIVATE EpidemicCore EpidemicPlatform)
set_target_properties(EpidemicWebContracts PROPERTIES SUFFIX ".js" OUTPUT_NAME web-contracts RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")
target_link_options(EpidemicWebContracts PRIVATE -sEXIT_RUNTIME=1 -sENVIRONMENT=node -sASSERTIONS=1)
epidemic_configure_target(EpidemicWebContracts)

if(EPIDEMIC_WEB_ASCENT_SOURCE)
    if(NOT TARGET EpidemicGameFrameworkSession OR NOT EXISTS "${EPIDEMIC_WEB_ASCENT_SOURCE}/Source/gameplay.cpp")
        message(FATAL_ERROR "ASCENT diagnostic requires Framework and a Game directory containing Source/gameplay.cpp")
    endif()
    set(_logic circuit_board tree_graph skill_tree attract_scene cards localization abilities developer world route_geometry corridor gameplay progression bosses copy_boss enemies fusions combat loot profile)
    list(TRANSFORM _logic PREPEND "${EPIDEMIC_WEB_ASCENT_SOURCE}/Source/")
    list(TRANSFORM _logic APPEND ".cpp")
    add_library(AscentWebSimulation STATIC ${_logic})
    target_include_directories(AscentWebSimulation PUBLIC "${EPIDEMIC_WEB_ASCENT_SOURCE}/Source")
    # Combat uses the portable header-only cutout math for enemy body bounds.
    # This does not link or claim a Web implementation of desktop Presentation.
    target_include_directories(AscentWebSimulation PRIVATE
        "${PROJECT_SOURCE_DIR}/EngineFramework/RuntimeBoundary/Presentation/include")
    target_link_libraries(AscentWebSimulation PUBLIC EpidemicGameFrameworkSession EpidemicGameFrameworkPresentation)
    target_compile_features(AscentWebSimulation PUBLIC cxx_std_23)
    epidemic_web_page(AscentWebDiagnostic ascent_diagnostic.cpp ascent ascent-shell.html)
    target_link_libraries(AscentWebDiagnostic PRIVATE AscentWebSimulation EpidemicEngineBaseSupport)
    include("${EPIDEMIC_WEB_ASCENT_SOURCE}/cmake/WebSources.cmake")
    epidemic_web_page(AscentWebGame
        "${EPIDEMIC_WEB_ASCENT_SOURCE}/Source/web_main.cpp" ascent-game game-shell.html)
    target_sources(AscentWebGame PRIVATE ${ASCENT_WEB_PRESENTATION_SOURCES})
    target_link_libraries(AscentWebGame PRIVATE AscentWebSimulation EpidemicGameFrameworkPresentation)
    target_link_options(AscentWebGame PRIVATE
        "--pre-js=${PROJECT_SOURCE_DIR}/EngineRuntime/Support/Presentation/Web/presentation.js"
        "--pre-js=${PROJECT_SOURCE_DIR}/EngineRuntime/Support/Presentation/Web/audio.js"
        -lidbfs.js "-sEXPORTED_RUNTIME_METHODS=['FS','IDBFS','callMain']" -sFORCE_FILESYSTEM=1)
    set_property(TARGET AscentWebGame APPEND PROPERTY LINK_DEPENDS
        "${PROJECT_SOURCE_DIR}/EngineRuntime/Support/Presentation/Web/presentation.js"
        "${PROJECT_SOURCE_DIR}/EngineRuntime/Support/Presentation/Web/audio.js")
endif()
configure_file(index.html "${CMAKE_BINARY_DIR}/web/index.html" COPYONLY)
