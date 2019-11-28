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

//Function credit: GeeksForGeeks - MOVE TO HASH FUNCTION H
void AdaptiveHashTable::make_hash_table(){
	if (currHash == "LP") {

	} else if (currHash == "QP") {

	} else if (currHash == "CH") {

	} else if (currHash == "RH") {
		
	} else {

	}

}
