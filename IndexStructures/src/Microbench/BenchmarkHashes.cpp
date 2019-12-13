//
//  BenchmarkHashes.cpp
//  IndexStructure
//
//  Created by Stefan Richter on 02.10.14.
//  Copyright (c) 2014 Stefan Richter. All rights reserved.
//

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <set>
#include <random>
#include <map>
#include <algorithm>
#include "Timer.hpp"
#include "hashFunctions.hpp"
#include <immintrin.h>

#define SIMD_WIDTH 4

using namespace std;

template<class H>
void runTest() {
	std::mt19937_64 gen(4019820781021);
	std::uniform_int_distribution<uint64_t> dis_key(1ULL, ~0ULL);
	
    size_t iterations = 200000000;
    uint64_t *data = new uint64_t[iterations];
    for (int i = 0; i < iterations; ++i) {
		data[i] = dis_key(gen);
//        data[i] = i;
    }
	
//	random_shuffle(data, data + 100000000);

    cout << "hasher: " << typeid(H).name() << endl;
    H hasher;
    uint64_t res = 0;
    double start = getTimeSec();
    for (int i = 0; i < iterations; ++i) {
//        res += hasher(i);
        res += hasher(data[i]);
//        res += hasher((i*0xca1b9857a43d173ULL)^i);
    }
    double end = getTimeSec();
    double duration = end - start;
    cout << "time: " << duration << " sec" << endl;
    cout << "throughput: " << (iterations / duration) / 1000000.00 << " MOp/s" << endl;
    cout << res << endl;
	cout << endl;
}

void putInternal(uint64_t kti, uint64_t vti) {
    uint64_t *const keys = _keys;
    uint64_t *const vals = _values;
    const uint64_t moduloMask = _totalSlots - 1;
    const uint64_t tableSizeLog2 = this->_tableSizeLog2;
    const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
    const uint64_t lookupKeyHashIdx = _hasher(kti) >> (divisionShift);
    uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
    int mask = (~0) << (lookupKeyHashIdx - curIdx);
    while (true) {
        const __m256i mkeys = _mm256_load_si256(reinterpret_cast<__m256i *>(keys + curIdx));
        __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), mkeys);
        unsigned bitfield = _mm256_movemask_pd((__m256d)mres) & mask;
        if (bitfield) {
            curIdx += __builtin_ctz(bitfield);
            keys[curIdx] = kti;
            vals[curIdx] = vti;
            break;
        }
        curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
        mask = (~0);
    }
}

uint64_t getInternal(const uint64_t &key) {
    uint64_t *const keys = _keys;
    uint64_t *const vals = _values;
    const uint64_t moduloMask = _totalSlots - 1;
    const uint64_t tableSizeLog2 = this->_tableSizeLog2;
    const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
    const uint64_t lookupKeyHashIdx = _hasher(key) >> (divisionShift);
    uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
    int mask = (~0) << (lookupKeyHashIdx - curIdx);
    while (true) {
        __m256i mkeys = _mm256_load_si256(reinterpret_cast<__m256i *>(keys + curIdx));
        __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(key), mkeys);
        unsigned bitfield = _mm256_movemask_pd((__m256d)mres);
        if (bitfield) {
            unsigned idx = __builtin_ctz(bitfield) + curIdx;
            return vals[idx];
        }

        mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), mkeys);
        bitfield = _mm256_movemask_pd((__m256d)mres) & mask;
        if (bitfield) {
            break;
        }
        curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
        mask = (~0);
    }
    return 0;
};

int main(int argc, char **argv) {
//    runTest<MurmurHash>();
//    runTest<TabulationHash>();
//    runTest<MultiplicativeHash>();
//	runTest<MultiplyAddHash>();
    return 0;
}


