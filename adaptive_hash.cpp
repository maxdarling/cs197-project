#include "adaptive_hash.hh"

AdaptiveHashTable::AdaptiveHashTable():
	currHash("LP"),
	currFunc("M-S"),
	lookup_ratio(0),
	load_factor(0),
	table_size(0),
	table_capacity(0),
	table_density(0),
	read_time(0),
	write_time(0){
		make_hash_table(1); //FIX DEFAULT 1 VALUE!
	}

void AdaptiveHashTable::update_load_factor() {
	load_factor = table_capacity/table_size;
}

void AdaptiveHashTable::update_density() {
	//run python script 
}

void AdaptiveHashTable::make_hash_table(size_t index){
	if (currHash == "LP") {
		universal_table_index = 1;
		table1 = new LinearHashTable<int, int, (HashFunc)index, 0L, false> hash_table;
	} else if (currHash == "QP") {
		universal_table_index = 2;
 		table2 = new QuadraticHashTable<int, int, (HashFunc)index, 0L, false> hash_table;
	} else if (currHash == "CH") {
		universal_table_index = 3;
		table3 = new ChainedHashMap<(HashFunc)index> hash_table; 
	} else if (currHash == "RH") {
		universal_table_index = 4;
		//nothing here yet- the hpp file doesn't work
	} else {
		universal_table_index = -1;
	}

}

bool AdaptiveHashTable::assess_switching() {
	//cost function
	if (load_factor < .5) return false;
	int cost_func = get_read_time() + get_write_time() - get_rehash_time();	
	//assess kde py
	if (cost_func > 0) return true;
	return false;
}

bool switch_tables() {
	if (!assess_switching()) return false;
	
}
