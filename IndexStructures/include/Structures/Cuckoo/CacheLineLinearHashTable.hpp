/*
 * File:   CacheLineHashTable.hpp
 * Author: stefan
 *
 * Created on 26. September 2014, 12:12
 */

#ifndef CACHELINEHASHTABLE_HPP
#define    CACHELINEHASHTABLE_HPP

#include <stdlib.h>
#include <cassert>
#import <string.h>

#define CACHE_LINE_SIZE 64
//#define CACHE_LINE_NUM_ENTRIES CACHE_LINE_SIZE / sizeof(Entry)
#define EMPTY_KEY 0UL
#define DELETED_KEY 0xFFFFFFFFFFFFFFFFUL

template<class HASHER>
class CacheLineHashTable {
    typedef int64_t Key_t;
    typedef int64_t Val_t;

private:

    struct Entry {
        Key_t key;
        Val_t value;
    };

    HASHER hasher;
    Entry *table;
    size_t count;
    size_t slots;
    size_t tableSizePower2;

public:

    struct Result {

        Result(bool found = false, const Val_t &value = Val_t()) :
                found(found),
                value(value) {
        }

        bool found;
        Val_t value;
    };

    CacheLineHashTable() : table(0), count(0), tableSizePower2(16), slots(getTotalNumberOfSlots()) {
        size_t tableSize = getTotalNumberOfSlots() * sizeof(Entry);
        //TODO static assert dass entry size den CL size ohne rest teilt!
        if (0 != posix_memalign((void **) &table, CACHE_LINE_SIZE, tableSize)) {//0 indicates success
            throw;
        }
        memset(table, 0, tableSize);
    }

    CacheLineHashTable(size_t sizePower2) : table(0), count(0), tableSizePower2(sizePower2), slots(getTotalNumberOfSlots()) {
        size_t tableSize = getTotalNumberOfSlots() * sizeof(Entry);
        //TODO static assert dass entry size den CL size ohne rest teilt!
        if (0 != posix_memalign((void **) &table, CACHE_LINE_SIZE, tableSize)) {//0 indicates success
            throw;
        }
        memset(table, 0, tableSize);
    }

    virtual ~CacheLineHashTable() {
        free(table);
    }

    enum HMOperation {
        PUT, GET, DELETE
    };

    template<HMOperation OPERATION>
    inline Result operation(const Key_t &key, const Val_t &val = Val_t()) {
        Result result(false, Val_t());
//        Entry *const tableStart = _table;
        const uint64_t tableSizeLog2 = this->tableSizePower2;
        const uint64_t divisionShift = hasher.hashBits() - tableSizeLog2;
        const uint64_t moduloMask = getTotalNumberOfSlots() - 1;
        uint64_t iteration = 0;
        Entry *tableStart = table;
        uint64_t idx = hasher(key) >> (divisionShift);//TODO change this shift if not using multiplicative hashing?;
        uint64_t idxBucketStart = idx & ~(4 - 1);
        if (GET == OPERATION) std::cout << "----------------" << std::endl;
        do {
            for (int i = 0; i < 4; ++i) {
                Entry *curSlot = tableStart + idxBucketStart + ((idx + i) & (4 - 1));
                if(getTotalNumberOfSlots())
                if (PUT == OPERATION) {
                    if (EMPTY_KEY == curSlot->key || DELETED_KEY == curSlot->key) {
                        ++count;
                        curSlot->key = key;
                        curSlot->value = val;
                        return result;
                    }
                    if (key == curSlot->key) {
                        result.found = true;
                        result.value = curSlot->value;
                        curSlot->value = val;
                        return result;
                    }
                } else if (GET == OPERATION) {

                    std::cout <<"idx:" << idx<< " slot: " << (curSlot - tableStart) << std::endl;
                    if (key == curSlot->key) {
                        result.found = true;
                        result.value = curSlot->value;
                        return result;
                    }
                    if (EMPTY_KEY == curSlot->key) {
                        return result;
                    }
                }
            }
            idxBucketStart = (idxBucketStart + 4) & moduloMask;
            //todo max number of iterations?
        } while (true);
        return result;
    }

    Result put(const Key_t &key, const Val_t &val) {
        return operation<PUT>(key, val);
    }

    Result remove(const Key_t &key) {
        throw;
    }

    Result get(const Key_t &key) {
        return operation<GET>(key);
    }

    inline size_t getTotalNumberOfSlots() const {
        return 1ULL << tableSizePower2;
    }

//    inline int64_t getHashedTableIndex(const Key_t & key) const {
//        return hasher(key) >> (hasher.hashBits() - _tableSizePower2);
//    }

    inline size_t getCount() const {
        return count;
    };

    inline float getLoadFactor() const {
        return static_cast<float> (getCount()) / static_cast<float> (getTotalNumberOfSlots());
    }

    void printDebugInfo() {
        //NOP
    }

    MWord &operator[](MWord key) {
        throw;
    }

};

#endif	/* CACHELINEHASHTABLE_HPP */