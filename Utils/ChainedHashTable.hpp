//
// Created by Stefan Richter on 20.05.15.
//

#ifndef INDEXSTRUCTURE_CHAINEDHASHMAP_HPP
#define INDEXSTRUCTURE_CHAINEDHASHMAP_HPP

#include "Defines.hpp"
#include "OpenTableConstants.hpp"
#include "Types.hpp"
#include "Hash_Functions.hpp"
#include <cmath>
#include <cstddef>
#include <iostream>

#define CHAIN_KEY 0xFFFFFFFFFFFFFFFFLL
#define EMPTY_KEY 0x0LL

class Entry;

class BaseEntry {
public:
    //TODO: nicer to make extra constructor for empty key...
    BaseEntry() : key(EMPTY_KEY), value(0) {
    }

    MWord key;
    MWord value;


    MWord getKey() const {
        return key;
    }

    MWord getValue() const {
        return value;
    }

    Entry *getChainPointer() {
        return reinterpret_cast<Entry * >(value);
    }

    void setChainPointer(Entry *e) {
        BaseEntry::key = CHAIN_KEY;
        BaseEntry::value = reinterpret_cast<MWord>(e);
    }

    void setKey(MWord key) {
        BaseEntry::key = key;
    }

    void setValue(MWord value) {
        BaseEntry::value = value;
    }
};

class Entry : public BaseEntry {
public:
    Entry() : next(nullptr) {
    }

    Entry *next;


    Entry *getNext() const {
        return next;
    }


    void setNext(Entry *next) {
        Entry::next = next;
    }
};

#ifdef DYNAMIC_GROW
static constexpr size_t slabSize = (2 * 1024 * 1023) / sizeof(Entry);

class Slab {
public:
    Entry entries[slabSize];
    Slab *next;

    Slab() : next(nullptr) {
        init();
    };

    void init() {
        Entry *const e = entries;
        for (size_t i = 1; i < slabSize; ++i) {
            e[i - 1].next = (e + i);
        }
        e[slabSize - 1].next = nullptr;
    }
};

class SLABAllocator {
private:
    Slab *firstSlab;
    Slab *lastSlab;
    Entry *freeList;

    static Slab *createNewSlab() {
        Slab *newSlab = static_cast<Slab *>(malloc(sizeof(Slab)));
        newSlab->init();
        newSlab->next = nullptr;
        return newSlab;
    }

public:
    SLABAllocator(size_t preallocatedSize) : firstSlab(createNewSlab()), lastSlab(firstSlab),
                                             freeList(firstSlab->entries) {
    }

    ~SLABAllocator() {
        Slab *current = firstSlab;
        while (current) {
            Slab *del = current;
            current = current->next;
            free(del);
        }
    }

    Entry *newEntry() {
        if (nullptr == freeList) {
            grow();
        }
        Entry *e = freeList;
        freeList = e->next;
        e->next = nullptr;
        return e;
    }

    void deleteEntry(Entry *e) {
        e->next = freeList;
        freeList = e;
    }

    size_t debugFreeSpace() {
        Entry *fl = freeList;
        size_t wasted = 0;
        while (fl) {
            ++wasted;
            fl = fl->next;
        }
        return wasted * sizeof(Entry);
    }

private:
    void grow() {
        Slab *newSlab = createNewSlab();
        lastSlab->next = newSlab;
        lastSlab = newSlab;
        freeList = newSlab->entries;
    }
};

#else
class SLABAllocator {
private:
    Entry *freeListBase;
    Entry *freeList;
    size_t size;

public:
    SLABAllocator(size_t preallocatedSize) : freeListBase(
            new(malloc(preallocatedSize * sizeof(Entry))) Entry[preallocatedSize]), freeList(freeListBase),
                                             size(preallocatedSize) {
        if (nullptr == freeListBase) throw;
        //init freeList
        MWord initSize = preallocatedSize - 1;
        for (size_t i = 0; i < initSize; ++i) {
            freeList[i].next = (freeList + i + 1);
        }
    }

    ~SLABAllocator() {
        free(freeListBase);
    }

    Entry *newEntry() {
        if (nullptr == freeList) {
            grow();
        }
        Entry *e = freeList;
        freeList = e->next;
        e->next = nullptr;
        return e;
    }

    void deleteEntry(Entry *e) {
        e->next = freeList;
        freeList = e;
    }

    size_t debugFreeSpace() {
        return freeList == nullptr ? 0 : (size - (freeList - freeListBase)) * sizeof(Entry);
    }

private:
    void grow() {
        throw;
        //TODO of course this will not work as the underlying array might get relocated and all pointer hold by the map  become invalid.
        //TODO i could implement an allocator that just allows growth but no incremental shrinking
    }
};
#endif

class MAllocator {
public:
    MAllocator(size_t preallocatedSize) {
        //NOP
    }

    Entry *newEntry() {
        return new Entry();
    }

    void deleteEntry(Entry *e) {
        delete e;
    }

    size_t debugFreeSpace() {
        return 0;
    }

};

template<class HASHER, class ALLOC = SLABAllocator>
class ChainedHashMap {

    //Changed.
public:
    Entry **map;
private:
    //Entry **map;
    HASHER hasher;
    uint64_t arraySize;
    uint64_t arraySizeLog2;
    uint64_t count;
    //
    ALLOC alloc;
    size_t highWatermark;
    size_t lowWatermark;


public:
    ChainedHashMap() : map(new Entry *[1 << 10]), arraySize(1 << 10), arraySizeLog2(10), count(0), alloc(1 << 10) {
        throw;
    }

    ChainedHashMap(MWord size) : count(0), alloc(size) {
//        uint64_t logSize = std::log2(size);
//        uint64_t oaMem = (1ULL << (logSize + 2)) * 2 * sizeof(MWord); //+2 because of rounding and lf < 50
//        oaMem += oaMem / 2;
//        uint64_t cEntriesMem = 3 * size * sizeof(MWord);
//        uint64_t directoryMem = oaMem - cEntriesMem;
////        directoryMem += directoryMem / 10;
//        uint64_t directoryMaxSize = directoryMem / sizeof(Entry *);
//        arraySizeLog2 = std::log2(directoryMaxSize) - 2;

        arraySizeLog2 = std::log2(size);

        /*
        arraySizeLog2 = std::log2(size);
#ifndef HIGH_LOAD_FACTOR
        ++arraySizeLog2;
#endif
         */
        arraySize = 1ULL << arraySizeLog2;
        //
        this->map = new Entry *[arraySize];
        for (size_t i = 0; i < arraySize; ++i) {
            map[i] = nullptr;
        }
        highWatermark = arraySize * LH_MAX_LOAD_FACTOR;
        lowWatermark = arraySize * LH_MIN_LOAD_FACTOR;
        
        //changed
        std::cout<<"arraySize: "<<arraySize<<" highWatermark: "<<highWatermark<<std::endl;
    }

    ~ChainedHashMap() {
        delete[] map;
    }

    void rehash(size_t newSizePow2) {
        Entry **oldMap = map;
        uint64_t oldArraySize = arraySize;
        arraySizeLog2 = newSizePow2;
        arraySize = 1ULL << newSizePow2;
        Entry **newMap = new Entry *[arraySize]();
        for (size_t i = 0; i < oldArraySize; ++i) {
            Entry *current = oldMap[i];
            //move all entries in the current slot to new map
            while (current) {
                Entry *movingEntry = current;
                current = current->next;
                const size_t idx = hasher(movingEntry->key) >> (hasher.hashBits() - arraySizeLog2);
                Entry **const b = newMap + idx;
                movingEntry->next = *b;
                *b = movingEntry;
            }
        }
        //free old directory
        delete[] map;
        map = newMap;
        highWatermark = arraySize * LH_MAX_LOAD_FACTOR;
        lowWatermark = arraySize * LH_MIN_LOAD_FACTOR;
    }


    MWord get(MWord key) {
        Entry *e = map[hasher(key) >> (hasher.hashBits() - arraySizeLog2)];
        while (e != nullptr) {
            if (key == e->key) {
                return e->value;
            } else {
                e = e->next;
            }
        }
        return 0;
    }

    MWord &operator[](const MWord key) {
        throw;
    }

    void put(MWord key, MWord value) {
#ifdef DYNAMIC_GROW
        if (count >= highWatermark) {
            std::cout<<"rehash from 2^"<<arraySizeLog2<<"to 2^"<<arraySizeLog2+1<<std::endl;
            rehash(arraySizeLog2 + 1);
        }
#endif
        const size_t idx = hasher(key) >> (hasher.hashBits() - arraySizeLog2);
        Entry **const b = map + idx;
        Entry *e = *b;
        //go through chain to check for duplicate key
        while (e != nullptr) {
            if (key == e->key) {
                e->value = value;
                return;
            } else {
                e = e->next;
            }
        }
        //insert
        ++count;
        //alloc new

        e = alloc.newEntry();
        //setup new
        e->next = *b;
        e->key = key;
        e->value = value;
        *b = e;
    }

    bool remove(MWord key) {
#ifdef DYNAMIC_GROW
        if (count <= lowWatermark) {
            rehash(arraySizeLog2 - 1);
        }
#endif
        const size_t idx = hasher(key) >> (hasher.hashBits() - arraySizeLog2);
        Entry *prev = nullptr;
        Entry *e = map[idx];
        //
        while (e != nullptr) {
            if (key == e->key) {
                --count;
                if (prev == nullptr) {
                    map[idx] = e->next;
                } else {
                    prev->next = e->next;
                }
                //free entry e
                alloc.deleteEntry(e);
                //
                return true;
            } else {
                prev = e;
                e = e->next;
            }
        }
        return false;
    }
    
    //Changed
    uint64_t getArraySize() {
        return arraySize;
    }
    
    //Changed
    void printElems(){
        size_t total = 0;
        for (size_t i=0; i<arraySize; i++){
            Entry* p = map[i];
            if(p) {
                while (p!= nullptr){
                    std::cout<<"#"<<++total<<": "<<p->getKey()<<std::endl;
                    p = p->next;
                }
            }
        }
    }

    size_t size() {
#ifdef PRINT_STATISTICS
        size_t allLengths = 0;
        size_t occSlots = 0;
        for(size_t i = 0; i< arraySize;++i) {
            Entry* p = map[i];
            if(p) {
                ++occSlots;
            }
            while(nullptr != p) {
                ++allLengths;
                p = p->next;
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
        return ((double) alloc.debugFreeSpace()) / (1024 * 1024);
    }
};

#endif //INDEXSTRUCTURE_CHAINEDHASHMAP_HPP
