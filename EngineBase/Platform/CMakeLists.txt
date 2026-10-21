if(NOT WIN32 AND NOT EMSCRIPTEN)
    message(FATAL_ERROR "The platform slice supports Windows or Emscripten Web.")
endif()

if(EMSCRIPTEN)
    add_library(EpidemicPlatform STATIC src/Web/browser_platform_runtime.cpp src/Web/web_file_package.cpp)
else()
    add_library(EpidemicPlatform STATIC src/Windows/windows_platform_runtime.cpp src/Windows/windows_file_package.cpp)
endif()

target_include_directories(EpidemicPlatform PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_include_directories(EpidemicPlatform PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(EpidemicPlatform
    PUBLIC EpidemicFoundation
)
if(WIN32)
    target_link_libraries(EpidemicPlatform PRIVATE user32 shell32)
endif()

epidemic_configure_target(EpidemicPlatform)
