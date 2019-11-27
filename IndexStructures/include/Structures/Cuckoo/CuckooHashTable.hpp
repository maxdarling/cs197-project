#ifndef _CUCKOO_HASH_H_
#define _CUCKOO_HASH_H_

#include <cstdlib>
#include <cstring>
#include <random>
#include <iostream>
#include <tuple>

#include "Types.hpp"
#include "OpenTableConstants.hpp"

template<typename Hasher, uint8_t N>
class Cuckoo_hashtable {
private:

    // we store pairs in each location of the table
    typedef std::pair<MWord, MWord> __attribute__((aligned(64))) tuple;

    // whether a rehash was performed
    bool _rehashed;
    // number of hash functions
    // each table has total size a power of two -- this is the exponent
    uint16_t _power_size;
	uint16_t _numRehashes;
    static constexpr uint8_t MAX_HASH_NUM = 4;
	static constexpr uint8_t MAX_REHASHES = 30;
#ifdef MULTIPLE_MULTHASHING
	static constexpr uint8_t NUM_SEEDS = 3;
#else
	static constexpr uint8_t NUM_SEEDS = 1;
#endif
    static constexpr uint32_t MAX_LOOP = 500;
    static constexpr MWord EMPTY_SLOT = 0ULL;
	MWord _table_size;
    // number of inserted items
    MWord _population;
	// To take load factor into consideration
	MWord _watermark;
    // table seeds
    MWord _seeds[NUM_SEEDS][N][MAX_REHASHES];
    // pointer to the table
    tuple *_table[N];
    // for computing the hash function
    Hasher _hasher;

    bool _occupied(tuple *const *const _table, const uint8_t& tidx, const MWord& index) const {
        return _table[tidx][index].first != EMPTY_SLOT;
    }
	
	MWord _hash(const MWord& key, const uint8_t& table, const uint8_t& entry, const MWord& shiftBits) const{
#ifdef MULTIPLE_MULTHASHING
		return ((_hasher.withSeed(key, _seeds[0][table][entry]) >> shiftBits) * (_hasher.withSeed(key, _seeds[1][table][entry]) >> shiftBits) * (_hasher.withSeed(key, _seeds[2][table][entry]) >> shiftBits)) >> shiftBits;
//		return ((_hasher.withSeed(key, _seeds[table][entry]) >> shiftBits) * (_hasher.withSeed(key, _seeds_extra[table][entry]) >> shiftBits) * (_hasher.withSeed(key, _seeds_extra_extra[table][entry]) >> shiftBits)) & _table_size;
//		return ((_hasher.withSeed(key, _seeds[table][entry]) >> shiftBits) * (_hasher.withSeed(key, _seeds_extra[table][entry]) >> shiftBits)) >> shiftBits;
#else
		return (_hasher.withSeed(key, _seeds[0][table][entry]) >> shiftBits);
#endif
	}

	std::pair<MWord*, bool> _put(tuple to_b_inserted) {
		
//		if (_population >= _watermark) {
//			rehash();
//		}
		
        MWord hk = 0;
		std::tuple<MWord, MWord, bool> free(0, 0, false);
        MWord i, j;
        const uint16_t power_size = this->_power_size;
        tuple *const *const table = this->_table;
        const MWord shiftBits = _hasher.hashBits() - power_size;
		const uint16_t numRehashes = this->_numRehashes;
		
#ifdef PRIMARY_KEY_ONLY_CUCKOO
		// No-duplicate assumption
		for (i = 0; i < N; ++i) {
			hk = _hash(to_b_inserted.first, i, numRehashes, shiftBits);
			if (!_occupied(table, i, hk)) {
				table[i][hk] = to_b_inserted;
				++_population;
				return std::make_pair(&(table[i][hk].second), false);
			}
		}
#else
		// This handles duplicates
		MWord currentTable = 0;
		
        for (i = 0; i < N; ++i) {
			currentTable = (N - 1) - i;
			
			hk = _hash(to_b_inserted.first, currentTable, numRehashes, shiftBits);
			
			if (_occupied(table, currentTable, hk)) {
				//We check if its a duplicate and return completely
				if (table[currentTable][hk].first == to_b_inserted.first){
					// We CAN rely on these indexes later on since there will be no rehash whatsoever, the key is duplicate.
					return std::move(std::make_pair(&(table[currentTable][hk].second), true));

				}
				continue;
			}
			//What comes now is gonna be executed only if the slot is not occupied
			//We know the slot is free, so we can record it
			free = std::make_tuple(currentTable, hk, true);
		}
		
		//If the loop above finishes, then we know that to_b_inserted was not
		//found in the table, so we have to decide whether whether we can
		//insert it directly, or whether we have to start cuckoo cycles

		//Here we can put it directly
		if (std::get<2>(free) == true) {
			i = std::get<0>(free);
			hk = std::get<1>(free);
			table[i][hk] = to_b_inserted;
			++_population;
			// We CAN'T in general (case of rehash) rely on these indexes later on
			return std::move(std::make_pair(&(table[i][hk].second), false));
		}
#endif
		
		//Getting to this point means that no empty slot, or a duplicate (in case of duplicate handling), was found above, so we have to initiate Cuckoo cycles
        for (i = 0; i < MAX_LOOP; ++i) {
            for (j = 0; j < N; ++j) {

				hk = _hash(to_b_inserted.first, j, numRehashes, shiftBits);
				
                if (!_occupied(table, j, hk)) {
                    std::swap(to_b_inserted, table[j][hk]);
                    ++_population;
					// We CAN'T rely in general (case of rehash) on these indexes later on
                    return std::move(std::make_pair(&(table[j][hk].second), false));
                }
                else
                    std::swap(to_b_inserted, table[j][hk]);
            }
        }
		
		// If we got here, then the inserted key was no duplicate whatsoever.
        rehash();
        return _put(to_b_inserted);
    }

    void rehash() {

//		std::cout << "rehash yay!" << std::endl;
		//This changes the set of seeds to be used now
		++_numRehashes;
        tuple **table = this->_table;
        _rehashed = false;

        tuple *tmp_table[N];
        const MWord old_table_size = _table_size + 1;
        ++_power_size;
		_table_size = (1ULL << _power_size) - 1;
		_watermark = LH_MAX_LOAD_FACTOR * (_table_size + 1) * N;
        const uint16_t new_power_size = this->_power_size;
        const MWord new_table_size = this->_table_size;
		const uint16_t numRehashes = this->_numRehashes;

        for (uint8_t i = 0; i < N; ++i) {
            // allocating the new space
            posix_memalign((void **) &tmp_table[i], 64, (new_table_size + 1) * sizeof(tuple));
            // marking all empty
            memset(tmp_table[i], EMPTY_SLOT, (new_table_size + 1) * sizeof(tuple));
            // copying previous data
            memcpy(tmp_table[i], table[i], old_table_size * sizeof(tuple));

            std::swap(tmp_table[i], table[i]);

            // freeing up previous data
            free(tmp_table[i]);
            tmp_table[i] = 0;
        }

        uint8_t n = 0;
        MWord current_position = 0;
        tuple tmp;
		bool occupied = 0;
		MWord hk = 0;
		const MWord shiftBits = _hasher.hashBits() - new_power_size;

        for (n = 0; n < N; ++n) {
            current_position = 0;
            while (current_position < old_table_size) {
				
				occupied = _occupied(table, n, current_position);
				hk = _hash(table[n][current_position].first, n, numRehashes, shiftBits);
				
                // we only pay attention if the current element gets rehashed to a different location
                if (occupied && (current_position != hk)) {
                    // we symbolically delete the element from the table
                    tmp = table[n][current_position];
                    --_population;
                    table[n][current_position].first = EMPTY_SLOT;
                    // we try to insert with the new parameters
                    _put(tmp);
                    // we move on if the managed to insert tmp
                    ++current_position;
                    // if inserting tmp triggered another rehash, this boolean will be set to true there, we set it to false
                    // again for future rehash operations and we get out of this rehash since it is now unnecessary
                    if (_rehashed)
                        return;
                } else {
                    ++current_position;
                }
            }
        }
        _rehashed = true;
    }
	
public:
    Cuckoo_hashtable(unsigned short power_size = 10) {
        static_assert(N <= MAX_HASH_NUM, "Unsupported number of hash functions");
		
        _rehashed = false;
        _population = 0;
        _power_size = power_size;
		_numRehashes = 0;
        _table_size = (1ULL << power_size) - 1;
		_watermark = LH_MAX_LOAD_FACTOR * (_table_size + 1) * N;
		uint32_t numSeeds = NUM_SEEDS * 8 * N * MAX_REHASHES;

//		std::cout << "parameters of the cuckoo table: " << std::endl;
//		std::cout << _population << ", " << static_cast<unsigned short>(N) << ", " << _power_size << ", " << _table_size << std::endl;
		
//		std::cout << _power_size << std::endl;
//		std::cout << _watermark << std::endl;
		
		FILE *pfile = fopen("RandomNumbers", "rb");
		assert(pfile);
		assert(fread((void*)_seeds, 1, numSeeds, pfile) == numSeeds);
		fclose(pfile);
		
		// All seeds are initialized here that could possibly be used are iniialized here
		for (uint8_t s = 0; s < NUM_SEEDS; ++s) {
			for (uint8_t i = 0; i < N; ++i){
				for (uint8_t r = 0; r < MAX_REHASHES; ++r) {
#ifdef FIBONACCI_DENSE
					_seeds[s][i][r] = _seeds[s][i][r] * 0.6180339887498948482L;
					_seeds[s][i][r] |= 1ULL;
#else
					_seeds[s][i][r] |= 1ULL;
#endif
				}
			}
		}

        for (uint8_t i = 0; i < N; ++i) {
            posix_memalign((void **) &_table[i], 64, (_table_size + 1) * sizeof(tuple));
            memset(_table[i], EMPTY_SLOT, (_table_size + 1) * sizeof(tuple));
        }
    }
	
	MWord& operator[](const MWord& key){
		std::pair<MWord*, bool> result = std::move(_put(std::move(std::make_pair(key, 0))));
		// If it was a duplicate we return the reference to its value
		if (result.second)
			return *(result.first);
		// Otherwise it was no duplicate but a new element was inserted
		// We don't know where it is in the table, so we have to look for it
		return *(get(key));
	}
	
	void put(MWord &key, MWord &value) {
        _put(std::move(std::make_pair(key, value)));
    }

    MWord* get(const MWord& key) {
        MWord hk = 0;
        const uint16_t _power_size = this->_power_size;
        tuple *const *const _table = this->_table;
		const MWord shiftBits = _hasher.hashBits() - _power_size;
		const uint16_t _numRehashes = this->_numRehashes;
		
        for (uint8_t i = 0; i < N; ++i) {
			hk = _hash(key, i, _numRehashes, shiftBits);
			if (_occupied(_table, i, hk) && _table[i][hk].first == key)
                return &(_table[i][hk].second);
        }
        return nullptr;
    }

    bool remove(const MWord& key) {
        MWord hk = 0;
        const uint16_t _power_size = this->_power_size;
        tuple *const *const _table = this->_table;
		const MWord shiftBits = _hasher.hashBits() - _power_size;
		const uint16_t numRehashes = _numRehashes;
		
        for (uint8_t i = 0; i < N; ++i) {
			hk = _hash(key, i, numRehashes, shiftBits);
            if (_occupied(_table, i, hk) && _table[i][hk].first == key){
                _table[i][hk].first = EMPTY_SLOT;
                --_population;
                return true;
            }
        }
        return false;
    }

    MWord get_size() {
        return _population;
    }
	
    ~Cuckoo_hashtable() {
//		std::cout << _power_size << std::endl;
        for (int i = 0; i < N; ++i)
            free(_table[i]);
    }
};

#endif