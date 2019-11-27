/* 
 * File:   HashTable.hpp
 * Author: stefan
 *
 * Created on 19. September 2014, 10:56
 */

#ifndef LINEARTOMBSTONEHASHTABLE_HPP
#define LINEARTOMBSTONEHASHTABLE_HPP

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
class LinearTombstoneHashTable {

private:

#ifdef DYNAMIC_GROW
    static constexpr bool DYN_GROW = true;
#else
    static constexpr bool DYN_GROW = false;
#endif

public:

    struct Entry {
        K key;
        V value;
    };

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
    Entry *_table;
    size_t _tableSizeLog2;
    size_t _totalSlots;
    size_t _count;
    size_t _highWatermark;
    size_t _lowWatermark;

#ifdef LH_DEBUG_PROBE_COUNT
    size_t n_cache_lines;
    size_t n_accesses;
#endif

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
            memset(reinterpret_cast<void *> (_table), 0, sizeof(Entry) * getTotalNumberOfSlots());
        } else {
            for (size_t i = 0; i < getTotalNumberOfSlots(); ++i) {
                _table[i] = {EMPTY, V()};
            }
        }

#ifdef LH_DEBUG_PROBE_COUNT
        n_cache_lines = 0;
        n_accesses = 0;
#endif
    }

    void rehash(size_t newSizePow2) {
        //TODO realloc + inplace algorithm?
        //TODO vectorization?
        //save old state
        Entry *oldTable = _table;
        const size_t oldSize = getTotalNumberOfSlots();
        //create new table
        _tableSizeLog2 = newSizePow2;
        _totalSlots = (1ULL << newSizePow2);
        const size_t newNumSlots = getTotalNumberOfSlots();
        _table = static_cast<Entry *> (malloc(newNumSlots * sizeof(Entry)));
        init();
        if (EXP_TRUE(_count > 0)) {
            //copy old to new
            const Entry *pEntry = oldTable;
            while (pEntry < oldTable + oldSize) {
                if (EXP_TRUE(pEntry->key != EMPTY && pEntry->key != DELETED)) {
                    //use shortcut version of put here, because we do not have to handle delete-tombstones or duplicates
                    putInternal<true>(*pEntry);
                }
                ++pEntry;
            }
        }
        //free old table
        free(static_cast<void *> (oldTable));
    }

    int64_t getHashedTableIndex(const K &key) const {
        return _hasher(key) >> (_hasher.hashBits() - _tableSizeLog2);
    }

#ifdef SIMD_INSERT

    template<bool REHASH = false>
    Result putInternal(Entry toInsert) {
        Result result(false, V());
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t lookupKeyHashIdx = _hasher(toInsert.key) >> (_hasher.hashBits() - this->_tableSizeLog2);
        uint64_t curIdx = lookupKeyHashIdx & ~(SIMD_WIDTH - 1); //SIMD alignment
        int mask = (~0) << (lookupKeyHashIdx - curIdx);
        while (true) {
            Entry * pEntry = _table + curIdx;
            __m256i fourKeys = {(long long) pEntry[0].key, (long long) (pEntry[1]).key, (long long) (pEntry[2]).key, (long long) (pEntry[3]).key};
            __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), fourKeys);
            int bitfield = _mm256_movemask_pd((__m256d) mres) & mask;
            if (bitfield) {
                switch(__builtin_ctz(bitfield)) {
                    case 1: curIdx += 1; break;
                    case 2: curIdx += 2; break;
                    case 3: curIdx += 3; break;
                }
                _table[curIdx] = toInsert;
                ++_count;
                break;
            }
            curIdx = (curIdx + SIMD_WIDTH) & (moduloMask);
            mask = (~0);
        }
        return result;
    }


#else

    template<bool REHASH = false>
    Result putInternal(Entry toInsert) {
        if (DYN_GROW && !REHASH) {
            checkGrow();
        }
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        uint64_t curIdx =
                _hasher(toInsert.key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        Entry *lastTombstone = 0;
        while (true) {
            Entry *pEntry = tableStart + curIdx;
            //Vacant slot found?
            if (EMPTY == pEntry->key) {
                if (REHASH || 0 == lastTombstone) {
                    *pEntry = toInsert;
                } else {
                    //we encountered a tombstone and prefer to replace it
                    *lastTombstone = toInsert;
                }

                if (!REHASH) {
                    ++_count;
                }
                break;
            }
            //Key already contained?
            if (!REHASH && (pEntry->key == toInsert.key)) {
                result.found = true;
                result.value = pEntry->value;
                pEntry->value = toInsert.value;
                break;
            }

            if (!REHASH && DELETED == pEntry->key) {
                lastTombstone = pEntry;
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
        Entry *const tableStart = _table;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t lookupKeyHashIdx = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t curIdx = lookupKeyHashIdx & ~(4 - 1); //SIMD alignment
        int mask = (~0) << (lookupKeyHashIdx - curIdx);
        while (true) {
            Entry *pEntry = tableStart + curIdx;
            __m256i fourKeys = {(long long) pEntry[0].key, (long long) (pEntry[1]).key, (long long) (pEntry[2]).key, (long long) (pEntry[3]).key};
//            __m256i offs = {0, 16, 32, 48};
//            __m256i fourKeys = _mm256_i64gather_epi64(reinterpret_cast<long long*>(pEntry), offs, 1);
            __m256i mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(key), fourKeys);
            int bitfield = _mm256_movemask_pd((__m256d) mres);
            //1. found case
            if (bitfield) {
                pEntry += __builtin_ctz(bitfield);
                result.found = true;
                result.value = pEntry->value;
                //TODO IMPLEMENT REMOVE
                break;
            }
            //2. not found, abort condition: empty slot

            mres = _mm256_cmpeq_epi64(_mm256_set1_epi64x(EMPTY), fourKeys);
            bitfield = _mm256_movemask_pd((__m256d) mres) & mask;
            if (bitfield) {
                break;
            }
            curIdx = (curIdx + 4) & (moduloMask);
            mask = ~0;
        }
        return result;
    }
#else

    template<LHOperation OPERATION = GET>
    Result getInternal(const K &key, const V &updateDelta = V()) {
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t lookupKeyHashIdx =
                _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t curIdx = lookupKeyHashIdx;
#ifdef LH_DEBUG_PROBE_COUNT
        uintptr_t lastCL = 0;
#endif
        while (true) {
            Entry *pEntry = tableStart + curIdx;
#ifdef LH_DEBUG_PROBE_COUNT
            uintptr_t curCL = reinterpret_cast<uintptr_t >(pEntry);
            if((curCL & (~63ULL)) != (lastCL & (~63ULL))) {
                lastCL = curCL;
                ++n_cache_lines;
            }
            ++n_accesses;
#endif
            //1. found case
            if (key == pEntry->key) {
                result.found = true;
                result.value = pEntry->value;
                if (REMOVE == OPERATION) {
                    //optimization: we only need a tombstone if there is actually a cluster we must hold together, i.e. the next slot is not empty.
                    bool tombstoneNeeded = (EMPTY != tableStart[(curIdx + 1) & moduloMask].key);
                    if (tombstoneNeeded) {
                        //default
                        pEntry->key = DELETED;
                    } else {
                        pEntry->key = EMPTY;
                    }
                    pEntry->value = V();
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
            if (EMPTY == pEntry->key) {
                break;
            }
            curIdx = (curIdx + 1) & (moduloMask);

        }
        return result;
    }

#endif

public:

    LinearTombstoneHashTable(const size_t sizePow2 = LH_MIN_SIZE_LOG_2) :
            _table(static_cast<Entry *> (malloc(sizeof(Entry) * (1ULL << sizePow2)))),
            _tableSizeLog2(sizePow2),
            _totalSlots(1ULL << sizePow2),
            _count(0) {
        static_assert(EMPTY != DELETED, "EMPTY and DELETED can not be equal!");
        //initialize table
        init();
    }

    virtual ~LinearTombstoneHashTable() {
        //delete table
        free(_table);
    }

    LinearTombstoneHashTable(const LinearTombstoneHashTable &other) :
            _table(other._table), _tableSizeLog2(other._tableSizeLog2), _totalSlots(other._totalSlots) {
        throw; //TODO
    }

    LinearTombstoneHashTable &operator=(const LinearTombstoneHashTable &right) {
        // Check for self-assignment!
        if (this == &right) // Same object?
            return *this; // Yes, so skip assignment, and just return *this.
        // Deallocate, allocate new space, copy values...
        //        return *this;
        throw; //TODO
    }

    V &operator[](K key) {
        if (DYN_GROW) {
            checkGrow();
        }
        Entry *const tableStart = _table;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        uint64_t curIdx = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        Entry *lastTombstone = 0;
        Entry *pEntry;
        while (true) {
            pEntry = tableStart + curIdx;
            //Vacant slot found?
            if (EMPTY == pEntry->key) {
                ++_count;
                if (0 != lastTombstone) {
                    pEntry = lastTombstone;
                }
                pEntry->key = key;
                break;
            }
            //Key already contained?
            if (pEntry->key == key) {
                break;
            }

            if (DELETED == pEntry->key) {
                lastTombstone = pEntry;
            }

            curIdx = (curIdx + 1) & (moduloMask);
        }
        return pEntry->value;
    }

    Result put(const K &key, const V &val) {
        return putInternal({key, val});
    }

    Result remove(const K &key) {
        return getInternal<REMOVE>(key);
    }

    Result get(const K &key) {
        auto nonConstThisP = const_cast<LinearTombstoneHashTable<K, V, H, EMPTY, DELETED> *> (this);
        return nonConstThisP->getInternal(key);
    }

    inline V rawGet(const Entry *const tableStart, const K key, uint64_t idx, const uint64_t moduloMask) const {
        while (true) {
            const Entry *pEntry = tableStart + idx;
            //1. found case
            if (key == pEntry->key) {
                return pEntry->value;
            }
            //2. not found, abort condition: empty slot
            if (EMPTY == pEntry->key) {
                return 0;
            }
            idx = (idx + 1) & (moduloMask);
        }
    }

    MWord getBulkSum(const K *keys, const size_t startIdx, const size_t endIdx) {
        MWord result = 0;
        Entry *const tableStart = _table;
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

    void clear() {
        _count = 0;
        if (DYN_GROW) {
            rehash(LH_MIN_SIZE_LOG_2);
        } else {
            size_t size = _totalSlots;
            Entry *table = _table;
            for (size_t i = 0; i < size; ++i) {
                table[i] = {EMPTY, V()};
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


#ifdef LH_DEBUG_PROBE_COUNT

    void printDebugInfo() const {
        std::vector<int64_t> distances(10); //TODO lets hope there is no longer distance ever :-)
        int64_t maxDist = 0;
        int64_t sumDist = 0;
        int64_t maxI = 0;
        for (int64_t i = 0; i < getTotalNumberOfSlots(); ++i) {
            Entry *e = _table + i;
            if (e->key != EMPTY && e->key != DELETED) {
                int64_t dist = (i - getHashedTableIndex(e->key));
                if (dist < 0) {
                    dist = getTotalNumberOfSlots() - getHashedTableIndex(e->key) + i;
                }
                sumDist += dist;
                if (dist > maxDist) {
                    maxDist = dist;
                    maxI = i;
                }
                if (dist < 10) {
                    distances[dist] += 1;
                }
            }
        }
//        int64_t diff;
//        do {
//            Entry *e = table + maxI;
//            diff = maxI - getHashedTableIndex(e->key);
//            if (e->key != EMPTY && e->key != DELETED) {
//                std::cout << "<" << e->key << ", " << e->value << "> : (" << maxI << ", " << getHashedTableIndex(e->key) << ") - " << (diff) << std::endl;
//            } else {
//                break;
//            }
//        } while (--maxI >= 0);

        std::cout << "max dist: " << maxDist << std::endl;
        std::cout << "avg dist: " << (static_cast<double> (sumDist) / getCount()) << std::endl;
        for (size_t i = 0; i < distances.size(); ++i) {
            if (distances[i] > 0)
                std::cout << "i = " << i << " p = " << (static_cast<double> (distances[i]) / getCount()) << std::endl;
        }
        std::cout << "load factor: " << getLoadFactor() << std::endl;
        std::cout << "memory accesses: " << n_accesses << std::endl;
        std::cout << "accessed cache lines: " << n_cache_lines << std::endl;
    }

#endif

};

#endif	/* LINEARTOMBSTONEHASHTABLE_HPP */