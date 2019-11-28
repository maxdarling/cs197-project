#ifndef ADAPTIVE_HASH_TABLE_HH
#define ADAPTIVE_HASH_TABLE_HH

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "Utils/Hash_Functions.hpp"
#include "Utils/LinearHashTable.hpp"
#include "Utils/QuadraticHashTable.hpp"
#include "Utils/ChainedHashTable.hpp"
//#include "Utils/RobinDistanceHashTable.hpp"
#include <math.h>

//TODO: Think of all variations with resizing, key-value pairs, etc.
class AdaptiveHashTable{
private:
	int distrType; //Dense = -1, Sparse = 0, Grid = 1
	std::string currHash;
	std::string currFunc;
	std::unordered_set<std::string> valid_schemes{"LP", "QP", "CH"};
	std::unordered_set<std::string> valid_funcs{"M-S", "M-A-S"};
	size_t lookup_ratio;
	size_t load_factor;
	size_t table_size;
	size_t table_capacity;
	size_t table_density; // sparse = 0 <= t_d <= 1 = dense
	size_t read_time;
	size_t write_time;
/*	template<typename H>
	struct HashTable{
		size_t table_size;
		std::string hash_scheme;
		std::string hash_func;
		std::unordered_map<size_t, H>hash_table;
	};*/
	
public:
	//AdaptiveHashTable(some data);
	AdaptiveHashTable(); 
	//get all the appropriate variables
	std::string get_hash_scheme() {return currFunc; };
	std::string get_hash_function() {return currHash; };
	size_t get_lookup_ratio() {return lookup_ratio; };
	size_t get_load_factor() {return load_factor; };
	size_t get_table_size() {return table_size; };
	size_t get_table_capacity() {return table_capacity; };
	size_t get_density() {return table_density; };
	size_t get_read_time() {return read_time; };
	size_t get_write_time() {return write_time; };
 	
	//update the appropriate variables
	void update_load_factor();
	void update_lookup_ratio();
	void update_density();
	void update_read_time();
	void update_write_time();

	//other important functions
	void make_hash_table();
	void print_out_hash_table();
	void insert_into_table();
	void remove_from_table();
	void read_from_table();
	void assess_switching();
};

#endif
