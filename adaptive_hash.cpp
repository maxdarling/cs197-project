#include "adaptive_hash.hh"

AdaptiveHashTable::AdaptiveHashTable(int distr, std::string specificHash, std::string specificFunc, size_t my_size):
	distrType(distr),
	preferredHash(specificHash),
	preferredFunc(specificFunc),
	size_of_table(my_size){}

std::string AdaptiveHashTable::curr_hash_scheme() {
	return preferredHash;
}

std::string AdaptiveHashTable::curr_distr_type() {
	if (distrType == 1) return "Grid";
	else if (distrType == 0 ) return "Sparse";
	else return "Dense";
}

size_t AdaptiveHashTable::our_hash_function(char input) {
	return (int)input;
}

//Function credit: GeeksForGeeks - MOVE TO HASH FUNCTION H
unsigned int countBits (unsigned int number) {
	return (int)log2(number) + 1;
}

template<typename H, typename T>
void AdaptiveHashTable::get_hash_scheme(std::pair<T, H>kv_pair) { //uses key to insert into table
	(void)kv_pair;
} 

template<typename H>
size_t AdaptiveHashTable::get_hash_function(H input) { //generates key
	//if (preferredFunc == "M-S") multshift(input);
	return 0;
}

//template<typename H>
void AdaptiveHashTable::make_hash_table(std::string hash_string, std::vector<char>& hash_table){
	//get_hash_function<char>('a');
	//std::unordered_map<size_t, H>hashtable;
	//HashTable ht{hash_string.size(), preferredHash, preferredFunc, hashtable};
	size_t index0 =(int)'a';
	for (size_t i = 0; i < hash_string.size(); i++) {
		size_t hash_index = our_hash_function(hash_string[i]);
		size_t vec_index = hash_index - index0;
		hash_table[vec_index] = hash_string[i];
	}
	//return ht;
}
