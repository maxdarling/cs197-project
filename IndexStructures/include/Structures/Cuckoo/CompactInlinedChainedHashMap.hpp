//
// Created by Stefan Richter on 20.05.15.
//

#ifndef INDEXSTRUCTURE_COMPACTINLINEDCHAINEDHASHMAP_HPP
#define INDEXSTRUCTURE_COMPACTINLINEDCHAINEDHASHMAP_HPP

#include "Defines.hpp"
#include "ChainedHashMap.hpp"
#include <Types.hpp>
#include <cmath>
#include <cstddef>
#include <iostream>


template<class HASHER, class ALLOC = SLABAllocator>
class CompactInlinedChainedHashMap {

private:
    BaseEntry *map;
    HASHER hasher;
    uint64_t arraySize;
    uint64_t arraySizeLog2;
    uint64_t count;
    //
    ALLOC alloc;


public:
    CompactInlinedChainedHashMap() : map(new BaseEntry[1 << 10]()), arraySize(1 << 10), arraySizeLog2(10), count(0),
                            alloc(1 << 10) {
        throw;
    }

    CompactInlinedChainedHashMap(MWord size) : count(0), alloc((size >> 1) + size / 5) {
        uint64_t logSize = std::log2(size);
        uint64_t oaMem = (1ULL << (logSize + 2)) * 2 * sizeof(MWord); //+2 because of rounding and lf < 50
        oaMem += oaMem / 2;
        uint64_t estChainEntries = (size >> 1) + size / 5;
        uint64_t cEntriesMem = estChainEntries * 3 * sizeof(MWord);
        uint64_t directoryMem = oaMem - cEntriesMem;
//        directoryMem += directoryMem / 10;
        uint64_t directoryMaxSize = directoryMem / sizeof(BaseEntry);
        arraySizeLog2 = std::log2(directoryMaxSize);

//        arraySizeLog2 = std::log2(size);
//#ifndef HIGH_LOAD_FACTOR
//        ++arraySizeLog2;
//#endif
        arraySize = 1ULL << arraySizeLog2;
        //
        this->map = new BaseEntry[arraySize]();
    }

    ~CompactInlinedChainedHashMap() {
        delete[] map;
    }


    MWord getValue(const MWord key) {
        BaseEntry b = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];

        //simple case
        if (key == b.key) {
            return b.value;
        }

        //chain case
        if (b.key == CHAIN_KEY) {
            Entry *e = b.getChainPointer();
            while (e != nullptr) {
                if (key == e->key) {
                    return e->value;
                } else {
                    e = e->next;
                }
            }
        }
        //not found
        return 0;
    }

    MWord &operator[](const MWord key) {
        throw;
    }

    void putValue(MWord key, MWord value) {
        BaseEntry &b = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];
        //simple empty
        if (EMPTY_KEY == b.key) {
            b.key = key;
            b.value = value;
            ++count;
            //chain cases
        } else if (CHAIN_KEY == b.key) {
            Entry *e = b.getChainPointer();
            //go through chain to check for duplicate key
            while (e != nullptr) {
                if (key == e->key) {
                    e->value = value;
                    return;
                } else {
                    e = e->next;
                }
            }
            e = alloc.newEntry();
            //setup new
            e->next = b.getChainPointer();
            e->key = key;
            e->value = value;
            b.setChainPointer(e);
            ++count;
            //inline cases
        } else {
            //duplicate
            if (key == b.key) {
                b.value = value;
                //make inlined to chain and add
            } else {
                //transform old
                Entry *e = alloc.newEntry();
                e->key = b.key;
                e->value = b.value;
                b.setChainPointer(e);
                //add the new
                e->next = alloc.newEntry();
                e = e->next;
                e->key = key;
                e->value = value;
                e->next = nullptr;
                ++count;
            }
        }
    }

    bool remove(const MWord key) {
        BaseEntry &b = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];
        //simple case
        if (key == b.key) {
            b.key = EMPTY_KEY;
            --count;
            return true;
            //chain case
        } else if (CHAIN_KEY == b.key) {
            Entry *prev = nullptr;
            Entry *e = b.getChainPointer();
            //
            while (e != nullptr) {
                if (key == e->key) {
                    if (prev == nullptr) {
                        prev = e->next;
                        //transform chain to inline, because chain size is down to 1
                        if (prev->next == nullptr) {
                            b.key = prev->key;
                            b.value = prev->value;
                            alloc.deleteEntry(prev);
                            //remove from chain
                        } else {
                            b.setChainPointer(prev);
                        }
                    } else {
                        prev->next = e->next;
                    }
                    //free entry e
                    alloc.deleteEntry(e);
                    //
                    --count;
                    return true;
                } else {
                    prev = e;
                    e = e->next;
                }
            }
        }
        return false;


    }

    size_t size() {
#ifdef PRINT_STATISTICS
        std::cout << "LF: " << (((float) count) / arraySize) << std::endl;
        std::cout << "WASTED: " << debugGetWastedSpaceMB() << " MB" << std::endl;
        //std::cout << "touched cl: " << deref_get << std::endl;
        //std::cout << "gets " << xxx << std::endl;
#endif
        return count;
    }

    double debugGetWastedSpaceMB() {
        return ((double)alloc.debugFreeSpace())/(1024*1024);
    }
};

#endif //INDEXSTRUCTURE_COMPACTINLINEDCHAINEDHASHMAP_HPP
