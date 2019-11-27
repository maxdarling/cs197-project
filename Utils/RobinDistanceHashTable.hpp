/* 
 * File:   HashTable.hpp
 * Author: stefan
 *
 * Created on 19. September 2014, 10:56
 */

#ifndef ROBINDISTANCEHASHTABLE_HPP
#define ROBINDISTANCEHASHTABLE_HPP

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <immintrin.h>
#include <vector>
#include "OpenTableConstants.hpp"

#define EXP_TRUE(EXPRESSION) __builtin_expect(EXPRESSION, true)
#define EXP_FALSE(EXPRESSION) __builtin_expect(EXPRESSION, false)

/**
 *
 * TODO:
 * rehash if distance > 255!
 * delete needs to be fixed!
 * 
 */
template<class K, class V, class H, K EMPTY>
class RobinDistanceHashTable {

private:
#ifdef DYNAMIC_GROW
    static constexpr bool DYN_GROW = true;
#else
    static constexpr bool DYN_GROW = false;
#endif

    struct __attribute__ ((__packed__)) Entry {
        uint8_t distance;
        K key;
        V value;
    };

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
    Entry *_table;
    size_t _tableSizeLog2;
    size_t _totalSlots;
    size_t _count;
    size_t _highWatermark;
    size_t _lowWatermark;

    enum LHOperation {
        GET, REMOVE, UPDATE_INC, UPDATE_DEC
    };

    inline void checkGrow() {
        if (EXP_FALSE(_count >= _highWatermark)) {
            rehash(_tableSizeLog2 + 1);
        }
    }

    void init() {
        assert(_tableSizeLog2 >= LH_MIN_SIZE_LOG_2);
        _highWatermark = getTotalNumberOfSlots() * LH_MAX_LOAD_FACTOR;
        _lowWatermark = getTotalNumberOfSlots() * LH_MIN_LOAD_FACTOR;
        if (EMPTY == 0 && V() == 0) {
            memset(reinterpret_cast<void *> (_table), 0, sizeof (Entry) * getTotalNumberOfSlots());
        } else {
            for (int i = 0; i < getTotalNumberOfSlots(); ++i) {
                _table[i] = {EMPTY, V()};
            }
        }
    }

    void rehash(size_t newSizePow2) {
        //TODO realloc + inplace algorithm?
        //TODO vectorization?
        //save old state
        Entry *oldTable = _table;
        const size_t oldSize = getTotalNumberOfSlots();
        //create new table
        _tableSizeLog2 = newSizePow2;
        _totalSlots = (1UL << newSizePow2);
        const size_t newNumSlots = getTotalNumberOfSlots();
        _table = static_cast<Entry *> (malloc(newNumSlots * sizeof (Entry)));
        init();
        if (EXP_TRUE(_count > 0)) {
            //copy old to new
            Entry *pEntry = oldTable;
            while (pEntry < oldTable + oldSize) {
                if (EXP_TRUE(pEntry->key != EMPTY)) {
                    //use shortcut version of put here, because we do not have to handle delete-tombstones or duplicates
                    pEntry->distance = 0;
                    putInternal < true > (*pEntry);
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

    //--------------------Methods for Robin Hood approach--------------------
    template<bool REHASH = false>
    inline Result putInternal(Entry toInsert) {
        if (DYN_GROW && !REHASH)
            checkGrow();
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        uint64_t curIdx = _hasher(toInsert.key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        while (true) {
            Entry *pEntry = tableStart + curIdx;
            //Vacant slot found?
            if (EMPTY == pEntry->key) {
                *pEntry = toInsert;
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
            //------------ROBIN SWAP--------------------
            //is the occupant a candidate for swapping? (smaller distance or on ties we sort by key)
            if (pEntry->distance < toInsert.distance || ((pEntry->distance == toInsert.distance) && (pEntry->key > toInsert.key))) {
                //swap
                Entry tmp = toInsert;
                toInsert = *pEntry;
                *pEntry = tmp;
            }

            curIdx = ++curIdx & (moduloMask);
            ++toInsert.distance;
        }
        return result;
    }
    //----------------END Methods related to Robin Hood approach END----------------

    template<LHOperation OPERATION = GET >
    inline Result getInternal(const K &key, const V &updateDelta = V()) {
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = _totalSlots - 1;
        uint64_t curIdx = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t distanceKeyElement = 0;
        while (true) {
            Entry *pEntry = tableStart + curIdx;
            //found case
            if (pEntry->key == key) {
                result.found = true;
                result.value = pEntry->value;
                if (REMOVE == OPERATION) {
                    if (DYN_GROW && EXP_FALSE(--_count <= _lowWatermark && tableSizeLog2 > LH_MIN_SIZE_LOG_2)) {
                        //just set key empty, ignore the shift because resize will take care already of RH invariant
                        pEntry->key = EMPTY;
                        rehash(tableSizeLog2 - 1);
                    } else {
                        if (!DYN_GROW) {
                            --_count;
                        }
                        while (true) {
                            curIdx = ++curIdx & (moduloMask);
                            Entry *pNext = tableStart + curIdx;
                            //criterion to stop shifting: next element is empty or in perfect position (distance 0)
                            if (EMPTY == pNext->key || 0 == (curIdx - (_hasher(pNext->key) >> (divisionShift)))) {//TODO no modulo needed, right? change this shift if not using multiplicative hashing?
                                pEntry->key = EMPTY;
                                break;
                            }
                            //shift left by 1
                            *pEntry = *pNext;//TODO bulk shifting with memmove instead?
                            pEntry = pNext;
                            --(pEntry->distance);
                        }
                    }
                    break;
                } else if (UPDATE_INC == OPERATION) {
                    pEntry->value += updateDelta;
                } else if (UPDATE_DEC == OPERATION) {
                    pEntry->value -= updateDelta;
                }
                break;
            }
            //not found, abort condition: empty slot
            if (EMPTY == pEntry->key) {
                break;
            }
            //not found, abort condition: robin hood criterion
            //special trick: we check the additional stop condition only on every 8th iteration...
            if (EXP_FALSE(0 == (curIdx & 7))) {
                if (pEntry->distance < distanceKeyElement || ((pEntry->distance == distanceKeyElement) && (pEntry->key > key))) {
                    break;
                }
            }
            curIdx = ++curIdx & (moduloMask);
            ++distanceKeyElement;
        }
        return result;
    }

public:

    RobinDistanceHashTable(const size_t sizePow2 = LH_MIN_SIZE_LOG_2) :
    _table(static_cast<Entry *> (malloc(sizeof (Entry) * (1UL << sizePow2)))), _tableSizeLog2(sizePow2), _totalSlots(1UL << sizePow2), _count(0) {
        //initialize table
        init();
    }

    virtual ~RobinDistanceHashTable() {
        //delete table
        free(_table);
    }

    RobinDistanceHashTable(const RobinDistanceHashTable &other) :
    _table(other._table), _tableSizeLog2(other._tableSizeLog2), _totalSlots(other, _totalSlots) {
        throw; //TODO
    }

    RobinDistanceHashTable &operator = (const RobinDistanceHashTable &right) {
        // Check for self-assignment!
        if (this == &right) // Same object?
            return *this; // Yes, so skip assignment, and just return *this.
        // Deallocate, allocate new space, copy values...
        //        return *this;
        throw; //TODO
    }

    Result put(const K &key, const V &val) {
        return putInternal({0, key, val});
    }

    Result remove(const K &key) {
        return getInternal<REMOVE>(key);
    }

    Result updateValueInc(const K &key, const V &delta) {
        return getInternal<UPDATE_INC>(key, delta);
    }

    Result updateValueDec(const K &key, const V &delta) {
        return getInternal<UPDATE_DEC>(key, delta);
    }

    Result get(const K &key) const {
        auto nonConstThisP = const_cast<RobinDistanceHashTable<K, V, H, EMPTY> *> (this);
        return nonConstThisP->getInternal(key);
    }

    void clear() {
        _count = 0;
        if (DYN_GROW) {
            rehash(LH_MIN_SIZE_LOG_2);
        } else {
            size_t size = _totalSlots;
            Entry *table = _table;
            for (size_t i = 0; i < size; ++i) {
                table[i] = {0, EMPTY, V()};
            }
        }
    }

    inline size_t getCount() const {
        return _count;
    };

    inline size_t getTotalNumberOfSlots() const {
        return _totalSlots;
    }

    inline float getLoadFactor() const {
        return static_cast<float> (getCount()) / static_cast<float> (getTotalNumberOfSlots());
    }

    //TODO
    //    V& operator[](const K& key);

#ifdef LH_DEBUG_PROBE_COUNT

    void printDebugInfo() const {
        std::vector<int64_t> distances(10); //TODO lets hope there is no longer distance ever :-)
        int64_t maxDist = 0;
        int64_t sumDist = 0;
        int64_t maxI = 0;
        for (int64_t i = 0; i < getTotalNumberOfSlots(); ++i) {
            Entry *e = _table + i;
            if (e->key != EMPTY) {
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
        for (int i = 0; i < distances.size(); ++i) {
            if (distances[i] > 0)
                std::cout << "i = " << i << " p = " << (static_cast<double> (distances[i]) / getCount()) << std::endl;
        }
        std::cout << "load factor: " << getLoadFactor() << std::endl;
    }

#endif

};

#endif	/* ROBINDISTANCEHASHTABLE_HPP */