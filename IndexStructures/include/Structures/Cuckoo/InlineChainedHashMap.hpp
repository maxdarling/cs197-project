//
// Created by Stefan Richter on 20.05.15.
//

#ifndef INDEXSTRUCTURE_INLINEDCHAINEDHASHMAP_HPP
#define INDEXSTRUCTURE_INLINEDCHAINEDHASHMAP_HPP

#include "Defines.hpp"
#include "ChainedHashMap.hpp"
#include <Types.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>


template<class HASHER, class ALLOC = SLABAllocator>
class InlineChainedHashMap {

private:
    Entry *map;
    HASHER hasher;
    uint64_t arraySize;
    uint64_t arraySizeLog2;
    uint64_t count;
    //
    ALLOC alloc;
    size_t highWatermark;
    size_t lowWatermark;


public:
    InlineChainedHashMap() : map(new Entry[1 << 10]()), arraySize(1 << 10), arraySizeLog2(10), count(0),
                             alloc(1 << 10) {
        throw;
    }

    InlineChainedHashMap(MWord size) : count(0), alloc((size >> 1) + size / 15) {
//        uint64_t logSize = std::log2(size);
//        uint64_t oaMem = (1ULL << (logSize + 2)) * 2 * sizeof(MWord); //+2 because of rounding and lf < 50
//        oaMem += oaMem / 2;
//        uint64_t estChainEntries = (size >> 1) + size / 15;
//        uint64_t cEntriesMem = estChainEntries * 3 * sizeof(MWord);
//        uint64_t directoryMem = oaMem - cEntriesMem;
////        directoryMem += directoryMem / 10;
//        uint64_t directoryMaxSize = directoryMem / sizeof(Entry);
//        arraySizeLog2 = std::log2(directoryMaxSize);

        arraySizeLog2 = std::log2(size);
//#ifndef HIGH_LOAD_FACTOR
//        ++arraySizeLog2;
//#endif
        arraySize = 1ULL << arraySizeLog2;
        //
        this->map = new Entry[arraySize]();
        highWatermark = arraySize * LH_MAX_LOAD_FACTOR * 2;
        lowWatermark = arraySize * LH_MIN_LOAD_FACTOR * 2;
    }

    ~InlineChainedHashMap() {
        delete[] map;
    }

    void rehash(size_t newSizePow2) {
        Entry *oldMap = map;
        uint64_t oldArraySize = arraySize;
        arraySizeLog2 = newSizePow2;
        arraySize = 1ULL << newSizePow2;
        Entry *newMap = new Entry[arraySize]();
        for (size_t i = 0; i < oldArraySize; ++i) {
            Entry *start = oldMap + i;
            Entry *current = oldMap + i;
            //move all entries in the current slot to new map
            if (EMPTY_KEY != current->key) {
                do {
                    Entry *movingEntry = current;
                    current = current->next;
                    const size_t idx = hasher(movingEntry->key) >> (hasher.hashBits() - arraySizeLog2);
                    Entry *const b = newMap + idx;
                    if (EMPTY_KEY == b->key) {
                        //simply put to inlined slot
                        b->key = movingEntry->key;
                        b->value = movingEntry->value;
                    } else {
                        //put to chain
                        Entry *n = alloc.newEntry();
                        n->key = movingEntry->key;
                        n->value = movingEntry->value;
                        n->next = b->next;
                        b->next = n;
                    }
                    if (movingEntry != start) {
                        alloc.deleteEntry(movingEntry);
                    }
                } while (current);
            }
        }
        //free old directory
        delete[] map;
        map = newMap;
        highWatermark = arraySize * LH_MAX_LOAD_FACTOR * 2;
        lowWatermark = arraySize * LH_MIN_LOAD_FACTOR * 2;
    }


    MWord getValue(const MWord key) {
        Entry *e = map + (hasher(key) >> (hasher.hashBits() - arraySizeLog2));
        do {
            if (key == e->key) {
                return e->value;
            } else {
                e = e->next;
            }
        } while (e != nullptr);
        return 0;
    }

    MWord &operator[](const MWord key) {
        throw;
    }

    void putValue(MWord key, MWord value) {
#ifdef DYNAMIC_GROW
        if (count == highWatermark) {
            rehash(arraySizeLog2 + 1);
        }
#endif
        Entry *const b = map + (hasher(key) >> (hasher.hashBits() - arraySizeLog2));
        //simple empty
        if (EMPTY_KEY == b->key) {
            b->key = key;
            b->value = value;
        } else {
            Entry *e = b;
            //chain cases
            //go through chain to check for duplicate key
            do {
                if (key == e->key) {
                    e->value = value;
                    return;
                } else {
                    e = e->next;
                }
            } while (e != nullptr);
            //setup new
            e = alloc.newEntry();
            e->key = key;
            e->value = value;
            e->next = b->next;
            b->next = e;
        }
        ++count;
    }

    bool remove(const MWord key) {
#ifdef DYNAMIC_GROW
        if (count == lowWatermark) {
            rehash(arraySizeLog2 - 1);
        }
#endif
        Entry *prev = nullptr;
        Entry *e = map + (hasher(key) >> (hasher.hashBits() - arraySizeLog2));

        do {
            if (key == e->key) {
                if (prev == nullptr) {
                    if (e->next) {
                        Entry *next = e->next;
                        e->key = next->key;
                        e->value = next->value;
                        e->next = next->next;
                    } else {
                        e->key = EMPTY_KEY;
                    }
                } else {
                    prev->next = e->next;
                    alloc.deleteEntry(e);
                }
                --count;
                return true;
            } else {
                prev = e;
                e = e->next;
            }
        } while (e != nullptr);
        return false;
    }

    MWord getBulkSum(const MWord *keys, const size_t startIdx, const size_t endIdx) {
        MWord result = 0;
        const uint64_t divisionShift = (hasher.hashBits() - arraySizeLog2);
        const uint8_t cacheSize = 16;
        std::pair<uint64_t, uint64_t> idxs[cacheSize];
        const uint64_t cacheMask = cacheSize - 1;
        //TODO ensure len >= cacheSize
        //init cache
        for (size_t i = startIdx; i < startIdx + cacheSize; ++i) {
            const MWord key = keys[i];
            const uint64_t idx = hasher(key) >> (divisionShift);
            __builtin_prefetch(map + idx, 1, 1);
            idxs[i & cacheMask] = {key, idx};
        }

        // probe [0 .. n - prefetchOffset)
        size_t prefetchIndex = startIdx + cacheSize;  // adopted from the ETH NPO join algorithm
        const uint64_t endIndexPrefetch = endIdx - cacheSize;
        for (uint64_t i = startIdx; i < endIndexPrefetch; i++) {
            // re-use hash values and position, which have been already computed during prefetching
            const uint64_t cacheIndex = i & cacheMask;
            const MWord key = idxs[cacheIndex].first;
            uint64_t curIdx = idxs[cacheIndex].second;
            Entry *e = map + curIdx;
            do {
                if (key == e->key) {
                    result += e->value;
                    break;
                } else {
                    e = e->next;
                }
            } while (e != nullptr);

            // prefetching
            const MWord pKey = keys[prefetchIndex];
            const uint64_t pIdx = hasher(pKey) >> (divisionShift);
            __builtin_prefetch(map + pIdx, 1, 1);
            idxs[cacheIndex] = {pKey, pIdx};
            prefetchIndex++;
        }
        for (uint64_t i = endIndexPrefetch; i < endIdx; i++) {
            const uint64_t cacheIndex = i & cacheMask;
            const MWord key = idxs[cacheIndex].first;
            uint64_t curIdx = idxs[cacheIndex].second;
            Entry *e = map + curIdx;
            do {
                if (key == e->key) {
                    result += e->value;
                    break;
                } else {
                    e = e->next;
                }
            } while (e != nullptr);
        }
        return result;
    }

    size_t size() {
#ifdef PRINT_STATISTICS
        std::cout << "LF: " << (((float) count) / arraySize) << std::endl;
        std::cout << "WASTED: " << ((double) alloc.debugFreeSpace()) / (1024 * 1024) << " MB" << std::endl;
        //std::cout << "touched cl: " << deref_get << std::endl;
        //std::cout << "gets " << xxx << std::endl;
#endif
        return count;
    }

    double debugGetWastedSpaceMB() {
        return ((double) alloc.debugFreeSpace()) / (1024 * 1024);
    }
};

#endif //INDEXSTRUCTURE_INLINEDCHAINEDHASHMAP_HPP
