cmake_minimum_required(VERSION 3.25)
project(EpidemicMinimalGame LANGUAGES CXX)
set(EPIDEMIC_REQUIRED_ABI 1 CACHE STRING "Required Epidemic ABI")
find_package(Epidemic 1.1.0 CONFIG REQUIRED COMPONENTS Presentation Session)
add_executable(EpidemicMinimalGame main.cpp)
target_link_libraries(EpidemicMinimalGame PRIVATE
    Epidemic::EpidemicGameFrameworkPresentation Epidemic::EpidemicGameFrameworkSession)
target_compile_features(EpidemicMinimalGame PRIVATE cxx_std_23)
