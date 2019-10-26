#ifndef SPONGE_LIBSPONGE_ADAPTIVE_HASH_TABLE_HH
#define SPONGE LIBSPONGE_ADAPTIVE_HASH_TABLE_HH

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

class AdaptiveHashTable{
private:
	int distrType; //Dense = -1, Sparse = 0, Grid = 1
	std::string preferredHash;
	template<typename H, typename T>
	struct HashTable{
		std::unordered_map<H, T>hash_table{};
	};
	
public:
	AdaptiveHashTable(int distr, std::string specificHash); //return value must be hashtable
	std::string curr_distr_type();
	std::string curr_hash_scheme();
	size_t our_hash_function(char input);
	void make_hash_table(std::string hash_string, std::vector<char>& hash_table);
};

#endif
