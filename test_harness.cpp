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
	AdaptiveHashTable test;
	std::cout << "Current Hash Scheme: " << test.get_hash_scheme() << std::endl;
	std::cout << "Current distribution type: " << test.get_density() << std::endl;
	test.make_hash_table();
}


