cmake_minimum_required(VERSION 3.2)
project(neo4j-c-driver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(neo4j-c-driver ${SOURCE_FILES} packstream.cpp bolt.cpp main.cpp)
