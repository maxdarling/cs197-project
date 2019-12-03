#include "adaptive_hash.hh"
#include <iostream>
#include <vector>

void create_simple_hash_table(); 

int main(int argc, char*argv[]) {
	//First set of tests
	create_simple_hash_table();
	return 0;
}

void create_simple_hash_table() { //assumes all input is valid
	AdaptiveHashTable test(20);
	//test.make_hash_table(1);
}


