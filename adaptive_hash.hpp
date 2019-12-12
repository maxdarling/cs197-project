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
    string type = "LP";
	pair<uint64_t, uint64_t> lookup_ratio; //first = numerator; second = denominator
	size_t load_factor;
	size_t table_size;
	size_t table_capacity;
	size_t table_density; // sparse = 0 <= t_d <= 1 = dense
	size_t reads;
	size_t writes;
	size_t universal_table_index;
    float MAX_LF = 0.5;

public:
    AdaptiveHashTable(size_t initial_size, std::string type = "LP"):
    lookup_ratio(0,0),
    load_factor(0),
    table_size(0),
    table_capacity(initial_size),
    table_density(0),
    reads(0),
    writes(0){
        make_hash_table();
    }
	//get all the appropriate variables
	double get_lookup_ratio() {return lookup_ratio.second == 0 ? 0 : lookup_ratio.first/lookup_ratio.second; };
	size_t get_load_factor() {return load_factor; };
	size_t get_table_capacity() {return table_capacity; };
	size_t get_density() {return table_density; };	
	//update the appropriate variables
	void update_load_factor();
	void update_lookup_ratio();
	void update_density();

	//other important functions
	void make_hash_table();
	void printElems() {generic_table->printElems();};

	void insert(uint64_t key, uint64_t value);
	void remove(uint64_t key) {generic_table->remove(key);};
	uint64_t lookup(uint64_t key) {
        reads++;
        pair<bool, uint64_t> get_val = generic_table->get(key); 
        update_lookup_ratio(!get_val.first); 
        return generic_table->get(key);
    };
	bool assess_switching();
	bool switch_tables();
};


//if table is full, decides how to rehash
void AdaptiveHashTable::insert(uint64_t key, uint64_t value) {
    writes++;
    //true if table full
    if (generic_table->put2(key, value)) {
        std::cout<<"adaptive table resizing..."<<std::endl;
        //placeholder
        std::cout<<"after assessment, table should NOT adapt"<<std::endl;
        generic_table->put(key, value);
    }
}

void AdaptiveHashTable::update_lookup_ratio(bool success) {
    lookup_ratio.second++;
    if (success) lookup_ratio.first++;
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
        generic_table = new ChainedHashMap<MultiplicativeHash>(table_capacity);
    } else if (curr_pair.first == "LP") {
        generic_table = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>(std::log2(table_capacity));
    } else if (curr_pair.first == "QP") {
        generic_table = new QuadraticHashTable<int, int, MultiplicativeHash, 0L, false>(); //TODO: find last template arg.
    } else {
        throw "error - invalid type argument for adaptive table";
    }
}

pair<double, pair<double, bool>> AdaptiveHashTable::assess_switching() {
    pair<double, pair<bool, bool>> ret_pair;
    //cost function
    //if (load_factor < MAX_LF) return false;
    ret_pair.first = load_factor;
    //lookup ratio dimension
    double ratio = lookup_ratio.second == 0 ? 0 : lookup_ratio.first/lookup_ratio.second;
    bool ratio_bad = ratio < .5;
    ret_pair.second.first = ratio_bad;
    //density dimension
    size_t original_density = table_density;
    update_density();
    bool density_changed = abs(table_density-original_density) >= 0.5;
    ret_pair.second.second = density_changed;
    //if the data distribution is too dense OR lookup ratio is too low
    return ret_pair;
}

bool AdaptiveHashTable::switch_tables() {
    pair<double, pair<double, bool>> ret_pair = assess_switching();
    if (ret_pair.first > .5 {
        if (ret_pair.second.first > .5) generic_table.transferHash(/*CH*/);
        else generic_table.transferHash(/*LP*/);
    } else {
        if (writes > reads) {
            generic_table.transferHash(/*LP*/);
        } else {
            if (ret_pair.second.second) {
                generic_table.transferHash(/*LP*/);
            } else {
                if (ret_pair.second.first) generic_table.transferHash(/*LP*/);
                else generic_table.transferHash(/*CH*/);
            }
        }
    } 
    return true;
}


#endif
