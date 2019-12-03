//
// Created by Stefan Richter on 21.05.15.
//

#ifndef INDEXSTRUCTURE_ARRAYHASHTABLE_HPP
#define INDEXSTRUCTURE_ARRAYHASHTABLE_HPP

#include "Defines.hpp"
#include "ChainedHashMap.hpp"
#include "Types.hpp"
#include <cmath>
#include <cstddef>
#include <iostream>

template<class HASHER>
class ArrayHashTable {

private:

    struct AEntry {
        uint64_t key;
        uint64_t value;
    };

    uint64_t *map;
    HASHER hasher;
    uint64_t arraySize;
    uint64_t arraySizeLog2;
    uint64_t count;


public:
    ArrayHashTable() : map(new uint64_t[1ULL << 10]()), arraySize(1ULL << 10), arraySizeLog2(10), count(0) {
        throw;
    }

    ArrayHashTable(MWord size) : count(0) {
        arraySizeLog2 = std::log2(size);
        arraySize = 1ULL << arraySizeLog2;
        //
        this->map = new uint64_t[arraySize]();
    }

    ~ArrayHashTable() {
        //TODO
        delete[] map;
    }


    MWord getValue(const MWord key) {
        uint64_t x = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];
        if (x) {
            AEntry *b = reinterpret_cast<AEntry *>(x & 0xFFFFFFFFFFFF);
            AEntry *end = b + (x >> 48);
            do {
                if (key == b->key) {
                    return b->value;
                }
                if (0 == b->key) {
                    break;
                }
            } while (++b < end);
        }
        return 0;
    }

    MWord &operator[](const MWord key) {
        throw;
    }

    void putValue(MWord key, MWord value) {
        uint64_t slot = hasher(key) >> (hasher.hashBits() - arraySizeLog2);
        uint64_t x = map[slot];
        uint64_t size = x >> 48;
        x &= 0xFFFFFFFFFFFF;
        AEntry *a = reinterpret_cast<AEntry *>(x);
        AEntry *b = a + size;
        if (nullptr != a) {
            while (a < b) {
                if (key == a->key) {
                    a->value = value;
                    return;
                }
                ++a;
            }
        }
        b = reinterpret_cast<AEntry *>(realloc(reinterpret_cast<AEntry *>(x), (size + 1) * sizeof(AEntry)));
        a = b + size;
        map[slot] = ((size + 1) << 48) | reinterpret_cast<uint64_t >(b);
        a->key = key;
        a->value = value;
        ++count;
    }

//    void putValue(MWord key, MWord value) {
//        uint64_t slot = hasher(key) >> (hasher.hashBits() - arraySizeLog2);
//        uint64_t x = map[slot];
//        uint64_t size = x >> 48;
//        x &= 0xFFFFFFFFFFFF;
//        AEntry *a = reinterpret_cast<AEntry *>(x);
//        AEntry *b = a + size;
//        if (a) {
//            while (a < b) {
//                if (key == a->key) {
//                    a->value = value;
//                    return;
//                }
//                ++a;
//            }
//        }
//        b = new AEntry[size + 1];
//        if (a) {
//            a -= size;
//            std::memcpy(b, a, size);
//            delete[] a;
//        }
//        a = b + size;
//        map[slot] = ((size + 1) << 48) | reinterpret_cast<uint64_t >(b);
//        a->key = key;
//        a->value = value;
//        ++count;
//    }

    bool remove(const MWord key) {
//        Bucket &b = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];

        return false;

    }

    size_t size() {
#ifdef PRINT_STATISTICS
        size_t allLengths = 0;
        size_t occSlots = 0;
        for (size_t i = 0; i < arraySize; ++i) {
            uint64_t p = map[i];
            if (0 != p) {
                ++occSlots;
                allLengths += (p >> 48);
            }
        }
        std::cout << "LF: " << (((float) count) / arraySize) << std::endl;
        std::cout << "avgChainsLen: " << (((float) allLengths) / occSlots) << std::endl;
        //std::cout << "touched cl: " << deref_get << std::endl;
        //std::cout << "gets " << xxx << std::endl;
#endif
        return count;
    }

    double debugGetWastedSpaceMB() {
        return 0;
    }
};

#endif //INDEXSTRUCTURE_ARRAYHASHTABLE_HPP
