cmake_minimum_required(VERSION 3.16)
project(SfmlApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimized build by default, otherwise the time measurements are meaningless.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SFML 3.1 COMPONENTS Graphics Window System Audio REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE SFML::Graphics SFML::Audio)

# Shared code for all convex hull algorithms: data loading, visualization, benchmark.
add_library(hull_common STATIC
  common/dataset.cpp
  common/visualizer.cpp
  common/benchmark.cpp
  common/hull_app.cpp
)
target_include_directories(hull_common PUBLIC common)
target_link_libraries(hull_common PUBLIC SFML::Graphics)
target_compile_definitions(hull_common PRIVATE HULL_PROJECT_DIR="${CMAKE_SOURCE_DIR}")

# One executable per algorithm.
add_executable(andrew algorithms/andrew.cpp)
target_link_libraries(andrew PRIVATE hull_common)
