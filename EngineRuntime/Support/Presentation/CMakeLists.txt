add_library(EpidemicRuntimePresentation STATIC host.cpp renderer.cpp sound.cpp music.cpp blob_store.cpp)
target_include_directories(EpidemicRuntimePresentation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(EpidemicRuntimePresentation PRIVATE
    EpidemicRuntimeSupport EpidemicRuntimeRenderer EpidemicRuntimeAudio EpidemicEngineBaseSupport
    d2d1 dxguid dwrite d3d11 dxgi windowscodecs ole32 winmm shell32 xaudio2 user32)
target_compile_definitions(EpidemicRuntimePresentation PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_compile_options(EpidemicRuntimePresentation PRIVATE /utf-8)
epidemic_configure_target(EpidemicRuntimePresentation)

if(BUILD_TESTING)
    add_executable(EpidemicPresentationTests tests/presentation_tests.cpp)
    target_link_libraries(EpidemicPresentationTests PRIVATE EpidemicRuntimePresentation)
    target_compile_definitions(EpidemicPresentationTests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    epidemic_configure_target(EpidemicPresentationTests)
    epidemic_add_test(EpidemicPresentationTests)
endif()
