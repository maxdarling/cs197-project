#include "adaptive_hash.hh"


AdaptiveHashTable::AdaptiveHashTable(int distr, std::string specificHash):
	distrType(distr),
	preferredHash(specificHash){}


std::string AdaptiveHashTable::curr_hash_scheme() {
	return preferredHash;
}

std::string AdaptiveHashTable::curr_distr_type() {
	if (distrType == 1) {
		return "Grid";
	} else if (distrType == 0 ) {
		return "Sparse";
	} else {
		return "Dense";
	}
}

size_t AdaptiveHashTable::our_hash_function(char input) {
	return (int)input;
}

void AdaptiveHashTable::make_hash_table(std::string hash_string, std::vector<char>& hash_table){
	size_t index0 =(int)'a';
	for (size_t i = 0; i < hash_string.size(); i++) {
		size_t hash_index = our_hash_function(hash_string[i]);
		size_t vec_index = hash_index - index0;
		hash_table[vec_index] = hash_string[i];
	}
}
