//
//  Adapter.hpp
//  IndexStructure
//
//  Created by Stefan Richter on 11.02.14.
//  Copyright (c) 2014 Stefan Richter. All rights reserved.
//

#ifndef IndexStructure_Adapter_hpp
#define IndexStructure_Adapter_hpp

#include <cstddef>
#include <cmath>
#include <unordered_map>
#include <random>
#include <iostream>
#include <ChainedHashMap.hpp>

#include "Defines.hpp"
#include "Types.hpp"
#include "CuckooHashTable.hpp"
#include "OpenTableConstants.hpp"

typedef std::pair<MWord, bool> KeyPair;

enum Distribution {
    DENSE_SORTED = 0,
    DENSE_UNSORTED = 1,
    SPARSE_UNSORTED = 2,
    ZIPF = 3,
    GRID = 4
};

enum OpCode {
    INSERTION = 0,
    DELETION = 1,
    RANGE = 2,
    LOOKUP = 3
};

typedef struct Workload {
    long insertion;
    long deletion_hit;
    long deletion_miss;
//    long range;
    long lookup_hit;
    long lookup_miss;
    size_t pre_n;
    size_t nops;
} Workload;

struct Range {
    MWord first;
    MWord second;
};

union OpData {
    MWord key;
    Range range;
};

typedef struct Operation {
    OpCode opcode;
    OpData opdata;
} Operation;

struct Config {
    char benchmark;
    //decides what benchmark is executed
    size_t numberOfTuples;
    //number of elements in data
    MWord *data;
    //the key data
    Distribution distribution;
    //the distribution for datagen
    //used for mixed only...
//    KeyPair *keyPairs;
    Workload workload;
    //the configuration for the workload we generate
    Operation *ops;//the generated workload as array of <op, value> pairs
};


class ArrayAdapter {

private:
    MWord *map;
    size_t count;
    size_t max;

public:
    ArrayAdapter() : map(new MWord[1024]()) {
        throw;
    }

#ifdef DYNAMIC_GROW

    //we overallocate according to the loadfactor
    ArrayAdapter(size_t numTup) : map(new MWord[numTup + 1]()), max(numTup) {
    }

#else

    //we overallocate according to the loadfactor
    ArrayAdapter(size_t numTup) : map(new MWord[numTup + 1]) {
    }

#endif


    ArrayAdapter(const ArrayAdapter &orig) {
        //TODO unsupported
        throw;
    }

    MWord *getPointer(MWord key) {
        return reinterpret_cast<MWord * >(getValue(key));
    }

    MWord getValue(MWord key) {
        if (key > max)
            return 0;
        else
            return map[key];
    }

    MWord &operator[](const MWord key) {
        return map[key];
    }

    void putPointer(MWord key, MWord *valuePointer) {
        putValue(key, reinterpret_cast<MWord>(valuePointer));
    }

    void putValue(MWord key, MWord value) {
        if (map[key] == 0) {
            ++count;
        }
        map[key] = value;
    }

    bool remove(MWord key) {
        bool res = (0 != map[key]);
        if (res) {
            map[key] = 0;
            --count;
        }
        return res;
    }

    size_t size() {
        return count;
    }

    MWord traverse(MWord startKey, MWord endKey) {
        throw;
    }
};

template<class LHT>
class LinearHashTableAdapter {

private:
    LHT map;

public:
    LinearHashTableAdapter() : map() {
    }

#ifdef DYNAMIC_GROW

    //we overallocate according to the loadfactor
    LinearHashTableAdapter(size_t numTup) : map(std::log2(numTup * (1 / LH_MAX_LOAD_FACTOR)) + (!(numTup & (numTup - 1)) ? 0 : 1)) {
//		std::cout << numTup << std::endl;
//		std::cout << 1 / LH_MAX_LOAD_FACTOR << std::endl;
//		std::cout << (!(numTup & (numTup - 1)) ? 0 : 1) << std::endl;
//		std::cout << std::log2(numTup * (1 / LH_MAX_LOAD_FACTOR)) << std::endl;
//		std::cout << std::log2(numTup * (1 / LH_MAX_LOAD_FACTOR)) + (!(numTup & (numTup - 1)) ? 0 : 1) << std::endl;
    }

#else

    LinearHashTableAdapter(size_t numTup) : map(std::log2(numTup) + 1) {
    }

#endif


    LinearHashTableAdapter(const LHT &orig) {
        //TODO unsupported
        throw;
    }

    MWord *getPointer(MWord key) {
        return reinterpret_cast<MWord * >(getValue(key));
    }

    MWord getValue(MWord key) {
        auto result = map.get(key);
        return result.value;
    }

    MWord &operator[](const MWord key) {
        return map[key];
    }

    void putPointer(MWord key, MWord *valuePointer) {
        //tree.insert(std::pair<MWord, MWord >(key, reinterpret_cast<MWord >(valuePointer)));
        map.put(key, reinterpret_cast<MWord >(valuePointer));
    }

    void putValue(MWord key, MWord value) {
        //tree.insert(std::pair<MWord, MWord >(key, value));
        map.put(key, value);
    }

    bool remove(MWord key) {
        return map.remove(key).found;
    }

    size_t size() {
#ifdef LH_DEBUG_PROBE_COUNT
        map.printDebugInfo();
#endif
        return map.getCount();
    }

    MWord traverse(MWord startKey, MWord endKey) {
        throw;
    }

    MWord getBulkSum(const MWord *keys, const size_t startIdx, const size_t endIdx) {
        return map.getBulkSum(keys, startIdx, endIdx);
    }
};

MWord getSizeForCuckoo(MWord size, unsigned char N) {
    //Accounting for load factors using different number of tables
//	size /= N;
#ifdef FAST_MORE_TABLES
	size *= 2;
#else
    if (N == 2) {
        //Using two tables we get load factor of roughly 50%
//		size *= 2;
    } else if (N == 3) {
        //Using three tables we get a load factor of 88-90%
        size /= N;
        size *= 1.1;
    } else if (N == 4) {
        //Using four tables we get load factor of roughly 97%
        size /= N;
        size *= 1.04;
    }
#endif
    return size;
}

template<class Hasher, uint8_t N>
class CuckooNHashAdapter {
private:
    Cuckoo_hashtable<Hasher, N> hashmap;

public:
    CuckooNHashAdapter() : hashmap(6) {
    }

    CuckooNHashAdapter(MWord size, unsigned char n = N)
            : hashmap(static_cast<unsigned short>(std::log2(getSizeForCuckoo(size, N))) + (!(getSizeForCuckoo(size, N) & (getSizeForCuckoo(size, N) - 1)) ? 0 : 1)) {

//        hashmap = *(new Cuckoo_hashtable<Hasher, N>(static_cast<unsigned short>(std::log2(size)) + (!(size & (size - 1)) ? 0 : 1)));
    }

    MWord *getPointer(MWord key) {
        return reinterpret_cast<MWord *>(getValue(key));
    }

    MWord getValue(MWord key) {
//		return hashmap[key];
        MWord *result = hashmap.get(key);
        return (result != nullptr) ? *result : 0;
    }

    void putPointer(MWord key, MWord *valuePointer) {
        hashmap.put(key, reinterpret_cast<MWord >(valuePointer));
    }

    void putValue(MWord key, MWord value) {
        hashmap.put(key, value);
//		MWord& location = hashmap[key];
//		location = value;
    }

    MWord &operator[](const MWord key) {
        return hashmap[key];
    }

    bool remove(MWord key) {
        return hashmap.remove(key);
    }

    size_t size() {
        return hashmap.get_size();
    }

    MWord traverse(MWord startKey, MWord endKey) {
//        //In case the range is discrete and "small"
        MWord sum = 0;

//        hashmap.traverse(startKey, endKey, sum);

        for (MWord elem = startKey; elem <= endKey; ++elem)
            sum += getValue(elem);

        return sum;
    }

};

template<class Hasher>
class UnorderedMapAdapter {
private:
    std::unordered_map<MWord, MWord, Hasher> map;

public:
    UnorderedMapAdapter() : map(std::unordered_map<MWord, MWord, Hasher>()) {
    }

    UnorderedMapAdapter(MWord size) : map(std::unordered_map<MWord, MWord, Hasher>()) {
#ifdef UNORDERED_CONTROLLED_LOAD_FACTOR
//		std::cout << "unordered_map controlled load factor" << std::endl;
        uint32_t log2Size = std::log2(size) + 1;
        uint64_t nextPower2 = std::pow(2, log2Size);
//        map.max_load_factor(static_cast<float>(size) / nextPower2);
//		std::cout << log2Size << ", " << nextPower2 << std::endl;
        map.max_load_factor(1);
        map.reserve(nextPower2);
#else
		map.max_load_factor(LH_MAX_LOAD_FACTOR);
		map.reserve(size);
#endif

//		std::cout << map.max_load_factor() << std::endl;
//		std::cout << map.bucket_count() << std::endl;
//		std::cout << map.size() << std::endl;
    }

    ~UnorderedMapAdapter() {
        std::cout << map.max_load_factor() << std::endl;
        std::cout << map.bucket_count() << std::endl;
        std::cout << map.size() << std::endl;
        std::cout << map.load_factor() << std::endl;
    }

    MWord *getPointer(MWord key) {
        return reinterpret_cast<MWord *> (getValue(key));
    }

    MWord getValue(MWord key) {
        auto result = map.find(key);
        return (result != map.end()) ? result->second : 0;
    }

    MWord &operator[](const MWord key) {
        return map[key];
    }

    void putPointer(MWord key, MWord *valuePointer) {
        putValue(key, reinterpret_cast<MWord>(valuePointer));
    }

    void putValue(MWord key, MWord value) {
        map.insert({key, value});
    }

    bool remove(MWord key) {
        return map.erase(key);
    }

    size_t size() {
        return map.size();
    }

    MWord traverse(MWord startKey, MWord endKey) {
        //TODO unsupported
        return 0;
    }
};

template<class MAP>
class UnorderedMapAdapter2 {

private:
    MAP map;


public:
    UnorderedMapAdapter2() : map() {
    }

    UnorderedMapAdapter2(MWord size) : map(size) {

    }

    ~UnorderedMapAdapter2() {

    }

    MWord *getPointer(MWord key) {
        return reinterpret_cast<MWord *> (getValue(key));
    }

    MWord getValue(MWord key) {
        return map.getValue(key);
    }

    MWord &operator[](const MWord key) {
        return map[key];
    }

    void putPointer(MWord key, MWord *valuePointer) {
        map.putValue(key, reinterpret_cast<MWord>(valuePointer));
    }

    void putValue(MWord key, MWord value) {
        return map.putValue(key,value);
    }

    bool remove(MWord key) {
        return map.remove(key);
    }

    size_t size() {
        return map.size();
    }

    MWord traverse(MWord startKey, MWord endKey) {
        throw;
    }

    uint64_t debugGetWastedSpaceMB() {
        return map.debugGetWastedSpaceMB();
    }

    MWord getBulkSum(const MWord *keys, const size_t startIdx, const size_t endIdx) {
        return map.getBulkSum(keys, startIdx, endIdx);
    }
};

#endif
