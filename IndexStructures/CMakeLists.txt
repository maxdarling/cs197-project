cmake_minimum_required(VERSION 3.2)
project(IndexStructure)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    include/Adapter/Adapters.hpp
    include/Structures/Cuckoo/CompactInlinedChainedHashMap.hpp
    include/Structures/Cuckoo/InlineChainedHashMap.hpp
    include/Structures/Cuckoo/CacheLineLinearHashTable.hpp
    include/Structures/Cuckoo/CuckooHashTable.hpp
    include/Structures/Cuckoo/LinearHashTable.hpp
    include/Structures/Cuckoo/LinearTombstoneHashTable.hpp
    include/Structures/Cuckoo/LinearTombstoneHashTableSIMD.hpp
    include/Structures/Cuckoo/OpenTableConstants.hpp
    include/Structures/Cuckoo/QuadraticHashTable.hpp
    include/Structures/Cuckoo/RobinDistanceHashTable.hpp
    include/Util/Avalanche.hpp
    include/Util/Collisions.hpp
    include/Util/Defines.hpp
    include/Util/hashFunctions.hpp
    include/Util/HashTestCommons.hpp
    include/Util/Maurer.hpp
    include/Util/MemoryMeasurement.hpp
    include/Util/Mobius.hpp
    include/Util/Timer.hpp
    include/Util/Types.hpp
    include/Util/Zipf.h
    ./test.cpp)

include_directories(
    include/Adapter
    include/Structures/Cuckoo
    include/Util)

add_executable(IndexStructure ${SOURCE_FILES} include/Structures/Cuckoo/ChainedHashMap.hpp include/Structures/Cuckoo/ArrayHashTable.hpp)