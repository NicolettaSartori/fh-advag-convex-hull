cmake_minimum_required(VERSION 3.16)
project(SfmlApp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

add_executable(app main.cpp)
target_link_libraries(app sfml-graphics sfml-window sfml-system sfml-audio)
