cmake_minimum_required(VERSION 3.24)
project(SfmlApp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimized build by default, otherwise the time measurements are meaningless.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Prefer a pre-installed SFML (e.g. in the Docker image); otherwise fetch 3.1.0.
find_package(SFML 3.1 COMPONENTS Graphics Window System QUIET)
if(NOT SFML_FOUND)
  message(STATUS "SFML 3.1 not found locally; fetching SFML 3.1.0 via FetchContent")
  include(FetchContent)
  set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
  )
  FetchContent_MakeAvailable(SFML)
endif()

# Shared code for all convex hull algorithms: data loading, visualization, benchmark.
add_library(hull_common STATIC
  common/dataset.cpp
  common/visualizer.cpp
  common/benchmark.cpp
  common/hull_app.cpp
)
target_include_directories(hull_common PUBLIC common)
target_link_libraries(hull_common PUBLIC SFML::Graphics)
target_compile_definitions(hull_common PRIVATE HULL_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if(MSVC)
  target_compile_definitions(hull_common PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# One program with all algorithms (menu to choose one).
add_executable(hull
  main.cpp
  algorithms/andrew.cpp
  algorithms/quickhull.cpp
)
target_link_libraries(hull PRIVATE hull_common)

if(WIN32)
  add_custom_command(TARGET hull POST_BUILD
    COMMAND $<$<BOOL:$<TARGET_RUNTIME_DLLS:hull>>:${CMAKE_COMMAND}>
    ARGS $<$<BOOL:$<TARGET_RUNTIME_DLLS:hull>>:-E>
         $<$<BOOL:$<TARGET_RUNTIME_DLLS:hull>>:copy_if_different>
         $<TARGET_RUNTIME_DLLS:hull>
         $<$<BOOL:$<TARGET_RUNTIME_DLLS:hull>>:$<TARGET_FILE_DIR:hull>>
    COMMAND_EXPAND_LISTS
  )
  if(MINGW)
    target_link_options(hull PRIVATE -static-libgcc -static-libstdc++)
    get_filename_component(_mingw_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)
    if(EXISTS "${_mingw_bin}/libwinpthread-1.dll")
      add_custom_command(TARGET hull POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${_mingw_bin}/libwinpthread-1.dll"
          "$<TARGET_FILE_DIR:hull>"
      )
    endif()
  endif()
endif()
