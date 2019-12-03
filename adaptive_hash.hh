#ifndef ADAPTIVE_HASH_TABLE_HH
#define ADAPTIVE_HASH_TABLE_HH

//#include <string>
#include <vector>
#include <variant>
#include <cstdlib>
#include <iostream>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "Utils/GenericHashTable.hpp"
#include "Utils/Hash_Functions.hpp"
#include "Utils/LinearHashTable.hpp"
#include "Utils/QuadraticHashTable.hpp"
#include "Utils/ChainedHashTable.hpp"
//#include "Utils/RobinDistanceHashTable.hpp"
#include <math.h>
#include <any>
//#include "Utils/GenericHashTable.hpp"

//TODO: Think of all variations with resizing, key-value pairs, etc.
class AdaptiveHashTable{
private:
	GenericHashTable* generic_table;
	int distrType; //Dense = -1, Sparse = 0, Grid = 1
	std::pair<std::string, size_t> curr_pair;
	size_t lookup_ratio;
	size_t load_factor;
	size_t table_size;
	size_t table_capacity;
	size_t table_density; // sparse = 0 <= t_d <= 1 = dense
	size_t read_time;
	size_t write_time;
	size_t universal_table_index;
	std::map<std::string, std::vector<std::any> > hash_vec{};

public:
	//AdaptiveHashTable(some data);
	AdaptiveHashTable(size_t initial_size); 
	//get all the appropriate variables
	size_t get_lookup_ratio() {return lookup_ratio; };
	size_t get_load_factor() {return load_factor; };
	//size_t get_table_size() {return hash_vec[curr_pair.first][curr_pair.second]->size(); };
	size_t get_table_capacity() {return table_capacity; };
	size_t get_density() {return table_density; };
	size_t get_read_time() {return read_time; };
	size_t get_write_time() {return write_time; };
        size_t get_rehash_time() {return write_time*table_size;};	
	//update the appropriate variables
	void update_load_factor();
	void update_lookup_ratio();
	void update_density();

	//other important functions
	void make_hash_table();
	void print_out_hash_table() {generic_table->printElems();};

	void insert(uint64_t Key, uint64_t Value) {generic_table->put(Key, Value);};
	void remove(uint64_t Key) {generic_table->remove(Key);};
	uint64_t read(uint64_t Key) {return generic_table->get(Key);};
	
	bool assess_switching();
	bool switch_tables();
};

#endif
