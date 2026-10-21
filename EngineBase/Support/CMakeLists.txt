add_library(EpidemicEngineBaseSupport
    src/engine_base_support.cpp
)

target_include_directories(EpidemicEngineBaseSupport
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(EpidemicEngineBaseSupport
    PUBLIC
        EpidemicFoundation
        EpidemicCore
        EpidemicMemory
        EpidemicDiagnostics
        EpidemicPlatform
        EpidemicInput
        EpidemicRHI
)
if(WIN32)
    target_link_libraries(EpidemicEngineBaseSupport PRIVATE EpidemicRHI_D3D11)
elseif(EMSCRIPTEN)
    target_link_libraries(EpidemicEngineBaseSupport PRIVATE EpidemicRHI_WebGL)
endif()

epidemic_configure_target(EpidemicEngineBaseSupport)
