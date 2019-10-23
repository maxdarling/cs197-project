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
	AdaptiveHashTable test{1, "Identity"};
	std::cout << "Current Hash Scheme: " << test.curr_hash_scheme() << std::endl;
	std::cout << "Current distribution type: " << test.curr_distr_type() << std::endl;
	std::string input_str = "abcdefgh";
	std::vector<char> our_hash_table(input_str.size(), '0');
	test.make_hash_table(input_str, our_hash_table);
	std::cout << "Here are the entries in your hash table: " << std::endl;
        for (size_t i = 0; i < our_hash_table.size(); i++) {
		std::cout << i << " -> " << our_hash_table[i] << std::endl;
	}	
}


