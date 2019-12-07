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
#include <cmath>
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
    float MAX_LF = 0.5;

public:
    AdaptiveHashTable(size_t initial_size, std::string type = "LP"):
    curr_pair(type, 1),
    lookup_ratio(0),
    load_factor(0),
    table_size(0),
    table_capacity(initial_size),
    table_density(0),
    read_time(0),
    write_time(0){
        make_hash_table();
    }
	//get all the appropriate variables
	size_t get_lookup_ratio() {return lookup_ratio; };
	size_t get_load_factor() {return load_factor; };
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
	void printElems() {generic_table->printElems();};

	void insert(uint64_t key, uint64_t value);
	void remove(uint64_t key) {generic_table->remove(key);};
	uint64_t lookup(uint64_t key) {return generic_table->get(key);};
	
	bool assess_switching();
	bool switch_tables();
};


//if table is full, decides how to rehash
void AdaptiveHashTable::insert(uint64_t key, uint64_t value) {
    //true if table full
    if (generic_table->put2(key, value)) {
        std::cout<<"adaptive table resizing..."<<std::endl;

        //placeholder
        std::cout<<"after assessment, table should NOT adapt"<<std::endl;
        generic_table->put(key, value);
    }
}

void AdaptiveHashTable::update_load_factor() {
    load_factor = table_size/table_capacity;
}

void AdaptiveHashTable::update_density() {
    table_density = table_size/table_capacity;
}

//below commented where curr_pair.second != 1, since that indicates hash funciton
//but we only use mult for this (since paper says mult best for all).
void AdaptiveHashTable::make_hash_table(){
    if (curr_pair.first == "CH") {
        if (curr_pair.second == 1) generic_table = new ChainedHashMap<MultiplicativeHash>(table_capacity);
//        else if (curr_pair.second == 2) generic_table = new ChainedHashMap<MultiplyAddHash>();
//        else if (curr_pair.second == 3) generic_table = new ChainedHashMap<MurmurHash>();
//        else generic_table = new ChainedHashMap<TabulationHash>();
    } else if (curr_pair.first == "LP") {
        if (curr_pair.second == 1) generic_table = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>(std::log2(table_capacity));
//        else if (curr_pair.second == 2) generic_table = new LinearHashTable<int, int, MultiplyAddHash, 0L, false>();
//        else if (curr_pair.second == 3) generic_table = new LinearHashTable<int, int, MurmurHash, 0L, false>();
//        else generic_table = new LinearHashTable<int,int, TabulationHash, 0L, false>();
    } else if (curr_pair.first == "QP") {
//        generic_table = new QuadraticHashTable<int, int, MultiplicativeHash, 0L, false>(); //todo: find last template arg.
    } else {
        throw "error - invalid type argument for adaptive table";
    }
}

bool AdaptiveHashTable::assess_switching() {
    //cost function + density
    if (load_factor < .5) return false;
    int cost_func = get_read_time() + get_write_time() - get_rehash_time();
    size_t original_density = table_density;
    update_density();
    bool density_changed = original_density;
    //bool density_changed = abs(original_density-table_density) > (.5*original_density);
    if (cost_func > 0 || density_changed) return true;
    return false;
}

bool AdaptiveHashTable::switch_tables() {
    if (!assess_switching()) return false;
    //what should we switch to?
    return true;
}


#endif
