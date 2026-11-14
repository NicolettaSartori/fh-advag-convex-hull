cmake_minimum_required(VERSION 3.16)
project(SfmlApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimized build by default, otherwise the time measurements are meaningless.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(SFML 3.1 COMPONENTS Graphics Window System REQUIRED)

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

# One program with all algorithms (menu to choose one).
add_executable(hull
  main.cpp
  algorithms/andrew.cpp
  algorithms/quickhull.cpp
)
target_link_libraries(hull PRIVATE hull_common)
