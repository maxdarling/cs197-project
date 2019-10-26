#ifndef SPONGE_LIBSPONGE_ADAPTIVE_HASH_TABLE_HH
#define SPONGE LIBSPONGE_ADAPTIVE_HASH_TABLE_HH

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

//TODO: Think of all variations with resizing, key-value pairs, etc.
class AdaptiveHashTable{
private:
	int distrType; //Dense = -1, Sparse = 0, Grid = 1
	std::string preferredHash;
	std::string preferredFunc;
	std::unordered_set<std::string> valid_schemes{"LP", "QP"};
	std::unordered_set<std::string> valid_funcs{"M-S", "M-A-S"};
	size_t size_of_table;
	template<typename H>
	struct HashTable{
		size_t table_size;
		std::string hash_scheme;
		std::string hash_func;
		std::unordered_map<size_t, H>hash_table{};
	};
	
public:
	AdaptiveHashTable(int distr, std::string specificHash, std::string specificFunc, size_t my_size); //return value must be hashtable
	std::string curr_distr_type();
	std::string curr_hash_scheme();
	//add function for hash function
	size_t our_hash_function(char input);
	template<typename H>
	size_t hash_function(H input); //possible inputs: number, char/str, random struct
	void make_hash_table(std::string hash_string, std::vector<char>& hash_table);
};

#endif
