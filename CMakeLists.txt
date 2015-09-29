cmake_minimum_required(VERSION 3.2)
project(seabolt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out")

set(SOURCE_FILES main.cpp)
add_executable(seabolt ${SOURCE_FILES} packstream.cpp bolt.cpp main.cpp)
