#include "adaptive_hash.hh"
#include <cmath>

AdaptiveHashTable::AdaptiveHashTable(size_t initial_size):
	curr_pair("LH", 1),
	lookup_ratio(0),
	load_factor(0),
	table_size(0),
	table_capacity(initial_size),
	table_density(0),
	read_time(0),
	write_time(0){
		make_hash_table();
	}

void AdaptiveHashTable::update_load_factor() {
	load_factor = table_size/table_capacity;
}

void AdaptiveHashTable::update_density() {
	table_density = table_size/table_capacity;
}


void AdaptiveHashTable::make_hash_table(){
	//HashFunc(curr_pair.second)
	if (curr_pair.first == "CH") {
		if (curr_pair.second == 1) generic_table = new ChainedHashMap<MultiplicativeHash>();
		else if (curr_pair.second == 2) generic_table = new ChainedHashMap<MultiplyAddHash>();
		else if (curr_pair.second == 3) generic_table = new ChainedHashMap<MurmurHash>();
		else generic_table = new ChainedHashMap<TabulationHash>();
	} else if (curr_pair.first == "LH") {
		if (curr_pair.second == 1) generic_table = new LinearHashTable<int, int, MultiplicativeHash, 0L, false>();
		else if (curr_pair.second == 2) generic_table = new LinearHashTable<int, int, MultiplyAddHash, 0L, false>();
		else if (curr_pair.second == 3) generic_table = new LinearHashTable<int, int, MurmurHash, 0L, false>();
		else generic_table = new LinearHashTable<int,int, TabulationHash, 0L, false>();
	} else if (curr_pair.first == "QH") {
		//generic_table = new QuadraticHashTable<int, int, MurmurHash, 0L, false>();
	} else {
		universal_table_index = -1;
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
