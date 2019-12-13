/* 
 * File:   HashTable.hpp
 * Author: stefan
 *
 * Created on 19. September 2014, 10:56
 */

#ifndef LINEARTOMBSTONEHASHTABLEV2_HPP
#define LINEARTOMBSTONEHASHTABLEV2_HPP

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>
#include <vector>
#include "OpenTableConstants.hpp"


/**
*
* TODO:
* - robin: just find idx and bulk move?
* - simd
* - iterator?
*
*/
template<class K, class V, class H, K EMPTY, K DELETED>
class LinearTombstoneHashTableSIMD {

private:

#ifdef DYNAMIC_GROW
    static constexpr bool DYN_GROW = true;
#else
    static constexpr bool DYN_GROW = false;
#endif

public:

    struct Result {

        Result(bool found = false, const V &value = V()) :
                found(found),
                value(value) {
        }

        bool found;
        V value;
    };

private:
    H _hasher;
    K *_keys;
    K *_values;
    size_t _tableSizeLog2;
    size_t _totalSlots;
    size_t _count;
    size_t _highWatermark;
    size_t _lowWatermark;

    //debug stuff
//    size_t _cacheLinesTouched;

    enum LHOperation {
        GET, REMOVE
    };

    void checkGrow() {
        if (EXP_FALSE(_count >= _highWatermark)) {
            rehash(_tableSizeLog2 + 1);
        }
    }

    void init() {
        assert(_tableSizeLog2 >= LH_MIN_SIZE_LOG_2);
        _highWatermark = getTotalNumberOfSlots() * LH_MAX_LOAD_FACTOR;
        _lowWatermark = getTotalNumberOfSlots() * LH_MIN_LOAD_FACTOR;
        if (EMPTY == 0 && V() == 0) {
            memset(reinterpret_cast<void *> (_keys), 0, sizeof(K) * getTotalNumberOfSlots());
            memset(reinterpret_cast<void *> (_values), 0, sizeof(V) * getTotalNumberOfSlots());
        } else {
            for (size_t i = 0; i < getTotalNumberOfSlots(); ++i) {
                _keys[i] = EMPTY;
                _values[i] = V();
            }
        }
    }

    //TODO SIMDIFY?
    void rehash(size_t newSizePow2) {
        //save old state
        K *oldKeys = _keys;
        V *oldVals = _values;
        const size_t oldSize = getTotalNumberOfSlots();
        //create new table
        _tableSizeLog2 = newSizePow2;
        _totalSlots = (1ULL << newSizePow2);
        const size_t newNumSlots = getTotalNumberOfSlots();
        _keys = static_cast<K *> (malloc(newNumSlots * sizeof(K)));
        _values = static_cast<V *> (malloc(newNumSlots * sizeof(V)));
        init();
        if (EXP_TRUE(_count > 0)) {
            //copy old to new
            for (size_t i = 0; i < oldSize; ++i) {
                K currentKey = oldKeys[i];
                if (EXP_TRUE(currentKey != EMPTY && currentKey != DELETED)) {
                    //use shortcut version of put here, because we do not have to handle delete-tombstones or duplicates
                    putInternal<true>(currentKey, _values[i]);
                }
            }
        }
        //free old table
        free(static_cast<void *> (oldKeys));
        free(static_cast<void *> (oldVals));
    }

    int64_t getHashedTableIndex(const K &key) const {
        return _hasher(key) >> (_hasher.hashBits() - _tableSizeLog2);
    }

#ifdef SIMD_INSERT

    template<bool REHASH = false>
    Result putInternal(K kti, V vti) {
        if (DYN_GROW && !REHASH) {
            checkGrow();
        }
        Result result(false, V());
        K *const keys = _keys;
        V *const vals = _values;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t lookupKeyHashIdx = _hasher(kti) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
        int mask = (~0U) << (lookupKeyHashIdx - curIdx);
        size_t lastTombstoneIdx = 0xFFFFFFFFFFFFFFFF;
        while (true) {
            //Vacant slot found?
            const __m256i mkeys = _mm256_load_si256(reinterpret_cast<__m256i *>(keys + curIdx));
            __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), mkeys);
            int bitfield = _mm256_movemask_pd((__m256d) mres) & mask;
            if (bitfield) {
                if (REHASH || 0xFFFFFFFFFFFFFFFF == lastTombstoneIdx) {
                    //actually we would like to just add ctz to curIdx, but well...
                    switch (__builtin_ctz(bitfield)) {
                        case 1:
                            curIdx += 1;
                            break;
                        case 2:
                            curIdx += 2;
                            break;
                        case 3:
                            curIdx += 3;
                            break;
                    }
                    keys[curIdx] = kti;
                    vals[curIdx] = vti;
                } else {
                    //we encountered a tombstone and prefer to replace it
                    keys[lastTombstoneIdx] = kti;
                    vals[lastTombstoneIdx] = vti;
                }

                if (!REHASH) {
                    ++_count;
                }
                break;
            }
            if (!REHASH) {
                //Key already contained?
                mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(kti), mkeys);
                bitfield = _mm256_movemask_pd((__m256d) mres);
                if (bitfield) {
                    //actually we would like to just add ctz to curIdx, but well...
                    switch (__builtin_ctz(bitfield)) {
                        case 1:
                            curIdx += 1;
                            break;
                        case 2:
                            curIdx += 2;
                            break;
                        case 3:
                            curIdx += 3;
                            break;
                    }
                    result.found = true;
                    result.value = vals[curIdx];
                    vals[curIdx] = vti;
                    break;
                }

                mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(DELETED), mkeys);
                bitfield = _mm256_movemask_pd(mres) & mask;
                if (bitfield) {
                    //actually we would like to just add ctz to curIdx, but well...
                    switch (__builtin_ctz(bitfield)) {
                        case 1:
                            curIdx += 1;
                            break;
                        case 2:
                            curIdx += 2;
                            break;
                        case 3:
                            curIdx += 3;
                            break;
                    }
                    lastTombstoneIdx = curIdx;
                }
            }

            curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
            mask = (~0);
        }
        return result;
    }

#else

    template<bool REHASH = false>
    Result putInternal(K kti, V vti) {
        if (DYN_GROW && !REHASH) {
            checkGrow();
        }
        Result result(false, V());
        K *const keys = _keys;
        V *const vals = _values;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        uint64_t curIdx = _hasher(kti) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        size_t lastTombstoneIdx = 0xFFFFFFFFFFFFFFFF;
        while (true) {
            //Vacant slot found?
            if (EMPTY == keys[curIdx]) {
                if (REHASH || 0xFFFFFFFFFFFFFFFF == lastTombstoneIdx) {
                    keys[curIdx] = kti;
                    vals[curIdx] = vti;
                } else {
                    //we encountered a tombstone and prefer to replace it
                    keys[lastTombstoneIdx] = kti;
                    vals[lastTombstoneIdx] = vti;
                }

                if (!REHASH) {
                    ++_count;
                }
                break;
            }
            //Key already contained?
            if (!REHASH && (keys[curIdx] == kti)) {
                result.found = true;
                result.value = vals[curIdx];
                vals[curIdx] = vti;
                break;
            }

            if (!REHASH && DELETED == keys[curIdx]) {
                lastTombstoneIdx = curIdx;
            }

            curIdx = (curIdx + 1) & (moduloMask);
        }
        return result;
    }

#endif
//----------------END Methods related to Robin Hood approach END----------------

#ifdef SIMD_LOOKUP

    template<LHOperation OPERATION = GET>
    Result getInternal(const K &key, const V &updateDelta = V()) {
        Result result(false, V());
        K *const keys = _keys;
        V *const vals = _values;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t lookupKeyHashIdx = _hasher(key) >> (divisionShift);
        uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
        int mask = (~0) << (lookupKeyHashIdx - curIdx);
        while (true) {
            __m256i mkeys = _mm256_load_si256(reinterpret_cast<__m256i *>(keys + curIdx));
            __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(key), mkeys);
            unsigned bitfield = _mm256_movemask_pd((__m256d) mres);
            if (bitfield) {
                curIdx += __builtin_ctz(bitfield);
                result.found = true;
                result.value = vals[curIdx];
                break;
            }

            mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), mkeys);
            bitfield = _mm256_movemask_pd((__m256d) mres) & mask;
            if (bitfield) {
                break;
            }
            curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
            mask = (~0);
        }
        return result;
    };

//    template<LHOperation OPERATION = GET>
//    Result getInternal(const K &key, const V &updateDelta = V()) {
//        Result result(false, V());
//        K *const keys = _keys;
//        V *const vals = _values;
//        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
//        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
//        const uint64_t moduloMask = _totalSlots - 1;
//        const uint64_t lookupKeyHashIdx = _hasher(key) >> (divisionShift);
//        uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
//        int mask = (~0) << (lookupKeyHashIdx - curIdx);
//        while (true) {
//            //KEY?
//            __m256i mkeys = _mm256_load_si256(reinterpret_cast<__m256i *>(keys + curIdx));
//            __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(key), mkeys);
//            unsigned bitfield = _mm256_movemask_pd((__m256d)mres);
//            if (bitfield) {
//                unsigned idx = __builtin_ctz(bitfield) + curIdx;
//                result.found = true;
//                result.value = vals[idx];
//                if (REMOVE == OPERATION) {
//                    //optimization: we only need a tombstone if there is actually a cluster we must hold together, i.e. the next slot is not empty.
//                    bool tombstoneNeeded = (EMPTY != keys[(idx + 1) & moduloMask]);
//                    if (tombstoneNeeded) {
//                        //default
//                        keys[idx] = DELETED;
//                    } else {
//                        keys[idx] = EMPTY;
//                    }
//                    vals[idx] = V();
//                    if (DYN_GROW && EXP_FALSE(--_count <= _lowWatermark && tableSizeLog2 > LH_MIN_SIZE_LOG_2)) {
//                        //just set key empty, ignore the shift because resize will take care already of RH invariant
//                        rehash(tableSizeLog2 - 1);
//                    } else if (!DYN_GROW) {
//                        --_count;
//                    }
//                }
//                break;
//            }
//            //EMPTY and stop?
//            mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), mkeys);
//            bitfield = _mm256_movemask_pd((__m256d)mres) & mask;
//            if (bitfield) {
////                if (curIdx != (lookupKeyHashIdx & ~(SIMD_WIDTH - 1)) || __builtin_ctz(bitfield) >= lookupKeyHashIdx - curIdx) {
//                break;
////                }
//            }
//            curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
//            mask = (~0);
//        }
//        return result;
//    };

#else

    template<LHOperation OPERATION = GET>
    Result getInternal(const K &key, const V &updateDelta = V()) {
        Result result(false, V());
        K *const keys = _keys;
        V *const vals = _values;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t lookupKeyHashIdx =
                _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t curIdx = lookupKeyHashIdx; // & (moduloMask);
//        ++_cacheLinesTouched;
        while (true) {
            //1. found case
            K curKey = keys[curIdx];
            if (key == curKey) {
//                ++_cacheLinesTouched;
                result.found = true;
                result.value = vals[curIdx];
                if (REMOVE == OPERATION) {
                    //optimization: we only need a tombstone if there is actually a cluster we must hold together, i.e. the next slot is not empty.
                    bool tombstoneNeeded = (EMPTY != keys[(curIdx + 1) & moduloMask]);
                    if (tombstoneNeeded) {
                        //default
                        keys[curIdx] = DELETED;
                    } else {
                        keys[curIdx] = EMPTY;
                    }
                    vals[curIdx] = V();
                    if (DYN_GROW && EXP_FALSE(--_count <= _lowWatermark && tableSizeLog2 > LH_MIN_SIZE_LOG_2)) {
                        //just set key empty, ignore the shift because resize will take care already of RH invariant
                        rehash(tableSizeLog2 - 1);
                    } else if (!DYN_GROW) {
                        --_count;
                    }
                }
                break;
            }
            //2. not found, abort condition: empty slot
            if (EMPTY == curKey) {
                break;
            }
            curIdx = (curIdx + 1) & (moduloMask);
//            if (0 == curIdx % (64 / sizeof(K))) {
//                ++_cacheLinesTouched;
//            }
        }
        return result;
    }

#endif

public:

    LinearTombstoneHashTableSIMD(const size_t sizePow2 = LH_MIN_SIZE_LOG_2) :
            _keys(static_cast<K *> (malloc(sizeof(K) * (1ULL << sizePow2)))),
            _values(static_cast<V *> (malloc(sizeof(V) * (1ULL << sizePow2)))),
            _tableSizeLog2(sizePow2), _totalSlots(1ULL << sizePow2), _count(0) /*,_cacheLinesTouched(0)*/ {
        static_assert(EMPTY != DELETED, "EMPTY and DELETED can not be equal!");
        //initialize table
        init();
    }

    virtual ~LinearTombstoneHashTableSIMD() {
        //delete table
        free(_keys);
        free(_values);
    }

    LinearTombstoneHashTableSIMD(const LinearTombstoneHashTableSIMD &other) {
        throw; //TODO
    }

    LinearTombstoneHashTableSIMD &operator=(const LinearTombstoneHashTableSIMD &right) {
        // Check for self-assignment!
        if (this == &right) // Same object?
            return *this; // Yes, so skip assignment, and just return *this.
        // Deallocate, allocate new space, copy values...
        //        return *this;
        throw; //TODO
    }

    V &operator[](K key) {
        throw;
//        if (DYN_GROW) {
//            checkGrow();
//        }
//        Entry *const tableStart = _table;
//        const uint64_t moduloMask = _totalSlots - 1;
//        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
//        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
//        uint64_t curIdx = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
//        Entry *lastTombstone = 0;
//        Entry *pEntry;
//        while (true) {
//            pEntry = tableStart + curIdx;
//            //Vacant slot found?
//            if (EMPTY == pEntry->key) {
//                ++_count;
//                if (0 != lastTombstone) {
//                    pEntry = lastTombstone;
//                }
//                pEntry->key = key;
//                break;
//            }
//            //Key already contained?
//            if (pEntry->key == key) {
//                break;
//            }
//
//            if (DELETED == pEntry->key) {
//                lastTombstone = pEntry;
//            }
//
//            curIdx = (curIdx+1) & (moduloMask);
//        }
//        return pEntry->value;
    }

    Result put(const K &key, const V &val) {
        return putInternal(key, val);
    }

    Result remove(const K &key) {
        return getInternal<REMOVE>(key);
    }

    Result get(const K &key) const {
        auto nonConstThisP = const_cast<LinearTombstoneHashTableSIMD<K, V, H, EMPTY, DELETED> *> (this);
        return nonConstThisP->getInternal(key);
    }

    void clear() {
        _count = 0;
        if (DYN_GROW) {
            rehash(LH_MIN_SIZE_LOG_2);
        } else {
            size_t size = _totalSlots;
            for (size_t i = 0; i < size; ++i) {
                _keys[i] = EMPTY;
                _values[i] = V();
            }
        }
    }

    size_t getCount() const {
        return _count;
    };

    size_t getTotalNumberOfSlots() const {
        return _totalSlots;
    }

    float getLoadFactor() const {
        return static_cast<float> (getCount()) / static_cast<float> (getTotalNumberOfSlots());
    }

    inline V rawGet(const K * const tableStart, const K key, uint64_t idx, const uint64_t moduloMask) const {
        while (true) {
            const K *pEntry = tableStart + idx;
            //1. found case
            if (key == *pEntry) {
                return _values[idx];
            }
            //2. not found, abort condition: empty slot
            if (EMPTY == *pEntry) {
                return 0;
            }
            idx = (idx + 1) & (moduloMask);
        }
    }

    MWord getBulkSum(const K *keys, const size_t startIdx, const size_t endIdx) {
        MWord result = 0;
        K *const tableStart = _keys;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint8_t cacheSize = 16;
        std::pair<uint64_t, uint64_t> idxs[cacheSize];
        const uint64_t cacheMask = cacheSize - 1;
        //TODO ensure len >= cacheSize
        //init cache
        for (size_t i = startIdx; i < startIdx + cacheSize; ++i) {
            const K key = keys[i];
            const uint64_t idx = _hasher(key) >> (divisionShift);
            __builtin_prefetch(tableStart + idx, 0, 1);
            idxs[i & cacheMask] = {key, idx};
        }

        // probe [0 .. n - prefetchOffset)
        size_t prefetchIndex = startIdx + cacheSize;  // adopted from the ETH NPO join algorithm
        const uint64_t endIndexPrefetch = endIdx - cacheSize;
        for (uint64_t i = startIdx; i < endIndexPrefetch; i++) {
            // re-use hash values and position, which have been already computed during prefetching
            const uint64_t cacheIndex = i & cacheMask;
            const K key = idxs[cacheIndex].first;
            uint64_t curIdx = idxs[cacheIndex].second;
            result += rawGet(tableStart, key, curIdx, moduloMask);

            // prefetching
            const K pKey = keys[prefetchIndex];
            const uint64_t pIdx = _hasher(pKey) >> (divisionShift);
            __builtin_prefetch(tableStart + pIdx, 0, 1);
            idxs[cacheIndex] = {pKey, pIdx};
            prefetchIndex++;
        }
        for (uint64_t i = endIndexPrefetch; i < endIdx; i++) {
            const uint64_t cacheIndex = i & cacheMask;
            const K key = idxs[cacheIndex].first;
            uint64_t curIdx = idxs[cacheIndex].second;
            result += rawGet(tableStart, key, curIdx, moduloMask);
        }
        return result;
    }


#ifdef LH_DEBUG_PROBE_COUNT

    void printDebugInfo() const {
//        std::vector<int64_t> distances(10); //TODO lets hope there is no longer distance ever :-)
//        int64_t maxDist = 0;
//        int64_t sumDist = 0;
//        int64_t maxI = 0;
//        for (int64_t i = 0; i < getTotalNumberOfSlots(); ++i) {
//            Entry *e = _table + i;
//            if (e->key != EMPTY && e->key != DELETED) {
//                int64_t dist = (i - getHashedTableIndex(e->key));
//                if (dist < 0) {
//                    dist = getTotalNumberOfSlots() - getHashedTableIndex(e->key) + i;
//                }
//                sumDist += dist;
//                if (dist > maxDist) {
//                    maxDist = dist;
//                    maxI = i;
//                }
//                if (dist < 10) {
//                    distances[dist] += 1;
//                }
//            }
//        }
////        int64_t diff;
////        do {
////            Entry *e = table + maxI;
////            diff = maxI - getHashedTableIndex(e->key);
////            if (e->key != EMPTY && e->key != DELETED) {
////                std::cout << "<" << e->key << ", " << e->value << "> : (" << maxI << ", " << getHashedTableIndex(e->key) << ") - " << (diff) << std::endl;
////            } else {
////                break;
////            }
////        } while (--maxI >= 0);
//
//        std::cout << "max dist: " << maxDist << std::endl;
//        std::cout << "avg dist: " << (static_cast<double> (sumDist) / getCount()) << std::endl;
//        for (int i = 0; i < distances.size(); ++i) {
//            if (distances[i] > 0)
//                std::cout << "i = " << i << " p = " << (static_cast<double> (distances[i]) / getCount()) << std::endl;
//        }
        std::cout << "load factor: " << getLoadFactor() << std::endl;
//        std::cout << "cache lines: " << _cacheLinesTouched << std::endl;
    }

#endif

};

#endif	/* LINEARTOMBSTONEHASHTABLEV2_HPP */