/* 
 * File:   HashTable.hpp
 * Author: stefan
 *
 * Created on 19. September 2014, 10:56
 */

#ifndef QUADRATICHASHTABLE_HPP
#define QUADRATICHASHTABLE_HPP

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
 * - operator[]
 * - iterator?
 * 
 */
template<class K, class V, class H, K EMPTY, K DELETED>
class QuadraticHashTable {

private:
#ifdef DYNAMIC_GROW
    static constexpr bool DYN_GROW = true;
#else
    static constexpr bool DYN_GROW = false;
#endif

    struct Entry {
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
            memset(reinterpret_cast<void *> (_table), 0, sizeof (Entry) * getTotalNumberOfSlots());
        } else {
            for (size_t i = 0; i < getTotalNumberOfSlots(); ++i) {
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
        _totalSlots = (1ULL << newSizePow2);
        const size_t newNumSlots = getTotalNumberOfSlots();
        _table = static_cast<Entry *> (malloc(newNumSlots * sizeof (Entry)));
        init();
        if (EXP_TRUE(_count > 0)) {
            //copy old to new
            const Entry *pEntry = oldTable;
            while (pEntry < oldTable + oldSize) {
                if (EXP_TRUE(pEntry->key != EMPTY && pEntry->key != DELETED)) {
                    //use shortcut version of put here, because we do not have to handle delete-tombstones or duplicates
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
    Result putInternal(Entry toInsert) {
        if (DYN_GROW && !REHASH) {
            checkGrow();
        }
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t moduloMask = getTotalNumberOfSlots() - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t hash = _hasher(toInsert.key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t iteration = 0;
        uint64_t quad = 0;
        while (true) {
            Entry *pEntry = tableStart + ((hash + quad) & moduloMask);
            //Vacant slot found?
            if (EMPTY == pEntry->key || DELETED == pEntry->key) {
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
            quad += ++iteration;
        }
        return result;
    }
    //----------------END Methods related to Robin Hood approach END----------------

    template<LHOperation OPERATION = GET >
    Result getInternal(const K &key) {
        Result result(false, V());
        Entry *const tableStart = _table;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = getTotalNumberOfSlots() - 1;
        const uint64_t hash = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        uint64_t iteration = 0;
        uint64_t quad = 0;
        while (true) {
            Entry *pEntry = tableStart + ((hash + quad) & moduloMask);
            //1. found case
            if (key == pEntry->key) {
                result.found = true;
                result.value = pEntry->value;
                if (REMOVE == OPERATION) {
                    pEntry->key = DELETED;
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
            quad += ++iteration;
        }
        return result;
    }

public:

    QuadraticHashTable(const size_t sizePow2 = LH_MIN_SIZE_LOG_2) :
    _table(static_cast<Entry *> (malloc(sizeof (Entry) * (1ULL << sizePow2)))), _tableSizeLog2(sizePow2), _totalSlots(1ULL << sizePow2), _count(0) {
        static_assert(EMPTY != DELETED, "EMPTY and DELETED can not be equal!");
        //initialize table
        init();
    }

    virtual ~QuadraticHashTable() {
        //delete table
        free(_table);
    }

    QuadraticHashTable(const QuadraticHashTable &other) :
    _table(other._table), _tableSizeLog2(other._tableSizeLog2), _totalSlots(other, _totalSlots) {
        throw; //TODO
    }

    QuadraticHashTable &operator = (const QuadraticHashTable &right) {
        // Check for self-assignment!
        if (this == &right) // Same object?
            return *this; // Yes, so skip assignment, and just return *this.
        // Deallocate, allocate new space, copy values...
        //        return *this;
        throw; //TODO
    }

    Result put(const K &key, const V &val) {
        return putInternal({key, val});
    }

    Result remove(const K &key) {
        return getInternal<REMOVE>(key);
    }


    Result get(const K &key) const {
        auto nonConstThisP = const_cast<QuadraticHashTable<K, V, H, EMPTY, DELETED> *> (this);
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

    V &operator [](K key) {
        if (DYN_GROW) {
            checkGrow();
        }
        Entry *const tableStart = _table;
        const uint64_t moduloMask = _totalSlots - 1;
        const uint64_t tableSizeLog2 = this->_tableSizeLog2;
        const uint64_t divisionShift = _hasher.hashBits() - tableSizeLog2;
        const uint64_t hash = _hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?
        Entry *lastTombstone = 0;
        Entry *pEntry;
        uint64_t iteration = 0;
        uint64_t quad = 0;
        while (true) {
            pEntry = tableStart + ((hash + quad) & moduloMask);
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
            quad += ++iteration;
		}
        return pEntry->value;
    }

#ifdef LH_DEBUG_PROBE_COUNT

    void printDebugInfo() const {

    }

#endif

};

#endif	/* QUADRATICHASHTABLE_HPP */
