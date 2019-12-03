cmake_minimum_required(VERSION 3.2)
project(IndexStructure)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    ./adaptive_hash.hpp
    ./test_harness.cpp)

include_directories(
    ./Util)

add_executable(IndexStructure ${SOURCE_FILES} Utils/ChainedHashTable.hpp Utils/ArrayHashTable.hpp)