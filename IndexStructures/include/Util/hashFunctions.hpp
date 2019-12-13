#ifndef _HASH_FUNCTIONS_HPP
#define _HASH_FUNCTIONS_HPP

#include <cstdint>
#include <cmath>
#include <random>
#include <cstdio>
#include <cassert>

#include "Types.hpp"

class MurmurHash3Finalizer {

public:

    size_t hashBits() const {
        return sizeof(MWord) * 8;
    }

    uint64_t operator ()(uint64_t h) const {
        //Murmur 3 Finalizer
        h ^= h >> 33;
        h *= 0xff51afd7ed558ccd;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53;
        h ^= h >> 33;
        return h;

        /**
        //David Stafford’s Mix13
        h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9L;
        h = (h ^ (h >> 27)) * 0x94d049bb133111ebL;
        return h ^ (h >> 31);
        **/
    }


    uint64_t withSeed(uint64_t h, const uint64_t &seed) const {
		h *= seed;
		h ^= h >> 33;
        h *= 0xff51afd7ed558ccd;
        h ^= h >> 33;
        h *= 0xc4ceb9fe1a85ec53;
        h ^= h >> 33;
        return h;
		
		
//		//David Stafford’s Mix13
//		h = (h ^ (h >> 30)) * 0xbf58476d1ce4e5b9L;
//		h = (h ^ (h >> 27)) * 0x94d049bb133111ebL;
//		return h ^ (h >> 31);
		
//        return operator()(seed ^ key);
    }

};

class MurmurHash {
private:
    uint64_t MurmurHash64A(const void *key, size_t len, uint64_t seed) const {
        const uint64_t m = 0xc6a4a7935bd1e995ULL;
        const int r = 47;

        uint64_t h = seed ^ (len * m);

        const uint64_t *data = (const uint64_t *) key;
        const uint64_t *end = data + (len / 8);

        while (data != end) {
            uint64_t k = *data++;

            k *= m;
            k ^= k >> r;
            k *= m;

            h ^= k;
            h *= m;
        }

        const unsigned char *data2 = (const unsigned char *) data;

        switch (len & 7) {
            case 7:
                h ^= uint64_t(data2[6]) << 48;
            case 6:
                h ^= uint64_t(data2[5]) << 40;
            case 5:
                h ^= uint64_t(data2[4]) << 32;
            case 4:
                h ^= uint64_t(data2[3]) << 24;
            case 3:
                h ^= uint64_t(data2[2]) << 16;
            case 2:
                h ^= uint64_t(data2[1]) << 8;
            case 1:
                h ^= uint64_t(data2[0]);
                h *= m;
        };

        h ^= h >> r;
        h *= m;
        h ^= h >> r;

        return h;
    }

public:

    size_t hashBits() const {
        return sizeof(MWord) * 8;
    }

    uint64_t operator ()(const uint64_t &key) const {
        return MurmurHash64A(&key, sizeof(key), 8550536114492331771ULL);
    }

    uint64_t withSeed(const uint64_t &key, const uint64_t &seed) const {
        return MurmurHash64A(&key, sizeof(key), seed);
    }
};

class MultiplicativeHash {
public:
    uint64_t operator ()(const uint64_t &key) const {
        return key * 11400714819323198485ULL; //golden ratio for 64 bits constant
//		return key * 0xca1b9857a43d173ULL;
    }

    size_t hashBits() const {
        return sizeof(MWord) * 8;
    }

    uint64_t withSeed(const uint64_t &key, const uint64_t &seed) const {
        return key * seed;
    }
};

class TabulationHash {
private:
    uint64_t _tables[8][256];
    std::mt19937_64 _generator;
    std::uniform_int_distribution <uint64_t> _disSeed;

public:
    TabulationHash(void) {
//		FILE *pfile = fopen("RandomNumbers2", "rb");
//		assert(pfile);
//		assert(fread((void*)_tables, 1, 16384, pfile) == 16384);
//		fclose(pfile);

        _generator.seed(252097800623);
        uint64_t highest = ~0;
        _disSeed = std::uniform_int_distribution < uint64_t > (1ULL, highest);

        //Initializing the tables for tabulation hash
        for (size_t t = 0; t < 8; ++t)
            for (size_t pos = 0; pos < 256; ++pos)
                _tables[t][pos] = _disSeed(_generator);
    }

    size_t hashBits() const {
        return sizeof(MWord) * 8;
    }

    // To be equivalent to standard operator()
    uint64_t operator ()(const uint64_t &key) const {
        return (_tables[0][key & 0xFF] ^ _tables[1][(key >> 8) & 0xFF] ^ _tables[2][(key >> 16) & 0xFF] ^ _tables[3][(key >> 24) & 0xFF] ^ _tables[4][(key >> 32) & 0xFF] ^ _tables[5][(key >> 40) & 0xFF] ^ _tables[6][(key >> 48) & 0xFF] ^ _tables[7][(key >> 56) & 0xFF]);
    }

    // To be equivalent to standard operator()
    uint64_t withSeed(uint64_t key, const uint64_t &seed) const {
        key = key * seed;
        return (_tables[0][key & 0xFF] ^ _tables[1][(key >> 8) & 0xFF] ^ _tables[2][(key >> 16) & 0xFF] ^ _tables[3][(key >> 24) & 0xFF] ^ _tables[4][(key >> 32) & 0xFF] ^ _tables[5][(key >> 40) & 0xFF] ^ _tables[6][(key >> 48) & 0xFF] ^ _tables[7][(key >> 56) & 0xFF]);
    }
};

class MultiplyAddHash {
private:
//	static constexpr uint64_t __attribute__((aligned(64)))  _seeds[6] = {9738047536068648934ULL, 16102444696661213356ULL, 15607049721585420932ULL, 8880781859566157380ULL, 14332494294243105906ULL, 6446204480539395894ULL};
    uint64_t __attribute__((aligned(64))) _seeds[6];
    std::mt19937_64 _generator;
    std::uniform_int_distribution <uint64_t> _disSeed;
public:
    MultiplyAddHash(void) {
        _generator.seed(252097800623);
        uint64_t highest = ~0;
        _disSeed = std::uniform_int_distribution < uint64_t > (1ULL, highest);

        //Initializing the tables for tabulation hash
        for (size_t pos = 0; pos < 6; ++pos) {
            _seeds[pos] = _disSeed(_generator);
        }

    }

    uint64_t operator ()(uint64_t key) const {
        return (((((key & 0x00000000FFFFFFFF) + _seeds[0]) * ((key & 0xFFFFFFFF00000000) + _seeds[1])) + _seeds[2]) & 0xFFFFFFFF00000000) | (((((key & 0x00000000FFFFFFFF) + _seeds[3]) * ((key & 0xFFFFFFFF00000000) + _seeds[4])) + _seeds[5]) >> 32);
    }

    uint64_t withSeed(uint64_t key, const uint64_t &seed) const {
        key = key * seed;
//		const uint64_t key_high = key & 0xFFFFFFFF00000000;
//		const uint64_t key_low = key & 0x00000000FFFFFFFF;
//		return (((((key_low) + _seeds[0]) * ((key_high) + _seeds[1])) + _seeds[2]) & 0xFFFFFFFF00000000) | (((((key_low) + _seeds[3]) * ((key_high) + _seeds[4])) + _seeds[5]) >> 32);
        return (((((key & 0x00000000FFFFFFFF) + _seeds[0]) * ((key & 0xFFFFFFFF00000000) + _seeds[1])) + _seeds[2]) & 0xFFFFFFFF00000000) | (((((key & 0x00000000FFFFFFFF) + _seeds[3]) * ((key & 0xFFFFFFFF00000000) + _seeds[4])) + _seeds[5]) >> 32);
    }

    size_t hashBits() const {
        return sizeof(MWord) * 8;
    }
};

#endif