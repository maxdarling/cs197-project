/*
 * File:   main.cpp
 * Author: stefan
 *
 * Created on 6. Februar 2014, 11:02
 */
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <set>
#include <random>
#include <map>

#include <unordered_set>
#include <algorithm>
#include <ArrayHashTable.hpp>

#include "hashFunctions.hpp"
#include "Timer.hpp"
#include "Adapters.hpp"
#include "MemoryMeasurement.hpp"
#include "Types.hpp"
#include "Zipf.h"
#include "Defines.hpp"
#include "LinearHashTable.hpp"
#include "LinearTombstoneHashTable.hpp"
#include "LinearTombstoneHashTableSIMD.hpp"
#include "QuadraticHashTable.hpp"
#include "CompactInlinedChainedHashMap.hpp"
#include "InlineChainedHashMap.hpp"
#include "CompactInlinedChainedHashMap.hpp"

#if defined(PROFILE_LOOKUPS) || defined(PROFILE_INSERTIONS)
#include <ittnotify.h>
#endif

using namespace std;

const size_t MEGA_BYTE = 1024 * 1024;
const double MILLION = 1000000.0;
const MWord MIN_LOOKUPS = 10000000;
//double SELECTIVITY = 0.001;
double SKEW = 0.8614; // 80-20 rule for Stefan's Zipf.


typedef void (*TestFunPtr)(char **argv, int argc, Config &);

typedef pair<string, TestFunPtr> IdxTestCase;

template<class T>
void runMixedBenchmark(const Config &conf) {
    size_t startMemory = getCurrentRSS();
#ifndef FOR_PLOT
    cout << "TypeID: " << typeid(T).name() << endl;
#endif
    const size_t n = conf.workload.nops;
    // number of pre inserted tuples
    const size_t pre_n = conf.workload.pre_n;

#ifndef INIT_WITH_SIZE
    T map;
#else
    // total number of inserted tuples is the sum of pre inserted tuples and
    // after start benchmarking
    T map(conf.workload.pre_n);
#endif

    MWord *keys = conf.data;
    for (size_t i = 0; i < pre_n; ++i) {
        map.putValue(keys[i], i + 14352456);
    }

//    std::cout << "Check!" << std::endl;

    MWord check_sum = 0;
    MWord range_sum = 0;

    // start counting time after initialization
    double start = getTimeSec();

    Operation *ops = conf.ops;
    for (size_t i = 0; i < n; ++i) {
        Operation operation = ops[i];
        MWord key = operation.opdata.key;
        switch (operation.opcode) {
            case INSERTION:
                map.putValue(key, key + 42);
                break;
            case DELETION:
                map.remove(key);
                break;
            case RANGE:
                range_sum += map.traverse(operation.opdata.range.first, operation.opdata.range.second);
                break;
            default:
                check_sum += map.getValue(key);
                break;
        }
    }

    double time = getTimeSec() - start;
    size_t endMemory = getCurrentRSS();
    float memoryUsage = (endMemory - startMemory) / ((float) MEGA_BYTE);

#ifdef FOR_PLOT
    cout << (n / MILLION) / (time) << " ";
    cout << memoryUsage << " ";
    cout << check_sum << " ";
    cout << map.size() << " ";
#else
//    cout << n << "nops" << endl;
    cout << (n / MILLION) / (time) << "M requests per second" << endl;
    cout << "map size: " << map.size() << endl;
    cout << "memory footprint (MB): " << memoryUsage << endl;
    cout << "checksum: " << check_sum << endl;
    cout << "range checksum: " << range_sum << endl;
//    map.printHistogram();
#endif

}

template<class T>
void runBenchmarkIndex(const Config &conf) {
//-----------------------------------------------------------------------------------------------------INIT
    size_t startMemoryB = getCurrentRSS();

#ifndef FOR_PLOT
    cout << "TypeID: " << typeid(T).name() << endl;
#endif
    const size_t n = conf.numberOfTuples;
#ifndef INIT_WITH_SIZE
    T map;
#else
    T map(conf.numberOfTuples);
#endif
//-----------------------------------------------------------------------------------------------------INSERTS
    MWord *keys = conf.data;
    double start = getTimeSec();

#ifdef PROFILE_INSERTIONS
    __itt_resume();
#endif

    // if it's zipf distribution, we insert dense keys from 1 to n
    if (conf.distribution == ZIPF) {
        for (MWord i = 1; i <= n; ++i)
            map.putValue(i, i);
    } else {
        /*
        for (MWord i = 0; i < n; i++) {
            //		std::cout << "key: " << keys[i] << std::endl;
            //Inserting
#ifdef COVERING_IDX
            map.putValue(keys[i], keys[i]); //to see art w/o store fail, add some value to keys[i]
#else
			map.putPointer(keys[i], &(keys[i]));
#endif
        }
         */
        for (MWord i = 0; i < n - 1; i++) {
            map.putValue(keys[i], keys[i + 1]);
        }
        map.putValue(keys[n - 1], keys[0]);
    }

#ifdef PROFILE_INSERTIONS
    __itt_pause();
#endif

    double time = getTimeSec() - start;

#ifdef FOR_PLOT
    cout << (n / MILLION) / (time) << " ";
#else
    cout << "insert," << n << "," << (n / MILLION) / (time) << " M/s," << time << " (s)" << endl;
#endif

//-----------------------------------------------------------------------------------------------------POINT QUERY
    MWord pointSum = 0;
#ifdef DO_POINT_QUERIES
    if (conf.workload.lookup_miss > 0) {
        std::random_shuffle(keys, keys + n);
        size_t numFailKeys = n * conf.workload.lookup_miss / 100;
        std::mt19937_64 gen(325246);
        unsigned long long int max = ~0ULL;
        std::uniform_int_distribution<MWord> dis_key(1, max);
        for (size_t i = 0; i < numFailKeys; ++i) {
            keys[i] = dis_key(gen);
        }
    }
    //We shuffle the keys once more before performing lookups
    std::random_shuffle(keys, keys + n);

    /*// for verifying the correctness of remove function
    for (MWord i = 0; i < n/2; i++) {
        map.remove(keys[i]); //to see art w/o store fail, add some value to keys[i]
    }*/

    // Repeat lookup for small maps to get reproducable results
    MWord repeat = MIN_LOOKUPS / n;
    if (repeat < 1)
        repeat = 1;

    start = getTimeSec();

//	std::cout << "performing: " << repeat * n << " lookups" << std::endl;

#ifdef PROFILE_LOOKUPS
    __itt_resume();
#endif

#ifndef FOR_PLOT
    cout << "repeats: " << repeat << endl;
#endif
/*
    for (MWord r = 0; r < repeat; r++) {
        for (MWord i = 0; i < n; i++) {
#ifdef COVERING_IDX
            pointSum += (map.getValue(keys[i]));
#else
			pointSum += *(map.getPointer(keys[i]));
#endif
        }
    }
*/

    for (uint64_t r = 0; r < repeat; r++) {
        uint64_t key = keys[0];
        uint64_t val = key;
        for (uint64_t i = 0; i < n; i++) {
#       ifdef LOOKUP_DEPENDENCY
            key = val;
#       else
            key = keys[i];
#       endif
            val = map.getValue(key);
            pointSum += val;
        }
    }

/*
    for (MWord r = 0; r < repeat; r++) {

        pointSum += (map.getBulkSum(keys, 0, n));

    }
*/

#ifdef PROFILE_LOOKUPS
    __itt_pause();
#endif

    time = getTimeSec() - start;

#ifdef FOR_PLOT
    cout << (n * repeat / MILLION) / (time) << " ";
#else
    cout << "lookup," << n << "," << (n * repeat / MILLION) / (time) << " M/s," << time << " (s)" << endl;
#endif
#endif
////-----------------------------------------------------------------------------------------------------RANGE QUERY
    size_t endMemoryB = getCurrentRSS();
    MWord rangeSum = 0;
    const MWord totalQueries = 1000;//number of range queries fired
    pair<MWord, MWord> *queries = new pair<MWord, MWord>[totalQueries];
    float currentMemoryMB;
#ifdef COVERING_IDX
    currentMemoryMB = ((endMemoryB - startMemoryB) / (float) MEGA_BYTE);
#else
    currentMemoryMB = ((endMemoryB) / (float) MEGA_BYTE);
#endif
#ifdef FOR_PLOT
    cout << currentMemoryMB << " ";
    cout << pointSum << " ";
    cout << rangeSum << " ";
    //cout << map.debugGetWastedSpaceMB();
    //cout << RangeSum;
//    printf("%f ", (n * repeat / MILLION) / (time));
//    printf("%f", currentMemoryMB);
//    //avoid dead code elimination
//    if (sum == 666) {
//        printf("anti-dce: this should never happen!\n");
//    }
#else
//    cout << "range lookup," << n << "," << (n * repeat / MILLION) / (time) << " M/s," << time << " (s)" << endl;
    cout << "map size: " << map.size() << endl;
    cout << "memory footprint (MB): " << currentMemoryMB << endl;
    cout << "checksum point: " << pointSum << endl;
    cout << "checksum range: " << rangeSum << endl;
//    map.printHistogram();
#endif
    delete[] queries;
}

Distribution findDistributionFromParam(string distString) {
    std::unordered_map<std::string, uint8_t> valueDistributionMap;
//    google::sparse_hash_map<string, uint8_t > valueDistributionMap;
    valueDistributionMap["DENSE_SORTED"] = DENSE_SORTED;
    valueDistributionMap["DENSE_UNSORTED"] = DENSE_UNSORTED;
    valueDistributionMap["SPARSE_UNSORTED"] = SPARSE_UNSORTED;
    valueDistributionMap["ZIPF"] = ZIPF;
    valueDistributionMap["GRID"] = GRID;
    //get the distribution
    auto distRes = valueDistributionMap.find(distString);
    if (distRes == valueDistributionMap.end()) {
        printf("Available value distributions are:\n");
        for (auto iter = valueDistributionMap.begin(); iter != valueDistributionMap.end(); ++iter) {
            printf("- %s\n", iter->first.c_str());
        }
        exit(1);
    }
    return static_cast<Distribution >(distRes->second);
}


void generateIndexData(MWord *keys, Distribution dist, size_t n, size_t seed) {
    // Generate keys
    string msg = "dense, sorted";

#ifdef __x86_64__
    std::mt19937_64 gen(seed);
    unsigned long long int max = ~0ULL;
#else
    std::mt19937 gen(19508);
    unsigned long int max = ~0ULL;
#endif

    MWord holeFactor = 0;
    // create sparse, unique keys
    for (MWord i = 0; i < n; i++) {
        // dense, sorted
//        keys[i] = i + 1;
        keys[i] = (i);
//        if (i % 1000 == 0) {
//            holeFactor += (123 + (i/1000000));
//        }
    }

    if (dist == ZIPF) {
        std::uniform_int_distribution<MWord> dis_key(1, max);
        Zipf zipf(n, SKEW, dis_key(gen));
        for (MWord i = 0; i < n; ++i) {
            keys[i] = zipf.next();
        }

#ifndef FOR_PLOT
        std::cout << "Zipf generation finished!" << std::endl;
#endif
    }

    if (dist == DENSE_UNSORTED) {
        // dense, random
        std::uniform_int_distribution<MWord> dis_key(0, max);
        uint64_t randomLocation = 0;

        // Adding our own random shuffling using the already allocated mersenne twister
        // We do this because we have different seeds for different runs.
        for (MWord i = 0; i < n; ++i) {
            randomLocation = dis_key(gen) % (n - i);
            std::swap(keys[i], keys[i + randomLocation]);
        }
    }

    if (dist == SPARSE_UNSORTED) {
#ifndef FOR_PLOT
        std::cout << "Using Mersenne Twister generation!" << std::endl;
#endif
        // "pseudo-sparse"
        msg = "pseudo-sparse";
        std::uniform_int_distribution<MWord> dis_key(1, max);

        // create sparse, unique keys
        unsigned count = 0;
        LinearTombstoneHashTable<MWord, MWord, MurmurHash, 0L, 0xFFFFFFFFFFFFFFFF> keySet(
                std::log2(n) + 1);
        while (count < n) {
            MWord key = dis_key(gen);
            if (!keySet.put(key, 42).found) {
                keys[count] = key;
                ++count;
            }
        }
        keySet.clear();
    }

    if (dist == GRID) {
        size_t cElem = 0x0101010101010101;
        size_t pos = 0;
        uint8_t byte = 0;
        uint8_t *smallDigits = 0;
        uint8_t numDigits = 14;
        uint64_t universeSize = std::pow(numDigits, 8);

        if (n > universeSize) {
            std::cout << "Either: increase the universe or decrease the total number of elements (pre + insertions)" <<
            std::endl;
            std::cout << "Current universe size: " << universeSize << std::endl;
            exit(1);
        }

        keys[pos] = cElem;
        ++pos;

        while (byte < sizeof(MWord)) {
            uint64_t currentNumElems = pos;
            for (uint64_t elem = 0; elem < currentNumElems; ++elem) {
                cElem = keys[elem];
                smallDigits = reinterpret_cast<uint8_t *>(&cElem);
                for (uint8_t digit = 2; digit <= numDigits; ++digit) {
                    if (pos >= n) {
                        elem = currentNumElems;
                        byte = sizeof(MWord);
                        break;
                    } else {
                        smallDigits[byte] = digit;
                        keys[pos] = reinterpret_cast<size_t *>(smallDigits)[0];
                        ++pos;
                    }
                }
            }
            ++byte;
        }

        std::uniform_int_distribution<MWord> dis_key(0, max);
        uint64_t randomLocation = 0;
        // Adding our own random shuffling using the already allocated mersenne twister
        // We do this because we have different seeds for different runs.
        for (MWord i = 0; i < n; ++i) {
            randomLocation = dis_key(gen) % (n - i);
            std::swap(keys[i], keys[i + randomLocation]);
        }
    }
}

void generateMixedData(MWord *genKeys, Operation *ops, const Workload &workload, Distribution dist, size_t seed) {
    size_t numOperations = workload.nops;
    MWord total_tuples = workload.pre_n + (size_t) ((workload.insertion / 100.0) * workload.nops);
    KeyPair *keys = new KeyPair[total_tuples];

    size_t nput = numOperations * (workload.insertion / 100.0);
    size_t nget_h = numOperations * (workload.lookup_hit / 100.0);
    size_t nget_m = numOperations * (workload.lookup_miss / 100.0);
    size_t ndel_h = numOperations * (workload.deletion_hit / 100.0);
    size_t ndel_m = numOperations - nput - nget_h - nget_m - ndel_h;

    //Dirty fix to fix and endless loop in data generation below for nops not a round number like 1000000000
//	--numOperations;

#ifdef __x86_64__
    std::mt19937_64 gen(seed);
    std::mt19937_64 gen_op(805223753);
    std::mt19937_64 gen_get_del(52830291);
    MWord max = ~0ULL;
#else
    std::mt19937 gen(seed);
    std::mt19937 gen_op(805223753);
    std::mt19937 gen_get_del(52830291);
    MWord max = ~0ULL;
#endif
    std::uniform_int_distribution<MWord> dis_op(1, 100);
    std::uniform_int_distribution<MWord> dis_get_del(1, max);

    size_t del_hit_boundary = workload.insertion + workload.deletion_hit;
    size_t del_miss_boundary = del_hit_boundary + workload.deletion_miss;
    size_t lookup_hit_boundary = del_miss_boundary + workload.lookup_hit;

    if (dist == SPARSE_UNSORTED) {
        std::uniform_int_distribution<MWord> dis_key(1, max);

        size_t count = 0;

        // used for deduplication
        LinearTombstoneHashTable<MWord, MWord, MurmurHash, 0L, 0xFFFFFFFFFFFFFFFF> keySet(
                std::log2(workload.pre_n) + 1);

        while (count < workload.pre_n) {
            MWord key = dis_key(gen);
            if (!keySet.put(key, 42).found) {
                keys[count].first = key;
                keys[count].second = false;
                ++count;
            }
        }

        MWord r_op;
        size_t put_idx = 0;
        size_t get_idx_h = 0;
        size_t get_idx_m = 0;
        size_t del_idx_h = 0;
        size_t del_idx_m = 0;
        count = 0;
        while (count < numOperations) {
            r_op = dis_op(gen_op);
            // insertions
            if (r_op <= workload.insertion) {
                if (put_idx >= nput)
                    continue;
                ops[count].opcode = INSERTION;
                do {
                    ops[count].opdata.key = dis_key(gen);
                }
                while (keySet.get(ops[count].opdata.key).found);
                keys[put_idx + workload.pre_n].first = ops[count].opdata.key;
                keys[put_idx + workload.pre_n].second = false;
                ++put_idx;
            }
                // deletions
            else if (r_op <= del_hit_boundary) {
                if (del_idx_h >= ndel_h)
                    continue;
                ops[count].opcode = DELETION;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                // mark deleted
                keys[ops[count].opdata.key].second = true;
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++del_idx_h;
            }
            else if (r_op <= del_miss_boundary) {
                if (del_idx_m >= ndel_m)
                    continue;
                ops[count].opcode = DELETION;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++del_idx_m;
            }
            else if (r_op <= lookup_hit_boundary) {
                if (get_idx_h >= nget_h)
                    continue;
                ops[count].opcode = LOOKUP;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++get_idx_h;

            } else /*if (r_op <= lookup_miss_boundary)*/ {
                if (get_idx_m >= nget_m)
                    continue;
                ops[count].opcode = LOOKUP;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++get_idx_m;
            }
            ++count;
        }
        keySet.clear();
    }
    else if (dist == DENSE_SORTED) {
        for (size_t i = 0; i < workload.pre_n + nput; ++i) {
            keys[i].first = i + 1 + 725097805;
            keys[i].second = false;
        }

        MWord r_op;
        size_t put_idx = 0;
        size_t get_idx_h = 0;
        size_t del_idx_h = 0;
        size_t get_idx_m = 0;
        size_t del_idx_m = 0;
        size_t count = 0;
        while (count < numOperations) {
            r_op = dis_op(gen_op);
            // insertions
            if (r_op <= workload.insertion) {
                if (put_idx >= nput)
                    continue;
                ops[count].opcode = INSERTION;
                ops[count].opdata.key = keys[put_idx + workload.pre_n].first;
                ++put_idx;
            }
                // deletions
            else if (r_op <= del_hit_boundary) {
                if (del_idx_h >= ndel_h)
                    continue;
                ops[count].opcode = DELETION;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                // mark deleted
                keys[ops[count].opdata.key].second = true;
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++del_idx_h;
            }
            else if (r_op <= del_miss_boundary) {
                if (del_idx_m >= ndel_m)
                    continue;
                ops[count].opcode = DELETION;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++del_idx_m;
            }
            else if (r_op <= lookup_hit_boundary) {
                if (get_idx_h >= nget_h)
                    continue;
                ops[count].opcode = LOOKUP;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++get_idx_h;
            }
            else /*if (r_op <= lookup_miss_boundary)*/ {
                if (get_idx_m >= nget_m)
                    continue;
                ops[count].opcode = LOOKUP;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++get_idx_m;
            }
            ++count;
        }
    }
    else if (dist == DENSE_UNSORTED) {
        for (size_t i = 0; i < workload.pre_n + nput; ++i) {
            keys[i].first = 725097805 + i;
            keys[i].second = false;
        }

//        std::cout << "DENSE_UNSORTED" << std::endl;
        std::uniform_int_distribution<MWord> dis_key(0, max);
        uint64_t randomLocation = 0;

        // Adding our own random shuffling using the already allocated mersenne twister
        // We do this because we have different seeds for different runs.
        for (MWord i = 0; i < workload.pre_n + nput; ++i) {
            randomLocation = dis_key(gen) % ((workload.pre_n + nput) - i);
            std::swap(keys[i], keys[i + randomLocation]);
        }
//        std::random_shuffle(keys, keys + workload.pre_n + nput);

        MWord r_op;
        size_t put_idx = 0;
        size_t get_idx_h = 0;
        size_t del_idx_h = 0;
        size_t get_idx_m = 0;
        size_t del_idx_m = 0;
        size_t count = 0;
        while (count < numOperations) {
            r_op = dis_op(gen_op);
            // insertions
            if (r_op <= workload.insertion) {
                if (put_idx >= nput)
                    continue;
                ops[count].opcode = INSERTION;
                ops[count].opdata.key = keys[put_idx + workload.pre_n].first;
                ++put_idx;
            }
                // deletions
            else if (r_op <= del_hit_boundary) {
                if (del_idx_h >= ndel_h)
                    continue;
                ops[count].opcode = DELETION;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                // mark deleted
                keys[ops[count].opdata.key].second = true;
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++del_idx_h;
            }
            else if (r_op <= del_miss_boundary) {
                if (del_idx_m >= ndel_m)
                    continue;
                ops[count].opcode = DELETION;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++del_idx_m;
            }
            else if (r_op <= lookup_hit_boundary) {
                if (get_idx_h >= nget_h)
                    continue;
                ops[count].opcode = LOOKUP;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++get_idx_h;
            }
            else /*if (r_op <= lookup_miss_boundary)*/ {
                if (get_idx_m >= nget_m)
                    continue;
                ops[count].opcode = LOOKUP;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++get_idx_m;
            }
            ++count;
        }
    }
    else {
        //GRID
        size_t cElem = 0x0101010101010101;
        size_t pos = 0;
        uint8_t byte = 0;
        uint8_t *smallDigits = 0;
        uint8_t numDigits = 14;
        uint64_t universeSize = std::pow(numDigits, 8);

        if ((workload.pre_n + nput) > universeSize) {
            std::cout << "Either: increase the universe or decrease the total number of elements (pre + insertions)" <<
            std::endl;
            std::cout << "Current universe size: " << universeSize << std::endl;
            exit(1);
        }

        keys[pos].first = cElem;
        keys[pos].second = false;
        ++pos;

        while (byte < sizeof(MWord)) {
            uint64_t currentNumElems = pos;
            for (uint64_t elem = 0; elem < currentNumElems; ++elem) {
                cElem = keys[elem].first;
                smallDigits = reinterpret_cast<uint8_t *>(&cElem);
                for (uint8_t digit = 2; digit <= numDigits; ++digit) {
                    if (pos >= (workload.pre_n + nput)) {
                        elem = currentNumElems;
                        byte = sizeof(MWord);
                        break;
                    } else {
                        smallDigits[byte] = digit;
                        keys[pos].first = reinterpret_cast<size_t *>(smallDigits)[0];
                        keys[pos].second = false;
                        ++pos;
                    }
                }
            }
            ++byte;
        }

        std::uniform_int_distribution<MWord> dis_key(0, max);
        uint64_t randomLocation = 0;
        // Adding our own random shuffling using the already allocated mersenne twister
        // We do this because we have different seeds for different runs.
        for (MWord i = 0; i < workload.pre_n + nput; ++i) {
            randomLocation = dis_key(gen) % ((workload.pre_n + nput) - i);
            std::swap(keys[i], keys[i + randomLocation]);
        }

        MWord r_op;
        size_t put_idx = 0;
        size_t get_idx_h = 0;
        size_t del_idx_h = 0;
        size_t get_idx_m = 0;
        size_t del_idx_m = 0;
        size_t count = 0;
        while (count < numOperations) {
            r_op = dis_op(gen_op);
            // insertions
            if (r_op <= workload.insertion) {
                if (put_idx >= nput)
                    continue;
                ops[count].opcode = INSERTION;
                ops[count].opdata.key = keys[put_idx + workload.pre_n].first;
                ++put_idx;
            }
                // deletions
            else if (r_op <= del_hit_boundary) {
                if (del_idx_h >= ndel_h)
                    continue;
                ops[count].opcode = DELETION;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                // mark deleted
                keys[ops[count].opdata.key].second = true;
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++del_idx_h;
            }
            else if (r_op <= del_miss_boundary) {
                if (del_idx_m >= ndel_m)
                    continue;
                ops[count].opcode = DELETION;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++del_idx_m;
            }
            else if (r_op <= lookup_hit_boundary) {
                if (get_idx_h >= nget_h)
                    continue;
                ops[count].opcode = LOOKUP;
                // find a key that is not deleted yet
                do {
                    ops[count].opdata.key = dis_get_del(gen_get_del) % (put_idx + workload.pre_n);
                }
                while (keys[ops[count].opdata.key].second);
                ops[count].opdata.key = keys[ops[count].opdata.key].first;
                ++get_idx_h;
            }
            else /*if (r_op <= lookup_miss_boundary)*/ {
                if (get_idx_m >= nget_m)
                    continue;
                ops[count].opcode = LOOKUP;
                ops[count].opdata.key = dis_get_del(gen_get_del);
                ++get_idx_m;
            }
            ++count;
        }
    }
    //after generation, we copy the keys without the helper information
    for (uint64_t i = 0; i < workload.pre_n; ++i) {
        genKeys[i] = keys[i].first;
    }
    delete[] keys;
}

void generateAggregationData(MWord *keys, Distribution dist, size_t n, size_t seed) {
#ifdef __x86_64__
    std::mt19937_64 gen(seed);
    unsigned long long int max = ~0ULL;
#else
    std::mt19937 gen(19508);
    unsigned long int max = ~0ULL;
#endif
    MWord *domain = new MWord[n];
    generateIndexData(domain, dist, n, seed);
    std::uniform_int_distribution<MWord> dis_key(1, max);
    Zipf zipf(n, SKEW, dis_key(gen));
    for (MWord i = 0; i < n; ++i) {
        keys[i] = domain[zipf.next() - 1];
    }
    delete[] domain;
}

template<class T>
void runJoin(MWord *innerKeys, size_t innerSize, MWord *outerKeys, size_t outerSize) {

    size_t startMemoryB = getCurrentRSS();

#ifndef INIT_WITH_SIZE
    T map;
#else
    T map(innerSize);
#endif
//-----------------------------------------------------------------------------------------------------INSERTS

    //building
    double start = getTimeSec();
    //TODO bulkload?
    for (size_t i = 0; i < innerSize; i++) {
        map.putValue(innerKeys[i], i + 345345);
    }
    double buildTime = getTimeSec() - start;

    size_t endMemoryB = getCurrentRSS();

    float currentMemoryMB = ((endMemoryB - startMemoryB) / (float) MEGA_BYTE);

    delete[] innerKeys;

    //probing
    start = getTimeSec();
    size_t count = 0;
    for (size_t i = 0; i < outerSize; i++) {
        if (map.getValue(outerKeys[i]) != 0) {
            ++count;
        }
    }
    double probeTime = getTimeSec() - start;

    delete[] outerKeys;

#ifdef FOR_PLOT
    cout << (innerSize / MILLION) / buildTime << " ";
    cout << (outerSize / MILLION) / probeTime << " ";
    cout << ((innerSize + outerSize) / MILLION) / (buildTime + probeTime) << " ";
    cout << count << " ";
    cout << currentMemoryMB << " ";
#else
    cout << "build throughput: " << (innerSize / MILLION) / buildTime << " M/s," << buildTime << " (s)" << endl;
    cout << "probe throughput: " << (outerSize / MILLION) / probeTime << " M/s," << probeTime << " (s)" << endl;
    cout << "total throughput: " << ((innerSize + outerSize) / MILLION) / (buildTime + probeTime) << " M/s," <<
    (buildTime + probeTime) << " (s)" << endl;
    cout << "matches: " << count << endl;
    cout << "memory (MB): " << currentMemoryMB << endl;
#endif
}

template<class T>
void runCountAggregation(const Config &conf) {
    size_t startMemoryB = getCurrentRSS();

    MWord *keys = conf.data;
    const size_t n = conf.numberOfTuples;
#ifndef INIT_WITH_SIZE
    T map(60000);
#else
    T map(conf.numberOfTuples);
#endif
    //-----------------------------------------------------------------------------------------------------INSERTS
    double start = getTimeSec();

    for (size_t i = 0; i < n; i++) {
//        assert(keys[i] != 0);
        ++map[keys[i]];
    }

    double time = getTimeSec() - start;

#ifdef FOR_PLOT
    cout << (n / MILLION) / (time) << " ";
#else
    cout << "size: " << map.size() << " aggregation: " << n << "," << (n / MILLION) / (time) << " M/s," << time <<
    " (s)" << endl;
#endif

    size_t endMemoryB = getCurrentRSS();
    float currentMemoryMB = 0;

#ifdef COVERING_IDX
    currentMemoryMB = ((endMemoryB - startMemoryB) / (float) MEGA_BYTE);
#else
    currentMemoryMB = ((endMemoryB) / (float) MEGA_BYTE);
#endif

#ifdef FOR_PLOT
    cout << currentMemoryMB << " " << map.size() << " ";
#else
    cout << "memory footprint (MB): " << currentMemoryMB << endl;
    cout << "final size: " << map.size() << std::endl;
#endif

}

template<class STRUCTURE>
void dispatchAndRunBenchmark(char **argv, int argc, Config &conf) {
    MWord seed = 19508;
    char benchmark = conf.benchmark;
    //CASE: INDEX
    if ('i' == benchmark || 'I' == benchmark) {

#ifndef PRIMARY_KEY_ONLY_CUCKOO
        std::cout << "PRIMARY_KEY_ONLY_CUCKOO macro not activated, activated first. In this experiment Cuckoo does not have to handle duplicates." << std::endl;
        throw;
#endif

        if (argc > 5) {
            seed = atol(argv[5]);
            if (argc > 6) {
                conf.workload.lookup_miss = atoi(argv[6]);
                if (argc > 7 && conf.distribution == ZIPF) {
                    SKEW = atof(argv[5]);
                }
            } else
                conf.workload.lookup_miss = 0;
        }
        conf.data = new MWord[conf.numberOfTuples];
        generateIndexData(conf.data, conf.distribution, conf.numberOfTuples, seed);
        runBenchmarkIndex<STRUCTURE>(conf);
        delete[] conf.data;
        //CASE: MIXED
    } else if ('m' == benchmark || 'M' == benchmark) {

#ifndef PRIMARY_KEY_ONLY_CUCKOO
        std::cout << "PRIMARY_KEY_ONLY_CUCKOO macro not activated, activated first. In this experiment Cuckoo does not have to handle duplicates." << std::endl;
        throw;
#endif

        if (argc < 11) {
            cout <<
            "./main M number_of_requests distribution test_name number_of_pre_inserted_tuples insertion_ratio deletion_ratio_hit deletion_ratio_miss lookup_ratio_hit lookup_ratio_miss (random_seed)" <<
            endl;
            exit(1);
        }
        Workload &workload = conf.workload;
        workload.nops = atol(argv[2]);
        workload.pre_n = atol(argv[5]);
        workload.insertion = atol(argv[6]);
        workload.deletion_hit = atol(argv[7]);
        workload.deletion_miss = atol(argv[8]);
        workload.lookup_hit = atol(argv[9]);
        workload.lookup_miss = atol(argv[10]);

        // Setting the seed if available
        if (argc > 11)
            seed = atol(argv[11]);
        if (workload.insertion + workload.deletion_hit + workload.deletion_miss + workload.lookup_hit +
            workload.lookup_miss != 100) {
            cout << "the sum of ratios should be 100" << endl;
            exit(1);
        }

        Operation *ops = new Operation[workload.nops];
        MWord total_tuples = workload.pre_n + (size_t) ((workload.insertion / 100.0) * workload.nops);

        conf.data = new MWord[total_tuples];
        generateMixedData(conf.data, ops, workload, conf.distribution, seed);
        conf.numberOfTuples = workload.nops + workload.pre_n;
        conf.ops = ops;
        runMixedBenchmark<STRUCTURE>(conf);
        delete[] ops;
        delete[] conf.data;
        //CASE: AGGREGATION
    } else if ('a' == benchmark || 'A' == benchmark) {

#ifdef PRIMARY_KEY_ONLY_CUCKOO
        std::cout <<
        "PRIMARY_KEY_ONLY_CUCKOO macro activated, deactivated first. In this experiment Cuckoo has to handle duplicates." <<
        std::endl;
        throw;
#endif

        if (argc > 5)
            seed = atol(argv[5]);
        if (argc > 6)
            SKEW = atof(argv[6]);
        if (argc > 7)
            throw;

        conf.data = new MWord[conf.numberOfTuples];
        generateAggregationData(conf.data, conf.distribution, conf.numberOfTuples, seed);
        runCountAggregation<STRUCTURE>(conf);
        delete[] conf.data;
        //CASE: JOIN
    } else if ('j' == benchmark || 'J' == benchmark) {
#ifndef PRIMARY_KEY_ONLY_CUCKOO
        std::cout << "PRIMARY_KEY_ONLY_CUCKOO macro not activated, activated first. In this experiment Cuckoo does not have to handle duplicates." << std::endl;
        throw;
#endif

        size_t outerSize = (1 << OUTER_RELATION); //2^30 as default for outer.
        if (argc > 5)
            seed = atol(argv[5]);
        if (argc > 6)
            conf.workload.lookup_miss = atoi(argv[6]);
        else
            conf.workload.lookup_miss = 0;
        if (argc > 7) throw;

        conf.data = new MWord[conf.numberOfTuples];
        generateIndexData(conf.data, conf.distribution, conf.numberOfTuples, seed);
        MWord *outerRelation = new MWord[outerSize];

#ifdef __x86_64__
        std::mt19937_64 gen(seed);
        unsigned long long int max = ~0ULL;
#else
        std::mt19937 gen(19508);
        unsigned long int max = ~0ULL;
#endif

        std::uniform_int_distribution<MWord> dis_key(1, max);
        Zipf zipf(conf.numberOfTuples, SKEW, dis_key(gen));

        for (MWord i = 0; i < outerSize; ++i)
            outerRelation[i] = conf.data[zipf.next() - 1];

        // In case of unsuccessful queries
        if (conf.workload.lookup_miss > 0) {
            size_t numFailKeys = outerSize * conf.workload.lookup_miss / 100;
            for (size_t i = 0; i < numFailKeys; ++i) {
                outerRelation[i] = dis_key(gen) * dis_key(gen);
            }
        }
        //We shuffle the keys once more before performing lookups
        std::random_shuffle(outerRelation, outerRelation + outerSize);

        runJoin<STRUCTURE>(conf.data, conf.numberOfTuples, outerRelation, outerSize);
    } else throw;
};

IdxTestCase findTestCaseFromParam(string testStructureString) {
    //prepare the test sets...
    vector<IdxTestCase> allTestIdxStructures;

#ifdef ARRAY
    allTestIdxStructures.push_back(IdxTestCase("Array", &dispatchAndRunBenchmark<ArrayAdapter>));
#endif

#ifdef LINEAR
    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTSoAMult",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTableSIMD<MWord, MWord, MultiplicativeHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTSoAMurmur",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTableSIMD<MWord, MWord, MurmurHash3Finalizer, 0L, 0xFFFFFFFFFFFFFFFF> > >));

    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTMurmur",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTable<MWord, MWord, MurmurHash3Finalizer, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTMult",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTable<MWord, MWord, MultiplicativeHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTTabulation",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTable<MWord, MWord, TabulationHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("LinearTSHTMultiplyAdd",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearTombstoneHashTable<MWord, MWord, MultiplyAddHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
#endif

#ifdef ROBINHT
    allTestIdxStructures.push_back(IdxTestCase("RobinHTMurmur",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearHashTable<MWord, MWord, MurmurHash3Finalizer, 0L> > >));
    allTestIdxStructures.push_back(IdxTestCase("RobinHTMult",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearHashTable<MWord, MWord, MultiplicativeHash, 0L> > >));
    allTestIdxStructures.push_back(IdxTestCase("RobinHTTabulation",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearHashTable<MWord, MWord, TabulationHash, 0L> > >));
    allTestIdxStructures.push_back(IdxTestCase("RobinHTMultiplyAdd",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<LinearHashTable<MWord, MWord, MultiplyAddHash, 0L> > >));
#endif

#ifdef CUCKOO2
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash2Murmur", &dispatchAndRunBenchmark<CuckooNHashAdapter<MurmurHash3Finalizer, 2> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash2Mult", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplicativeHash, 2> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash2Tabulation", &dispatchAndRunBenchmark<CuckooNHashAdapter<TabulationHash, 2> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash2MultiplyAdd", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplyAddHash, 2> >));
#endif

#ifdef CUCKOO3
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash3Murmur", &dispatchAndRunBenchmark<CuckooNHashAdapter<MurmurHash3Finalizer, 3> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash3Mult", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplicativeHash, 3> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash3Tabulation", &dispatchAndRunBenchmark<CuckooNHashAdapter<TabulationHash, 3> >));
    allTestIdxStructures.push_back(IdxTestCase("CuckooNhash3MultiplyAdd", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplyAddHash, 3> >));
#endif

#ifdef CUCKOO4
    allTestIdxStructures.push_back(
            IdxTestCase("CuckooNhash4Murmur", &dispatchAndRunBenchmark<CuckooNHashAdapter<MurmurHash3Finalizer, 4> >));
    allTestIdxStructures.push_back(
            IdxTestCase("CuckooNhash4Mult", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplicativeHash, 4> >));
    allTestIdxStructures.push_back(
            IdxTestCase("CuckooNhash4Tabulation", &dispatchAndRunBenchmark<CuckooNHashAdapter<TabulationHash, 4> >));
    allTestIdxStructures.push_back(
            IdxTestCase("CuckooNhash4MultiplyAdd", &dispatchAndRunBenchmark<CuckooNHashAdapter<MultiplyAddHash, 4> >));
#endif

//#ifdef UNORDERED
////    allTestIdxStructures.push_back(IdxTestCase("UnorderedMapMurmur", &dispatchAndRunBenchmark<UnorderedMapAdapter<MurmurHash3Finalizer>>));
////    allTestIdxStructures.push_back(IdxTestCase("UnorderedMapMult", &dispatchAndRunBenchmark<UnorderedMapAdapter<MultiplicativeHash>>));
////    allTestIdxStructures.push_back(IdxTestCase("UnorderedMapTabulation", &dispatchAndRunBenchmark<UnorderedMapAdapter<TabulationHash>>));
////    allTestIdxStructures.push_back(IdxTestCase("UnorderedMapMultiplyAdd", &dispatchAndRunBenchmark<UnorderedMapAdapter<MultiplyAddHash>>));
//
//    allTestIdxStructures.push_back(IdxTestCase("ChainedMapMurmur",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ChainedHashMap<MurmurHash3Finalizer> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ChainedMapMult",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ChainedHashMap<MultiplicativeHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ChainedMapTabulation",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ChainedHashMap<TabulationHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ChainedMapMultiplyAdd",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ChainedHashMap<MultiplyAddHash> > >));
//
    allTestIdxStructures.push_back(IdxTestCase("InlinedChainedMapMurmur",
                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<InlineChainedHashMap<MurmurHash3Finalizer> > >));
    allTestIdxStructures.push_back(IdxTestCase("InlinedChainedMapMult",
                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<InlineChainedHashMap<MultiplicativeHash> > >));
    allTestIdxStructures.push_back(IdxTestCase("InlinedChainedMapTabulation",
                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<InlineChainedHashMap<TabulationHash> > >));
    allTestIdxStructures.push_back(IdxTestCase("InlinedChainedMapMultiplyAdd",
                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<InlineChainedHashMap<MultiplyAddHash> > >));
//
//    allTestIdxStructures.push_back(IdxTestCase("CInlinedChainedMapMurmur",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<CompactInlinedChainedHashMap<MurmurHash3Finalizer> > >));
//    allTestIdxStructures.push_back(IdxTestCase("CInlinedChainedMapMult",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<CompactInlinedChainedHashMap<MultiplicativeHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("CInlinedChainedMapTabulation",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<CompactInlinedChainedHashMap<TabulationHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("CInlinedChainedMapMultiplyAdd",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<CompactInlinedChainedHashMap<MultiplyAddHash> > >));
//
//    allTestIdxStructures.push_back(IdxTestCase("ArrayMapMurmur",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ArrayHashTable<MurmurHash3Finalizer> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ArrayMapMult",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ArrayHashTable<MultiplicativeHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ArrayMapTabulation",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ArrayHashTable<TabulationHash> > >));
//    allTestIdxStructures.push_back(IdxTestCase("ArrayMapMultiplyAdd",
//                                               &dispatchAndRunBenchmark<UnorderedMapAdapter2<ArrayHashTable<MultiplyAddHash> > >));
//#endif
//
#ifdef QUADRATIC
    allTestIdxStructures.push_back(IdxTestCase("QuadraticHTMurmur",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<QuadraticHashTable<MWord, MWord, MurmurHash3Finalizer, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("QuadraticHTMult",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<QuadraticHashTable<MWord, MWord, MultiplicativeHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("QuadraticHTabulation",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<QuadraticHashTable<MWord, MWord, TabulationHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
    allTestIdxStructures.push_back(IdxTestCase("QuadraticHTMultiplyAdd",
                                               &dispatchAndRunBenchmark<LinearHashTableAdapter<QuadraticHashTable<MWord, MWord, MultiplyAddHash, 0L, 0xFFFFFFFFFFFFFFFF> > >));
#endif

    std::map<string, IdxTestCase *> structureTestCasestMap;
    for (auto iter = allTestIdxStructures.begin(); iter != allTestIdxStructures.end(); ++iter) {
        IdxTestCase *testCase = &(*iter);
        structureTestCasestMap[iter->first] = testCase;
    }



    //get the structure to test
    auto testToRunRes = structureTestCasestMap.find(testStructureString);
    if (testToRunRes == structureTestCasestMap.end()) {
        printf("Available index structures are:\n");
        for (auto iter = allTestIdxStructures.begin(); iter != allTestIdxStructures.end(); ++iter) {
            printf("- %s\n", iter->first.c_str());
        }
        exit(1);
    }
    return *testToRunRes->second;
}

int main(int argc, char **argv) {

#if defined(PROFILE_LOOKUPS) || defined(PROFILE_INSERTIONS)
    __itt_pause();
#endif

//    if (argc == 3) {
//        seed = 12345;
//        conf.workload.lookup_miss = atoi(argv[2]);
//        conf.distribution = SPARSE_UNSORTED;
//        conf.numberOfTuples = atol(argv[1]);
//        conf.data = new MWord[conf.numberOfTuples];
//        generateIndexData(conf.data, conf.distribution, conf.numberOfTuples, seed);
//        runBenchmarkIndex<STRUCTURE>(conf);
//        delete[] conf.data;
//        return 0;
//    }

    if (argc < 5) {
        printf("usage: %s b n d s r\nb: benchmark to run((I)ndex, (M)ixed, (J)oin, (A)ggregate)\nn: number of keys\nd: value distribution\ns: data structure\n(r: random seed)\n(f: ratio of failed lookups (0,..,100))\n",
               argv[0]);
        return 1;
    }
    char benchmark = argv[1][0];

    MWord n = atol(argv[2]);
    Distribution dist = findDistributionFromParam(argv[3]);
    IdxTestCase testCase = findTestCaseFromParam(argv[4]);
    Config conf;
    conf.numberOfTuples = n;
    conf.benchmark = benchmark;
    conf.distribution = dist;

#ifndef FOR_PLOT
    printf("---> %s\n", testCase.first.c_str());
#endif
    testCase.second(argv, argc, conf);
    return 0;
}


