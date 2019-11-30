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
		make_hash_table();
	}

void AdaptiveHashTable::update_load_factor() {
	load_factor = table_capacity/table_size;
}

void AdaptiveHashTable::update_lookup_ratio() {
 	//update lookup ratio
}

void AdaptiveHashTable::update_density() {
	//run python script 
}

void AdaptiveHashTable::update_read_time() {
	//read_time = 
}

void AdaptiveHashTable::update_write_time() {
	//write_time = 
}

void AdaptiveHashTable::make_hash_table(){
	if (currHash == "LP") {

	} else if (currHash == "QP") {

	} else if (currHash == "CH") {

	} else if (currHash == "RH") {
		
	} else {

	}

}

bool AdaptiveHashTable::assess_switching() {
	//cost function
	if (load_factor < .5) return false;
	int cost_func = get_read_time() + get_write_time() - /*REHASH_TIME()*/;	
	//assess kde py
	
}
