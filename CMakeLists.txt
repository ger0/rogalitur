cmake_minimum_required(VERSION 3.7)

project(rogalik)

set(CMAKE_CXX_STANDARD 20)
find_package(SDL2 REQUIRED)
find_package(fmt)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(rogalik main.cpp player.cpp)
target_link_libraries(rogalik ${SDL2_LIBRARIES})
target_link_libraries(rogalik fmt::fmt)
